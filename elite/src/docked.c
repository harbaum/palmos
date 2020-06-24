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
#include "sections.h"

#include "config.h"
#include "gfx.h"
#include "elite.h"
#include "planet.h"
#include "shipdata.h"
#include "space.h"
#include "debug.h"
#include "docked.h"
#include "random.h"
#include "options.h"
#include "sound.h"

#ifdef SBAY
#include "sbay.h"
#endif

static void SECDK show_distance (int, struct galaxy_seed, struct galaxy_seed);
static void SECDK draw_fuel_limit_circle (int, int);
static void SECDK display_stock_price (int);
static void SECDK display_equip_price (int);
static void SECDK highlight_equip (int);
static void SECDK list_equip_prices (void);
static void SECDK collapse_equip_list (void);
static int  SECDK laser_refund (int);
static void SECDK display_market_title (void);
static void SECDK draw_planet(int x, int y);
static void SECDK plasma(Int8 *planet, int xl, int xh, int yl, int yh);
static void SECDK planet_base(Int8 *planet, int xl, int xh, int yl, int yh);
static void SECDK draw_cross (int cx, int cy);

int cross_x = 0;
int cross_y = 0;

static void
draw_fuel_limit_circle (int cx, int cy)
{
  int radius;

  if (current_screen == SCR_GALACTIC_CHART)
    radius = 2 + GALAXY_SCALE (cmdr.fuel / 4);
  else
    radius = 2 + GALAXY_SCALE (cmdr.fuel);

  gfx_draw_circle (cx, cy, radius, GFX_COL_GREEN_1);

//  gfx_draw_line (cx, cy - 2, cx, cy + 2);
//  gfx_draw_line (cx - 2, cy, cx + 2, cy);
}

int
calc_distance_to_planet (struct galaxy_seed from_planet,
			 struct galaxy_seed to_planet)
{
  long dx, dy;
  int light_years;

  dx = abs (to_planet.d - from_planet.d);
  dy = abs (to_planet.b - from_planet.b);

  dx = dx * dx;
  dy = dy / 2;
  dy = dy * dy;

  light_years = int_sqrt (dx + dy);
  light_years *= 4;

  return light_years;
}


static void
show_distance (int ypos, struct galaxy_seed from_planet,
	       struct galaxy_seed to_planet)
{
  char str[36];
  int light_years;

  light_years = calc_distance_to_planet (from_planet, to_planet);

  if (light_years > 0)
    StrPrintF (str, "Distance: %d.%d Light Years ",
	       light_years / 10, light_years % 10);
  else
    StrCopy (str, "Present System");

  gfx_clear_area (0, ypos, 120, ypos + GFX_CELL_HEIGHT - 1);
  gfx_display_text(2, ypos, ALIGN_LEFT, str, GFX_COL_WHITE);
}

/*
 * Draw the cross hairs at the specified position.
 */

static void
draw_cross (int cx, int cy)
{
  UInt8 color;

  if(color_gfx) color = GFX_COL_RED    ^ GFX_COL_BLACK;
  else          color = 0xaa;

  if (current_screen == SCR_SHORT_RANGE) {
    gfx_set_clip_region (0, TITLE_LINE_Y + 1, 159, DSP_HEIGHT - 1);
    gfx_xor_colour_line (cx - 6, cy, cx + 6, cy, color);
    gfx_xor_colour_line (cx, cy - 6, cx, cy + 6, color);
  }
  else if (current_screen == SCR_GALACTIC_CHART) {
    gfx_set_clip_region (0, TITLE_LINE_Y + 1, 159, TITLE_LINE_Y + 82);
    gfx_xor_colour_line (cx - 3, cy, cx + 3, cy, color);
    gfx_xor_colour_line (cx, cy - 3, cx, cy + 3, color);
  }
  gfx_set_clip_region (0, 0, 0, 0);
}


#define INFO_Y_GAL  94
#define INFO_Y_LOC  97

void
show_distance_to_planet (void)
{
  int px, py, dy;
  char planet_name[16];
  char str[32];

  if (current_screen == SCR_GALACTIC_CHART) {
    px = GALAXY_UNSCALE (cross_x);
    py = GALAXY_Y_UNSCALE (cross_y);
    dy = INFO_Y_GAL;
  }
  else {
    px = GALAXY_UNSCALE(((cross_x - GFX_X_CENTRE)/4)) + docked_planet.d;
    py = GALAXY_UNSCALE(((cross_y - GFX_Y_CENTRE)/2)) + docked_planet.b;
    dy = INFO_Y_LOC;
  }

//  DEBUG_PRINTF("search at %d/%d ", px,py);
  hyperspace_planet = find_planet (px, py);
//  DEBUG_PRINTF("-> %d/%d\n", hyperspace_planet.d, hyperspace_planet.b);

  name_planet (planet_name, hyperspace_planet);
  capitalise_name (planet_name);

  if (current_screen == SCR_GALACTIC_CHART)
    gfx_clear_area (0, dy, 120, dy + GFX_CELL_HEIGHT - 1);
  else 
    gfx_clear_area (0, dy, 65, dy + GFX_CELL_HEIGHT - 1);

  StrPrintF (str, "%-18s", planet_name);
  gfx_display_text(2, dy, ALIGN_LEFT, str, GFX_COL_WHITE);

  show_distance (dy + GFX_CELL_HEIGHT +1, docked_planet, hyperspace_planet);

  if (current_screen == SCR_GALACTIC_CHART) {
    cross_x = GALAXY_SCALE ((int) hyperspace_planet.d);
    cross_y = GALAXY_Y_SCALE ((int) hyperspace_planet.b);
  } else {
    cross_x = (GALAXY_SCALE ((hyperspace_planet.d - docked_planet.d)*4))
      + GFX_X_CENTRE;
    cross_y = (GALAXY_SCALE ((hyperspace_planet.b - docked_planet.b)*2))
      + GFX_Y_CENTRE;
  }

  if ((cross_x != old_cross_x) || (cross_y != old_cross_y)) {
    if (old_cross_x != -100)
      draw_cross (old_cross_x, old_cross_y);
    
    old_cross_x = cross_x;
    old_cross_y = cross_y;
    
    draw_cross (old_cross_x, old_cross_y);
  }
}

