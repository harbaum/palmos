/*
    explorer - axxPac explorer

    Till Harbaum

    t.harbaum@tu-bs.de
    http://www.ibr.cs.tu-bs.de/
*/

#ifdef OS35HDR
#include <PalmOS.h>
#include <PalmCompatibility.h>
#else
#include <Pilot.h>
#endif

#include "../api/axxPac.h"

#include "explorerRsc.h"
#include "explorer.h"
#include "callback.h"
#include "list_dir.h"
#include "db.h"
#include "backup.h"

WinHandle draw_win;

/* axxPac driver library reference */
UInt LibRef=0;

/* Prototypes */
void card_callback(axxPacCardState);

/* preferences are global */
struct SM_Preferences prefs;

Boolean redraw_free_mem = false;
Boolean redraw_list = false;
int glob_lp;

#ifdef DEBUG_RS232
UInt ComRef;                        /* reference for serial lib */
#endif

static Boolean StartApplication(Boolean alarm, UIntPtr lib) {
  Word error, size, i;
  Err err;
  axxPacLibInfo linfo;
  char *p;

  *lib = 0;

#ifdef DEBUG_RS232
  if(!alarm) {

    /* open rs232 */
    if (SysLibFind("Serial Library", &ComRef)) {
      ComRef = 0;  /* failed ... */
    } else {
      if(err = SerOpen(ComRef, 0, 19200l)) {
	/* failed ... */
	if (err == serErrAlreadyOpen)
	  SerClose(ComRef);
	
	ComRef = 0;
      }
    }
  }

  DEBUG_MSG("axxPac shell debug version\n");
#endif

  /* check if lib already installed */
  err = SysLibFind(AXXPAC_LIB_NAME, lib);

  /* otherwise try to load the lib */
  if(err) {
    err = SysLibLoad('libr', AXXPAC_LIB_CREATOR, lib);
  }

  if(err) {
    *lib = 0;
    FrmCustomAlert(alt_err, "Unable to load axxPac driver!",0,0);
    return false;
  }

  /* try to open lib */
  if(axxPacLibOpen(*lib) != 0) {
    FrmCustomAlert(alt_err, axxPacStrError(*lib), 0, 0);
    return false;
  }

  /* verify library interface revision */
  axxPacGetLibInfo(*lib, &linfo);
  if(linfo.revision < 2) {
    list_error(StrLibVer);
    return false;
  }

  /* more than just backup alarm */
  if(!alarm) {

    /* read preferences */
    size = sizeof(struct SM_Preferences);
    error=PrefGetAppPreferences( CREATOR, 0, &prefs, &size, true);
  
    /* init sprites etc */
    list_initialize();

    /* no or wrong prefs version */
    if((error==noPreferenceFound)||(prefs.version!=PREF_VERSION)) {
      prefs.version = PREF_VERSION;
      prefs.flags = PREF_OVERWRITE | PREF_DELETE | PREF_SHOWROM;
      prefs.view  = LIST_PALM;
      prefs.mode  = SEL_COPY;
      StrCopy(prefs.path,"/");
      
      /* init excludes */
      for(i=0;i<8;i++) {
	prefs.exclude_enable[i>>1][i&1] = false;
	prefs.exclude[i>>1][i&1] = 0;
      }
      
      /* no auto backup */
      prefs.auto_backup = false;
      prefs.backup_time = 0;
    }
    
    /* in new restore mode? */
    if(prefs.flags & PREF_MODE) {
      /* set old settings */
      list_set_disp_mode(prefs.view, false);
      list_set_click_mode(prefs.mode);
      
      /* return to saved path */
      axxPacChangeDir(*lib, prefs.path);
      
      if((long)(p = axxPacGetCwd(*lib))>0)
	StrCopy(saved_path, p);
    }

    /* start main dialog */
    FrmGotoForm(frm_Main);

    /* save fact that we're running the shell */
    FtrSet(CREATOR, 0, 42);
  }

  return true;
}

