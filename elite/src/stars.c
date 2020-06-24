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
 * stars.c
 */

#include <PalmOS.h>

#include "config.h"
#include "elite.h"
#include "gfx.h"
#include "vector.h"
#include "stars.h"
#include "random.h"
#include "debug.h"
#include "threed.h"
#include "options.h"

#ifdef BLOBS
#define LIMIT_2  0x16000   // stars made of two pixels
#define LIMIT_4  0x0c000   // stars made of four pixels
#define LIMIT_6  0x08000   // stars made of six pixels
#define LIMIT_9  0x04000   // stars made of nine pixels
#else
#define LIMIT_2  0xc000    // stars made of two pixels
#define LIMIT_4  0x8000    // stars made of four pixels
#endif

static void create_new_star (int) SEC3D;
static void front_starfield (void) SEC3D;
static void rear_starfield (void) SEC3D;
static void side_starfield (void) SEC3D;

int warp_stars;

struct star {
  Int16 x, y;
  UInt16 z;
};

#define SLOW_STARS   (12)
#define WITCH_STARS  (3)

struct star stars[SLOW_STARS];

#define STAR_WIDTH       (DSP_WIDTH-(20*256/DSP_WIDTH))
#define STAR_HEIGHT      (DSP_HEIGHT-(20*256/DSP_WIDTH))

#define STAR_N_WIDTH     (DSP_WIDTH-2)
#define STAR_N_HEIGHT    (DSP_HEIGHT-2)

static void
create_new_star (int i)
{
  stars[i].x = rand255 () * STAR_WIDTH - 128 * STAR_WIDTH;
  stars[i].y = rand255 () * STAR_HEIGHT - 128 * STAR_HEIGHT;

  stars[i].x += ((stars[i].x < 0) ? -1 : 1) * 256 * 8;
  stars[i].y += ((stars[i].y < 0) ? -1 : 1) * 256 * 4;

  stars[i].z = 256 * (rand255 () | 0x90);
}

void
create_new_stars (void)
{
  int i;
  int nstars;

  nstars = witchspace ? WITCH_STARS : SLOW_STARS;

  for (i = 0; i < nstars; i++)
    create_new_star (i);

  warp_stars = 0;
}

static void
front_starfield (void) {
  int i;
  UInt32 Q;
  Int16 delta, alpha, beta;
  Int16 xx, yy;
  UInt16 zz;
  int sx;
  int sy;
  int nstars;

  nstars = witchspace ? WITCH_STARS : SLOW_STARS;

  delta = 128 * (warp_stars ? 50 : FLIGHT_DESCALE_TICKS(flight_speed));
  alpha = FLIGHT_DESCALE_TICKS(flight_roll);
  beta = 256 * FLIGHT_DESCALE_TICKS(flight_climb);

  for (i = 0; i < nstars; i++) {
    /* Plot the stars in their current locations... */

    sx = stars[i].x / 256 + DSP_WIDTH / 2;
    sy = stars[i].y / 256 + DSP_HEIGHT / 2;
    zz = stars[i].z;

    if (!warp_stars) {

      /* far away: one pixel */
      gfx_plot_pixel   (sx,     sy,     GFX_COL_WHITE);

      /* a little closer: two pixels */
      if (zz < LIMIT_2)
	gfx_plot_pixel (sx + 1, sy    , GFX_COL_WHITE);

      /* very close: four pixels */
      if (zz < LIMIT_4) {
	gfx_plot_pixel (sx    , sy + 1, GFX_COL_WHITE);
	gfx_plot_pixel (sx + 1, sy + 1, GFX_COL_WHITE);
      }

#ifdef BLOBS
      /* even closer: six pixels */
      if (zz < LIMIT_6) {
	gfx_plot_pixel (sx + 2, sy    , GFX_COL_WHITE);
	gfx_plot_pixel (sx + 2, sy + 1, GFX_COL_WHITE);
      }

      /* super close: nine pixels */
      if (zz < LIMIT_9) {
	gfx_plot_pixel (sx    , sy + 2, GFX_COL_WHITE);
	gfx_plot_pixel (sx + 1, sy + 2, GFX_COL_WHITE);
	gfx_plot_pixel (sx + 2, sy + 2, GFX_COL_WHITE);
      }
#endif
    }

    /* Move the stars to their new locations... */
    Q = 256l * delta / stars[i].z;

    stars[i].z -= delta;
    xx = stars[i].x + (stars[i].x * Q) / 256l;
    yy = stars[i].y + (stars[i].y * Q) / 256l;
    zz = stars[i].z;

    yy = yy + (((long)xx * (long)alpha) / 256);
    xx = xx - (((long)yy * (long)alpha) / 256);

/*
  tx = yy * beta;
  xx = xx + (tx * tx * 2);
*/
    yy = yy + beta;

    stars[i].x = xx;
    stars[i].y = yy;

    xx = xx / 256 + DSP_WIDTH / 2;
    yy = yy / 256 + DSP_HEIGHT / 2;

    if ((xx < GFX_VIEW_TX) || (xx >= GFX_VIEW_BX) ||
	(yy < GFX_VIEW_TY) || (yy >= GFX_VIEW_BY) || (zz < 256 * 16)) {

      create_new_star (i);
    } else if (warp_stars)
      draw_line_3d(sx, sy, xx, yy);
  }

  warp_stars = 0;
}