void
find_planet_by_name (char *find_name)
{
  int i;
  struct galaxy_seed glx;
  char planet_name[16];
  Bool found;
  char str[32];

  glx = cmdr.galaxy;
  found = 0;

  for (i = 0; i < 256; i++) {
    name_planet(planet_name, glx);

    if (StrCaselessCompare(planet_name, find_name) == 0) {
      found = 1;
      break;
    }

    waggle_galaxy (&glx);
    waggle_galaxy (&glx);
    waggle_galaxy (&glx);
    waggle_galaxy (&glx);
  }

  /* undraw old cross */
  if (old_cross_x != -100) {
    draw_cross (old_cross_x, old_cross_y);
    old_cross_x = -100;
  }

  if (!found) {
    gfx_clear_area (0, INFO_Y_GAL, 120, INFO_Y_GAL + 2*GFX_CELL_HEIGHT);
    StrPrintF(str, "Unknown Planet: %s", find_name);
    gfx_display_text(2, INFO_Y_GAL, ALIGN_LEFT, str, GFX_COL_RED);
    return;
  }

  hyperspace_planet = glx;

  /* draw planet info */
  gfx_clear_area (0, INFO_Y_GAL, 120, INFO_Y_GAL + GFX_CELL_HEIGHT - 1);
  StrPrintF (str, "%-18s", planet_name);
  gfx_display_text(2, INFO_Y_GAL, ALIGN_LEFT, str, GFX_COL_WHITE);

  show_distance (INFO_Y_GAL + GFX_CELL_HEIGHT + 1, docked_planet, 
		 hyperspace_planet);

  /* draw new cross */
  old_cross_x = cross_x = GALAXY_SCALE ((int) hyperspace_planet.d);
  old_cross_y = cross_y = GALAXY_Y_SCALE ((int) hyperspace_planet.b);

  draw_cross (old_cross_x, old_cross_y);
}

#define GAL_BUT_Y1 (92)
#define GAL_BUT_X (123)
#define GAL_BUT_H (GFX_CELL_HEIGHT+1)
#define GAL_BUT_W (35)
#define GAL_BUT_C (GAL_BUT_X+GAL_BUT_W/2)
#define GAL_BUT_Y2 (GAL_BUT_Y1+GAL_BUT_H+2)

UInt8 last_chart = SCR_SHORT_RANGE;

void
display_short_range_chart (void)
{
  int i;
  struct galaxy_seed glx;
  int px, py;
  char planet_name[16];
  int row_used[20];
  int row;
  int blob_size;

  last_chart = current_screen = SCR_SHORT_RANGE;
  gfx_update_button_map(4);

  gfx_draw_title("SHORT RANGE CHART", 1);

  /* only draw into display area */
  gfx_set_clip_region (0, TITLE_LINE_Y + 1, 159, DSP_HEIGHT - 1);

  draw_fuel_limit_circle (GFX_X_CENTRE, GFX_Y_CENTRE);

  for (i = 0; i < 20; i++)
    row_used[i] = 0;

  glx = cmdr.galaxy;

  for (i = 0; i < 256; i++) {

    px = ((int) glx.d - (int) docked_planet.d);
    px = GALAXY_SCALE (px * 4) + GFX_X_CENTRE;	/* Convert to screen co-ords */

    py = ((int) glx.b - (int) docked_planet.b);
    py = GALAXY_SCALE (py * 2) + GFX_Y_CENTRE;	/* Convert to screen co-ords */

    /* try to dind an empty row for the name */
    row = py / 6;

    /* try to find an unused row */
    if (row_used[row] == 1)
      row++;			// next row
    if (row_used[row] == 1)
      row -= 2;			// row before

    if ((row <= 1) || (row > 16))	// too far up in the title bar???
      goto next;

    if (row_used[row] == 0) {
      name_planet (planet_name, glx);
      capitalise_name (planet_name);
    }

    /* The next bit calculates the size of the circle used to represent */
    /* a planet.  The carry_flag is left over from the name generation. */
    /* Yes this was how it was done... don't ask :-( */

    blob_size = (glx.f & 1) + 2 + carry_flag;
    blob_size = 1 + GALAXY_SCALE (blob_size);

    /* only draw blob if it's fully visible on the screen */
    if ((px >= blob_size) &&
	(px < DSP_WIDTH - blob_size) &&
	(py >= TITLE_LINE_Y + 1 + blob_size) &&
	(py < DSP_HEIGHT - blob_size)) {

      /* only draw text that fits on the screen */
      if (row_used[row] == 0) {
	row_used[row] = 1;
	if (px + 4 + gfx_str_width (planet_name) < DSP_WIDTH)
	  gfx_display_text(px + 4, row * 6, ALIGN_LEFT, 
			   planet_name, GFX_COL_WHITE);
      }
      gfx_draw_filled_circle (px, py, blob_size, GFX_COL_GOLD);
    }

  next:
    waggle_galaxy (&glx);
    waggle_galaxy (&glx);
    waggle_galaxy (&glx);
    waggle_galaxy (&glx);
  }

  cross_x = GALAXY_SCALE ((hyperspace_planet.d - docked_planet.d) * 4)
    + GFX_X_CENTRE;
  cross_y = GALAXY_SCALE ((hyperspace_planet.b - docked_planet.b) * 2)
    + GFX_Y_CENTRE;

  gfx_set_clip_region (0, 0, 0, 0);

  show_distance_to_planet ();

  /* show "galactic" button */
  draw_option_button(GAL_BUT_X, GAL_BUT_Y2, GAL_BUT_X+GAL_BUT_W, 
		     "Galactic", GFX_COL_GREEN_2, GFX_COL_GREEN_4);
}

/* display the galactic chart via F5 */
void
display_galactic_chart (void)
{
  int i;
  struct galaxy_seed glx;
  char str[64];
  int px, py;

  last_chart = current_screen = SCR_GALACTIC_CHART;
  gfx_update_button_map(4);

  StrPrintF (str, "GALACTIC CHART %d", cmdr.galaxy_number + 1);
  gfx_draw_title(str, 1);

  gfx_draw_line (0, TITLE_LINE_Y + 83, 159, TITLE_LINE_Y + 83);

  /* only draw into display area */
  gfx_set_clip_region (0, TITLE_LINE_Y + 1, 159, TITLE_LINE_Y + 82);

  draw_fuel_limit_circle (GALAXY_SCALE ((int) docked_planet.d),
			  GALAXY_Y_SCALE ((int) docked_planet.b));

  glx = cmdr.galaxy;

  for (i = 0; i < 256; i++) {
    px = GALAXY_SCALE (glx.d);
    py = GALAXY_Y_SCALE (glx.b);

    gfx_plot_pixel (px, py, GFX_COL_WHITE);

    waggle_galaxy (&glx);
    waggle_galaxy (&glx);
    waggle_galaxy (&glx);
    waggle_galaxy (&glx);
  }

  cross_x = GALAXY_SCALE ((int) hyperspace_planet.d);
  cross_y = GALAXY_Y_SCALE ((int) hyperspace_planet.b);

  gfx_set_clip_region (0, 0, 0, 0);

  show_distance_to_planet ();

  /* draw two buttons: "local" and "find" */
  draw_option_button(GAL_BUT_X, GAL_BUT_Y1, GAL_BUT_X+GAL_BUT_W, 
		     "Find", GFX_COL_GREEN_2, GFX_COL_GREEN_4);

  draw_option_button(GAL_BUT_X, GAL_BUT_Y2, GAL_BUT_X+GAL_BUT_W, 
		     "Local", GFX_COL_GREEN_2, GFX_COL_GREEN_4);
}

