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
 * swat.c
 *
 * Special Weapons And Tactics.
 */

#include <PalmOS.h>

#include "config.h"
#include "gfx.h"
#include "elite.h"
#include "vector.h"
#include "swat.h"
#include "shipdata.h"
#include "space.h"
#include "main.h"
#include "sound.h"
#include "random.h"
#include "trade.h"
#include "pilot.h"
#include "debug.h"
#include "colors.h"
#include "threed.h"
#include "docked.h"

/* local prototypes */
static void make_angry (int) SEC3D;
static void track_object (struct univ_object *, Int32, RotVector) SEC3D;
static void missile_tactics (int) SEC3D;

static void launch_enemy (int, int, int, int) SEC3D;
static void launch_loot (int, int) SEC3D;
static void launch_shuttle (void) SEC3D;

static int  create_other_ship (int) SEC3D;
static void create_cougar (void) SEC3D; 
static void create_trader (void) SEC3D;
static void create_lone_hunter (void) SEC3D;

static void check_for_asteroids (void) SEC3D;
static void check_for_cops (void) SEC3D;
static void check_for_others (void) SEC3D;

UInt8 laser_counter;
UInt8 laser;
UInt8 laser2;
UInt8 laser_x;
UInt8 laser_y;

UInt8 ecm_active;
Int8 missile_target;
UInt8 ecm_ours;
UInt8 in_battle;

struct univ_object universe[MAX_UNIV_OBJECTS];
int ship_count[NO_OF_SHIPS + 1];	/* many */

void
clear_universe (void)
{
  int i;

  for (i = 0; i < MAX_UNIV_OBJECTS; i++)
    universe[i].type = 0;

  for (i = 0; i <= NO_OF_SHIPS; i++)
    ship_count[i] = 0;

  in_battle = 0;
}

int
add_new_ship (int ship_type, Int32 x, Int32 y, Int32 z,
	      RotVector *rotmat, int rotx, int rotz)
{
  int i;

  const UInt16 initial_flags[NO_OF_SHIPS + 1] = {
    0,				        // NULL,
    0,				        // missile 
    0,				        // coriolis
    FLG_SLOW | FLG_FLY_TO_PLANET,	// escape
    FLG_INACTIVE,			// alloy
    FLG_INACTIVE,			// cargo
    FLG_INACTIVE,			// boulder
    FLG_INACTIVE,			// asteroid
    FLG_INACTIVE,			// rock
    FLG_FLY_TO_PLANET | FLG_SLOW,	// shuttle
    FLG_FLY_TO_PLANET | FLG_SLOW,	// transporter

    0,				        // cobra3
    0,				        // python
    0,				        // boa
    FLG_SLOW,			        // anaconda

    FLG_SLOW,			        // hermit
    FLG_BOLD | FLG_POLICE,	        // viper

    FLG_BOLD | FLG_ANGRY,		// sidewinder
    FLG_BOLD | FLG_ANGRY,		// mamba
    FLG_BOLD | FLG_ANGRY,		// krait
    FLG_BOLD | FLG_ANGRY,		// adder
    FLG_BOLD | FLG_ANGRY,		// gecko
    FLG_BOLD | FLG_ANGRY,		// cobra1
    FLG_SLOW | FLG_ANGRY,		// worm
    FLG_BOLD | FLG_ANGRY,		// moray

    FLG_BOLD | FLG_ANGRY,		// cobra3
    FLG_BOLD | FLG_ANGRY,		// asp2
    FLG_BOLD | FLG_ANGRY,		// python
    FLG_POLICE,			        // fer_de_lance

    FLG_BOLD | FLG_ANGRY,		// thargoid
    FLG_ANGRY,			        // thargon
    FLG_ANGRY,			        // constrictor
    FLG_POLICE | FLG_CLOAKED,	        // cougar
    0				        // dodec
  };
  
  if(ship_type == -2) {
    DEBUG_PRINTF ("Adding a new ship -2 (Sun) ");
  } else if(ship_type == -1) {
    DEBUG_PRINTF ("Adding a new ship -1 (Planet) ");
  } else if(ship_type > 0) {
    DEBUG_PRINTF ("Adding a new ship %ld (%s) ", 
		  (long)ship_type, ship_list[ship_type]->name);
  } else {
    DEBUG_PRINTF ("Adding unknown new ship %ld ", 
		  (long)ship_type);
  }

  DEBUG_PRINTF("at %ld/%ld/%ld ", x,y,z);

  /* search a free space entry */
  for (i = 0; i < MAX_UNIV_OBJECTS; i++) {
    if (universe[i].type == 0) {
#if 0
      DEBUG_PRINTF("object[1]: %ld/%ld/%ld ",
		   universe[1].location.x,
		   universe[1].location.y,
		   universe[1].location.z);
#endif

      universe[i].type = ship_type;
      universe[i].location.x = x;
      universe[i].location.y = y;
      universe[i].location.z = z;

      universe[i].distance = get_distance(x, y, z);

      universe[i].rotmat[0] = rotmat[0];
      universe[i].rotmat[1] = rotmat[1];
      universe[i].rotmat[2] = rotmat[2];

      universe[i].rotx = rotx;
      universe[i].rotz = rotz;

      universe[i].velocity = 0;
      universe[i].acceleration = 0;
      universe[i].bravery = 0;
      universe[i].target = 0;

      universe[i].exp_delta = 0;

      if ((ship_type != SHIP_PLANET) && (ship_type != SHIP_SUN)) {
	universe[i].flags = initial_flags[ship_type];
	universe[i].energy = ship_list[ship_type]->energy;
	universe[i].missiles = ship_list[ship_type]->missiles;
	ship_count[ship_type]++;
      } else {
	universe[i].flags = 0;
	universe[i].energy = 0;
	universe[i].missiles = 0;
      }

      DEBUG_PRINTF("-> slot %d\n", i);
      return i;
    }
  }

  DEBUG_PRINTF("-> no free slot!n");
  return -1;
}




