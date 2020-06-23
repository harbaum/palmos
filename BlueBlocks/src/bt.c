/*
  bluetooth communication routines 
*/

#include <PalmOS.h>
#include <System/DLServer.h>

#include <BtLib.h>
#include <BtLibTypes.h>

#include "Rsc.h"
#include "bt.h"
#include "debug.h"
#include "bt_error.h"

/* library reference */
UInt16 btLibRefNum = 0;

/* open the bluetooth library */
Boolean bt_init(void) {
  Err error;

  DEBUG_PRINTF("opening bluetooth library\n");  

  // Find the Library
  if( SysLibFind( btLibName, &btLibRefNum) ) {
    // load it if it can't be found	
    error = SysLibLoad( sysFileTLibrary , sysFileCBtLib, &btLibRefNum) ;
    btLibRefNum = 0;
    if (error) {
      FrmCustomAlert(alt_err, "Unable to load bluetooth library.",0,0);
      return false;
    }
  }

  // Open the library, but ensure, that no error message pops up
  if((error = BtLibOpen(btLibRefNum, true)) != btLibErrNoError) {
    if(error != btLibErrRadioInitFailed) btLibRefNum = 0;

    FrmCustomAlert(alt_err, "Unable to open bluetooth library.",0,0);
    return false;
  }

  // Register the management callback function
//  BtLibRegisterManagementNotification(btLibRefNum, BtLibMECallbackProc, 0);
  
  return true;
}

/* close the bluetooth library */
void bt_free(void) {
  DEBUG_PRINTF("closing bluetooth library\n");

  if (btLibRefNum!=0) {
    BtLibRegisterManagementNotification(btLibRefNum, 0, 0);
    BtLibClose(btLibRefNum);
    btLibRefNum = 0;
  }
}
