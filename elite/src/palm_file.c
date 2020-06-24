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

/*
 * file.c
 */

#include <PalmOS.h>

#include "elite.h"
#include "config.h"
#include "file.h"
#include "options.h"
#include "debug.h"

int
rename_commander(int id, char *new) 
{
  Err err;
  UInt8 block[sizeof(struct commander)];
  UInt16 size = sizeof(struct commander);

#ifdef DEBUG
  if(id>=0) DEBUG_PRINTF("rename commander in slot %d\n", id);
  else      DEBUG_PRINTF("rename commander in auto save slot\n");
#endif

  /* save in prefs */
  err = PrefGetAppPreferences(CREATOR, SAVE_PREFS_0+id, block, &size, true);

  /* error while loading prefs? */
  if(err == noPreferenceFound) return 1;
  if(size != sizeof(struct commander)) return 2;

  /* save new name */
  StrCopy(block, new);

  /* save in prefs */
  PrefSetAppPreferences(CREATOR, SAVE_PREFS_0+id, 1, block, size, true);

  return 0;
}

int
save_commander(int id)
{
  int i;
  UInt8 block[sizeof(struct commander)];
  UInt16 size = sizeof(struct commander);
  UInt32 save_timer;

#ifdef DEBUG
  if(id>=0) DEBUG_PRINTF("save commander in slot %d\n", id);
  else      DEBUG_PRINTF("save commander in auto save slot\n");
#endif

  /* saved docked planet in commander structure */
  cmdr.ship_x = docked_planet.d;
  cmdr.ship_y = docked_planet.b;

  /* set game timer */
  save_timer = cmdr.timer;

  /* saved timer is the time between game start and now */
  /* plus the time played in previous sessions */
  cmdr.timer += TimGetSeconds() - game_timer;

  /* save current stock in commander structure */
  for (i = 0; i < NO_OF_STOCK_ITEMS; i++)
    cmdr.station_stock[i] = stock_market[i].current_quantity;

  /* save commander info */
  MemMove(block, &cmdr, sizeof(struct commander));

  /* save in prefs */
  PrefSetAppPreferences(CREATOR, SAVE_PREFS_0+id, 1, block, size, true);

  /* restore old timer value */
  cmdr.timer = save_timer;
  return 0;
}

int
load_commander(int id)
{
  UInt8 block[sizeof(struct commander)];
  UInt16 size = sizeof(struct commander);
  Err err;

#ifdef DEBUG
  if(id>=0) DEBUG_PRINTF("load commander from slot %d\n", id);
  else      DEBUG_PRINTF("load commander from auto save slot\n");
#endif

  /* save in prefs */
  err = PrefGetAppPreferences(CREATOR, SAVE_PREFS_0+id, block, &size, true);

  /* error while loading prefs? */
  if(err == noPreferenceFound) return 1;
  if(size != sizeof(struct commander)) return 2;

  /* restore commander */
  MemMove(&cmdr, block, sizeof(struct commander));

  docked_planet = find_planet (cmdr.ship_x, cmdr.ship_y);
  hyperspace_planet = docked_planet;

  generate_planet_data (&current_planet_data, docked_planet);
  generate_stock_market ();
  set_stock_quantities (cmdr.station_stock);

  /* restart game timer on load */
  game_timer = TimGetSeconds();

  return 0;
}
