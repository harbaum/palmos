/*
  db.c - database routines for SmartSync

  (c) 1999 Till Harbaum

  install prc/pdb databases into PalmOS

  ToDo/extensions:
  - sortInfoArea (???)
  - nextRecordList (???)

  - optionally skip records marked 'delete on next hotsync'
    (always stored at the end of the database)
*/

#ifdef OS35HDR
#include <PalmOS.h>
#include <PalmCompatibility.h>
#else
#include <Pilot.h>
#endif

#include <ExgMgr.h>

#include "../api/axxPac.h"

#include "explorerRsc.h"
#include "db.h"
#include "callback.h"

/* draw some kind of progess bar */
void db_progress_bar(Boolean init, char *str, int percent, int *lp, int progress_pos) {
  RectangleType rect;
  char pstr[5];
  int  x,y;

  if(percent<0)   percent=0;
  if(percent>100) percent=100;

  switch(progress_pos) {
  case 0:
    x = 80; y = 80-(65/2)+34;
    break;

  case 1:
    x = 80; y = 80-(65/2)+46;
    break;

  case 2:
    x = 70; y = 10+46;
    break;
  }

  if(init) {
    /* no 'special' progressbar mode? */
    if(!progress_pos) {
      rect.topLeft.x = 80-(BAR_WIDTH/2);
      rect.topLeft.y = 80-(BAR_HEIGHT/2);
      rect.extent.x = BAR_WIDTH;
      rect.extent.y = BAR_HEIGHT;

      WinEraseRectangle(&rect, 3);
      WinDrawRectangleFrame(dialogFrame, &rect);

      WinDrawChars(str, StrLen(str), 80-(BAR_WIDTH/2)+10, 80-(BAR_HEIGHT/2)+5);

    }

    /* draw box */
    rect.topLeft.x = x-(BAR_WIDTH/2)+10;
    rect.topLeft.y = y;
    rect.extent.x = BAR_WIDTH-20;
    rect.extent.y = 11;
    WinEraseRectangle(&rect, 0);
    WinDrawRectangleFrame(rectangleFrame, &rect);
  }

  /* bar has to be drawn */
  if((percent != *lp)||(init)) {

    /* erase inards */
    rect.topLeft.x = x-(BAR_WIDTH/2)+11;
    rect.topLeft.y = y+1;
    rect.extent.x = BAR_WIDTH-22;
    rect.extent.y = 9;
    WinEraseRectangle(&rect, 0);
    
    /* draw bar */
    rect.topLeft.x = x-(BAR_WIDTH/2)+11;
    rect.topLeft.y = y+1;
    rect.extent.x = (percent*(BAR_WIDTH-22))/100;
    rect.extent.y = 9;
    WinDrawRectangle(&rect, 0);

    /* new percentage */
    StrPrintF(pstr, "%d%%", percent);
    WinInvertChars(pstr, StrLen(pstr), 
		   x-(FntLineWidth(pstr, StrLen(pstr))/2),y);
  
    *lp = percent;
  }
}

Boolean 
delete_proc(const char *name, Word version, Int card, 
	    LocalID id, void *udata) {
  Boolean ret=true;
  LocalID dbId;
  struct read_info *rinfo = (struct read_info *)udata;

  CALLBACK_PROLOGUE;

  /* delete database if already existant */
  if((dbId = DmFindDatabase(0, (char*)name))!=0) {

    /* ask for permission? */
    if((prefs.flags & PREF_OVERWRITE)&&(!rinfo->force)) {
      if(FrmCustomAlert(alt_overwrite, (char*)name, 0,0)==1) {
	ret = false;
	goto del_quit;
      }
    }

    /* try to delete db */
    if(DmDeleteDatabase(0, dbId) != 0) {
      if(!rinfo->force)
	list_error(StrDeleteDB);

      ret = false;
    }
  }

del_quit:
  CALLBACK_EPILOGUE;
  return ret;
}

