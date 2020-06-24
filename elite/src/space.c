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
 * space.c
 *
 * This module handles all the flight system and 
 * management of the space universe.
 */

#include <PalmOS.h>
#include "sections.h"

#include "vector.h"
#include "config.h"
#include "elite.h"
#include "gfx.h"
#include "docked.h"
#include "intro.h"
#include "shipdata.h"
#include "space.h"
#include "threed.h"
#include "sound.h"
#include "main.h"
#include "swat.h"
#include "random.h"
#include "trade.h"
#include "stars.h"
#include "pilot.h"
#include "debug.h"
#include "rumble.h"
#include "dialog.h"
#include "options.h"
#include "file.h"

#include "sintab.h"

/* save data on lollipops to undraw them */
struct lollipop_struct {
  UInt8 x1, y1, y2;
  Boolean mapped;
} lollipop[MAX_UNIV_OBJECTS];

static SEC3D void do_game_over (void);
static SEC3D void display_missiles (void);
static SEC3D void update_compass (Boolean draw);
static SEC3D void draw_id (int id);
static SEC3D void show_status(void);
static SEC3D void draw_instr_bar (Bool x, Bool red, Int16 y, Int16 value);
static SEC3D void draw_instr_ind (Int16 y, Int16 value);
static SEC3D void update_scanner (void);
static SEC3D void draw_lollipop(struct lollipop_struct *l, UInt8 colour, UInt8 gray);
static SEC3D void undraw_lollipop(struct lollipop_struct *l);

static SEC3D Bool is_docking (int);
static SEC3D void check_docking (int i);
static SEC3D void make_station_appear (void);
static SEC3D void switch_to_view (struct univ_object *);

static SEC3D void rotate_matrix (RotVector *mat, Boolean axis_x, int dir);
static SEC3D void rotate_matrix2(RotVector *mat, Int32 alpha, Int32 beta);

static SEC3D int  rotate_byte_left (int x);
static SEC3D void enter_next_galaxy (void);
static SEC3D void enter_witchspace (void);
static SEC3D void complete_hyperspace (void);

struct galaxy_seed destination_planet;
int hyper_ready;
int hyper_countdown;
char hyper_name[16];
int hyper_distance;
int hyper_galactic;

#define DISMISS_DIST 57344  // objects are removed when this far away
#define RADAR_DIST   24000  // objects in this distance can be seen in the radar


/* scanner/instrument coordinates */
#define LEFT_SIDE_X      (10)
#define RIGHT_SIDE_OFF   (120)
#define BAR_WIDTH        (20)

#define INST_FS_Y        (2)
#define INST_AS_Y        (7)
#define INST_FU_Y        (14)
#define INST_CT_Y        (19)
#define INST_LT_Y        (24)
#define INST_AL_Y        (29)
#define INST_MS_Y        (36)

#define INST_SP_Y        (2)
#define INST_RL_Y        (7)
#define INST_CL_Y        (12)
#define INST_EN_Y        (20)

#define ID_X (34)
#define ID_Y (DSP_HEIGHT+2)

#define ID_SAVE 0
#define ID_ECM  1
#define ID_NONE 2

/* direct draw routines for scanner */
extern char *vidbase;

static void draw_instr_bar (Bool x, Bool red, Int16 y, Int16 value)
{
  char *p2, *p1 = vidbase + ((160 * (DSP_HEIGHT + y)) + LEFT_SIDE_X)/pixperbyte;
  const UInt8 colors[2][2] = { {0x72, 0x79}, {0x77, 0x8f} };
  int i;
  UInt8 c0,c1;

  if (value < 0)    value = 0;
  if (value > 255)  value = 255;
  value = BAR_WIDTH * value / 255;
  
  c0 = colors[red][0];
  c1 = colors[red][1];

  if(color_gfx) {
    /* draw color bar */

    /* draw on right side */
    if (x) p1 += RIGHT_SIDE_OFF;
    p2 = p1 + 160;

    /* draw highlighted part */
    for (i = 0; i < value; i++) {
      *p1++ = c0; *p2++ = c1;
    }
    
    /* draw black part */
    for (; i < BAR_WIDTH; i++) {
      *p1++ = 0xff; *p2++ = 0xff;
    }
  } else {
    /* draw grayscale bar */
    c0 = graytab[c0];
    c1 = graytab[c1];

    /* draw on right side */
    if (x) p1 += RIGHT_SIDE_OFF/2;
    p2 = p1 + 80;

    /* draw highlighted part */
    for (i = 0; i < value/2; i++) {
      *p1++ = c0; *p2++ = c1;
    }

    /* center part to be drawn? */
    if(value&1) {
      if(options.bool[OPTION_INVERT]) {
	*p1++ = c0&0xf0; *p2++ = c1&0xf0;
      } else {
	*p1++ = c0|0x0f; *p2++ = c1|0x0f;
      }

      i++;
    }

    /* draw black part */
    for (c0 = graytab[0xff]; i < BAR_WIDTH/2; i++) {
      *p1++ = c0; *p2++ = c0;
    }
  }
}

static void
draw_instr_ind (Int16 y, Int16 value)
{
  char *p2, *p1 = vidbase + ((160 * (DSP_HEIGHT + y)) + 
		    LEFT_SIDE_X + RIGHT_SIDE_OFF)/pixperbyte;
  int i;

  if(color_gfx) {
    /* draw color indicator */
    p2 = p1 + 160;
    for (i = 0; i < BAR_WIDTH; i++)
      *p1++ = *p2++ = 0xff; 

    p1 -= BAR_WIDTH - value;
    p1[0] = p1[160] = 0x72;
  } else {
    /* draw grayscale indicator */
    p2 = p1 + 80;
    for (i = 0; i < BAR_WIDTH/2; i++)
      *p1++ = *p2++ = graytab[0xff];

    p1 -= BAR_WIDTH/2 - value/2;

    if(options.bool[OPTION_INVERT])
      i = graytab[0x72] & ((value&1)?0x0f:0xf0);
    else
      i = graytab[0x72] | ((value&1)?0xf0:0x0f);

    p1[0] = p1[80] = i;
  }
}

