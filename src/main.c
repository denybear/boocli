/** @file main.c
 *
 * @brief This is the main file for the looper program. It uses the basic features of JACK.
 *
 */

#include "types.h"
#include "main.h"
#include "config.h"
#include "process.h"
#include "led.h"
#include "time.h"
#include "utils.h"
#include "disk.h"

// For testing purpose only
//#include <math.h>
//#define PI 3.14159265

/*************/
/* functions */
/*************/

static void init_globals ( )
{
	int i;

	/******************************/
	/* INIT SOME GLOBAL VARIABLES */
	/******************************/

	/* pulse per BAR is set to standard value, ie. 4 * 24 = 96 pulse per bar */
	ppbar = MIDI_CLOCK_RATE;
	/* time signature is 0, ie. 4/4 by default */
	timesign = _4_4;
	/* number of bars to be recorded (if any specified) */
	number_of_bars = 0;

	/* allocate memory to read parameter table: client/server input-output automated connection */
	/* let's make it 255 strings of 255 characters */
	ports_to_connect = calloc (255, sizeof(char*));
	for (i=0; i<255; i++) ports_to_connect[i] = calloc (255,sizeof(char));

	/* clear structure that will get control details, ie. track structure */
	for (i = 0; i<NB_TRACKS; i++) {
		memset (&track[i], 0, sizeof (track_t));
		/* set volume to 1 for each track */
		track [i].volume = 1.0f;

		/* for each track, create audio buffers and fill with 0 */
		/* we take the max size, plus add some more room (8192) to avoid overflows */
		if ((track [i].left = calloc (NB_SAMPLES + 8192, sizeof (jack_default_audio_sample_t))) == NULL) {
			fprintf ( stderr, "error in creating left audio buffer for track %d.\n",i);
			exit ( 1 );
		}
		if ((track [i].right = calloc (NB_SAMPLES + 8192, sizeof (jack_default_audio_sample_t))) == NULL) {
			fprintf ( stderr, "error in creating right audio buffer for track %d.\n",i);
			exit ( 1 );
		}
	}

	/* clear structure that will get control details for bar rows, ie. bar structure */
	for (i = 0; i<NB_BAR_ROWS; i++) {
		memset (&bar[i], 0, sizeof (bar_t));
	}

	/* clear load/save flags */
	is_load = FALSE;
	is_save = FALSE;

}


static void signal_handler ( int sig )
{
	jack_client_close ( client );
	fprintf ( stderr, "signal received, exiting ...\n" );
	exit ( 0 );
}


/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */
void jack_shutdown ( void *arg )
{
	free ( input_ports );
	free ( output_ports );
	free (midi_input_port);
	free (midi_output_port);
	free (clock_input_port);
	exit ( 1 );
}

/* usage: boocli (config_file) (jack client name) (jack server name)*/