Err
read_proc(void *data, ULong *size, void *udata) {
  Long ret;
  Err err;
  struct read_info *rinfo = (struct read_info *)udata;

  CALLBACK_PROLOGUE;

#ifdef HANDLE_OLD_FORMAT
  if(((rinfo->state == 1)||(rinfo->state == 2))&&(rinfo->skip==0)) {
    DEBUG_MSG("Old format, returning 2 additional 0 bytes\n");

    /* return two additional 0 bytes */
    *(char*)data = 0; data++;
    *(char*)data = 0; data++;
    *size -= 2;

    rinfo->state = 3;
    if(*size == 0) {
      err = 0;
      goto read_quit;
    }
  }
#else
  #warning "No support for old format!!"
#endif

  db_progress_bar(false, NULL, (100l*rinfo->cnt)/rinfo->flen, rinfo->lp, rinfo->pos);

  ret = axxPacRead(rinfo->lib, rinfo->fd, data, *size);
  DEBUG_MSG("read %ld (%ld) bytes\n", ret, *size);

  if(ret < 0) {
    FrmCustomAlert(alt_err, axxPacStrError(rinfo->lib), 0,0);
    DEBUG_MSG("IO error: %s\n", axxPacStrError(rinfo->lib));
    err = ret;
  } else {
#ifdef HANDLE_OLD_FORMAT
    if(rinfo->state == 0) {

      /* build header */
      if((rinfo->cnt + *size)<sizeof(struct db_header)) 
	MemMove( ((char*)&rinfo->header)+rinfo->cnt, data, *size);
      else {
	MemMove( ((char*)&rinfo->header)+rinfo->cnt, data, 
		 sizeof(struct db_header)-rinfo->cnt);
	
	/* differ between resource and record */
	if(rinfo->header.fileAttributes & dmHdrAttrResDB)
	  rinfo->skip = rinfo->header.numberOfRecords * 
	    sizeof(struct db_rsrc_header);
	else
	  rinfo->skip = rinfo->header.numberOfRecords * 
	    sizeof(struct db_record_header);

	rinfo->state = 3;  /* default: do nothing */

	/* try to detect whether this is one of our old format databases */
	if((rinfo->header.nextRecordListID == 0) && (rinfo->header.sortInfoArea == 0)) {
	  if(rinfo->header.appInfoArea != 0) {
	    DEBUG_MSG("got app info area, offset = %d\n", (int)(rinfo->header.appInfoArea - rinfo->skip));

	    /* data starts behind header -> old format */
	    if((rinfo->header.appInfoArea - rinfo->skip) == sizeof(struct db_header)) rinfo->state = 2;
	  } else {
	    DEBUG_MSG("No app info area\n");
	    rinfo->state = 1;
	  }
	} 
      }
    } else if((rinfo->state==1)||(rinfo->state==2)) {
      if(rinfo->state == 1) {
	rinfo->state = 2;

	DEBUG_MSG("save first rec/rsrc header\n");

	if(rinfo->header.numberOfRecords != 0) {
	  if(rinfo->header.fileAttributes & dmHdrAttrResDB)
	    MemMove(&rinfo->rsrc1st, data, sizeof(struct db_rsrc_header));
	  else
	    MemMove(&rinfo->rec1st, data, sizeof(struct db_record_header));
	}
	
	if(rinfo->header.numberOfRecords != 0) {
	  if(rinfo->header.fileAttributes & dmHdrAttrResDB) {
	    /* new format, cancel skip */
	    if(rinfo->rsrc1st.offset - rinfo->skip != sizeof(struct db_header))
	      rinfo->state = 3;
	  } else {
	    /* new format, cancel skip */
	    if(rinfo->rec1st.offset - rinfo->skip != sizeof(struct db_header))
	      rinfo->state = 3;
	  }
	}
      }

      rinfo->skip -= ret;
    }
#endif

    rinfo->cnt += ret;

    if(ret != *size) *size = ret;

    err = 0;
  }

read_quit:
  CALLBACK_EPILOGUE;
  return err;
}

Err
write_proc(const void *data, ULong *size, void *udata) {
  Long ret;
  Err err;
  struct write_info *winfo = (struct write_info *)udata;

  CALLBACK_PROLOGUE;

  db_progress_bar(false, NULL, (100l*winfo->cnt)/winfo->flen, winfo->lp, winfo->pos);

  ret = axxPacWrite(winfo->lib, winfo->fd, (void*)data, *size);

  if(ret < 0) {
    err = ret;
  } else {
    if(ret != *size) 
      *size = ret;

    winfo->cnt += *size;
    err = 0;
  }

  CALLBACK_EPILOGUE;
  return err;
}

