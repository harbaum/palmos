/*
   PBrickIO.h

   Part of the PBrick Library, a PalmOS shared library for bidirectional
   infrared communication between a PalmOS device and the Lego Mindstorms
   RCX controller.

   Copyright (C) 2002 Till Harbaum <t.harbaum@web.de>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef PBRICKIO_H
#define PBRICKIO_H

#define PBRICK_NOTRAPDEFS
#include "PBrick.h"

/* registers for vz/ez cpu */
#define PEDIR   (*(volatile unsigned char*)0xfffff420l)
#define PEDATA  (*(volatile unsigned char*)0xfffff421l)
#define PEPUEN  (*(volatile unsigned char*)0xfffff422l)
#define PESEL   (*(volatile unsigned char*)0xfffff423l)

#define PE_TXD  (0x20)
#define PE_RXD  (0x10)

#define TXD_ON_EZ  { PEDATA |=  PE_TXD; }
#define TXD_OFF_EZ { PEDATA &= ~PE_TXD; }

#define RXD_ON_EZ  (PEDATA & PE_RXD)

/* registers for ordinary 328 cpu */
#define PGDIR   (*(volatile unsigned char*)0xfffff430l)
#define PGDATA  (*(volatile unsigned char*)0xfffff431l)
#define PGPUEN  (*(volatile unsigned char*)0xfffff432l)
#define PGSEL   (*(volatile unsigned char*)0xfffff433l)

#define PG_TXD (0x01)
#define PG_RXD (0x02)

#define IMR     (*(volatile unsigned long*)0xfffff304l)

/* timer registers required for calibration */
#define TCTL  (*(unsigned short*)0xfffff600l)
#define TPRER (*(unsigned short*)0xfffff602l)
#define TCN   (*(unsigned short*)0xfffff608l)

#define CALI_REZ  16

/* receive answer from rcx */
#define RCV_BYTES 128 /* size of receiver buffer in bytes */
#define RCV_BITS  12  /* at most this number of bits per byte */
#define BUFFER_SIZE (RCV_BYTES*RCV_BITS)   /* this has to be changed in the asm part as well */

#define DEF_RETRY 5   /* default number of retrys */

typedef enum { s_start=0, s_d0, s_d1, s_d2, s_d3, s_d4, s_d5, s_d6, s_d7, s_p, s_stop, s_last } rcv_state;

/* replacement for library globals */
typedef struct {
    UInt16 refcount;

    UInt16 portId;
    UInt16 timer_val;

    Int16  cpu_is_ez;    // flag, that cpu is ez/vz type

    UInt16  check_alive;
    UInt16  retries;

    UInt16  toggle;
    Int16   error;

    Int16  rcv_buffer[BUFFER_SIZE];
} PBrickGlobals;

extern void hw_setup(PBrickGlobals *gl);
extern void hw_restore(PBrickGlobals *gl);
extern Err  open_pbrick(PBrickGlobals *gl);
extern Err  close_pbrick(PBrickGlobals *gl);

extern void calibrate(PBrickGlobals *gl);
extern Int16 send_msg(PBrickGlobals *gl, Int8 *data, UInt16 len, Int8 *rcv, UInt16 *rcv_len);
extern Err check_alive(PBrickGlobals *gl);
extern Err play_sound(PBrickGlobals *gl, UInt16 snd);
extern Err get_versions(PBrickGlobals *gl, UInt16 *rom, UInt16 *fw);
extern Err get_battery(PBrickGlobals *gl, UInt16 *bat);

#endif // PBRICKIO_H




