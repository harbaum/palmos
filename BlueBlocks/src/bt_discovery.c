/*
  bt_discovery.c

  a more user friendly discovery dialog
  
  (c) 2003 by Till Harbaum, BeeCon GmbH

*/

#include <PalmOS.h>
#include <BtLib.h>
#include <BtLibTypes.h>

#include "Rsc.h"
#include "bt.h"
#include "bt_error.h"
#include "bt_discovery.h"
#include "bt_discovery_db.h"
#include "bt_discovery_ui.h"
#include "bt_discovery_pb.h"
#include "debug.h"

// Application state flags
Boolean gInquiring = false;
Boolean gCanceling = false;
Boolean gDiscovering = false;
Boolean gSuccessfulDiscovery = false;

// number of devices that have to be named
UInt16 gcUnnamedDevices = 0;
UInt16 gcUnnamedDevicesRemaining = 0;
UInt16 giCurrentDevice = 0;

// Bluetooth constants
#define BT_INQUIRY_TIME 10.24

Char *bt_get_name(BtLibDeviceAddressType *bdaddr, BtLibGetNameEnum mode) {
  static BtLibFriendlyNameType gName;
  static Char gNameBuffer[btLibMaxDeviceNameLength];
  Err error;

  MemSet(&gName, sizeof(gName), 0);
  gName.name = (UInt8 *)gNameBuffer;
  gName.nameLength = btLibMaxDeviceNameLength;
  error = BtLibGetRemoteDeviceName(btLibRefNum, bdaddr, &gName, mode);
  
  ErrFatalDisplayIf((btLibErrPending != error) && (error), 
		    "Couldn't get remote device's name");

  return gNameBuffer;
}

/* fetch a name of a device */
void bt_name_device(void) {
  Err error;
  UInt32 progress;

  DEBUG_BLUE_PRINTF("bt_name_device()\n");
  
  // Start the process
  if(!gDiscovering) {
    gDiscovering = true;

    // Get unnamed device totals
    gcUnnamedDevices = 0;
    for (giCurrentDevice = 0; giCurrentDevice < gcDevices; giCurrentDevice++) {
      if ((gDevices[giCurrentDevice].state & (DEV_VISIBLE | DEV_NAMED)) == 
	  DEV_VISIBLE) {
	gcUnnamedDevices++;
      }
    }
    DEBUG_BLUE_PRINTF("unnamed devices: %d\n", gcUnnamedDevices);

    /* currently no device has been named */
    gcUnnamedDevicesRemaining = gcUnnamedDevices;
    giCurrentDevice = 0;
  }
  
   // Find the next device that is unnamed and near
   while ((giCurrentDevice < gcDevices) &&
          ((gDevices[giCurrentDevice].state & (DEV_VISIBLE | DEV_NAMED)) != 
	   DEV_VISIBLE)) {
     giCurrentDevice++;
   }

   // Out of devices?
   if (giCurrentDevice >= gcDevices) {

      // Progress bar to full
     DEBUG_BLUE_PRINTF("all devices done\n");
     bt_discovery_pb(false, 100);
     return;
   }
   
   // Update the progress bar
   progress = 50 + ((50 * (gcUnnamedDevices - 
			   gcUnnamedDevicesRemaining)) / gcUnnamedDevices);
   DEBUG_BLUE_PRINTF("progress set value %d\n", progress);
   bt_discovery_pb(false, progress);

   // Get the name
   bt_get_name(&(gDevices[giCurrentDevice].bdaddr), btLibRemoteOnly);
}

/* bluetooth event handler used during inquiry */
void bt_discovery_evt_handler(BtLibManagementEventType *mEventP, 
			      UInt32 refCon) {
  switch(mEventP->event) {
    case btLibManagementEventInquiryResult:
      DEBUG_BLUE_PRINTF("BtEvent: InquiryResult\n");

      // If your application filters by class of device or Bluetooth
      // address, that logic goes here.
      gSuccessfulDiscovery = true;

      DevDBAdd(&(mEventP->eventData.inquiryResult.bdAddr),
               mEventP->eventData.inquiryResult.classOfDevice,
               DEV_VISIBLE);
      break;
      
    case btLibManagementEventRadioState:
      //  Report radio failures
      if (btLibErrRadioInitFailed == mEventP->status &&
          btLibErrRadioFatal == mEventP->status &&
          btLibErrRadioSleepWake == mEventP->status) {
	ErrFatalDisplay("Radio initialization failed");
      }
      break;
      
    case btLibManagementEventInquiryCanceled:
      
      DEBUG_BLUE_PRINTF("BtEvent: Inquiry Cancelled\n");
      
      // Shut down the Bluetooth library. Closing the library while
      // handling a management event would cause a crash, so set a
      // state flag and handle it the next time we reach
      // AppHandleEvent().
      gCanceling = true;
      break;
     
    case btLibManagementEventInquiryComplete: 
      DEBUG_BLUE_PRINTF("BtEvent: Inquiry Complete\n");
      
      gInquiring = false;
      bt_name_device();
      break;
      
    case btLibManagementEventNameResult: {
      BtLibFriendlyNameType *nameP;
      UInt16 len;
      
      DEBUG_BLUE_PRINTF("BtEvent: Name Result\n");
      
      // Name discovery succeeded
      if (btLibErrNoError == mEventP->status) {
	nameP = &(mEventP->eventData.nameResult.name);
	DEBUG_BLUE_PRINTF("db set name %s\n", nameP->name);
	DevDBSetName(giCurrentDevice, (Char *)nameP->name, 
		     StrLen((Char *)nameP->name));	
      }
      
      // Move on to the next unnamed device
      giCurrentDevice++;
      gcUnnamedDevicesRemaining--;
      bt_name_device();
      break;
    }

    default:
      break;
  }  
  return;
}

/* start a bluetooth discovery */
void bt_discovery_start(void) {
  Err error;

  DevDBInit();

  /* do nothing if inquiry is already in progress */
  if (gInquiring) {
    DEBUG_BLUE_PRINTF("inquiry already in progress\n");
    return;
  }

  gInquiring = true;

  /* mark all devices in cache as not been seen yet */
  for (giCurrentDevice = 0; giCurrentDevice < gcDevices; giCurrentDevice++) {
    gDevices[giCurrentDevice].state &= ~DEV_VISIBLE;
  }
  
  // Register the callback function
  BtLibRegisterManagementNotification(btLibRefNum, 
				      bt_discovery_evt_handler, 0);

  // Initiate the inquiry
  error = BtLibStartInquiry(btLibRefNum, 0, 255);
  ErrFatalDisplayIf((btLibErrPending != error), "Failed to initiate inquiry");

  bt_discovery_ui_draw();

  /* stop inquiry if still in progress */
  if(gInquiring)
    BtLibCancelInquiry(btLibRefNum);

  // Unregister the callback function
  BtLibRegisterManagementNotification(btLibRefNum, 0, 0);

  DevDBShutdown();
}

