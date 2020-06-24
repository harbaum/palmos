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
 * random.c
 */


#include <stdlib.h>

#include "random.h"

static UInt32 rand_seed;

/*
 * Portable random number generator implementing the recursion:
 *     IX = 16807 * IX MOD (2**(31) - 1)
 * Using only 32 bits, including sign.
 *
 * Taken from "A Guide to Simulation" by Bratley, Fox and Schrage.
 */

Int32
randint (void)
{
  UInt32 k1;
  UInt32 ix = rand_seed;

  k1 = ix / 127773;
  ix = 16807 * (ix - k1 * 127773) - k1 * 2836;
  if (ix < 0)
    ix += 2147483647l;
  rand_seed = ix;

  return (Int32)ix;
}


void
set_rand_seed (UInt32 seed)
{
  rand_seed = seed;
}


UInt32
get_rand_seed (void)
{
  return rand_seed;
}

Int16
rand255 (void)
{
  return (randint () & 255);
}