void
check_missiles (int un)
{
  int i;

  if (missile_target == un) {
    missile_target = MISSILE_UNARMED;
    info_message ("Target Lost");
  }

  // remove all missiles aiming for that target
  for (i = 0; i < MAX_UNIV_OBJECTS; i++) {
    if ((universe[i].type == SHIP_MISSILE) && (universe[i].target == un))
      universe[i].flags |= FLG_DEAD;
  }
}


void
remove_ship (int un)
{
  int type;
  RotMatrix rotmat;
  Int32 px, py, pz;

  type = universe[un].type;

  DEBUG_PRINTF("remove ship id %d (type = %ld)\n", un, (long)type);

  if (type == 0)
    return;

  if (type > 0)
    ship_count[type]--;

  universe[un].type = 0;

  check_missiles (un);

  if ((type == SHIP_CORIOLIS) || (type == SHIP_DODEC)) {
    set_init_matrix (rotmat);
    px = universe[un].location.x;
    py = universe[un].location.y;
    pz = universe[un].location.z;

    py &= 0xFFFF;
    py |= 0x60000;

    add_new_ship (SHIP_SUN, px, py, pz, rotmat, 0, 0);
  }
}


void
add_new_station (Int32 sx, Int32 sy, Int32 sz, RotMatrix rotmat)
{
  int station;

  station =
    (current_planet_data.techlevel >= 10) ? SHIP_DODEC : SHIP_CORIOLIS;
  universe[1].type = 0;
  add_new_ship (station, sx, sy, sz, rotmat, 0, -127);
}




void
reset_weapons (void)
{
  laser_temp = 0;
  laser_counter = 0;
  laser = 0;
  ecm_active = 0;
  missile_target = MISSILE_UNARMED;
}


static void
launch_enemy (int un, int type, int flags, int bravery)
{
  int newship;
  struct univ_object *ns;
  RotVector t;

  newship = add_new_ship (type,
			  universe[un].location.x,
			  universe[un].location.y,
			  universe[un].location.z,
			  universe[un].rotmat,
			  universe[un].rotx, universe[un].rotz);

  if (newship == -1)
    return;

  ns = &universe[newship];

  if ((universe[un].type == SHIP_CORIOLIS) ||
      (universe[un].type == SHIP_DODEC)) {

    /* place launching ships */
    ns->velocity = 32;
    ns->location.x += ns->rotmat[2].x / (ROTDIV/2);
    ns->location.y += ns->rotmat[2].y / (ROTDIV/2);
    ns->location.z += ns->rotmat[2].z / (ROTDIV/2);

    /* rotate ship by 90 degree */
    /* save x vector */
    t = ns->rotmat[0];
    
    /* x vector = - y vector */
    ns->rotmat[0].x = -ns->rotmat[1].x; 
    ns->rotmat[0].y = -ns->rotmat[1].y;
    ns->rotmat[0].z = -ns->rotmat[1].z;

    /* y vector = x vector */
    ns->rotmat[1] = t;

    /* z vector does not change */
    /* ... */
  }

  ns->flags |= flags;
//  ns->rotz /= 2;
//  ns->rotz *= 2;
  ns->bravery = bravery;

  if ((type == SHIP_CARGO) || (type == SHIP_ALLOY) || (type == SHIP_ROCK)) {
    ns->rotz = ((rand255 () * 2) & 255) - 128;
    ns->rotx = ((rand255 () * 2) & 255) - 128;
    ns->velocity = rand255 () & 15;
  }
}


