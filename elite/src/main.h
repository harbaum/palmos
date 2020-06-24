/*
 * Elite - The New Kind.
 *
 * Reverse engineered from the BBC disk version of Elite.
 * Additional material by C.J.Pinder.
 *
 * The original Elite code is (C) I.Bell & D.Braben 1984.
 * This version re-engineered in C by C.J.Pinder 1999-2001.
 *
 * email: <christian@newkind.co.uk>
 *
 *
 */

#ifndef MAIN_H
#define MAIN_H

void info_message (char *message) SEC3D;
void save_commander_screen (void);
void load_commander_screen (void);
void update_screen (void);
void draw_laser_sights (void) SEC3D;


#endif
