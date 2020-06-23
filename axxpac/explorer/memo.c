/*
  memo.c - memopad interface routines for axxPac

  (c) 1999 Till Harbaum

  install texts into PalmOS memo pad

  ToDo:
  + memo crashes sometimes while working on our memos (fixed)
  + view memo before installing it (done)
  + export memo to smartmedia

*/

#ifdef OS35HDR
#include <PalmOS.h>
#include <PalmCompatibility.h>
#else
#include <Pilot.h>
#endif

#include "../api/axxPac.h"
#include "explorerRsc.h"

#include "db.h"
#include "list_dir.h"
#include "explorer.h"
#include "memo.h"

#define VIEW_LINES         11

static void set_scrollbar(FormPtr form, FieldPtr field)
{
  ScrollBarPtr scrollbar;
  Word scroll_pos, text_height, field_height, max_value;

  scrollbar = FrmGetObjectPtr(form,
	FrmGetObjectIndex(form, ViewScrollbar));

  FldGetScrollValues(field, &scroll_pos, &text_height, &field_height);
  if (text_height > field_height) {
    max_value = text_height - field_height;
  } else if (scroll_pos > 0) {
    max_value = scroll_pos;
  } else {
    max_value = 0;
  }
  SclSetScrollBar(scrollbar, scroll_pos, 0, max_value, field_height);
}

static void scroll_field(int delta, Boolean update_scrollbar)
{
  FormPtr form;
  FieldPtr field;

  form = FrmGetActiveForm();
  field = FrmGetObjectPtr(form, FrmGetObjectIndex(form, ViewField));
  
  if (delta < 0)
    FldScrollField(field, - delta, up);
  else
    FldScrollField(field, delta, down);
  
  if (update_scrollbar)
    set_scrollbar(form, field);
}

static Boolean handle_read_form_event(EventPtr event) {
  Boolean handled = false;

  switch (event->eType) {
  case keyDownEvent:
    if (event->data.keyDown.chr == pageUpChr) {
      scroll_field(- VIEW_LINES + 1, true);
      handled = true;
    } else if (event->data.keyDown.chr == pageDownChr) {
      scroll_field(VIEW_LINES - 1, true);
      handled = true;
    }
    break;
    
  case fldChangedEvent:
    scroll_field(0, true);
    handled = true;
    break;
    
  case sclRepeatEvent:
    scroll_field(event->data.sclRepeat.newValue - 
		 event->data.sclRepeat.value, false);
    break;
  default:
    break;
  }
  return handled;
}

/* View memo before installing it */
void memo_import(char *name, long flen) {
  LocalID dbId;
  DmOpenRef dbRef;
  FormPtr frm;
  FieldPtr fld;
  VoidHand hand;
  char  *ptr, *data, *dst;
  int  i;
  long len;
  VoidHand recordH;
  UInt record=0;
  axxPacFD fd;

  /* get a new chunk */
  if((hand = MemHandleNew(flen + 1)) == 0) {
    list_error(StrMemBuf);
    return;
  }

  /* open file */
  if((fd = axxPacOpen(LibRef, name, O_RdOnly))<0) {
    MemHandleFree(hand);   
    FrmCustomAlert(alt_err, axxPacStrError(LibRef), 0, 0);
    return;
  }

  ptr = MemHandleLock(hand);

  /* read file */
  if(axxPacRead(LibRef, fd, ptr, flen) != flen) {
    MemHandleFree(hand);   
    FrmCustomAlert(alt_err, axxPacStrError(LibRef), 0, 0);
    return;
  }

  /* and close file */
  axxPacClose(LibRef, fd);

  /* remove returns */
  for(dst=data=ptr, i=0; i<flen; i++, data++)
    if(*data != 13) *dst++ = *data;

  /* mark last byte */
  *dst++ = 0;

  frm = FrmInitForm(frm_View);
  FrmSetTitle(frm, name);
  fld = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, ViewField));
  FldSetTextHandle(fld, (Handle) hand);
  set_scrollbar(frm, fld);
  FrmSetEventHandler(frm, handle_read_form_event);
  i=FrmDoDialog(frm);

  /* user wants to install memo */
  if(i==ViewInst) {

    /* install ascii text file into memo pad application */
    len = dst-ptr;    /* length of memo */

    /* find memo database */
    if((dbId = DmFindDatabase(0, MEMODB_NAME))==0) {
      list_error(StrFindDB);
      return;
    }
    
    /* and open it */
    if((dbRef = DmOpenDatabase(0, dbId, dmModeReadWrite))==0) {
      list_error(StrOpenDB);
      return;
    }

    /* create new record */
    if((recordH = DmNewRecord(dbRef, &record, len + StrLen(name) + 2))==NULL) {
      list_error(StrMemRec);
      goto memo_exit;
    }
    
    /* lock resource */
    data = MemHandleLock(recordH);
    
    /* finally copy data */
    DmWrite(data, 0, name, StrLen(name));           /* Name (memo title) */
    DmWrite(data, StrLen(name), "\012", 1);         /* new line */
    DmWrite(data, StrLen(name)+1, ptr, len);        /* data */
    
    /* unlock resource */
    MemPtrUnlock(data);   
    
    /* release record */
    DmReleaseRecord(dbRef, record, true); 
    
    /* installation successful */    
memo_exit:
    DmCloseDatabase(dbRef);
  }

  /* free memory */
  MemPtrUnlock(ptr);   

  /* this frees the textbuffer, too */
  FrmDeleteForm(frm);
}

