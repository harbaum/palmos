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
 *
 */

#ifndef SWAT_H
#define SWAT_H

#include "space.h"

#define MISSILE_UNARMED	-2
#define MISSILE_ARMED	-1

extern UInt8 ecm_active;
extern Int8 missile_target;
extern UInt8 in_battle;

void SEC3D reset_weapons (void);
void SEC3D tactics (int un);
Boolean SEC3D in_target (int type, Int32 x, Int32 y, Int32 z);
void SEC3D add_new_station (Int32 sx, Int32 sy, Int32 sz, RotMatrix rotmat);
void SEC3D check_target (int un, struct univ_object *flip);
void SEC3D check_missiles (int un);
void SEC3D draw_laser_lines (void);
int SEC3D fire_laser (void);
void SEC3D cool_laser (void);
void SEC3D arm_missile (void);
void SEC3D unarm_missile (void);
void SEC3D fire_missile (void);
void SEC3D activate_ecm (int ours);
void SEC3D time_ecm (void);
void SEC3D random_encounter (void);
void SEC3D explode_object (int un);
void SEC3D abandon_ship (void);
void SEC3D create_thargoid (void);
void SEC3D dock_it (struct univ_object *ship);



#endif
