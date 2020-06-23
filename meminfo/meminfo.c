/*
    meminfo.c  -  http://www.ibr.cs.tu-bs.de/~harbaum/pilot/meminfo.html

    This code is free!

    Till Harbaum

    t.harbaum@tu-bs.de
    http://www.ibr.cs.tu-bs.de/
*/


#include <Pilot.h>
#include "meminfoRsc.h"

/////////////////////////
// Function Prototypes
/////////////////////////

static void    StartApplication(void);
static Boolean MainFormHandleEvent(EventPtr event);
static Boolean ApplicationHandleEvent(EventPtr event);
static void    EventLoop(void);

/////////////////////////
// StartApplication
/////////////////////////

static void StartApplication(void)
{
  FrmGotoForm(frm_Main);
}

static Boolean MainFormHandleEvent(EventPtr event)
{
  Boolean handled = false;
  char tmp[32];
  long csx, size, base;
  int i,type;
  const char *sstr[]={"Byte", "Kilobyte", "Megabyte" };
  const char *nstr[]={"ROM0, OS-ROM", "RAM0", "ROM1, unused", "RAM1, Palm III only" };
  const char *waits[]={"0 wait-states", "1 wait-state", "2 wait-states", "3 wait-states", 
		       "4 wait-states", "5 wait-states", "6 wait-states", "external dtack"};

  switch (event->eType) {
  case frmOpenEvent:
    FrmDrawForm(FrmGetActiveForm());

#if 1
    for(i=0;i<4;i++) {
      FntSetFont(1);
      StrPrintF(tmp, "CSA%d (%s):", i, nstr[i]);
      WinDrawChars(tmp, StrLen(tmp), 0, 18+35*i);
      FntSetFont(0);

      /* output memory information */
      csx = *(long*)(0xfffff110+4*i);

      base = ((csx&0xff000000)>>8)&(((csx&0xff00)^0xff00)<<8);

      type=0;
      size = (((csx&0xff00)>>8)+1)<<16;

      while(!(size&1023)) { size>>=10; type++; }

      StrPrintF(tmp, "%ld %s%s at 0x%lx", size, sstr[type], (size==1)?"":"s", base);
      WinDrawChars(tmp, StrLen(tmp), 0, 29+35*i);

      StrPrintF(tmp, "%s, read%s, %s bits", waits[csx&0x07], (csx&8)?" only":"/write", (csx&0x10000)?"16":"8");
      WinDrawChars(tmp, StrLen(tmp), 0, 40+35*i);
    }
#else
    for(i=0;i<4;i++) {
      FntSetFont(1);
      csx = *(long*)(0xfffff130+4*i);
      StrPrintF(tmp, "CSC%d:", i);
      WinDrawChars(tmp, StrLen(tmp), 0, 18+35*i);
      FntSetFont(0);

      /* output memory information */
      csx = *(long*)(0xfffff130+4*i);

      if(csx==0x10007) {
	StrCopy(tmp, "not active");
	WinDrawChars(tmp, StrLen(tmp), 0, 29+35*i);
      } else {
	base = ((csx&0xfff00000)>>8)&(((csx&0xfff0)^0xfff0)<<8);
	
	type=0;
	size = (((csx&0xfff0)>>4)+1)<<12;
	
	while(!(size&1023)) { size>>=10; type++; }
	
	StrPrintF(tmp, "%ld %s%s at 0x%lx", size, sstr[type], (size==1)?"":"s", base);
	WinDrawChars(tmp, StrLen(tmp), 0, 29+35*i);
	
	StrPrintF(tmp, "%s, read%s, %s bits", waits[csx&0x07], (csx&8)?" only":"/write", (csx&0x10000)?"16":"8");
	WinDrawChars(tmp, StrLen(tmp), 0, 40+35*i);
      }
    }
#endif

    handled = true;
    break;
  }

  return(handled);
}

/////////////////////////
// ApplicationHandleEvent
/////////////////////////

static Boolean ApplicationHandleEvent(EventPtr event)
{
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

/////////////////////////
// EventLoop
/////////////////////////

static void EventLoop(void)
{
    EventType event;
    Word      error;

    do
    {
        EvtGetEvent(&event,evtWaitForever);
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

/////////////////////////
// PilotMain
/////////////////////////

DWord PilotMain(Word cmd, Ptr cmdBPB, Word launchFlags)
{
    if (cmd == sysAppLaunchCmdNormalLaunch)
    {
        StartApplication();
        EventLoop();
        return(0);
    }
    return(0);
}













