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
 * pilot.c
 *
 * The auto-pilot code.  Used for docking computers and for
 * flying other ships to and from the space station.
 */

#include <PalmOS.h>

#include "config.h"
#include "gfx.h"
#include "elite.h"
#include "vector.h"
#include "main.h"
#include "space.h"
#include "sound.h"
#include "pilot.h"
#include "debug.h"

static Bool SEC3D fly_to_planet (struct univ_object *ship);
static void SEC3D fly_to_station (struct univ_object *ship);
static Bool SEC3D fly_to_station_front (struct univ_object *ship, Int32 d);
static void SEC3D fly_to_docking_bay (struct univ_object *ship);

/*
 * Fly to a given point in space.
 */

Bool
fly_to_vector (struct univ_object *ship, PosVector vec)
{
  RotVector nvec;
  Int16 direction;
  Int16 dir;
  Int32 dist;

  #define RAT       3

  /* if angle < rat2 -> correct, original value = 0.08333 = 7.5 deg */
  #define RAT2      ANGLE(0.01)   /* 0.9 deg */

  nvec = unit_pos_vector(&vec);

  /* angle front/rear */
  direction = vector_dot_product_rot_rot (&nvec, &ship->rotmat[2]); 
	
  /* destination is far behind us */
//  if (direction < ANGLE(-0.6666)) rat2 = 0;

  /* get vertical offset */
  dir = vector_dot_product_rot_rot (&nvec, &ship->rotmat[1]);

  /* dest is very far behind us, so climb/dive very fast according to */
  /* vertical offset */
  if (direction < ANGLE(-0.861)) {
    ship->acceleration = -3;
    ship->rotx = (dir < 0) ? 7 : -7;
    ship->rotz = 0;
  } else {

    /* do not dive/climb */
    ship->rotx = 0;
    
    /* dest is too low/high -> dive/climb */
    if (abs(dir) >= RAT2)
      ship->rotx = (dir < 0) ? RAT : -RAT;
    
    /* ship isn't already rolling very fast */
    if (abs(ship->rotz) < 16) {
      /* horizontal offset */
      dir = vector_dot_product_rot_rot (&nvec, &ship->rotmat[0]);
      
      /* do not roll */
      ship->rotz = 0;
      
      /* dest is to far left/right */
      if (abs(dir) >= RAT2) {
	/* roll! */
	ship->rotz = (dir < 0) ? RAT : -RAT;
	
	/* if we are already diving, roll the opposite direction */
	if (ship->rotx < 0)
	  ship->rotz = -ship->rotz;
      }		
    }
    
    /* destination is at least slightly behind -> break */
    if (direction <= ANGLE(-0.167))
      ship->acceleration = -1;
    /* destination is quite exactly in front -> accelerate */
    else if (direction >= ANGLE(0.8055))
      ship->acceleration = 3;
  }

  /* get proximity to dest vector */
  dist = get_distance(vec.x, vec.y, vec.z);

  /* don't pass meeting point with full speed */
  if((dist < 250)&&(ship->velocity>4))
      ship->acceleration = -1;

  /* check if we have reached the destination vector */
  if(dist < 50) return 1;
  
  return 0;
}



/*
 * Fly towards the planet.
 */

static Bool
fly_to_planet (struct univ_object *ship)
{
  PosVector vec;

  vec.x = universe[0].location.x - ship->location.x;
  vec.y = universe[0].location.y - ship->location.y;
  vec.z = universe[0].location.z - ship->location.z;

  if(get_distance(vec.x, vec.y, vec.z)>10000)
    ship->velocity = MAX_SPEED;

  return fly_to_vector (ship, vec);
}

/*
 * Fly towards the space station.
 */

static void
fly_to_station (struct univ_object *ship)
{
  PosVector vec;
  
  vec.x = universe[1].location.x - ship->location.x;
  vec.y = universe[1].location.y - ship->location.y;
  vec.z = universe[1].location.z - ship->location.z;
  
  fly_to_vector (ship, vec);	
}

/*
 * Fly to a point in front of the station docking bay.
 * Done prior to the final stage of docking.
 */

#define DIST_FRONT0 4096l
#define DIST_FRONT1 0l

Bool
fly_to_station_front (struct univ_object *ship, Int32 offset)
{
  PosVector vec;

  vec.x = universe[1].location.x - ship->location.x;
  vec.y = universe[1].location.y - ship->location.y;
  vec.z = universe[1].location.z - ship->location.z;

  vec.x += ((Int32)universe[1].rotmat[2].x * offset)>>ROTBITS;
  vec.y += ((Int32)universe[1].rotmat[2].y * offset)>>ROTBITS;
  vec.z += ((Int32)universe[1].rotmat[2].z * offset)>>ROTBITS;

  return fly_to_vector (ship, vec);
}

/*
 * Final stage of docking.
 * Fly into the docking bay.
 */