static void
draw_id (int id)
{
  char *s, *p = vidbase + ((160 * ID_Y) + ID_X)/pixperbyte;
  int y, x;
  const char gray[]={0xff,0xf0,0x0f,0x00};
  const char col[] = { 0xd2, 0x77, 0xff };
  const char chr[][8] = { 
    {0x7f, 0xff, 0xc0, 0xfe, 0x7f, 0x03, 0xff, 0xfe},	// S
    {0xff, 0xff, 0xc0, 0xf8, 0xf8, 0xc0, 0xff, 0xff},	// E
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
  };

  s = (char *) chr[id];		// fetch bitmap id

  if(color_gfx) {
    /* draw color id logo */
    for (y = 0; y < 8; y++) {
      for (x = 7; x >= 0; x--)
	*p++ = ((s[y]>>x) & 1)?col[id]:0xff;

      p += 160 - 8;
    }
  } else {
    if(options.bool[OPTION_INVERT]) {
      /* draw inverse grayscale id logo */
      for (y = 0; y < 8; y++, p += 80 - 4) {
	for (x = 3; x >= 0; x--)
	  *p++ = ~gray[(s[y]>>(2*x)) & 3];
      }
    } else {
      /* draw normal grayscale id logo */
      for (y = 0; y < 8; y++, p += 80 - 4) {
	for (x = 3; x >= 0; x--)
	  *p++ =  gray[(s[y]>>(2*x)) & 3];
      }
    }
  }
}

/* rotate object around objects axis */

void
rotate_matrix (RotVector *mat, Boolean axis_x, int direction)
{
  Int32 dcos, dsin;

  Int16 angle = ((long)ticks*(SIN_STEPS/8l))/(45 * 10);

  dsin = sintab[angle];
  dcos = sintab[SIN_STEPS/4+angle];

  if (direction > 0) dsin = -dsin;

  if (axis_x) {
    /* rotate around x axis (climb) */

    /* rotate mat[2] (z-vec) */
    mat[2].x = ((Int32)mat[1].x * dsin + (Int32)mat[2].x * dcos)>>ROTBITS;
    mat[2].y = ((Int32)mat[1].y * dsin + (Int32)mat[2].y * dcos)>>ROTBITS;
    mat[2].z = ((Int32)mat[1].z * dsin + (Int32)mat[2].z * dcos)>>ROTBITS;

    /* make sure, that the length of this vector is ok (1.000) */
    unit_rot_vector (&mat[2]);

    /* re-create mat[1] (y-vec) */
    mat[1].x = -((Int32)mat[0].y * (Int32)mat[2].z - 
		 (Int32)mat[0].z * (Int32)mat[2].y)>>ROTBITS;
    mat[1].y = -((Int32)mat[0].z * (Int32)mat[2].x - 
		 (Int32)mat[0].x * (Int32)mat[2].z)>>ROTBITS;
    mat[1].z = -((Int32)mat[0].x * (Int32)mat[2].y - 
		 (Int32)mat[0].y * (Int32)mat[2].x)>>ROTBITS;
  }
  else {
    /* rotate around z axis (roll) */

    /* rotate mat[1] (x-vec) */
    mat[1].x = ((Int32)mat[0].x * dsin + (Int32)mat[1].x * dcos)>>ROTBITS;
    mat[1].y = ((Int32)mat[0].y * dsin + (Int32)mat[1].y * dcos)>>ROTBITS;
    mat[1].z = ((Int32)mat[0].z * dsin + (Int32)mat[1].z * dcos)>>ROTBITS;

    /* make sure, that the length of this vector is ok (1.000) */
    unit_rot_vector (&mat[1]);

    /* re-create mat[1] (y-vec) */
    mat[0].x = -((Int32)mat[2].y * (Int32)mat[1].z - 
		 (Int32)mat[2].z * (Int32)mat[1].y)>>ROTBITS;
    mat[0].y = -((Int32)mat[2].z * (Int32)mat[1].x - 
		 (Int32)mat[2].x * (Int32)mat[1].z)>>ROTBITS;
    mat[0].z = -((Int32)mat[2].x * (Int32)mat[1].y - 
		 (Int32)mat[2].y * (Int32)mat[1].x)>>ROTBITS;
  }
}

/* alpha and beta 0..1 */
static void
rotate_matrix2 (RotVector *mat, Int32 alpha, Int32 beta)
{
  RotVector v;
  int i;

  for(i=0;i<3;i++) {
    v = mat[i];

    v.y = v.y - ((alpha * (Int32)v.x)>>ROTBITS);
    v.x = v.x + ((alpha * (Int32)v.y)>>ROTBITS);
    v.y = v.y - ((beta  * (Int32)v.z)>>ROTBITS);
    v.z = v.z + ((beta  * (Int32)v.y)>>ROTBITS);

    mat[i] = v;
  }
}

Int32 get_distance(Int32 x, Int32 y, Int32 z) {
  Int32 uni, fact=1;

  uni = 32768;
  while((labs(x)>uni)||(labs(y)>uni)||(labs(z)>uni)) {
    x >>= 1;
    y >>= 1;
    z >>= 1;
    fact <<= 1;
  }

  return(fact * int_sqrt (SQUARE(x) + SQUARE(y) + SQUARE(z)));
}

/*
 * Update an objects location in the universe.
 */

void
move_univ_object (struct univ_object *obj)
{
  Int32 x, y, z;
  Int32 k2;
  Int16 alpha, beta;
  Int32 speed;

  /* tune me ... */
  alpha = (FLIGHT_DESCALE_TICKS(flight_roll))<<(ROTBITS-8);
  beta  = (FLIGHT_DESCALE_TICKS(flight_climb))<<(ROTBITS-8);

  x = obj->location.x;
  y = obj->location.y;
  z = obj->location.z;

//  DEBUG_PRINTF("move object, a/b = %ld/%ld ", alpha, beta);

//  DEBUG_PRINTF("old pos: ");
//  DEBUG_PRINTF("%ld/%ld/%ld\n",  x,y,z);

  if (!(obj->flags & FLG_DEAD)) {

    if (obj->velocity != 0) {
      speed = 3 * obj->velocity / 2;
      x += ((Int32)obj->rotmat[2].x * speed)>>ROTBITS;
      y += ((Int32)obj->rotmat[2].y * speed)>>ROTBITS;
      z += ((Int32)obj->rotmat[2].z * speed)>>ROTBITS;
    }

    if (obj->acceleration != 0) {
      obj->velocity += obj->acceleration;
      obj->acceleration = 0;
      if (obj->velocity > ship_list[obj->type]->velocity)
	obj->velocity = ship_list[obj->type]->velocity;

      if (obj->velocity <= 0)
	obj->velocity = 1;
    }
  }

//  DEBUG_PRINTF("int pos: ");
//  DEBUG_PRINTF("%s/",  fix32_str(x));
//  DEBUG_PRINTF("%s/",  fix32_str(y));
//  DEBUG_PRINTF("%s\n", fix32_str(z));

//  DEBUG_PRINTF("distance = %ld\n", obj->distance); 
  
  /* rotate object around player */
  k2 = y - ((alpha * x)>>ROTBITS);
  z =  z + ((beta * k2)>>ROTBITS);
  y = k2 - ((beta *  z)>>ROTBITS);
  x =  x + ((alpha * y)>>ROTBITS);

  /* move object */
  z = z - (((long)ticks*(long)FLIGHT_DESCALE_TICKS(flight_speed)))/50l;

  obj->location.x = x;
  obj->location.y = y;
  obj->location.z = z;

//  DEBUG_PRINTF("dist: %ld -> ", obj->distance);

  obj->distance = get_distance(x,y,z);
//  DEBUG_PRINTF("%ld\n", obj->distance);

  // circles do not need to be rotated
  if(obj->type == SHIP_PLANET)  beta = 0;   

  /* rotate object around its own axis */
  rotate_matrix2(obj->rotmat, alpha, beta);
  
  if (obj->flags & FLG_DEAD) return;

  /* apply objects own rotation */
  if(obj->rotx != 0)  rotate_matrix(obj->rotmat, 1,  obj->rotx);
  if(obj->rotz != 0)  rotate_matrix(obj->rotmat, 0, -obj->rotz);
}

