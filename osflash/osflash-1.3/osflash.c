/*
    osflash.c  -  http://www.ibr.cs.tu-bs.de/~harbaum/pilot/osflash.html

    This code is free. Written for use with gcc/pilrc.

    DON'T USE THIS PROGRAM IF YOU ARE NOT REALLY SURE ABOUT THE POSSIBLE
    CONSEQUENCES!!!

    Till Harbaum

    t.harbaum@tu-bs.de
    http://www.ibr.cs.tu-bs.de/

    Version 16.6.99 (<Tim Charron> tcharron@interlog.com)
                    - Added detection of Fujitsu MBM29LV160B chip
                    - Added messages indicating chip type, which chip being flashed
					- Added check that all required sectors are unprotected
					  (Done before ANY are written)
					- Added warning when database file appears truncated
    Version 17.6.99 (Till) 
                    - increased image length to 44 blocks and added checking for valid block
                      number in database image
*/


#include <PalmOS.h>
#include <PalmCompatibility.h>
#include <stdarg.h>

#include "osflashRsc.h"
#include "screen.h"
#include "osflash.h"
#include "flash_io.h"

/////////////////////////
// Function Prototypes
/////////////////////////

static int     StartApplication(void);
static Boolean MainFormHandleEvent(EventPtr event);
static Boolean ApplicationHandleEvent(EventPtr event);
static void    EventLoop(void);

/////////////////////////
// StartApplication
/////////////////////////

/* database stuff */
LocalID dbid;
UInt card;
DmOpenRef dbref;

unsigned char *block[OS_BLOCKS];

static int StartApplication(void) {
  DmSearchStateType stateInfo;
  Err err;

  /* find OS database */
  err=DmGetNextDatabaseByTypeCreator(true, &stateInfo, DB_TYPE, CREATOR, true, &card, &dbid);

  if(err)
    FrmCustomAlert(alt_err, "No OS database installed.",0,0);
  else
    FrmGotoForm(frm_Main);

  return !err;
}

int is_protected(long addr, long * protsector) {
	if (flash_sector_checkprotect(addr)) {
		*protsector=addr;
		return 1;
	} else {
		return 0;
	}
}