/* read a database from dir entry db and install it in the Pilot */
Boolean db_read(char *filename, long flength, int pos, Boolean force) {
  LocalID dbID;
  Int card;
  Boolean rst;
  struct read_info rinfo;
  int lp;

  /* setup read info structure */
  rinfo.flen = flength;
  rinfo.cnt = 0;
#ifdef HANDLE_OLD_FORMAT
  rinfo.state = 0;
#endif
  rinfo.force = force;
  rinfo.lib = LibRef;
  rinfo.lp  = &lp;
  rinfo.pos = pos;

  /* open file */
  if((rinfo.fd = axxPacOpen(LibRef, filename, O_RdOnly))<0) {
    FrmCustomAlert(alt_err, axxPacStrError(LibRef), 0, 0);
    return false;
  }

  db_progress_bar(true, "Copying ...", 0, &lp, pos);

  /* start installation process */
  ExgDBRead(read_proc, delete_proc, &rinfo, &dbID, 0, &rst, false);

  db_progress_bar(false, NULL, 100, &lp, pos);

  /* and close file */
  axxPacClose(LibRef, rinfo.fd);

  return true;
}

#define EXTRA_BYTES 2  // like palm does it
/* read a database from pilot and write it to the card */
Boolean db_write(UInt lib, char *name, long flength, int pos,
		 Boolean force, Boolean quiet) {
  struct write_info winfo;
  char db_name[39];
  LocalID dbId;
  uword fileAttributes;
  Err err;
  int i;
  int lp;

  /* find database */
  if((dbId = DmFindDatabase(0, name))==0) {
    list_error(StrFindDB);
    return false;
  }
  
  DmDatabaseInfo(0, dbId, NULL,                /* name */
		 &fileAttributes,              /* attributes */
		 NULL, NULL, NULL, NULL, NULL,
		 NULL, NULL, NULL, NULL	);

  StrCopy(db_name, name);

  /* check whether file is resource or record database */
  if(fileAttributes & dmHdrAttrResDB)  StrCat(db_name, ".prc");
  else                                 StrCat(db_name, ".pdb");

  /* remove slashes '/', '*' and '?' from name */
  for(i=0;db_name[i]!=0;i++) 
    if((db_name[i]=='/')||(db_name[i]=='?')||(db_name[i]=='*')) db_name[i]='_';

  if((!force)&&(!quiet)) {
    /* database already exists */
    if((prefs.flags & PREF_OVERWRITE)&&(axxPacExists(lib, db_name) == 0)) {
      if(FrmCustomAlert(alt_overwrite, db_name, 0,0)==1) {
	return false;
      }
    }
  }

  winfo.flen  = flength;
  winfo.cnt   = 0;
  winfo.quiet = quiet;
  winfo.lib   = lib;
  winfo.pos   = pos;
  winfo.lp    = &lp;

  if((winfo.fd = axxPacOpen(lib, db_name, O_Creat))<0) {
    if(!quiet) FrmCustomAlert(alt_err, axxPacStrError(lib), 0, 0);
    return false;
  }

  db_progress_bar(true, "Copying ...", 0, &lp, pos);

  err = ExgDBWrite(write_proc, &winfo, name, 0, 0);

  db_progress_bar(false, NULL, 100, &lp, pos);

  if(err<0) {
    if(!quiet) FrmCustomAlert(alt_err, axxPacStrError(lib), 0, 0);
    axxPacClose(lib, winfo.fd);

    /* try to delete file that caused the error */
    axxPacDelete(lib, db_name);

    return false;
  }

  axxPacClose(lib, winfo.fd);
  return true;
}

/* delete a palm database */
Boolean db_delete(char *name) {
  LocalID dbId;
 
  /* confirm deletion */
  if(prefs.flags & PREF_DELETE) {
    if(FrmCustomAlert(alt_delete, name, 0,0)==1)
      return false;
  }
 
  if((dbId = DmFindDatabase(0, name))==0) {
    list_error(StrFindDB);
    return false;
  }    

  if(DmDeleteDatabase(0, dbId) != 0) {
    list_error(StrDeleteDB);
    return false;
  }

  return true;
}
