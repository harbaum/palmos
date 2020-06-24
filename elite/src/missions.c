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
 * missions.c
 *
 * Code to handle the special missions.
 */

#include <PalmOS.h>

#include "config.h"
#include "elite.h"
#include "gfx.h"
#include "vector.h"
#include "space.h"
#include "planet.h"
#include "main.h"
#include "missions.h"
#include "keyboard.h"
#include "debug.h"

static void draw_message_head(int num, int of);
static Bool wait_for_button(void);

static void constrictor_mission_brief (void);
static void constrictor_mission_debrief (void);
static void thargoid_mission_first_brief (void);
static void thargoid_mission_second_brief (void);
static void thargoid_mission_debrief (void);

static void
draw_message_head(int num, int of) {
  char str[24];
  
  StrCopy(str, get_text(TXT_INCOMING));

  if(num != 0)
    StrPrintF(str+16, " - %d/%d", num, of);

  gfx_draw_title(str, 1);
}

static Bool
wait_for_button(void) {

  gfx_display_text(DSP_CENTER, DSP_HEIGHT - GFX_CELLS (1, 1), ALIGN_CENTER,
		   get_text(TXT_BUT_CONT), GFX_COL_GOLD);
  gfx_update_screen ();

  do {
    kbd_poll_keyboard ();
  } while (!(kbd_button_map & KBD_TAP) && (!finish));

  return !finish;
}

char *
mission_planet_desc (struct galaxy_seed planet)
{
  int pnum;

  if(!is_docked) return NULL;

  /* have to have been at these six planets */
  if ((planet.a != docked_planet.a) ||
      (planet.b != docked_planet.b) ||
      (planet.c != docked_planet.c) ||
      (planet.d != docked_planet.d) ||
      (planet.e != docked_planet.e) || 
      (planet.f != docked_planet.f))
    return NULL;

  /* find number of current planet */
  pnum = find_planet_number (planet);

  if (cmdr.galaxy_number == 0) {
    switch (pnum) {
    case 150:
      return get_text(TXT_MPD1);

    case 36:
      return get_text(TXT_MPD2);

    case 28:
      return get_text(TXT_MPD3);
    }
  }

  if (cmdr.galaxy_number == 1) {
    switch (pnum) {
    case 32:
    case 68:
    case 164:
    case 220:
    case 106:
    case 16:
    case 162:
    case 3:
    case 107:
    case 26:
    case 192:
    case 184:
    case 5:
      return get_text(TXT_MPD4);

    case 253:
      return get_text(TXT_MPD5);

    case 79:
      return get_text(TXT_MPD6);

    case 53:
      return get_text(TXT_MPD7);

    case 118:
      return get_text(TXT_MPD8);

    case 193:
      return get_text(TXT_MPD9);
    }
  }

  if ((cmdr.galaxy_number == 2) && (pnum == 101))
    return get_text(TXT_MPD10);

  return NULL;
}

#ifdef LIMIT
void mission_limit(void) {
  draw_message_head(0,0);

  gfx_display_pretty_text(2, TITLE_LINE_Y+2, 
			  158, DSP_HEIGHT-2*GFX_CELL_HEIGHT, 
			  1, get_text(TXT_LIMIT));

  if(!wait_for_button()) return;
}
#endif

static void
constrictor_mission_brief(void)
{
  RotMatrix rotmat;

  cmdr.mission = 1;

  current_screen = SCR_FRONT_VIEW;

  /* show first part of message */
  draw_message_head(1,3);
  gfx_display_pretty_text(2, TITLE_LINE_Y+2, 
			  158, DSP_HEIGHT-2*GFX_CELL_HEIGHT, 
			  1, get_text(TXT_CMBA));
  if(!wait_for_button()) return;

  /* show whip on second screen */
  clear_universe();
  set_init_matrix(rotmat);
  add_new_ship(SHIP_CONSTRICTOR, 0, 0, 250, rotmat, -127, -127);
  universe[0].flags = 0;
  
  flight_roll  = FLIGHT_SCALE(0);
  flight_climb = FLIGHT_SCALE(0);
  flight_speed = FLIGHT_SCALE(0);

  do {
    draw_message_head(2,3);
    update_universe ();
    universe[0].location.z = 250;
    gfx_display_text(DSP_CENTER, DSP_HEIGHT - GFX_CELLS (2, 1), ALIGN_CENTER,
		     "The Constrictor", GFX_COL_RED);
    gfx_display_text(DSP_CENTER, DSP_HEIGHT - GFX_CELLS (1, 1), ALIGN_CENTER,
		     get_text(TXT_BUT_CONT), GFX_COL_GOLD);
    gfx_update_screen ();
    kbd_poll_keyboard ();
  } while (!(kbd_button_map & KBD_TAP) && (!finish));

  if(finish) return;

  /* show third part of message */
  draw_message_head(3,3);
  gfx_display_pretty_text(2, TITLE_LINE_Y+2, 
			  158, DSP_HEIGHT-2*GFX_CELL_HEIGHT, 1,
			  get_text((cmdr.galaxy_number == 0) ? 
				   TXT_CMBB : TXT_CMBC));
  wait_for_button();
}


