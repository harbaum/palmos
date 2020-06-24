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

#include "config.h"
#include "elite.h"
#include "gfx.h"
#include "vector.h"
#include "shipdata.h"
#include "threed.h"
#include "space.h"
#include "stars.h"
#include "intro.h"
#include "debug.h"
#include "dialog.h"
#include "options.h"

static int ship_no;
static int show_time;
static int direction;

/* ships:
  1  - missile    2  - coriolis    3  - esccaps     4  - alloy
  5  - cargo      6  - boulder     7  - asteroid    8  - rock
  9  - orbit      10 - transp      11 - cobra3a     12 - pythona
  13 - boa        14 - anacnda     15 - hermit      16 - viper
  17 - sidewnd    18 - mamba       19 - krait       20 - adder
  21 - gecko      22 - cobra1      23 - worm        24 - moray
  25 - cobra3b    26 - asp2        27 - pythonb     28 - ferdlce     
  29 - thargoid   30 - thargon     31 - constrct    32 - cougar
  33 - dodec
*/

#ifdef SHOW_ALL_SHIPS		//spoiler
static const int min_dist[NO_OF_SHIPS + 1] = { 
  0,
  200, 800, 200, 200, 200, 300, 384, 200,  // MISSILE -> ROCK
  200, 200, 420, 900, 500, 800, 384, 384,  // SHUTTLE -> VIPER
  384, 384, 384, 200, 384, 384, 384, 384,  // SIDEWINDER -> MORAY
  384, 384, 900, 384, 700, 384, 384, 384,  // COBRA3_LONE -> COUGAR
  900                                      // DODEC
};
#else
static const int min_dist[NO_OF_SHIPS + 1] = { 
  0,
  200, 800, 200,   0, 200,   0, 384,   0,  // MISSILE -> ROCK
  200, 200, 420, 900, 500, 800, 384, 384,  // SHUTTLE -> VIPER
  384, 384, 384, 200, 384, 384, 384, 384,  // SIDEWINDER -> MORAY
    0, 384,   0, 384, 700, 384,   0,   0,  // COBRA3_LONE -> COUGAR
  900                                      // DODEC
};
#endif

static RotMatrix intro_ship_matrix;

#ifdef INTRO1
void
initialise_intro1 (void)
{
  clear_universe ();
  set_init_matrix (intro_ship_matrix);
  add_new_ship (SHIP_COBRA3, 0, 0, 4500, intro_ship_matrix, -127, -127);
}
#endif

#ifdef INTRO2
void
initialise_intro2 (void)
{
  ship_no = 1;			// 1 = start with missile
  show_time = 0;
  direction = -100;

  clear_universe ();
  create_new_stars ();
  set_init_matrix (intro_ship_matrix);
  add_new_ship (ship_no, 0, 0, 4500, intro_ship_matrix, -127, -127);
}
#endif

#ifdef INTRO1
void
update_intro1 (void)
{
  universe[0].location.z -= 100;

  if (universe[0].location.z < 384)
    universe[0].location.z = 384;

  gfx_clear_display();

  flight_roll = FLIGHT_SCALE(1);
  update_universe ();

  gfx_display_text(DSP_CENTER, DSP_HEIGHT - GFX_CELLS (5, 1), ALIGN_CENTER,
      get_text(TXT_BUT_CONT), GFX_COL_GOLD);

  gfx_display_text(DSP_CENTER, DSP_HEIGHT - GFX_CELLS (3, 1), ALIGN_CENTER,
      "\275 1984 I.Bell & D.Braben", GFX_COL_WHITE);
  gfx_display_text(DSP_CENTER, DSP_HEIGHT - GFX_CELLS (2, 1), ALIGN_CENTER,
      "PalmOS Version \275 2002 T.Harbaum", GFX_COL_WHITE);
  gfx_display_text(DSP_CENTER, DSP_HEIGHT - GFX_CELLS (1, 1), ALIGN_CENTER,
      "http:\014\014www.harbaum.org\014till\014palm\014elite", GFX_COL_WHITE);
}
#endif

#ifdef INTRO2
void
update_intro2 (void)
{
  show_time++;

  /* reverse movement after some time */
  if ((show_time >= 140) && (direction < 0))
    direction = -direction;

  /* move the ship */
  universe[0].location.z += direction;

  /* prevent ships from passing through the viewer */
  if (universe[0].location.z < min_dist[ship_no])
    universe[0].location.z = min_dist[ship_no];

  /* ship is far away */
  if (universe[0].location.z > 4500) {

    /* cycle through all ships */
    do {
      ship_no++;
      if (ship_no > NO_OF_SHIPS)
	ship_no = 1;
    } while (min_dist[ship_no] == 0);

    show_time = 0;
    direction = -100;

    ship_count[universe[0].type] = 0;	// force remove old ship
    universe[0].type = 0;

    add_new_ship (ship_no, 0, 0, 4500, intro_ship_matrix, -127, -127);
  }

  gfx_clear_display();
  update_starfield ();
  update_universe ();

  /* only ask user to tap screen, if no name dialog is being displayed */
  if(!dialog_btn_active) {
    gfx_display_text(DSP_CENTER, DSP_HEIGHT - 1 - GFX_CELLS (1, 1), 
		     ALIGN_CENTER, get_text(TXT_BUT_CONT), GFX_COL_GOLD);
  }

  gfx_display_text(DSP_CENTER, DSP_HEIGHT - 1 - GFX_CELLS (2, 1), ALIGN_CENTER,
		   (char *)ship_list[ship_no]->name, GFX_COL_WHITE);
}
#endif
