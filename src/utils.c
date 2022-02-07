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


// add led request to the list of requests to be processed
int push_to_list (int dest, int tracknum, int type, int on_off) {

	// add to list
	list_buffer [list_index][0] = (unsigned char) dest;
	list_buffer [list_index][1] = (unsigned char) tracknum;
	list_buffer [list_index][2] = (unsigned char) type;
	list_buffer [list_index][3] = (unsigned char) on_off;

	// increment index and check boundaries
	//list_index = (list_index >= LIST_ELT) ? 0; list_index++;
	if (list_index >= LIST_ELT) {
		fprintf ( stderr, "too many led requests in the list.\n" );
		list_index = 0;
	}
	else list_index++;
}


// pull out led request from the list of requests to be processed (FIFO style)
// returns 0 if pull request has failed (nomore request to be pulled out)
int pull_from_list (int *dest, int *tracknum, int *type, int *on_off) {

	// check if we have requests to be pulled; if not, leave
	if (list_index == 0) return 0;

	// remove first element from list
	*dest = list_buffer [0][0];
	*tracknum = list_buffer [0][1];
	*type = list_buffer [0][2];
	*on_off = list_buffer [0][3];

	// decrement index
	list_index--;

	// move the rest of the list 1 item backwards
	memmove (&list_buffer[0][0], &list_buffer[1][0], list_index * 4);

	return 1;
}


// determines if 2 midi messages (ie. events) are the same; returns 1 if yes
int same_event (unsigned char * evt1, unsigned char * evt2) {

	if ((evt1[0]==evt2[0]) && (evt1[1]==evt2[1])) return 1;
	return 0;
}


// get a status, process state machine with 4 states and returns next status
unsigned char next_status_4 (unsigned char status) {

	switch (status)	{
		case OFF:
			return PENDING_ON;
		case ON:
			return PENDING_OFF;
		default:
			return status;
	}
}


// get a status, process state machine with 2 states and returns next status
unsigned char next_status_2 (unsigned char status) {

	if (status == OFF) return ON;
	else return OFF;
}


// determines if a pending action (play, record...) shall be done on a track, that is mode = OFF and new bar (returns ON_BBT), or mode == ON (returns ON)
int is_pending_action (int i) {

	// this shall be done only if MODE == OFF AND we have a new bar (that is: is_BBT = 1), or in any case if MODE == ON
	if ((track[i].status[MODE] == OFF) && (is_BBT==ON)) return ON_BBT;
	if (track[i].status[MODE] == ON) return ON;
	return OFF;
}


// reset all the status bytes of a track
int reset_status (track_t *t) {

	// set status memory to 0
	memset (t->status, OFF, LAST_ELT);
}