static void
launch_loot (int un, int loot)
{
  int i, cnt;

  if (loot == SHIP_ROCK) {
    cnt = rand255 () & 3;
  }
  else {
    cnt = rand255 ();
    if (cnt >= 128)
      return;

    cnt &= ship_list[universe[un].type]->max_loot;
    cnt &= 15;
  }

  for (i = 0; i < cnt; i++) {
    launch_enemy (un, loot, 0, 0);
  }
}




Boolean
in_target (int type, Int32 x, Int32 y, Int32 z)
{
  UInt16 size;

  if (z < 0) return 0;

  size = 128*ship_list[type]->size;

#if 0
  if ((x * x + y * y) <= size)
    DEBUG_PRINTF("in target\n");
#endif

  return ((x * x + y * y) <= size);
}



static void
make_angry (int un)
{
  int type;
  int flags;

  type = universe[un].type;
  flags = universe[un].flags;

  if (flags & FLG_INACTIVE)
    return;

  if ((type == SHIP_CORIOLIS) || (type == SHIP_DODEC)) {
    universe[un].flags |= FLG_ANGRY;
    return;
  }

  if (type > SHIP_ROCK) {
    universe[un].rotx = 4;
    universe[un].acceleration = 2;
    universe[un].flags |= FLG_ANGRY;
  }
}


void
explode_object (int un)
{
  cmdr.score++;

  if ((cmdr.score & 255) == 0)
    info_message ("Right On Commander!");

  snd_play_sample (SND_EXPLODE);
  universe[un].flags |= FLG_DEAD;

  if (universe[un].type == SHIP_CONSTRICTOR)
    cmdr.mission = 2;
}

static void 
reduce_energy(struct univ_object *univ, UInt16 damage) {
  char str[32];
    
  if(univ->energy >= damage) univ->energy -= damage;
  else                       univ->energy  = 0;

  /* only display this for ships with at least some energy */
  if(ship_list[univ->type]->energy > 10) {
    str[0]='*'; 
    str[1] = univ->energy/2;
    str[2] = (ship_list[univ->type]->energy)/2;
    StrCopy(str+3, ship_list[univ->type]->name);
  } else
    StrCopy(str, ship_list[univ->type]->name);

  info_message(str);
}

void
check_target (int un, struct univ_object *flip) {
  struct univ_object *univ;
  char str[32];

  univ = &universe[un];

  if (in_target
      (univ->type, flip->location.x, flip->location.y, flip->location.z)) {

    if ((missile_target == MISSILE_ARMED) && (univ->type >= 0)) {
      missile_target = un;

      StrCopy(str, "Locked on ");
      StrCat(&str[StrLen(str)], ship_list[univ->type]->name);
      info_message (str);

      snd_play_sample (SND_BEEP);
    }

    if (laser) {
      snd_play_sample (SND_HIT_ENEMY);

      /* coriolis, dodoc, constrictor and cougar have special hulls */
      if ((univ->type != SHIP_CORIOLIS) && (univ->type != SHIP_DODEC)) {
	if ((univ->type == SHIP_CONSTRICTOR) || (univ->type == SHIP_COUGAR)) {
          if (laser == (MILITARY_LASER & 127))
	    reduce_energy(univ, laser/4);
	} else
	  reduce_energy(univ, laser);
      }

      if (univ->energy == 0) {
	explode_object (un);

	if (univ->type == SHIP_ASTEROID) {
	  if (laser == (MINING_LASER & 127))
	    launch_loot (un, SHIP_ROCK);
	}
	else {
	  launch_loot (un, SHIP_ALLOY);
	  launch_loot (un, SHIP_CARGO);
	}
      }

      make_angry (un);
    }
  }
}



void
activate_ecm (int ours)
{
  if (ecm_active == 0) {
    if(ours) info_message("E.C.M. activated");

    ecm_active = 32;
    ecm_ours = ours;
    snd_play_sample(SND_ECM);
  }
}


