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

#ifndef SHIPDATA_H
#define SHIPDATA_H

#ifdef MKSHIPDATA
typedef char Int8;
typedef unsigned char UInt8;
typedef short Int16;
typedef unsigned short UInt16;
typedef long Int32;
typedef unsigned long UInt32;
typedef char Boolean;
typedef char Char;
#else
#include <PalmOS.h>
#endif

#define NO_OF_SHIPS		33

/* define colors */
#define MK_COLOR(a)     (((UInt32)(a)<<24)|((UInt32)(a)<<16)|\
                         ((UInt32)(a)<<8)|((UInt32)(a)<<0))

#ifdef MKSHIPDATA
struct ship_point {
  Int16 x, y, z;
  Int16 dist;
  Int16 face1, face2, face3, face4;
} __attribute__ ((packed));
struct ship_face {
  UInt16 colour;		// not endianess sensitive
  Int16 norm_x, norm_y, norm_z;
  UInt16 points, p[8];
} __attribute__ ((packed));
#else
struct ship_point {
  Int8 x, y, z;
  Int8 dist;
  Int8 face1, face2, face3, face4;
} __attribute__ ((packed));
struct ship_face {
  UInt8 colour;			// not endianess sensitive
  UInt8 points, p[8];
} __attribute__ ((packed));
#endif

struct ship_data {
  Char name[32];
  UInt8 num_points;
  UInt8 num_faces;             // number of surfaces
  UInt8 low_detail_faces;
  UInt8 max_loot;
  UInt8 scoop_type;
  UInt16 size;
  UInt8 front_laser;           // point to which the front laser is attached
  UInt16 bounty;
  UInt8 energy;
  UInt8 velocity;
  UInt8 missiles;
  UInt8 laser_strength;
} __attribute__ ((packed));


#define SHIP_SUN			-2
#define SHIP_PLANET			-1
#define SHIP_MISSILE			1
#define SHIP_CORIOLIS			2
#define SHIP_ESCAPE_CAPSULE		3
#define SHIP_ALLOY			4
#define SHIP_CARGO			5
#define SHIP_BOULDER			6
#define SHIP_ASTEROID			7
#define SHIP_ROCK			8
#define SHIP_SHUTTLE			9    // shuttle
#define SHIP_TRANSPORTER		10   // shuttle
#define SHIP_COBRA3			11   // trader
#define SHIP_PYTHON			12   // trader
#define SHIP_BOA			13   // trader
#define SHIP_ANACONDA			14   // trader
#define SHIP_HERMIT			15
#define SHIP_VIPER			16   // police
#define SHIP_SIDEWINDER			17   // pack hunter
#define SHIP_MAMBA			18   // pack hunter
#define SHIP_KRAIT			19   // pack hunter
#define SHIP_ADDER			20   // pack hunter
#define SHIP_GECKO			21   // pack hunter
#define SHIP_COBRA1			22   // pack hunter
#define SHIP_WORM			23   // pack hunter
#define SHIP_MORAY			24   // pack hunter
#define SHIP_COBRA3_LONE		25   // lone hunter
#define SHIP_ASP2			26   // lone hunter
#define SHIP_PYTHON_LONE		27   // lone hunter
#define SHIP_FER_DE_LANCE		28   // lone hunter
#define SHIP_THARGOID			29
#define SHIP_THARGLET			30
#define SHIP_CONSTRICTOR		31
#define SHIP_COUGAR			32
#define SHIP_DODEC			33

#endif