void
find_named_planet(void) {
  const struct dialog find_planet = {
    DLG_TXTINPUT, DLG_FIND_PLANET,
    "Find system",
    "Enter name:",
    NULL,
    "OK",
    "Cancel"
  };

  dialog_draw(&find_planet);
}

Bool
handle_map_tap(int x, int y) {
  /* horizintally out of range */
  if((x>=GAL_BUT_X)&&(x<=GAL_BUT_X+GAL_BUT_W)) {

    if(current_screen == SCR_GALACTIC_CHART) {

      /* find button */
      if((y>=GAL_BUT_Y1)&&(y<=GAL_BUT_Y1+GAL_BUT_H)) {
	snd_click();
	find_named_planet();
	return 1;
      }

      /* local button */
      if((y>=GAL_BUT_Y2)&&(y<=GAL_BUT_Y2+GAL_BUT_H)) {
	snd_click();
	old_cross_x = -100;
	display_short_range_chart ();
	return 1;
      }
      
    } else {

      /* galactic button */
      if((y>=GAL_BUT_Y2)&&(y<=GAL_BUT_Y2+GAL_BUT_H)) {
	snd_click();
	old_cross_x = -100;
	display_galactic_chart ();
	return 1;
      }
    }
  }

  return 0;
}

/*
 * Displays data on the currently selected Hyperspace Planet.
 */

#define PLANET_WIDTH  33

#define PLANET_BASE 46
#define SC 100

static void 
plasma(Int8 *planet, int xl, int xh, int yl, int yh) {
  int dist = (xh-xl)/2;
  int cl = xh-xl;
  if(cl>7) cl = 7;

  /* calculate four border-midpoints */
  planet[PLANET_WIDTH*(xl+dist)+yl] = 
    (planet[PLANET_WIDTH*xl+yl] + planet[PLANET_WIDTH*xh+yl])/2 +
    cl*(char)rand255()/SC;
  planet[PLANET_WIDTH*(xl+dist)+yh] = 
    (planet[PLANET_WIDTH*xl+yh] + planet[PLANET_WIDTH*xh+yh])/2 +
    cl*(char)rand255()/SC;
  planet[PLANET_WIDTH*xl+yl+dist] = 
    (planet[PLANET_WIDTH*xl+yl] + planet[PLANET_WIDTH*xl+yh])/2 +
    cl*(char)rand255()/SC;
  planet[PLANET_WIDTH*xh+yl+dist] = 
    (planet[PLANET_WIDTH*xh+yl] + planet[PLANET_WIDTH*xh+yh])/2 +
    cl*(char)rand255()/SC;

  /* calculate center */
  planet[PLANET_WIDTH*(xl+dist)+yl+dist] = 
    (planet[PLANET_WIDTH*(xl+dist)+yl] + 
     planet[PLANET_WIDTH*(xl+dist)+yh] + 
     planet[PLANET_WIDTH*xl+yl+dist] + 
     planet[PLANET_WIDTH*xh+yl+dist])/4 +
    cl*(char)rand255()/SC;

  /* stop processing */
  if(xh-xl == 2) return;

  /* calculate four sub-plasmas by recoursion */
  plasma(planet, xl, xl+dist, yl, yl+dist);
  plasma(planet, xl+dist, xh, yl, yl+dist);
  plasma(planet, xl, xl+dist, yl+dist, yh);
  plasma(planet, xl+dist, xh, yl+dist, yh);
}

static void 
planet_base(Int8 *planet, int xl, int xh, int yl, int yh) {

  /* select for random corners */
  planet[PLANET_WIDTH*xl+yl] = PLANET_BASE + (char)rand255()/SC;
  planet[PLANET_WIDTH*xl+yh] = PLANET_BASE + (char)rand255()/SC;
  planet[PLANET_WIDTH*xh+yl] = PLANET_BASE + (char)rand255()/SC;
  planet[PLANET_WIDTH*xh+yh] = PLANET_BASE + (char)rand255()/SC;

  plasma(planet, xl,xh, yl,yh);
}

static void  
draw_planet(int x, int y) {
  const UInt8 planet_color[32] = {
    0x65, 0x65, 0x65, 0x65, 0x65, 0x65,   // blue
    0x65, 0x65, 0x65, 0x65, 0x64, 0x62,   // blue to cyan
    0x72, 0x84, 0x96, 0xa8, 0xba, 0xcc,   // yellow to lgreen
    0xd2, 0xd3, 0xd4, 0xd5, 0xd6,         // lgreen to dgreen
    0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf // dgreen to white
  };
  
  const UInt8 pw[16]={4,7,9,10,11,12,13,14,14,15,15,15,16,16,16,16};

  Int8 *planet;

  int i,j;
  UInt32 old_seed;

  planet = MemPtrNew(PLANET_WIDTH * PLANET_WIDTH);
  if(planet != NULL) {

    /* save random seed */
    old_seed = get_rand_seed();  
    set_rand_seed(hyperspace_planet.a + 256 * hyperspace_planet.b);  
    
    // one big plasma
    planet_base(planet, 0, PLANET_WIDTH-1, 0, PLANET_WIDTH-1);

#if 0 // square planet
    for(j=0;j<PLANET_WIDTH;j++) 
      for(i=0;i<PLANET_WIDTH;i++) 
	gfx_plot_pixel(x+i, y+j, planet_color[(planet[PLANET_WIDTH*j+i])/4]);

#else
    for(j=0;j<PLANET_WIDTH/2;j++) {
      for(i=0;i<2*pw[j];i++) {
	gfx_plot_pixel(x+i+16-pw[j], y+j, 
       	       planet_color[(planet[16l*i/pw[j]+PLANET_WIDTH*j])/4]);
	gfx_plot_pixel(x+i+16-pw[j], y+31-j, 
       	       planet_color[(planet[16l*i/pw[j]+PLANET_WIDTH*(31-j)])/4]);
      }
    }
#endif

    /* restore seed */
    set_rand_seed(old_seed);  

    MemPtrFree(planet);
  }
}

