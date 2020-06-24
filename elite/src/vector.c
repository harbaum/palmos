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
 * vector.c
 */

#include <PalmOS.h>

#include "config.h"
#include "vector.h"
#include "debug.h"

static RotMatrix start_matrix = {
  { -ROTDIV,       0,       0 },
  {       0,  ROTDIV,       0 },
  {       0,       0, -ROTDIV }
};

#define N_BITS 32
#define MAX_BIT ((N_BITS + 1) / 2 - 1)

/* fast integer sqrt in c */
UInt16
int_sqrt (UInt32 x)
{
  UInt32 xroot, m2, x2;

  xroot = 0;
  m2 = (UInt32) 1 << MAX_BIT * 2;

  do {
    x2 = xroot + m2;
    xroot >>= 1;
    if (x2 <= x) {
      x -= x2;
      xroot += m2;
    }
  } while (m2 >>= 2);
  if (xroot < x)
    return xroot + 1;
  return xroot;
}

#ifdef DEBUG
void
dbg_print_pos_vec(char *msg, PosVector * vec) {
  DEBUG_PRINTF ("%s: %ld/%ld/%ld\n", msg, vec->x, vec->y, vec->z);
}

void
dbg_print_rot_vec(char *msg, RotVector * vec) {
  DEBUG_PRINTF ("%s: %ld/%ld/%ld\n", msg, 
		(1000l*vec->x)>>ROTBITS, 
		(1000l*vec->y)>>ROTBITS, 
		(1000l*vec->z)>>ROTBITS);
}

void
dbg_print_rot_mat(char *msg, RotVector * vec) {
  int i;
  for(i=0;i<3;i++)
    DEBUG_PRINTF ("%s[%d]: %ld/%ld/%ld\n", msg, i, 
		  (1000l*vec[i].x)>>ROTBITS, 
		  (1000l*vec[i].y)>>ROTBITS, 
		  (1000l*vec[i].z)>>ROTBITS);
}
#endif

void
mult_vector (PosVector *vec, RotVector *mat)
{
  PosVector tmp;

  tmp.x = ((vec->x * mat[0].x) + 
	   (vec->y * mat[0].y) + 
	   (vec->z * mat[0].z)) >> ROTBITS;

  tmp.y = ((vec->x * mat[1].x) +
           (vec->y * mat[1].y) +
           (vec->z * mat[1].z)) >> ROTBITS;

  tmp.z = ((vec->x * mat[2].x) +
           (vec->y * mat[2].y) +
           (vec->z * mat[2].z)) >> ROTBITS;

  *vec = tmp;
}


/*
 * Calculate the dot product of two vectors sharing a common point.
 * Returns the cosine of the angle between the two vectors.
 */

/* return value is of scale ROTDIV */
Int32 vector_dot_product_pos_rot(PosVector *a, RotVector *b) {
  return (( a->x * (Int32)(b->x) ) +
	  ( a->y * (Int32)(b->y) ) +
	  ( a->z * (Int32)(b->z) ) );
}

/* return value is of scale ROTDIV */
Int16 vector_dot_product_rot_rot(RotVector *a, RotVector *b) {
  return (( (Int32)(a->x) * (Int32)(b->x) ) +
	  ( (Int32)(a->y) * (Int32)(b->y) ) +
	  ( (Int32)(a->z) * (Int32)(b->z) )>>ROTBITS );
}

/*
 * Convert a vector into a vector of unit (1) length.
 */

void
unit_rot_vector(RotVector *vec) {
  UInt16 uni;

  uni = int_sqrt(SQUARE(((Int32)vec->x)) + SQUARE(((Int32)vec->y)) + 
		 SQUARE(((Int32)vec->z)));
  if (uni == 0) {
    DEBUG_PRINTF ("INTERNAL ERROR!\n");
    uni = 1;
  }

  vec->x = (uni/2 + ROTDIV * (Int32)vec->x) / uni;
  vec->y = (uni/2 + ROTDIV * (Int32)vec->y) / uni;
  vec->z = (uni/2 + ROTDIV * (Int32)vec->z) / uni;
}

/*
 * Convert a position into a vector of unit (1) length.
 */

RotVector
unit_pos_vector(PosVector *vec) {
  UInt16 uni;
  PosVector in = *vec;
  RotVector res;

  // FIXME
  uni = 32768;
  while((labs(in.x) > uni)||(labs(in.y) > uni)||(labs(in.z) > uni)) {
    in.x = (in.x + 1) / 2;
    in.y = (in.y + 1) / 2;
    in.z = (in.z + 1) / 2;
  }

  uni = int_sqrt(SQUARE(in.x) + SQUARE(in.y) + SQUARE(in.z));
  if (uni == 0) {
    DEBUG_PRINTF ("INTERNAL ERROR!\n");
    uni = 1;
  }

  res.x = (uni/2 + ROTDIV * in.x) / uni;
  res.y = (uni/2 + ROTDIV * in.y) / uni;
  res.z = (uni/2 + ROTDIV * in.z) / uni;

  return res;
}

void
set_init_matrix (RotVector *mat)
{
  int i;

  for (i = 0; i < 3; i++)
    mat[i] = start_matrix[i];
}
