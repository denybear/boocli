/** @file process.c
 *
 * @brief The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 */

#include "types.h"
#include "globals.h"
#include "config.h"
#include "process.h"
#include "led.h"
#include "time.h"
#include "utils.h"



// main process callback called at capture of (nframes) frames/samples
int process ( jack_nframes_t nframes, void *arg )
{
	jack_nframes_t h;
	int i,j,k;
	int mute = OFF;
	void *midiin;
	void *midiclock;
	void *midiout;
	jack_nframes_t nframes_half;			// anti-crack variables
	jack_default_audio_sample_t sample;		// anti-crack variables
	jack_default_audio_sample_t *in, *out;
	jack_midi_event_t *clock_event, *in_event;
	jack_midi_data_t buffer[5];				// midi out buffer for lighting the pad leds
	int dest, tracknum, type, on_off;		// variables used to manage lighting of the pad leds


	/***********************************************/
	/* set recording and playing values by default */
	/* these values may be altered by incoming     */
	/* MIDI in events which are processed after    */
	/* this is why it is important to put this     */
	/* init here, so init values can be overwritten*/
	/* later on in the processing                  */
	/***********************************************/

	// used later for audio-crack removal
//	nframes_half = nframes / 16;
	nframes_half = 8;


	/****************************************/
	/* First, process MIDI and CLOCK events */
	/****************************************/

	// allocate structure that will receive MIDI events
	clock_event = calloc (1, sizeof (jack_midi_event_t));
	in_event = calloc (1, sizeof (jack_midi_event_t));

	// Get midi clock midi out and midi in buffers
	midiclock = jack_port_get_buffer(midi_clock_port, nframes);
	midiin = jack_port_get_buffer(midi_input_port, nframes);

	// process MIDI IN events
	for (i=0; i< jack_midi_get_event_count(midiin); i++) {
		if (jack_midi_event_get (in_event, midiin, i) != 0) {
			fprintf ( stderr, "Missed in event\n" );
			continue;
		}
		// call processing function
		midi_in_process (in_event,nframes);
	}

	// process MIDI CLOCK events
	for (i=0; i< jack_midi_get_event_count(midiclock); i++) {
		if (jack_midi_event_get (clock_event, midiclock, i) != 0) {
			fprintf ( stderr, "Missed clock event\n" );
			continue;
		}
		// call processing function
		midi_clock_process (clock_event,nframes);
	}


	/****************************************/
	/* Second, process MIDI out (UI) events */
	/****************************************/

	// define midi out port to write to
	midiout = jack_port_get_buffer (midi_output_port, nframes);

	i = 0;

	// clear midi write buffer
	jack_midi_clear_buffer (midiout);

	//go through the list of led requests
	while (pull_from_list(&dest, &tracknum, &type, &on_off)) {

		// if this is a track led
		// copy midi event required to light led into midi buffer
		if (dest == TRACK) memcpy (buffer, &track[tracknum].led [type][on_off][0], 3);
		// copy midi event required to light led into midi buffer
		if (dest == BAR) memcpy (buffer, &bar[tracknum].led [type][on_off][0], 3);

		// if buffer is not empty, then send as midi out event
		// we take care of writing led events at different time marks to make sure all of these are taken into account
		if (buffer [0] | buffer [1] | buffer [2]) {
			jack_midi_event_write (midiout, 0, buffer, 3);
//			jack_midi_event_write (midiout, i++, buffer, 3);
// UNCOMMENT THE BELOW LINE FOR MIDI OUT TRACING
// 0x50 is the "clock" led
//if (buffer[1]!=0x50) printf ("%02x %02x %02x\n", buffer[0], buffer[1], buffer[2]);
		}
	}



	/*******************************/
	/* Third, process AUDIO events */
	/*******************************/

	// now process audio events: as this "process" function has been called, it means that the audio buffer is full
	// 2 inputs ports : sound card has 2 mono inputs (also could be considered as Left, Right)
	for (i = 0; i < 2; i++)
	{
		in = jack_port_get_buffer ( input_ports[i], nframes );
		out = jack_port_get_buffer ( output_ports[i], nframes );

		/* in any case, copy audio in to audio out */
		memcpy ( out, in, nframes * sizeof ( jack_default_audio_sample_t ) );

		/* process each track */
		for (j=0; j<NB_TRACKS; j++) {
			// NOTE: we process PLAY events before RECORD to allow playing and recording at the same time
			// this allows to play the internal track buffer before potentially overwriting it with new data
			// (although the result is not so great ;-) )

			/*******************/
			/* PLAY processing */
			/*******************/

			// test if play is on or pending_off (ie. still on)
			if ((track[j].status[PLAY] == ON) || (track[j].status[PLAY] == PENDING_OFF)) {

				mute = OFF;
				// determine if track shoud be muted, either because it has mute button, or because one of the tracks is in solo
				if (track[j].status[MUTE] == ON) mute = ON;
				// check if another track is in solo mode
				for (k=0; k< NB_TRACKS; k++) {
				// check status of the other tracks; if one of the other tracks is SOLO, then mute current track
					if ((k != j) && (track[k].status[SOLO] == ON)) mute = ON;
				}

				if (i == 0) {
					// left channel
					// check if we are in BBT mode, and we have a new bar
					// check if length played in bar is equal to length in bar of what has been recorded; if this is the case, then loop
					if (is_pending_action (j) == ON_BBT) {
						if ((BBT_bar - track[j].play_bar_left) >= (track[j].end_bar_left - track[j].record_bar_left)) {
							track[j].play_index_left = 0;
							track[j].play_bar_left = BBT_bar;
						}
					}

					for (k = 0 ; k <nframes; k++) {
						// audio crack removal mechanism
						// this is basically making sure that the end of recording (last sample) corresponds to the start of recording (first sample).
						// so when we play the start of recording, we allocate "nframes_half" samples to make a linear progression between last sample value and first sample value

						// audio sample to be send to audio out
						sample = track [j].left [(track[j].play_index_left + k)];

						// check if we are at the beginning of the sample, ie: new loop
						if (track[j].play_index_left == 0) {
							// we are at the beginning of the new loop: we need to make the link with previous sample played to avoid cracking noises
							// to do this, we will do a linear progression from the last sample (played in the last callback call) to the middle of the frame set
							if (k <= nframes_half) {
								sample = track[j].last_sample_left + ((track [j].left [(track[j].play_index_left + nframes_half)] - track[j].last_sample_left) * (jack_default_audio_sample_t)((float) k / (float) nframes_half));
							}
						}

						// copy only if track is not muted
						if (mute == OFF) out [k] += sample * track[j].volume;
					}

					// set new value for last sample : sample contains the last sample to be played
					track[j].last_sample_left = sample;

					// increment index and check if not overflow or not over end of the recording
					track[j].play_index_left += nframes;
					if ((track[j].play_index_left >= NB_SAMPLES) || (track[j].play_index_left >= track[j].end_index_left)) track[j].play_index_left = 0;
				}

				if (i == 1) {
					// right channel
					// check if we are in BBT mode, and we have a new bar
					// check if length played in bar is equal to length in bar of what has been recorded; if this is the case, then loop
					if (is_pending_action (j) == ON_BBT) {
						if ((BBT_bar - track[j].play_bar_right) >= (track[j].end_bar_right - track[j].record_bar_right)) {
							track[j].play_index_right = 0;
							track[j].play_bar_right = BBT_bar;
						}
					}

					for (k = 0 ; k <nframes; k++) {
						// audio crack removal mechanism
						// this is basically making sure that the end of recording (last sample) corresponds to the start of recording (first sample).
						// so when we play the start of recording, we allocate "nframes_half" samples to make a linear progression between last sample value and first sample value

						// audio sample to be send to audio out
						sample = track [j].right [(track[j].play_index_right + k)];

						// check if we are at the beginning of the sample, ie: new loop
						if (track[j].play_index_right == 0) {
							// we are at the beginning of the new loop: we need to make the link with previous sample played to avoid cracking noises
							// to do this, we will do a linear progression from the last sample (played in the last callback call) to the middle of the frame set
							if (k <= nframes_half) {
								sample = track[j].last_sample_right + ((track [j].right [(track[j].play_index_right + nframes_half)] - track[j].last_sample_right) * (jack_default_audio_sample_t)((float) k / (float) nframes_half));
							}
						}

						// copy only if track is not muted
						if (mute == OFF) out [k] += sample * track[j].volume;
					}

					// set new value for last sample : sample contains the last sample to be played
					track[j].last_sample_right = sample;

					// increment index and check if not overflow or not over end of the recording
					track[j].play_index_right += nframes;
					if ((track[j].play_index_right >= NB_SAMPLES) || (track[j].play_index_right >= track[j].end_index_right)) track[j].play_index_right = 0;
				}
			}


			/*********************/
			/* RECORD processing */
			/*********************/
			// test if record is on or pending_off (ie. still on)
			if ((track[j].status[RECORD] == ON) || (track[j].status[RECORD] == PENDING_OFF)) {

				if (i == 0) {
					// left channel
					// copy input to record buffer
					memcpy ((track[j].left + track[j].record_index_left), in, nframes * sizeof ( jack_default_audio_sample_t ));
					// increment index and check if not overflow
					track[j].record_index_left = (track[j].record_index_left >= NB_SAMPLES) ? 0 : (track[j].record_index_left + nframes);
				}

				if (i == 1) {
					// right channel
					// copy input to record buffer
					memcpy ((track[j].right + track[j].record_index_right), in, nframes * sizeof ( jack_default_audio_sample_t ));
					// increment index and check if not overflow
					track[j].record_index_right = (track[j].record_index_right >= NB_SAMPLES) ? 0 : (track[j].record_index_right + nframes);
				}
			}
		}


		// check if out audio buffer is not out of boundaries {-1.0, +1.0} to limit saturation
		for (h=0; h<nframes; h++) {
			if (out [h] > 1.0f) out [h] = 1.0f;
			if (out [h] < -1.0f) out [h] = -1.0f;
		}
	}

	return 0;
}


