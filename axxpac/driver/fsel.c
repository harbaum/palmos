/*

  fsel.c - axxPac file selector

  (c) 2000 by Till Harbaum

*/

#ifdef OS35HDR
#include <PalmOS.h>
#include <PalmCompatibility.h>
#else
#include <Pilot.h>
#include <System/SysEvtMgr.h>
#endif

#include "axxPacDrv.h"
#include "../api/axxPac.h"

#include "smartmedia.h"
#include "errors.h"

#include "fsel.h"

#define noDEBUG_FSEL

#ifdef DEBUG_FSEL
#define DEBUG_MSG_FSEL(args...) DEBUG_MSG(##args)
#else
#define DEBUG_MSG_FSEL(args...)
#endif

#define LINES 8

struct dir_entry {
  char name[38];
  unsigned char attr;
};

/* get locals from heap instead the small stack */
struct fsel_local {
  char path[AXXPAC_FSEL_BUFFERSIZE];
  char name[AXXPAC_FSEL_BUFFERSIZE];
  struct dir_entry *files;
  int file_num, offset;
};

void *fsel_GetObjectPtr(FormPtr frm, Word object) {
  return FrmGetObjectPtr(frm, FrmGetObjectIndex (frm, object));
}

/* scan given dir for files */
int fsel_scan(Lib_globals *gl, char *path, struct dir_entry *entry) {
  /* this creates a axxPacFileInfo without the space for 'RESERVED' */
  short buffer[(sizeof(axxPacFileInfo)-RESERVED_SIZE+1)/2];
  char *base;
  int cluster;
  Err err;
  find_state state;
  int cnt=0;

  /* preprocess path */
  if((err = fs_process_path(gl, path, &cluster, &base))<0) return err;

  DEBUG_MSG_FSEL("scan cluster %d (%s)\n", cluster, base);

  /* search for entry */
  if(err = fs_findfirst(gl, cluster, base, &state, 
			(axxPacFileInfo*)buffer, FIND_FILES_N_ALL_DIRS, 0))
    if(err == -ErrFsNotFound) return 0;

  while(!err) {
    if(entry != NULL) {
      StrNCopy(entry[cnt].name, ((axxPacFileInfo*)buffer)->name, 37);
      entry[cnt].attr = ((axxPacFileInfo*)buffer)->attrib;
    }

    cnt++;
    err = fs_findnext(gl, base, &state, (axxPacFileInfo*)buffer, 0);
  }
  
  if(err != -ErrFsNotFound) return err;
  return cnt;
}

/* write length limited string followed by dots at pos */
/* identical to code in explorer */
static void 
win_draw_chars_len(int x, int y, SWord max, char *str) {
  SWord lenp;
  Boolean fit;
  const char dots[]="...";

  lenp = StrLen(str);
  fit = true;
  FntCharsInWidth(str, &max, &lenp, &fit);

  /* something cut off? */
  if(lenp != StrLen(str)) {
    max = max-FntLineWidth((char*)dots,3);
    FntCharsInWidth(str, &max, &lenp, &fit);
    WinDrawChars(str, lenp, x, y);
    WinDrawChars((char*)dots, 3, x+max, y);
  } else
    WinDrawChars(str, lenp, x, y);
}

void draw_dir(FormPtr frm, struct dir_entry *entry, int i, int offset) {
  Handle    resH;
  BitmapPtr resP;
  RectangleType rect1 = {{11,31},{127,LINES*11}};
  RectangleType rect2 = {{10,30},{136,LINES*11}};
  int k,j = (i>LINES)?LINES:i;

  /* erase file area */
  WinEraseRectangle(&rect1, 0);

  /* draw rectangle around file area */
  WinDrawRectangleFrame(1, &rect2);

  /* get and lock folder icon */
  resH = (Handle)DmGetResource(bitmapRsc, rsc_folder_bmp);
  resP = MemHandleLock((VoidHand)resH);

  for(k=0;k<j;k++) {
    if(entry[k+offset].attr & FA_SUBDIR) WinDrawBitmap (resP, 12, 30 + k*11);
    win_draw_chars_len(22, 30+11*k, 120, entry[k+offset].name);

    /* enable button for this entry */
    CtlSetEnabled(fsel_GetObjectPtr(frm, rsc_name0 + k), true);
  }

  /* disable remaining buttons */
  while(k<LINES) {
    /* enable button for this entry */
    CtlSetEnabled(fsel_GetObjectPtr(frm, rsc_name0 + k++), false);
  }

  MemPtrUnlock(resP);
  DmReleaseResource((VoidHand) resH);
}

