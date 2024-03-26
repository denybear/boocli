/** @file main.h
 *
 * @brief This file contains global variables definition
 *
 */


/********************/
/* global variables */
/********************/

/* define audio ports and midi ports */
jack_port_t **input_ports;
jack_port_t **output_ports;
jack_port_t *midi_input_port;
jack_port_t *midi_output_port;
jack_port_t *clock_input_port;
char **ports_to_connect;

/* define JACKD client : this is this program */
jack_client_t *client;

/* number of frames per Jack packet and sample-rate */
uint32_t nb_frames_per_packet, sample_rate;

/* define the structures for managing leds of midi control surface */
unsigned char list_buffer[LIST_ELT][4];	// list buffer of LIST_ELT led request
unsigned char list_index = 0;			// index where to write led request to
unsigned char led_status [NB_TRACKS][LAST_ELT]; 	// this table will contain whether each light is on/off at a time; this is to avoid sending led requests which are not required

unsigned char bar_led_status [NB_BAR_ROWS][LAST_BAR_ELT]; 	// this table will contain whether each light is on/off at a time for a bar row; this is to avoid sending led requests which are not required
int number_of_bars;		// this value indicates the number of bars to be recorded (when specified and not 0

/* BBT information */
int BBT_numerator=4;        // 4/4 bar time signature at start
int BBT_denominator=4;
unsigned int BBT_bar=1;		// set bar number as 1
int BBT_beat;
int	BBT_previous_beat;
int BBT_tick;
int BBT_wait_4_ticks = 8;		// purpose of this is to delay switch on/off of led of about 4 clock ticks, so hardware can support it
int is_BBT=PENDING_ON; 	// OFF: no new bar, ON: new bar, PENDING_ON: (play event, change in sign time) means new bar 1 at the next clock event

int timesign;			// the value of this variable indicates which is the current time signature

/* define track structure for each track of the looper */
track_t track [NB_TRACKS];
/* define bar row structure */
bar_t bar [NB_BAR_ROWS];

/* load & save globals */
int is_load;
int is_save;

/* PPBAR can vary from 96 to 99, depending on the attached midi clock device */
float ppbar;

