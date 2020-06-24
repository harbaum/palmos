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

#include <PalmOS.h>

#include "config.h"
#include "elite.h"
#include "gfx.h"
#include "planet.h"
#include "vector.h"
#include "shipdata.h"
#include "threed.h"
#include "space.h"
#include "random.h"
#include "debug.h"
#include "sintab.h"
#include "options.h"

#include "3d_limit.h"

#define  DIST_COLOR  (0x7a)
#define  DIST_GRAY   (0x88)

static void SEC3D render_ship (struct univ_object *univ);

static void SEC3D draw_planet (struct univ_object *planet);
static void SEC3D render_sun_line (int xo, int yo, int x, int y, int radius);
static void SEC3D render_sun (int xo, int yo, int radius);
static void SEC3D draw_sun (struct univ_object *planet);
static void SEC3D draw_explosion (struct univ_object *univ);

#define MAX(x,y) (((x) > (y)) ? (x) : (y))

/* table use to store points in list */
static struct point2d point_list[MAX_POINTS];;

/*
  special 2d/3d graphic routines 
*/

static void draw_hline_c256 (int, int, int, UInt32) SEC3D;
static void draw_hline_g16 (int, int, int, UInt32) SEC3D;

struct z_map {
  int z;
  int poly;
};

static void z_map_qsort (struct z_map *base, Int16 nel) SEC3D;

static void draw_pixel (int, int, int) SEC3D;

extern char *vidbuff;

/*
  special drawing routines used during 3d drawing 
*/
inline static void draw_pixel (int x, int y, int color)
{
  if ((x < 0) || (x >= DSP_WIDTH) || (y < 0) || (y > DSP_HEIGHT - 1)) return;

  if(color_gfx) 
    vidbuff[DSP_WIDTH * y + x] = color;
  else {
    UInt8 *d = (UInt8*)(vidbuff + (DSP_WIDTH * y + x)/2);
    UInt8 mask1 = (x&1)?0xf0:0x0f, mask2 = ~mask1;

    *d = (*d & mask1) | (color & mask2);
  }
}

void
draw_line_3d (int x1, int y1, int x2, int y2)
{
  int cnt, err, yoff = 1;
  int dx, dy;
  UInt8 col;

  if(color_gfx) col = GFX_COL_WHITE;
  else          col = graytab[GFX_COL_WHITE];

  if (x2 < x1) {
    cnt = x1;
    x1 = x2;
    x2 = cnt;
    cnt = y1;
    y1 = y2;
    y2 = cnt;
  }

  dx = x2 - x1;
  dy = y2 - y1;

  if (dy < 0) {
    yoff = (-1);
    dy = (-dy);
  }

  if (dx > dy) {
    err = (cnt = dx) >> 1;
    do {
      draw_pixel (x1, y1, col);
      if ((err -= dy) < 0)
	err += dx, y1 += yoff;
      x1++;
    } while (--cnt >= 0);
  }
  else {
    err = (cnt = dy) >> 1;
    do {
      draw_pixel (x1, y1, col);
      if ((err -= dx) < 0)
	err += dy, x1++;
      y1 += yoff;
    } while (--cnt >= 0);
  }
}

/* draw a horizontal line clipped in 3d view area */
static void
draw_hline_c256 (int y, int start, int end, UInt32 shade)
{
  const UInt32 hline_mask0[] =
    { 0xffffffff, 0x00ffffff, 0x0000ffff, 0x000000ff };
  const UInt32 hline_mask1[] =
    { 0xff000000, 0xffff0000, 0xffffff00, 0xffffffff };

  UInt32 mask, *ptr = (unsigned long *) (vidbuff + y * 160);
  int i;

  /* y in range of 0..159? */
  if (((unsigned int) y < DSP_HEIGHT) && (start < DSP_WIDTH) && (end >= 0)) {

    /* horizontal clipping */
    if (start < 0)
      start = 0;
    if (end > DSP_WIDTH - 1)
      end = DSP_WIDTH - 1;

    if (start > end) return;

    ptr += start >> 2;

    /* is the whole line within a single long word */
    if ((start & 0xfc) != (end & 0xfc)) {
      /* draw first word */
      mask = hline_mask0[start & 0x03];
      *ptr++ = (*ptr & ~mask) | (shade & mask);
      for (i = 1; i < ((end >> 2) - (start >> 2)); i++)
	*ptr++ = shade;
      mask = hline_mask1[end & 0x03];
      *ptr = (*ptr & ~mask) | (shade & mask);
    }
    else {
      mask = (hline_mask0[start & 0x03] & hline_mask1[end & 0x03]);
      *ptr = (*ptr & ~mask) | (shade & mask);
    }
  }
}