void
display_data_on_planet (void)
{
  char planet_name[16];
  char str[100];
  char *description;
  struct planet_data hyper_planet_data;
  int i;

  const char *economy_type[] = {
    "Rich Industrial", "Average Industrial", "Poor Industrial",
    "Mainly Industrial", "Mainly Agricultural", "Rich Agricultural",
    "Average Agricultural", "Poor Agricultural"
  };

  const char *government_type[] = {
    "Anarchy", "Feudal", "Multi-Government", "Dictatorship",
    "Communist", "Confederacy", "Democracy", "Corporate State"
  };

  current_screen = SCR_PLANET_DATA;
  gfx_update_button_map(5);

  name_planet (planet_name, hyperspace_planet);
  StrPrintF (str, "DATA ON %s", planet_name);

  gfx_draw_title(str, 1);

  generate_planet_data (&hyper_planet_data, hyperspace_planet);

  show_distance (TXT_ROW (1), docked_planet, hyperspace_planet);

  StrPrintF (str, "Economy: %s", economy_type[hyper_planet_data.economy]);
  gfx_display_text(2, TXT_ROW(2), ALIGN_LEFT, str, GFX_COL_WHITE);

  StrPrintF (str, "Government: %s",
	     government_type[hyper_planet_data.government]);
  gfx_display_text(2, TXT_ROW(3), ALIGN_LEFT, str, GFX_COL_WHITE);

  StrPrintF (str, "Tech.Level: %d", hyper_planet_data.techlevel + 1);
  gfx_display_text(2, TXT_ROW(4), ALIGN_LEFT, str, GFX_COL_WHITE);

  StrPrintF (str, "Population: %d.%d Billion",
	     hyper_planet_data.population / 10,
	     hyper_planet_data.population % 10);
  gfx_display_text(2, TXT_ROW(5), ALIGN_LEFT, str, GFX_COL_WHITE);

  describe_inhabitants (str, hyperspace_planet);
  gfx_display_text(2, TXT_ROW(6), ALIGN_LEFT, str, GFX_COL_WHITE);

  StrPrintF (str, "Gross Productivity: %u M CR",
	     hyper_planet_data.productivity);
  gfx_display_text(2, TXT_ROW(7), ALIGN_LEFT, str, GFX_COL_WHITE);

  StrPrintF (str, "Average Radius: %d km", hyper_planet_data.radius);
  gfx_display_text(2, TXT_ROW(8), ALIGN_LEFT, str, GFX_COL_WHITE);

  draw_planet(1, TXT_ROW(10)-4);

  /* limit text to five lines */
  description = describe_planet (hyperspace_planet);
  gfx_display_pretty_text (36, TXT_ROW(10)-4, 158, TXT_ROW(17), 0, 
			   description);
}

struct rank {
  int score;
  char *title;
};

#define NO_OF_RANKS	9

static const char *
laser_type (int strength)
{
  const char *laser_name[5] = {
    "Pulse", "Beam", "Military", "Mining", "Custom"
  };

  switch (strength) {
  case PULSE_LASER:
    return laser_name[0];

  case BEAM_LASER:
    return laser_name[1];

  case MILITARY_LASER:
    return laser_name[2];

  case MINING_LASER:
    return laser_name[3];
  }

  return laser_name[4];
}


#define EQUIP_START_X	        20
#define Y_INC			(GFX_CELL_HEIGHT)

static const char *condition_txt[] = {
  "Docked", "Green", "Yellow", "Red"
};

void
set_time_str(char *str, UInt32 time) {
  UInt16 h, l;
  Int8 uh, ul;

  /* days / hours */
  h = time/86400l; time %= 86400l; l = time/3600l; uh = 'd'; ul = 'h';

  /* no days yet */
  if(h == 0) {
    h = l; time %= 3600l; l = time/60l; uh = 'h'; ul = 'm';

    /* no hours yet */
    if(h==0) {
      h = l; time %= 60l; l = time; uh = 'm'; ul = 's';
    }
  }

  StrPrintF(str, "%02d%c%02d%c", h, uh, l, ul);
}

void
display_game_timer() {
  char str[64];

  StrPrintF (str, "COMMANDER %s: ", cmdr.name);
  set_time_str(&str[StrLen(str)], cmdr.timer + TimGetSeconds()-game_timer);

  gfx_draw_title(str, 0);
}

