/*

  fsel.c  - (c) 2000 by Till Harbaum

  file selector 

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
#include <VFSMgr.h>
#include <ExpansionMgr.h>

#include "axxPacFTP.h"
#include "ftpRsc.h"
#include "fselRsc.h"
#include "ftp.h"

/*************************************************/
/* fileselector, since PalmOS does not have one  */

#define LINES 8
#define DELIMITER '/'
#define DELIMITER_STR "/"

struct dir_entry {
  /* directory entry */
  char name[32];
  unsigned char attr;
};

/* volume entries */
static UInt16 vol_nums;
static UInt16 *volume=NULL;

/* get locals from heap instead the small stack */
static char path[256];
static char name[256];
static struct dir_entry *files;
static int file_num, offset;

void *fsel_GetObjectPtr(FormPtr frm, Word object) {
  return FrmGetObjectPtr(frm, FrmGetObjectIndex (frm, object));
}

UInt16 volRefNum=-1;

void ALERT_NUM(char *str, int num) {
  char lstr[64];
  StrPrintF(lstr, "%s %d", str, num);
  FrmCustomAlert(alt_err, lstr, 0,0);
}

int fsel_enumerate(void) {
  UInt16 RefNum, i, vol_cnt;
  UInt32 volIterator = vfsIteratorStart;
  VolumeInfoType volInfo;
  char str[40], name[32];

  /* scan number of card vols */
  vol_cnt = 0; while(volIterator != vfsIteratorStop)
    if((VFSVolumeEnumerate(&RefNum, &volIterator))==errNone)
      vol_cnt++;

  /* allocate file buffer */
  volume = MemPtrNew(vol_cnt * sizeof(UInt16));
  if(volume == NULL) return -1;  

  /* create card list */
  volIterator = vfsIteratorStart;
  for(i=0;i<vol_cnt;i++) {
    if((VFSVolumeEnumerate(&RefNum, &volIterator))==errNone) {
      /* get volume info */
      if(VFSVolumeInfo(RefNum, &volInfo) == errNone) {
	/* get volume name */
	if(VFSVolumeGetLabel(RefNum, name, 31) == errNone) {
	  volume[i] = RefNum;
#if 0
	  StrPrintF(str, "Vol %d: %s", RefNum, name);
	  FrmCustomAlert(alt_err, str,0,0);
#endif
	}
      }
    }
  }
  return vol_cnt;
}