/* used for systems sort function for directory entries */
static Int 
compare_entry(VoidPtr object1, VoidPtr object2, Long other) {
  struct dir_entry *d1=object1, *d2=object2;
  
  /* both same type (DIR or FILE?) */
  if(((d1->attr & FA_SUBDIR)&&(d2->attr & FA_SUBDIR))||
     ((!(d1->attr & FA_SUBDIR))&&(!(d2->attr & FA_SUBDIR))))
    return StrCaselessCompare(d1->name, d2->name);

  if(d1->attr & FA_SUBDIR)
    return -1;

  return 1;
}

Err rescan_dir(Lib_globals *gl, FormPtr frm, struct fsel_local *loc) {
  int i;
  ScrollBarPtr bar=fsel_GetObjectPtr(frm, rsc_scrollbar);

  if(loc->files != NULL) {
    DEBUG_MSG_MEM("free: %lx\n", loc->files);

    MemPtrFree(loc->files);
    loc->files = NULL;
  }

  /* draw dir */
  loc->file_num = i = fsel_scan(gl, loc->path, NULL);
  if(i>0) {
    loc->files = MemPtrNew(i*sizeof(struct dir_entry));
    if(loc->files == NULL) return -666;

    DEBUG_MSG_MEM("alloc fsel %ld = %lx\n", i*sizeof(struct dir_entry), loc->files);

    fsel_scan(gl, loc->path, loc->files);

    SysQSort( (Byte*)loc->files, i, 
	      sizeof(struct dir_entry), compare_entry, 0);

    /* update scrollbar */
    if(i>LINES) SclSetScrollBar(bar, 0, 0, i-LINES, LINES-1);
    else        SclSetScrollBar(bar, 0, 0, 0, 0);

    /* and draw them ... */
    loc->offset = 0;
    draw_dir(frm, loc->files, i, 0);
  } else {
    SclSetScrollBar(bar, 0, 0, 0, 0);
    draw_dir(frm, loc->files, 0, 0);
  }
  return i;
}

static Boolean MainFormHandleEvent(EventPtr event) {
  return false;
}