void
display_commander_status (void)
{
  char planet_name[16];
  char str[32];
  int i;
  int y = 1;
  int condition;
  int type;
  UInt32 now;

  const struct rank rating[NO_OF_RANKS] = {
    {0x0000, "Harmless"},
    {0x0008, "Mostly Harmless"},
    {0x0010, "Poor"},
    {0x0020, "Average"},
    {0x0040, "Above Average"},
    {0x0080, "Competent"},
    {0x0200, "Dangerous"},
    {0x0A00, "Deadly"},
    {0x1900, "---- E L I T E ---"}
  };
  
#ifndef DEMO
#ifdef CHEAT
  /* add a docking computer */
  cmdr.docking_computer = 1;
  if(!cmdr.rear_laser)
    cmdr.rear_laser = PULSE_LASER;

  // magically increased income
  if (cmdr.credits == 1000)
    cmdr.credits = 1000000l;
#endif
#else
  /* limit credits to 150.0 */
  if (cmdr.credits >= 1500)
    cmdr.credits = 1500l;
#endif

  current_screen = SCR_CMDR_STATUS;
  gfx_update_button_map(7);

  gfx_clear_display();
  display_game_timer();

  y += Y_INC + 2;
  gfx_display_text(2, y, ALIGN_LEFT, "Present System:", GFX_COL_GREEN_1);

  if (!witchspace) {
    name_planet (planet_name, docked_planet);
    capitalise_name (planet_name);
    StrPrintF (str, "%s", planet_name);
    gfx_display_text(100, y, ALIGN_LEFT, str, GFX_COL_WHITE);
  }
  y += Y_INC;

  gfx_display_text(2, y, ALIGN_LEFT, "Hyperspace System:", GFX_COL_GREEN_1);
  name_planet (planet_name, hyperspace_planet);
  capitalise_name (planet_name);
  StrPrintF (str, "%s", planet_name);
  gfx_display_text(100, y, ALIGN_LEFT, str, GFX_COL_WHITE);
  y += Y_INC;

  if (is_docked)
    condition = 0;
  else {
    condition = 1;

    for (i = 0; i < MAX_UNIV_OBJECTS; i++) {
      type = universe[i].type;

      if ((type == SHIP_MISSILE) ||
	  ((type > SHIP_ROCK) && (type < SHIP_DODEC))) {
	condition = 2;
	break;
      }
    }

    if ((condition == 2) && (energy < 128))
      condition = 3;
  }

  gfx_display_text(2, y, ALIGN_LEFT, "Condition:", GFX_COL_GREEN_1);
  gfx_display_text(100, y, ALIGN_LEFT, 
		   (char *)condition_txt[condition], GFX_COL_WHITE);
  y += Y_INC;

  StrPrintF(str, "%d.%d Light Years", cmdr.fuel / 10, cmdr.fuel % 10);
  gfx_display_text(2, y, ALIGN_LEFT, "Fuel:", GFX_COL_GREEN_1);
  gfx_display_text(75, y, ALIGN_LEFT, str, GFX_COL_WHITE);
  y += Y_INC;

  StrPrintF (str, "%ld.%ld " CURRENCY, cmdr.credits / 10, cmdr.credits % 10);
  gfx_display_text(2, y, ALIGN_LEFT, "Cash:", GFX_COL_GREEN_1);
  gfx_display_text(75, y, ALIGN_LEFT, str, GFX_COL_WHITE);
  y += Y_INC;

  if (cmdr.legal_status == 0)
    StrCopy (str, "Clean");
  else
    StrCopy (str, cmdr.legal_status > 50 ? "Fugitive" : "Offender");

  gfx_display_text(2, y, ALIGN_LEFT, "Legal Status:", GFX_COL_GREEN_1);
  gfx_display_text(75, y, ALIGN_LEFT, str, GFX_COL_WHITE);
  y += Y_INC;

  for (i = 0; i < NO_OF_RANKS; i++)
    if (cmdr.score >= rating[i].score)
      StrCopy (str, rating[i].title);

#ifdef CHEAT
  StrPrintF(&str[StrLen(str)], "(%d)", cmdr.score);
#endif

  gfx_display_text(2, y, ALIGN_LEFT, "Rating:", GFX_COL_GREEN_1);
  gfx_display_text(75, y, ALIGN_LEFT, str, GFX_COL_WHITE);
  y += Y_INC + 2;

  gfx_display_text (2, y, ALIGN_LEFT, "LASERS:", GFX_COL_GREEN_1);
  gfx_display_text (75, y, ALIGN_LEFT, "EQUIPMENT:", GFX_COL_GREEN_1);
  y += Y_INC;
  i = y;

  if (cmdr.cargo_capacity > 20) {
    gfx_display_text(75, i, ALIGN_LEFT, "Large Cargo Bay", GFX_COL_WHITE);
    i += Y_INC;
  }

  if (cmdr.escape_pod) {
    gfx_display_text(75, i, ALIGN_LEFT, "Escape Capsule", GFX_COL_WHITE);
    i += Y_INC;
  }

  if (cmdr.fuel_scoop) {
    gfx_display_text(75, i, ALIGN_LEFT, "Fuel Scoops", GFX_COL_WHITE);
    i += Y_INC;
  }

  if (cmdr.ecm) {
    gfx_display_text(75, i, ALIGN_LEFT, "E.C.M. System", GFX_COL_WHITE);
    i += Y_INC;
  }

  if (cmdr.energy_bomb) {
    gfx_display_text(75, i, ALIGN_LEFT, "Energy Bomb", GFX_COL_WHITE);
    i += Y_INC;
  }

  if (cmdr.energy_unit) {
    gfx_display_text(75, i, ALIGN_LEFT, cmdr.energy_unit == 1 ?
		     "Extra Energy Unit" : "Naval Energy Unit", GFX_COL_WHITE);
    i += Y_INC;
  }

  if (cmdr.docking_computer) {
    gfx_display_text(75, i, ALIGN_LEFT, "Docking Computers", GFX_COL_WHITE);
    i += Y_INC;
  }


  if (cmdr.galactic_hyperdrive) {
    gfx_display_text(75, i, ALIGN_LEFT, "Galactic Hyperdrive", GFX_COL_WHITE);
  }

  if (cmdr.front_laser) {
    StrPrintF(str, "Front %s", laser_type (cmdr.front_laser));
    gfx_display_text(2, y, ALIGN_LEFT, str, GFX_COL_WHITE);
    y += Y_INC;
  }

  if (cmdr.rear_laser) {
    StrPrintF(str, "Rear %s", laser_type (cmdr.rear_laser));
    gfx_display_text(2, y, ALIGN_LEFT, str, GFX_COL_WHITE);
    y += Y_INC;
  }

  if (cmdr.left_laser) {
    StrPrintF(str, "Left %s", laser_type (cmdr.left_laser));
    gfx_display_text(2, y, ALIGN_LEFT, str, GFX_COL_WHITE);
    y += Y_INC;
  }

  if (cmdr.right_laser) {
    StrPrintF(str, "Right %s", laser_type (cmdr.right_laser));
    gfx_display_text(2, y, ALIGN_LEFT, str, GFX_COL_WHITE);
  }
}



/***********************************************************************************/

#define TONNES		0
#define	KILOGRAMS	1
#define GRAMS		2

#define CASH_X 40
#define CASH_Y 120
#define CASH_W 80

static void
display_stock_price (int i)
{
  int y,col;
  char str[64];
  const char *unit_name[] = { "t", "kg", "g" };

  y = i * GFX_CELL_HEIGHT + TITLE_LINE_Y + 1;

  gfx_fill_rectangle(0, y, 159, y + GFX_CELL_HEIGHT - 1, 
		     (i&1)?GFX_COL_BLACK:GFX_COL_GREY_0);

  col = (i&1)?GFX_COL_GREY_5:GFX_COL_WHITE;

  gfx_display_text(2, y, ALIGN_LEFT, stock_market[i].name, col);

  StrPrintF (str, "%d.%d " CURRENCY "\014%s", stock_market[i].current_price / 10,
	     stock_market[i].current_price % 10,
	     unit_name[stock_market[i].units]);

  gfx_display_text(98, y, ALIGN_RIGHT, str, col);

  if (stock_market[i].current_quantity > 0) {
    if(is_docked)
      gfx_display_text(138-4, y, ALIGN_LEFT, "\003", 
		       (i&1)?GFX_COL_GREEN_2:GFX_COL_GREEN_1);

    StrPrintF (str, "%d%s", stock_market[i].current_quantity,
	       unit_name[stock_market[i].units]);
  } else
    StrCopy (str, "-");

  gfx_display_text(120, y, ALIGN_RIGHT, str, col);

  if (cmdr.current_cargo[i] > 0) {
    if(is_docked) {
      gfx_display_text(138-16, y, ALIGN_LEFT, "\004",
		       (i&1)?GFX_COL_RED_1:GFX_COL_RED);
#ifdef SBAY
      if(sbay_enabled)
	gfx_display_text(138-10, y, ALIGN_LEFT, "\012",
			 (i&1)?GFX_COL_BLUE_2:GFX_COL_BLUE_1);
#endif
    }
    StrPrintF (str, "%d%s", cmdr.current_cargo[i],
	       unit_name[stock_market[i].units]);
  } else
    StrCopy (str, "-");

  gfx_display_text(158, y, ALIGN_RIGHT, str, col);
}

