/*
    backup.c - axxPac explorer backup/restore functions

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

#define CARD 0     /* only work with card 0 */
#define BACKUPDIR  "/backup"

// #define PERIOD   120  // backup every two minutes
#define PERIOD   86400  // daily backup

#if PERIOD != 86400
#ifndef DEBUG
#error Not daily backup!
#endif
#endif

/* check if alarm is enabled and set it */
void backup_check_alarm(void) {
  struct SM_Preferences lprefs;
  Word i, error;

  /* load prefs */
  i = sizeof(struct SM_Preferences);
  error = PrefGetAppPreferences( CREATOR, 0, &lprefs, &i, true);

  /* no or wrong prefs version */
  if((error == noPreferenceFound)||(lprefs.version != PREF_VERSION))
    return;

  /* and set/disable the alarm */
  backup_set_alarm(lprefs.auto_backup);
}

void backup_set_alarm(Boolean set) {
  LocalID dbId;
  ULong alarm;

  /* find the axxPac shell */
  dbId = DmFindDatabase(CARD, "axxPac");
  if(dbId != 0) {

    if(set) {
      /* seconds until midnight */
      alarm =  TimGetSeconds();
      alarm += (PERIOD - (alarm%PERIOD));
    } else
      alarm = 0;
    
    AlmSetAlarm(CARD, dbId, 0, alarm, 0);
  }
}

void backup_options(void) {
  int i;
  VoidHand hand[8];
  FieldPtr fld;
  char *ptr;
  DateTimeType date;
  char date_str[longDateStrLength + timeStringLength + 5];
  SystemPreferencesType sysPrefs;

  FormPtr frm = FrmInitForm(frm_Options);

  if(prefs.backup_time != 0) {
    /* get system preferences for date format */
    PrefGetPreferences(&sysPrefs);

    /* set date and time */
    TimSecondsToDateTime(prefs.backup_time, &date);
    DateToAscii(date.month, date.day, date.year, 
		sysPrefs.longDateFormat, date_str);
    StrCat(date_str, "     ");
    TimeToAscii(date.hour, date.minute, sysPrefs.timeFormat, &date_str[StrLen(date_str)]);
    CtlSetLabel(GetObjectPtr(frm, OptionsTime), date_str);
  }

  /* set auto backup */
  if(prefs.auto_backup)
    CtlSetValue(FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,OptionsAuto)),1);  

  /* setup options from prefs */
  for(i=0;i<8;i++) {

    /* get a new chunk for text */
    if((hand[i] = MemHandleNew(5)) == 0) {
      list_error(StrMemBuf);
      return;
    }

    /* fill in name */
    ptr = MemHandleLock(hand[i]);
    MemMove(ptr, &prefs.exclude[i>>1][i&1],4); ptr[4]=0;
    MemHandleUnlock(hand[i]);

    fld = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, OptionsCreat0+2*i));
    FldSetTextHandle(fld, (Handle)hand[i]);

    if(prefs.exclude_enable[i>>1][i&1])
      CtlSetValue(FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,OptionsECreat0+2*i)),1);
  }

  if(FrmDoDialog(frm) == OptionsOK) {
    for(i=0;i<8;i++) {
    
      ptr = FldGetTextPtr(FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, OptionsCreat0+2*i)));
      if(ptr != 0) MemMove(&prefs.exclude[i>>1][i&1], ptr, 4);

      /* read values */
      prefs.exclude_enable[i>>1][i&1] = CtlGetValue(FrmGetObjectPtr(frm, 
				FrmGetObjectIndex(frm,OptionsECreat0+2*i)));
    }

    /* get auto backup */
    prefs.auto_backup =
      CtlGetValue(FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,OptionsAuto)));  

    /* start/stop timer */
    backup_set_alarm(prefs.auto_backup);
  }
  
  FrmDeleteForm(frm);
}

static void draw_progress_box(Boolean draw_border) {
  RectangleType rect;

  if(draw_border) {
    /* draw progress box */
    rect.topLeft.x = 80-(140/2);
    rect.topLeft.y = 80-(65/2);
    rect.extent.x = 140;
    rect.extent.y = 65;
  
    WinEraseRectangle(&rect, 3);
    WinDrawRectangleFrame(dialogFrame, &rect);

    /* draw total bar */
    rect.topLeft.x = 80-(BAR_WIDTH/2)+10;
    rect.topLeft.y = 80-(65/2)+26;
  } else {
    /* draw total bar */
    rect.topLeft.x = 70-(BAR_WIDTH/2)+10;
    rect.topLeft.y = 10+26;
  }

  rect.extent.x = BAR_WIDTH-20;
  rect.extent.y = 11;
  WinDrawRectangleFrame(rectangleFrame, &rect);
}