/* draw a horizontal line clipped in 3d view area */
static void
draw_hline_g16 (int y, int start, int end, UInt32 shade)
{
  const UInt32 hline_mask0[] =
    { 0xffffffff, 0x0fffffff, 0x00ffffff, 0x000fffff, 
      0x0000ffff, 0x00000fff, 0x000000ff, 0x0000000f };
  const UInt32 hline_mask1[] =
    { 0xf0000000, 0xff000000, 0xfff00000, 0xffff0000, 
      0xfffff000, 0xffffff00, 0xfffffff0, 0xffffffff };

  UInt32 mask, *ptr = (unsigned long *) (vidbuff + y * 80);
  int i;

  /* y in range of 0..159? */
  if (((unsigned int) y < DSP_HEIGHT) && (start < DSP_WIDTH) && (end >= 0)) {

    /* horizontal clipping */
    if (start < 0)
      start = 0;
    if (end > DSP_WIDTH - 1)
      end = DSP_WIDTH - 1;

    if (start > end) return;

    ptr += start >> 3;

    /* is the whole line within a single long word */
    if ((start & 0xf8) != (end & 0xf8)) {
      /* draw first word */
	mask = hline_mask0[start & 0x07];
	*ptr++ = (*ptr & ~mask) | (shade & mask);
	for (i = 1; i < ((end >> 3) - (start >> 3)); i++)
	  *ptr++ = shade;
	mask = hline_mask1[end & 0x07];
	*ptr = (*ptr & ~mask) | (shade & mask);
    } else {
      mask = (hline_mask0[start & 0x07] & hline_mask1[end & 0x07]);
      *ptr = (*ptr & ~mask) | (shade & mask);
    }
  }
}