static void 
display_market_title(void) {
  char title[64];

  name_planet(title, docked_planet);
  StrCat(title, " MARKET");

  if (is_docked)
    StrPrintF (&title[StrLen(title)], ": %ld.%ld" CURRENCY ", %d\014%dt",
	       cmdr.credits / 10, cmdr.credits % 10,
	       total_cargo(), cmdr.cargo_capacity);

  gfx_draw_title(title, 0);
}

void
buy_stock (int i)
{
  struct stock_item *item;
  int cargo_held;

  if (!is_docked) return;

  item = &stock_market[i];

  if ((item->current_quantity == 0) || (cmdr.credits < item->current_price))
    return;

  cargo_held = total_cargo ();

  if ((item->units == TONNES) && (cargo_held == cmdr.cargo_capacity))
    return;

  snd_click();

  cmdr.current_cargo[i]++;
  item->current_quantity--;
  cmdr.credits -= item->current_price;

  display_stock_price(i);
  display_market_title();
}


void
sell_stock (int i)
{
  struct stock_item *item;

  if ((!is_docked) || (cmdr.current_cargo[i] == 0))
    return;

  snd_click();

  item = &stock_market[i];

  cmdr.current_cargo[i]--;
  item->current_quantity++;
  cmdr.credits += item->current_price;

  display_stock_price(i);
  display_market_title();
}

void
display_market_prices (void)
{
  int i;

  current_screen = SCR_MARKET_PRICES;
  gfx_update_button_map(6);

  gfx_clear_display();
  display_market_title();

  for (i = 0; i < 17; i++) display_stock_price (i);
}

void
display_inventory (void)
{
  int i, y, x;
  char str[40];
  const char *unit_name[] = { "t", "kg", "g" };

  current_screen = SCR_INVENTORY;
  gfx_update_button_map(8);

  gfx_draw_title("INVENTORY", 1);

  StrPrintF (str, "%d.%d Light Years", cmdr.fuel / 10, cmdr.fuel % 10);
  gfx_display_text(2, TITLE_LINE_Y + 2, ALIGN_LEFT, "Fuel:", GFX_COL_GREEN_1);
  gfx_display_text(40, TITLE_LINE_Y + 2, ALIGN_LEFT, str, GFX_COL_WHITE);

  StrPrintF (str, "%ld.%ld " CURRENCY, cmdr.credits / 10, cmdr.credits % 10);
  gfx_display_text(2, TITLE_LINE_Y + 1 * (GFX_CELL_HEIGHT + 1) + 2,
		   ALIGN_LEFT, "Cash:", GFX_COL_GREEN_1);
  gfx_display_text(40, TITLE_LINE_Y + 1 * (GFX_CELL_HEIGHT + 1) + 2, 
		   ALIGN_LEFT, str, GFX_COL_WHITE);

  StrPrintF (str, "%d of %d Tons", total_cargo (), cmdr.cargo_capacity);
  gfx_display_text(2, TITLE_LINE_Y + 2 * (GFX_CELL_HEIGHT + 1) + 2,
		   ALIGN_LEFT, "Cargo:", GFX_COL_GREEN_1);
  gfx_display_text(40, TITLE_LINE_Y + 2 * (GFX_CELL_HEIGHT + 1) + 2, 
		   ALIGN_LEFT, str, GFX_COL_WHITE);

  x = 3;
  y = TITLE_LINE_Y + 3 * (GFX_CELL_HEIGHT + 1) + 4;
  for (i = 0; i < 17; i++) {
    if (cmdr.current_cargo[i] > 0) {
      if((((y/GFX_CELL_HEIGHT)&1)+(x<80))==1)
        gfx_fill_rectangle(x-1, y, x+75, y+GFX_CELL_HEIGHT-1, GFX_COL_GREY_0);
      else
        gfx_fill_rectangle(x-1, y, x+75, y+GFX_CELL_HEIGHT-1, GFX_COL_GREY_1);

      gfx_display_text(x, y, ALIGN_LEFT, stock_market[i].name, GFX_COL_WHITE);

      StrPrintF (str, "%d%s", cmdr.current_cargo[i],
		 unit_name[stock_market[i].units]);

      gfx_display_text(x + 75, y, ALIGN_RIGHT, str, GFX_COL_WHITE);

      if (x == 3)
	x = 82;
      else {
	x = 3;
	y += GFX_CELL_HEIGHT;
      }
    }
  }
}

/***********************************************************************/

enum equip_types {
  EQ_FUEL, EQ_MISSILE, EQ_CARGO_BAY, EQ_ECM, EQ_FUEL_SCOOPS,
  EQ_ESCAPE_POD, EQ_ENERGY_BOMB, EQ_ENERGY_UNIT, EQ_DOCK_COMP,
  EQ_GAL_DRIVE, EQ_PULSE_LASER, EQ_FRONT_PULSE, EQ_REAR_PULSE,
  EQ_LEFT_PULSE, EQ_RIGHT_PULSE, EQ_BEAM_LASER, EQ_FRONT_BEAM,
  EQ_REAR_BEAM, EQ_LEFT_BEAM, EQ_RIGHT_BEAM, EQ_MINING_LASER,
  EQ_FRONT_MINING, EQ_REAR_MINING, EQ_LEFT_MINING, EQ_RIGHT_MINING,
  EQ_MILITARY_LASER, EQ_FRONT_MILITARY, EQ_REAR_MILITARY,
  EQ_LEFT_MILITARY, EQ_RIGHT_MILITARY
};

#define NO_OF_EQUIP_ITEMS	30

#define BUY_OK      0
#define BUY_2EXP    1
#define BUY_ALREADY 2

struct equip_item {
  UInt8 buy_state;
  UInt8 y;
  Boolean show;
  UInt8 level;
  UInt16 price;
  Char *name;
  UInt8 type;
};

