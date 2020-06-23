/*
  debug.h
*/

#ifndef DEBUG_H
#define DEBUG_H

#include <HostControl.h>

extern HostFILE *hostFile;

extern void debug_init (void);
extern void debug_free (void);

#define DEBUG_NORM    "\033[m"
#define DEBUG_RED     "\033[0;31m"
#define DEBUG_GREEN   "\033[0;32m"
#define DEBUG_BROWN   "\033[0;33m"
#define DEBUG_BLUE    "\033[0;34m"
#define DEBUG_MAGENTA "\033[0;35m"
#define DEBUG_CYAN    "\033[0;36m"
#define DEBUG_GREY    "\033[0;37m"
#define DEBUG_DGREY   "\033[1;30m"

#ifdef DEBUG
#define DEBUG_INIT  debug_init()
#define DEBUG_FREE  debug_free()
#define DEBUG_PRINTF(format, args...) HostFPrintF(hostFile, DEBUG_NORM format, ## args)
#define DEBUG_BLUE_PRINTF(format, args...) HostFPrintF(hostFile, DEBUG_BLUE format, ## args)
#define DEBUG_RED_PRINTF(format, args...) HostFPrintF(hostFile, DEBUG_RED format, ## args)
#define DEBUG_GREEN_PRINTF(format, args...) HostFPrintF(hostFile, DEBUG_GREEN format, ## args)

#else
#define DEBUG_INIT
#define DEBUG_FREE
#define DEBUG_PRINTF(format, args...)
#define DEBUG_BLUE_PRINTF(format, args...)
#define DEBUG_RED_PRINTF(format, args...)
#define DEBUG_GREEN_PRINTF(format, args...)
#endif

#endif // DEBUG_H
