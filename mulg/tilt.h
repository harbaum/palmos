/*
   tilt.h  -  (c) 1999 Till Harbaum

   palm tilt sensor interface

   t.harbaum@tu-bs.de
*/

#ifndef __TILT_H__
#define __TILT_H__

#define TILT_WRONG_OS  3
#define TILT_NO_SENSOR 4
#define TILT_HW_BUSY   5

/* tilt interface function declarations */
Err TiltLibOpen(UInt16 refNum)                       SYS_TRAP(sysLibTrapOpen);
Err TiltLibClose(UInt16 refNum, UInt16 *numappsP)    SYS_TRAP(sysLibTrapClose);
Err TiltLibZero(UInt16 refNum)                       SYS_TRAP(sysLibTrapCustom+0);
Err TiltLibGet(UInt16 refNum, Int16 *x, Int16 *y)    SYS_TRAP(sysLibTrapCustom+1);
Err TiltLibGetAbs(UInt16 refNum, Int16 *x, Int16 *y) SYS_TRAP(sysLibTrapCustom+2);

#endif