Err fsel(Lib_globals *gl, char *title, char *file, 
	 char *pattern, Boolean editable) {

  FormPtr frm;
  FieldPtr fld;
  LocalID dbId;
  DmOpenRef dbRef=0;
  VoidHand hand;
  char *ptr;
  struct fsel_local *loc;
  Err err=ErrNone;
  FieldAttrType attr;
  Boolean quit=false;
  EventType event;
  int i;

  /* get memory for path and friends */
  if(!(loc = (struct fsel_local*)MemPtrNew(sizeof(struct fsel_local)))) {
    error(gl, false, ErrFsel);
    err = -ErrFsel;
    goto fs_quit;
  }

  DEBUG_MSG_MEM("alloc fsel %ld = %lx\n", sizeof(struct fsel_local), loc);

  /* currently no dir in buffer */
  loc->files = NULL;

  /* see whether name contains path */
  if((ptr = fs_strrchr(file, DELIMITER))!=NULL) {
    /* get base file name address */
    StrCopy(loc->name, ptr+1);

    /* and extract path name */
    StrCopy(loc->path, file);
    *(fs_strrchr(loc->path, DELIMITER)+1) = 0;
  } else {
    /* copy whole name */
    StrCopy(loc->name, file);

    /* get current directory */
    ptr = fs_getcwd(gl);
    if((long)ptr < 0) {
      err = (long)ptr;
      DEBUG_MSG_FSEL("error getting current dir\n");
      goto fs_quit;
    }

    StrCopy(loc->path, ptr);
  }

  /* attach pattern string to path */
  StrCat(loc->path, pattern);

  /* try to parse this path */
  if((err = fs_process_path(gl, loc->path, &i, &ptr))<0) {
    /* on error try root dir */
    StrCopy(loc->path, DELIMITER_STR);
    StrCat(loc->path, pattern);

    /* and erase file name as well */
    loc->name[0]=0;

    /* and try once more */
    if((err = fs_process_path(gl, loc->path, &i, &ptr))<0)
      goto fs_quit;
  }

  DEBUG_MSG_FSEL("dir: '%s'\n", loc->path);
  DEBUG_MSG_FSEL("name: '%s'\n", loc->name);

  /* find lib and open it to get access to resources */
  if(((dbId = DmFindDatabase(0, "axxPac Driver Lib"))==0)||
     ((dbRef = DmOpenDatabase(0, dbId, dmModeReadOnly))==0)) {
    error(gl, false, ErrFsel);
    err = -ErrFsel;
    goto fs_quit;
  }

  frm = FrmInitForm(rsc_fsel);
  FrmSetTitle(frm, title);

  FrmSetActiveForm(frm);
  FrmSetEventHandler(frm, MainFormHandleEvent);
  FrmPopupForm(rsc_fsel);

  /* set path */
  hand = MemHandleNew(StrLen(loc->path)+1);
  ptr = MemHandleLock(hand);
  StrCopy(ptr, loc->path);
  MemPtrUnlock(ptr);
  fld = fsel_GetObjectPtr(frm, rsc_path);
  FldSetTextHandle(fld, (Handle) hand);

  /* set name */
  hand = MemHandleNew(StrLen(loc->name)+1);
  ptr = MemHandleLock(hand);
  StrCopy(ptr, loc->name);
  MemPtrUnlock(ptr);
  fld = fsel_GetObjectPtr(frm, rsc_name);
  FldSetTextHandle(fld, (Handle) hand);

  FrmDrawForm(frm);

  /* if editable set selector on name else disable name */
  if(editable)
    FrmSetFocus(frm, FrmGetObjectIndex (frm, rsc_name));
  else {
    FldGetAttributes(fsel_GetObjectPtr(frm, rsc_name), &attr);
    attr.editable = false;
    FldSetAttributes(fsel_GetObjectPtr(frm, rsc_name), &attr);
  }

  /* draw dir */
  if(rescan_dir(gl, frm, loc)==-666) {
    /* out of memory */
    error(gl, false, ErrFsel);
    err = -ErrFsel;
    goto fs_quit;
  }

  err = 0;

  do {
    /* exit loop every ten system ticks (about 10Hz) */
    EvtGetEvent(&event, 10);  

    if (!SysHandleEvent(&event)) {
      switch (event.eType) {

      case sclRepeatEvent:
	loc->offset = event.data.sclExit.newValue;
	draw_dir(frm, loc->files, loc->file_num, loc->offset);
	break;
	
      case ctlSelectEvent:
	switch(event.data.ctlEnter.controlID) {
	case rsc_name0:
	case rsc_name1:
	case rsc_name2:
	case rsc_name3:
	case rsc_name4:
	case rsc_name5:
	case rsc_name6:
	case rsc_name7:
	  i = event.data.ctlEnter.controlID - rsc_name0 + loc->offset; 
	  /* clicked on file? */
	  if(!(loc->files[i].attr & FA_SUBDIR)) {
	    /* re-set name */
	    fld = fsel_GetObjectPtr(frm, rsc_name);
	    hand = MemHandleNew(StrLen(loc->files[i].name)+1);
	    ptr = MemHandleLock(hand);
	    StrCopy(ptr, loc->files[i].name);
	    MemPtrUnlock(ptr);
	    FldSetTextHandle(fld, (Handle) hand);
	    FldDrawField(fld);
	    break;
	  } else {
	    /* ignore '.' dir */
	    if(StrCompare(loc->files[i].name, ".") == 0)
	      break;

	    DEBUG_MSG_FSEL("clicked dir '%s'\n", loc->files[i].name);

	    /* one dir up */
	    if(StrCompare(loc->files[i].name, "..") == 0) {
	      /* remove pattern */
	      *(fs_strrchr(loc->path, DELIMITER)) = 0;
	      /* remove last path part */
	      *(fs_strrchr(loc->path, DELIMITER)+1) = 0;
	      /* re-attach pattern */
	      StrCat(loc->path, pattern);
	    } else {
	      /* remove pattern */
	      *(fs_strrchr(loc->path, DELIMITER)+1) = 0;
	      /* attach new dir */
	      StrCat(loc->path, loc->files[i].name);
	      /* and attach new slash */
	      StrCat(loc->path, DELIMITER_STR);
	      /* re-attach pattern */
	      StrCat(loc->path, pattern);
	    }
	  }

	case rsc_reload:

	  quit = false;

	  do {
	    /* try to access media */
	    if((err = fs_prepare_access(gl, false))>=0) {
	      /* try to use this path */
	      if((err = fs_process_path(gl, loc->path, &i, &ptr))<0) {
		/* on error try root dir */
		StrCopy(loc->path, DELIMITER_STR);
		StrCat(loc->path, pattern);
		
		/* and erase file name as well */
		loc->name[0]=0;
		
		/* and try once more */
		err = fs_process_path(gl, loc->path, &i, &ptr);
	      }
	    }    
	    
	    /* something went really wrong */
	    if(err<0) {
	      if(FrmCustomAlert(rsc_error, error_string(gl),0,0)==1) 
		quit = true;
	    } else
	      quit = true;

	  } while(!quit);

	  quit = false;

	  /* everything's fine, just continue */
	  if(err>=0) {
	    /* set new path */
	    fld = fsel_GetObjectPtr(frm, rsc_path);
	    hand = MemHandleNew(StrLen(loc->path)+1);
	    ptr = MemHandleLock(hand);
	    StrCopy(ptr, loc->path);
	    MemPtrUnlock(ptr);
	    FldSetTextHandle(fld, (Handle) hand);
	    FldDrawField(fld);

	    if(!editable) {
	      /* erase current file name */
	      fld = fsel_GetObjectPtr(frm, rsc_name);
	      FldSetTextHandle(fld, (Handle)0);
	      FldDrawField(fld);
	    }

	    /* and rescan directory */
	    if((err = rescan_dir(gl, frm, loc))<0) {
	      DEBUG_MSG_FSEL("error scanning dir -> exit\n");
		
	      /* out of memory */
	      if(err == -666) err = -ErrFsel;

	      /* exit on error */
	      quit = true;
	    }
	  } else {
	    /* error has been reported */
	    err = ErrNone;
	    quit = true;
	  }
	  break;
	  
	case rsc_ok:
	  DEBUG_MSG_FSEL("clicked ok\n");
	  /* remove pattern from path */
	  *(fs_strrchr(loc->path, DELIMITER)+1) = 0;
	  /* and attach file name */
	  fld = fsel_GetObjectPtr(frm, rsc_name);
	  if((ptr = FldGetTextPtr(fld)) != NULL) 
	    StrCat(loc->path, ptr);

	  /* and copy whole name to file */
	  StrCopy(file, loc->path);

	  DEBUG_MSG_FSEL("selected %s\n", file);	  

	  err = 1;
	  quit = true;
	  break;

	case rsc_cancel:
	  err = 0;
	  quit = true;
	  break;
	}
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

fs_quit:
  if(dbRef != 0) DmCloseDatabase(dbRef);

  if(loc->files != NULL) {    
    DEBUG_MSG_MEM("free fsel: %lx\n", loc->files);
    MemPtrFree(loc->files);
  }

  if(loc != 0)   {
    DEBUG_MSG_MEM("free fsel: %lx\n", loc);
    MemPtrFree(loc);
  }

  return err;
}