static void StopApplication(Boolean alarm, UIntPtr lib) {
  UInt numapps;
  char *p;

  if(!alarm) {

    prefs.view  = list_get_disp_mode();
    prefs.mode  = list_get_click_mode();

    /* close driver library */
    if(*lib!=0) {
      if((long)(p=axxPacGetCwd(*lib))<0) StrCopy(prefs.path, "/");
      else                               StrCopy(prefs.path, p);

      /* save preferences */
      PrefSetAppPreferences( CREATOR, 0, 1, &prefs, 
			     sizeof(struct SM_Preferences), true);

      axxPacLibClose(*lib, &numapps);
    
      /* Check for errors in the Close() routine */
      if (numapps == 0) SysLibRemove(*lib);
    }
    
    /* release icons etc */
    list_release();
    
    /* if we are debugging with RS232, close the lib */
#ifdef DEBUG_RS232
    SerClose(ComRef);
#endif
    /* save fact that we're running the shell */
    FtrUnregister(CREATOR, 0);

  } else {
    if(*lib != NULL) {

      /* just close the lib */
      axxPacLibClose(*lib, &numapps);

      /* Check for errors in the Close() routine */
      if (numapps == 0) SysLibRemove(*lib);
    }
  }
}

void *GetObjectPtr(FormPtr frm, Word object) {
  return FrmGetObjectPtr(frm, FrmGetObjectIndex (frm, object));
}

/* there must be a nicer way to do this ... */
/* (CtlSetUsable doesn't work) */
void show(FormPtr frm, Word object) {
  ((FormLabelType*)GetObjectPtr(frm, object))->attr.usable = true;
}

void hide(FormPtr frm, Word object) {
  ((FormLabelType*)GetObjectPtr(frm, object))->attr.usable = false;
}

/* function called by axxPac driver to dispay card state */
#define ICON_LEFT  148  // x position of state icon
axxPacCardState last_state = axxPacCardNone;
void card_callback(axxPacCardState state) {
  RectangleType rect;
  FormPtr frm = FrmGetActiveForm ();

  CALLBACK_PROLOGUE;

  /* redraw free mem if card inserted/removed */
  if(((state <  axxPacCardOk)&&(last_state >= axxPacCardOk))||
     ((state >= axxPacCardOk)&&(last_state <  axxPacCardOk))) {
    redraw_free_mem = true;  /* ask main loop to redraw free memory */
    redraw_list = true;      /* ask main loop to redraw card list */
  }
    
  /* draw icon if draw window active and state changed */
  if((draw_win == WinGetDrawWindow())&&(state != last_state)) {
    if(state!=0) {
      /* draw icon */
      WinDrawBitmap (resP[state-1], ICON_LEFT, 1);
    } else {
      /* no icon */
      rect.topLeft.x = ICON_LEFT; rect.topLeft.y=1; 
      rect.extent.x  = 11;        rect.extent.y=11;
      WinEraseRectangle(&rect,0);
    }

    /* save last output state */
    last_state = state;
  }
  
  CALLBACK_EPILOGUE;
}

#define Y_POS 90
void draw_zone_info(int offset, axxPacCardInfo *info) {
  char str[4][8];
  int i, j, x_pos[]={10, 40, 70, 100};
  RectangleType rect={{10,Y_POS},{130,11*4}};

  FntSetFont(1);  /* bold font */
  WinEraseRectangle(&rect,0);

  /* display zone infos */
  for(i=0;i<4;i++) {
    StrPrintF(str[0], "%d", i+offset);

    if(offset+i<info->zones) {
      StrPrintF(str[1], "%d", info->zone_info[i+offset].free); 
      StrPrintF(str[2], "%d", info->zone_info[i+offset].used); 
      StrPrintF(str[3], "%d", info->zone_info[i+offset].unusable); 
    } else {
      StrCopy(str[1], "-");
      StrCopy(str[2], "-");
      StrCopy(str[3], "-");
    }

    for(j=0;j<4;j++)
      WinDrawChars(str[j], StrLen(str[j]), x_pos[j], Y_POS+11*i);
  }
  FntSetFont(0);  /* normal font */
}

static Boolean InfoFormHandleEvent(EventPtr event) {
  return false;
}