static void
rear_starfield (void)
{
  int i, k;
  UInt32 Q;
  Int16 delta, alpha, beta;
  Int16 xx, yy;
  UInt32 zz;
  int sx, sy;
  int nstars;

  nstars = witchspace ? WITCH_STARS : SLOW_STARS;

  delta = 128 * (warp_stars ? 50 : FLIGHT_DESCALE_TICKS(flight_speed));
  alpha = FLIGHT_DESCALE_TICKS(-flight_roll);
  beta = FLIGHT_DESCALE_TICKS(-flight_climb) * 256;

  for (i = 0; i < nstars; i++) {
    /* Plot the stars in their current locations... */

    sx = stars[i].x / 256 + DSP_WIDTH / 2;
    sy = stars[i].y / 256 + DSP_HEIGHT / 2;
    zz = stars[i].z;

    if (!warp_stars) {
      /* far away: one pixel */
      gfx_plot_pixel   (sx,     sy,     GFX_COL_WHITE);

      /* a little closer: two pixels */
      if (zz < LIMIT_2)
	gfx_plot_pixel (sx + 1, sy    , GFX_COL_WHITE);

      /* very close: four pixels */
      if (zz < LIMIT_4) {
	gfx_plot_pixel (sx    , sy + 1, GFX_COL_WHITE);
	gfx_plot_pixel (sx + 1, sy + 1, GFX_COL_WHITE);
      }

#ifdef BLOBS
      /* even closer: six pixels */
      if (zz < LIMIT_6) {
	gfx_plot_pixel (sx + 2, sy    , GFX_COL_WHITE);
	gfx_plot_pixel (sx + 2, sy + 1, GFX_COL_WHITE);
      }

      /* super close: nine pixels */
      if (zz < LIMIT_9) {
	gfx_plot_pixel (sx    , sy + 2, GFX_COL_WHITE);
	gfx_plot_pixel (sx + 1, sy + 2, GFX_COL_WHITE);
	gfx_plot_pixel (sx + 2, sy + 2, GFX_COL_WHITE);
      }
#endif
    }

    /* Move the stars to their new locations... */
    Q = 256l * delta / stars[i].z;

    zz += delta;
    stars[i].z = (zz > 65535) ? 65525 : zz;
    xx = stars[i].x - (stars[i].x * Q) / 256l;
    yy = stars[i].y - (stars[i].y * Q) / 256l;

    yy = yy + (((Int32)xx * (Int32)alpha) / 256);
    xx = xx - (((Int32)yy * (Int32)alpha) / 256);

    yy = yy + beta;

    stars[i].y = yy;
    stars[i].x = xx;

    xx = xx / 256 + DSP_WIDTH / 2;
    yy = yy / 256 + DSP_HEIGHT / 2;

    if ((xx < GFX_VIEW_TX) || (xx >= GFX_VIEW_BX) ||
	(yy < GFX_VIEW_TY) || (yy >= GFX_VIEW_BY) || (zz > 65535)) {

      k = rand255 ();
      stars[i].z = 256 * ((k & 127) + 51);

      if (k & 1) {
	stars[i].x = rand255 () * STAR_N_WIDTH - 128 * DSP_WIDTH;
	stars[i].y =
	  (rand255 () & 1) ? (-STAR_N_HEIGHT * 128) : (STAR_N_HEIGHT * 128) -
	  1;
      }
      else {
	stars[i].x =
	  (rand255 () & 1) ? (-STAR_N_WIDTH * 128) : (STAR_N_WIDTH * 128 - 1);
	stars[i].y = rand255 () * STAR_N_HEIGHT - 128 * DSP_HEIGHT;
      }
    }
    else {
      if (warp_stars)
	draw_line_3d(sx, sy, xx, yy);
    }
  }
  warp_stars = 0;
}

