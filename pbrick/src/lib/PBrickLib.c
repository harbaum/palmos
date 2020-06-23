/* 
   PBrickLib.c

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

#include <PalmOS.h>

#include "PBrick.h"
#include "PBrickIO.h"

/* Dispatch table declarations */
UInt32 install_dispatcher(UInt16, SysLibTblEntryPtr);
void   *gettable(void);

UInt32
start (UInt16 refnum, SysLibTblEntryPtr entryP) {
  UInt32 ret;

  asm("
        movel %%fp@(10),%%sp@-
        movew %%fp@(8),%%sp@-
        jsr install_dispatcher(%%pc)
        movel %%d0,%0
        jmp out(%%pc)
gettable:
        lea jmptable(%%pc),%%a0
        rts

        .set FUNC_NO, 13             | number of library functions

jmptable:
        dc.w (6*FUNC_NO+2)

        # offsets to the system function calls
        dc.w ( 0*4 + 2*FUNC_NO + 2)  | open
        dc.w ( 1*4 + 2*FUNC_NO + 2)  | close
        dc.w ( 2*4 + 2*FUNC_NO + 2)  | sleep
        dc.w ( 3*4 + 2*FUNC_NO + 2)  | wake

        # offsets to the user function calls
        # config stuff
        dc.w ( 4*4 + 2*FUNC_NO + 2)  | PBrickGetConfig
        dc.w ( 5*4 + 2*FUNC_NO + 2)  | PBrickSetConfig
        # low level stuff
        dc.w ( 6*4 + 2*FUNC_NO + 2)  | PBrickCmd
        dc.w ( 7*4 + 2*FUNC_NO + 2)  | PBrickCmdReply
        # high level stuff
        dc.w ( 8*4 + 2*FUNC_NO + 2)  | PBrickAlive
        dc.w ( 9*4 + 2*FUNC_NO + 2)  | PBrickSound
        dc.w (10*4 + 2*FUNC_NO + 2)  | PBrickGetVersions
        dc.w (11*4 + 2*FUNC_NO + 2)  | PBrickGetBattery
        dc.w (12*4 + 2*FUNC_NO + 2)  | PBrickRemote

        # functions for system use
        jmp PBrickOpen(%%pc)
        jmp PBrickClose(%%pc)
        jmp PBrickSleep(%%pc)
        jmp PBrickWake(%%pc)

        # config stuff
        jmp PBrickGetConfig(%%pc)
        jmp PBrickSetConfig(%%pc)

        # low level functions
        jmp PBrickCmd(%%pc)
        jmp PBrickCmdReply(%%pc)

        # high level functions
        jmp PBrickAlive(%%pc)
        jmp PBrickSound(%%pc)
        jmp PBrickGetVersions(%%pc)
        jmp PBrickGetBattery(%%pc)
        jmp PBrickRemote(%%pc)

        .asciz \"PBrick Library\"
    .even
    out:
        " : "=r" (ret) :);
  return ret;
}

/* routine to install library into system */
UInt32
install_dispatcher(UInt16 refnum, SysLibTblEntryPtr entryP) {
  void *table = gettable();
  entryP->dispatchTblP = table;
  entryP->globalsP = NULL;
  return 0;
}

Err 
PBrickOpen(UInt16 refnum) {
  SysLibTblEntryPtr entryP = SysLibTblEntry(refnum);
  PBrickGlobals *gl = (PBrickGlobals*)entryP->globalsP;
  Err err;

  /* Get the current globals value */
  if (gl) {
    /* We are already open in some other application; just
       increment the refcount */
    gl->refcount++;
    return errNone;
  }

  /* We need to allocate space for the globals */
  entryP->globalsP = MemPtrNew(sizeof(PBrickGlobals));

  MemPtrSetOwner(entryP->globalsP, 0);
  gl = (PBrickGlobals*)entryP->globalsP;

  gl->refcount = 1;
  gl->check_alive = false;
  gl->retries = DEF_RETRY;
  gl->toggle = 0;
  gl->portId = 0;

  /* initialize everything */
  err = open_pbrick(gl);
  calibrate(gl);

  /* everything is fine */
  return err;
}

Err 
PBrickClose(UInt16 refnum, UInt16 *numappsP) {
  /* Get the current globals value */
  SysLibTblEntryPtr entryP = SysLibTblEntry(refnum);
  PBrickGlobals *gl = (PBrickGlobals*)entryP->globalsP;

  /* Clean up */
  *numappsP = --gl->refcount;
  if (*numappsP == 0) {

    /* shutdown everything */
      close_pbrick(gl);

    MemChunkFree(entryP->globalsP);
    entryP->globalsP = NULL;
  }
  return errNone;
}

Err 
PBrickSleep(UInt16 refnum) {
  /* Get the current globals value */
  PBrickGlobals *gl = (PBrickGlobals*)(SysLibTblEntry(refnum)->globalsP);

  /* restore systems hardware state (may save power) */
  hw_restore(gl);

  return errNone;
}

Err 
PBrickWake(UInt16 refnum) {
  /* Get the current globals value */
  PBrickGlobals *gl = (PBrickGlobals*)(SysLibTblEntry(refnum)->globalsP);

  /* grab hardware */
  hw_setup(gl);

  return errNone;
}

/* library config functions */
Err
PBrickGetConfig(UInt16 refnum, PBrickConfigType *config) {

  /* Get the current globals value */
  PBrickGlobals *gl = (PBrickGlobals*)(SysLibTblEntry(refnum)->globalsP);

  /* interface settings */
  config->com_speed = 2400;
  config->check_alive = gl->check_alive;
  config->retries = gl->retries;

  /* internal stuff */
  config->version   = 100;
  config->cpu_is_ez = gl->cpu_is_ez;
  config->timer_val = gl->timer_val;
  config->error = gl->error;

  return errNone;
}

Err
PBrickSetConfig(UInt16 refnum, PBrickConfigType *config) {

  /* Get the current globals value */
  PBrickGlobals *gl = (PBrickGlobals*)(SysLibTblEntry(refnum)->globalsP);

  /* setting the com speed is currently not supported */
  if(config->com_speed != -1) {
      if(config->com_speed != 2400)
	  return errSpeed;
  }

  if(config->check_alive != -1) gl->check_alive = config->check_alive;
  if(config->retries != -1)     gl->retries     = config->retries;

  return errNone;
}

/* low level functions */
Err
PBrickCmd(UInt16 refnum, Int8 *cmd, UInt16 clen) {

  /* Get the current globals value */
  PBrickGlobals *gl = (PBrickGlobals*)(SysLibTblEntry(refnum)->globalsP);

  /* send message and don't expect a reply */
  return send_msg(gl, cmd, clen, NULL, 0);
}

Err
PBrickCmdReply(UInt16 refnum, Int8 *cmd,   UInt16 clen,
	                      Int8 *reply, UInt16 *replylen) {

  /* Get the current globals value */
  PBrickGlobals *gl = (PBrickGlobals*)(SysLibTblEntry(refnum)->globalsP);

  /* send message and get a reply */
  return send_msg(gl, cmd, clen, reply, replylen);
}

/* high level functions */
Err
PBrickAlive(UInt16 refnum) {
  /* Get the current globals value */
  PBrickGlobals *gl = (PBrickGlobals*)(SysLibTblEntry(refnum)->globalsP);

  return check_alive(gl);
}

Err
PBrickSound(UInt16 refnum, UInt16 sound_id) {
  /* Get the current globals value */
  PBrickGlobals *gl = (PBrickGlobals*)(SysLibTblEntry(refnum)->globalsP);
  Err err;

  /* check if rcx will talk to us */
  if(gl->check_alive)
    if(err=check_alive(gl))
      return err;
  
  return play_sound(gl, sound_id);
}

Err
PBrickGetVersions(UInt16 refnum, UInt16 *rom_ver, UInt16 *fw_ver) {
  /* Get the current globals value */
  PBrickGlobals *gl = (PBrickGlobals*)(SysLibTblEntry(refnum)->globalsP);
  Err err;

  /* check if rcx will talk to us */
  if(gl->check_alive)
    if(err=check_alive(gl))
      return err;
  
  return get_versions(gl, rom_ver, fw_ver);
}

Err
PBrickGetBattery(UInt16 refnum, UInt16 *millivolts) {
  /* Get the current globals value */
  PBrickGlobals *gl = (PBrickGlobals*)(SysLibTblEntry(refnum)->globalsP);
  Err err;

  /* check if rcx will talk to us */
  if(gl->check_alive)
    if(err=check_alive(gl))
      return err;
  
  return get_battery(gl, millivolts);
}

Err
PBrickRemote(UInt16 refnum, UInt16 command) {
  /* Get the current globals value */
  PBrickGlobals *gl = (PBrickGlobals*)(SysLibTblEntry(refnum)->globalsP);
  UInt8 msg[3]={0xd2,command>>8,command&0xff};
  Err err;

  /* check if rcx will talk to us */
  if(gl->check_alive)
    if(err=check_alive(gl))
      return err;
  
  /* send message and don't expect a reply */
  return send_msg(gl, msg, 3, NULL, 0);
}