static void progress_total(int percent, Boolean alarm) {
  RectangleType rect;

  if(!alarm) {
    rect.topLeft.x = 80-(BAR_WIDTH/2)+11;
    rect.topLeft.y = 80-(65/2)+27;
  } else {
    rect.topLeft.x = 70-(BAR_WIDTH/2)+11;
    rect.topLeft.y = 10+27;
  }

  rect.extent.x = (percent*(BAR_WIDTH-22))/100;
  rect.extent.y = 9;
  WinDrawRectangle(&rect, 0);
}

static void draw_filename(char *name, Boolean alarm) {
  RectangleType rect;
  SWord width, len;
  Boolean fit;

  /* erase/draw file names */
  width = 120; len = StrLen(name);
  FntCharsInWidth(name, &width, &len, &fit);

  if(!alarm) {
    rect.topLeft.x = 20; rect.topLeft.y = 80-(65/2)+9;
  } else {
    rect.topLeft.x = 10; rect.topLeft.y = 10+9;
  }

  rect.extent.x = 120; rect.extent.y = 12;
  WinEraseRectangle(&rect, 0);

  if(!alarm)
    WinDrawChars(name, len, 80-(width/2), 80-(65/2)+9);
  else
    WinDrawChars(name, len, 70-(width/2), 10+9);
}

/* backup all db's to the smartmedia card into a folder named backup */

/* options:                */
/* - destination directory */
/* - read ROM files        */
/* - remove old files      */
/* - update all/newer      */