// process callback called to process midi_in events in realtime
int midi_in_process (jack_midi_event_t *event, jack_nframes_t nframes) {

	int i, j;
	int k,l;

	// check change in time signature by checking if pad has been pressed; if yes, set new time signature and force new bar
	if (same_event(event->buffer,track[0].ctrl[TIMESIGN])) {
		change_timesign ();
		// no need to switch any pad led on: as we are forcing new bar, next clock event will be a new bar, which will lit the timesign pad on
	}

	// check all the tracks to see if MIDI in event (ie. UI event) corresponds to one of the track
	// event->buffer contains the midi buffer
	for (i=0; i< NB_TRACKS; i++) {

		// check MIDI code of midi in event with track functions
		// PLAY
		if (same_event(event->buffer,track[i].ctrl[PLAY])) {
			// check whether there is any recording to play; if not, then PLAY shall be OFF
			if ((track[i].end_index_left == 0) && (track[i].end_index_right == 0)) track[i].status[PLAY] = OFF;
			else {
				// play event: set status accordingly
				track[i].status[PLAY] = next_status_4 (track[i].status[PLAY]);
			}
			// switch led on according to status
			led (i, PLAY, track[i].status[PLAY]);
		}

		// RECORD
		if (same_event(event->buffer,track[i].ctrl[RECORD])) {
			// record event: set status accordingly
			track[i].status[RECORD] = next_status_4 (track[i].status[RECORD]);
			// switch led on according to status
			led (i, RECORD, track[i].status[RECORD]);
		}

		// MUTE
		if (same_event(event->buffer,track[i].ctrl[MUTE])) {
			// mute event: set status accordingly
			track[i].status[MUTE] = next_status_2 (track[i].status[MUTE]);
			// switch led on according to status
			led (i, MUTE, track[i].status[MUTE]);
		}

		// SOLO
		if (same_event(event->buffer,track[i].ctrl[SOLO])) {
			// solo event: set status accordingly
			track[i].status[SOLO] = next_status_2 (track[i].status[SOLO]);
			// switch led on according to status
			led (i, SOLO, track[i].status[SOLO]);

			// if solo event is set ON, remove solo from the other tracks
			if (track[i].status[SOLO] == ON) {
				for (j=0; j< NB_TRACKS; j++) {
					if (j != i) {
						track[j].status[SOLO] = OFF;
						// switch led on according to status
						led (j, SOLO, track[j].status[SOLO]);
					}
				}
			}
		}

		// VOLDOWN
		if (same_event(event->buffer,track[i].ctrl[VOLDOWN])) {
			// volume down event: volume does not have a real "status"
			// switch led on according to status
			led (i, VOLDOWN, PENDING_ON);
			led (i, VOLUP, OFF);

			// this is an decrement of 0.1
			if (track [i].volume > 0.1f) {
				track [i].volume -=0.1f;   // decrement of 0.1
				// switch led off according to status
				led (i, VOLDOWN, OFF);
			}
			else {
				track [i].volume = 0.0f;                            // line is useless, we keep it to be safe
				// in case we are at min volume, we keep the pad lit (on)
				led (i, VOLDOWN, ON);
			}
		}

		// VOLUP
		if (same_event(event->buffer,track[i].ctrl[VOLUP])) {
			// volume up event: volume does not have a real "status"
			// switch led on according to status
			led (i, VOLUP, PENDING_ON);
			led (i, VOLDOWN, OFF);

			// this is an increment of 0.1
			if (track [i].volume < 0.9f) {
				track [i].volume +=0.1f;   // increment of 0.1
				// switch led off according to status
				led (i, VOLUP, OFF);
			}
			else {
				track [i].volume = 1.0f;                            // line is useless, we keep it to be safe
				// in case we are at max volume, we keep the pad lit (on)
				led (i, VOLUP, ON);
			}
		}

		// MODE
		if (same_event(event->buffer,track[i].ctrl[MODE])) {
			// mute event: set status accordingly
			track[i].status[MODE] = next_status_2 (track[i].status[MODE]);
			// switch led on according to status
			led (i, MODE, track[i].status[MODE]);
		}

		// DELETE
		if (same_event(event->buffer,track[i].ctrl[DELETE])) {
			// delete event: set status accordingly
			track[i].status[DELETE] = next_status_4 (track[i].status[DELETE]);
			// switch led on according to status
			led (i, DELETE, track[i].status[DELETE]);
		}
	}

	// check all the bars to see if MIDI in event (ie. UI event) corresponds to one of the bar rows
	// event->buffer contains the midi buffer
	for (i=0; i< NB_BAR_ROWS; i++) {

		// check MIDI code of midi in event with track functions
		for (j=0; j<LAST_BAR_ELT; j++) {
			if (same_event(event->buffer,bar[i].ctrl[j])) {

				// we pressed one pad in the bar row; change its value to ON or OFF based on its previous status
				bar[i].status[j] = next_status_2 (bar[i].status[j]);

				// make sure no other bar led is ON, except the one we have pressed
				for (k=0; k< NB_BAR_ROWS; k++) {
					for (l=0; l< LAST_BAR_ELT; l++) {
						if ((k!=i) || (l!=j)) {
							// force the other bar leds to OFF
							bar[k].status[l] = OFF;
							bar_led (k, l, OFF);
						}
					}
				}

				// calculate new value of number_of_bars, depending on pad that has been pressed
				if (bar[i].status[j] == ON)
					number_of_bars = ((i * LAST_BAR_ELT) + j + 1);
				else number_of_bars = 0;
				// switch led on according to status
				bar_led (i, j, bar[i].status[j]);
			}
		}
	}
}


