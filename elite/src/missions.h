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

#ifndef MISSIONS_H
#define MISSIONS_H

char *mission_planet_desc (struct galaxy_seed planet);
void check_mission_brief (void);

#ifdef LIMIT
void mission_limit(void);
#endif

#endif
