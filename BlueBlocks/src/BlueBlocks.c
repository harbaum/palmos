/*
  BlueTris.c

  BeeCon Bluetooth Tetris
*/

#include <PalmOS.h>
#include "Rsc.h"
#include "debug.h"
#include "graphics.h"
#include "tetris.h"
#include "bt.h"
#include "bt_discovery.h"

static Boolean StartApplication(void);
static void    StopApplication(void);

static Boolean MainFormHandleEvent(EventPtr event);
static Boolean ApplicationHandleEvent(EventPtr event);
static void    EventLoop(void);

static Boolean StartApplication(void) {
  UInt16 wPalmOSVerMajor;
  UInt32 dwVer, newDepth, depth;
  SystemPreferencesType sysPrefs;

  DEBUG_INIT;

  /* check for palmos 3 */
  FtrGet(sysFtrCreator, sysFtrNumROMVersion, &dwVer);
  wPalmOSVerMajor = sysGetROMVerMajor(dwVer);
  if(wPalmOSVerMajor < 3) {
    FrmAlert(alt_wrongOs);
    return false;
  }

  /* switch to appropriate video mode */
  /* if BW, try to enter greyscale */
  WinScreenMode(winScreenModeGet, NULL, NULL, &depth, NULL);
  if(depth == 1) {
    WinScreenMode(winScreenModeGetSupportedDepths, NULL, NULL, &depth, NULL);
    
    if(depth & ((1<<1)|(1<<3))) {

      /* is 4bpp or 2bpp supported? */
      if (depth & (1<<3))     newDepth = 4;
      else if(depth & (1<<1)) newDepth = 2;
      
      WinScreenMode(winScreenModeSet, NULL, NULL, &newDepth, NULL);
    }
  } else {
    if(depth > 8) {
      /* force 8 bit color */
      newDepth = 8;
      WinScreenMode(winScreenModeSet, NULL, NULL, &newDepth, NULL);
    }
  }

  /* Mask hardware keys */
  KeySetMask(~(keyBitHard1 | keyBitHard2 | keyBitHard3 | keyBitHard4 |
	       keyBitLeft | keyBitRight | keyBitCenter));
  
  /* get system preferences for sound */
//  PrefGetPreferences(&sysPrefs);
//  if(sysPrefs.gameSoundLevelV20 == slOff) setup.sound = 0;

  graphics_init();

  /* try to access bluetooth */
  bt_init();

  FrmGotoForm(frm_Main);
  return true;
}

static void    StopApplication(void) {
  bt_free();

  /* unmask keys */
  KeySetMask( keyBitsAll );

  tetris_free();
  graphics_free();

  /* return to default video mode */
  WinScreenMode(winScreenModeSetToDefaults, NULL, NULL, NULL, NULL);

  DEBUG_FREE;
}

static Boolean MainFormHandleEvent(EventPtr event) {
  Boolean handled = false;
  EventType newEvent;

  switch (event->eType) {

  case ctlSelectEvent:
    DEBUG_PRINTF("ctlSelectEvent() " DEBUG_RED 
		 "with controlID = %d" DEBUG_NORM "\n",
		 event->data.ctlEnter.controlID);

    if (event->data.ctlEnter.controlID == btn_Start) {
      DEBUG_PRINTF("clicked btn_Hello\n");
      FrmAlert(alt_ClickMe);
      handled = true;
    }

    if (event->data.ctlEnter.controlID == btn_Join) {
      DEBUG_PRINTF("clicked btn_Join\n");
      bt_discovery_start();
      handled = true;
    }
    break;

  case frmOpenEvent:
    DEBUG_PRINTF("frmOpenEvent\n");
    FrmDrawForm(FrmGetActiveForm());
    tetris_init();
    handled = true;
    break;
    
  case menuEvent:
    DEBUG_PRINTF("menuEvent\n");
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
  Int32     ticks_wait, next_event, i;

  /* initialize Timer */
  ticks_wait = SysTicksPerSecond()/25;
  if(ticks_wait < 1) ticks_wait = 1;
  
  next_event = TimGetTicks() + ticks_wait;

  do {
    i = next_event - TimGetTicks();  // ticks until next event
//    EvtGetEvent(&event, (i<0)?0:i);
    EvtGetEvent(&event, 0);

    if (SysHandleEvent(&event))
      continue;
    if (MenuHandleEvent(NULL, &event, &error))
      continue;
    if (ApplicationHandleEvent(&event))
      continue;

    FrmDispatchEvent(&event);

    /* do some kind of 25Hz timer event */
    if(TimGetTicks() > next_event) {
      next_event = TimGetTicks() + ticks_wait;
      tetris_advance();
    }
  }
  while (event.eType != appStopEvent);

  FrmCloseAllForms();
}

UInt32	PilotMain(UInt16 cmd, void *cmdPBP, UInt16 launchFlags) {
  if (cmd == sysAppLaunchCmdNormalLaunch) {
    if(StartApplication()) {
      EventLoop();
      StopApplication();
    }
  }
  return(0);
}