int main ( int argc, char *argv[] )
{
	int i;
	const char *client_name;
	const char *server_name = NULL;
	char *config_name;
	jack_options_t options = JackNullOption;
	jack_status_t status;


	/* use basename of argv[0] */
	client_name = strrchr ( argv[0], '/' );
	if ( client_name == 0 ) client_name = argv[0];
	else client_name++;

	/* assign default name to boocli config file */
	config_name = calloc (20, sizeof(char));
	strcpy (config_name, "./boocli.cfg");

	if ( argc >=2 ) {
		free (config_name);
		config_name = argv [1];	// config file name specified
	}

	if ( argc >= 3 )        /* client name specified? */
	{
		client_name = argv[2];
		if ( argc >= 4 )    /* server name specified? */
		{
			server_name = argv[3];
			options |= JackServerName;
		}
	}

	/* open a client connection to the JACK server */

	client = jack_client_open ( client_name, options, &status, server_name );
	if ( client == NULL )
	{
		fprintf ( stderr, "jack_client_open() failed, status = 0x%2.0x.\n", status );
		if ( status & JackServerFailed )
		{
			fprintf ( stderr, "Unable to connect to JACK server.\n" );
		}
		exit ( 1 );
	}
	if ( status & JackServerStarted )
	{
		fprintf ( stderr, "JACK server started.\n" );
	}
	if ( status & JackNameNotUnique )
	{
		client_name = jack_get_client_name ( client );
		fprintf ( stderr, "unique name `%s' assigned.\n", client_name );
	}

	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/

	/* assign number of frames per packet and sample rate */
	sample_rate = jack_get_sample_rate(client);
	nb_frames_per_packet =  jack_get_buffer_size(client);

	/* set callback function to process jack events */
	jack_set_process_callback ( client, process, 0 );

	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/
	jack_on_shutdown ( client, jack_shutdown, 0 );

	/* register midi-in port: this port will get the midi keys notification */
	midi_input_port = jack_port_register (client, "midi_input_1", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
	if (midi_input_port == NULL ) {
		fprintf ( stderr, "no more JACK MIDI ports available.\n" );
		exit ( 1 );
	}

	/* register midi-out port: this port will send the midi notifications to light on/off pad leds */
	midi_output_port = jack_port_register (client, "midi_output_1", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
	if (midi_output_port == NULL ) {
		fprintf ( stderr, "no more JACK MIDI ports available.\n" );
		exit ( 1 );
	}

	/* register clock-input port: this port will get the midi clock notification */
	clock_input_port = jack_port_register (client, "clock_input_1", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
	if (clock_input_port == NULL ) {
		fprintf ( stderr, "no more JACK MIDI ports available.\n" );
		exit ( 1 );
	}

	/* create two audio ports pairs */
	input_ports = ( jack_port_t** ) calloc ( 2, sizeof ( jack_port_t* ) );
	output_ports = ( jack_port_t** ) calloc ( 2, sizeof ( jack_port_t* ) );

	/* register 2 ports (Left, Right) as audio input and audio output */
	char port_name[16];
	for ( i = 0; i < 2; i++ )
	{
		sprintf ( port_name, "input_%d", i + 1 );
		input_ports[i] = jack_port_register ( client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0 );
		sprintf ( port_name, "output_%d", i + 1 );
		output_ports[i] = jack_port_register ( client, port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );
		if ( ( input_ports[i] == NULL ) || ( output_ports[i] == NULL ) )
		{
			fprintf ( stderr, "no more JACK ports available.\n" );
			exit ( 1 );
		}
	}

	/* Tell the JACK server that we are ready to roll.  Our
	 * process() callback will start running now. */

	if ( jack_activate ( client ) )
	{
		fprintf ( stderr, "cannot activate client.\n" );
		exit ( 1 );
	}

	/* init global variables */
	init_globals();


	/**************/
	/* MAIN START */
	/**************/

	/* read config file to get all the parameters */
	if (read_config (config_name)==EXIT_FAILURE) {
		fprintf ( stderr, "error in reading config file.\n" );
		exit ( 1 );
	}

	/* Connect the ports.  You can't do this before the client is
	 * activated, because we can't make connections to clients
	 * that aren't running.  Note the confusing (but necessary)
	 * orientation of the driver backend ports: playback ports are
	 * "input" to the backend, and capture ports are "output" from
	 * it.
	 */

	/* go through the list of ports to be connected and connect them by pair (server, client) */
	i=0;
	fprintf (stderr, "attempt to connect input-output ports together.\n");
	while ((ports_to_connect[i][0]!='\x0') && (ports_to_connect[i+1][0]!='\x0')) {
		fprintf (stderr, "server: %s , client: %s\n", ports_to_connect[i], ports_to_connect[i+1]);
		if ( jack_connect ( client, ports_to_connect [i], ports_to_connect [i+1]) ) {
			fprintf ( stderr, "cannot connect ports (between client and server).\n" );
//			exit (1);
		}
		/* increment index of 2 position to move to next (server, client) port couple */
		i+=2;
	}

	/* clear memory allocated to connect client/server input-output */
	for (i=0; i<255; i++) free (ports_to_connect[i]);
	free (ports_to_connect);



	/* install a signal handler to properly quits jack client */
#ifdef WIN32
	signal ( SIGINT, signal_handler );
	signal ( SIGABRT, signal_handler );
	signal ( SIGTERM, signal_handler );
#else
	signal ( SIGQUIT, signal_handler );
	signal ( SIGTERM, signal_handler );
	signal ( SIGHUP, signal_handler );
	signal ( SIGINT, signal_handler );
#endif


	/* switch all leds off for all the tracks */
	for (i = 0; i<NB_TRACKS; i++) {
		/* set structure that contains status for each pad led to ON : this is to force all leds off */
		memset (&led_status[i][0], ON, LAST_ELT);

		led_off (i);

		/* set structure that contains status for each pad led to OFF : this is useless: done by led() function, but you never know */
		memset (&led_status[i][0], OFF, LAST_ELT);
		// track volume set to 1.0 by default
		led (i, VOLUP, ON);
	}

	for (i = 0; i<NB_BAR_ROWS; i++) {
		/* set structure that contains status for each pad led to ON : this is to force all leds off */
		memset (&bar_led_status[i][0], ON, LAST_BAR_ELT);
		bar_led_off (i);

		/* set structure that contains status for each pad led to OFF : this is useless: done by led() function, but you never know */
		memset (&bar_led_status[i][0], OFF, LAST_BAR_ELT);
	}


	/* this code is for testing purpose only */
	/* it injects a sin wave in the record buffer of 1st track */
	/* sin wave duration is 4 beats, ie. 4 sec at 60 BPM, ie. 4 x 48000 samples, ie. 4 x 750 x 64 samples for 1 bar */
	/* code is used to make sure plays and bar are synchronized, as my test midi clock had a lot of drifting */
/*
	int j;
	for (i=0; i<(4*750); i++) {
		for (j=0;j<64; j++) {
			// sin wave
//			track[0].left[(64*i)+j] = (jack_default_audio_sample_t) sin (j*2*PI/64);
//			track[0].right[(64*i)+j] = (jack_default_audio_sample_t) sin (j*2*PI/64);
			// square wave
			track[0].left[(64*i)+j] = (jack_default_audio_sample_t) (1-(((int) (j/32))*2));
			track[0].right[(64*i)+j] = (jack_default_audio_sample_t) (1-(((int) (j/32))*2));
		}
	}
	track[0].end_index_left = 4*750*64;
	track[0].end_index_right =  4*750*64;
	track[0].end_bar_left = 1;
	track[0].end_bar_right = 1;
	track[0].record_bar_left = 0;
	track[0].record_bar_right = 0;
*/

	/* keep running until the transport stops */

	while (1)
	{
		// load pad has been pressed
		if (is_load) {

			load ();
			is_load = FALSE;

			// reset status of all the tracks, to have a fresh start
			for (i = 0; i < NB_TRACKS; i++) {

				// reset track status
				reset_status (&track[i]);
				// reset volume to max
				track[i].volume = 1.0f;
				// switch all leds off for the track
				led_off (i);
				// track volume set to 1.0 by default
				led (i, VOLUP, ON);

			}

			// load led off
			led (0, LOAD, OFF);
		}

		// save pad has been pressed
		if (is_save) {

			save ();
			is_save = FALSE;

			// save led off
			led (0, SAVE, OFF);
		}

#ifdef WIN32
		Sleep ( 1000 );
#else
		sleep ( 1 );
#endif
	}

	jack_client_close ( client );
	exit ( 0 );
}
