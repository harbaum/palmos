/*
  beam.c - beam routines for axxPac

  (c) 2000 Till Harbaum

  beam prc/pdb databases to other palms
*/

#ifdef OS35HDR
#include <PalmOS.h>
#include <PalmCompatibility.h>
#else
#include <Pilot.h>
#include <ExgMgr.h>
#endif

#include "../api/axxPac.h"

#include "explorerRsc.h"
#include "explorer.h"
#include "callback.h"
#include "beam.h"
#include "db.h"

#define BEAM_CHUNK 4096

static Err  BeamBytes(ExgSocketPtr s, void *buffer, 
		      unsigned long bytesToSend) {
  Err err = 0;
  
  while (!err && bytesToSend > 0) {
    unsigned long bytesSent = ExgSend(s, buffer, bytesToSend, &err);
    bytesToSend -= bytesSent;
    buffer = ((char *) buffer) + bytesSent;
  }
  return err;
}

void 
file_beam(char *name, long length) {
  Err err;
  char *buffer;
  axxPacFD fd;
  ExgSocketType s;
  ULong bytes2copy;

  buffer = MemPtrNew(BEAM_CHUNK);
  if(buffer == 0) return;

  fd = axxPacOpen(LibRef, name, O_RdOnly);

  MemSet(&s, sizeof(s), 0);
  s.description = name;
  s.target = 'lnch';        /* beam to launcher */
  s.length = 1;

  if (!ExgPut(&s)) {
    while(length>0) {
      bytes2copy = (length>BEAM_CHUNK)?BEAM_CHUNK:length;
      axxPacRead(LibRef, fd, (void*)buffer, bytes2copy);
      err = BeamBytes(&s, (void*)buffer, bytes2copy);
      length -= bytes2copy;
    }
    err = ExgDisconnect(&s, err);
  }

  axxPacClose(LibRef, fd);
  MemPtrFree(buffer);
}
