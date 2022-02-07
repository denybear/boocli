/** @file utils.h
 *
 * @brief This file defines prototypes of functions inside utils.c
 *
 */

int push_to_list (int, int , int , int);
int pull_from_list (int *, int *, int *, int *);
int same_event (unsigned char *, unsigned char *);
unsigned char next_status_4 (unsigned char);
unsigned char next_status_2 (unsigned char);
int is_pending_action (int);
int reset_status (track_t *);
