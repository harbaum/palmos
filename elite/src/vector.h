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
 * vector.h
 */


#ifndef VECTOR_H
#define VECTOR_H

#include "sections.h"

#define SQUARE(a) ((a)*(a))

/* this vector describes an objects position in space */
struct pos_vec {
  Int32 x,y,z;
};

/* this vector is a unit vector and usually part of a rotation matrix */
struct rot_vec {
  Int16 x,y,z;
};

typedef struct rot_vec RotMatrix[3];
typedef struct rot_vec RotVector;
typedef struct pos_vec PosVector;

extern long labs(long);

/* more complex operations on fix32 */
#ifdef DEBUG
void dbg_print_pos_vec(char *msg, PosVector * vec) SEC3D;
void dbg_print_rot_vec(char *msg, RotVector * vec) SEC3D;
void dbg_print_rot_mat(char *msg, RotVector * vec) SEC3D;
#endif

UInt16 SEC3D int_sqrt (UInt32 quad);

#define ROTDIV       (16384)
#define ROTBITS      (14)

Int32 vector_dot_product_pos_rot(PosVector *first, RotVector *second) SEC3D;
Int16 vector_dot_product_rot_rot(RotVector *first, RotVector *second) SEC3D;
void unit_rot_vector(RotVector *vec) SEC3D;
RotVector unit_pos_vector(PosVector *vec) SEC3D;
void mult_vector (PosVector *vec, RotVector *mat) SEC3D;
void set_init_matrix (RotVector *mat) SEC3D;

#endif
