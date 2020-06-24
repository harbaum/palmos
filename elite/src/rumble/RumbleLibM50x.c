/*
  RumbleLibM50x.c

  Game Rumble driver for M50x
*/

#include <PalmOS.h>
#include "rumble_lib.h"

#define PORTBDATA  ((char*)0xfffff409l)
#define PORTBLED   0x40

#define PORTKDATA  ((char*)0xfffff441l)
#define PORTKVIB   0x10

/* Dispatch table declarations */
UInt32 install_dispatcher(UInt16, SysLibTblEntryPtr);
MemPtr *gettable(void);

typedef struct {
  Int16 refcount;
} Lib_globals;

UInt32 start (UInt16 refnum, SysLibTblEntryPtr entryP) {
  UInt32 ret;

  asm("
        movel %%fp@(10),%%sp@-
        movew %%fp@(8),%%sp@-
        jsr install_dispatcher(%%pc)
        movel %%d0,%0
        jmp out(%%pc)
gettable:
        lea jmptable(%%pc),%%a0
        rts

jmptable:
        dc.w 50            | 6n+2 = 50
        dc.w 18            | 2n+4i+2
        dc.w 22
        dc.w 26
        dc.w 30
        dc.w 34
        dc.w 38
        dc.w 42
        dc.w 46
        jmp open(%%pc)
        jmp close(%%pc)
        jmp sleep(%%pc)
        jmp wake(%%pc)
        jmp do_rumble(%%pc)
        jmp do_led(%%pc)
        jmp do_led(%%pc)   | unused1
        jmp do_led(%%pc)   | unused2
        .asciz \"Rumble Lib\"
    .even
    out:
        " : "=r" (ret) :);
  return ret;
}

UInt32 install_dispatcher(UInt16 refnum, SysLibTblEntryPtr entryP) {
  MemPtr *table = gettable();
  entryP->dispatchTblP = table;
  entryP->globalsP = NULL;
  return 0;
}

Boolean check_hw(void) {
  UInt32 ftr;
  Err err;

  /* check for correct cpu type */
  err = FtrGet(sysFtrCreator, sysFtrNumProcessorID, &ftr);
  if (err) return 0;
  
  /* check for vz cpu */
  if((ftr & sysFtrNumProcessorMask) != sysFtrNumProcessorVZ)
    return 0;

  /* check for vibrate feature */
  err = FtrGet(kAttnFtrCreator, kAttnFtrCapabilities, &ftr);
  if (err) return 0;

  /* check for vibration hw */
  if(!(ftr & kAttnFlagsHasVibrate))
    return 0;

  /* check for led */
  if(!(ftr & kAttnFlagsHasLED))
    return 0;
  
  return 1;
}

Err open(UInt16 refnum) {

  /* Get the current globals value */
  SysLibTblEntryPtr entryP = SysLibTblEntry(refnum);
  Lib_globals *gl = (Lib_globals*)entryP->globalsP;

  if (gl) {
    /* We are already open in some other application; just
       increment the refcount */
    gl->refcount++;
    return 0;
  }

  /* this driver currently supports 68VZ328/rumble hw */
  if(!check_hw())
    return RUMBLE_UNSUPPORTED;

  /* We need to allocate space for the globals */
  entryP->globalsP = MemPtrNew(sizeof(Lib_globals));
  MemPtrSetOwner(entryP->globalsP, 0);
  gl = (Lib_globals*)entryP->globalsP;

  /* Initialize the globals */
  gl->refcount = 1;

  /* sensor is fine */
  return 0;
}

Err close(UInt16 refnum, UInt16 *numappsP) {
  /* Get the current globals value */
  SysLibTblEntryPtr entryP = SysLibTblEntry(refnum);
  Lib_globals *gl = (Lib_globals*)entryP->globalsP;
  if (!gl) return 1;

  /* Clean up */
  *numappsP = --gl->refcount;
  if (*numappsP == 0) {
    MemChunkFree(entryP->globalsP);
    entryP->globalsP = NULL;
  }

  return 0;
}

Err sleep(UInt16 refnum) {
  /* Get the current globals value */
  SysLibTblEntryPtr entryP = SysLibTblEntry(refnum);
  Lib_globals *gl = (Lib_globals*)entryP->globalsP;
  if (!gl) return 1;

  return 0;
}

Err wake(UInt16 refnum) {
  /* Get the current globals value */
  SysLibTblEntryPtr entryP = SysLibTblEntry(refnum);
  Lib_globals *gl = (Lib_globals*)entryP->globalsP;
  if (!gl) return 1;

  return 0;
}

Err do_rumble(UInt16 refnum, Boolean on) {
  /* Get the current globals value */
  SysLibTblEntryPtr entryP = SysLibTblEntry(refnum);
  Lib_globals *gl = (Lib_globals*)entryP->globalsP;
  if (!gl) return 1; /* We're not open! */

  /* do rumble stuff by direct hardware access, */
  /* palmos does not provide direct access */
  if(on)  *PORTKDATA |=  PORTKVIB;
  else    *PORTKDATA &= ~PORTKVIB;

  return 0;
}

Err do_led(UInt16 refnum, Boolean on) {
  /* Get the current globals value */
  SysLibTblEntryPtr entryP = SysLibTblEntry(refnum);
  Lib_globals *gl = (Lib_globals*)entryP->globalsP;
  if (!gl) return 1; /* We're not open! */

  if(on)  *PORTBDATA |=  PORTBLED;
  else    *PORTBDATA &= ~PORTBLED;

  return 0;
}

