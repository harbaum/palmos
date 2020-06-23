/* 
   axxPacDrv.c  - (c) 2000 Till Harbaum

   axxPac driver library
*/

#ifdef OS35HDR
#include <PalmOS.h>
#include <PalmCompatibility.h>
#else
#include <Pilot.h>
#endif

#define  AXXPAC_INTERNAL
#include "axxPacDrv.h"
#include "../api/axxPac.h"

#include "smartmedia.h"

#define noCHECK4GLOBALS

/* Dispatch table declarations */
ULong install_dispatcher(UInt,SysLibTblEntryPtr);
Ptr   *gettable(void);

ULong 
start (UInt refnum, SysLibTblEntryPtr entryP) {
  ULong ret;

  asm("
        movel %%fp@(10),%%sp@-
        movew %%fp@(8),%%sp@-
        jsr install_dispatcher(%%pc)
        movel %%d0,%0
        jmp out(%%pc)
gettable:
        lea jmptable(%%pc),%%a0
        rts

        .set FUNC_NO, 29             | number of library functions

jmptable:
        dc.w (6*FUNC_NO+2)

        # offsets to the system function calls
        dc.w ( 0*4 + 2*FUNC_NO + 2)  | open
        dc.w ( 1*4 + 2*FUNC_NO + 2)  | close
        dc.w ( 2*4 + 2*FUNC_NO + 2)  | sleep
        dc.w ( 3*4 + 2*FUNC_NO + 2)  | wake

        # offsets to the user function calls
        dc.w ( 4*4 + 2*FUNC_NO + 2)
        dc.w ( 5*4 + 2*FUNC_NO + 2)
        dc.w ( 6*4 + 2*FUNC_NO + 2)
        dc.w ( 7*4 + 2*FUNC_NO + 2)
        dc.w ( 8*4 + 2*FUNC_NO + 2)
        dc.w ( 9*4 + 2*FUNC_NO + 2)
        dc.w (10*4 + 2*FUNC_NO + 2)
        dc.w (11*4 + 2*FUNC_NO + 2)
        dc.w (12*4 + 2*FUNC_NO + 2)
        dc.w (13*4 + 2*FUNC_NO + 2)
        dc.w (14*4 + 2*FUNC_NO + 2)
        dc.w (15*4 + 2*FUNC_NO + 2)
        dc.w (16*4 + 2*FUNC_NO + 2)
        dc.w (17*4 + 2*FUNC_NO + 2)
        dc.w (18*4 + 2*FUNC_NO + 2)
        dc.w (19*4 + 2*FUNC_NO + 2)
        dc.w (20*4 + 2*FUNC_NO + 2)
        dc.w (21*4 + 2*FUNC_NO + 2)
        dc.w (22*4 + 2*FUNC_NO + 2)
        dc.w (23*4 + 2*FUNC_NO + 2)
        dc.w (24*4 + 2*FUNC_NO + 2)
        dc.w (25*4 + 2*FUNC_NO + 2)
        dc.w (26*4 + 2*FUNC_NO + 2)
        dc.w (27*4 + 2*FUNC_NO + 2)
        dc.w (28*4 + 2*FUNC_NO + 2)

        # functions for system use
        jmp axxPacLibOpen(%%pc)
        jmp axxPacLibClose(%%pc)
        jmp axxPacLibSleep(%%pc)
        jmp axxPacLibWake(%%pc)

        # lib functions
        jmp axxPacStrError(%%pc)
        jmp axxPacGetLibInfo(%%pc)

        # card functions
        jmp axxPacInstallCallback(%%pc)
        jmp axxPacGetCardInfo(%%pc)
        jmp axxPacFormat(%%pc)
        jmp axxPacDiskspace(%%pc)

        # directory functions
        jmp axxPacExists(%%pc)
        jmp axxPacFindFirst(%%pc)
        jmp axxPacFindNext(%%pc)
        jmp axxPacNewDir(%%pc)
        jmp axxPacChangeDir(%%pc)
        jmp axxPacGetCwd(%%pc)
        jmp axxPacDelete(%%pc)

        # file io functions
        jmp axxPacOpen(%%pc)
        jmp axxPacClose(%%pc)
        jmp axxPacRead(%%pc)
        jmp axxPacWrite(%%pc)
        jmp axxPacSeek(%%pc)

        # new functions
        jmp axxPacRename(%%pc)
        jmp axxPacSetWP(%%pc)
        jmp axxPacStat(%%pc)
        jmp axxPacTell(%%pc)
        jmp axxPacFsel(%%pc)
        jmp axxPacSetAttrib(%%pc)
        jmp axxPacSetDate(%%pc)

        .asciz \"axxPac driver\"
    .even
    out:
        " : "=r" (ret) :);
  return ret;
}

/* routine to install library into system */
ULong 
install_dispatcher(UInt refnum, SysLibTblEntryPtr entryP) {
  Ptr *table = gettable();
  entryP->dispatchTblP = table;
  entryP->globalsP = NULL;
  return 0;
}

Err 
axxPacLibOpen(UInt refnum) {
  SysLibTblEntryPtr entryP = SysLibTblEntry(refnum);
  Lib_globals *gl = (Lib_globals*)entryP->globalsP;
  Err err;

  /* Get the current globals value */
  if (gl) {
    /* We are already open in some other application; just
       increment the refcount */
    gl->refcount++;
    return ErrNone;
  }

  /* We need to allocate space for the globals */
  entryP->globalsP = MemPtrNew(sizeof(Lib_globals));
  DEBUG_MSG_MEM("alloc globs %ld = %lx\n", sizeof(Lib_globals), entryP->globalsP);

  MemPtrSetOwner(entryP->globalsP, 0);
  gl = (Lib_globals*)entryP->globalsP;
  FtrSet(CREATOR, FTR_GLOBS, (DWord)gl);

  /* Initialize the globals */
  fs_init_globals(gl);
  smartmedia_init_globals(gl);
  error_init_globals(gl);
  TrapInitGlobals(gl);

  gl->callback = NULL;
  gl->refcount = 1;

#ifdef DEBUG_RS232
  /* open rs232 */
  if (SysLibFind("Serial Library", &gl->ComRef)) {
    gl->ComRef = 0;  /* failed ... */
  } else {
    if(err = SerOpen(gl->ComRef, 0, 19200l)) {
      /* failed ... */
      if (err == serErrAlreadyOpen)
        SerClose(gl->ComRef);

      gl->ComRef = 0;
    }
  }
#endif

  DEBUG_MSG_IO("axxPacLibOpen(%d)\n", refnum);

  /* check for hardware ... */
  if(err=smartmedia_initialize(gl)) {
    error(gl, false, ErrLibOpen);
    return -ErrLibOpen;
  }
  
  /* install own traps */
  TrapInstall(gl);
  
  /* axxPac is fine */
  return ErrNone;
}

Err 
axxPacLibClose(UInt refnum, UIntPtr numappsP) {
  /* Get the current globals value */
  SysLibTblEntryPtr entryP = SysLibTblEntry(refnum);
  Lib_globals *gl = (Lib_globals*)entryP->globalsP;
  error_reset(gl);

  DEBUG_MSG_IO("axxPacLibClose(%d, %d)\n", refnum, *numappsP);

  /* Clean up */
  *numappsP = --gl->refcount;
  if (*numappsP == 0) {

    /* uninstall own traps */
    TrapRemove(gl);

    /* shutdown filesystem support */
    fs_release(gl);

    /* shutdown hardware */
    smartmedia_release(gl);

#ifdef DEBUG_RS232
    SerClose(gl->ComRef);
#endif

    DEBUG_MSG_MEM("free globs: %lx\n", entryP->globalsP);
    MemChunkFree(entryP->globalsP);
    entryP->globalsP = NULL;
  }
  
  return 0;
}

Err 
axxPacLibSleep(UInt refnum) {
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);

#ifdef DELAYED_CLOSE
#ifndef DEBUG_RS232
  /* save the environment, power down your axxPac */
  /* only in non-debug, since debugging output via */
  /* rs232 will crash on sleep (rs232 is already */
  /* sleeping) */
  smartmedia_force_close(gl);
#endif
#endif

  return ErrNone;
}