static Boolean MainFormHandleEvent(EventPtr event)
{
  Boolean handled = false;
  int i,flash_err,flash_count,flash_type;
  long err,addr,protectedsector=0;
  char serial[13];
  char str[64];
  VoidHand record;
  unsigned long checksum=0, truncate_err=0, *checkme,*p;

  switch (event->eType) {
  case frmOpenEvent:
    FrmDrawForm(FrmGetActiveForm());

    /* generate checksum over everything except */
    /* serial number sector (which is at address */
    /* 0x6000 to 0x7fff */

    for(checkme=(unsigned long*)FLASHBASE, 
	  addr=0;addr<(0x6000/4);addr++) 
      checksum ^= *checkme++;

    for(checkme=(unsigned long*)(FLASHBASE+0x8000), 
	  addr=(0x8000/4);addr<(OS_BLOCKS*BLOCK_LEN)/4;addr++)
      checksum ^= *checkme++;
    
    /* output flash content checksum */
    StrPrintF(str,"flash checksum = %08lx", checksum);
    WinDrawChars(str, StrLen(str), 10, 110);

    /* generate database checksum */
    /* open the database */
    dbref = DmOpenDatabase(card, dbid, dmModeReadOnly);

    /* check number of blocks */
    if(DmNumRecords(dbref) != OS_BLOCKS) {
      FrmCustomAlert(alt_err, "OS Database contains illegal number of records (not an OS image or image from prior osflash version?)",0,0);
    }

    /* get addresses of all records in the OS database */
    for(checksum=0,i=0;i<OS_BLOCKS;i++) {
      record = DmGetRecord(dbref, i);
      checkme = p = MemHandleLock(record);

      /* don't sum serial number block */
      if(i==0)
		for(addr=0;addr<6144;addr++) checksum ^= *checkme++;
      if ((i>0) && (i < OS_BLOCKS-1))
		for(addr=0;addr<8192;addr++) checksum ^= *checkme++;
      if (i == OS_BLOCKS-1) {
		  for(addr=0;addr<8192;addr++) {
			  /* Check if the last 4k of the final 32k block are all 0xFF*/
			  /* Assume that if they aren't, then this image is a truncated one*/
			  if (addr>=7168)
				if (0xFFFFFFFF != *checkme) truncate_err++;
			  checksum ^= *checkme++;
		  }
	  }

      MemPtrUnlock(p);
      DmReleaseRecord(dbref, i, false);    
    } 
    DmCloseDatabase(dbref);

    /* output database content checksum */
    StrPrintF(str,"database checksum = %08lx", checksum);
    WinDrawChars(str, StrLen(str), 10, 121);

    if (truncate_err)
		FrmCustomAlert(alt_err, "Warning: database may be truncated!",0,0);

    handled = true;
    break;

  case ctlSelectEvent:
    switch(event->data.ctlEnter.controlID) {
    case ButFlash:
      flash_type=flash_is_present();
      if (flash_type) {
	if (!is_protected(0x0000,&protectedsector)) {
	  if (!is_protected(0x2000,&protectedsector)) {
	    if (!is_protected(0x4000,&protectedsector)) {
	      for(addr=0x08000 ; addr<(OS_BLOCKS*0x8000) ; addr+=0x08000)
		if (is_protected(addr,&protectedsector)) {
		  break;
		}
	    }
	  }
	}
      }

      if(flash_type && !protectedsector) {
	if(FrmAlert(alt_Flash)==1) {
	  scr_clear();

	  /* open the database */
	  dbref = DmOpenDatabase(card, dbid, dmModeReadOnly);

	  /* get addresses of all records in the OS database */
	  for(i=0;i<OS_BLOCKS;i++) {
	    record = DmGetRecord(dbref, i);
	    block[i] = MemHandleLock(record);
	  }
	  
	  if (flash_type==1) {
          scr_draw_string("Found AMD chip.    \n");
      } else {
          scr_draw_string("Found Fujitsu chip.\n");
      }
#ifndef SECOND_FLASH
	  scr_draw_string("Flashing PalmOS ...\n");
#else
	  scr_draw_string("Flashing 2nd chip..\n");
#endif

	  flash_open();

	  /* start address */
	  addr = 0x0; /* 8 blocks = 512k */
	  for(i=0;i<OS_BLOCKS/2;i++) {

	    if(addr==0) {

	      /********************/
	      /* erase 64k sector */
	      /********************/

	      flash_err = 0;    /* no error for now */
	      flash_count = 5;  /* max retries */

	      do {
		/* erase three boot blocks (skip the one with serial number) */
		if(flash_sector_erase(0)!=0)        { scr_draw_char('0'); flash_err=1; }
		if(flash_sector_erase(0x2000)!=0)   { scr_draw_char('1'); flash_err=1; }
		if(flash_sector_erase(0x4000)!=0)   { scr_draw_char('3'); flash_err=1; }

		if(!flash_err) flash_count=0;
		else           flash_count--;

	      } while(flash_count>0);

	      /********************/
	      /* write 24k sector */
	      /********************/

	      flash_err = 0;    /* no error for now */
	      flash_count = 5;  /* max retries */

	      do {
		/* flash two blocks (24k + 32k total) at once */
		if(flash_write((unsigned short*)block[2*i], addr, 12288)!=0)
		  scr_draw_char('w');

		if(flash_verify((unsigned short*)block[2*i], addr, 12288)==0) {
		  scr_draw_char('v');
		  flash_err=1;
		}
		if(!flash_err) flash_count=0;
		else           flash_count--;

	      } while(flash_count>0);
		
	      /********************/
	      /* write 32k sector */
	      /********************/

	      flash_err = 0;    /* no error for now */
	      flash_count = 5;  /* max retries */

	      do {
		if(flash_write((unsigned short*)block[2*i+1], addr+0x4000, 16384)!=0)
		  scr_draw_char('W');

		if(flash_verify((unsigned short*)block[2*i+1], addr+0x4000, 16384)==0) {
		  scr_draw_char('V');
		  flash_err=1;
		}

		if(!flash_err) flash_count=0;
		else           flash_count--;

	      } while(flash_count>0);

	      /* stop flashing if verify failed */
	      if(flash_err) {
		scr_draw_string("verify failed\nstopping flash operation\nplease press reset\n");
		for(;;);
	      }
	      
	    } else {
	      /********************/
	      /* erase 64k sector */
	      /********************/

	      flash_err = 0;    /* no error for now */
	      flash_count = 5;  /* max retries */

	      do {
		/* erase block */
		if(flash_sector_erase(addr)!=0) {
		  scr_draw_char('E');
		  flash_err =1;
		}
		if(!flash_err) flash_count=0;
		else           flash_count--;

	      } while(flash_count>0);

	      /********************/
	      /* write 32k sector */
	      /********************/

	      flash_err = 0;    /* no error for now */
	      flash_count = 5;  /* max retries */

	      do {

		/* flash two blocks (64k total) at once */
		if(flash_write((unsigned short*)block[2*i], addr, 16384)!=0)
		  scr_draw_char('w');

		if(flash_verify((unsigned short*)block[2*i], addr, 16384)==0) {
		  scr_draw_char('v');
		  flash_err=1;
		}
		
		if(!flash_err) flash_count=0;
		else           flash_count--;
		
	      } while(flash_count>0);
	      
	      /********************/
	      /* write 32k sector */
	      /********************/

	      flash_err = 0;    /* no error for now */
	      flash_count = 5;  /* max retries */

	      do {
		if(flash_write((unsigned short*)block[2*i+1], addr+0x4000, 16384)!=0)
		  scr_draw_char('W');

		if(flash_verify((unsigned short*)block[2*i+1], addr+0x4000, 16384)==0) {
		  scr_draw_char('V');
		  flash_err=1;
		}

		if(!flash_err) flash_count=0;
		else           flash_count--;

	      } while(flash_count>0);

	      /* stop flashing if verify failed */
	      if(flash_err) {
		scr_draw_string("verify failed\nstopping flash operation\nplease press reset\n");
		for(;;);
	      }
	    }

	    scr_draw_char('*');

	    /* next block */
	    addr += 0x8000;
	  }

	  /* erase remaining blocks */
	  for(;i<32;i++) {
	    if(flash_sector_erase(addr)!=0)
	      scr_draw_char('E');

	    scr_draw_char('*');

	    /* next block */
	    addr += 0x8000;
	  }

	  scr_draw_string("\nfinished!\n\n");
	  
	  /* halt or unlock all records */
#ifndef SECOND_FLASH
	  scr_draw_string("Please press the\nreset button now!");
	  while(1);
#else
	  flash_close(); // enables IRQs and doesn't work with first flash

	  for(i=0;i<OS_BLOCKS;i++) {
	    MemPtrUnlock(block[i]);
	    DmReleaseRecord(dbref, i, false);    
	  }
	  DmCloseDatabase(dbref);
#endif
	}
      } else {
            if (!protectedsector) {
			  FrmCustomAlert(alt_err, "No Am29LV160BB/MBM29LV160B flash found!",0,0);
		  } else {
			  StrPrintF(str,"Unexpected protected sector at %08lx!", 2*protectedsector);
			  FrmCustomAlert(alt_err, str,0,0);
		  }
      }
      handled=true;
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
        if(StartApplication())
	  EventLoop();
        return(0);
    }
    return(0);
}

