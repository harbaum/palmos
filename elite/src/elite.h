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

#ifndef ELITE_H
#define ELITE_H

#include <PalmOS.h>

#define  Bool  Boolean

#include "planet.h"
#include "trade.h"
#include "sections.h"
#include "dialog.h"

#define SCR_FLAG_3D             (0x80)  // this is a 3d view
#define SCR_FLAG_NI             (0x40)  // this is a non-interactive view

#define SCR_INTRO_DANUBE        (0 | SCR_FLAG_NI)
#define SCR_INTRO_ONE		(1 | SCR_FLAG_3D | SCR_FLAG_NI)
#define SCR_INTRO_TWO		(2 | SCR_FLAG_3D | SCR_FLAG_NI)
#define SCR_GALACTIC_CHART	(3)
#define SCR_SHORT_RANGE		(4)
#define	SCR_PLANET_DATA		(5)
#define SCR_MARKET_PRICES	(6)
#define SCR_CMDR_STATUS		(7)
#define SCR_FRONT_VIEW		(8 | SCR_FLAG_3D)
#define SCR_REAR_VIEW		(9 | SCR_FLAG_3D)
#define SCR_LEFT_VIEW		(10 | SCR_FLAG_3D)
#define SCR_RIGHT_VIEW		(11 | SCR_FLAG_3D)
#define SCR_INVENTORY		(12)
#define SCR_EQUIP_SHIP		(13)
#define SCR_OPTIONS		(14)
#define SCR_GAME_OVER		(15 | SCR_FLAG_NI)
#define SCR_ESCAPE_POD		(16 | SCR_FLAG_3D | SCR_FLAG_NI)
#define SCR_LAUNCH_SEQUENCE     (17 | SCR_FLAG_3D | SCR_FLAG_NI)
#define SCR_DOCKING_SEQUENCE    (18 | SCR_FLAG_3D | SCR_FLAG_NI)
#define SCR_HYPERSPACE          (19 | SCR_FLAG_3D | SCR_FLAG_NI)
#define SCR_GALACTIC_HYPERSPACE (20 | SCR_FLAG_3D | SCR_FLAG_NI)
#define SCR_SBAY                (21)

#define PULSE_LASER		0x0F
#define BEAM_LASER		0x8F
#define MILITARY_LASER	        0x97
#define MINING_LASER	        0x32

#define MESSAGE_Y               (10)

#define FLG_DEAD		(1)
#define	FLG_REMOVE		(2)
#define FLG_EXPLOSION		(4)
#define FLG_ANGRY		(8)
#define FLG_FIRING		(16)
#define FLG_HAS_ECM		(32)
#define FLG_HOSTILE		(64)
#define FLG_CLOAKED		(128)
#define FLG_FLY_TO_PLANET	(256)
#define FLG_FLY_TO_STATION	(512)
#define FLG_INACTIVE		(1024)
#define FLG_SLOW		(2048)
#define FLG_BOLD		(4096)
#define FLG_POLICE		(8192)


#define MAX_UNIV_OBJECTS	20


struct commander {
  Char name[DLG_MAX_LEN+1];
  UInt32 timer;
  UInt8 mission;
  Int16 ship_x;
  Int16 ship_y;
  struct galaxy_seed galaxy;
  UInt32 credits;
  UInt16 fuel;
  UInt8 galaxy_number;
  UInt8 front_laser;
  UInt8 rear_laser;
  UInt8 left_laser;
  UInt8 right_laser;
  int cargo_capacity;
  int current_cargo[NO_OF_STOCK_ITEMS];
  Boolean ecm;
  Boolean fuel_scoop;
  Boolean energy_bomb;
  Boolean energy_unit;
  Boolean docking_computer;
  Boolean galactic_hyperdrive;
  Boolean escape_pod;
  UInt8 missiles;
  UInt8 legal_status;
  int station_stock[NO_OF_STOCK_ITEMS];
  Int16 market_rnd;
  UInt16 score;
  Boolean saved;
};

#define FLIGHT_SCALE(a)   ((a)*256)
#define FLIGHT_DESCALE(a) ((a)/256)

#define FLIGHT_DESCALE_TICKS(a) ((ticks*(Int32)a)/(256*50))

/* constants removed from player_ship */
#define MAX_CLIMB  (FLIGHT_SCALE(8))	 /* CF 8 */
#define MAX_ROLL   (FLIGHT_SCALE(31))
#define MAX_SPEED  (FLIGHT_SCALE(40))	 /* 0.27 Light Mach */
#define MAX_FUEL   (70)                  /* 7.0 Light Years */

struct player_ship {
  UInt8 altitude;
  UInt8 cabtemp;
  UInt8 dock_state;
};

extern UInt32 game_timer;

extern struct player_ship myship;

extern struct commander cmdr;

extern struct galaxy_seed docked_planet;

extern struct galaxy_seed hyperspace_planet;

extern struct planet_data current_planet_data;

extern Boolean carry_flag;
extern UInt8 current_screen;

extern struct ship_data *ship_list[];

extern UInt8 message_count;
extern char message_string[32];

extern Boolean game_over;
extern Boolean is_docked;
extern Boolean finish;

extern Int16 flight_speed;
extern Int16 flight_roll;
extern Int16 flight_climb;

extern UInt8 front_shield;
extern UInt8 aft_shield;
extern UInt8 energy;
extern UInt8 laser_temp;

extern UInt8 mcount;
extern Boolean detonate_bomb;
extern Boolean witchspace;
extern Boolean auto_pilot;

extern Boolean color_gfx;

extern Bool rolling;
extern Bool climbing;

void restore_jameson_data(void) SECDK;
void restore_jameson(void) SECDK;
void restore_saved_commander(void) SECDK;
void init_ship_data(void) SECDK;
void free_ship_data(void) SECDK;

#endif
