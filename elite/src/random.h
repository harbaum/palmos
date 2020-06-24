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
 * random.h
 */

#ifndef RANDOM_H
#define RANDOM_H

Int32 randint (void);
void set_rand_seed (UInt32 seed);
UInt32 get_rand_seed (void);
Int16 rand255 (void);

#endif