/*
 * Dock the player into the space station.
 */

void
dock_player (void)
{
  disengage_auto_pilot();
  is_docked = 1;
  flight_speed = FLIGHT_SCALE(0);
  flight_roll  = FLIGHT_SCALE(0);
  flight_climb = FLIGHT_SCALE(0);
  front_shield = 255;
  aft_shield = 255;
  energy = 255;
  myship.altitude = 255;
  myship.cabtemp = 30;
  reset_weapons ();

  /* save docked state */
  save_commander(AUTO_SAVE_SLOT);
}

/*
 * Check if we are correctly aligned to dock.
 */

static Bool
is_docking (int sn)
{
  RotVector vec;
  Int16 fz;
  Int16 ux;

  if (auto_pilot)		// Don't want it to kill anyone!
    return 1;

  fz = universe[sn].rotmat[2].z;

  /* check if ship is in front of station */
  if (fz > -90l * (Int32)ROTDIV / 100l) { // -0.90 
    DEBUG_PRINTF("docking: ship is not in front of station\n");
    return 0;
  }

  vec = unit_pos_vector(&universe[sn].location);

  /* check if station is in front of ship */
  if (vec.z < 927l * (Int32)ROTDIV / 1000l) { // 0.927
    DEBUG_PRINTF("docking: station is not in front of ship\n");
    return 0;
  }

  ux = universe[sn].rotmat[1].x;
  if (ux < 0) ux = -ux;

  /* correctly aligned to docking bay? */
  if (ux < 84l * (Int32)ROTDIV / 100l) { // 0.84
    DEBUG_PRINTF("docking: docking bay is not aligned\n");
    return 0;
  }

  return 1;
}

/*
 * Game Over...
 */

static void
do_game_over (void)
{
  snd_play_sample (SND_GAMEOVER);
  game_over = 1;
}

void
update_altitude (void)
{
  Int32 x, y, z;
  Int32 dist;

  myship.altitude = 255;

  if (witchspace)
    return;

  x = labs(universe[0].location.x);
  y = labs(universe[0].location.y);
  z = labs(universe[0].location.z);

  if ((x > 65535) || (y > 65535) || (z > 65535))
    return;

  x /= 256;
  y /= 256;
  z /= 256;

  dist = (x * x) + (y * y) + (z * z);

  if (dist > 65535)
    return;

  dist -= 9472;
  if (dist < 1) {
    myship.altitude = 0;
    DEBUG_PRINTF("game over: altitude low 1\n");
    do_game_over ();
    return;
  }

  dist = int_sqrt (dist);
  if (dist < 1) {
    myship.altitude = 0;
    DEBUG_PRINTF("game over: altitude low 2\n");
    do_game_over ();
    return;
  }

  myship.altitude = dist;
}

void
update_cabin_temp (void)
{
  Int32 x, y, z;
  Int32 dist;

  myship.cabtemp = 30;

  if (witchspace)
    return;

  if (ship_count[SHIP_CORIOLIS] || ship_count[SHIP_DODEC])
    return;

  x = labs(universe[1].location.x);
  y = labs(universe[1].location.y);
  z = labs(universe[1].location.z);

  /* too far from sun */
  if ((x > 65535) || (y > 65535) || (z > 65535))
    return;

  x /= 256;
  y /= 256;
  z /= 256;

  dist = ((x * x) + (y * y) + (z * z)) / 256;

  if (dist > 255)
    return;

  dist ^= 255;

  myship.cabtemp = dist + 30;

  if (dist > 255-30) {
    myship.cabtemp = 255;
    DEBUG_PRINTF("game over: cabin temp\n");
    do_game_over ();
    return;
  }

  if ((myship.cabtemp < 224) || (cmdr.fuel_scoop == 0))
    return;

  cmdr.fuel += FLIGHT_DESCALE_TICKS(flight_speed) / 2;
  if (cmdr.fuel > MAX_FUEL)
    cmdr.fuel = MAX_FUEL;

  info_message ("Fuel Scoop On");
}


/*
 * Regenerate the shields and the energy banks.
 */

void
regenerate_shields (void)
{
  if (energy > 127) {
    if (front_shield < 255) {
      front_shield++;
      energy--;
    }

    if (aft_shield < 255) {
      aft_shield++;
      energy--;
    }
  }

  if(energy < 255) energy++;

  if(cmdr.energy_unit) {
    if(energy <= 255-cmdr.energy_unit) 
      energy += cmdr.energy_unit;
    else
      energy = 255;
  }
}


void
decrease_energy (int amount)
{
  if(energy >= amount) energy -= amount;
  else                 energy  = 0;

  if (energy == 0) {
    DEBUG_PRINTF("game over: out of energy\n");
    do_game_over ();
  }
}


/*
 * Deplete the shields.  Drain the energy banks if the shields fail.
 */

void
damage_ship (int damage, int front)
{
  int shield;

  if (damage <= 0)		/* sanity check */
    return;

  shield = front ? front_shield : aft_shield;

  shield -= damage;
  if (shield < 0) {
    decrease_energy (-shield);
    shield = 0;
  }

  if (front)
    front_shield = shield;
  else
    aft_shield = shield;

  /* some force feedback */
  rumble(1);
}

//#define STAT_DIST 65792l
#define STAT_DIST 49152l

static void
make_station_appear (void)
{
  Int32 px, py, pz;
  Int32 sx, sy, sz;
  RotVector vec;
  RotMatrix rotmat;

  px = universe[0].location.x;
  py = universe[0].location.y;
  pz = universe[0].location.z;

  vec.x = (randint () & 32767) - 16384;
  vec.y = (randint () & 32767) - 16384;
  vec.z = randint () & 32767;

  unit_rot_vector (&vec);

  sx = px - ((Int32)vec.x * STAT_DIST)>>ROTBITS;
  sy = py - ((Int32)vec.y * STAT_DIST)>>ROTBITS;
  sz = pz - ((Int32)vec.z * STAT_DIST)>>ROTBITS;

//war schon     set_init_matrix (rotmat);

  rotmat[0].x = ROTDIV;
  rotmat[0].y = 0;
  rotmat[0].z = 0;

  rotmat[1].x = vec.x;
  rotmat[1].y = vec.z;
  rotmat[1].z = -vec.y;

  rotmat[2].x = vec.x;
  rotmat[2].y = vec.y;
  rotmat[2].z = vec.z;

  add_new_station (sx, sy, sz, rotmat);
}