void
time_ecm (void)
{
  if (ecm_active != 0) {
    ecm_active--;
    if (ecm_ours)
      decrease_energy(1);
  }
}


void
arm_missile (void)
{
  if ((cmdr.missiles != 0) && (missile_target == MISSILE_UNARMED)) {
    info_message("Missile armed");
    missile_target = MISSILE_ARMED;
  }
}


void
unarm_missile (void)
{
  if(missile_target > MISSILE_UNARMED)
    info_message("Missile unarmed");
  
  missile_target = MISSILE_UNARMED;
  snd_play_sample (SND_BOOP);
}

void
fire_missile (void)
{
  int newship;
  struct univ_object *ns;
  RotMatrix rotmat;

  if (missile_target < 0)
    return;

  set_init_matrix (rotmat);

  // make missile face forward
  rotmat[2].z =  ROTDIV;
  rotmat[1].y = -ROTDIV;

  // start a missile from underneath the ship
  newship = add_new_ship (SHIP_MISSILE, 0, -28, 14, rotmat, 0, 0);

  if (newship == -1) {
    info_message ("Missile Jammed");
    return;
  }

  ns = &universe[newship];

  /* missile has twice the ship speed */
  ns->velocity = FLIGHT_DESCALE(flight_speed) * 2;
  ns->flags = FLG_ANGRY;
  ns->target = missile_target;

  /* whatever ship is targetted gets angry */
  if (universe[missile_target].type > SHIP_ROCK)
    universe[missile_target].flags |= FLG_ANGRY;

  /* we have one missile less installed */
  cmdr.missiles--;
  missile_target = MISSILE_UNARMED;

  snd_play_sample (SND_MISSILE);
}


static void
track_object (struct univ_object *ship, Int32 direction, RotVector nvec)
{
  Int16 dir;

  #define RAT  3
  #define RAT2 ANGLE(0.05555)

//  DEBUG_PRINTF("ship@%p, dir = %ld\n", ship, (long)direction);

//  DEBUG_PRINTF("direction of %d %ld (limit=%ld)\n", un, 
//	       (long)direction,(long)ANGLE(-0.833));

//  DEBUG_PRINTF("track object, dist = %ld\n", (long)ship->distance);

//  dbg_print_rot_vec("nvec", &nvec);
//  dbg_print_rot_mat("rotmat", ship->rotmat);

  /* nvec = direction vector to enemy */
  dir = vector_dot_product_rot_rot (&nvec, &ship->rotmat[1]);
//  DEBUG_PRINTF("y dir = %ld\n", (long)dir);

  /* fast turn if complete wrong direction */
  if (direction <= ANGLE(-0.861)) {
    ship->rotx = (dir < 0) ? 7 : -7;
    ship->rotz = 0;
    return;
  }

  ship->rotx = 0;

  if (abs(dir) >= RAT2)
    ship->rotx = (dir < 0) ? RAT : -RAT;

  if (abs (ship->rotz) < 16) {
    dir = vector_dot_product_rot_rot (&nvec, &ship->rotmat[0]);

    ship->rotz = 0;

    if (abs(dir) >= RAT2) {
      ship->rotz = (dir < 0) ? RAT : -RAT;

      if (ship->rotx < 0)
	ship->rotz = -ship->rotz;
    }
  }
}

static void
missile_tactics (int un)
{
  struct univ_object *missile;
  struct univ_object *target;
  PosVector vec;
  RotVector nvec;
  Int16 direction;
  Int16 cnt2 = ANGLE(0.223);

  missile = &universe[un];

  if (ecm_active) {
    snd_play_sample (SND_EXPLODE);
    missile->flags |= FLG_DEAD;
    return;
  }

  if (missile->target == 0) {
    if (missile->distance < 256) {
      missile->flags |= FLG_DEAD;
      snd_play_sample (SND_EXPLODE);
      damage_ship (250, missile->location.z >= 0);
      return;
    }

    vec.x = missile->location.x;
    vec.y = missile->location.y;
    vec.z = missile->location.z;
  }
  else {
    target = &universe[missile->target];

    vec.x = missile->location.x - target->location.x;
    vec.y = missile->location.y - target->location.y;
    vec.z = missile->location.z - target->location.z;

    if ((abs(vec.x) < 256) && (abs(vec.y) < 256) && (abs(vec.z) < 256)) {
      missile->flags |= FLG_DEAD;

      if ((target->type != SHIP_CORIOLIS) && (target->type != SHIP_DODEC))
	explode_object (missile->target);
      else
	snd_play_sample (SND_EXPLODE);

      return;
    }

    if ((rand255 () < 16) && (target->flags & FLG_HAS_ECM)) {
      activate_ecm (0);
      return;
    }
  }

  nvec = unit_pos_vector (&vec);
  direction = vector_dot_product_rot_rot (&nvec, &missile->rotmat[2]);
  nvec.x = -nvec.x;
  nvec.y = -nvec.y;
  nvec.z = -nvec.z;
  direction = -direction;

  track_object (missile, direction, nvec);

  if (direction <= ANGLE(-0.167)) {
    missile->acceleration = -2;
    return;
  }

  if (direction >= cnt2) {
    missile->acceleration = 3;
    return;
  }

  if (missile->velocity < 6)
    missile->acceleration = 3;
  else if (rand255 () >= 200)
    missile->acceleration = -2;
  return;
}


