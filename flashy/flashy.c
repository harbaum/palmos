/*
    flashy.c  -  http://www.ibr.cs.tu-bs.de/~harbaum/pilot/flashy.html

    This code is free. Written for use with gcc/pilrc.

    BE VERY, VERY, VERY CAREFUL WITH THIS. YOU MIGHT ERASE THE OPERATING SYSTEM
    OF YOUR PALM CAUSING IT TO BE UNRECOVERABLE DEAD!!!!!

    DON'T USE THIS PROGRAM IF YOU ARE NOT REALLY SURE ABOUT THE POSSIBLE
    CONSEQUENCES!!!

    Till Harbaum

    t.harbaum@tu-bs.de
    http://www.ibr.cs.tu-bs.de/
*/


#include <Pilot.h>
#include <stdarg.h>
#include "flashyRsc.h"
#include "flash_io.h"

/* base address, we write some test words to (far behind the OS) */
#define TEST_ADDRESS 0xf0000

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

char info_buffer[256];
unsigned short data_buffer[8192];

static void StartApplication(void)
{
  FrmGotoForm(frm_Main);
}

int pos;
#define PRINTF(format, args...) { char tmp[80]; StrPrintF(tmp, format, ## args); \
WinDrawChars(tmp, StrLen(tmp), 2, pos); pos+=11; }

void clear_screen(void) { 
  RectangleType rect;   

  pos=12;

  rect.topLeft.x=2; rect.topLeft.y=pos; 
  rect.extent.x=156; rect.extent.y=120;
  WinEraseRectangle(&rect,0);
}

void show_info(char *buffer) {
  /* output some information */
  clear_screen();

  PRINTF("CFI id (should be QRY): '%c%c%c'", 
	 buffer[0x10], buffer[0x11], buffer[0x12]); 
  PRINTF("Vcc min: %d.%d V / max: %d.%d V", 
	 buffer[0x1b]/16, buffer[0x1b]%16, buffer[0x1c]/16, buffer[0x1c]%16);
  PRINTF("Device size: %ld bytes", 1l<<buffer[0x27]  );
  PRINTF("typ. timeout for single write: %d us", 1<<buffer[0x1f]);
  PRINTF("typ. to. for block erase: %d us", 1<<buffer[0x21]);
  PRINTF("max. to. for single write: %d times", 1<<buffer[0x23]);
  PRINTF("max. to. for block erase: %d times", 1<<buffer[0x25]);
  PRINTF("pr. vendor id (should be PRI): '%c%c%c'", 
	 buffer[0x40], buffer[0x41], buffer[0x42]); 
  PRINTF("Version number: %c.%c", buffer[0x43], buffer[0x44]); 
  PRINTF("Last used word in flash: $%lx", flash_last_word_used());
}

static Boolean MainFormHandleEvent(EventPtr event)
{
  Boolean handled = false;
  int i;
  long err;

  switch (event->eType) {
  case frmOpenEvent:
    FrmDrawForm(FrmGetActiveForm());
    clear_screen();
    PRINTF("If you are not sure");
    PRINTF("what this program does:");
    PRINTF("EXIT IMMEDIATELY WITHOUT");
    PRINTF("TOUCHING ANY BUTTONS!");
    PRINTF(" ");

    if(flash_is_present()) {
      PRINTF("AM29LV160BB flash rom detected");

      PRINTF(" ");
      PRINTF("checking unused flash area ...");

      /* OS goes to 0x9ffff, followed by 0x60000 */
      /* words (768kBytes) empty space */

      if(!flash_is_empty(0xa0000, 0x60000)) {
	PRINTF("WARNING:");
	PRINTF("user space is not empty");
      } else {
	PRINTF("user space is empty");
	PRINTF("768kBytes flash available");
      }
    } else {
      PRINTF("ERROR:");
      PRINTF("No AM29LV160BB flash rom found!!");
    }

    handled = true;
    break;

  case ctlSelectEvent:
    switch(event->data.ctlEnter.controlID) {
    case ButInfo:
      if(flash_is_present()) {
	flash_get_info(info_buffer);
	show_info(info_buffer);
      } else {
	clear_screen();
	PRINTF("no flash info available"); 
      }
      handled=true;
      break;  

    case ButWrite:
      clear_screen();
      if(flash_is_present()) {
	PRINTF("writing to word offset $%lx ...", TEST_ADDRESS);

	data_buffer[0]=0x1234;
	data_buffer[1]=0x5678;
	data_buffer[2]=0xaaaa;
	data_buffer[3]=0x5555;
	err=flash_write(data_buffer, TEST_ADDRESS, 8192);
	
	if(err==0) {
	  PRINTF("write successful");
	  
	  PRINTF("verifying ...");
	  if(flash_verify(data_buffer, TEST_ADDRESS, 8192)) {
	    PRINTF("verify successful");
	  } else {
	    PRINTF("verify failed");
	  }
	} else {
	  PRINTF("write error at word offset $%x", err); 
	}
      } else {
	PRINTF("write not possible without flash rom");
      }
      handled=true;
      break;

    case ButRead:
      clear_screen();
      flash_read(data_buffer, TEST_ADDRESS, 16);
      PRINTF("rom contents at word offset:");
      for(i=0;i<8;i++) PRINTF("$%lx: $%lx", TEST_ADDRESS + i, (long)data_buffer[i]);
      handled=true;
      break;

    case ButErase:
      clear_screen();

      if(flash_is_present()) {
	PRINTF("erasing sector at $%lx ...", TEST_ADDRESS);

	if((i=flash_sector_erase(TEST_ADDRESS))==0) {
	  PRINTF("erase successful");

	  PRINTF("blankchecking sector ...");

	  /* 1 sector = 64kBytes = 32kWords */
	  if(flash_is_empty(TEST_ADDRESS, 32768)) {  
	    PRINTF("blankcheck successful");
	  } else {
	    PRINTF("blankcheck failed");
	  }
	} else {
	  PRINTF("error %d during erase operation", i);
	}
      } else {
	PRINTF("erase not possible without flash rom");
      }
      break;
    }
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













