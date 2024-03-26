/** @file globals.h
 *
 * @brief This file the global variables for the program
 *
 */

/* global variables */
/* define audio ports and midi ports */
extern jack_port_t **input_ports;
extern jack_port_t **output_ports;
extern jack_port_t *midi_input_port;
extern jack_port_t *midi_output_port;
extern jack_port_t *clock_input_port;
extern char **ports_to_connect;

/* define JACKD client : this is this program */
extern jack_client_t *client;

/* number of frames per Jack packet and sample-rate */
extern uint32_t nb_frames_per_packet, sample_rate;

/* define the structures for managing leds of midi control surface */
extern unsigned char list_buffer[LIST_ELT][4];	// list buffer of LIST_ELT led request
extern unsigned char list_index;			// index where to write led request to
extern unsigned char led_status [NB_TRACKS][LAST_ELT]; 	// this table will contain whether each light is on/off at a time; this is to avoid sending led requests which are not required

extern unsigned char bar_led_status [NB_BAR_ROWS][LAST_BAR_ELT]; 	// this table will contain whether each light is on/off at a time for a bar row; this is to avoid sending led requests which are not required
extern int number_of_bars;

/* BBT information */
/* warning ! in case of change in time signature, we will need to reset tick_counter as well */
extern int BBT_numerator;
extern int BBT_denominator;
extern unsigned int BBT_bar;
extern int BBT_beat;
extern int BBT_previous_beat;
extern int BBT_tick;
extern int BBT_wait_4_ticks;
extern int is_BBT; 	// OFF: no new bar, ON: new bar, PENDING_ON: (play event, change in sign time) means new bar 1 at the next clock event

extern int timesign;					// the value of this variable indicates which is the current time signature


/* define track structure for each track of the looper */
extern track_t track [];
/* define bar row structure */
extern bar_t bar [];

/* load & save globals */
extern int is_load;
extern int is_save;

/* PPBAR can vary from 96 to 99, depending on the attached midi clock device */
extern float ppbar;

