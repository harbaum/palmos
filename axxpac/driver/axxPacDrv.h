/*
   axxPacDrv.h  -  (c) 2000 Till Harbaum
*/

#ifndef __AXXPACDRV_H__
#define __AXXPACDRV_H__

#define AXXPAC_NOTRAPDEFS
#include "../api/axxPac.h"

#ifdef DEBUG
#define noDEBUG_POSE    // either pose or 
#define DEBUG_RS232   // rs232
#endif

#ifdef DEBUG
#ifdef DEBUG_POSE
#warning "POSE DEBUG version"
#define DEBUG_PORT 0x30000000
#define DEBUG_MSG(format, args...) { char tmp[64]; int xyz; StrPrintF(tmp, format, ## args); for(xyz=0;xyz<StrLen(tmp);xyz++) *((char*)DEBUG_PORT)=tmp[xyz]; }
#endif
#ifdef DEBUG_RS232
#warning "RS232 DEBUG version"
#define DEBUG_MSG(format, args...) { UInt debug_err; char debug_tmp[64]; StrPrintF(debug_tmp, format, ## args); SerSend(gl->ComRef, debug_tmp, StrLen(debug_tmp), &debug_err); }
#endif
#else
#define DEBUG_MSG(format, args...)
#endif

#define noDEBUG_IO
#define noDEBUG_MEM

#ifdef DEBUG_MEM
#ifdef DEBUG_RS232
#error "DEBUG_MEM does not work with RS232 debug"
#endif
#define DEBUG_MSG_MEM(args...) DEBUG_MSG(##args)
#else
#define DEBUG_MSG_MEM(args...)
#endif

#ifdef DEBUG_IO
#define DEBUG_MSG_IO(args...) DEBUG_MSG(##args)
#else
#define DEBUG_MSG_IO(args...)
#endif

#ifdef DEBUG_RS232
#include <SerialMgr.h>
#endif

#define DELAYED_CLOSE        // use delayed close for card
#define DELAYED_TIMEOUT 50   // 500 ms

#include "smartmedia.h"
#include "errors.h"
#include "filesys.h"

/* feature ids */ 
#define FTR_GLOBS   0   // pointer to globals

typedef struct {
  DWord old;
} trap_globals;

typedef struct {
  smartmedia_globals smartmedia;      /* global variables of smartmedia.c */
  error_globals      error;           /* global variables of errors.c */
  trap_globals       trap;            /* global variables of TrapHook.c */
  filesys_globals    filesys;         /* global variables of filesys.c */

  void(*callback)(axxPacCardState);   /* callback function for card state */
  Word refcount;                      /* number of open calls */

#ifdef DEBUG_RS232
  UInt ComRef;                        /* reference for serial lib */
#endif
} Lib_globals;

/* smartmedia.c */
extern void     smartmedia_init_globals(Lib_globals *globs);
extern Err      smartmedia_initialize(Lib_globals *globs);
extern Err      smartmedia_release(Lib_globals *globs);
extern void     smartmedia_get_info(Lib_globals *globs, axxPacCardInfo *info);
extern Boolean  smartmedia_open(Lib_globals *globs, Boolean complain);
extern void     smartmedia_close(Lib_globals *globs);
#ifdef DELAYED_CLOSE
extern void     smartmedia_force_close(Lib_globals *gl);
#endif
extern Err      smartmedia_read(Lib_globals *globs, void *data, 
				long start_sec, int number, Boolean);
extern axxPacCardState smartmedia_card_status(Lib_globals *globs);
extern Boolean  smartmedia_free_lblock(Lib_globals *globs, long sec);
extern Err      smartmedia_write(Lib_globals *globs, 
				 void *data, long start_sec, int number);
extern Err      smartmedia_format(Lib_globals *globs, 
				  void(*callback)(unsigned char));

/* filesys.c */
extern void     fs_init_globals(Lib_globals *gl);
extern void     fs_release(Lib_globals *gl);
extern Err      fs_prepare_access(Lib_globals *gl, Boolean write);
extern Err      fs_init(Lib_globals *gl);
extern Err      fs_findfirst(Lib_globals *gl, int, char *, find_state *, 
			     axxPacFileInfo *, int, unsigned char);
extern Err      fs_findnext(Lib_globals *gl, char *, find_state *,
			     axxPacFileInfo *, unsigned char);
extern Err      fs_get_diskspace(Lib_globals *gl, DWord *);
extern Err      fs_changedir(Lib_globals *gl, char*);
extern axxPacFD fs_open(Lib_globals *gl, char *, axxPacMode);
extern Err      fs_close(Lib_globals *gl, axxPacFD fd);
extern long     fs_read(Lib_globals *gl, axxPacFD, void*, long);
extern long     fs_write(Lib_globals *gl, axxPacFD, void*, long);
extern char*    fs_getcwd(Lib_globals *gl);
extern Err      fs_delete(Lib_globals *gl, char* );
extern Err      fs_process_path(Lib_globals *gl, char*, int*, char**); 
extern Err      fs_rename(Lib_globals *gl, char*, char*);
extern Err      fs_setwp(Lib_globals *gl, char*, Boolean);
extern Err      fs_stat(Lib_globals *gl, char*, axxPacStatType*);
extern long     fs_tell(Lib_globals *gl, axxPacFD);
extern char*    fs_strrchr(char*, char);
extern Err      fs_setattr(Lib_globals *gl, char*, unsigned char, Boolean);
extern Err      fs_setdate(Lib_globals *gl, char*, UInt32);

/* errors.c */
extern void     error_init_globals(Lib_globals *gl);
extern void     error(Lib_globals *gl, Boolean type, Err err);
extern void     error_reset(Lib_globals *gl);
extern char*    error_string(Lib_globals *gl);

/* TrapHook.c */
extern void     TrapInitGlobals(Lib_globals *gl);
extern void     TrapInstall(Lib_globals *gl);
extern void     TrapRemove(Lib_globals *gl);

/* fsel.c */
extern Err      fsel(Lib_globals *gl, char*, char*, char*, Boolean);

#endif // __AXXPACDRV_H__




