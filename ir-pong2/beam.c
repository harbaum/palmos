/*

  beam.c - routines to beam the application itself

  (c) 2000 - Till Harbaum

*/

#include <PalmOS.h>
#include <PalmCompatibility.h>

#include <BtExgLib.h>

#include "Rsc.h"
#include "IR-Pong2.h"

static Err  BeamBytes(ExgSocketPtr s, void *buffer, ULong bytesToSend) {
   Err err = 0;
   
   while (!err && bytesToSend > 0) {
      ULong bytesSent = ExgSend(s, buffer, bytesToSend, &err);
      bytesToSend -= bytesSent;
      buffer = ((char *) buffer) + bytesSent;
   }
   return err;
}

Err
write_proc(const void *data, ULong *size, void *udata) {
  Err err;
  ExgSocketPtr s = (ExgSocketPtr)udata;

  /* send data ... */
  err = BeamBytes(s, (void*)data, *size);

  return err;
}

void beam_app(void) {
  ExgSocketType  s;
  Err      err;

  /* no beaming while game is running */
  if((state == SETUP)||(single_game)) {
    ir_close();

    MemSet(&s, sizeof(s), 0);
    s.description = "IR-Pong 2";
    s.target = 'lnch';
    s.length = 1;
    s.goToCreator = CREATOR;
    
    if (!ExgPut(&s)) {
      /* beam db */
      err = ExgDBWrite(write_proc, &s, s.description, 0, 0);
      err = ExgDisconnect(&s, err);
    }
    ir_open();  /* reopen irda */
  }
}

void send_app(void) {
#if 0
  ExgSocketType  s;
  Err err;
  UInt32 version;
  LocalID dbId = DmFindDatabase(0, "IR-Pong 2");

  /* no sending while game is running */
  if((state == SETUP)||(single_game)) {

    /* check if bluetooth exchange library is installed */
    err = FtrGet(btexgFtrCreator, btexgFtrNumVersion, &version);
    if(err != errNone) {
      FrmCustomAlert(alt_err, "Bluetooth exchange library not installed",0,0);
      return;
    }

    bt_close();
    ir_close();

    MemSet(&s, sizeof(s), 0);
    s.description = btexgSingleSufix "IR-Pong 2";
    s.target = 'lnch';
    s.length = 1;
    s.goToCreator = CREATOR;
    
    if (!ExgPut(&s)) {
      /* beam db */
      err = ExgDBWrite(write_proc, &s, NULL, dbId, 0);
      err = ExgDisconnect(&s, err);
    }

    ir_open();
    bt_open();
  }
#endif
}
