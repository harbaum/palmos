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
 * trade.h
 */

#ifndef TRADE_H
#define TRADE_H

#define CURRENCY "Cr"

struct stock_item {
  char name[16];
  int current_quantity;
  int current_price;
  int base_price;
  int eco_adjust;
  int base_quantity;
  int mask;
  int units;
};


#define NO_OF_STOCK_ITEMS	17
#define ALIEN_ITEMS_IDX		16

extern struct stock_item stock_market[NO_OF_STOCK_ITEMS];

void generate_stock_market (void);
void set_stock_quantities (int *quant);
int carrying_contraband (void);
int total_cargo (void);
void scoop_item (int un);

#endif
