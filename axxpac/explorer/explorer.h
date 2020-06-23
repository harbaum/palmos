/*
    explorer.h

    header file for axxPac explorer
*/

#ifndef __EXPLORER_H__
#define __EXPLORER_H__

#include "../api/axxPac.h"

#define SHOW_FREE_MEM        // always show free media space
#define USE_DATABASE_CACHE
#define SELECT_ICON

// debug enable/disable
#define noDEBUG_PLUGIN
#define noDEBUG_SELECT

#define noDEBUG_RS232

#ifdef DEBUG
#ifndef DEBUG_RS232
#define IMPORT_VIA_NEWDIR
#endif

#ifdef IMPORT_VIA_NEWDIR
// test stuff
#define FILEPORT 0x31000000

#define FILE_OPEN  ((*(volatile char*)FILEPORT) = 0x55)
#define FILE_CLOSE ((*(volatile char*)FILEPORT) = 0x00)
#define FILE_GET   ((*(volatile char*)FILEPORT))
#define FILE_LEN   ((*(volatile long*)FILEPORT))
#endif
#endif

#define CACHE_TYPE 'Cach'

#define PREF_VERSION      5

#define PREF_DELETE    0x01
#define PREF_OVERWRITE 0x02
#define PREF_SHOWROM   0x04
#define PREF_MODE      0x08

#define PREF_SORT      0x30

/* preferences */
struct SM_Preferences {
  unsigned char version;  /* preferneces version */
  unsigned char flags;    /* settings */
  unsigned char view;     /* palm/card view */
  unsigned char mode;     /* copy/move/info/del */
  char path[256];

  /* new options for backup */
  Boolean exclude_enable[4][2];
  unsigned long exclude[4][2];
  Boolean auto_backup;

  /* last successful full backup */
  ULong backup_time;
};

extern struct SM_Preferences prefs;
extern WinHandle draw_win;
extern UInt LibRef;

extern void *GetObjectPtr(FormPtr frm, Word object);

#ifdef DEBUG
#ifdef DEBUG_RS232
#include <SerialMgr.h>

extern UInt ComRef;

#define DEBUG_MSG(format, args...) { UInt debug_err; char debug_tmp[64]; StrPrintF(debug_tmp, format, ## args); SerSend(ComRef, debug_tmp, StrLen(debug_tmp), &debug_err); }
#else
#define DEBUG_PORT 0x30000000
#define DEBUG_MSG(format, args...) { char tmp[128]; int xyz; StrPrintF(tmp, format, ## args); for(xyz=0;xyz<StrLen(tmp);xyz++) *((char*)DEBUG_PORT)=tmp[xyz]; }
#endif
#else
#define DEBUG_MSG(format, args...)
#endif

#endif
