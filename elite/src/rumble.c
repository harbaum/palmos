
#include <PalmOS.h>
#include "elite.h"
#include "rumble.h"
#include "rumble/rumble_lib.h"
#include "debug.h"
#include "options.h"

UInt16 RumbleRef=0;
UInt16 rumble_cnt=0;

Boolean
rumble_open(void) 
{
  Err err;

  /* try to open tilt sensor lib */
  err = SysLibFind(RUMBLE_NAME, &RumbleRef);
  if (err) err = SysLibLoad('libr', RUMBLE_CREATOR, &RumbleRef);

  if(!err) {
    /* try to initialize sensor */
    err = RumbleLibOpen(RumbleRef);
    if(err != 0) {
      RumbleRef = 0;
      DEBUG_PRINTF("cannot install rumble support\n");
      return 0;
    }
  } else {
    RumbleRef = 0;
    DEBUG_PRINTF("cannot load rumble lib\n");
    return 0;
  }
  return 1;
}

void rumble_close(void)
{
  UInt16 numapps;

  if(RumbleRef != 0) {
    /* close sensor lib */
    RumbleLibClose(RumbleRef, &numapps);

    /* Check for errors in the Close() routine */
    if (numapps == 0) {
      SysLibRemove(RumbleRef);
    }

    RumbleRef = 0;
  }
}

void rumble(Bool on)
{
  if(RumbleRef != 0) { 
    RumbleLibRumble(RumbleRef, on);
    RumbleLibLED(RumbleRef, on);

    rumble_cnt = 3;
  }
}
