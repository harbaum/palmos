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
 */


#ifndef THREED_H
#define THREED_H

#include "space.h"

#define noDEPTH_SORT   // adds a little load, make missile look better,
		       // can cause problems with 
		       // lines like the ones at the bottom of the targoid

#ifndef DEPTH_SORT
#define POINT_2D(a,b)    {a,b}
#define SET_POINT(p,a,b) { p.x = a; p.y = b;}
#else
#define POINT_2D(a,b)  {a,b,0}
#define SET_POINT(p,a,b) { p.x = a; p.y = b; p.z = 0;}
#endif

/* table use to store points in list */
struct point2d {
  Int16 x, y;
#ifdef DEPTH_SORT
  Int16 z;
#endif
};

void SEC3D draw_ship (struct univ_object *ship);
void SEC3D draw_filled_circle(int cx, int cy, int radius, UInt32 col);
void SEC3D draw_polygon (struct point2d *, UInt8 *, int, UInt8);
void SEC3D draw_line_3d(int, int, int, int);

#endif
