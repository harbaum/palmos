/*
 * Elite - The New Kind, PalmOS port
 *
 * Reverse engineered from the BBC disk version of Elite.
 * Additional material by C.J.Pinder and T.Harbaum
 *
 * The original Elite code is (C) I.Bell & D.Braben 1984.
 * This version re-engineered in C by C.J.Pinder 1999-2001.
 *
 * email: <christian@newkind.co.uk>
 *
 * PalmOS port (c) 2002 by Till Harbaum <till@harbaum.org>
 *
 */

#ifndef INTRO_H
#define INTRO_H

#ifdef INTRO1
void SEC3D initialise_intro1 (void);
void SEC3D update_intro1 (void);
#endif

#ifdef INTRO2
void SEC3D initialise_intro2 (void);
void SEC3D update_intro2 (void);
#endif

#endif