static Boolean MemoFormHandleEvent(EventPtr event) {
  return false;
}

void memo_WinDrawCharsRight(char *str, int x, int y) {
  WinDrawChars(str, StrLen(str), x-FntLineWidth(str,StrLen(str)), y);
}

static void memo_list(FormPtr frm, struct memo_info *minfo, 
		      int i, int offset) {
  RectangleType rect1 = {{11,21},{127,XLINES*11}};
  RectangleType rect2 = {{10,20},{136,XLINES*11}};
  int k,j = (i>XLINES)?XLINES:i;
  char num[4];

  /* erase file area */
  WinEraseRectangle(&rect1, 0);

  /* draw rectangle around file area */
  WinDrawRectangleFrame(1, &rect2);

  for(k=0;k<j;k++) {
    StrPrintF(num, "%d.", 1+k+offset);
    memo_WinDrawCharsRight(num, 12+NUMWID, 20+11*k);
    win_draw_chars_len(12+NUMWID+4, 20+11*k, 127-LENWID-NUMWID-4, 
		       minfo[k+offset].name);
    memo_WinDrawCharsRight(minfo[k+offset].len, 12+124, 20+11*k);

    /* enable button for this entry */
    CtlSetEnabled(GetObjectPtr(frm, MemoName0 + k), true);
  }

  /* disable remaining buttons */
  while(k<XLINES) {
    /* enable button for this entry */
    CtlSetEnabled(GetObjectPtr(frm, MemoName0 + k++), false);
  }
}

/* get info on all installed memos */
int memo_scan(DmOpenRef dbRef, struct memo_info **minfo) {
  int memos,i,j,memo_cnt=0;
  VoidHand hand;
  char *data;

  if(*minfo != NULL) {
    MemPtrFree(*minfo);
    *minfo = NULL;
  }

  /* get first line (title) of all memos */
  if((memos = DmNumRecords(dbRef))!=0) {
    *minfo = (struct memo_info*)MemPtrNew(sizeof(struct memo_info) * memos);
    if(*minfo == NULL) {
      list_error(StrMemBuf);
      return -1;
    }

    for(memo_cnt=0,i=0;i<memos;i++) {
      if((hand = DmGetRecord(dbRef, i)) != NULL) {
	memo_cnt++;

	data = MemHandleLock(hand);

	(*minfo)[i].record = i;

	/* save record length */
	(*minfo)[i].size = MemPtrSize(data);
	length2ascii((*minfo)[i].len, (*minfo)[i].size);

	/* copy name */
	for(j=0;(j<MAX_MEMO_NAME)&&(data[j]!='\n')&&
	      (data[j]!=0)&&(j<(*minfo)[i].size);j++)
	  (*minfo)[i].name[j] = data[j];

	/* terminate name */
	(*minfo)[i].name[j]=0;
	
	MemPtrUnlock(data);
	DmReleaseRecord(dbRef, i, false);
      }
    }
  }

  return memo_cnt;
}

