/** @file disk.c
 *
 * @brief Contains load and save functions.
 *
 */

#include "types.h"
#include "globals.h"
#include "config.h"
#include "process.h"
#include "led.h"
#include "time.h"
#include "utils.h"
#include "disk.h"


// function called in case user pressed the load pad
int load () {

	FILE *fp;
	int i;
	int number_of_tracks;
	track_t tr;
	jack_default_audio_sample_t *l, *r;

	// create file in write mode
	fp = fopen ("./boocli.sav", "r");
	if (fp==NULL) {
		fprintf ( stderr, "Cannot read save file.\n" );
		return 0;
	}

	// read number of tracks
	fread ((int *) &number_of_tracks, sizeof (int), 1, fp);
	// if greater than current number of tracks, set to current number of tracks
	if (number_of_tracks > NB_TRACKS) number_of_tracks = NB_TRACKS;

	// read time signature
	fread ((int*) &timesign, sizeof (int), 1, fp);
	// if timesign is out of boundaries, set to first timesign (_4_4)
	if (timesign > LAST_TIMESIGN) timesign = FIRST_TIMESIGN;

	// read track one by one
	for (i=0; i<NB_TRACKS;i++) {

		// read the whole track struct from file
		fread (&tr, sizeof (track_t), 1, fp);
		// check whether there is some audio recorded; if not, read next track
		// this way: in case the track in the file is empty (non-recorded), the track already in memory is kept and is not overwritten by an empty track
		if ((tr.end_index_left == 0) && (tr.end_index_right == 0)) continue;

		// save address of audio buffers (absolutely required otherwise we will point anywhere in memory!)
		l = track[i].left;
		r = track[i].right;

		// copy tr variable to track variable
		memcpy (&track[i], &tr, sizeof(track_t));

		// restore address of audio buffers
		track[i].left = l;
		track[i].right = r;

		// read the audio buffers and write to memory
		if (track[i].end_index_left !=0) fread (track[i].left, sizeof (jack_default_audio_sample_t), track[i].end_index_left, fp);
		if (track[i].end_index_right !=0) fread (track[i].right, sizeof (jack_default_audio_sample_t), track[i].end_index_right, fp);
	}

	// close file
	fclose (fp);
}


// function called in case user pressed the save pad
int save () {

	FILE *fp;
	int i;

	// create file in write mode
	fp = fopen ("./boocli.sav", "w");
	if (fp==NULL) {
		fprintf ( stderr, "Cannot write save file.\n" );
		return 0;
	}

	// write number of tracks
	i = NB_TRACKS;
	fwrite ((int*) &i, sizeof (int), 1, fp);

	// write time signature
	fwrite ((int*) &timesign, sizeof (int), 1, fp);

	// for each track, write key information
	// so basically start/stop pointers and samples
	for (i=0; i<NB_TRACKS;i++) {

		// write the whole track struct in file
		fwrite (&track[i], sizeof (track_t), 1, fp);

		// write the audio buffers
		if (track[i].end_index_left !=0) fwrite (track[i].left, sizeof (jack_default_audio_sample_t), track[i].end_index_left, fp);
		if (track[i].end_index_right !=0) fwrite (track[i].right, sizeof (jack_default_audio_sample_t), track[i].end_index_right, fp);
	}

	// close file
	fclose (fp);
}


