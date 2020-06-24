/*
  limit.c

  routines for user/time limitation 
*/

/*
  m68k-palmos-gcc -O2 -D LIMIT_NAME -o limit.o -c limit.c
  m68k-palmos-objcopy -O binary limit.o limit

  The generated code has to contain 4e56ff (link a6,#-xx) 
  in the first few bytes. This is the entry point.
*/

#include <PalmOS.h>

int check_limit(void);

#ifdef LIMIT_NAME
#include <System/DLServer.h>

const char user_name[]=USER_NAME;

/* user name based limit */
int check_limit(void) {
  char name[dlkUserNameBufSize];

  DlkGetSyncInfo(0, 0, 0, name, 0, 0);
  return (StrCaselessCompare(name, user_name) == 0);
}
#endif

#ifdef LIMIT_TIME
#include "limit.h"

int check_limit(void) {
  return TimGetSeconds() < EXPIRE_TIME_SEC;
}

#endif