static void
launch_shuttle (void)
{
  int type;

  if ((ship_count[SHIP_TRANSPORTER] != 0) ||
      (ship_count[SHIP_SHUTTLE] != 0) || (rand255 () < 253) || (auto_pilot))
    return;

  type = rand255 () & 1 ? SHIP_SHUTTLE : SHIP_TRANSPORTER;
  launch_enemy (1, type, FLG_HAS_ECM | FLG_FLY_TO_PLANET, 113);
}


void
tactics (int un)
{
  int type;
  int energy;
  int maxeng;
  int flags;
  struct univ_object *ship;
  RotVector nvec;
  Int16 cnt2 = ANGLE(0.223);
  Int16 direction;
  Bool attacking;

  ship = &universe[un];
  type = ship->type;
  flags = ship->flags;

  if ((type == SHIP_PLANET) || (type == SHIP_SUN))
    return;

  if (flags & FLG_DEAD)
    return;

  if (flags & FLG_INACTIVE)
    return;

  if (type == SHIP_MISSILE) {
    if (flags & FLG_ANGRY)
      missile_tactics (un);
    return;
  }

  if (((un ^ mcount) & 7) != 0)
    return;

  /* a angry space station launches vipers */
  if ((type == SHIP_CORIOLIS) || (type == SHIP_DODEC)) {

    if (flags & FLG_ANGRY) {
      if (rand255 () < 240)  // 6.25%
	return;

      /* make sure not to launch vipers, while we are */
      /* near entry of docking bay */
      nvec = unit_pos_vector(&ship->location);
      direction = vector_dot_product_rot_rot (&nvec, &ship->rotmat[2]) + ANGLE(1); 

      if((abs(direction) <= ANGLE(0.5)) && ship->distance < 2000) {
        /* don't launch more than 1 missile */
        if (ship_count[SHIP_MISSILE] < 1) {
  	  launch_enemy (un, SHIP_MISSILE, FLG_ANGRY, 126);
	  info_message ("INCOMING MISSILE");
	}
	return;
      } else {
        /* don't launch more than 4 vipers */
        if (ship_count[SHIP_VIPER] < 4) {
	  /* this is the position of the station */
	  launch_enemy (un, SHIP_VIPER, FLG_ANGRY | FLG_HAS_ECM, 113);
	}
	return;
      }
    }

    nvec = unit_pos_vector(&ship->location);
    direction = vector_dot_product_rot_rot(&nvec, &ship->rotmat[2]) + ANGLE(1);
    if((abs(direction) <= ANGLE(0.5)) && ship->distance < 2000)
      return;

    launch_shuttle ();
    return;
  }

  /* a hermit launches sidewinders */
  if (type == SHIP_HERMIT) {
    if (rand255 () > 200) {
      launch_enemy (un, SHIP_SIDEWINDER + (rand255 () & 3),
		    FLG_ANGRY | FLG_HAS_ECM, 113);
      ship->flags |= FLG_INACTIVE;
    }

    return;
  }


  if (ship->energy < ship_list[type]->energy)
    ship->energy++;

  /* targlets get dumb without mother ship */
  if ((type == SHIP_THARGLET) && (ship_count[SHIP_THARGOID] == 0)) {
    ship->flags = 0;
    ship->velocity /= 2;
    return;
  }

  if (flags & FLG_SLOW) {
    if (rand255 () > 50)
      return;
  }

  /* police ships get angry on offenders */
  if (flags & FLG_POLICE) {
    if (cmdr.legal_status >= 64) {
      flags |= FLG_ANGRY;
      ship->flags = flags;
    }
  }

  /* not-angry ships */
  if ((flags & FLG_ANGRY) == 0) {
    if ((flags & FLG_FLY_TO_PLANET) || (flags & FLG_FLY_TO_STATION)) {
      auto_pilot_ship (&universe[un]);
    }

    return;
  }


  /* If we get to here then the ship is angry so start attacking... */

  if (ship_count[SHIP_CORIOLIS] || ship_count[SHIP_DODEC]) {
    if ((flags & FLG_BOLD) == 0)
      ship->bravery = 0;
  }


  if (type == SHIP_ANACONDA) {
    if (rand255 () > 200) {
      launch_enemy (un, rand255 () > 100 ? SHIP_WORM : SHIP_SIDEWINDER,
		    FLG_ANGRY | FLG_HAS_ECM, 113);
      return;
    }
  }


  if (rand255 () >= 250) {
    ship->rotz = rand255 () | 0x68;
    if (ship->rotz > 127)
      ship->rotz = -(ship->rotz & 127);
  }

  maxeng = ship_list[type]->energy;
  energy = ship->energy;

  if (energy < (maxeng / 2)) {
    if ((energy < (maxeng / 8)) && (rand255 () > 230)
	&& (type != SHIP_THARGOID)) {
      ship->flags &= ~FLG_ANGRY;
      ship->flags |= FLG_INACTIVE;
      launch_enemy (un, SHIP_ESCAPE_CAPSULE, 0, 126);
      return;
    }

    if ((ship->missiles != 0) && (ecm_active == 0) &&
	(ship->missiles >= (rand255 () & 31))) {
      ship->missiles--;
      if (type == SHIP_THARGOID)
	launch_enemy (un, SHIP_THARGLET, FLG_ANGRY, ship->bravery);
      else {
	launch_enemy (un, SHIP_MISSILE, FLG_ANGRY, 126);
	info_message ("INCOMING MISSILE");
      }
      return;
    }
  }

  /* make the enemy ship fire on me */
  nvec = unit_pos_vector (&universe[un].location);
  direction = vector_dot_product_rot_rot (&nvec, &ship->rotmat[2]);

  if ((ship->distance < 8192) && (direction <= ANGLE(-0.833)) &&
      (ship_list[type]->laser_strength != 0)) {
    if (direction <= ANGLE(-0.917))
      ship->flags |= FLG_FIRING | FLG_HOSTILE;

    if (direction <= ANGLE(-0.972)) {
      damage_ship (ship_list[type]->laser_strength, ship->location.z >= 0.0);
      ship->acceleration--;
      if (((ship->location.z >= 0) && (front_shield == 0)) ||
	  ((ship->location.z < 0) && (aft_shield == 0)))
	snd_play_sample (SND_INCOMMING_FIRE_2);
      else
	snd_play_sample (SND_INCOMMING_FIRE_1);
    } else {
      nvec.x = -nvec.x;
      nvec.y = -nvec.y;
      nvec.z = -nvec.z;
      direction = -direction;
      track_object (&universe[un], direction, nvec);
    }

//  if ((fabs(ship->location.z) < 768) && 
//    (ship->bravery <= ((rand255() & 127) | 64)))
    if (abs(ship->location.z) < 768) {
      ship->rotx = rand255 () & 0x87;
      if (ship->rotx > 127)
	ship->rotx = -(ship->rotx & 127);

      ship->acceleration = 3;
      return;
    }

    if (ship->distance < 8192)
      ship->acceleration = -1;
    else
      ship->acceleration = 3;
    return;
  }

  attacking = 0;

  if ((abs(ship->location.z) >= 768) ||
      (abs(ship->location.x) >= 512) || 
      (abs(ship->location.y) >= 512)) {
    if (ship->bravery > (rand255 () & 127)) {
      attacking = 1;
      nvec.x = -nvec.x;
      nvec.y = -nvec.y;
      nvec.z = -nvec.z;
      direction = -direction;
    }
  }

  track_object (&universe[un], direction, nvec);

  if ((attacking == 1) && (ship->distance < 2048)) {
    if (direction >= cnt2) {
      ship->acceleration = -1;
      return;
    }

    if (ship->velocity < 6)
      ship->acceleration = 3;
    else if (rand255 () >= 200)
      ship->acceleration = -1;
    return;
  }

  if (direction <= ANGLE(-0.167)) {
    ship->acceleration = -1;
    return;
  }

  if (direction >= cnt2) {
    ship->acceleration = 3;
    return;
  }

  if (ship->velocity < 6)
    ship->acceleration = 3;
  else if (rand255 () >= 200)
    ship->acceleration = -1;
}


