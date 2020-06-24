/*
  debug.c
*/

#include "debug.h"

#ifdef DEBUG
#include <HostControl.h>
#include <stdarg.h>

HostFILE *hostFile = NULL;

#define LOGFILE "/dev/tty"

void
debug_init (void)
{
  if (hostFile == NULL)
    hostFile = HostFOpen (LOGFILE, "w");

  DEBUG_PRINTF ("Console debugging enabled\n");
}

void
debug_free (void)
{
  if (hostFile != NULL)
    HostFClose (hostFile);
}

#endif