Err 
axxPacLibWake(UInt refnum) {
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);

  DEBUG_MSG("WAKE\n");

#ifdef DELAYED_CLOSE
  smartmedia_force_close(gl);
#endif

  gl->smartmedia.type = CARD_NONE;  /* force reload of card info */

  if(gl->callback != NULL)
    gl->callback(axxPacCardNone);

  return ErrNone;
}

axxPacFD
axxPacOpen(UInt refnum, char *filename, axxPacMode mode) {
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
  Err err;
  
  if((err = fs_prepare_access(gl, (mode != O_RdOnly) && (mode != O_RdFast))) < 0) return err;

  DEBUG_MSG_IO("axxPacOpen(%d, %s, %d)\n", refnum, filename, mode);
  return fs_open(gl, filename, mode);
}

Err 
axxPacClose(UInt refnum, axxPacFD fd) {
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
  Err err;

  if((err = fs_prepare_access(gl, false)) < 0) return err;

  DEBUG_MSG_IO("axxPacClose(%d, %d)\n", refnum, fd);
  return fs_close(gl, fd);
}

char*
axxPacStrError(UInt refnum) {
#ifdef DEBUG_RS232
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
#endif

  DEBUG_MSG_IO("axxPacStrError(%d)\n", refnum);
  return((char*)error_string((Lib_globals*)
			     (SysLibTblEntry(refnum)->globalsP)));
}

