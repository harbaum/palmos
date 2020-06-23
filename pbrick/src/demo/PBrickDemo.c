/*
   PBrickDemo.c

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
#include <unix_stdarg.h>

#include "AppStdIO.h"
#include "Rsc.h"
#include "PBrick.h"

static Boolean StartApplication(void);
static void    StopApplication(void);

static Boolean MainFormHandleEvent(EventPtr event);
static Boolean ApplicationHandleEvent(EventPtr event);
static void    EventLoop(void);

/* reference for library */
UInt16 PBrickRef=0;

void print_error(Err err) {
  switch(err) {
  case errNone:
    printf("ok.\n");
    break;
  case errLevel0:
    printf("ERROR: No reply.\n");
    break;
  case errLevel1:
    printf("ERROR: Reply decode error.\n");
    break;
  case errReplyLen:
    printf("ERROR: Unexpected reply length.\n");
    break;
  case errReply:
    printf("ERROR: Unexplected reply.\n");
    break;
  default:
    printf("ERROR: Unknown error.\n");
  }
}

static Boolean StartApplication(void) {
  Err err;

  StdInit(frm_Main, Field, Scrollbar, NULL);

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

  FrmGotoForm(frm_Main);
  return true;
}

/* shutdown library communication */
static void StopApplication(void) {
  UInt16 numapps;

  StdFree();

  if(PBrickRef != 0) {
    /* close sensor lib */
    PBrickClose(PBrickRef, &numapps);

    /* Check for errors in the Close() routine */
    if (numapps == 0) {
      SysLibRemove(PBrickRef);
    }
  }
}

static Boolean MainFormHandleEvent(EventPtr event) {
  Boolean handled = false;
  EventType newEvent;
  Err err;
  UInt16 val0, val1;
  PBrickConfigType config;
  static Boolean first=true;

  if(StdHandleEvent(event)) return true;

  switch (event->eType) {

  case ctlSelectEvent:
    SysTaskDelay(10);

    /* send alive check on first try */
    if(first) {
      printf("Alive check: ");
      print_error(PBrickAlive(PBrickRef));
      first = false;
    }

    switch(event->data.ctlEnter.controlID) {
    case btn_Snd0:
    case btn_Snd1:
    case btn_Snd2:
      val0 = 1 + event->data.ctlEnter.controlID - btn_Snd0;
      printf("Play sound %d: ", val0);
      print_error(PBrickSound(PBrickRef, val0));
      break;

    case btn_Firm:
      printf("Get versions: ");
      err = PBrickGetVersions(PBrickRef, &val0, &val1);
      if(err) print_error(err);
      else    printf("\nROM: %x, Firmware: %x\n", val0, val1);
      break;

    case btn_Bat:
      printf("Get battery voltage: ");
      err = PBrickGetBattery(PBrickRef, &val0);
      if(err) print_error(err);
      else    printf("%d.%04dV\n", val0/1000, val0%1000);
      break;    
    }
    handled = true;
    break;
    
  case frmOpenEvent:
    FrmDrawForm(FrmGetActiveForm());

    printf("PBrick Demo V1.0\n");
    printf("(c) 2002 Till Harbaum\n");

    PBrickGetConfig(PBrickRef, &config);
    printf("Always check alive: %s\n", config.check_alive?"on":"off");
    printf("Data rate: %d bit/s\n", config.com_speed);
    printf("Max. retries: %d\n", config.retries);

    printf("Library version: %d.%02d\n", 
	   config.version/100, config.version%100);
    printf("CPU type: %s\n", config.cpu_is_ez?"EZ/VZ":"68328");
    printf("Internal timer: %d ticks\n", config.timer_val);
    printf("Relative error: %d/1000\n", config.error);

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
  UInt16    error;

  do {
    EvtGetEvent(&event, 10);

    if (SysHandleEvent(&event))
      continue;
    if (MenuHandleEvent(NULL, &event, &error))
      continue;
    if (ApplicationHandleEvent(&event))
      continue;

    FrmDispatchEvent(&event);
  }
  while (event.eType != appStopEvent);
}

UInt32	PilotMain(UInt16 cmd, void *cmdPBP, UInt16 launchFlags) {
  if (cmd == sysAppLaunchCmdNormalLaunch) {
    if(StartApplication())
      EventLoop();

    StopApplication();
  }
  return(0);
}









