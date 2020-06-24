/*
 * pilot.h
 */

#ifndef PILOT_H
#define PILOT_H

#include "sections.h"

/* docking states */
#define DOCK_PLANET  0
#define DOCK_FRONT0  1
#define DOCK_FRONT1  2
#define DOCK_BAY     3

#define PLAYER_AUTO_DOCK -96

#define ANGLE(a)  ((Int16)(ROTDIV*(a)))

Bool SEC3D fly_to_vector (struct univ_object *ship, PosVector vec);
void SEC3D auto_pilot_ship (struct univ_object *ship);
void SEC3D engage_auto_pilot (void);
void SEC3D disengage_auto_pilot (void);
void SEC3D auto_dock (void);

#endif