static void
fly_to_docking_bay (struct univ_object *ship)
{
  PosVector diff;
  RotVector vec;
  Int16 dir;

//  DEBUG_PRINTF("fly to docking bay\n");

  diff.x = ship->location.x - universe[1].location.x;
  diff.y = ship->location.y - universe[1].location.y;
  diff.z = ship->location.z - universe[1].location.z;

  vec = unit_pos_vector (&diff);	

  ship->rotx = 0;

  /* it's our own ship */
  if (ship->type < 0) {
    ship->rotz = 1;
    if (((vec.x >= 0) && (vec.y >= 0)) ||
	((vec.x < 0) && (vec.y < 0))) {
      ship->rotz = -ship->rotz;
    }

    /* if we are too far left/right */
    if (abs(vec.x) >= ANGLE(0.0625)) {
      ship->acceleration = 0;
      ship->velocity = 1;
      return;
    }

    if (abs(vec.y) > ANGLE(0.002436))
      ship->rotx = (vec.y < 0) ? -1 : 1;

    if (abs(vec.y) >= ANGLE(0.0625)) {
      ship->acceleration = 0;
      ship->velocity = 1;
      return;
    }
  }
  
  ship->rotz = 0;
  
  dir = vector_dot_product_rot_rot (&ship->rotmat[0], &universe[1].rotmat[1]);
  
  if (abs(dir) >= ANGLE(0.98)) {
    ship->acceleration++;
    ship->rotz = 127;
    return;
  }
  
  ship->acceleration = 0;
  ship->rotz = 0;
}


/*
 * Fly a ship to the planet or to the space station and dock it.
 */

void
auto_pilot_ship (struct univ_object *ship)
{
//  DEBUG_PRINTF("docking state: %d\n", ship->dock_state);

  if(ship->dock_state != DOCK_PLANET) {
    if(get_distance(ship->location.x - universe[1].location.x,
		    ship->location.y - universe[1].location.y,
		    ship->location.z - universe[1].location.z) < 160) {
      ship->flags |= FLG_REMOVE;	// Ship has docked.
      return;
    }
  }

  switch(ship->dock_state) {

    /* first approach the planet */
    case DOCK_PLANET:
      if ((ship->flags & FLG_FLY_TO_PLANET) ||
	  ((ship_count[SHIP_CORIOLIS] == 0) && 
	   (ship_count[SHIP_DODEC] == 0))) {
	fly_to_planet (ship);
      } else if (ship->flags & FLG_FLY_TO_STATION) {
	fly_to_station (ship);
      } else
	ship->dock_state = DOCK_FRONT0;
      break;

      /* then approach the front of the station */
    case DOCK_FRONT0:
      if(fly_to_station_front (ship, DIST_FRONT0))
	ship->dock_state = DOCK_FRONT1;
      break;

      /* then approach the front of the station */
    case DOCK_FRONT1:
      fly_to_station_front (ship, DIST_FRONT1);

      /* switch to bay dock mode once close enough */
      if(universe[1].distance < 1536)
	ship->dock_state = DOCK_BAY;
      break;

    case DOCK_BAY:
      fly_to_docking_bay (ship);
  }
}


void
engage_auto_pilot (void)
{
  if (auto_pilot || witchspace || hyper_ready)
    return;

  auto_pilot = 1;
  myship.dock_state = DOCK_PLANET;
}


void
disengage_auto_pilot (void) {
  auto_pilot = 0;
}

void
auto_dock (void)
{
  struct univ_object ship;

  /* create a ship object for our own ship */
  ship.location.x = 0;
  ship.location.y = 0;
  ship.location.z = 0;

  set_init_matrix (ship.rotmat);
  ship.rotmat[2].z =  ROTDIV;
  ship.rotmat[0].x = -ROTDIV;
  ship.type = PLAYER_AUTO_DOCK;
  ship.velocity = FLIGHT_DESCALE(flight_speed);
  ship.acceleration = 0;
  ship.bravery = 0;
  ship.rotz = 0;
  ship.rotx = 0;
  ship.flags = 0;
  ship.dock_state = myship.dock_state;

  /* let computer fly us a little step */
  auto_pilot_ship (&ship);

  myship.dock_state = ship.dock_state;

  /* and transfer controls of ship object to our ship */
  if (ship.velocity > FLIGHT_SCALE(22))
    flight_speed = FLIGHT_SCALE(22);
  else
    flight_speed = FLIGHT_SCALE(ship.velocity);

  /* slow at front2/bay */
  if((myship.dock_state > 1)&&(flight_speed > FLIGHT_SCALE(10)))
    flight_speed = FLIGHT_SCALE(10);

  if (ship.acceleration > 0) {
    flight_speed += FLIGHT_SCALE(1);
    if (flight_speed > FLIGHT_SCALE(22))
      flight_speed = FLIGHT_SCALE(22);
  }

  if (ship.acceleration < 0) {
    flight_speed -= FLIGHT_SCALE(1);
    if (flight_speed < FLIGHT_SCALE(1))
      flight_speed = FLIGHT_SCALE(1);
  }

  /* stop climb/dive if required */
  if (ship.rotx == 0)
    flight_climb = FLIGHT_SCALE(0);

  /* climb slow/fast */
  if (ship.rotx < 0) {
    climbing = 1;
    increase_flight_climb(1);

    if (ship.rotx < -1)
      increase_flight_climb(1);
  }

  /* climb slow/fast */
  if (ship.rotx > 0) {
    climbing = 1;

    decrease_flight_climb(1);

    if (ship.rotx > 1)
      decrease_flight_climb(1);
  }

  /* roll */
  if (ship.rotz == 127) {
    rolling = 1;
    flight_roll = FLIGHT_SCALE(-40);    // roll with station speed
  } else {
    if (ship.rotz == 0)
      flight_roll = FLIGHT_SCALE(0);

    if (ship.rotz > 0) {
      rolling = 1;
      increase_flight_roll(4);

//      if (ship.rotz > 1)
//	increase_flight_roll(2);
    }

    if (ship.rotz < 0) {
      decrease_flight_roll(4);

//      if (ship.rotz < -1)
//	decrease_flight_roll(2);
    }
  }
}

