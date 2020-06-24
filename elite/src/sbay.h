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

#ifndef SBAY_H
#define SBAY_H

#include "sections.h"

/* offer from seller to potential buyer */
typedef struct {
  Char   seller[DLG_MAX_LEN+1];   // name of seller

  // infos on item to trade
  UInt8  item;                    // item to be sold
  UInt16 quantity;                // number of items to be sold
  UInt16 price;                   // price per item

  // sellers position in space
  Int16 ship_x, ship_y;           // coordinates in galaxy
  UInt8 galaxy_number;            // galaxy id
} sbay_offer;

/* initial reply from buyer to seller */
typedef struct {
  Char   buyer[DLG_MAX_LEN+1];    // name of buyer

  // buyers position in space
  Int16 ship_x, ship_y;           // coordinates in galaxy
  UInt8 galaxy_number;            // galaxy id

  Bool   ack;                     // buyer wants to negotiate/trade
} sbay_reply;

Boolean sbay_enabled;             // sbay system is up and running

void SECDK sbay_init(void);
void SECDK sbay_free(void);
void SECDK sbay_sell_stock(int);
void SECDK sbay_handle_tap(UInt8 x, UInt8 y);

#endif //SBAY_H
