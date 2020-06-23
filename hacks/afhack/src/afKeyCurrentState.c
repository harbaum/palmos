/*
  afKeyCurrentState.c   (c) 2000 by Till Harbaum

*/

#pragma pack(2)
#include <Common.h>
#include <System/SysAll.h>
#include <UI/UIAll.h>

#define NON_PORTABLE
#include <System/KeyMgr.h>
#include <Hardware/Hardware.h>
#include <System/Globals.h>
#include <System/SysEvtMgr.h>

DWord NewKeyCurrentState(void) {
  DWord orig, mask;
  Err err;

  /* jump to original routine */
  FtrGet(CREATOR, 1000, &orig);
  orig = ((DWord(*)(void))orig)();

  /* get button mask */
  err = FtrGet(CREATOR, 1001, &mask);
  if(err) mask = 0;

  /* mask if counter bits match */
  if((mask & 0xff00)&((mask & 0xff0000)>>8))
    orig &= ~(mask&0xff);

  /* increment magic counter */
  mask = (mask & 0xff00ffff) | ((mask+0x10000) & 0xff0000);

  /* and save it */
  FtrSet(CREATOR, 1001, mask);

  /* return alternating button presses */
  return orig;
}

