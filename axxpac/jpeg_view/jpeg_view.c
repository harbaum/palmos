/* 
   jpeg_view.c - (c) 2000 by Till Harbaum

   axxPac JPEG plugin library
*/

#ifdef OS35HDR
#include <PalmOS.h>
#include <PalmCompatibility.h>
#else
#include <Pilot.h>
#endif

#include "../api/axxPac.h"
#include "jinclude.h"

/* Dispatch table declarations */
ULong install_dispatcher(UInt,SysLibTblEntryPtr);
Ptr   *gettable(void);

ULong 
start (UInt refnum, SysLibTblEntryPtr entryP) {
  ULong ret;

  asm("
        movel %%fp@(10),%%sp@-
        movew %%fp@(8),%%sp@-
        jsr install_dispatcher(%%pc)
        movel %%d0,%0
        jmp out(%%pc)
gettable:
        lea jmptable(%%pc),%%a0
        rts

        .set FUNC_NO, 7             | number of library functions

jmptable:
        dc.w (8*FUNC_NO+2)

        # offsets to the system function calls
        dc.w ( 0*4 + 2*FUNC_NO + 2)  | open
        dc.w ( 1*4 + 2*FUNC_NO + 2)  | close
        dc.w ( 2*4 + 2*FUNC_NO + 2)  | sleep
        dc.w ( 3*4 + 2*FUNC_NO + 2)  | wake

        # offsets to the user function calls
        dc.w ( 4*4 + 2*FUNC_NO + 2)
        dc.w ( 5*4 + 2*FUNC_NO + 2)
        dc.w ( 6*4 + 2*FUNC_NO + 2)

        # functions for system use
        jmp PluginDummy(%%pc)
        jmp PluginDummy(%%pc)
        jmp PluginDummy(%%pc)
        jmp PluginDummy(%%pc)

        # functions for application use
        jmp PluginGetInfo(%%pc)
        jmp PluginExec(%%pc)
        jmp PluginConfig(%%pc)

        .asciz \"axxPac JPEG plugin\"
    .even
    out:
        " : "=r" (ret) :);
  return ret;
}

/* routine to install library into system */
ULong 
install_dispatcher(UInt refnum, SysLibTblEntryPtr entryP) {
  Ptr *table = gettable();
  entryP->dispatchTblP = table;
  entryP->globalsP = NULL;
  return 0;
}

/* this function is always fine */
Err 
PluginDummy(UInt refnum) {
  return 0;
}

/* interface to plugins */
typedef struct {
  char extension[4];    /* file extension (jpg, gif, ...) */
  char icon[36];
  char name[32];
} ExplorerPlugin;

const char icon[36] = { 
  /* header ... */
  0x00, 0x0A, 0x00, 0x0A,  0x00, 0x02, 0x00, 0x00,  
  0x01, 0x01, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
  
  /* data ... */
  0x00,0x00, 0x00,0x00,  0xFE,0x00, 0xAA,0x00,  
  0xFE,0x00, 0xFE,0x00,  0xFE,0x00, 0xAA,0x00,
  0xFE,0x00, 0x00,0x00 
};

Err
PluginGetInfo(UInt refNum, ExplorerPlugin *plugin) {
  StrCopy(plugin->extension, "jpg");
  MemMove(plugin->icon, icon, 36);
  StrCopy(plugin->name, "JPEG image viewer (jpg)");

  return 0;   /* ok */
}

extern void jpeg_view(UInt LibRef, axxPacFD fd, char *name, long length);

Err 
PluginExec(UInt refNum, UInt axxPacRef, axxPacFD fd, char *name, long length) {
  DEBUG_MSG("PLUGIN for %ld bytes!!\n", length);

  jpeg_view(axxPacRef, fd, name, length);
  
  return 0;   /* don't do any error handling, just return */
}

Err 
PluginConfig(UInt refNum) {
  DEBUG_MSG("PLUGIN for %ld bytes!!\n", length);

  jpeg_config();
  
  return 0;   /* don't do any error handling, just return */
}