Err 
axxPacInstallCallback(UInt refnum, void(*callback)(axxPacCardState)) {
#ifdef DEBUG_RS232
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
#endif

  DEBUG_MSG_IO("axxPacInstallCallback(%d, %lx)\n", 
	       refnum, (unsigned long)callback);
  /* save callback address */
  ((Lib_globals*)(SysLibTblEntry(refnum)->globalsP))->callback = callback;
}

Err 
axxPacGetCardInfo(UInt refnum, axxPacCardInfo *info) {
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
  Err err;

  if((err = fs_prepare_access(gl, false)) < 0) return err;

  DEBUG_MSG_IO("axxPacGetCardInfo(%d, %lx)\n", refnum, (unsigned long)info);

  /* reload card info if needed ... */
  smartmedia_card_status(gl);

  /* and get info from smartmedia layer */
  smartmedia_get_info(gl, info);

  return ErrNone;
}

Err 
axxPacFormat(UInt refnum, void(*callback)(unsigned char)) {
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);

  DEBUG_MSG_IO("axxPacFormat(%d, %lx)\n", refnum, (unsigned long)callback);
  /* format card */
  return smartmedia_format(gl, callback);
}

Err 
axxPacDiskspace(UInt refnum, DWord* free) {
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
  Err err;

  if((err = fs_prepare_access(gl, false)) < 0) return err;

  DEBUG_MSG_IO("axxPacDiskspace(%d, %lx)\n", refnum, (unsigned long)free);
  /* format card */
  return fs_get_diskspace(gl, free);
}

Err 
axxPacGetLibInfo(UInt refnum, axxPacLibInfo* info) {
#ifdef DEBUG_RS232
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
#endif

  DEBUG_MSG_IO("axxPacGetLibInfo(%d, %lx)\n", refnum, (unsigned long)info);

  /* set library info */
  StrCopy(info->version, "1.22");            /* library version string */
  StrCopy(info->creator, "AMS/T.Harbaum");   /* library creator string */
  info->revision = 3;                        /* lib interface revision */

  return ErrNone;
}

Err 
axxPacFindFirst(UInt refnum, char *pattern, axxPacFileInfo* info, int mode) {
  int cluster;
  Err err;
  char *basename;

  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
  if((err = fs_prepare_access(gl, false)) < 0) return err;
  
  DEBUG_MSG_IO("axxPacFindFirst(%d, \"%s\", %lx)\n", 
	    refnum, pattern, (unsigned long)info);

#ifdef DEBUG
  if(sizeof(find_state) > 270)
    DEBUG_MSG_IO("State info too big!!!\n");
#endif

  if(StrLen(pattern)>MAX_FIND_PATTERN) {
    error(gl, false, ErrFsPat2Long);
    return -ErrFsPat2Long;
  }
  
  /* handle path */
  if((err = fs_process_path(gl, pattern, &cluster, &basename))<0) return err;

  /* and search ... */
  err = fs_findfirst(gl, cluster, basename, 
		     (find_state*)&(info->reserved), 
		     info, mode, 0);

  /* save filename part of pattern in reserved area */
  StrCopy(((find_pstate*)&(info->reserved))->pattern, basename);  
    
  if(err == -ErrFsNotFound) /* this is positive, since it is no error */
    err = ErrFsNotFound;    /* in this function */

  return err;
}

Err 
axxPacFindNext(UInt refnum, axxPacFileInfo* info) {
  Err err;
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
  if((err = fs_prepare_access(gl, false)) < 0) return err;
  
  DEBUG_MSG_IO("axxPacFindNext(%d, %lx)\n", refnum, (unsigned long)info);

  err = fs_findnext(gl, ((find_pstate*)&(info->reserved))->pattern, 
		    (find_state*)&(info->reserved), info, 0);

  if(err == -ErrFsNotFound) /* this is positive, since it is no error */
    err = ErrFsNotFound;    /* in this function */

  return err;
}

Err 
axxPacChangeDir(UInt refnum, char *dir) {
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
  Err err;

  if((err = fs_prepare_access(gl, false)) < 0) return err;

  DEBUG_MSG_IO("axxPacChangeDir(%d, %s)\n", refnum, dir);
  return fs_changedir(gl, dir);
}

long
axxPacRead(UInt refnum, axxPacFD fd, void *addr, long size) {
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
  error_reset(gl);

  DEBUG_MSG_IO("axxPacRead(%d, %d, %lx, %ld)\n", refnum, fd, 
	    (unsigned long)addr, size);
  return fs_read(gl, fd, addr, size);
}

Err
axxPacSeek(UInt refnum, axxPacFD fd, long offset, int whence) {
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
  error_reset(gl);

  DEBUG_MSG_IO("axxPacSeek(%d, %d, %ld, %d)\n", refnum, fd, offset, whence);
  return fs_seek(gl, fd, offset, whence);
}

