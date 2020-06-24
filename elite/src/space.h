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

/*
 * space.h
 */

#ifndef SPACE_H
#define SPACE_H

#include "vector.h"
#include "shipdata.h"

struct point {
  long x;
  long y;
  long z;
};


struct univ_object {
  Int8 type;
  PosVector location;
  RotMatrix rotmat;
  Int16 rotx, rotz;
  int flags;
  int energy;
  int velocity;
  int acceleration;
  Int8 missiles;
  Int8 target;
  UInt8 dock_state;
  int bravery;
  int exp_delta;
  Int32 distance;
};

#define MAX_UNIV_OBJECTS	20

extern struct univ_object universe[MAX_UNIV_OBJECTS];
extern int ship_count[NO_OF_SHIPS + 1];	/* many */



void SEC3D clear_universe (void);
int SEC3D add_new_ship (int ship_type, Int32 x, Int32 y, Int32 z,
			RotVector *rotmat, int rotx, int rotz);
void SEC3D remove_ship (int un);
void SEC3D move_univ_object (struct univ_object *obj);
void SEC3D update_universe (void);

void SEC3D scanner_clear_buffer(void);

Int32 SEC3D get_distance(Int32 x, Int32 y, Int32 z);

void update_console (Bool all);

void update_altitude (void);
void update_cabin_temp (void);
void regenerate_shields (void);

void increase_flight_roll (int n);
void decrease_flight_roll (int n);
void increase_flight_climb (int n);
void decrease_flight_climb (int n);
void dock_player (void);

void damage_ship (int damage, int front);
void decrease_energy (int amount);

extern int hyper_ready;

void start_hyperspace (void);
void start_galactic_hyperspace (void);
void display_hyper_status (void);
void countdown_hyperspace (void);
Boolean jump_warp (void);
void launch_player (void);

void engage_docking_computer (void);

#endif
