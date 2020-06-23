/*
   PBrickRemote.c

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
#include <KeyMgr.h>

#include "Rsc.h"
#include "PBrick.h"

static Boolean StartApplication(void);
static void    StopApplication(void);

static Boolean MainFormHandleEvent(EventPtr event);
static Boolean ApplicationHandleEvent(EventPtr event);
static void    EventLoop(void);

/* reference for library */
UInt16 PBrickRef=0;

#define TIMEOUT 10
UInt32 Timer=0;

static Boolean StartApplication(void) {
  Err err;

  /* try to open tilt sensor lib */
  err = SysLibFind(PBRICK_LIB_NAME, &PBrickRef);
  if (err) err = SysLibLoad('libr', PBRICK_LIB_CREATOR, &PBrickRef);

  if(!err) {
    /* try to initialize sensor */
    if((err = PBrickOpen(PBrickRef)) != errNone) {
      switch(err) {
      case errNoMgr:
	FrmCustomAlert(alt_err, 
	  "PBrickOpen: No NewSerialManager (PalmOS 3.3 req.).",0,0);
	break;
      case errOpenSer:
	FrmCustomAlert(alt_err, 
          "PBrickOpen: Unable to open serial interface.",0,0);
	break;
      case errSetPort:
	FrmCustomAlert(alt_err, 
          "PBrickOpen: Unable to set port parameters.",0,0);
	break;
      case errNoIR:
	FrmCustomAlert(alt_err, 
	  "PBrickOpen: Unable to switch to IR operation.",0,0);
	break;
      default:
	FrmCustomAlert(alt_err, "PBrickOpen: Unknown error!",0,0);
      }
      return false;
    }
  } else {
    FrmCustomAlert(alt_err, "Unable to open PBrick Library.",0,0);
    PBrickRef = 0;
    return false;
  }

  /* mask hard keys */
  KeySetMask(~(keyBitHard1  | keyBitHard2 | 
	       keyBitHard3  | keyBitHard4 |
	       keyBitPageUp | keyBitPageDown));
 
  FrmGotoForm(frm_Main);
  return true;
}

/* shutdown library communication */
static void StopApplication(void) {
  UInt16 numapps;
 
  KeySetMask( keyBitsAll );
 
  if(PBrickRef != 0) {
    /* close sensor lib */
    PBrickClose(PBrickRef, &numapps);

    /* Check for errors in the Close() routine */
    if (numapps == 0)
      SysLibRemove(PBrickRef);
  }
}

static Boolean MainFormHandleEvent(EventPtr event) {
  Boolean handled = false;
  EventType newEvent;
  Err err;
  UInt16 val0, val1;
  PBrickConfigType config;
  static Boolean first=true;

  switch (event->eType) {

  case ctlSelectEvent:
    SysTaskDelay(10);

    switch(event->data.ctlEnter.controlID) {
    case btn_beep:
      PBrickRemote(PBrickRef, REMOTE_BEEP_CMD);
      break;    
    case btn_stop:
      PBrickRemote(PBrickRef, REMOTE_STOP_CMD);
      break;    

    case btn_a_fwd:
      PBrickRemote(PBrickRef, REMOTE_AFWD_CMD);
      break;    
    case btn_a_rev:
      PBrickRemote(PBrickRef, REMOTE_AREV_CMD);
      break;    
    case btn_b_fwd:
      PBrickRemote(PBrickRef, REMOTE_BFWD_CMD);
      break;    
    case btn_b_rev:
      PBrickRemote(PBrickRef, REMOTE_BREV_CMD);
      break;    
    case btn_c_fwd:
      PBrickRemote(PBrickRef, REMOTE_CFWD_CMD);
      break;    
    case btn_c_rev:
      PBrickRemote(PBrickRef, REMOTE_CREV_CMD);
      break;    

    case btn_msg1:
      PBrickRemote(PBrickRef, REMOTE_MSG1_CMD);
      break;    
    case btn_msg2:
      PBrickRemote(PBrickRef, REMOTE_MSG2_CMD);
      break;    
    case btn_msg3:
      PBrickRemote(PBrickRef, REMOTE_MSG3_CMD);
      break;    

    case btn_prg1:
      PBrickRemote(PBrickRef, REMOTE_PRG1_CMD);
      break;    
    case btn_prg2:
      PBrickRemote(PBrickRef, REMOTE_PRG2_CMD);
      break;    
    case btn_prg3:
      PBrickRemote(PBrickRef, REMOTE_PRG3_CMD);
      break;    
    case btn_prg4:
      PBrickRemote(PBrickRef, REMOTE_PRG4_CMD);
      break;    
    case btn_prg5:
      PBrickRemote(PBrickRef, REMOTE_PRG5_CMD);
      break;    

    }

    Timer = TIMEOUT;
    handled = true;
    break;
    
  case frmOpenEvent:
    FrmDrawForm(FrmGetActiveForm());
    handled = true;
    break;
    
  case menuEvent:
    switch (event->data.menu.itemID) {
    case mit_About:
      FrmAlert(alt_About);
      handled = true;
    }
  }
  return(handled);
}

static Boolean ApplicationHandleEvent(EventPtr event) {
  FormPtr form;
  UInt16  formId;
  Boolean handled = false;

  if (event->eType == frmLoadEvent) {
    formId = event->data.frmLoad.formID;
    form = FrmInitForm(formId);
    FrmSetActiveForm(form);

    switch (formId) {
    case frm_Main:
      FrmSetEventHandler(form, MainFormHandleEvent);
      break;
    }
    handled = true;
  }
  return handled;
}

static void EventLoop(void) {
  EventType event;
  UInt16    error, key;
  UInt32    hardKeyState;

  do {
    EvtGetEvent(&event, 10);

    if (SysHandleEvent(&event))
      continue;
    if (MenuHandleEvent(NULL, &event, &error))
      continue;
    if (ApplicationHandleEvent(&event))
      continue;

    /* check if hardkeys are pressed */
    hardKeyState = KeyCurrentState(); key = 0;
    if(hardKeyState & keyBitHard1)    key |= REMOTE_AFWD_CMD;
    if(hardKeyState & keyBitHard2)    key |= REMOTE_AREV_CMD;
    if(hardKeyState & keyBitPageUp)   key |= REMOTE_BFWD_CMD;
    if(hardKeyState & keyBitPageDown) key |= REMOTE_BREV_CMD;
    if(hardKeyState & keyBitHard3)    key |= REMOTE_CFWD_CMD;
    if(hardKeyState & keyBitHard4)    key |= REMOTE_CREV_CMD;

    /* decrease timeout timer */
    if(Timer > 0) Timer--;

    /* all buttons are released */
    if((key != 0)||(Timer == 0))
      PBrickRemote(PBrickRef, key);

    FrmDispatchEvent(&event);
  }
  while(event.eType != appStopEvent);
}

UInt32	PilotMain(UInt16 cmd, void *cmdPBP, UInt16 launchFlags) {
  if (cmd == sysAppLaunchCmdNormalLaunch) {
    if(StartApplication())
      EventLoop();

    StopApplication();
  }
  return(0);
}