void show_card_info(void) {
  char size_str[16];

  FormPtr frm = FrmInitForm(frm_Card);
  axxPacCardInfo info;
  axxPacLibInfo linfo;
  Boolean quit=false;
  int i, offset=0;
  EventType event;

  axxPacGetLibInfo(LibRef, &linfo);
  CtlSetLabel(GetObjectPtr(frm, CardLibVer), linfo.version);

  /* get low level card data */
  if(axxPacGetCardInfo(LibRef, &info)<0)
    info.inserted = false;

  /* hide all entries */
  for(i=CardNone;i<CardLast;i++) hide(frm, i);

  /* card inserted? */
  if(!info.inserted) {
    show(frm, CardNone);
  } else {
    for(i=CardLblVendor;i<=CardType;i++) show(frm, i);
    
    CtlSetLabel(GetObjectPtr(frm, CardVendor), info.vendor_name);
    CtlSetLabel(GetObjectPtr(frm, CardType),   info.device_name);

    /* valid card? */
    if(info.size != 0) {
      ScrollBarPtr scrollbar = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, CardScl));
      SclSetScrollBar(scrollbar, 0, 0, 4, 4);

      /* show things incl. scrollbar */
      for(i=CardLblSize;i<CardLast;i++) show(frm, i);
      
      /* display card size */
      StrPrintF(size_str, "%d MBytes", info.size );
      CtlSetLabel(GetObjectPtr(frm, CardSize), size_str);
    }
  }    

  FrmSetActiveForm(frm);
  FrmSetEventHandler(frm, InfoFormHandleEvent);
  FrmPopupForm(frm_Card);

  FrmDrawForm(frm);

  if((info.inserted)&&(info.size!=0))
    draw_zone_info(offset, &info);

  do {
    /* exit loop every ten system ticks (about 10Hz) */
    EvtGetEvent(&event, 10);  
    
    if (!SysHandleEvent(&event)) {
      switch (event.eType) {

      case sclRepeatEvent:
	offset = event.data.sclExit.newValue;
	draw_zone_info(offset, &info);
	break;

      case ctlSelectEvent:
	quit = true;
	break;
      }
      FrmDispatchEvent(&event);
    }

    if((event.eType) == appStopEvent) {
      /* put event back into queue */
      EvtAddEventToQueue(&event);
      quit=true;
    }
 } while (!quit);

  FrmReturnToForm(0);

#if 0
  FrmDoDialog(frm);
  FrmDeleteForm(frm);
#endif
};

/* progressbar callback for format */
void format_progress(unsigned char percentage) {
  CALLBACK_PROLOGUE;
  db_progress_bar(false, NULL, percentage, &glob_lp, 0);
  CALLBACK_EPILOGUE;
}

static Boolean PluginMainFormHandleEvent(EventPtr event) {
  return false;
}