char *
axxPacGetCwd(UInt refnum) {
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
  Err err;

  if((err = fs_prepare_access(gl, false)) < 0) return (char*)(long)err;

  DEBUG_MSG_IO("axxPacGetCwd(%d)\n", refnum);
  return fs_getcwd(gl);
}

Err
axxPacDelete(UInt refnum, char *name) {
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
  Err err;

  if((err = fs_prepare_access(gl, true)) < 0) return err;

  DEBUG_MSG_IO("axxPacDelete(%d, %s)\n", refnum, name);
  return fs_delete(gl, name);
}

Err
axxPacNewDir(UInt refnum, char *name) {
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
  Err err;

  if((err = fs_prepare_access(gl, true)) < 0) return err;

  DEBUG_MSG_IO("axxPacNewDir(%d, %s)\n", refnum, name);
  return fs_newdir(gl, name);
}

long
axxPacWrite(UInt refnum, axxPacFD fd, void *addr, long size) {
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
  error_reset(gl);

  DEBUG_MSG_IO("axxPacWrite(%d, %d, %lx, %ld)\n", refnum, fd, 
	       (unsigned long)addr, size);
  return fs_write(gl, fd, addr, size);
}

Err
axxPacExists(UInt refnum, char *name) {
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);

  /* this creates a axxPacFileInfo without the space for 'RESERVED' */
  short buffer[(sizeof(axxPacFileInfo)-RESERVED_SIZE+1)/2];
  find_state state;
  int cluster;
  Err err;
  char *basename;

  if((err = fs_prepare_access(gl, false)) < 0) return err;

  DEBUG_MSG_IO("axxPacExists(%d, %s)\n", refnum, name); 

  /* handle path */
  if((err = fs_process_path(gl, name, &cluster, &basename))<0) return err;

  /* search for entry */
  err = fs_findfirst(gl, cluster, basename, &state, 
		     (axxPacFileInfo*)buffer, FIND_FILES_N_DIRS, 0);

  DEBUG_MSG("axxPacExists(%d, %s) = %d\n", refnum, name, err); 
  return err;
}

Err
axxPacRename(UInt refnum, char *old_name, char *new_name) {
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
  Err err;

  if((err = fs_prepare_access(gl, true)) < 0) return err;

  DEBUG_MSG_IO("axxPacRename(%d, %s -> %s)\n", refnum, old_name, new_name); 

  return fs_rename(gl, old_name, new_name);
}

Err
axxPacSetWP(UInt refnum, char *name, Boolean protect) {
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
  Err err;

  if((err = fs_prepare_access(gl, true)) < 0) return err;

  DEBUG_MSG_IO("axxPacSetWP(%d, %s, %d)\n", refnum, name, protect); 

  return fs_setattr(gl, name, protect?FA_RDONLY:0, false);
}

Err
axxPacStat(UInt refnum, char *name, axxPacStatType *stat) {
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
  Err err;

  if((err = fs_prepare_access(gl, false)) < 0) return err;

  DEBUG_MSG_IO("axxPacStat(%d, %s, %lx)\n", refnum, name, stat); 

  return fs_stat(gl, name, stat);
}

long
axxPacTell(UInt refnum, axxPacFD fd) {
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
  Err err;

  if((err = fs_prepare_access(gl, false)) < 0) return err;

  DEBUG_MSG_IO("axxPacTell(%d, %d)\n", refnum, fd); 

  return fs_tell(gl, fd);
}

Err
axxPacFsel(UInt refnum, char *title, char *file, 
	   char *pattern, Boolean editable) {

  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
  Err err;

  if((err = fs_prepare_access(gl, false)) < 0) return err;

  DEBUG_MSG_IO("axxPacFsel(%d, %s, %s, %s, %d)\n", 
	       refnum, title, file, pattern, editable); 

  return fsel(gl, title, file, pattern, editable);
}

Err
axxPacSetAttrib(UInt refnum, char *name, unsigned char attrib) {
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
  Err err;

  if((err = fs_prepare_access(gl, true)) < 0) return err;

  DEBUG_MSG_IO("axxPacSetAttrib(%d, %s, %u)\n", refnum, name, (unsigned int)attrib); 

  return fs_setattr(gl, name, attrib, true);
}

Err
axxPacSetDate(UInt refnum, char *name, UInt32 date) {
  /* Get the current globals value */
  Lib_globals *gl = (Lib_globals*)(SysLibTblEntry(refnum)->globalsP);
  Err err;

  if((err = fs_prepare_access(gl, true)) < 0) return err;

  DEBUG_MSG_IO("axxPacSetDate(%d, %s, %lu)\n", refnum, name, date); 

  return fs_setdate(gl, name, date);
}