void
draw_polygon (struct point2d *edges, UInt8 * coo, int edgenum, UInt8 color)
{
  int from, to, add, m, x, y;
  int e1, e2, i, dx, dy, *p;
  UInt32 col;

  const int edge_table[16] = { 0,0,0,1,1,1,2,2,2,3,3,3,4,4,4,5 };

  /* draw _konvex_ polygon in 3d area */
  static int bresen_buffer[DSP_HEIGHT][2];

  struct {
    int x, y;
  } polygon[MAX_EDGES];		/* buffer to save polygon */

  const UInt32 color32[] = {
    /* TD_COL_ENGINE   */ 0,

    /* TD_COL_BLACK    */ (MK_COLOR (0xff)),
    /* TD_COL_WHITE    */ (MK_COLOR (0x00)),
    /* TD_COL_WHITE_2  */ (MK_COLOR (0xdf)),

    /* TD_COL_RED      */ (MK_COLOR (0x77)),
    /* TD_COL_DARK_RED */ (MK_COLOR (0x8f)),
    /* TD_COL_RED_3    */ (MK_COLOR (0xa1)),
    /* TD_COL_RED_4    */ (MK_COLOR (0xb3)),

    /* TD_COL_GREY_1   */ (MK_COLOR (0xdd)),
    /* TD_COL_GREY_2   */ (MK_COLOR (0xdc)),
    /* TD_COL_GREY_3   */ (MK_COLOR (0xdb)),
    /* TD_COL_GREY_4   */ (MK_COLOR (0xda)),

    /* TD_COL_BLUE_1   */ (MK_COLOR (0x4b)),
    /* TD_COL_BLUE_2   */ (MK_COLOR (0x4d)),
    /* TD_COL_BLUE_3   */ (MK_COLOR (0x65)),
    /* TD_COL_BLUE_4   */ (MK_COLOR (0x6b)),

    /* TD_COL_YELLOW_1 */ (MK_COLOR (0x72)),
    /* TD_COL_YELLOW_2 */ (MK_COLOR (0x79)),

    /* TD_COL_GREEN_1  */ (MK_COLOR (0xd2)),
    /* TD_COL_GREEN_2  */ (MK_COLOR (0xd3)),
    /* TD_COL_GREEN_3  */ (MK_COLOR (0xd4)),
    /* TD_COL_GREEN_4  */ (MK_COLOR (0xd5)),

    /* TD_COL_PINK_1   */ (MK_COLOR (0x05)),

    /* not used in ships: */
    /* TD_COL_GREY_5   */ (MK_COLOR (0xd9)),
    /* TD_COL_GREY_6   */ (MK_COLOR (0xd8)),
    /* TD_COL_GREY_7   */ (MK_COLOR (0xd7)),
    /* TD_COL_GREY_0   */ (MK_COLOR (0xde))
  };

  const UInt32 engine_col[] = {
    /* TD_COL_RED      */ (MK_COLOR (0x77)),
    /* TD_COL_DARK_RED */ (MK_COLOR (0x8f)),
    /* TD_COL_RED_3    */ (MK_COLOR (0xa1)),
    /* TD_COL_RED_4    */ (MK_COLOR (0xb3)),
    /* TD_COL_RED_4    */ (MK_COLOR (0xb3)),
    /* TD_COL_RED_3    */ (MK_COLOR (0xa1)),
    /* TD_COL_DARK_RED */ (MK_COLOR (0x8f)),
    /* TD_COL_RED      */ (MK_COLOR (0x77))
  };

  /* copy edges */
  for (i = 0; i < edgenum; i++) {
    polygon[i].x = edges[coo[i]].x;
    polygon[i].y = edges[coo[i]].y;
  }

  /* check if polygon is visible */
  dy = 2 * (dx = edge_table[edgenum]);

  /* only draw if visible */
  e1 = polygon[dx].x - polygon[0].x;	/* e1 == x-offset X1->X2 */
  e2 = polygon[dx].y - polygon[0].y;	/* e2 == y-offset Y1->Y2 */

  if (e1 == 0) {
    if (((e2 < 0) && (polygon[dy].x <= polygon[0].x)) ||
	((e2 > 0) && (polygon[dy].x >= polygon[0].x)))
	return;
  }
  else {
    i = polygon[0].y + ((long)(polygon[dy].x - polygon[0].x)) * e2 / e1;

    if (((e1 > 0) && (polygon[dy].y <= i)) ||
	((e1 < 0) && (polygon[dy].y >= i)))
      return;
  }

  /* find lowest/highest y coordinate */
  e2 = e1 = 0;
  dy = dx = polygon[0].y;	/* first y coordinate */
  for (i = 1; i < edgenum; i++) {
    if (polygon[i].y < dx) {
      e1 = i;
      dx = polygon[i].y;
    }
    if (polygon[i].y > dy) {
      e2 = i;
      dy = polygon[i].y;
    }
  }

  /* e1/e2 now contains number of edge with lowest/highest y coordinate */

  /* now do two times bresenham from e1 to e2 */
  to = e1;
  y = polygon[e1].y;

  do {
    from = to;
    to = (to == 0) ? edgenum - 1 : to - 1;
    dy = polygon[to].y - polygon[from].y;
    if (polygon[to].x > polygon[from].x) {
      add = 1;
      dx = polygon[to].x - polygon[from].x;
    }
    else {
      add = -1;
      dx = polygon[from].x - polygon[to].x;
    }

    m = dx / 2;
    x = polygon[from].x;

    while (y < polygon[to].y) {
      while (m > dy) {
	m -= dy;
	x += add;
      }
      if ((unsigned int) y < DSP_HEIGHT)
	bresen_buffer[y++][0] = x;
      else
	y++;
      m += dx;
    }

  } while (to != e2);

  to = e1;
  y = polygon[e1].y;

  do {
    from = to;
    to = (to == edgenum - 1) ? 0 : to + 1;
    dy = polygon[to].y - polygon[from].y;
    if (polygon[to].x > polygon[from].x) {
      add = 1;
      dx = polygon[to].x - polygon[from].x;
    }
    else {
      add = -1;
      dx = polygon[from].x - polygon[to].x;
    }

    m = dx / 2;
    x = polygon[from].x;

    while (y < polygon[to].y) {
      while (m > dy) {
	m -= dy;
	x += add;
      }
      if ((unsigned int) y < DSP_HEIGHT)
	bresen_buffer[y++][1] = x;
      else
	y++;
      m += dx;
    }
  } while (to != e2);

  /* und draw it */
  if (color) col = color32[color];
  else       col = engine_col[engine_idx];

  e1 = polygon[e1].y; 
  if(e1 < 0)            e1 = 0;
  if(e1 > DSP_HEIGHT)   e1 = DSP_HEIGHT;

  e2 = polygon[e2].y;
  if(e2 < 0)            e2 = 0;
  if(e2 > DSP_HEIGHT)   e2 = DSP_HEIGHT;

  if(e2<=e1) return;

  p = bresen_buffer[e1];

  if(color_gfx) {
    for (; e1 < e2; e1++, p += 2)
      draw_hline_c256 (e1, p[0], p[1], col);
  } else {
    col = COL_EXPAND_32(graytab[col&0xff]);

    for (; e1 < e2; e1++, p += 2)
      draw_hline_g16 (e1, p[0], p[1], col);
  }
}

