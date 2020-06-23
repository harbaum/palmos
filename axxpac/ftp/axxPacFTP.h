#ifndef __AXXPACFTP_H__
#define __AXXPACFTP_H__

#define MAX_NAME 64

#define PREFS_VERSION 2

struct prefs {
  int  version;
  char server[MAX_NAME];
  char user[MAX_NAME];
  char pass[MAX_NAME];
  char path[256];
  Boolean close;
};

extern struct prefs prefs;

#define noDEBUG_RS232

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