static Boolean MainFormHandleEvent(EventPtr event) {
  Boolean handled = false;
  FormPtr frm;
  unsigned char last_flags;
  UInt PluginRef;
  int i,j;
  long plugin_name = 'axP0';
  char str[32];
  ListPtr lst;

  switch (event->eType) {
  case keyDownEvent:

    /* scroll one page up */
    if(event->data.keyDown.chr == pageUpChr) {
      list_scroll(false, -1);
      handled = true;
    }

    /* scroll one page down */
    if(event->data.keyDown.chr == pageDownChr) {
      list_scroll(false, +1);
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

  case ctlSelectEvent:
    switch(event->data.ctlEnter.controlID) {
      
      /* user tapped 'Palm' button */
    case DispPalm:
      list_set_disp_mode(LIST_PALM, true);
      handled =true;
      break;
      
      /* user tapped 'Card' button */
    case DispCard:
      list_set_disp_mode(LIST_CARD, true);
      handled =true;
      break;

#ifdef SELECT_ICON
      /* user tapped 'info', 'copy', 'move' or 'delete' button */
    case SelInfo:
    case SelCopy:
    case SelMove:
    case SelDelete:
      list_set_click_mode(event->data.ctlEnter.controlID - SelInfo);
      handled =true;
      break;
#endif
    }
    
    break;

  case menuEvent:
    switch (event->data.menu.itemID) {

      /* some infos about the program */
    case menuAbout:
#if 0
      {
	LocalID dbId;
	DWord result;
	
	/* delete database if already existant */
	if((dbId = DmFindDatabase(0, "Calculator"))!=0) {
	  SysAppLaunch(0, dbId, 
		       sysAppLaunchFlagNewThread | sysAppLaunchFlagNewStack | 
		       sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp,  
		       sysAppLaunchCmdNormalLaunch, NULL, &result);
	}
      }
#else
      frm = FrmInitForm(frm_About);
      FrmDoDialog(frm);
      FrmDeleteForm(frm);
#endif
      handled = true;
      break;

      /* some infos about the card */
    case menuCard:
      show_card_info();
      handled = true;
      break;

      /* import memo */
    case menuMemoExp:
      memo_export();
      handled = true;
      break;

      /* backup/restore complete palm */
    case menuBackup:
      backup_all(false, LibRef);
      list_dir();
      handled = true;
      break;

    case menuRestore:
      restore_all();
      list_dir();
      handled = true;
      break;

    case menuOptions:
      backup_options();
      handled = true;
      break;

    case menuFormat:
      if(FrmAlert(alt_format)==0) {
	db_progress_bar(true, "formatting card ...", 0, &glob_lp, 0);
	if(axxPacFormat(LibRef, format_progress)<0)
	  FrmCustomAlert(alt_err, axxPacStrError(LibRef),0,0);

	list_dir();
      }
      handled = true;
      break;

      /* plugin configuration */
    case menuPlug:
      for(j=false, i=0; i<PLUGINS; i++)
	if(plugin[i].extension[0] != 0) j=true;
      
      if(j) {
	frm = FrmInitForm(frm_PlugCfg);

	for(i=0;i<PLUGINS;i++) {
	  /* valid plugin entry? */
	  if(plugin[i].extension[0] != 0) {
	    /* change entry */
	    CtlSetLabel(FrmGetObjectPtr(frm, 
		       FrmGetObjectIndex (frm, PlugCfg1Name+i)),
		       plugin[i].name);
	  } else {
	    CtlHideControl(FrmGetObjectPtr(frm, 
		       FrmGetObjectIndex (frm, PlugCfg1+i)));


	  }
	}

	i = FrmDoDialog(frm);
	FrmDeleteForm(frm);

	if((i >= PlugCfg1) && (i <= PlugCfg8)) {
	  i -= PlugCfg1;
	  
	  /* run plugin if existing */
	  if(plugin[i].extension[0] != 0) {
	    plugin_name = (plugin_name & 0xffffff00)|('0'+i); 
	    
	    if(!SysLibLoad('libr', plugin_name, &PluginRef)) {
#ifdef DEBUG_PLUGIN
	      DEBUG_MSG("configuring plugin %d\n", i);
#endif
	      PluginConfig(PluginRef);
	      SysLibRemove(PluginRef);
	    }
	  }
	}
      } else
	FrmAlert(alt_noplug);

      handled = true;
      break;
      
      /* create new directory */
    case menuNewDir:
      frm = FrmInitForm(frm_NewDir);
      FrmSetFocus(frm, FrmGetObjectIndex (frm, NewDirName));
      if(FrmDoDialog(frm) == NewDirOK) {

#ifdef IMPORT_VIA_NEWDIR
	/* files without leading dot are treated like normal dirs */
	if(FldGetTextPtr(FrmGetObjectPtr(
               frm, FrmGetObjectIndex (frm, NewDirName)))[0] != '.') {
#endif
	  if(axxPacNewDir(LibRef, FldGetTextPtr(FrmGetObjectPtr(
			  frm, FrmGetObjectIndex (frm, NewDirName))))<0) {
	    FrmCustomAlert(alt_err, axxPacStrError(LibRef),0,0);
	  }
#ifdef IMPORT_VIA_NEWDIR
	} else {
	  long flen,i;
	  axxPacFD fd;
	  char buffer[512];

	  FILE_OPEN;
	  flen = FILE_LEN;

	  DEBUG_MSG("import %ld bytes\n", flen);

	  fd = axxPacOpen(LibRef, &FldGetTextPtr(FrmGetObjectPtr(
               frm, FrmGetObjectIndex (frm, NewDirName)))[1], O_Creat);

	  while(flen>0) {
	    for(i=0;i<512;i++) buffer[i] = FILE_GET;
	    axxPacWrite(LibRef, fd, buffer, (flen>512)?512:flen);
	    flen -= 512;
	  }
	  axxPacClose(LibRef, fd);
	}
#endif
      }
      
      FrmDeleteForm(frm);
      /* redraw everything */
      list_dir();

      redraw_free_mem = true;  

      handled = true;
      break;

      /* change preferences */
    case menuPref:
      frm = FrmInitForm(frm_Pref);

      CtlSetValue(FrmGetObjectPtr(frm, 
	FrmGetObjectIndex(frm, PrefOverwrite)), prefs.flags & PREF_OVERWRITE);
      CtlSetValue(FrmGetObjectPtr(frm, 
	FrmGetObjectIndex(frm, PrefDelete)), prefs.flags & PREF_DELETE);
      CtlSetValue(FrmGetObjectPtr(frm, 
        FrmGetObjectIndex (frm, PrefROM)), prefs.flags & PREF_SHOWROM);
      CtlSetValue(FrmGetObjectPtr(frm, 
        FrmGetObjectIndex (frm, PrefMode)), prefs.flags & PREF_MODE);

      /* set sort mode */
      lst = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, PrefSortLst));
      LstSetSelection(lst, (prefs.flags & PREF_SORT)>>4);
      CtlSetLabel(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, PrefSortSel)), 
		  LstGetSelectionText(lst, (prefs.flags & PREF_SORT)>>4));

      if(FrmDoDialog(frm) == PrefOK) {
	last_flags = prefs.flags;

	prefs.flags = 
	  (CtlGetValue(FrmGetObjectPtr(frm, 
		FrmGetObjectIndex (frm, PrefOverwrite)))?PREF_OVERWRITE:0) +
	  (CtlGetValue(FrmGetObjectPtr(frm, 
                FrmGetObjectIndex (frm, PrefDelete)))?PREF_DELETE:0) +
	  (CtlGetValue(FrmGetObjectPtr(frm, 
		FrmGetObjectIndex (frm, PrefROM)))?PREF_SHOWROM:0) +
	  (CtlGetValue(FrmGetObjectPtr(frm, 
		FrmGetObjectIndex (frm, PrefMode)))?PREF_MODE:0);

	prefs.flags |= (LstGetSelection(lst)<<4);

	/* the 'show ROM' or 'sort' pref has changed? -> force reload */
	/* ... why the heck doesn't C have a logical xor?? */ 
	if( (last_flags & (PREF_SHOWROM|PREF_SORT)) != 
	    (prefs.flags & (PREF_SHOWROM|PREF_SORT)) ) {
	    
	  /* clear cache */
#ifdef USE_DATABASE_CACHE
	  list_cache_release();
#endif
	  /* force redisplay (in card mode, too :-() */
	  list_dir();
	}
      }
      
      FrmDeleteForm(frm);

      handled = true;
      break;
    }
    break;

  case frmOpenEvent:
    FrmDrawForm(FrmGetActiveForm());
    draw_win = WinGetDrawWindow();    /* save current draw win */

    /* install callback handler */
    axxPacInstallCallback(LibRef, card_callback);

    /* first redraw happens automatically due to card change detection */
    if((list_get_disp_mode()==LIST_PALM)||(last_state!=axxPacCardOk))
      list_dir();
    
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
  int i;
  RectangleType rect;
  DWord free;
  char str[20], *p;

  do {
    /* exit loop every ten system ticks (about 10Hz) */
    EvtGetEvent(&event, 10);  

    if (SysHandleEvent(&event))
      continue;
    if (MenuHandleEvent(NULL, &event, &error))
      continue;
    if (ApplicationHandleEvent(&event))
      continue;

    FrmDispatchEvent(&event);

    if(redraw_free_mem) {
      redraw_free_mem = false;

      rect.topLeft.x = 80; rect.topLeft.y =  2;
      rect.extent.x  = 64; rect.extent.y  = 11;
      WinEraseRectangle(&rect,0); 
    
      if(!axxPacDiskspace(LibRef, &free)) {
	StrPrintF(str, "%ldK free", free);
	WinDrawChars(str, StrLen(str), 144-FntLineWidth(str, StrLen(str)), 2);
      }
    }

    if(redraw_list) {
      /* try to restore last path */
      axxPacChangeDir(LibRef, saved_path);
      if((long)(p = axxPacGetCwd(LibRef))>0)
	StrCopy(saved_path, p);
      
      /* only try to scan for files if card inserted */
      list_refresh(last_state >= axxPacCardOk);
      redraw_list = false;
    }

  } while (event.eType != appStopEvent);

}