/* scan given dir for files */
int fsel_scan(char *path, struct dir_entry *entry) {
  FileRef dirRef;
  FileInfoType info;
  UInt32 dirIterator;
  int cnt;

  /* free buffers */
  if(files != NULL) {
    MemPtrFree(files);
    files = NULL;
  }

  if(VFSFileOpen(volRefNum, path, vfsModeRead, &dirRef) != errNone) {
    FrmCustomAlert(alt_err, "Unable to open directory!",0,0);
    return 0;
  }

  /* empty buffer */
  info.nameP = NULL;
  info.nameBufLen = 0;

  /* scan number of files ... */
  cnt = 0; dirIterator = vfsIteratorStart;
  while(dirIterator != vfsIteratorStop) {
    if(VFSDirEntryEnumerate(dirRef, &dirIterator, &info)==errNone)
      cnt++;
  }

  /* add 'up' dir */
  if(StrCompare(path, DELIMITER_STR) != 0) cnt++;

  files = MemPtrNew(cnt * sizeof(struct dir_entry));
  if(files == NULL) return 0;

  /* add 'up' dir */
  cnt = 0;
  if(StrCompare(path, DELIMITER_STR) != 0) {
    StrCopy(files[cnt].name, "..");
    files[cnt].attr = vfsFileAttrDirectory;
    cnt++;
  }

  /* and fetch info */
  dirIterator = vfsIteratorStart;
  while(dirIterator != vfsIteratorStop) {
    info.nameP = files[cnt].name;
    info.nameBufLen = 31;

    if(VFSDirEntryEnumerate(dirRef, &dirIterator, &info)==errNone) {
      files[cnt].attr = info.attributes;
      cnt++;
    }
  }

  /* close dir */
  VFSFileClose(dirRef);

  file_num = cnt;
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
  int k, j = (i>LINES)?LINES:i;

  /* erase file area */
  WinEraseRectangle(&rect1, 0);

  /* draw rectangle around file area */
  WinDrawRectangleFrame(1, &rect2);

  /* get and lock icons */
  resH = (Handle)DmGetResource(bitmapRsc, IconFolder);
  resP = MemHandleLock((VoidHand)resH);

  for(k=0;k<j;k++) {
    if(entry[k+offset].attr & vfsFileAttrDirectory) 
      WinDrawBitmap (resP, 12, 30 + k*11);

    win_draw_chars_len(22, 30+11*k, 120, entry[k+offset].name);

    /* enable this entry for valid cards and files */
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
  if(((d1->attr & vfsFileAttrDirectory)&&(d2->attr & vfsFileAttrDirectory))||
     ((!(d1->attr & vfsFileAttrDirectory))&&
      (!(d2->attr & vfsFileAttrDirectory))))

    return StrCaselessCompare(d1->name, d2->name);

  if(d1->attr & vfsFileAttrDirectory)
    return -1;

  return 1;
}

Err rescan_dir(FormPtr frm) {
  int i;
  ScrollBarPtr bar=fsel_GetObjectPtr(frm, rsc_scrollbar);

  /* scan for cards */
  if((i = fsel_scan(path, files)) > 0) {

    /* sort entries */
    SysQSort( (Byte*)files, i, 
	      sizeof(struct dir_entry), compare_entry, 0);

    /* update scrollbar */
    if(i>LINES) SclSetScrollBar(bar, 0, 0, i-LINES, LINES-1);
    else        SclSetScrollBar(bar, 0, 0, 0, 0);

    /* and draw them ... */
    offset = 0;
    draw_dir(frm, files, i, 0);
  } else {
    SclSetScrollBar(bar, 0, 0, 0, 0);
    draw_dir(frm, files, 0, 0);
  }
  return i;
}

/* replacement for missing strrchr in PalmOS */
char *fs_strrchr(char *str, char chr) {
  char* ret=NULL;

  /* scan whole string */
  while(*str) {
    if(*str==chr) ret=str;
    str++;
  }
  return ret;
}

static Boolean MainFormHandleEvent(EventPtr event) {
  return false;
}

Err fs_file_selector(char *title, char *file, Boolean editable) {

  FormPtr frm;
  FieldPtr fld;
  LocalID dbId;
  VoidHand hand;
  char *ptr;
  Err err=false;
  FieldAttrType attr;
  Boolean quit=false;
  EventType event;
  int i, vol;

  /* check cards */
  vol = fsel_enumerate();
  if(vol<1) return false;
  
  /* use first as default card */
  volRefNum = volume[0];

  /* see whether name contains path */
  if((ptr = fs_strrchr(file, DELIMITER))!=NULL) {
    /* get base file name address */
    StrCopy(name, ptr+1);

    /* and extract path name */
    StrCopy(path, file);
    *(fs_strrchr(path, DELIMITER)+1) = 0;
  } else {
    /* palmos does not have something like the "current slot" */
    /* if no path was give go to the slot select mode */
    StrCopy(path, "");
    StrCopy(name, file);

    /* get default dir ... */
  }

  /* name now contains the file name */
  /* path now contains the current path */

  frm = FrmInitForm(rsc_fsel);
  FrmSetTitle(frm, title);

  FrmSetActiveForm(frm);
  FrmSetEventHandler(frm, MainFormHandleEvent);
  FrmPopupForm(rsc_fsel);

  /* set path */
  hand = MemHandleNew(StrLen(path)+1);
  ptr = MemHandleLock(hand);
  StrCopy(ptr, path);
  MemPtrUnlock(ptr);
  fld = fsel_GetObjectPtr(frm, rsc_path);
  FldSetTextHandle(fld, (Handle) hand);

  /* set name */
  hand = MemHandleNew(StrLen(name)+1);
  ptr = MemHandleLock(hand);
  StrCopy(ptr, name);
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
  if(rescan_dir(frm)==-666) {
    /* out of memory */
    err = -1;
    goto fs_quit;
  }

  err = 0;

  do {
    /* exit loop every ten system ticks (about 10Hz) */
    EvtGetEvent(&event, 10);  

    if (!SysHandleEvent(&event)) {
      switch (event.eType) {

      case sclRepeatEvent:
	offset = event.data.sclExit.newValue;
	draw_dir(frm, files, file_num, offset);
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
	  i = event.data.ctlEnter.controlID - rsc_name0 + offset; 
	  /* clicked on file? */
	  if(!(files[i].attr & vfsFileAttrDirectory)) {
	    /* re-set name */
	    fld = fsel_GetObjectPtr(frm, rsc_name);
	    hand = MemHandleNew(StrLen(files[i].name)+1);
	    ptr = MemHandleLock(hand);
	    StrCopy(ptr, files[i].name);
	    MemPtrUnlock(ptr);
	    FldSetTextHandle(fld, (Handle) hand);
	    FldDrawField(fld);
	    break;
	  } else {
	    /* ignore '.' dir */
	    if(StrCompare(files[i].name, ".") == 0)
	      break;

	    /* one dir up */
	    if(StrCompare(files[i].name, "..") == 0) {
	      /* remove last path part */
	      path[StrLen(path)-1]=0;
	      *(fs_strrchr(path, DELIMITER)+1) = 0;
	    } else {
	      /* remove pattern */
	      *(fs_strrchr(path, DELIMITER)+1) = 0;
	      /* attach new dir */
	      StrCat(path, files[i].name);
	      /* and attach new slash */
	      StrCat(path, DELIMITER_STR);
	    }
	  }

#if 0
	case rsc_reload:
#endif

	  quit = false;
	  
	  /* everything's fine, just continue */
	  if(err>=0) {
	    /* set new path */
	    fld = fsel_GetObjectPtr(frm, rsc_path);
	    hand = MemHandleNew(StrLen(path)+1);
	    ptr = MemHandleLock(hand);
	    StrCopy(ptr, path);
	    MemPtrUnlock(ptr);
	    FldSetTextHandle(fld, (Handle) hand);
	    FldDrawField(fld);
	      
	    /* erase current file name */
	    fld = fsel_GetObjectPtr(frm, rsc_name);
	    FldSetTextHandle(fld, (Handle)0);
	    FldDrawField(fld);
	      
	    /* and rescan directory */
	    if((err = rescan_dir(frm))<0) {
		
	      /* out of memory */
	      if(err == -666) err = -1;

	      /* exit on error */
	      quit = true;
	    }
	  } else {
	    /* error has been reported */
	    err = false;
	    quit = true;
	  }
	  break;

	case rsc_ok:
	  /* remove slash from path */
	  if(path[StrLen(path)-1] == DELIMITER)
	    path[StrLen(path)-1] = DELIMITER;

	  /* and attach file name */
	  fld = fsel_GetObjectPtr(frm, rsc_name);
	  if((ptr = FldGetTextPtr(fld)) != NULL) 
	    StrCat(path, ptr);

	  /* and copy whole name to file */
	  StrCopy(file, path);

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

  /* free file buffer */
  if(files  != NULL) MemPtrFree(files);
  if(volume != NULL) MemPtrFree(volume);

  return err;
}