void
draw_laser_lines (void)
{
  struct point2d points[5];  
  const UInt8 laser_l_list[]={0,1,2};
  const UInt8 laser_r_list[]={3,1,4};

  SET_POINT(points[0], GALAXY_SCALE(32),  DSP_HEIGHT);
  SET_POINT(points[1], laser_x, laser_y);
  SET_POINT(points[2], GALAXY_SCALE(48),  DSP_HEIGHT);
  SET_POINT(points[3], GALAXY_SCALE(208), DSP_HEIGHT);
  SET_POINT(points[4], GALAXY_SCALE(224), DSP_HEIGHT);

  draw_polygon(points, (char*)laser_l_list, 3, TD_COL_RED);
  draw_polygon(points, (char*)laser_r_list, 3, TD_COL_RED);
}


int
fire_laser (void)
{
  if ((laser_counter == 0) && (laser_temp < 242)) {
    switch (current_screen) {
    case SCR_FRONT_VIEW:
      laser = cmdr.front_laser;
      break;

    case SCR_REAR_VIEW:
      laser = cmdr.rear_laser;
      break;

    case SCR_RIGHT_VIEW:
      laser = cmdr.right_laser;
      break;

    case SCR_LEFT_VIEW:
      laser = cmdr.left_laser;
      break;

    default:
      laser = 0;
    }

    if (laser != 0) {
      laser_counter = (laser > 127) ? 0 : (laser & 0xFA);
      laser &= 127;
      laser2 = laser;

      snd_play_sample (SND_PULSE);
      laser_temp += 8;
      if (energy > 1)
	energy--;

      laser_x = (rand255 () & 3) + DSP_WIDTH/2 - 2;
      laser_y = (rand255 () & 3) + DSP_HEIGHT/2 - 2;

      return 2;
    }
  }

  return 0;
}