static void
check_docking (int i)
{
  if (is_docking (i)) {
    snd_play_sample (SND_DOCK);
    dock_player ();
    current_screen = SCR_DOCKING_SEQUENCE;
    return;
  }

  if (flight_speed >= FLIGHT_SCALE(5)) {
    DEBUG_PRINTF("game over: fast dock\n");
    do_game_over ();
    return;
  }

  flight_speed = FLIGHT_SCALE(1);
  damage_ship (5, universe[i].location.z > 0);
  snd_play_sample (SND_CRASH);
}


static void
switch_to_view (struct univ_object *flip)
{
  Int32 tmp;

  if ((current_screen == SCR_REAR_VIEW) || (current_screen == SCR_GAME_OVER)) {
    flip->location.x = -flip->location.x;
    flip->location.z = -flip->location.z;

    flip->rotmat[0].x = -flip->rotmat[0].x;
    flip->rotmat[0].z = -flip->rotmat[0].z;

    flip->rotmat[1].x = -flip->rotmat[1].x;
    flip->rotmat[1].z = -flip->rotmat[1].z;

    flip->rotmat[2].x = -flip->rotmat[2].x;
    flip->rotmat[2].z = -flip->rotmat[2].z;
    return;
  }

  if (current_screen == SCR_LEFT_VIEW) {
    tmp = flip->location.x;
    flip->location.x = flip->location.z;
    flip->location.z = -tmp;

    if (flip->type < 0)
      return;

    tmp = flip->rotmat[0].x;
    flip->rotmat[0].x = flip->rotmat[0].z;
    flip->rotmat[0].z = -tmp;

    tmp = flip->rotmat[1].x;
    flip->rotmat[1].x = flip->rotmat[1].z;
    flip->rotmat[1].z = -tmp;

    tmp = flip->rotmat[2].x;
    flip->rotmat[2].x = flip->rotmat[2].z;
    flip->rotmat[2].z = -tmp;
    return;
  }

  if (current_screen == SCR_RIGHT_VIEW) {
    tmp = flip->location.x;
    flip->location.x = -flip->location.z;
    flip->location.z = tmp;

    if (flip->type < 0)
      return;

    tmp = flip->rotmat[0].x;
    flip->rotmat[0].x = -flip->rotmat[0].z;
    flip->rotmat[0].z = tmp;

    tmp = flip->rotmat[1].x;
    flip->rotmat[1].x = -flip->rotmat[1].z;
    flip->rotmat[1].z = tmp;

    tmp = flip->rotmat[2].x;
    flip->rotmat[2].x = -flip->rotmat[2].z;
    flip->rotmat[2].z = tmp;

  }
}


/*
 * Update all the objects in the universe and render them.
 */

void
update_universe (void)
{
  int i;
  int type;
  int bounty;
  char str[80];
  struct univ_object flip;

//  DEBUG_PRINTF("update universe\n");

  for (i = 0; i < MAX_UNIV_OBJECTS; i++) {
    type = universe[i].type;

    if (type != 0) {
//      DEBUG_PRINTF("update %d is type %ld\n", i, (long)type);

      if (universe[i].flags & FLG_REMOVE) {
	DEBUG_PRINTF("remove ship %d\n", i);

	if (type == SHIP_VIPER)
	  cmdr.legal_status |= 64;

	bounty = ship_list[type]->bounty;
	DEBUG_PRINTF("bounty = %d\n", bounty);

	if ((bounty != 0) && (!witchspace)) {
	  cmdr.credits += bounty;
	  StrPrintF (str, "Reward: %d.%d " CURRENCY, bounty / 10, bounty % 10);
	  info_message (str);
	}

	remove_ship (i);
	continue;
      }

      if ((detonate_bomb) && ((universe[i].flags & FLG_DEAD) == 0) &&
	  (type != SHIP_PLANET) && (type != SHIP_SUN) &&
	  (type != SHIP_CONSTRICTOR) && (type != SHIP_COUGAR) &&
	  (type != SHIP_CORIOLIS) && (type != SHIP_DODEC)) {
	snd_play_sample (SND_EXPLODE);
	universe[i].flags |= FLG_DEAD;
      }

      if ((current_screen != SCR_INTRO_ONE) &&
	  (current_screen != SCR_INTRO_TWO) &&
	  (current_screen != SCR_GAME_OVER) &&
	  (current_screen != SCR_ESCAPE_POD))
	tactics (i);

//      DEBUG_PRINTF("[%d]: ", i);
      move_univ_object (&universe[i]);

      flip = universe[i];
      switch_to_view (&flip);

      if (type == SHIP_PLANET) {
	if ((ship_count[SHIP_CORIOLIS] == 0) && 
	    (ship_count[SHIP_DODEC] == 0) && 
	    (universe[i].distance < 49152))	// was 49152 (65792)
	  make_station_appear ();

	draw_ship (&flip);
	continue;
      }

      if (type == SHIP_SUN) {
	draw_ship (&flip);
	continue;
      }

      if (universe[i].distance < 170) {
	if ((type == SHIP_CORIOLIS) || (type == SHIP_DODEC))
	  check_docking (i);
	else
	  scoop_item(i);

	continue;
      }

      if (universe[i].distance > DISMISS_DIST) {
	remove_ship (i);
	continue;
      }

      draw_ship (&flip);

      universe[i].flags = flip.flags;
      universe[i].exp_delta = flip.exp_delta;

      universe[i].flags &= ~FLG_FIRING;

      if (universe[i].flags & FLG_DEAD)
	continue;

      check_target (i, &flip);
    }
  }

  detonate_bomb = 0;
}


/*
 * Update the scanner and draw all the lollipops.
 */