void
draw_filled_circle(int cx, int cy, int radius, UInt32 col)
{
  int counter = radius, xval = 0, yval = radius, line;
  int y0 = cy-radius, y1 = cy+radius;

  if(color_gfx) {
    do {
      if (counter < 0) {
	counter += 2 * --yval;
	
	draw_hline_c256(++y0, cx-xval+2, cx+xval-2, col);  /* green planet */
	draw_hline_c256(--y1, cx-xval+2, cx+xval-2, col);  /* green planet */
      } else
	counter -= 2 * xval++;
    }
    while (yval > 1);
    
    draw_hline_c256(++y0, cx-xval+2, cx+xval-2, col);  /* green planet */
  } else {
    col = COL_EXPAND_32(graytab[col&0xff]);

    do {
      if (counter < 0) {
	counter += 2 * --yval;
	
	draw_hline_g16(++y0, cx-xval+2, cx+xval-2, col);  /* green planet */
	draw_hline_g16(--y1, cx-xval+2, cx+xval-2, col);  /* green planet */
      } else
	counter -= 2 * xval++;
    }
    while (yval > 1);
    
    draw_hline_g16(++y0, cx-xval+2, cx+xval-2, col);  /* green planet */
  }
}

#ifdef DEPTH_SORT
void
z_map_qsort (struct z_map *base, Int16 nel)
{
  Int16 wgap, i, j;
  struct z_map *a, *b, tmp;

  if (nel > 1) {
    for (wgap = 0; ++wgap < (nel - 1) / 3; wgap *= 3);

    do {
      for (i = wgap; i < nel; i++) {
	for (j = i - wgap;; j -= wgap) {
	  if (base[j].z > base[j + wgap].z)
	    break;

	  /* otherweis swap items */
	  tmp = base[j + wgap];
	  base[j + wgap] = base[j];
	  base[j] = tmp;

	  if (j < wgap)
	    break;

	}
      }
      wgap = (wgap - 1) / 3;
    } while (wgap);
  }
}
#endif

