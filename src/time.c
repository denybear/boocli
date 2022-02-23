/** @file time.c
 *
 * @brief Contains time and BBT-related functions.
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


// function called in case user pressed the time_signature pad
int change_timesign () {

	// time signatures are sorted in a particular order so it is easy to process
	// check boundaries
	if (++timesign > _5_4) timesign = _4_4;

	// changes time_signature according to predefined values
	switch (timesign) {
		case _4_4:
			BBT_numerator = 4;
			BBT_denominator = 4;
			break;
		case _2_2:
			BBT_numerator = 2;
			BBT_denominator = 2;
			break;
		case _2_4:
			BBT_numerator = 2;
			BBT_denominator = 4;
			break;
		case _3_4:
			BBT_numerator = 3;
			BBT_denominator = 4;
			break;
		case _6_8:
			BBT_numerator = 6;
			BBT_denominator = 8;
			break;
		case _9_8:
			BBT_numerator = 9;
			BBT_denominator = 8;
			break;
		case _12_8:
			BBT_numerator = 12;
			BBT_denominator = 8;
			break;
		case _5_4:
			BBT_numerator = 5;
			BBT_denominator = 4;
			break;
		default:
			BBT_numerator = 4;
			BBT_denominator = 4;
			break;
	}

		// new bar at next clock
		is_BBT = PENDING_ON;
}


// function called at each tick to progress the number of ticks, beats and bar (BBT)
// returns led to be lit (ON, OFF, PENDING_ON...)
int time_progress () {

int ret = OFF;

	// process the delay of 4 ticks in the switching on/off of the pad led... this is to make sure hardware can follow the pace
	// remove this if/else chunk to remove this functionality
	// and remove BBT_wait_4_ticks variable
	if (--BBT_wait_4_ticks <= 0) ret = OFF;
	else {
		if (BBT_wait_4_ticks <=3) ret = PENDING_ON;				// new beat only, then light PENDING_ON
		else {
			ret = ON;											// new bar, light shall be ful ON
			if (BBT_wait_4_ticks == 4) BBT_wait_4_ticks = 0;
		}
	}


	// check if we should have a new bar (because this is program's start, or play event or sign time change has occured right before
	if (is_BBT == PENDING_ON) {
		BBT_bar++;
		BBT_beat = 1;
		BBT_previous_beat = 1;
		BBT_tick = 0;		// set to 0 as we are going to increment right after to it becomes 1
		BBT_wait_4_ticks = 8;		// purpose of this is to delay switch on/off of led of about 4 clock ticks, so hardware can support it
		is_BBT = ON;		// indicates we have a new bar
		ret = ON;			// return value which could be used to set leds
	}
	else is_BBT = OFF;		// init as "no new bar", and we are going to calculate afterwards if new bar

	// process BBT according to time signature and tick counter
	// compute tick value
	BBT_tick++;
	// compute beat value
	BBT_beat_value = MIDI_CLOCK_RATE / BBT_denominator;

	// compute beat based on numerator time signature
	switch (BBT_numerator) {
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			// simple time signature
			BBT_beat = ((BBT_tick-1) / BBT_beat_value)+1;

			// check if we changed beat
			if (BBT_beat != BBT_previous_beat) {
				ret = PENDING_ON;			// return value which could be used to set leds
				BBT_previous_beat = BBT_beat;
				BBT_wait_4_ticks = 4;		// purpose of this is to delay switch on/off of led of about 4 clock ticks, so hardware can support it
			}

			/* check if we changed bar */
			if (BBT_beat > BBT_numerator) {
				BBT_bar++;
				BBT_beat = 1;
				BBT_previous_beat = 1;
				BBT_tick = 1;
				BBT_wait_4_ticks = 8;		// purpose of this is to delay switch on/off of led of about 4 clock ticks, so hardware can support it
				is_BBT = ON;		// indicates we have a new bar
				ret = ON;			// return value which could be used to set leds
			}
			break;
		case 6:
		case 9:
		case 12:
			/* compound time signature */
			BBT_beat = ((BBT_tick-1) / (BBT_beat_value*3))+1;

			/* check if we changed beat */
			if (BBT_beat != BBT_previous_beat) {
				ret = PENDING_ON;			// return value which could be used to set leds
				BBT_previous_beat = BBT_beat;
				BBT_wait_4_ticks = 4;		// purpose of this is to delay switch on/off of led of about 4 clock ticks, so hardware can support it
			}

			/* check if we change bar */
			if (BBT_beat*3 > BBT_numerator) {
				BBT_bar++;
				BBT_beat = 1;
				BBT_previous_beat = 1;
				BBT_tick = 1;
				BBT_wait_4_ticks = 8;		// purpose of this is to delay switch on/off of led of about 4 clock ticks, so hardware can support it
				is_BBT = ON;		// indicates we have a new bar
				ret = ON;			// return value which could be used to set leds
			}
			break;
	}

	return ret;
}