static void
draw_lollipop(struct lollipop_struct *lp, UInt8 colour, UInt8 gray) {
  UInt8 *dst = vidbase + (lp->x1 + lp->y1*DSP_WIDTH)/pixperbyte;
  int   dir = (lp->y2<lp->y1)?(-DSP_WIDTH):(DSP_WIDTH);
  int   i,cnt = abs(lp->y2-lp->y1);

  if(color_gfx) {
    /* draw color lollipop */

    /* draw vertical bar */
    for(i=0;i<cnt;i++,dst+=dir)
      *dst = colour;
    
    /* draw lollipop */
    *(dst)             = colour;
    *(dst-DSP_WIDTH)   = colour;
    *(dst+1)           = colour;
    *(dst-DSP_WIDTH+1) = colour;
  } else {
    /* draw grayscale lollipop */
    UInt8 mask_fg = (lp->x1&1)?0x0f:0xf0;
    UInt8 mask_bg = ~mask_fg;
    UInt8 vcol, lcol;

    vcol = gray & mask_fg;
    lcol = gray & mask_bg;

    dir /= 2;

    /* draw vertical bar */
    for(i=0;i<cnt;i++,dst+=dir)
      *dst = (*dst & mask_bg) | vcol;
    
    /* draw lollipop */
    *(dst)              = (* dst              & mask_bg) | vcol;
    *(dst-DSP_WIDTH/2)  = (*(dst-DSP_WIDTH/2) & mask_bg) | vcol;

    if(lp->x1&1) dst++;
    *(dst)              = (* dst              & mask_fg) | lcol;
    *(dst-DSP_WIDTH/2)  = (*(dst-DSP_WIDTH/2) & mask_fg) | lcol;
  }
}

static void
undraw_lollipop(struct lollipop_struct *lp) {
  UInt8 *src = sprite_data[SPRITE_SCANNER] + lp->x1 + (lp->y1-DSP_HEIGHT)*DSP_WIDTH;
  UInt8 *dst = vidbase + (lp->x1 + lp->y1*DSP_WIDTH)/pixperbyte;
  int  dir = (lp->y2<lp->y1)?(-DSP_WIDTH):(DSP_WIDTH);
  int  i,cnt = abs(lp->y2-lp->y1);

  if(color_gfx) {
    /* undraw color lollipop */

    /* undraw vertical bar */
    for(i=0;i<cnt;i++,src+=dir,dst+=dir)
      *dst = *src;
    
    /* undraw lollipop */
    *(dst)             = *(src);
    *(dst-DSP_WIDTH)   = *(src-DSP_WIDTH);
    *(dst+1)           = *(src+1);
    *(dst-DSP_WIDTH+1) = *(src-DSP_WIDTH+1);
  } else {
    /* undraw grayscale lollipop */
    UInt8 mask_fg = (lp->x1&1)?0x0f:0xf0;
    UInt8 mask_bg = ~mask_fg;
    int dir2 = dir/2;

    /* undraw vertical bar */
    for(i=0;i<cnt;i++,dst+=dir2,src+=dir)
      *dst = (*dst & mask_bg) | (*src & mask_fg);

    /* undraw lollipop */
    *(dst)             = (* dst              & mask_bg) | 
	                 (* src              & mask_fg);
    *(dst-DSP_WIDTH/2) = (*(dst-DSP_WIDTH/2) & mask_bg) | 
                         (*(src-DSP_WIDTH)   & mask_fg);

    if(lp->x1&1) dst++;
    src++;

    *(dst)             = (* dst              & mask_fg) | 
                         (* src              & mask_bg);
    *(dst-DSP_WIDTH/2) = (*(dst-DSP_WIDTH/2) & mask_fg) | 
                         (*(src-DSP_WIDTH)   & mask_bg);
  }
}

void 
scanner_clear_buffer(void) {
  int i;

  /* currently no lollipop has been drawn */
  for (i = 0; i < MAX_UNIV_OBJECTS; i++) 
    lollipop[i].mapped = 0;
}

static void
update_scanner (void)
{
  int i;
  int x1, y1, y2;
  UInt8 colour, gray;
  static int blink_cnt=0;

  blink_cnt++;

  /* mass lock due to all objects */
  /* objects are removed when dist > 57344 */

  for (i = 0; i < MAX_UNIV_OBJECTS; i++) {
    /* undraw an existing lollipop */
    if(lollipop[i].mapped) {
      undraw_lollipop(&lollipop[i]);
      lollipop[i].mapped = 0;
    }

    if ((universe[i].type <= 0) ||
	(universe[i].flags & FLG_DEAD) || (universe[i].flags & FLG_CLOAKED))
      continue;

    x1 = universe[i].location.x / 512;
    y1 = -universe[i].location.z / 2048;
    y2 = y1 - universe[i].location.y / 2048;

#if 0
    /* footprint of loollipop is in scanner 12 * 2048 = 24576 */
    if((y1 < -12) || (y1 > 12)) continue;

    /* lollipop has some additional space to the top 18 * 2048 = 36864 */
    if((y2 < -18) || (y2 > 12)) continue;

    /* horizontal limits to 43 * 512 = 22016 */
    if((x1 < -43) || (x1 > 43)) continue;
#else
    if(universe[i].distance > RADAR_DIST) continue;
#endif

    /* center in scanner */
    x1 += DSP_WIDTH/2;
    y1 += DSP_HEIGHT+20;
    y2 += DSP_HEIGHT+20;

    colour = GFX_COL_WHITE;   // default color
    gray   = 0x00;
    
    if(universe[i].flags & FLG_HOSTILE) {
      colour = GFX_COL_YELLOW_4;
      if(blink_cnt & 0x2) continue;
    }

    switch (universe[i].type) {
    case SHIP_MISSILE:
      colour = GFX_COL_PINK_1;	// pink
      gray   = 0x22;
      break;

    case SHIP_DODEC:
    case SHIP_CORIOLIS:
      colour = GFX_COL_GREEN_1;	// green
      gray   = 0x44;
      break;

    case SHIP_VIPER:
      colour = GFX_COL_BLUE_1;  // blue
      gray   = 0x66;
      break;
    }

    if(!color_gfx && options.bool[OPTION_INVERT])
      gray = ~gray;

    /* finally draw the lollipop */
    lollipop[i].mapped = 1;
    lollipop[i].x1 = x1;
    lollipop[i].y1 = y1;
    lollipop[i].y2 = y2;
    draw_lollipop(&lollipop[i], colour, gray);
  }
}

#define STATUS_CENTER_X  (36)
#define STATUS_CENTER_Y  (DSP_HEIGHT+33)

