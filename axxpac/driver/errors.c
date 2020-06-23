/*
  errors.c - 2000 Till Harbaum

  axxPac error message handler
 */

#ifdef OS35HDR
#include <PalmOS.h>
#include <PalmCompatibility.h>
#else
#include <Pilot.h>
#endif

#include "axxPacDrv.h"
#include "../api/axxPac.h"

#include "errors.h"

#define AXXPAC_ERROR(code,string)    string "\0"

const char error_messages[] =
#include "errors.h"
;

void error_init_globals(Lib_globals *gl) {
  gl->error.smartmedia_err = ErrNone;   /* no error */
  gl->error.filesys_err    = ErrNone;   /* no error */
}

void 
error(Lib_globals *gl, Boolean type, Err err) {
  if(err < ErrLast) {
    if(type) gl->error.smartmedia_err = err;
    else     gl->error.filesys_err    = err;
  }
}

void 
error_reset(Lib_globals *gl) {
  gl->error.filesys_err = ErrNone;
  gl->error.smartmedia_err = ErrNone;
}

char*
error_getaddr(Err err) {
  int n=0, j=0;

  while(n != err) {
    while(error_messages[j] != '\0') j++;
    j++; n++;
  }
  return((char*)&error_messages[j]);
}

char*
error_string(Lib_globals *gl) {
  /* is there a low level error code? */
  if(gl->error.smartmedia_err != ErrNone)
    StrPrintF(gl->error.msg_buffer, "%s:\n%s.", 
	      error_getaddr(gl->error.filesys_err),
	      error_getaddr(gl->error.smartmedia_err));
  else
    StrPrintF(gl->error.msg_buffer, "%s.", 
	      error_getaddr(gl->error.filesys_err));
  
  return gl->error.msg_buffer;
}




