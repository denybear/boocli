/** @file process.h
 *
 * @brief This file defines prototypes of functions inside process.c
 *
 */

int process ( jack_nframes_t, void *);
int midi_in_process (jack_midi_event_t *, jack_nframes_t);
int midi_clock_process (jack_midi_event_t *, jack_nframes_t);

