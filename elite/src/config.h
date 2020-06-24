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

/*
 * config.h
 *
 * Define the compile time options.
 * Most options are defined at runtime in the newkind.cfg file.
 * However some must be done at compile time either because they are
 * platform dependant or for playing speed.
 */


#ifndef CONFIG_H
#define CONFIG_H

/*
 * Set the graphics platform we are using...
 */

#define PALM

/*
 * Set the screen resolution...
 * (if nothing is defined, assume 256x256).
 */

// #define RES_800_600
// #define RES_512_512

/*
 * #define RES_640_480
 * #define RES_320_240
 */

#endif