static void
show_status(void) {
  UInt16 *d = (UInt16*)(vidbase + (160 * STATUS_CENTER_Y + STATUS_CENTER_X)/pixperbyte);
  int type,i,condition,pat;

  const UInt16 *s;
  const UInt16 col_patterns[][6] = {
    { 0xffff, 0xffff,  0xffff, 0xffff,  0xffff, 0xffff },
    { 0xd4d3, 0xd4ff,  0xd3d2, 0xd3ff,  0xd4d3, 0xd4ff },
    { 0x9373, 0x93ff,  0x7372, 0x73ff,  0x9373, 0x93ff },
    { 0xa18f, 0xa1ff,  0x8f77, 0x8fff,  0xa18f, 0xa1ff }};
  const UInt16 gray_patterns[][3] = {
    { 0xffff, 0xffff, 0xffff}, 
    { 0xffff, 0xffff, 0xffff}, 
    { 0xc8cf, 0x858f, 0xc8cf}, 
    { 0x505f, 0x000f, 0x505f}};

  if (is_docked) pat = 0;
  else {
    pat = 1;

    for (i = 0; i < MAX_UNIV_OBJECTS; i++) {
      type = universe[i].type;

      if (((type == SHIP_MISSILE) ||
	  ((type > SHIP_ROCK) && (type < SHIP_DODEC)))&&
	  (universe[i].distance <= RADAR_DIST)) {

	pat = 2;
	break;
      }
    }

    /* low energy */
    if ((pat == 2) && (energy < 128))
      pat = 3;
  }

  if(color_gfx) {
    /* draw color marker */
    s = col_patterns[pat];
    *d++=*s++; *d++=*s++; d+=(DSP_WIDTH-4)/2;
    *d++=*s++; *d++=*s++; d+=(DSP_WIDTH-4)/2;
    *d++=*s++; *d++=*s++; 
  } else {
    /* draw grayscale marker */
    s = gray_patterns[pat];

    if(options.bool[OPTION_INVERT]) {
      *d++=~*s++; d+=(DSP_WIDTH-4)/4;
      *d++=~*s++; d+=(DSP_WIDTH-4)/4;
      *d++=~*s++;
    } else {
      *d++=*s++; d+=(DSP_WIDTH-4)/4;
      *d++=*s++; d+=(DSP_WIDTH-4)/4;
      *d++=*s++;
    }
  }
}

/*
 * Update the compass which tracks the space station / planet.
 */

#define COMPASS_CENTER_X  (118)
#define COMPASS_CENTER_Y  (DSP_HEIGHT+7)

static void
update_compass(Boolean draw)
{
  UInt8 *s, *p;
  RotVector dest;
  int compass_x, compass_y;
  
  /* color */
  const UInt8 green[]={ 0xd4, 0xd3, 0xd4, 0xd3, 0xd2, 0xd3, 0xd4, 0xd3, 0xd4};
  const UInt8 red[]  ={ 0xa1, 0x8f, 0xa1, 0x8f, 0x77, 0x8f, 0xa1, 0x8f, 0xa1};
  
  /* normal grayscale */
  const UInt8 lgray[]={ 0x88, 0x33, 0x88, 0x33, 0x00, 0x33, 0x88, 0x33, 0x88};
  const UInt8 dgray[]={ 0xbb, 0x55, 0xbb, 0x55, 0x33, 0x55, 0xbb, 0x55, 0xbb};

  /* inverted grayscale */
  const UInt8 lgrai[]={ 0x77, 0xcc, 0x77, 0xcc, 0xff, 0xcc, 0x77, 0xcc, 0x77};
  const UInt8 dgrai[]={ 0x44, 0xaa, 0x44, 0xaa, 0xcc, 0xaa, 0x44, 0xaa, 0x44};

  /* data for sprite buffer */
  static char *last = NULL;

  if (last != NULL) {
    p = last;

    if(color_gfx) {
      /* undraw existing color sprite if required */
      s = sprite_data[SPRITE_SCANNER] + ((last-vidbase)-(DSP_HEIGHT*DSP_WIDTH));
      *p++ = *s++; *p++ = *s++; *p++ = *s++; s += DSP_WIDTH-3; p += DSP_WIDTH-3;
      *p++ = *s++; *p++ = *s++; *p++ = *s++; s += DSP_WIDTH-3; p += DSP_WIDTH-3;
      *p++ = *s++; *p++ = *s++; *p++ = *s++;
      p = vidbase + 160 * COMPASS_CENTER_Y + COMPASS_CENTER_X;
    } else {
      /* undraw existing grayscale sprite if required */
      s = sprite_data[SPRITE_SCANNER] + (2*(last-vidbase)-(DSP_HEIGHT*DSP_WIDTH));
      p[0] = (s[0] & 0xf0) | (s[1] & 0x0f); 
      p[1] = (s[2] & 0xf0) | (s[3] & 0x0f); 
      s += DSP_WIDTH; p += DSP_WIDTH/2;
      p[0] = (s[0] & 0xf0) | (s[1] & 0x0f); 
      p[1] = (s[2] & 0xf0) | (s[3] & 0x0f); 
      s += DSP_WIDTH; p += DSP_WIDTH/2;
      p[0] = (s[0] & 0xf0) | (s[1] & 0x0f); 
      p[1] = (s[2] & 0xf0) | (s[3] & 0x0f); 

      p = vidbase + (160 * COMPASS_CENTER_Y + COMPASS_CENTER_X)/2;
    }
    last == NULL;
  } else
    p = vidbase + (160 * COMPASS_CENTER_Y + COMPASS_CENTER_X)/pixperbyte;    

  if(!draw) return;

  /* no compass in with space */
  if (witchspace) return;

//  DEBUG_PRINTF("Pos: %ld/%ld/%ld\n", universe[1].location.x,
//	       universe[1].location.y, universe[1].location.z);

  dest = unit_pos_vector(&universe[(ship_count[SHIP_CORIOLIS]||
		   ship_count[SHIP_DODEC])?1:0].location);

  if(color_gfx) {
    /* draw color sprite */
    s = (UInt8*) ((dest.z < 0) ? red : green);

    p += (ROTDIV/2 + 6l * dest.x)>>ROTBITS;
    p -= 160 * ((ROTDIV/2 + 6l * dest.y)>>ROTBITS);

    /* save this position */
    last = p;

    /* draw marker */
    *p++=*s++; *p++=*s++; *p++=*s++; p+=DSP_WIDTH-3;
    *p++=*s++; *p++=*s++; *p++=*s++; p+=DSP_WIDTH-3;
    *p++=*s++; *p++=*s++; *p++=*s++;
  } else {
    /* draw grayscale sprite */
    Int32 n = (6l * dest.x)>>ROTBITS;

    if(options.bool[OPTION_INVERT])
      s = (UInt8*) ((dest.z < 0) ? lgrai : dgrai);
    else
      s = (UInt8*) ((dest.z < 0) ? lgray : dgray);

    p += n/2; n &= 1;
    p -= 80 * ((6l * dest.y)>>ROTBITS);

    /* save this position */
    last = p;


    /* wow, this is ulgy, but i am too lazy ... */
    if(n==0) {
      p[0] = (0xf0 & s[0]) | (0x0f & s[1]); p[1] = (p[1] & 0x0f) | (0xf0 & s[2]);
      p += 80; s += 3;
      p[0] = (0xf0 & s[0]) | (0x0f & s[1]); p[1] = (p[1] & 0x0f) | (0xf0 & s[2]);
      p += 80; s += 3;
      p[0] = (0xf0 & s[0]) | (0x0f & s[1]); p[1] = (p[1] & 0x0f) | (0xf0 & s[2]);
    } else {
      p[0] = (p[0] & 0xf0) | (0x0f & s[0]); p[1] = (0xf0 & s[1]) | (0x0f & s[2]);
      p += 80; s += 3;
      p[0] = (p[0] & 0xf0) | (0x0f & s[0]); p[1] = (0xf0 & s[1]) | (0x0f & s[2]);
      p += 80; s += 3;
      p[0] = (p[0] & 0xf0) | (0x0f & s[0]); p[1] = (0xf0 & s[1]) | (0x0f & s[2]);
    }
  }
}