static void
render_ship (struct univ_object *univ)
{
  RotMatrix trans_mat;
  int i, j, e1, e2;
#ifdef DEPTH_SORT
  int k;
#endif
  Int32 rz, rx, ry;
  PosVector vec;
  Int32 tmp;
  struct ship_point *ship_pnt;
  struct ship_face *face_data;
  struct ship_data *ship;
  struct z_map map[MAX_FACES];

  /* fetch ship data */
  ship = ship_list[univ->type];

  for (i = 0; i < 3; i++)
    trans_mat[i] = univ->rotmat[i];

  /* get pointer to various ship data */
  ship_pnt = (struct ship_point *) ((char *) ship +
				    sizeof (struct ship_data));
  face_data = (struct ship_face *) ((char *) ship_pnt +
				    sizeof (struct ship_point) *
				    ship->num_points);

//  DEBUG_PRINTF("dist = %ld (size=%d)\n", univ->location.z, 128*ship->size);

  /* display real ship or just a dot */
  if(univ->location.z > 256l*ship->size) {

    /* add viewer offset */
    rz =  univ->location.z;
    rx =  (320l * (univ->location.x))/rz + DSP_WIDTH/2;
    ry = -(320l * (univ->location.y))/rz + DSP_HEIGHT/2;

    draw_pixel(rx, ry, color_gfx?DIST_COLOR:DIST_GRAY);
    return;
  }

  /* invert matrix */
  tmp = trans_mat[0].y; trans_mat[0].y = trans_mat[1].x; trans_mat[1].x = tmp;
  tmp = trans_mat[0].z; trans_mat[0].z = trans_mat[2].x; trans_mat[2].x = tmp;
  tmp = trans_mat[1].z; trans_mat[1].z = trans_mat[2].y; trans_mat[2].y = tmp;

  for (i = 0; i < ship->num_points; i++) {

    /* get ship point info */
    vec.x = ship_pnt[i].x;
    vec.y = ship_pnt[i].y;
    vec.z = ship_pnt[i].z;

    /* rotate object around its own axes */
    mult_vector (&vec, trans_mat);

    /* add viewer offset */
    rz =  vec.z + univ->location.z;
    if(rz <= 0) rz = 1;

    rx =  (320l * (vec.x + univ->location.x))/rz + DSP_WIDTH/2;
    ry = -(320l * (vec.y + univ->location.y))/rz + DSP_HEIGHT/2;

    /* limit positions to Int16 range of point2d */

    /* convert for screen */
    point_list[i].x = rx;
    point_list[i].y = ry;
#ifdef DEPTH_SORT
    point_list[i].z = rz;
#endif
  }

#ifdef DEPTH_SORT
  /* search all faces for the smallest z coodinate */
  for (i = 0; i < ship->num_faces; i++) {
    for (tmp = 0, j = 0; j < face_data[i].points; j++) {
      if (tmp < point_list[face_data[i].p[j]].z)
	tmp = point_list[face_data[i].p[j]].z;
    }
    /* save max z koo of this polygon */
    map[i].z = tmp;
    map[i].poly = i;
  }

  /* now sort the depth map, hmm didn't 100% work as expected ... */
  z_map_qsort (map, ship->num_faces);

  /* draw all faces */
  for (i = 0; i < ship->num_faces; i++) {
    j = map[i].poly;

    if (face_data[j].points > 2) {
      draw_polygon (point_list, face_data[j].p,
		    face_data[j].points, face_data[j].colour);
    } else {
      /* check with third point if that line is visible */
      e1 = point_list[face_data[j].p[1]].x - point_list[face_data[j].p[0]].x;
      e2 = point_list[face_data[j].p[1]].y - point_list[face_data[j].p[0]].y;

      if (e1 == 0) {
	if (((e2 < 0)
	     && (point_list[face_data[j].p[2]].x <=
		 point_list[face_data[j].p[1]].x)) || ((e2 > 0) &&
		(point_list[face_data[j].p[2]].x >=point_list[face_data[j].p[1]].x)))
	  continue;

      }
      else {
	k = point_list[face_data[j].p[1]].y +
	  (point_list[face_data[j].p[2]].x -
	   point_list[face_data[j].p[1]].x) * e2 / e1;

	if (((e1 > 0) && (point_list[face_data[j].p[2]].y <= k)) ||
	    ((e1 < 0) && (point_list[face_data[j].p[2]].y >= k)))
	  continue;
      }

      draw_line_3d (point_list[face_data[j].p[0]].x,
		    point_list[face_data[j].p[0]].y,
		    point_list[face_data[j].p[1]].x,
		    point_list[face_data[j].p[1]].y);
    }
  }
#else
  /* draw all faces */
  for (i = 0; i < ship->num_faces; i++) {
    if (face_data[i].points > 2)
      draw_polygon (point_list, face_data[i].p,
		    face_data[i].points, face_data[i].colour);
    else {
      /* check with third point if that line is visible */
      e1 = point_list[face_data[i].p[1]].x - point_list[face_data[i].p[0]].x;
      e2 = point_list[face_data[i].p[1]].y - point_list[face_data[i].p[0]].y;

      if (e1 == 0) {
	if (((e2 < 0)
	     && (point_list[face_data[i].p[2]].x <=
		 point_list[face_data[i].p[1]].x)) || ((e2 > 0) &&
		(point_list[face_data[i].p[2]].x >= point_list[face_data[i].p[1]].x)))
	  continue;

      }
      else {
	j = point_list[face_data[i].p[1]].y +
	  (point_list[face_data[i].p[2]].x -
	   point_list[face_data[i].p[1]].x) * e2 / e1;

	if (((e1 > 0) && (point_list[face_data[i].p[2]].y <= j)) ||
	    ((e1 < 0) && (point_list[face_data[i].p[2]].y >= j)))
	  continue;
      }

      draw_line_3d (point_list[face_data[i].p[0]].x,
		    point_list[face_data[i].p[0]].y,
		    point_list[face_data[i].p[1]].x,
		    point_list[face_data[i].p[1]].y);
    }
  }
#endif

  if (univ->flags & FLG_FIRING) {
    /* point, the laser is atached to */
    struct point2d laser = point_list[ship_list[univ->type]->front_laser];

    if((laser.x >= 0) && (laser.x < DSP_WIDTH)&&
       (laser.y >= 0) && (laser.y < DSP_HEIGHT)) {
      
      /* draw a line from laser point to somewhere on right or left screen side */
      draw_line_3d(laser.x, laser.y, univ->location.x > 0 ? 0 : DSP_WIDTH - 1,
		   GALAXY_Y_SCALE (rand255 ()));
    }
  }
}