void
cool_laser (void)
{
  laser = 0;

  if (laser_temp > 0)
    laser_temp--;

  if (laser_counter > 0)
    laser_counter--;

  if (laser_counter > 0)
    laser_counter--;
}


static int
create_other_ship (int type)
{
  RotMatrix rotmat;
  int x, y, z;
  int newship;

  set_init_matrix (rotmat);

  /* ships are created far, far away */
  z = 12000;
  x = 1000 + (randint () & 8191);
  y = 1000 + (randint () & 8191);

  if (rand255 () > 127)  x = -x;
  if (rand255 () > 127)  y = -y;

  newship = add_new_ship (type, x, y, z, rotmat, 0, 0);

  return newship;
}


void
create_thargoid (void)
{
  int newship;

  newship = create_other_ship (SHIP_THARGOID);
  if (newship != -1) {
    universe[newship].flags = FLG_ANGRY | FLG_HAS_ECM;
    universe[newship].bravery = 113;

    if (rand255 () > 64)
      launch_enemy (newship, SHIP_THARGLET, FLG_ANGRY | FLG_HAS_ECM, 96);
  }
}



static void
create_cougar (void)
{
  int newship;

  if (ship_count[SHIP_COUGAR] != 0)
    return;

  newship = create_other_ship (SHIP_COUGAR);
  if (newship != -1) {
    universe[newship].flags = FLG_HAS_ECM;	// | FLG_CLOAKED;
    universe[newship].bravery = 121;
    universe[newship].velocity = 18;
  }
}



static void
create_trader (void)
{
  int newship;
  int rnd;
  int type;

  /* trader = cobra, python, boa, anaconda */
  type = SHIP_COBRA3 + (rand255 () & 3);

  newship = create_other_ship (type);

  if (newship != -1) {
    universe[newship].rotmat[2].z = -1.0;
    universe[newship].rotz = rand255 () & 7;

    rnd = rand255 ();
    universe[newship].velocity = (rnd & 31) | 16;
    universe[newship].bravery = rnd / 2;

    /* half of all traders have ecm */
    if (rnd & 1)
      universe[newship].flags |= FLG_HAS_ECM;

/* half of all traders are angry */
//  if (rnd & 2)
//    universe[newship].flags |= FLG_ANGRY; 
  }
}


