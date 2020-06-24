/*
  debug.h
*/

#ifndef DEBUG_H
#define DEBUG_H

#include <HostControl.h>

extern HostFILE *hostFile;

extern void debug_init (void);
extern void debug_free (void);

#ifdef DEBUG
#define DEBUG_INIT  debug_init()
#define DEBUG_FREE  debug_free()
#define DEBUG_PRINTF(format, args...) HostFPrintF(hostFile, format, ## args)
#else
#define DEBUG_INIT
#define DEBUG_FREE
#define DEBUG_PRINTF(format, args...)
#endif

#endif // DEBUG_H
