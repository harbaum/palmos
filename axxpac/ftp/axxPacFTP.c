/*

  axxPacFTP.c  - (c) 2000 by Till Harbaum

  ftp client for palmos

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
#include <PalmCompatibility.h>

#include "axxPacFTP.h"
#include "ftpRsc.h"
#include "ftp.h"
#include "filio.h"

static Boolean MainFormHandleEvent(EventPtr event);
static Boolean ApplicationHandleEvent(EventPtr event);
static void    EventLoop(void);
       Boolean DoConnect(void);

struct prefs prefs;

static Boolean StartApplication(void) {
  Word error, size;
  Err err;

  /* open network stuff */
  if(!open_netlib()) return false;

  /* try to access some file system driver */
  if(!fs_driver_open()) return false;

  /* read preferences */
  size  = sizeof(struct prefs);
  error = PrefGetAppPreferences( CREATOR, 0, &prefs, &size, true);
  
  /* no or wrong prefs version */
  if((error == noPreferenceFound)||(prefs.version != PREFS_VERSION)) {
    StrCopy(prefs.server, "localhost");
    StrCopy(prefs.user,   "anonymous");
    StrCopy(prefs.pass,   "your@email.org");
    StrCopy(prefs.path,   "/");
    prefs.close = true;
    prefs.version = PREFS_VERSION;
  }

  if(!DoConnect())
    return false;

  FrmGotoForm(frm_Main);

  return true;
}

static void StopApplication(void) {

  disconnect(false);

  /* finally close file system driver */
  fs_driver_close();

  /* save prefs */
  PrefSetAppPreferences( CREATOR, 0, 1, &prefs, 
			 sizeof(struct prefs), true);

  /* and close net lib */
  close_netlib();
}

VoidHand str2handle(char *str) {
  VoidHand hand;
  char *ptr;

  /* get a new chunk for text */
  if((hand = MemHandleNew(StrLen(str)+1)) != 0) {

    /* fill chunk */
    ptr = MemHandleLock(hand);
    StrCopy(ptr, str);
    MemHandleUnlock(hand);
  }

  return hand;
}

void *GetObjectPtr(FormPtr frm, Word object) {
  return FrmGetObjectPtr(frm, FrmGetObjectIndex (frm, object));
}

Boolean DoConnect(void) {
  FormPtr frm;
  Boolean ret=false;

  frm = FrmInitForm(frm_ServerOpen);

  /* set names */
  FldSetTextHandle(GetObjectPtr(frm, SrvName), (Handle)str2handle(prefs.server));
  FldSetTextHandle(GetObjectPtr(frm, SrvUser), (Handle)str2handle(prefs.user));
  FldSetTextHandle(GetObjectPtr(frm, SrvPass), (Handle)str2handle(prefs.pass));

  CtlSetValue(GetObjectPtr(frm, SrvClose), prefs.close);

  FrmSetFocus(frm, FrmGetObjectIndex (frm, SrvName));
  if(FrmDoDialog(frm) == SrvOK) {

    /* get names */
    StrCopy(prefs.server, FldGetTextPtr(GetObjectPtr(frm, SrvName)));
    StrCopy(prefs.user,   FldGetTextPtr(GetObjectPtr(frm, SrvUser)));
    StrCopy(prefs.pass,   FldGetTextPtr(GetObjectPtr(frm, SrvPass)));

    prefs.close = CtlGetValue(GetObjectPtr(frm, SrvClose));

    /* try to connect to server */
    if(!connect(prefs.server, 21, prefs.user, prefs.pass))
      ret = false;
    else
      ret = true;
  }
  FrmDeleteForm(frm);
  return ret;
}

static Boolean MainFormHandleEvent(EventPtr event) {
  Boolean handled = false;
  EventType newEvent;

  switch (event->eType) {
  case ctlSelectEvent:
    if (event->data.ctlEnter.controlID == btn_Upload) {
      upload();
      list();
      handled = true;
    } else if (event->data.ctlEnter.controlID == btn_Back) {
      change_dir("..");
      list();
      handled = true;
    }
    break;

  case penUpEvent:
    list_select(event->screenX, event->screenY, 0);
    break;

  case penMoveEvent:
    list_select(event->screenX, event->screenY, 1);
    break;

  case penDownEvent:
    list_select(event->screenX, event->screenY, 2);
    break;
    
  case sclRepeatEvent:
    list_scroll(true, event->data.sclExit.newValue);
    break;

  case frmOpenEvent:
    FrmDrawForm(FrmGetActiveForm());
    FrmSetTitle(FrmGetActiveForm(), prefs.server);
    list();
    handled = true;
    break;

  case menuEvent:
    switch (event->data.menu.itemID) {
    case menuAbout:
      FrmAlert(alt_about);
      break;
    }
  }
  return(handled);
}

static Boolean ApplicationHandleEvent(EventPtr event) {
  FormPtr form;
  Word    formId;
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
  Word      error;

  do {
    EvtGetEvent(&event, evtWaitForever);
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

DWord PilotMain(Word cmd, Ptr cmdBPB, Word launchFlags) {
  if (cmd == sysAppLaunchCmdNormalLaunch) {

    if(StartApplication())
      EventLoop();

    StopApplication();
    return(0);
  }
  return(0);
}