static struct equip_item equip_stock[NO_OF_EQUIP_ITEMS] = {
  {0, 0, 1,  1,     2, " Fuel", EQ_FUEL},
  {0, 0, 1,  1,   300, " Missile", EQ_MISSILE},
  {0, 0, 1,  1,  4000, " Large Cargo Bay", EQ_CARGO_BAY},
  {0, 0, 1,  2,  6000, " E.C.M. System", EQ_ECM},
  {0, 0, 1,  5,  5250, " Fuel Scoops", EQ_FUEL_SCOOPS},
  {0, 0, 1,  6, 10000, " Escape Capsule", EQ_ESCAPE_POD},
  {0, 0, 1,  7,  9000, " Energy Bomb", EQ_ENERGY_BOMB},
  {0, 0, 1,  8, 15000, " Extra Energy Unit", EQ_ENERGY_UNIT},
  {0, 0, 1,  9, 15000, " Docking Computers", EQ_DOCK_COMP},
  {0, 0, 1, 10, 50000, " Galactic Hyperdrive", EQ_GAL_DRIVE},
  {0, 0, 0,  3,  4000, "+\010Pulse Laser", EQ_PULSE_LASER},
  {0, 0, 1,  3,  4000, "-\007Pulse Laser", EQ_FRONT_PULSE},
  {0, 0, 1,  3,  4000, ">Rear", EQ_REAR_PULSE},
  {0, 0, 1,  3,  4000, ">Left", EQ_LEFT_PULSE},
  {0, 0, 1,  3,  4000, ">Right", EQ_RIGHT_PULSE},
  {0, 0, 1,  4, 10000, "+\010Beam Laser", EQ_BEAM_LASER},
  {0, 0, 0,  4, 10000, "-\007Beam Laser", EQ_FRONT_BEAM},
  {0, 0, 0,  4, 10000, ">Rear", EQ_REAR_BEAM},
  {0, 0, 0,  4, 10000, ">Left", EQ_LEFT_BEAM},
  {0, 0, 0,  4, 10000, ">Right", EQ_RIGHT_BEAM},
  {0, 0, 1, 10,  8000, "+\010Mining Laser", EQ_MINING_LASER},
  {0, 0, 0, 10,  8000, "-\007Mining Laser", EQ_FRONT_MINING},
  {0, 0, 0, 10,  8000, ">Rear", EQ_REAR_MINING},
  {0, 0, 0, 10,  8000, ">Left", EQ_LEFT_MINING},
  {0, 0, 0, 10,  8000, ">Right", EQ_RIGHT_MINING},
  {0, 0, 1, 10, 60000, "+\010Military Laser", EQ_MILITARY_LASER},
  {0, 0, 0, 10, 60000, "-\007Military Laser", EQ_FRONT_MILITARY},
  {0, 0, 0, 10, 60000, ">Rear", EQ_REAR_MILITARY},
  {0, 0, 0, 10, 60000, ">Left", EQ_LEFT_MILITARY},
  {0, 0, 0, 10, 60000, ">Right", EQ_RIGHT_MILITARY}
};

static int
equip_present (int type)
{
  switch (type) {
  case EQ_FUEL:
    return (cmdr.fuel >= 70);

  case EQ_MISSILE:
    return (cmdr.missiles >= 4);

  case EQ_CARGO_BAY:
    return (cmdr.cargo_capacity > 20);

  case EQ_ECM:
    return cmdr.ecm;

  case EQ_FUEL_SCOOPS:
    return cmdr.fuel_scoop;

  case EQ_ESCAPE_POD:
    return cmdr.escape_pod;

  case EQ_ENERGY_BOMB:
    return cmdr.energy_bomb;

  case EQ_ENERGY_UNIT:
    return cmdr.energy_unit;

  case EQ_DOCK_COMP:
    return cmdr.docking_computer;

  case EQ_GAL_DRIVE:
    return cmdr.galactic_hyperdrive;

  case EQ_FRONT_PULSE:
    return (cmdr.front_laser == PULSE_LASER);

  case EQ_REAR_PULSE:
    return (cmdr.rear_laser == PULSE_LASER);

  case EQ_LEFT_PULSE:
    return (cmdr.left_laser == PULSE_LASER);

  case EQ_RIGHT_PULSE:
    return (cmdr.right_laser == PULSE_LASER);

  case EQ_FRONT_BEAM:
    return (cmdr.front_laser == BEAM_LASER);

  case EQ_REAR_BEAM:
    return (cmdr.rear_laser == BEAM_LASER);

  case EQ_LEFT_BEAM:
    return (cmdr.left_laser == BEAM_LASER);

  case EQ_RIGHT_BEAM:
    return (cmdr.right_laser == BEAM_LASER);

  case EQ_FRONT_MINING:
    return (cmdr.front_laser == MINING_LASER);

  case EQ_REAR_MINING:
    return (cmdr.rear_laser == MINING_LASER);

  case EQ_LEFT_MINING:
    return (cmdr.left_laser == MINING_LASER);

  case EQ_RIGHT_MINING:
    return (cmdr.right_laser == MINING_LASER);

  case EQ_FRONT_MILITARY:
    return (cmdr.front_laser == MILITARY_LASER);

  case EQ_REAR_MILITARY:
    return (cmdr.rear_laser == MILITARY_LASER);

  case EQ_LEFT_MILITARY:
    return (cmdr.left_laser == MILITARY_LASER);

  case EQ_RIGHT_MILITARY:
    return (cmdr.right_laser == MILITARY_LASER);
  }
  return 0;
}


static void
display_equip_price (int i)
{
  int x, y;
  int col;
  char str[16];

  if((y = equip_stock[i].y)==0) return;

  gfx_fill_rectangle(0, y, 159, y + GFX_CELL_HEIGHT - 1, 
		     ((y/GFX_CELL_HEIGHT)&1)?GFX_COL_BLACK:GFX_COL_GREY_0);

  if(equip_stock[i].buy_state == BUY_OK)
    col = ((y/GFX_CELL_HEIGHT)&1)?GFX_COL_GREY_5:GFX_COL_WHITE;
  else if(equip_stock[i].buy_state == BUY_2EXP)
    col = ((y/GFX_CELL_HEIGHT)&1)?GFX_COL_RED_1:GFX_COL_RED;
  else
    col = ((y/GFX_CELL_HEIGHT)&1)?GFX_COL_GREEN_3:GFX_COL_GREEN_2;

  x = *(equip_stock[i].name) == '>' ? 80 : 2;

  gfx_display_text(x, y, ALIGN_LEFT, &equip_stock[i].name[1], 
		(equip_stock[i].name[0] == '-')?
		(((y/GFX_CELL_HEIGHT)&1)?GFX_COL_GREY_5:GFX_COL_WHITE):col);

  /* expanded collapsable entry */
  if (equip_stock[i].name[0] == '-')
    gfx_display_text (80, y, ALIGN_LEFT, "Front", col);

  if (equip_stock[i].price != 0) {
    StrPrintF (str, "%u.%u " CURRENCY, equip_stock[i].price / 10,
	       equip_stock[i].price % 10);
    gfx_display_text(158, y, ALIGN_RIGHT, str, col);
  }
}

