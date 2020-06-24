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

#include "config.h"
#include "elite.h"
#include "vector.h"
#include "planet.h"
#include "shipdata.h"
#include "debug.h"
#include "file.h"
#include "options.h"

struct galaxy_seed docked_planet;

struct galaxy_seed hyperspace_planet;

struct planet_data current_planet_data;

UInt32 game_timer;

UInt8 curr_galaxy_num = 1;
UInt8 curr_fuel = 70;
Boolean carry_flag = 0;
UInt8 current_screen = 0;
Boolean witchspace;

Boolean game_over;
Boolean is_docked;
Boolean finish;

Int16 flight_speed;
Int16 flight_roll;
Int16 flight_climb;

UInt8 front_shield;
UInt8 aft_shield;
UInt8 energy;
UInt8 laser_temp;

Boolean detonate_bomb;
Boolean auto_pilot;

struct commander cmdr;

struct player_ship myship;

struct ship_data *ship_list[NO_OF_SHIPS + 1];
static MemHandle ship_list_handle[NO_OF_SHIPS + 1];

void
init_ship_data (void)
{
  int i;

  ship_list[0] = NULL;

  // Keep the Object data locked because they are frequently used
  for (i = 1; i < NO_OF_SHIPS + 1; i++) {
    ship_list_handle[i] = DmGetResource ('ship', i - 1);
    ship_list[i] = (struct ship_data *) MemHandleLock (ship_list_handle[i]);
  }
}

void
free_ship_data (void)
{
  int i;

  for (i = 1; i < NO_OF_SHIPS + 1; i++) {
    MemPtrUnlock (ship_list[i]);
    DmReleaseResource (ship_list_handle[i]);
  }
}

void
restore_saved_commander (void)
{
  docked_planet = find_planet(cmdr.ship_x, cmdr.ship_y);
  hyperspace_planet = docked_planet;

  generate_planet_data (&current_planet_data, docked_planet);

  generate_stock_market ();
  set_stock_quantities (cmdr.station_stock);

  game_timer = TimGetSeconds();
}

void
restore_jameson_data(void) {
  const struct commander cmdr_jameson = {
    "JAMESON",			                /* Name                 */
    0,                                          /* initial game timer   */
    0,				                /* Mission Number       */
    0x14, 0xAD,			                /* Ship X,Y             */
    {0x4a, 0x5a, 0x48, 0x02, 0x53, 0xb7},	/* Galaxy Seed          */
    1000,				        /* Credits * 10         */
    70,				                /* Fuel * 10            */
//  0,
    0,				                /* Galaxy - 1           */
    PULSE_LASER,			        /* Front Laser          */
    0,				                /* Rear Laser           */
    0,			 	                /* Left Laser           */
    0,				                /* Right Laser          */
//  0, 0,
    20,				                /* Cargo Capacity       */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 
     0, 0, 0, 0, 0, 0, 0, 0},	                /* Current Cargo        */
    0,				                /* ECM                  */
    0,				                /* Fuel Scoop           */
    0,				                /* Energy Bomb          */
    0,				                /* Energy Unit          */
    0,				                /* Docking Computer */
    0,				                /* Galactic H'Drive     */
    0,				                /* Escape Pod           */
//  0,0,0,0,
    3,				                /* No. of Missiles      */
    0,				                /* Legal Status         */
    {0x10, 0x0F, 0x11, 0x00, 0x03, 0x1C,	/* Station Stock        */
     0x0E, 0x00, 0x00, 0x0A, 0x00, 0x11,
     0x3A, 0x07, 0x09, 0x08, 0x00},
    0,				                /* Fluctuation          */
    0,				                /* Score                */
    0x80			                /* Saved                */
  };

  DEBUG_PRINTF("restore jameson data\n");

  cmdr = cmdr_jameson;
}


void
restore_jameson(void) {

  game_timer = TimGetSeconds();

  /* try to load auto save state */
  if(load_commander(AUTO_SAVE_SLOT) != 0) {
    char name[DLG_MAX_LEN+1];
    UInt16 size = DLG_MAX_LEN+1;
    struct dialog new_name = {
      DLG_TXTINPUT, DLG_NAME,
      "Name Commander",
      "Enter your name:",
      NULL,
      "Ok",
      NULL
    };

    restore_jameson_data();

    StrCopy(name, cmdr.name);
    new_name.init = name;
    dialog_draw(&new_name);  

    docked_planet = find_planet (cmdr.ship_x, cmdr.ship_y);
    hyperspace_planet = docked_planet;

    generate_planet_data (&current_planet_data, docked_planet);
    generate_stock_market ();
    set_stock_quantities (cmdr.station_stock);
  }
}
