/** @file config.c
 *
 * @brief The config module reads looper config file to set key parameters such as:
 * - automated connection of the client to JACK ports
 * - control surface MIDI messages (connected to MIDI in)
 *
 */

#include "types.h"
#include "globals.h"
#include "config.h"
#include "process.h"
#include "led.h"
#include "time.h"


/* This example reads the configuration file 'example.cfg' and displays
 * some of its contents.
 */

int read_config (char *name)
{
	config_t cfg;
	config_setting_t *setting;
	const char *str;
	int index;
	int midi_byte1[8], midi_byte2[8];
	int i,j;
	config_setting_t *buffer;

	config_init(&cfg);

	/* Read the file. If there is an error, report it and exit. */
	if(! config_read_file(&cfg,name))
	{
		fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
		config_destroy(&cfg);
		return(EXIT_FAILURE);
	}

	/* Read a dummy name; this is mostly to remember how to read a single config parameter */
	if(!config_lookup_string(&cfg, "name", &str)) fprintf ( stderr, "Unable to read config name.\n" );

	/****************************************************************************/
	/* Read connection settings : connection of server port X to client port Y  */
	/****************************************************************************/

	index = 0;

	/* audio inputs */
	setting = config_lookup(&cfg, "connections.input");
	if(setting != NULL)
	{
		int count = config_setting_length(setting);

		for(i = 0; i < count; ++i)
		{
			config_setting_t *book = config_setting_get_elem(setting, i);

			/* Only output the record if all of the expected fields are present. */
			const char *port_server, *port_client;

			if(!(config_setting_lookup_string(book, "server", &port_server)
					 && config_setting_lookup_string(book, "client", &port_client)))
				continue;

			/* copy the ports found in config file to an array of string, and increment the index in the table */
			/* for inputs, jack port is the input and shall be first in the array */
			strcpy (ports_to_connect [index++], port_server);
			strcpy (ports_to_connect [index++], port_client);
		}
	}

	/* audio outputs */
	setting = config_lookup(&cfg, "connections.output");
	if(setting != NULL)
	{
		int count = config_setting_length(setting);

		for(i = 0; i < count; ++i)
		{
			config_setting_t *book = config_setting_get_elem(setting, i);

			/* Only output the record if all of the expected fields are present. */
			const char *port_server, *port_client;

			if(!(config_setting_lookup_string(book, "server", &port_server)
					 && config_setting_lookup_string(book, "client", &port_client)))
				continue;

			/* copy the ports found in config file to an array of string, and increment the index in the table */
			/* for outputs, jack port is the output (destination) and shall be second in the array */
			/* "server" should be the actual looper client */
			strcpy (ports_to_connect [index++], port_server);
			strcpy (ports_to_connect [index++], port_client);
		}
	}

	/* midi clock inputs */
	setting = config_lookup(&cfg, "connections.midi_clock");
	if(setting != NULL)
	{
		int count = config_setting_length(setting);

		for(i = 0; i < count; ++i)
		{
			config_setting_t *book = config_setting_get_elem(setting, i);

			/* Only output the record if all of the expected fields are present. */
			const char *port_server, *port_client;

			if(!(config_setting_lookup_string(book, "server", &port_server)
					 && config_setting_lookup_string(book, "client", &port_client)))
				continue;

			/* copy the ports found in config file to an array of string, and increment the index in the table */
			/* for inputs, jack port is the input and shall be first in the array */
			strcpy (ports_to_connect [index++], port_server);
			strcpy (ports_to_connect [index++], port_client);
		}
	}

	/* midi inputs */
	setting = config_lookup(&cfg, "connections.midi_input");
	if(setting != NULL)
	{
		int count = config_setting_length(setting);

		for(i = 0; i < count; ++i)
		{
			config_setting_t *book = config_setting_get_elem(setting, i);

			/* Only output the record if all of the expected fields are present. */
			const char *port_server, *port_client;

			if(!(config_setting_lookup_string(book, "server", &port_server)
					 && config_setting_lookup_string(book, "client", &port_client)))
				continue;

			/* copy the ports found in config file to an array of string, and increment the index in the table */
			/* for inputs, jack port is the input and shall be first in the array */
			strcpy (ports_to_connect [index++], port_server);
			strcpy (ports_to_connect [index++], port_client);
		}
	}

	/* midi outputs */
	setting = config_lookup(&cfg, "connections.midi_output");
	if(setting != NULL)
	{
		int count = config_setting_length(setting);

		for(i = 0; i < count; ++i)
		{
			config_setting_t *book = config_setting_get_elem(setting, i);

			/* Only output the record if all of the expected fields are present. */
			const char *port_server, *port_client;

			if(!(config_setting_lookup_string(book, "server", &port_server)
					 && config_setting_lookup_string(book, "client", &port_client)))
				continue;

			/* copy the ports found in config file to an array of string, and increment the index in the table */
			/* for inputs, jack port is the input and shall be first in the array */
			strcpy (ports_to_connect [index++], port_server);
			strcpy (ports_to_connect [index++], port_client);
		}
	}


	/*****************************************************************************************************/
	/* Read control settings : assign midi events to control each function of the looper, for each track */
	/*****************************************************************************************************/

	index = 0;


	/* control inputs */
	setting = config_lookup(&cfg, "controls.tracks");
	if (setting != NULL)
	{
		int count = config_setting_length(setting);

		/* Check we don't have a too large number of tracks defined, in which case we set to the maximum */
		if (count > NB_TRACKS) count = NB_TRACKS;

		 /* read element by element */
		for (i = 0; i < count; ++i)
		{
			config_setting_t *book = config_setting_get_elem (setting, i);

			/* read member by member */
			/* time signature */
			buffer = config_setting_get_member (book, "time");
			/* check buffer is not empty, and has 2 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=2) continue;
			track[i].ctrl[TIMESIGN][0] = config_setting_get_int_elem (buffer, 0);
			track[i].ctrl[TIMESIGN][1] = config_setting_get_int_elem (buffer, 1);

			/* play */
			buffer = config_setting_get_member (book, "play");
			/* check buffer is not empty, and has 2 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=2) continue;
			track[i].ctrl[PLAY][0] = config_setting_get_int_elem (buffer, 0);
			track[i].ctrl[PLAY][1] = config_setting_get_int_elem (buffer, 1);

			/* record */
			buffer = config_setting_get_member (book, "record");
			/* check buffer is not empty, and has 2 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=2) continue;
			track[i].ctrl[RECORD][0] = config_setting_get_int_elem (buffer, 0);
			track[i].ctrl[RECORD][1] = config_setting_get_int_elem (buffer, 1);

			/* mute */
			buffer = config_setting_get_member (book, "mute");
			/* check buffer is not empty, and has 2 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=2) continue;
			track[i].ctrl[MUTE][0] = config_setting_get_int_elem (buffer, 0);
			track[i].ctrl[MUTE][1] = config_setting_get_int_elem (buffer, 1);

			/* solo */
			buffer = config_setting_get_member (book, "solo");
			/* check buffer is not empty, and has 2 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=2) continue;
			track[i].ctrl[SOLO][0] = config_setting_get_int_elem (buffer, 0);
			track[i].ctrl[SOLO][1] = config_setting_get_int_elem (buffer, 1);

			/* volup */
			buffer = config_setting_get_member (book, "volup");
			/* check buffer is not empty, and has 2 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=2) continue;
			track[i].ctrl[VOLUP][0] = config_setting_get_int_elem (buffer, 0);
			track[i].ctrl[VOLUP][1] = config_setting_get_int_elem (buffer, 1);

			/* voldown */
			buffer = config_setting_get_member (book, "voldown");
			/* check buffer is not empty, and has 2 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=2) continue;
			track[i].ctrl[VOLDOWN][0] = config_setting_get_int_elem (buffer, 0);
			track[i].ctrl[VOLDOWN][1] = config_setting_get_int_elem (buffer, 1);

			/* mode */
			buffer = config_setting_get_member (book, "mode");
			/* check buffer is not empty, and has 2 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=2) continue;
			track[i].ctrl[MODE][0] = config_setting_get_int_elem (buffer, 0);
			track[i].ctrl[MODE][1] = config_setting_get_int_elem (buffer, 1);

			/* delete */
			buffer = config_setting_get_member (book, "delete");
			/* check buffer is not empty, and has 2 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=2) continue;
			track[i].ctrl[DELETE][0] = config_setting_get_int_elem (buffer, 0);
			track[i].ctrl[DELETE][1] = config_setting_get_int_elem (buffer, 1);
		}
	}

	/* leds on */
	setting = config_lookup(&cfg, "controls.led_on");
	if (setting != NULL)
	{
		int count = config_setting_length(setting);

		/* Check we don't have a too large number of tracks defined, in which case we set to the maximum */
		if (count > NB_TRACKS) count = NB_TRACKS;

		 /* read element by element */
		for (i = 0; i < count; ++i)
		{
			config_setting_t *book = config_setting_get_elem (setting, i);

			/* read member by member */
			/* time signature */
			buffer = config_setting_get_member (book, "time");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[TIMESIGN][ON][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[TIMESIGN][ON][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[TIMESIGN][ON][2] = config_setting_get_int_elem (buffer, 2);

			/* play */
			buffer = config_setting_get_member (book, "play");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[PLAY][ON][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[PLAY][ON][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[PLAY][ON][2] = config_setting_get_int_elem (buffer, 2);

			/* record */
			buffer = config_setting_get_member (book, "record");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[RECORD][ON][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[RECORD][ON][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[RECORD][ON][2] = config_setting_get_int_elem (buffer, 2);

			/* mute */
			buffer = config_setting_get_member (book, "mute");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[MUTE][ON][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[MUTE][ON][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[MUTE][ON][2] = config_setting_get_int_elem (buffer, 2);

			/* solo */
			buffer = config_setting_get_member (book, "solo");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[SOLO][ON][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[SOLO][ON][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[SOLO][ON][2] = config_setting_get_int_elem (buffer, 2);

			/* volup */
			buffer = config_setting_get_member (book, "volup");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[VOLUP][ON][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[VOLUP][ON][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[VOLUP][ON][2] = config_setting_get_int_elem (buffer, 2);

			/* voldown */
			buffer = config_setting_get_member (book, "voldown");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[VOLDOWN][ON][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[VOLDOWN][ON][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[VOLDOWN][ON][2] = config_setting_get_int_elem (buffer, 2);

			/* mode */
			buffer = config_setting_get_member (book, "mode");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[MODE][ON][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[MODE][ON][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[MODE][ON][2] = config_setting_get_int_elem (buffer, 2);

			/* delete */
			buffer = config_setting_get_member (book, "delete");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[DELETE][ON][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[DELETE][ON][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[DELETE][ON][2] = config_setting_get_int_elem (buffer, 2);
		}
	}

	/* leds pending on */
	setting = config_lookup(&cfg, "controls.led_pending_on");
	if (setting != NULL)
	{
		int count = config_setting_length(setting);

		/* Check we don't have a too large number of tracks defined, in which case we set to the maximum */
		if (count > NB_TRACKS) count = NB_TRACKS;

		 /* read element by element */
		for (i = 0; i < count; ++i)
		{
			config_setting_t *book = config_setting_get_elem (setting, i);

			/* read member by member */
			/* time signature */
			buffer = config_setting_get_member (book, "time");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[TIMESIGN][PENDING_ON][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[TIMESIGN][PENDING_ON][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[TIMESIGN][PENDING_ON][2] = config_setting_get_int_elem (buffer, 2);

			/* play */
			buffer = config_setting_get_member (book, "play");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[PLAY][PENDING_ON][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[PLAY][PENDING_ON][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[PLAY][PENDING_ON][2] = config_setting_get_int_elem (buffer, 2);

			/* record */
			buffer = config_setting_get_member (book, "record");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[RECORD][PENDING_ON][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[RECORD][PENDING_ON][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[RECORD][PENDING_ON][2] = config_setting_get_int_elem (buffer, 2);

			/* volup */
			buffer = config_setting_get_member (book, "volup");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[VOLUP][PENDING_ON][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[VOLUP][PENDING_ON][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[VOLUP][PENDING_ON][2] = config_setting_get_int_elem (buffer, 2);

			/* voldown */
			buffer = config_setting_get_member (book, "voldown");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[VOLDOWN][PENDING_ON][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[VOLDOWN][PENDING_ON][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[VOLDOWN][PENDING_ON][2] = config_setting_get_int_elem (buffer, 2);

			/* delete */
			buffer = config_setting_get_member (book, "delete");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[DELETE][PENDING_ON][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[DELETE][PENDING_ON][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[DELETE][PENDING_ON][2] = config_setting_get_int_elem (buffer, 2);
		}
	}

	/* leds pending off */
	setting = config_lookup(&cfg, "controls.led_pending_off");
	if (setting != NULL)
	{
		int count = config_setting_length(setting);

		/* Check we don't have a too large number of tracks defined, in which case we set to the maximum */
		if (count > NB_TRACKS) count = NB_TRACKS;

		 /* read element by element */
		for (i = 0; i < count; ++i)
		{
			config_setting_t *book = config_setting_get_elem (setting, i);

			/* read member by member */
			/* time signature */
			buffer = config_setting_get_member (book, "time");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[TIMESIGN][PENDING_OFF][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[TIMESIGN][PENDING_OFF][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[TIMESIGN][PENDING_OFF][2] = config_setting_get_int_elem (buffer, 2);

			/* play */
			buffer = config_setting_get_member (book, "play");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[PLAY][PENDING_OFF][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[PLAY][PENDING_OFF][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[PLAY][PENDING_OFF][2] = config_setting_get_int_elem (buffer, 2);

			/* record */
			buffer = config_setting_get_member (book, "record");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[RECORD][PENDING_OFF][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[RECORD][PENDING_OFF][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[RECORD][PENDING_OFF][2] = config_setting_get_int_elem (buffer, 2);

			/* volup */
			buffer = config_setting_get_member (book, "volup");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[VOLUP][PENDING_OFF][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[VOLUP][PENDING_OFF][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[VOLUP][PENDING_OFF][2] = config_setting_get_int_elem (buffer, 2);

			/* voldown */
			buffer = config_setting_get_member (book, "voldown");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[VOLDOWN][PENDING_OFF][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[VOLDOWN][PENDING_OFF][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[VOLDOWN][PENDING_OFF][2] = config_setting_get_int_elem (buffer, 2);

			/* delete */
			buffer = config_setting_get_member (book, "delete");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[DELETE][PENDING_OFF][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[DELETE][PENDING_OFF][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[DELETE][PENDING_OFF][2] = config_setting_get_int_elem (buffer, 2);
		}
	}

	/* leds off */
	setting = config_lookup(&cfg, "controls.led_off");
	if (setting != NULL)
	{
		int count = config_setting_length(setting);

		/* Check we don't have a too large number of tracks defined, in which case we set to the maximum */
		if (count > NB_TRACKS) count = NB_TRACKS;

		 /* read element by element */
		for (i = 0; i < count; ++i)
		{
			config_setting_t *book = config_setting_get_elem (setting, i);

			/* read member by member */
			/* time signature */
			buffer = config_setting_get_member (book, "time");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[TIMESIGN][OFF][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[TIMESIGN][OFF][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[TIMESIGN][OFF][2] = config_setting_get_int_elem (buffer, 2);

			/* play */
			buffer = config_setting_get_member (book, "play");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[PLAY][OFF][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[PLAY][OFF][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[PLAY][OFF][2] = config_setting_get_int_elem (buffer, 2);

			/* record */
			buffer = config_setting_get_member (book, "record");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[RECORD][OFF][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[RECORD][OFF][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[RECORD][OFF][2] = config_setting_get_int_elem (buffer, 2);

			/* mute */
			buffer = config_setting_get_member (book, "mute");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[MUTE][OFF][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[MUTE][OFF][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[MUTE][OFF][2] = config_setting_get_int_elem (buffer, 2);

			/* solo */
			buffer = config_setting_get_member (book, "solo");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[SOLO][OFF][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[SOLO][OFF][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[SOLO][OFF][2] = config_setting_get_int_elem (buffer, 2);

			/* volup */
			buffer = config_setting_get_member (book, "volup");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[VOLUP][OFF][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[VOLUP][OFF][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[VOLUP][OFF][2] = config_setting_get_int_elem (buffer, 2);

			/* voldown */
			buffer = config_setting_get_member (book, "voldown");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[VOLDOWN][OFF][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[VOLDOWN][OFF][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[VOLDOWN][OFF][2] = config_setting_get_int_elem (buffer, 2);

			/* mode */
			buffer = config_setting_get_member (book, "mode");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[MODE][OFF][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[MODE][OFF][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[MODE][OFF][2] = config_setting_get_int_elem (buffer, 2);

			/* delete */
			buffer = config_setting_get_member (book, "delete");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			track[i].led[DELETE][OFF][0] = config_setting_get_int_elem (buffer, 0);
			track[i].led[DELETE][OFF][1] = config_setting_get_int_elem (buffer, 1);
			track[i].led[DELETE][OFF][2] = config_setting_get_int_elem (buffer, 2);
		}
	}

	/***********************************************************************************/
	/* Read bar settings : assign midi events to control number of bars to be recorded */
	/***********************************************************************************/

	index = 0;


	/* control inputs */
	setting = config_lookup(&cfg, "bars.rows");
	if (setting != NULL)
	{
		int count = config_setting_length(setting);

		/* Check we don't have a too large number of rows defined, in which case we set to the maximum */
		if (count > NB_BAR_ROWS) count = NB_BAR_ROWS;

		 /* read element by element */
		for (i = 0; i < count; ++i)
		{
			config_setting_t *book = config_setting_get_elem (setting, i);

			/* read member by member */
			/* bar1 */
			buffer = config_setting_get_member (book, "bar1");
			/* check buffer is not empty, and has 2 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=2) continue;
			bar[i].ctrl[0][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].ctrl[0][1] = config_setting_get_int_elem (buffer, 1);

			/* bar2*/
			buffer = config_setting_get_member (book, "bar2");
			/* check buffer is not empty, and has 2 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=2) continue;
			bar[i].ctrl[1][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].ctrl[1][1] = config_setting_get_int_elem (buffer, 1);

			/* bar3 */
			buffer = config_setting_get_member (book, "bar3");
			/* check buffer is not empty, and has 2 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=2) continue;
			bar[i].ctrl[2][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].ctrl[2][1] = config_setting_get_int_elem (buffer, 1);

			/* bar4*/
			buffer = config_setting_get_member (book, "bar4");
			/* check buffer is not empty, and has 2 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=2) continue;
			bar[i].ctrl[3][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].ctrl[3][1] = config_setting_get_int_elem (buffer, 1);

			/* bar5 */
			buffer = config_setting_get_member (book, "bar5");
			/* check buffer is not empty, and has 2 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=2) continue;
			bar[i].ctrl[4][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].ctrl[4][1] = config_setting_get_int_elem (buffer, 1);

			/* bar6*/
			buffer = config_setting_get_member (book, "bar6");
			/* check buffer is not empty, and has 2 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=2) continue;
			bar[i].ctrl[5][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].ctrl[5][1] = config_setting_get_int_elem (buffer, 1);

			/* bar7 */
			buffer = config_setting_get_member (book, "bar7");
			/* check buffer is not empty, and has 2 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=2) continue;
			bar[i].ctrl[6][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].ctrl[6][1] = config_setting_get_int_elem (buffer, 1);

			/* bar8*/
			buffer = config_setting_get_member (book, "bar8");
			/* check buffer is not empty, and has 2 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=2) continue;
			bar[i].ctrl[7][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].ctrl[7][1] = config_setting_get_int_elem (buffer, 1);

		}
	}

	/* leds on */
	setting = config_lookup(&cfg, "bars.led_on");
	if (setting != NULL)
	{
		int count = config_setting_length(setting);

		/* Check we don't have a too large number of rows defined, in which case we set to the maximum */
		if (count > NB_BAR_ROWS) count = NB_BAR_ROWS;

		 /* read element by element */
		for (i = 0; i < count; ++i)
		{
			config_setting_t *book = config_setting_get_elem (setting, i);

			/* read member by member */
			/* bar1 */
			buffer = config_setting_get_member (book, "bar1");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			bar[i].led[0][ON][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].led[0][ON][1] = config_setting_get_int_elem (buffer, 1);
			bar[i].led[0][ON][2] = config_setting_get_int_elem (buffer, 2);

			/* bar2 */
			buffer = config_setting_get_member (book, "bar2");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			bar[i].led[1][ON][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].led[1][ON][1] = config_setting_get_int_elem (buffer, 1);
			bar[i].led[1][ON][2] = config_setting_get_int_elem (buffer, 2);

			/* bar3 */
			buffer = config_setting_get_member (book, "bar3");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			bar[i].led[2][ON][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].led[2][ON][1] = config_setting_get_int_elem (buffer, 1);
			bar[i].led[2][ON][2] = config_setting_get_int_elem (buffer, 2);

			/* bar4 */
			buffer = config_setting_get_member (book, "bar4");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			bar[i].led[3][ON][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].led[3][ON][1] = config_setting_get_int_elem (buffer, 1);
			bar[i].led[3][ON][2] = config_setting_get_int_elem (buffer, 2);

			/* bar5 */
			buffer = config_setting_get_member (book, "bar5");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			bar[i].led[4][ON][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].led[4][ON][1] = config_setting_get_int_elem (buffer, 1);
			bar[i].led[4][ON][2] = config_setting_get_int_elem (buffer, 2);

			/* bar6 */
			buffer = config_setting_get_member (book, "bar6");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			bar[i].led[5][ON][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].led[5][ON][1] = config_setting_get_int_elem (buffer, 1);
			bar[i].led[5][ON][2] = config_setting_get_int_elem (buffer, 2);

			/* bar7 */
			buffer = config_setting_get_member (book, "bar7");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			bar[i].led[6][ON][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].led[6][ON][1] = config_setting_get_int_elem (buffer, 1);
			bar[i].led[6][ON][2] = config_setting_get_int_elem (buffer, 2);

			/* bar8 */
			buffer = config_setting_get_member (book, "bar8");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			bar[i].led[7][ON][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].led[7][ON][1] = config_setting_get_int_elem (buffer, 1);
			bar[i].led[7][ON][2] = config_setting_get_int_elem (buffer, 2);

		}
	}

	/* leds off */
	setting = config_lookup(&cfg, "bars.led_off");
	if (setting != NULL)
	{
		int count = config_setting_length(setting);

		/* Check we don't have a too large number of rows defined, in which case we set to the maximum */
		if (count > NB_BAR_ROWS) count = NB_BAR_ROWS;

		 /* read element by element */
		for (i = 0; i < count; ++i)
		{
			config_setting_t *book = config_setting_get_elem (setting, i);

			/* read member by member */
			/* bar1 */
			buffer = config_setting_get_member (book, "bar1");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			bar[i].led[0][OFF][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].led[0][OFF][1] = config_setting_get_int_elem (buffer, 1);
			bar[i].led[0][OFF][2] = config_setting_get_int_elem (buffer, 2);

			/* bar2 */
			buffer = config_setting_get_member (book, "bar2");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			bar[i].led[1][OFF][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].led[1][OFF][1] = config_setting_get_int_elem (buffer, 1);
			bar[i].led[1][OFF][2] = config_setting_get_int_elem (buffer, 2);

			/* bar3 */
			buffer = config_setting_get_member (book, "bar3");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			bar[i].led[2][OFF][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].led[2][OFF][1] = config_setting_get_int_elem (buffer, 1);
			bar[i].led[2][OFF][2] = config_setting_get_int_elem (buffer, 2);

			/* bar4 */
			buffer = config_setting_get_member (book, "bar4");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			bar[i].led[3][OFF][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].led[3][OFF][1] = config_setting_get_int_elem (buffer, 1);
			bar[i].led[3][OFF][2] = config_setting_get_int_elem (buffer, 2);

			/* bar5 */
			buffer = config_setting_get_member (book, "bar5");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			bar[i].led[4][OFF][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].led[4][OFF][1] = config_setting_get_int_elem (buffer, 1);
			bar[i].led[4][OFF][2] = config_setting_get_int_elem (buffer, 2);

			/* bar6 */
			buffer = config_setting_get_member (book, "bar6");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			bar[i].led[5][OFF][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].led[5][OFF][1] = config_setting_get_int_elem (buffer, 1);
			bar[i].led[5][OFF][2] = config_setting_get_int_elem (buffer, 2);

			/* bar7 */
			buffer = config_setting_get_member (book, "bar7");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			bar[i].led[6][OFF][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].led[6][OFF][1] = config_setting_get_int_elem (buffer, 1);
			bar[i].led[6][OFF][2] = config_setting_get_int_elem (buffer, 2);

			/* bar8 */
			buffer = config_setting_get_member (book, "bar8");
			/* check buffer is not empty, and has 3 elements */
			if (!buffer) continue;
			if (config_setting_length(buffer)!=3) continue;
			bar[i].led[7][OFF][0] = config_setting_get_int_elem (buffer, 0);
			bar[i].led[7][OFF][1] = config_setting_get_int_elem (buffer, 1);
			bar[i].led[7][OFF][2] = config_setting_get_int_elem (buffer, 2);

		}
	}

	/* successful reading, exit */
	config_destroy(&cfg);
	return(EXIT_SUCCESS);
}
