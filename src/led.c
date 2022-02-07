/** @file led.c
 *
 * @brief led module allows switching LEDs on/off on the midi control surface.
 *
 */

#include "types.h"
#include "globals.h"
#include "config.h"
#include "process.h"
#include "led.h"
#include "time.h"
#include "utils.h"

// function called to turn pad led on/off
int led (int tracknum, int type, int on_off) {

	// check if the light is already ON, OFF, PENDING_ON according to what we want
	// this will allow to determine wether we take the request into account or not
	if (led_status [tracknum][type] != on_off) {
		// push to list of led requests to be processed
		push_to_list (TRACK,tracknum, type, on_off);
		// update led status so it matches with request
		led_status [tracknum][type] = (unsigned char) on_off;

	}
}


// function called to turn pad led off for a track
int led_off (int tracknum) {

	led (tracknum, PLAY, OFF);
	led (tracknum, RECORD, OFF);
	led (tracknum, MUTE, OFF);
	led (tracknum, SOLO, OFF);
	led (tracknum, VOLUP, OFF);
	led (tracknum, VOLDOWN, OFF);
	led (tracknum, MODE, OFF);
	led (tracknum, DELETE, OFF);
}


// function called to turn bar rows pad led on/off
int bar_led (int barrownum, int num, int on_off) {

	// check if the light is already ON, OFF, PENDING_ON according to what we want
	// this will allow to determine wether we take the request into account or not
	if (bar_led_status [barrownum][num] != on_off) {
		// push to list of led requests to be processed
		push_to_list (BAR, barrownum, num, on_off);
		// update led status so it matches with request
		bar_led_status [barrownum][num] = (unsigned char) on_off;

	}
}


// function called to turn pad led off for a bar row
int bar_led_off (int barrownum) {

int i;

	for (i=0; i<LAST_BAR_ELT; i++) bar_led (barrownum, i, OFF);
}