/*
 * Draw a planet.
 */

static void
draw_planet (struct univ_object *planet)
{
  int x, y;
  int radius;

  x =  (256l*planet->location.x)/planet->location.z;
  y = -(256l*planet->location.y)/planet->location.z;

  x = DSP_WIDTH/2 + GALAXY_SCALE(x);
  y = DSP_HEIGHT/2 + GALAXY_SCALE(y);

  radius = GALAXY_SCALE(6291456l / planet->distance);

  if ((x + radius < 0) || (x - radius >= DSP_WIDTH) || 
      (y + radius < 0) || (y - radius >= DSP_HEIGHT))
    return;

  /* planet is green */
  draw_filled_circle (x, y, radius, COL_EXPAND_32(0xd4));
}

static void
render_sun_line (int xo, int yo, int x, int y, int radius)
{
  int sy = yo + y;
  int sx, ex;
  int dx, dy;
  int distance;
  int inner, outer;
  int inner2;
  int mix;
  char *d =  vidbuff + (DSP_WIDTH * sy)/pixperbyte;

  if ((sy < 0) || (sy > DSP_HEIGHT-1))
    return;

  sx = xo - x;
  ex = xo + x;

  sx -= (radius * (2 + (randint () & 15))) >> 8;
  ex += (radius * (2 + (randint () & 15))) >> 8;

  if ((sx > DSP_WIDTH-1) || (ex < 0)) return;

  if (sx < 0) sx = 0;
  if (ex > DSP_WIDTH-1)  ex = DSP_WIDTH-1;

  inner = (radius * (200 + (randint () & 15))) >> 8;
  inner *= inner;

  inner2 = (radius * (220 + (randint () & 15))) >> 8;
  inner2 *= inner2;

  outer = (radius * (239 + (randint () & 15))) >> 8;
  outer *= outer;

  if(color_gfx) {
    /* draw color sun */
    dy = y * y;
    dx = sx - xo;
    d += sx;

    for (; sx <= ex; sx++, dx++) {
      mix = (sx ^ y) & 1;
      distance = dx * dx + dy;
      
      if (distance < inner)
	*d++ = GFX_COL_WHITE;
      else if (distance < inner2)
	*d++ = GFX_COL_YELLOW_4;
      else if (distance < outer)
	*d++ = GFX_ORANGE_3;
      else
	*d++ = mix ? GFX_ORANGE_1 : GFX_ORANGE_2;
    } 
  } else { 
    int mask_fg = (sx & 1)?0x0f:0xf0;
    int mask_bg = ~mask_fg;

    /* draw grayscale sun */
    dy = y * y;
    dx = sx - xo;
    d += sx/2;

    for (; sx <= ex; sx++, dx++) {
      mix = (sx ^ y) & 1;
      distance = dx * dx + dy;
      
      if (distance < inner)
	*d = (*d & mask_bg) | (mask_fg & 0x00);  // white
      else if (distance < inner2)
	*d = (*d & mask_bg) | (mask_fg & 0x33);  // yellow 4
      else if (distance < outer)
	*d = (*d & mask_bg) | (mask_fg & 0x55);  // orange 3
      else
	*d = (*d & mask_bg) | (mask_fg & (mix ? 0x77: 0x99));  // orange 1/2

      if(mask_fg == 0xf0) {
	mask_fg = 0x0f;
	mask_bg = 0xf0;
      } else {
	mask_fg = 0xf0;
	mask_bg = 0x0f;
	d++;
      }
    }
  }
}


