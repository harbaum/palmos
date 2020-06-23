/*

  filio.c  - (c) 2000 by Till Harbaum

  ftp client for axxpac and palm VFS

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
#include "ftp.h"
#include "filio.h"
#include "fsel.h"

#include "../api/axxPac.h"

/* axxPac driver library reference */
UInt axxPacRef=0;

extern UInt16 volRefNum;

Boolean fs_driver_open(void) {
  UInt32 MgrVersion;
  Err err;

  /* try to open already installed axxPac driver lib */
  err = SysLibFind(AXXPAC_LIB_NAME, &axxPacRef);

  /* didn't work, try to load it */
  if (err) err = SysLibLoad('libr', AXXPAC_LIB_CREATOR, &axxPacRef);
    
  if(!err) {
    /* try to open lib */
    if(axxPacLibOpen(axxPacRef) != 0) {
      FrmCustomAlert(alt_err, axxPacStrError(axxPacRef), 0, 0);
      return false;
    }
    return true;
  } else 
    axxPacRef = 0;

  /* no axxPac driver, try to access VFS */
  if(!FtrGet(sysFileCExpansionMgr, expFtrIDVersion, &MgrVersion)) { 
    if(!FtrGet(sysFileCVFSMgr, vfsFtrIDVersion, &MgrVersion)) {
      return true;
    }
  }

  FrmCustomAlert(alt_err, "No file system driver found!",0,0);
  return false;
}

void fs_driver_close(void) {
  UInt numapps;

  if(axxPacRef != 0) {
    axxPacLibClose(axxPacRef, &numapps);
    
    /* Check for errors in the Close() routine */
    if (numapps == 0) {
      SysLibRemove(axxPacRef);
    }
  }
}


UInt16 fs_fsel(char *title, char *name, Boolean new) {
  UInt16 i;

  if(axxPacRef != 0) {
    i = axxPacFsel(axxPacRef, title, name, "*", new);
    /* file error */
    if(i<0) FrmCustomAlert(alt_err, axxPacStrError(axxPacRef), 0, 0);
  } else {
    i = fs_file_selector(title, name, new);
  }

  return i;
}


axxPacFD axxpac_fd;
FileRef  vfsref;

extern void ALERT_NUM(char *, int);

Boolean fs_open(char *name, int cmode) {
  UInt16 mode;

  if(axxPacRef != 0) {
    if(cmode == FS_CREAT)       mode = O_Creat;
    else if(cmode == FS_RDONLY) mode = O_RdOnly;
    else if(cmode == FS_RDWR)   mode = O_RdWr;

    if((axxpac_fd = axxPacOpen(axxPacRef, name, mode))<0) {
      FrmCustomAlert(alt_err, axxPacStrError(axxPacRef), 0, 0);
      return false;
    }
  } else {
    if(cmode == FS_CREAT)       mode = vfsModeReadWrite;
    else if(cmode == FS_RDONLY) mode = vfsModeRead;
    else if(cmode == FS_RDWR)   mode = vfsModeReadWrite;

    if(cmode == FS_CREAT) {
//      ALERT_NUM("create file",0);

      if(VFSFileCreate(volRefNum, name) != errNone) {
	FrmCustomAlert(alt_err, "Unable to create file.", 0, 0);
	return false;
      }
    }

    if(VFSFileOpen(volRefNum, name, mode, &vfsref) != errNone) {
      FrmCustomAlert(alt_err, "Unable to open file.", 0, 0);
      return false;
    }
  }
  return true;
}

void fs_close(void) {
  if(axxPacRef != 0)
    axxPacClose(axxPacRef, axxpac_fd);
  else
    VFSFileClose(vfsref);
}

void fs_delete(char *name) {
  if(axxPacRef != 0)
    axxPacDelete(axxPacRef, name);
  else {
    if(VFSFileDelete(volRefNum, name) != errNone)
      FrmCustomAlert(alt_err, "Unable to delete file.", 0,0);
  }
}

long fs_tell(char *name) {
  axxPacStatType stat;
  UInt32 len=0;

  if(axxPacRef != 0) {
    /* get file length */
    if(axxPacStat(axxPacRef, name, &stat)) {
      FrmCustomAlert(alt_err, axxPacStrError(axxPacRef), 0, 0);
      return -1;
    }
    return stat.size;
  } else {
    if(fs_open(name, FS_RDONLY)) {
      VFSFileSize(vfsref, &len);
      fs_close();
      return len;
    }
  }

  return -1; /* error */
}

long fs_read(void *buf, long num) {
  UInt32 ret=0;
  Err err;

  if(axxPacRef != 0) {
    ret = axxPacRead(axxPacRef, axxpac_fd, buf, num);
    if(ret != num) FrmCustomAlert(alt_err, axxPacStrError(axxPacRef), 0, 0);
  } else {
    if((err=VFSFileRead(vfsref, num, buf, &ret)) != errNone) {
      char str[128];

      /* EOF is ok for us ... */
      if(err == vfsErrFileEOF)
	return err;

      StrPrintF(str, "Error reading data! (%x)", err);

      FrmCustomAlert(alt_err, str,0,0);
      return 0;
    }
  }
  return ret;
}

long fs_write(void *buf, long num) {
  UInt32 ret=0;

  if(axxPacRef != 0) {
    ret = axxPacWrite(axxPacRef, axxpac_fd, buf, num);
    if(ret != num) FrmCustomAlert(alt_err, axxPacStrError(axxPacRef), 0, 0);
  } else {
    if(VFSFileWrite(vfsref, num, buf, &ret) != errNone) {
      FrmCustomAlert(alt_err, "Error writing data!",0,0);
      return 0;
    }
  }

  return ret;
}