static void
create_lone_hunter (void)
{
  int rnd;
  int type;
  int newship;

  if ((cmdr.mission == 1) && (cmdr.galaxy_number == 1) &&
      (docked_planet.d == 144) && (docked_planet.b == 33) &&
      (ship_count[SHIP_CONSTRICTOR] == 0)) {
    type = SHIP_CONSTRICTOR;
  }
  else {
    rnd = rand255 ();
    type = SHIP_COBRA3_LONE + (rnd & 3) + (rnd > 127);
  }

  newship = create_other_ship (type);

  if (newship != -1) {
    universe[newship].flags = FLG_ANGRY;
    if ((rand255 () > 200) || (type == SHIP_CONSTRICTOR))
      universe[newship].flags |= FLG_HAS_ECM;

    universe[newship].bravery = ((rand255 () * 2) | 64) & 127;
    in_battle = 1;
  }
}



/* Check for a random asteroid encounter... */

static void
check_for_asteroids (void)
{
  int newship;
  int type;

  if ((rand255 () >= 35) || (ship_count[SHIP_ASTEROID] >= 3))
    return;

  if (rand255 () > 253)
    type = SHIP_HERMIT;
  else
    type = SHIP_ASTEROID;

  newship = create_other_ship (type);

  if (newship != -1) {
//              universe[newship].velocity = (rand255() & 31) | 16; 
    universe[newship].velocity = 8;
    universe[newship].rotz = rand255 () > 127 ? -127 : 127;
    universe[newship].rotx = 16;
  }
}



/* If we've been a bad boy then send the cops after us... */

static void
check_for_cops (void)
{
  int newship;
  int offense;

  offense = carrying_contraband () * 2;
  if (ship_count[SHIP_VIPER] == 0)
    offense |= cmdr.legal_status;

  if (rand255 () >= offense)
    return;

  newship = create_other_ship (SHIP_VIPER);

  if (newship != -1) {
    universe[newship].flags = FLG_ANGRY;
    if (rand255 () > 245)
      universe[newship].flags |= FLG_HAS_ECM;

    universe[newship].bravery = ((rand255 () * 2) | 64) & 127;
  }
}


static void
check_for_others (void)
{
  int x, y, z;
  int newship;
  RotMatrix rotmat;
  int gov;
  int rnd;
  int type;
  int i;

  gov = current_planet_data.government;
  rnd = rand255 ();

  if ((gov != 0) && ((rnd >= 90) || ((rnd & 7) < gov)))
    return;

  if (rand255 () < 100) {
    create_lone_hunter ();
    return;
  }

  /* Pack hunters... */

  set_init_matrix (rotmat);

  z = 12000;
  x = 1000 + (randint () & 8191);
  y = 1000 + (randint () & 8191);

  if (rand255 () > 127)
    x = -x;
  if (rand255 () > 127)
    y = -y;

  rnd = rand255 () & 3;

  for (i = 0; i <= rnd; i++) {
    type = SHIP_SIDEWINDER + (rand255 () & rand255 () & 7);
    newship = add_new_ship (type, x, y, z, rotmat, 0, 0);
    if (newship != -1) {
      universe[newship].flags = FLG_ANGRY;
      if (rand255 () > 245)
	universe[newship].flags |= FLG_HAS_ECM;

      universe[newship].bravery = ((rand255 () * 2) | 64) & 127;
      in_battle++;
    }
  }

}


void
random_encounter (void)
{
  if ((ship_count[SHIP_CORIOLIS] != 0) || (ship_count[SHIP_DODEC] != 0))
    return;

  if (rand255 () == 136) {
    if (((int) (universe[0].location.z) & 0x3e) != 0)
      create_thargoid ();
    else
      create_cougar ();

    return;
  }

  if ((rand255 () & 7) == 0) {
    create_trader ();
    return;
  }

  check_for_asteroids ();

  check_for_cops ();

  if (ship_count[SHIP_VIPER] != 0)
    return;

  if (in_battle)
    return;

  if ((cmdr.mission == 5) && (rand255 () >= 200))
    create_thargoid ();

  check_for_others ();
}


void
abandon_ship (void)
{
  int i;

  cmdr.escape_pod = 0;
  cmdr.legal_status = 0;
  cmdr.fuel = MAX_FUEL;

  for (i = 0; i < NO_OF_STOCK_ITEMS; i++)
    cmdr.current_cargo[i] = 0;

  snd_play_sample (SND_DOCK);
  dock_player();
}