Boolean backup_all(Boolean alarm, UInt lib) {
  unsigned int total, db_num;
  LocalID dbId;
  UInt attrib, len;
  char name[2][40], *p;
  ULong date, creator, type, length;
  char backupdir[]=BACKUPDIR;
  axxPacFD fd;
  struct db_header head;
  Err err;
  axxPacFileInfo info;
  Word i, error;
  Boolean do_backup;
  Boolean result = false;
  int progress_pos;
  struct SM_Preferences *lprefs;

  if(!alarm) {
    progress_pos = 1;

    /* confirm backup (with options?) */
    if(FrmAlert(alt_backup)==1)
      return false;

    lprefs = &prefs;  /* use prefs already loaded */
  } else {
    progress_pos = 2;

    /* check whether a card is inserted */    
    if(axxPacGetCardInfo(lib, (axxPacCardInfo*)name) < 0)
      return false;

    if(!((axxPacCardInfo*)name)->inserted)
      return false;
  }

  /* try to enter backup dir */
  if(axxPacChangeDir(lib, backupdir) < 0) {
    if(axxPacChangeDir(lib, "/")<0) {
      if(!alarm) FrmCustomAlert(alt_err, axxPacStrError(lib),0,0);
      return false;
    }

    if(axxPacNewDir(lib, backupdir+1) < 0) {
      if(!alarm) FrmCustomAlert(alt_err, axxPacStrError(lib),0,0);
      return false;
    }

    if(axxPacChangeDir(lib, backupdir) < 0) {
      if(!alarm) FrmCustomAlert(alt_err, axxPacStrError(lib),0,0);
      return false;
    }
  }

  if(alarm) {
    /* alloc space for prefs */
    lprefs = MemPtrNew(sizeof(struct SM_Preferences));
    if(lprefs == 0) return false;

    /* load prefs */
    i = sizeof(struct SM_Preferences);
    error = PrefGetAppPreferences( CREATOR, 0, lprefs, &i, true);

    /* no or wrong prefs version */
    if((error == noPreferenceFound)||(lprefs->version != PREF_VERSION))
      goto backup_quit;
  }

  /* ok, we are now in the backup directory */
  total = DmNumDatabases(CARD);

  draw_progress_box(!alarm);

  /* backup all databases */
  for(db_num=0;db_num<total;db_num++) {
    progress_total((db_num * 100l)/total, alarm);

    dbId = DmGetDatabase(CARD, db_num); 

    /* get size of database */
    DmDatabaseSize(CARD, dbId, NULL, &length, NULL); 

    /* get name of database */
    DmDatabaseInfo(CARD, dbId, name[0], &attrib, NULL, NULL, 
		   &date, NULL, NULL, NULL, NULL, &type, &creator);

    draw_filename(name[0], alarm);

    do_backup = true;

    for(i=0;i<4;i++) {
      /* verify creator */
      if((lprefs->exclude_enable[i][0])&&(!lprefs->exclude_enable[i][1]))
	if(creator == lprefs->exclude[i][0])
	  do_backup = false;
      
      /* verify type */
      if((lprefs->exclude_enable[i][1])&&(!lprefs->exclude_enable[i][0]))
	if(type == lprefs->exclude[i][1])
	  do_backup = false;
      
      /* verify type and creator */
      if((lprefs->exclude_enable[i][1])&&(lprefs->exclude_enable[i][0]))
	if((type == lprefs->exclude[i][1])&&(creator == lprefs->exclude[i][0]))
	  do_backup = false;
    }

    /* don't backup system files (restore crashes palm, at least psysLnchDB) */
    /* only psys files to backup are saved und unsaved prefs (type 'pref' and 'sprf') */
    if((creator == 'psys')&&(type != 'pref')&&(type != 'sprf')) do_backup = false;

    /* and don't backup axxPac */
    if((creator == CREATOR)||(creator == 'axPC')) do_backup = false;

    /* don't backup ROM files */ 
    if((!(attrib & dmHdrAttrReadOnly))&&(do_backup)) {

      /* check whether file already exists */
      StrCopy(name[1], name[0]);
      if(attrib & dmHdrAttrResDB) StrCat(name[1], ".prc");
      else                        StrCat(name[1], ".pdb");
      
      if(axxPacExists(lib, name[1]) == 0) {
	/* try to extract modification date from file */
	fd = axxPacOpen(lib, name[1], O_RdOnly);
	len = axxPacRead(lib, fd, &head, sizeof(struct db_header)); 
	axxPacClose(lib, fd);

	if(len != sizeof(struct db_header)) {
	  /* error loading header, file possibly truncated */
	  /* overwrite it */
	  if(!db_write(lib, name[0], length, progress_pos, true, alarm))
	    goto backup_quit;
	} else {
	  /* palm database is newer */
	  if(date >  head.modificationDate)
	    if(!db_write(lib, name[0], length, progress_pos, true, alarm))
	      goto backup_quit;
	}
      } else {
	/* and write the db */
	if(!db_write(lib, name[0], length, progress_pos, false, alarm))
	  goto backup_quit;
      }
    }
  }

  /* remove unused backup files */
  draw_filename("cleaning up", alarm);

  /* scan through all files and try to find the appropriate databases */
  err = axxPacFindFirst(lib, "*.p??", &info, FIND_FILES_ONLY);
  while(!err) {

    /* open db and read real name */
    fd = axxPacOpen(lib, info.name, O_RdOnly);
    axxPacRead(lib, fd, name[0], 32);
    name[0][32] = 0;
    axxPacClose(lib, fd);

    /* database not installed on palm? remove from card! */
    if((dbId = DmFindDatabase(CARD, name[0]))==0) {
      axxPacDelete(lib, info.name);
    }
    err = axxPacFindNext(lib, &info);
  }

  /* save date of successful backup */
  lprefs->backup_time = TimGetSeconds();

  result = true;
backup_quit:

  if(alarm) {
    /* write prefs of alarm backup was successful */
    if(result)
      PrefSetAppPreferences( CREATOR, 0, 1, lprefs, 
			     sizeof(struct SM_Preferences), true);
    
    MemPtrFree(lprefs);
  }

  /* restore working dir */
  axxPacChangeDir(lib, "/");

  if(!alarm) {
    /* free mem on card may have changed */
    redraw_free_mem = true;
  }

  return result;
}

void restore_all(void) {
  Err err;
  long total, db_num;
  char name[36], *p;
  char backupdir[]=BACKUPDIR;
  axxPacFileInfo info;

  /* confirm restore (with options?) */
  if(FrmAlert(alt_restore)==1)
    return;

  if(axxPacChangeDir(LibRef, backupdir) < 0) {
    FrmCustomAlert(alt_err, "Backup directory '/backup' not found!",0,0);
    return;
  }

#ifdef USE_DATABASE_CACHE
  list_cache_release();
#endif

  /* count files for progress bar */
  total = 0; db_num=0;
  err = axxPacFindFirst(LibRef, "*.p??", &info, FIND_FILES_ONLY);
  while(!err) {
    total++;
    err = axxPacFindNext(LibRef, &info);
  }

  DEBUG_MSG("found %ld files\n", total);

  draw_progress_box(true);
  
  /* scan through all files and try to find the appropriate databases */
  err = axxPacFindFirst(LibRef, "*", &info, FIND_FILES_ONLY);
  while(!err) {
    progress_total((db_num * 100l)/total, false);
    db_num++;

    /* cut file extension */
    StrCopy(name, info.name); p = my_strrchr(name, '.'); if(p!=NULL) *p=0;

    draw_filename(name, false);

    /* restore database */
    db_read(info.name, info.size, 1, true);

    err = axxPacFindNext(LibRef, &info);
  }

  /* restore working dir */
  axxPacChangeDir(LibRef, "/");
}

