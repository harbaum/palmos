/*
  ghWinScreenMode.c   (c) 2000 by Till Harbaum

*/

#include <Pilot.h>
#include <ScrDriverNew.h>

Err NewWinScreenMode(ScrDisplayModeOperation operation,
		     DWordPtr widthP,
		     DWordPtr heightP,
		     DWordPtr depthP,
		     BooleanPtr enableColorP) {
  DWord orig, value;
  Err ret, err;

  /* jump to original routine */
  FtrGet(CREATOR, 1000, &orig);
  ret = ((Err(*)(ScrDisplayModeOperation, DWordPtr, DWordPtr, DWordPtr, BooleanPtr))orig)(operation, widthP, heightP, depthP, enableColorP);

  /* get current pixel oscillator value */
  err = FtrGet(CREATOR, 1001, &value);
  if(err) return ret;

  if(value & 0x00010000) {
    *((unsigned  char*)0xfffffa33) = value&0xff;
  } else {
    *((unsigned short*)0xfffffa32) = value&0xffff;
  }

  return ret;
}

