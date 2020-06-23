/* 
   TrapHook.c  - (c) 2000 Till Harbaum

   routines for axxPac driver lib to hack into system traps
*/

#ifdef OS35HDR
#include <PalmOS.h>
#include <PalmCompatibility.h>
#else
#include <Pilot.h>
#endif

#include "axxPacDrv.h"
#include "../api/axxPac.h"

void TrapInitGlobals(Lib_globals *gl) {
  /* no valid trap saved */
  gl->trap.old = NULL;
}

void NewEvtGetEvent(EventPtr event, Long timeout)
{
  Lib_globals *gl;
  axxPacCardState state;

  /* get pointer to globals */
  FtrGet(CREATOR, FTR_GLOBS, (DWord*)&gl);

  /* limit timeout to allow frequent card checking */
  if((timeout > 10)||(timeout == evtWaitForever)) timeout = 10;

  /* jump into original trap */
  ((void(*)(EventPtr, Long))gl->trap.old)(event, timeout);

  /* handle power down and power up */
  /* is now done by axxPacWake in accPacDrv.c */

  /* check card status and try to catch remove/insert events */
  state = smartmedia_card_status(gl);

  /* optionally notify calling application about card state */
  if(gl->callback != NULL)
    gl->callback(state);

#ifdef DELAYED_CLOSE
  /* try to handle delayed close */
  if(gl->smartmedia.close_timeout > 0) {
    if(TimGetTicks() > gl->smartmedia.close_timeout) {
      smartmedia_force_close(gl);
      gl->smartmedia.close_timeout = 0;
    }
  }
#endif
}

/* hook into required system traps */
void TrapInstall(Lib_globals *gl) {

  /* save original trap */
  gl->trap.old = (DWord)SysGetTrapAddress(sysTrapEvtGetEvent);

  /* set new trap */
  SysSetTrapAddress(sysTrapEvtGetEvent, NewEvtGetEvent);
}

/* release system traps */
void TrapRemove(Lib_globals *gl) {
#ifdef DELAYED_CLOSE
  smartmedia_force_close(gl);
#endif

  if(gl->trap.old != 0) {
    /* restore original trap */
    SysSetTrapAddress(sysTrapEvtGetEvent, (void*)gl->trap.old);
  }
}