static void
constrictor_mission_debrief (void)
{
  cmdr.mission = 3;
  cmdr.score += 256;
  cmdr.credits += 50000;

  draw_message_head(0,0);

  gfx_display_text(DSP_CENTER, TITLE_LINE_Y+10, ALIGN_CENTER,
		   get_text(TXT_CONGRAT), GFX_COL_GOLD);

  gfx_display_pretty_text(4, TITLE_LINE_Y+30,
			  158, DSP_HEIGHT-2*GFX_CELL_HEIGHT, 
			  1, get_text(TXT_CMD));
  wait_for_button();
}


static void
thargoid_mission_first_brief (void)
{
  cmdr.mission = 4;

  draw_message_head(0,0);

  gfx_draw_sprite(SPRITE_FORT, 2, TITLE_LINE_Y+2);
  gfx_display_pretty_text(52, TITLE_LINE_Y+2, 
			  158, DSP_HEIGHT-2*GFX_CELL_HEIGHT, 
			  1, get_text(TXT_TMFB));
  wait_for_button();
}


static void
thargoid_mission_second_brief (void)
{
  cmdr.mission = 5;

  /* draw first part of message */
  draw_message_head(1,2);
  gfx_draw_sprite(SPRITE_BLAKE, 2, TITLE_LINE_Y+2);
  gfx_display_pretty_text(52, TITLE_LINE_Y+2, 
			  158, DSP_HEIGHT-2*GFX_CELL_HEIGHT, 
			  1, get_text(TXT_TMSBA));
  if(!wait_for_button()) return;

  /* draw second part of message */
  draw_message_head(2,2);
  gfx_display_pretty_text(2, TITLE_LINE_Y+2, 
			  158, DSP_HEIGHT-2*GFX_CELL_HEIGHT, 
			  1, get_text(TXT_TMSBB));
  wait_for_button();
}

static void
thargoid_mission_debrief (void)
{
  cmdr.mission = 6;
  cmdr.score += 256;
  cmdr.energy_unit = 2;

  draw_message_head(0,0);

  gfx_display_text(DSP_CENTER, TITLE_LINE_Y+10, ALIGN_CENTER,
		   get_text(TXT_WELLDONE), GFX_COL_GOLD);

  gfx_display_pretty_text (4, TITLE_LINE_Y+30,
			   158, DSP_HEIGHT-2*GFX_CELL_HEIGHT, 
			   1, get_text(TXT_TMDB));
  wait_for_button();
}


void
check_mission_brief (void)
{
  if ((cmdr.mission == 0) && (cmdr.score >= 256) && (cmdr.galaxy_number < 2)) {
    constrictor_mission_brief ();
    return;
  }

  if (cmdr.mission == 2) {
    constrictor_mission_debrief ();
    return;
  }

  if ((cmdr.mission == 3) && (cmdr.score >= 1280)
      && (cmdr.galaxy_number == 2)) {
    thargoid_mission_first_brief ();
    return;
  }

  if ((cmdr.mission == 4) && (docked_planet.d == 215)
      && (docked_planet.b == 84)) {
    thargoid_mission_second_brief ();
    return;
  }

  if ((cmdr.mission == 5) && (docked_planet.d == 63)
      && (docked_planet.b == 72)) {
    thargoid_mission_debrief ();
    return;
  }
}

