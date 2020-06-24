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

#ifndef DOCKED_H
#define DOCKED_H

#include "sections.h"

void SECDK display_game_timer(void);
void SECDK set_time_str(char *str, UInt32 time);
void SECDK display_short_range_chart (void);
void SECDK display_galactic_chart (void);
void SECDK display_data_on_planet (void);
void SECDK show_distance_to_planet (void);
void SECDK find_planet_by_name (char *);
void SECDK display_market_prices (void);
void SECDK display_commander_status (void);
int  SECDK calc_distance_to_planet (struct galaxy_seed, struct galaxy_seed);
void SECDK buy_stock (int);
void SECDK sell_stock (int);
void SECDK display_inventory (void);
void SECDK equip_ship (void);
void SECDK buy_equip (int, int);
Bool SECDK handle_map_tap(int x, int y);
void SECDK find_named_planet(void);

extern int cross_x, cross_y;
extern int old_cross_x, old_cross_y;
extern UInt8 last_chart;

#endif
