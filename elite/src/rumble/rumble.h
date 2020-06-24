/*
   rumble.h 
*/

#ifndef RUMBLE_LIB_H
#define RUMBLE_LIB_H

#define RUMBLE_UNSUPPORTED 1

#define RUMBLE_NAME    "Rumble Lib"
#define RUMBLE_CREATOR 'RLib'

/* tilt interface function declarations */
Err RumbleLibOpen(UInt16 refNum)                SYS_TRAP(sysLibTrapOpen);
Err RumbleLibClose(UInt16 refNum, UInt16 *num)  SYS_TRAP(sysLibTrapClose);
Err RumbleLibRumble(UInt16 refNum, Boolean on)  SYS_TRAP(sysLibTrapCustom+0);
Err RumbleLibLED(UInt16 refNum, Boolean on)     SYS_TRAP(sysLibTrapCustom+1);

#endif

