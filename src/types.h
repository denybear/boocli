/** @file types.h
 *
 * @brief This file defines constants and main global types
 *
 */

/* includes */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <jack/jack.h>
#include <jack/midiport.h>
#include <libconfig.h>



/* constants */
#define NB_TRACKS	4	// number of tracks for the looper
#define NB_BAR_ROWS 2	// number of bar rows to select tehe number of bars to record
#define MIDI_SYSEX	0xF0
#define MIDI_CLOCK 0xF8
#define MIDI_RESERVED 0xF9
#define MIDI_PLAY 0xFA
#define MIDI_STOP 0xFC
#define MIDI_CLOCK_RATE 99 // 24*4 ticks for full note, 24 ticks per quarter note
//#define MIDI_CLOCK_RATE 96 // 24*4 ticks for full note, 24 ticks per quarter note

#define FIRST_ELT 0		// used for declarations and loops
#define TIMESIGN 0
#define	LOAD 1
#define	SAVE 2
#define PLAY 3
#define RECORD 4
#define MUTE 5
#define SOLO 6
#define VOLDOWN 7
#define VOLUP 8
#define MODE 9
#define DELETE 10
#define LAST_ELT 11		// used for declarations and loops

#define LAST_BAR_ELT 8		// used for declarations and loops


/* max number of samples of each track buffer (L,R) */
#define NB_SAMPLES	13230000	// 13230000 samples at 44100 Hz means 300 seconds of music, ie. 5 min loops
								// 13230000 samples at 48000 Hz means 275 seconds of music, ie. 4.5 min loops

/* time signature values */
#define FIRST_TIMESIGN 0
#define _4_4 0
#define _2_2 1
#define	_2_4 2
#define	_3_4 3
#define	_6_8 4
#define	_9_8 5
#define	_12_8 6
#define	_5_4 7
#define LAST_TIMESIGN 7
// let's not support 3_8 nor 6_4 for now at it messes with our BBT calculations algorithm

/* define status, etc */
#define TRUE 1
#define FALSE 0

#define TRACK 0
#define BAR 1

#define ON_BBT 2

#define FIRST_STATE 0		// used for declarations and loops
#define OFF 0
#define ON 1
#define PENDING_ON 2
#define PENDING_OFF 3
#define LAST_STATE 4		// used for declarations and loops

/* list management (used for led mgmt) */
#define LIST_ELT 100

/* types */
typedef struct {						// structure for each of the 8 tracks
	unsigned char ctrl [LAST_ELT] [2];	//controls on the midi control surface
	unsigned char led [LAST_ELT] [LAST_STATE] [3];		// led lightings on the midi control surface (off, pending on, on, pending off...)
	unsigned char status [LAST_ELT];	// Status byte for each function

	jack_nframes_t record_index_left;		// index in the track buffer where to write
	jack_nframes_t record_index_right;	// index in the track buffer where to write
	jack_nframes_t record_bar_left;		// bar number where recording starts
	jack_nframes_t record_bar_right;	// bar number where recording starts

	jack_nframes_t play_index_left;		// index in the track buffer where to play
	jack_nframes_t play_index_right;	// index in the track buffer where to play
	jack_nframes_t play_bar_left;		// bar number where playing starts
	jack_nframes_t play_bar_right;		// bar number where playing starts

	jack_nframes_t end_index_left;		// index of end of the loop for left channel
	jack_nframes_t end_index_right;		// index of end of the loop for right channel
	jack_nframes_t end_bar_left;		// bar number of end of the loop for left channel
	jack_nframes_t end_bar_right;		// bar number of end of the loop for right channel

	jack_nframes_t record_nb_bar;		// number of bars that shall be recorded. If 0, then user shall define end of recording by pressing the record pad a 2nd time

	float volume;				// volume of the track, between 0 and 1 (by 0.1 increments)

	jack_default_audio_sample_t last_sample_left;	// last sample played for last frame played (left)
	jack_default_audio_sample_t last_sample_right;	// last sample played for last frame played (right)

	jack_default_audio_sample_t *left;	// audio buffer (left)
	jack_default_audio_sample_t *right;	// audio buffer (right)

} track_t;

typedef struct {						// structure for each of the 2 lines of bar selectors
	unsigned char ctrl [LAST_BAR_ELT] [2];	//controls on the midi control surface
	unsigned char led [LAST_BAR_ELT] [LAST_STATE] [3];		// led lightings on the midi control surface (off, pending on, on, pending off...)
	unsigned char status [LAST_BAR_ELT];	// Status byte for each function
} bar_t;