// process callback called to process midi_clock events in realtime
int midi_clock_process (jack_midi_event_t *event, jack_nframes_t nframes) {

	int i;

	// in case of midi play event, then next CLOCK event is a new bar
	if (event->buffer[0] == MIDI_PLAY) {
		is_BBT = PENDING_ON;		// indicates we have a new bar
	}

	// in case of midi clock event
	if (event->buffer[0] == MIDI_CLOCK) {

		// calculate new BBT (bar, beat, tick) as we had clock event
		// and switch leds
		led (0, TIMESIGN, time_progress ());

		// process the UI, ie. through MIDI IN events
		// there are 2 possibilities for each track : either mode == OFF, in which case we are in BBT mode, ie. events only occur at bar change
		// or mode == ON, in which case we are in free mode, and events occur at tick
		// note that some events (volume, mute, solo, mode...) always occur at tick, regardless of the mode
		// only play, record, and potentially delete are "mode dependent"

		// first, make sure we have a timing event, so we are in sync

		// process each track
		for (i=0; i<NB_TRACKS; i++) {


			/**************************************************/
			/* automatically PLAY after recording (free mode) */
			/**************************************************/
			// in case record shall stop now, then start playing from now on
			if ((track[i].status[MODE]==ON) && (track[i].status[RECORD] == PENDING_OFF)) {
				// set to next status (ie. PENDING_ON)
				track[i].status[PLAY] = PENDING_ON;
				// no need to switch led on according to status, as status will turn to ON in the next few lines
				//led (i, PLAY, track[i].status[PLAY]);
			}


			// check if a pending action (play, record, delete) is ready to be performed
			// this shall be done only if MODE == OFF AND we have a new bar (that is: is_BBT = 1), or in any case if MODE == ON
			if (is_pending_action (i)) {


				/*********************/
				/* RECORD processing */
				/*********************/
				// a new record event has appeared, and it was not here previously : treat record event
				if (track[i].status[RECORD] == PENDING_ON) {

					// this is a new recording : set index (where to write in the track buffer) to 0
					track[i].record_index_left = 0;
					track[i].record_index_right = 0;
					// recording starts at current bar number
					track[i].record_bar_left = BBT_bar;
					track[i].record_bar_right = BBT_bar;

					// set number of bars that are going to be recorded
					track[i].record_nb_bar = number_of_bars;

					// set to next status (ie. ON)
					track[i].status[RECORD] = ON;
					// switch led on according to status
					led (i, RECORD, track[i].status[RECORD]);
				}

				// a record event has disappeared, and it was there previously : treat record event
				if (track[i].status[RECORD] == PENDING_OFF) {

					// this is the end of the recording : set end index (index of end of the recording in the track audio buffer)
					track[i].end_index_left = track[i].record_index_left + nframes;
					track[i].end_index_right = track[i].record_index_right + nframes;
					// recording ends at current bar number
					track[i].end_bar_left = BBT_bar;
					track[i].end_bar_right = BBT_bar;

					// set to next status (ie. OFF)
					track[i].status[RECORD] = OFF;
					// switch led on according to status
					led (i, RECORD, track[i].status[RECORD]);
				}

				// if we are in bar mode and a recording is already ongoing, check for how many bars we need to record
				// if we need to do this for "0" bars, it means the user will stop the recording himself
				if ((is_pending_action (i) == ON_BBT) && (track[i].status[RECORD] == ON) && (track[i].record_nb_bar !=0)) {
					// if we reach the number of bars to be recorded, then we need to stop the recording
					if (BBT_bar >= (track[i].record_bar_left + track[i].record_nb_bar) -1) {
						// move to next status, ie. PENDING_OFF : we are basically mimic-ing the pressing the "record" pad to stop recording
						// we only test left channel as left and right channel are always in sync (as they are recorded together)
						track[i].status[RECORD] = next_status_4 (track[i].status[RECORD]);

						// switch led on according to status
						led (i, RECORD, track[i].status[RECORD]);
					}
				}


				/*******************/
				/* PLAY processing */
				/*******************/
				// a new play event has appeared, and it was not here previously : treat play event */
				if (track[i].status[PLAY] == PENDING_ON) {

					// this is a new playing : set index (where to read in the track buffer) to 0 */
					track[i].play_index_left = 0;
					track[i].play_index_right = 0;
					// playing starts at current bar number
					track[i].play_bar_left = BBT_bar;
					track[i].play_bar_right = BBT_bar;
					// anti crack mechanism
					track[i].last_sample_left = 0.0;
					track[i].last_sample_right = 0.0;


					// set to next status (ie. ON)
					track[i].status[PLAY] = ON;
					// switch led on according to status
					led (i, PLAY, track[i].status[PLAY]);
				}

				/* a play event has disappeared, and it was there previously : treat play event */
				if (track[i].status[PLAY] == PENDING_OFF) {
					// set to next status (ie. OFF)
					track[i].status[PLAY] = OFF;
					// switch led on according to status
					led (i, PLAY, track[i].status[PLAY]);
				}


				/*********************/
				/* DELETE processing */
				/*********************/
				if (track[i].status[DELETE] == PENDING_ON) {

					/* set audio buffer of the track to 0 : we take the max size, plus add some more room (8192) to avoid overflows */
					/* this coud be time consuming so let's comment for now */
					//memset (track [i].left, 0, (NB_SAMPLES + 8192) * sizeof (jack_default_audio_sample_t));
					//memset (track [i].right, 0, (NB_SAMPLES + 8192) * sizeof (jack_default_audio_sample_t));

					// also reset key variables (playing and recording index, status, etc)
					track[i].play_index_left = 0;
					track[i].play_index_right = 0;
					track[i].record_index_left = 0;
					track[i].record_index_right = 0;
					track[i].last_sample_left = 0.0;
					track[i].last_sample_right = 0.0;
					track[i].end_index_left = 0;
					track[i].end_index_right = 0;
					track[i].record_bar_left = 0;
					track[i].record_bar_right = 0;
					track[i].end_bar_left = 0;
					track[i].end_bar_right = 0;
					track[i].volume = 1.0f;
					track[i].record_nb_bar = 0;
					reset_status (&track[i]);

					// switch all leds off for the track
					led_off (i);
					// track volume set to 1.0 by default
					led (i, VOLUP, ON);

				}
			}


			/*************************************************/
			/* automatically PLAY after recording (bar mode) */
			/*************************************************/
			// in case record will stop at next bar (ie. MODE == OFF), then position play as pending_on
			if ((track[i].status[MODE]==OFF) && (track[i].status[RECORD] == PENDING_OFF)) {
				// set to next status (ie. PENDING_ON)
				track[i].status[PLAY] = PENDING_ON;
				// switch led on according to status
				led (i, PLAY, track[i].status[PLAY]);
			}
		}
	}
}