static void memo_write(DmOpenRef dbRef, int no) {
  VoidHand hand;
  char *data, name[AXXPAC_FSEL_BUFFERSIZE];
  int len,j,i;
  axxPacErr err;
  axxPacFD fd;
  axxPacStatType stat;

  /* read memo from memopad and write it onto the smartmedia */

  /* access memo contents */
  hand = DmGetRecord(dbRef, no);
  data = MemHandleLock(hand);

  /* save record length */
  len = MemPtrSize(data);

  /* copy name */
  for(j=0;(j<MAX_MEMO_NAME)&&(data[j]!='\n')&&
	(data[j]!=0)&&(j<len);j++)
    name[j] = data[j];

  /* terminate name */
  name[j]=0;

  /* check whether this looks like a real file name */
  /* (one of the last characters except the last character is a dot) */
  if((name[j-1]=='.')||(StrChr(name+j-5,'.')==NULL))
    StrCat(name, ".txt");

  /* open file selector */
  err = axxPacFsel(LibRef, "Export Memo", name, "*", true);

  if(err<0) {
    FrmCustomAlert(alt_err, axxPacStrError(LibRef),0,0);
  } else if(err > 0) {
    if(!axxPacStat(LibRef, name, &stat)) {

      /* ask for permission? */
      if(prefs.flags & PREF_OVERWRITE) {
	if(FrmCustomAlert(alt_overwrite, name, 0,0)==1) {
	  goto write_quit;
	}
      }
      axxPacDelete(LibRef, name);
    }

    /* open file */
    if((fd = axxPacOpen(LibRef, name, O_Creat))<0) {
      FrmCustomAlert(alt_err, axxPacStrError(LibRef), 0, 0);
    } else {
      /* skip return after first line */
      if(data[j] == '\n') j++;

      while(j<len) {
	/* copy data and re-insert returns */
	i=0;
	while((j<len)&&(i<(AXXPAC_FSEL_BUFFERSIZE-1))) {
	  if(data[j] == 10) name[i++] = 13;
	  name[i++] = data[j++];
	}
	
	/* write data to smartmedia */
	if(axxPacWrite(LibRef, fd, name, i)<0) {
	  FrmCustomAlert(alt_err, axxPacStrError(LibRef), 0, 0);
	  j = len;
	}
      }
      axxPacClose(LibRef, fd);
    }
  }

 write_quit:
  MemPtrUnlock(data);
  DmReleaseRecord(dbRef, no, false);
}

void memo_export(void) {
  FormPtr frm;
  Boolean quit=false;
  EventType event;
  int offset=0, memos=0;
  struct memo_info *minfo=NULL;
  ScrollBarPtr bar;

  DmOpenRef dbRef = NULL;
  LocalID dbId;

  /* find memo database */
  if((dbId = DmFindDatabase(0, MEMODB_NAME))==0) {
    list_error(StrFindDB);
    return;
  }
    
  /* and open it */
  if((dbRef = DmOpenDatabase(0, dbId, dmModeReadOnly))==0) {
    list_error(StrOpenDB);
    return;
  }

  /* scan installed memos */
  if((memos = memo_scan(dbRef, &minfo))<=0) {
    if(memos == 0) list_error(StrNoMemo);
    DmCloseDatabase(dbRef);
    return;
  }

  frm = FrmInitForm(frm_MemoSel);
  FrmSetActiveForm(frm);
  FrmSetEventHandler(frm, MemoFormHandleEvent);
  FrmPopupForm(frm_MemoSel);
  FrmDrawForm(frm);

  /* init scrollbar */
  bar = GetObjectPtr(frm, MemoScrollbar);
  if(memos>XLINES) SclSetScrollBar(bar, 0, 0, memos-XLINES, XLINES-1);
  else             SclSetScrollBar(bar, 0, 0, 0, 0);
  
  memo_list(frm, minfo, memos, offset);

  do {
    /* exit loop every ten system ticks (about 10Hz) */
    EvtGetEvent(&event, evtWaitForever);  

    if (!SysHandleEvent(&event)) {
      switch (event.eType) {

      case sclRepeatEvent:
	offset = event.data.sclExit.newValue;
	memo_list(frm, minfo, memos, offset);
	break;

      case ctlSelectEvent:
	switch(event.data.ctlEnter.controlID) {
	case MemoName0:
	case MemoName1:
	case MemoName2:
	case MemoName3:
	case MemoName4:
	case MemoName5:
	case MemoName6:
	case MemoName7:
	case MemoName8:
	case MemoName9:
	  memo_write(dbRef, 
               minfo[event.data.ctlEnter.controlID-MemoName0+offset].record);
	  quit = true;
	  break;
	  
	case MemoCancel:
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

  /* close memo database */
  DmCloseDatabase(dbRef);

  /* free name table */
  if(minfo != NULL) MemPtrFree(minfo);

  /* free mem on card may have changed */
  redraw_free_mem = true;

  /* rescan entry */
  list_dir();

  return;
}