static void
side_starfield (void)
{
  int i, k;
  Int16 delta, alpha, beta;
  Int16 xx, yy;
  UInt16 zz;
  int sx, sy;
  int nstars;
  Int16 delt8;

  nstars = witchspace ? WITCH_STARS : SLOW_STARS;

  delta = 128 * (warp_stars ? 50 : FLIGHT_DESCALE_TICKS(flight_speed));
  alpha = FLIGHT_DESCALE_TICKS(flight_roll);
  beta = FLIGHT_DESCALE_TICKS(flight_climb);

  if (current_screen == SCR_LEFT_VIEW) {
    delta = -delta;
    alpha = -alpha;
    beta = -beta;
  }

  for (i = 0; i < nstars; i++) {
    sx = stars[i].x / 256 + DSP_WIDTH / 2;
    sy = stars[i].y / 256 + DSP_HEIGHT / 2;
    zz = stars[i].z;

    if (!warp_stars) {
      /* far away: one pixel */
      gfx_plot_pixel   (sx,     sy,     GFX_COL_WHITE);

      /* a little closer: two pixels */
      if (zz < LIMIT_2)
	gfx_plot_pixel (sx + 1, sy    , GFX_COL_WHITE);

      /* very close: four pixels */
      if (zz < LIMIT_4) {
	gfx_plot_pixel (sx    , sy + 1, GFX_COL_WHITE);
	gfx_plot_pixel (sx + 1, sy + 1, GFX_COL_WHITE);
      }

#ifdef BLOBS
      /* even closer: six pixels */
      if (zz < LIMIT_6) {
	gfx_plot_pixel (sx + 2, sy    , GFX_COL_WHITE);
	gfx_plot_pixel (sx + 2, sy + 1, GFX_COL_WHITE);
      }

      /* super close: nine pixels */
      if (zz < LIMIT_9) {
	gfx_plot_pixel (sx    , sy + 2, GFX_COL_WHITE);
	gfx_plot_pixel (sx + 1, sy + 2, GFX_COL_WHITE);
	gfx_plot_pixel (sx + 2, sy + 2, GFX_COL_WHITE);
      }
#endif
    }

    yy = stars[i].y;
    xx = stars[i].x;
    zz = stars[i].z;

    delt8 = 256l * delta / (zz / 32);
    xx = xx + delt8;

    /* circular (climb) */
    xx += ((Int32)yy * (Int32)beta) >> 8;
    yy -= ((Int32)xx * (Int32)beta) >> 8;

    /* vertical (roll) */
    xx += ((yy * (Int32)alpha)/256l * (-xx))>>16;
    yy += ((yy * (Int32)alpha)/256l *  (yy))>>16;

    yy += 256*alpha;
    
    stars[i].y = yy;
    stars[i].x = xx;

    /* transform xx for screen display */
    xx = xx / 256 + DSP_WIDTH / 2;
    yy = yy / 256 + DSP_HEIGHT / 2;

    if ((xx < GFX_VIEW_TX) || (xx >= GFX_VIEW_BX-1)) {
      stars[i].y = rand255 () * STAR_N_HEIGHT - 128 * DSP_HEIGHT;
      stars[i].x = (delta<0)?(STAR_N_WIDTH*128)-1:(-STAR_N_WIDTH*128);
      stars[i].z = 256 * (rand255 () | 8);
    } else if ((yy < GFX_VIEW_TY) || (yy >= GFX_VIEW_BY-1)) {
      stars[i].y = (alpha<0)?(STAR_N_HEIGHT*128)-1:(-STAR_N_HEIGHT*128);
      stars[i].x = rand255 () * STAR_N_WIDTH - 128 * DSP_WIDTH;
      stars[i].z = 256 * (rand255 () | 8);
    } else if (warp_stars)
      draw_line_3d(sx, sy, xx, yy);
  }
  warp_stars = 0;
}


/*
 * When we change view, flip the stars over so they look like other stars.
 */

void
flip_stars (void)
{
  int i;
  int nstars;
  Int32 sx, sy;

  nstars = witchspace ? WITCH_STARS : SLOW_STARS;
  for (i = 0; i < nstars; i++) {
    sy = stars[i].y;
    sx = stars[i].x;
    stars[i].x = sy * DSP_WIDTH / (DSP_HEIGHT + 1);
    stars[i].y = sx * DSP_HEIGHT / (DSP_WIDTH + 1);
  }
}


void
update_starfield (void)
{
  switch (current_screen) {
  case SCR_FRONT_VIEW:
  case SCR_INTRO_ONE:
  case SCR_INTRO_TWO:
  case SCR_ESCAPE_POD:
    front_starfield ();
    break;

  case SCR_REAR_VIEW:
  case SCR_GAME_OVER:
    rear_starfield ();
    break;

  case SCR_LEFT_VIEW:
  case SCR_RIGHT_VIEW:
    side_starfield ();
    break;
  }
}
