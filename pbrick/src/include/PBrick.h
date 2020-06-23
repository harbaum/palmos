/*
   PBrick.h

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

#ifndef PBRICK_H
#define PBRICK_H

#define PBRICK_LIB_NAME     "PBrick Library"
#define PBRICK_LIB_CREATOR  'PLib'

#ifdef PBRICK_NOTRAPDEFS
#define PBRICK_SYS_TRAP(trap)
#else
#define PBRICK_SYS_TRAP(trap) SYS_TRAP(trap)
#endif

/* error codes */
#define errNoMgr    -1   /* no new serial manager installed */
#define errOpenSer  -2   /* unable to open serial lib */
#define errSetPort  -3   /* error setting port controls */
#define errNoIR     -4   /* setting IR failed */
#define errLevel0   -5   /* low level receive error */
#define errLevel1   -6   /* high level receive error */
#define errReplyLen -7   /* unexpected reply length */
#define errReply    -8   /* unexpected reply message */
#define errSpeed    -9   /* unsupported communication speed */

/* remote commands */
#define REMOTE_NULL_CMD	  0x0000
#define REMOTE_MSG1_CMD	  0x0001
#define REMOTE_MSG2_CMD	  0x0002
#define REMOTE_MSG3_CMD	  0x0004
#define REMOTE_AFWD_CMD	  0x0008
#define REMOTE_BFWD_CMD	  0x0010
#define REMOTE_CFWD_CMD	  0x0020
#define	REMOTE_AREV_CMD	  0x0040
#define	REMOTE_BREV_CMD	  0x0080
#define REMOTE_CREV_CMD	  0x0100
#define	REMOTE_PRG1_CMD	  0x0200
#define	REMOTE_PRG2_CMD	  0x0400
#define REMOTE_PRG3_CMD	  0x0800
#define REMOTE_PRG4_CMD	  0x1000
#define	REMOTE_PRG5_CMD	  0x2000
#define	REMOTE_STOP_CMD	  0x4000
#define	REMOTE_BEEP_CMD	  0x8000

typedef struct {
  /* entries to be read an set, -1 = don't set */
  Int16 check_alive;   /* 0/1 = enable */
  Int16 retries;       /* number of retries (default = 5) */
  Int16 com_speed;     /* interface speed (2400 bit/s) */

  /* read only entries */
  UInt16  version;     /* library version */
  Boolean cpu_is_ez;   /* a ez/vz cpu was detected */
  UInt16  timer_val;   /* internal timer value */
  Int16   error;       /* frequency generation error */
} PBrickConfigType;

/* lib functions */
Err      PBrickOpen(UInt16 refNum)
                     PBRICK_SYS_TRAP(sysLibTrapOpen);
Err      PBrickClose(UInt16 refNum, UInt16 *numappsP)
                     PBRICK_SYS_TRAP(sysLibTrapClose);

/* library control functions */
Err      PBrickGetConfig(UInt16 refnum, PBrickConfigType *config)
                     PBRICK_SYS_TRAP(sysLibTrapCustom+0);
Err      PBrickSetConfig(UInt16 refnum, PBrickConfigType *config)
                     PBRICK_SYS_TRAP(sysLibTrapCustom+1);

/* low level functions */
Err      PBrickCmd(UInt16 refnum, Int8 *cmd,   UInt16 clen)
                     PBRICK_SYS_TRAP(sysLibTrapCustom+2);
Err      PBrickCmdReply(UInt16 refnum, Int8 *cmd,   UInt16 clen,
			               Int8 *reply, UInt16 *replylen)
                     PBRICK_SYS_TRAP(sysLibTrapCustom+3);

/* high level functions */
Err      PBrickAlive(UInt16 refnum)
                     PBRICK_SYS_TRAP(sysLibTrapCustom+4);
Err      PBrickSound(UInt16 refnum, UInt16 sound_id)
                     PBRICK_SYS_TRAP(sysLibTrapCustom+5);
Err      PBrickGetVersions(UInt16 refnum, UInt16 *rom_ver, UInt16 *fw_ver)
                     PBRICK_SYS_TRAP(sysLibTrapCustom+6);
Err      PBrickGetBattery(UInt16 refnum, UInt16 *millivolts)
                     PBRICK_SYS_TRAP(sysLibTrapCustom+7);
Err      PBrickRemote(UInt16 refnum, UInt16 command)
                     PBRICK_SYS_TRAP(sysLibTrapCustom+8);

#endif // PBRICK_H