static void
display_missiles (void)
{
  char *p2, *p1 = vidbase + ((160 * (DSP_HEIGHT + INST_MS_Y)) + 
			     LEFT_SIDE_X + 1)/pixperbyte;
  int n = ((160 * (DSP_HEIGHT + INST_MS_Y)) + LEFT_SIDE_X + 1)&3;
  int i;
  UInt32 col;
  const UInt32 graymap[]={0xffff0000, 0x0ffff000, 0x00ffff00, 0x000ffff0};

  for (i = 0; i < 4; i++) {
    if (cmdr.missiles >= 4 - i) {
      col = 0xd2;	        /* green -> present */

      if ((cmdr.missiles == 4 - i) && (missile_target != MISSILE_UNARMED)) {
	if (missile_target < 0)
	  col = 0x72;	        /* yellow -> armed */
	else
	  col = 0x77;	        /* red -> armed and targetted */
      }
    }
    else
      col = 0xff;		/* black -> no missile */

    if(color_gfx) {
      /* draw color missile */
      p2 = p1 + 160;
      col = COL_EXPAND_32(col);

      if((long)p1 & 1) {
	/* odd address */
	*(UInt32*)(p1-1) = *(UInt32*)(p2-1) = col | 0xff000000;
	p1[3] = p2[3] = col;
      } else
	/* even address */
	*(UInt32*)p1 = *(UInt32*)p2 = col;

      p1+=5;
    } else {
      /* draw grayscale missile */
      p1 = (char*)((long)p1 & ~1);  // even address
      p2 = p1 + 80;
      col = COL_EXPAND_32(graytab[col]);

      *(UInt32*)p1 = *(UInt32*)p2 = ((*(UInt32*)p1) & ~graymap[n]) | 
	                                    (graymap[n] & col);
      n = (n+1)&3;
      p1 += 3;
      if(n==0) p1++;
    }
  }
}


void
update_console (Bool all)
{
  int i, e;
  static UInt8 dsp_cnt = 0;

  /*** left side ***/

  /* display shield strength */
  draw_instr_bar (0, 0, INST_FS_Y, front_shield);
  draw_instr_bar (0, 0, INST_AS_Y, aft_shield);

  /* don't always update everything */
  dsp_cnt++;

  if(all || ((dsp_cnt & 3) == 0)) {
    /* display fuel */
    draw_instr_bar (0, 0, INST_FU_Y, (cmdr.fuel * 256) / MAX_FUEL);
  }

  if(all || ((dsp_cnt & 3) == 1)) {
    /* display cabin temperature */
    draw_instr_bar (0, 0, INST_CT_Y, myship.cabtemp);
    /* display laser temperature */
    draw_instr_bar (0, 0, INST_LT_Y, laser_temp);
  }
  
  if(all || ((dsp_cnt & 3) == 2)) {
    /* display altitude */
    draw_instr_bar (0, 0, INST_AL_Y, myship.altitude);
    /* display missiles */
    display_missiles ();
  }

  /*** right side ***/

  /* display the speed bar */
  draw_instr_bar (1, flight_speed > (MAX_SPEED * 2 / 3),
		  INST_SP_Y, ((flight_speed) / (MAX_SPEED/256)) - 1);

  /* display roll indicator */
  draw_instr_ind (INST_RL_Y, 10 - ((flight_roll) / (MAX_ROLL/9)));

  /* display climb indicator */
  draw_instr_ind (INST_CL_Y, 10 - ((flight_climb) / (MAX_CLIMB/9)));

  /* display energy */
  for (e = 4 * (energy + 1) - 1, i = 0; i < 4; i++)
    draw_instr_bar (1, energy < 32, INST_EN_Y + 5 * i, e - 256 * (3 - i));

  if(dialog_btn_active) return;

  if(all || ((dsp_cnt & 3) == 3)) {
    /* show status docked/green/yellow/red */
    show_status();
  }

  /* do not update compass nor scanner if docked */
  if(all || ((dsp_cnt & 3) == 1))
    update_compass(!is_docked);

  if (is_docked) {
    draw_id (ID_NONE);
  } else {
    if(all || ((dsp_cnt & 3) == 2)) 
      update_scanner();

    if(all || ((dsp_cnt & 3) == 3)){
      if (ship_count[SHIP_CORIOLIS] || ship_count[SHIP_DODEC])
	draw_id (ID_SAVE);
      else if (ecm_active)
	draw_id (ID_ECM);
      else
	draw_id (ID_NONE);
    }
  }
}

#define ROLL_RESPONSE 5
#define DIVE_RESPONSE 5

void
increase_flight_roll (int n)
{
  n *= ticks * ROLL_RESPONSE;

  if (flight_roll <= MAX_ROLL-n)
    flight_roll += n;
  else
    flight_roll = MAX_ROLL;
}

void
decrease_flight_roll (int n)
{
  n *= ticks * ROLL_RESPONSE;

  if (flight_roll >= -(MAX_ROLL-n))
    flight_roll -= n;
  else
    flight_roll = -MAX_ROLL;
}

void
increase_flight_climb (int n)
{
  n *= ticks * DIVE_RESPONSE;

  if (flight_climb <= MAX_CLIMB-n)
    flight_climb += n;
  else
    flight_climb = MAX_CLIMB;
}

void
decrease_flight_climb (int n)
{
  n *= ticks * DIVE_RESPONSE;

  if (flight_climb >= -(MAX_CLIMB-n))
    flight_climb -= n;
  else
    flight_climb = -MAX_CLIMB;
}


void
start_hyperspace (void)
{
  if (hyper_ready)
    return;

  hyper_distance = calc_distance_to_planet (docked_planet, hyperspace_planet);

  if ((hyper_distance == 0) || (hyper_distance > cmdr.fuel))
    return;

  destination_planet = hyperspace_planet;
  name_planet (hyper_name, destination_planet);
  capitalise_name (hyper_name);

  hyper_ready = 1;
  hyper_countdown = 15;
  hyper_galactic = 0;

  disengage_auto_pilot();
}