static void
render_sun (int xo, int yo, int radius)
{
  int x, y;
  int s;

  s = -radius;
  x = radius;
  y = 0;

  // s -= x + x;
  while (y <= x) {
    render_sun_line (xo, yo, x, y, radius);
    render_sun_line (xo, yo, x, -y, radius);
    render_sun_line (xo, yo, y, x, radius);
    render_sun_line (xo, yo, y, -x, radius);

    s += y + y + 1;
    y++;
    if (s >= 0) {
      s -= x + x + 2;
      x--;
    }
  }
}

static void
draw_sun (struct univ_object *planet)
{
  int x, y;
  int radius;

  x =  (256*planet->location.x)/planet->location.z;
  y = -(256*planet->location.y)/planet->location.z;

  x = DSP_WIDTH/2 + GALAXY_SCALE(x);
  y = DSP_HEIGHT/2 + GALAXY_SCALE(y);

  radius = GALAXY_SCALE(6291456l / planet->distance);

  if ((x + radius < 0) || (x - radius >= DSP_WIDTH) || 
      (y + radius < 0) || (y - radius >= DSP_HEIGHT))
    return;

  render_sun (x, y, radius);
}



static void
draw_explosion (struct univ_object *univ)
{
  int radius;
  int x,y,n,x1,y1,c;
  UInt16 angle;
  UInt32 dist;

  /* 32 explosion shades */
  const UInt8 colors[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x06, 0x06, 0x0c, 0x0c, 0x6c, 0x6c, 0x6d, 
    0x6d, 0x72, 0x72, 0x73, 0x73, 0x74, 0x74, 0x75, 
    0x75, 0x76, 0x77, 0x7d, 0x8f, 0xa1, 0xb3, 0xc5};
    

  /* 
     nice filled polygon explosion: 
     separate all edges and move all polygons separately 
     and rotating from the center of the former ship
  */

  // some delta for the explosion is increasing
  if (univ->exp_delta > 230) {
    univ->flags |= FLG_REMOVE;
    return;
  }

  univ->exp_delta += 20;

  if (univ->location.z <= 0)
    return;

  /* draw explosion */
  /* a bunch of colorful pixels, depending on distance */

  x =  (256l*univ->location.x)/univ->location.z;
  y = -(256l*univ->location.y)/univ->location.z;

  radius = ((18000l / univ->distance) * univ->exp_delta) / 100;

  x = DSP_WIDTH/2 + GALAXY_SCALE(x);
  y = DSP_HEIGHT/2 + GALAXY_SCALE(y);

  /* draw a bunch of pixels */
  for(n=0;n<10*radius;n++) {
    angle = rand255()<<1;              // angle
    c = rand255();
    dist = ((UInt32)radius * c)/255;   // distance

    x1 = x - ((dist * sintab[angle])>>ROTBITS);                    // sin
    y1 = y - ((dist * sintab[SIN_STEPS/4 + angle])>>ROTBITS);      // cosin

    /* draw pixel, color depends distance inside radius */
    draw_pixel(x1, y1, colors[c>>3]);
  }
}



/*
 * Draws an object in the universe.
 * (Ship, Planet, Sun etc).
 */

void
draw_ship (struct univ_object *ship)
{
  if ((current_screen != SCR_FRONT_VIEW) 
      && (current_screen != SCR_REAR_VIEW)
      && (current_screen != SCR_LEFT_VIEW)
      && (current_screen != SCR_RIGHT_VIEW)
      && (current_screen != SCR_INTRO_ONE)
      && (current_screen != SCR_INTRO_TWO)
      && (current_screen != SCR_GAME_OVER)
      && (current_screen != SCR_ESCAPE_POD))
    return;

  if ( (ship->flags & FLG_DEAD) && 
      !(ship->flags & FLG_EXPLOSION)) {
    ship->flags |= FLG_EXPLOSION;
    ship->exp_delta = 18;
  }

  if (ship->flags & FLG_EXPLOSION) {
    draw_explosion (ship);
    return;
  }

  if (ship->location.z <= 0)	/* Only display ships in front of us. */
    return;

  if (ship->type == SHIP_PLANET) {
    draw_planet(ship);
    return;
  }

  if (ship->type == SHIP_SUN) {
    draw_sun(ship);
    return;
  }

  /* Check for field of vision. */
  if ((labs(ship->location.x) > ship->location.z) ||	
      (labs(ship->location.y) > ship->location.z))
    return;

  render_ship(ship);
}