static void
list_equip_prices (void)
{
  int i;
  int y;
  int tech_level;
  char str[64];

  StrPrintF (str, "EQUIP SHIP: %ld.%ld" CURRENCY,
	     cmdr.credits / 10, cmdr.credits % 10);
  gfx_draw_title(str, 1);

#ifndef DEMO
#ifdef CHEAT
  tech_level = 13;
#else
  tech_level = current_planet_data.techlevel + 1;
#endif
#else
  tech_level = 0;
#endif

  equip_stock[0].price = (70 - cmdr.fuel) * 2;

  /* create table of equipment */
  y = TITLE_LINE_Y + 1;
  for (i = 0; i < NO_OF_EQUIP_ITEMS; i++) {
    /* check if we have that already installed */
    if(equip_present (equip_stock[i].type))
      equip_stock[i].buy_state = BUY_ALREADY;
    else {
      /* stuff can only be bought, if it's available and enough money */
      if(equip_stock[i].price > cmdr.credits) 
	equip_stock[i].buy_state = BUY_2EXP;
      else
	equip_stock[i].buy_state = BUY_OK;
    }

    if (equip_stock[i].show && (tech_level >= equip_stock[i].level)) {
      equip_stock[i].y = y;
      y += GFX_CELL_HEIGHT;
    }
    else
      equip_stock[i].y = 0;

    display_equip_price (i);
  }
}


static void
collapse_equip_list (void)
{
  int i;
  int ch;

  for (i = 0; i < NO_OF_EQUIP_ITEMS; i++) {
    ch = *(equip_stock[i].name);
    equip_stock[i].show = ((ch == ' ') || (ch == '+'));
  }
}


static int
laser_refund (int laser_type)
{
  switch (laser_type) {
  case PULSE_LASER:
    return 4000;

  case BEAM_LASER:
    return 10000;

  case MILITARY_LASER:
    return 60000;

  case MINING_LASER:
    return 8000;
  }
  return 0;
}


void
buy_equip (int pos, int x)
{
  int i, item=-1;

  /* find the entry at pos */
  for (item =0, i = 0; i < NO_OF_EQUIP_ITEMS; i++) {
    if(equip_stock[i].show)
      if((pos >= equip_stock[i].y)&&
	 (pos <  equip_stock[i].y + GFX_CELL_HEIGHT))
	item = i;
  }

  if(item == -1) return;

  if (equip_stock[item].name[0] == '+') {
    snd_click();

    /* collapse all expanded entries */
    collapse_equip_list ();
    equip_stock[item].show = 0;

    /* and expand others */
    for (i = 1; i < 5; i++)
      equip_stock[item + i].show = 1;

    list_equip_prices ();
    return;
  }

  /* tapped on left area of expanded entry */
  if ((equip_stock[item].name[0] == '-') && (x < 80)) {
    snd_click();
    collapse_equip_list ();
    list_equip_prices ();
    return;
  }

  /* only buy things you are allowed to buy */
  if (equip_stock[item].buy_state != BUY_OK)
    return;

  /* tapped on left area of subentry */
  if ((equip_stock[item].name[0] == '>') && (x < 80))
    return;

  snd_click();

  switch (equip_stock[item].type) {
  case EQ_FUEL:
    cmdr.fuel = MAX_FUEL;
    update_console(1);
    break;

  case EQ_MISSILE:
    cmdr.missiles++;
    update_console(1);
    break;

  case EQ_CARGO_BAY:
    cmdr.cargo_capacity = 35;
    break;

  case EQ_ECM:
    cmdr.ecm = 1;
    break;

  case EQ_FUEL_SCOOPS:
    cmdr.fuel_scoop = 1;
    break;

  case EQ_ESCAPE_POD:
    cmdr.escape_pod = 1;
    break;

  case EQ_ENERGY_BOMB:
    cmdr.energy_bomb = 1;
    break;

  case EQ_ENERGY_UNIT:
    cmdr.energy_unit = 1;
    break;

  case EQ_DOCK_COMP:
    cmdr.docking_computer = 1;
    break;

  case EQ_GAL_DRIVE:
    cmdr.galactic_hyperdrive = 1;
    break;

  case EQ_FRONT_PULSE:
    cmdr.credits += laser_refund (cmdr.front_laser);
    cmdr.front_laser = PULSE_LASER;
    break;

  case EQ_REAR_PULSE:
    cmdr.credits += laser_refund (cmdr.rear_laser);
    cmdr.rear_laser = PULSE_LASER;
    break;

  case EQ_LEFT_PULSE:
    cmdr.credits += laser_refund (cmdr.left_laser);
    cmdr.left_laser = PULSE_LASER;
    break;

  case EQ_RIGHT_PULSE:
    cmdr.credits += laser_refund (cmdr.right_laser);
    cmdr.right_laser = PULSE_LASER;
    break;

  case EQ_FRONT_BEAM:
    cmdr.credits += laser_refund (cmdr.front_laser);
    cmdr.front_laser = BEAM_LASER;
    break;

  case EQ_REAR_BEAM:
    cmdr.credits += laser_refund (cmdr.rear_laser);
    cmdr.rear_laser = BEAM_LASER;
    break;

  case EQ_LEFT_BEAM:
    cmdr.credits += laser_refund (cmdr.left_laser);
    cmdr.left_laser = BEAM_LASER;
    break;

  case EQ_RIGHT_BEAM:
    cmdr.credits += laser_refund (cmdr.right_laser);
    cmdr.right_laser = BEAM_LASER;
    break;

  case EQ_FRONT_MINING:
    cmdr.credits += laser_refund (cmdr.front_laser);
    cmdr.front_laser = MINING_LASER;
    break;

  case EQ_REAR_MINING:
    cmdr.credits += laser_refund (cmdr.rear_laser);
    cmdr.rear_laser = MINING_LASER;
    break;

  case EQ_LEFT_MINING:
    cmdr.credits += laser_refund (cmdr.left_laser);
    cmdr.left_laser = MINING_LASER;
    break;

  case EQ_RIGHT_MINING:
    cmdr.credits += laser_refund (cmdr.right_laser);
    cmdr.right_laser = MINING_LASER;
    break;

  case EQ_FRONT_MILITARY:
    cmdr.credits += laser_refund (cmdr.front_laser);
    cmdr.front_laser = MILITARY_LASER;
    break;

  case EQ_REAR_MILITARY:
    cmdr.credits += laser_refund (cmdr.rear_laser);
    cmdr.rear_laser = MILITARY_LASER;
    break;

  case EQ_LEFT_MILITARY:
    cmdr.credits += laser_refund (cmdr.left_laser);
    cmdr.left_laser = MILITARY_LASER;
    break;

  case EQ_RIGHT_MILITARY:
    cmdr.credits += laser_refund (cmdr.right_laser);
    cmdr.right_laser = MILITARY_LASER;
    break;
  }

  cmdr.credits -= equip_stock[item].price;
  list_equip_prices ();
}


void
equip_ship (void)
{
  current_screen = SCR_EQUIP_SHIP;
  gfx_update_button_map(3);

  collapse_equip_list ();

  list_equip_prices ();
}