void
start_galactic_hyperspace (void)
{
  if (hyper_ready)
    return;

  if (cmdr.galactic_hyperdrive == 0)
    return;

  hyper_ready = 1;
  hyper_countdown = 5;
  hyper_galactic = 1;
  disengage_auto_pilot();
}

void
display_hyper_status (void)
{
  char str[32];

  if ((current_screen == SCR_FRONT_VIEW) || 
      (current_screen == SCR_REAR_VIEW) || 
      (current_screen == SCR_LEFT_VIEW) || 
      (current_screen == SCR_RIGHT_VIEW)) {

    /* count down timer */
    StrPrintF(str, "%d", hyper_countdown);
    gfx_display_text(5, 5, ALIGN_LEFT, str, GFX_COL_WHITE);

    if (hyper_galactic) {
      gfx_display_text(DSP_CENTER, MESSAGE_Y, ALIGN_CENTER,
		       "Galactic Hyperspace", GFX_COL_WHITE);
    } else {
      StrPrintF (str, "Hyperspace - %s", hyper_name);
      gfx_display_text(DSP_CENTER, 80, 
		       ALIGN_CENTER, str, GFX_COL_WHITE);
    }
  }
}


static int
rotate_byte_left (int x)
{
  return ((x << 1) | (x >> 7)) & 255;
}

static void
enter_next_galaxy (void)
{
  cmdr.galaxy_number++;
  cmdr.galaxy_number &= 7;

  cmdr.galaxy.a = rotate_byte_left (cmdr.galaxy.a);
  cmdr.galaxy.b = rotate_byte_left (cmdr.galaxy.b);
  cmdr.galaxy.c = rotate_byte_left (cmdr.galaxy.c);
  cmdr.galaxy.d = rotate_byte_left (cmdr.galaxy.d);
  cmdr.galaxy.e = rotate_byte_left (cmdr.galaxy.e);
  cmdr.galaxy.f = rotate_byte_left (cmdr.galaxy.f);

  docked_planet = find_planet (0x60, 0x60);
  hyperspace_planet = docked_planet;
}


static void
enter_witchspace (void)
{
  int i;
  int nthg;

  witchspace = 1;
  docked_planet.b ^= 31;
  in_battle = 1;

  flight_speed = FLIGHT_SCALE(12);
  flight_roll  = FLIGHT_SCALE(0);
  flight_climb = FLIGHT_SCALE(0);
  create_new_stars ();
  clear_universe ();

  nthg = (randint () & 3) + 1;

  for (i = 0; i < nthg; i++)
    create_thargoid ();

  if(hyper_galactic) current_screen = SCR_GALACTIC_HYPERSPACE;
  else               current_screen = SCR_HYPERSPACE;

  snd_play_sample (SND_HYPERSPACE);
}


static void
complete_hyperspace (void)
{
  RotMatrix rotmat;
  long px, py, pz;

  hyper_ready = 0;
  witchspace = 0;

  /* drop lock on target */
  if (missile_target > MISSILE_UNARMED) {
    missile_target = MISSILE_UNARMED;
    info_message ("Target Lost");
  }

  if (hyper_galactic) {
    cmdr.galactic_hyperdrive = 0;
    enter_next_galaxy ();
    cmdr.legal_status = 0;
  } else {
    cmdr.fuel -= hyper_distance;
    cmdr.legal_status /= 2;

    if ((rand255 () > 253) || (flight_climb == MAX_CLIMB)) {
      enter_witchspace ();
      return;
    }

    docked_planet = destination_planet;
  }

  cmdr.market_rnd = rand255 ();
  generate_planet_data (&current_planet_data, docked_planet);
  generate_stock_market ();

  flight_speed = FLIGHT_SCALE(12);
  flight_roll  = FLIGHT_SCALE(0);
  flight_climb = FLIGHT_SCALE(0);
  create_new_stars ();
  clear_universe ();

  set_init_matrix (rotmat);

  pz = (((docked_planet.b) & 7) + 7) / 2;
  px = pz / 2;
  py = px;

  px <<= 16;
  py <<= 16;
  pz <<= 16;

  if ((docked_planet.b & 1) == 0) {
    px = -px;
    py = -py;
  }

  add_new_ship (SHIP_PLANET, px, py, pz, rotmat, 0, 0);

  pz = -((((long) docked_planet.d & 7) | 1) << 16);
  px = (((long) docked_planet.f & 3) << 16) |
    (((long) docked_planet.f & 3) << 8);

  add_new_ship (SHIP_SUN, px, py, pz, rotmat, 0, 0);

  if(hyper_galactic) current_screen = SCR_GALACTIC_HYPERSPACE;
  else               current_screen = SCR_HYPERSPACE;

  snd_play_sample (SND_HYPERSPACE);
}


void
countdown_hyperspace (void)
{
  if (hyper_countdown == 0) {
    complete_hyperspace ();
    return;
  }

  hyper_countdown--;
}



Boolean
jump_warp (void)
{
  int i;
  int type;
  Int32 jump;
  const char *mass = "Mass Locked";

  for (i = 0; i < MAX_UNIV_OBJECTS; i++) {
    type = universe[i].type;

    /* object must be in radar scanner distance */
    if ((type > 0) && (type != SHIP_ASTEROID) && (type != SHIP_CARGO) &&
	(type != SHIP_ALLOY) && (type != SHIP_ROCK) &&
	(type != SHIP_BOULDER) && (type != SHIP_ESCAPE_CAPSULE) &&
        (universe[i].distance <= RADAR_DIST)) {
      info_message((char*)mass);
      return 0;
    }
  }

  /* Mass locked due to planet */
  if ((universe[0].distance < 75001) || (universe[1].distance < 75001)) {
    info_message((char*)mass);
    return 0;
  }


  if (universe[0].distance < universe[1].distance)
    jump = universe[0].distance - 75000l;
  else
    jump = universe[1].distance - 75000l;

  if (jump > 1024l)
    jump = 1024l;

  for (i = 0; i < MAX_UNIV_OBJECTS; i++) {
    if (universe[i].type != 0)
      universe[i].location.z -= jump;
  }

  warp_stars = 1;
  mcount &= 63;
  in_battle = 0;

  return 1;
}

void
launch_player (void)
{
  current_screen = SCR_LAUNCH_SEQUENCE;
  gfx_update_button_map(0);
  snd_play_sample (SND_LAUNCH);
}



/*
 * Engage the docking computer.
 * For the moment we just do an instant dock if we are in the safe zone.
 */

void
engage_docking_computer (void)
{
  if (ship_count[SHIP_CORIOLIS] || ship_count[SHIP_DODEC]) {
    snd_play_sample (SND_DOCK);
    dock_player ();
    current_screen = SCR_DOCKING_SEQUENCE;
  }
}