/////////////////////////
// PilotMain
/////////////////////////

DWord PilotMain(Word cmd, Ptr cmdBPB, Word launchFlags) {
  Boolean alarm = (cmd == sysAppLaunchCmdAlarmTriggered);
  DWord ftr;
  UInt lib;

  if ((cmd == sysAppLaunchCmdNormalLaunch)||(alarm)) {
    /* if no alarm or application not running */
    if((!alarm) || (FtrGet(CREATOR, 0, &ftr) != 0)) {
      if(StartApplication(alarm, &lib)) {
	if(alarm) {
	  FormPtr frm = FrmInitForm(frm_Auto);
	  FrmDrawForm(frm);

	  /* do automatic backup */
	  backup_all(true, lib);

	  FrmEraseForm(frm);
	  FrmDeleteForm(frm);
	} else {

	  /* save library reference in globals */
	  LibRef = lib;
	  EventLoop();
	}
      }
      StopApplication(alarm, &lib);
    }

    /* re-schedule alarm */
    if(alarm) backup_set_alarm(true);
    else      backup_set_alarm(prefs.auto_backup);
  }

  /* commands that require us to re-schedule an alarm */
  if((cmd == sysAppLaunchCmdSyncNotify)||
     (cmd == sysAppLaunchCmdTimeChange)||
     (cmd == sysAppLaunchCmdSystemReset))
    backup_check_alarm();

  return(0);
}


