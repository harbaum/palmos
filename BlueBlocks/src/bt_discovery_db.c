/*
  bt_discovery_db.c

  database routine for more user friendly discovery dialog
  
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

#include "debug.h"

// Favorites database
DmOpenRef gFavsDB = NULL;

// Array of discovered devices
bt_discovery_device_t *gDevices = NULL;
// Number of entries in gDevices[] array
UInt16 gcDevices = 0;

/* check if name is neither Unnamed nor Unknown */
static Boolean DevDBIsNamed(Char *name, UInt16 len) {
   const Char UNNAMED[] = "Unnamed ";
   const Char UNKNOWN[] = "Unknown ";

   if (len < sizeof(UNNAMED)-1) return false;
   if (0 == StrNCompare(UNNAMED, name, sizeof(UNNAMED)-1)) return false;
   if (0 == StrNCompare(UNKNOWN, name, sizeof(UNKNOWN)-1)) return false;

   return true;
}

void DevDBAddToFavorites(UInt16 iDevice, UInt16 iRec) {
  bt_discovery_dbentry_t rec;
  UInt16 len;
  MemHandle recH;
  void *recP;
  
  // Check if it's already in the favorites
  if ((gDevices[iDevice].state & DEV_FAVORITE) == DEV_FAVORITE) {
    return;
  }
  
  // Prepare the data
  MemMove(&(rec.bdaddr), &(gDevices[iDevice].bdaddr), sizeof(rec.bdaddr));
  rec.classOfDevice = gDevices[iDevice].classOfDevice;
  len = StrLen((Char *)gDevices[iDevice].name);
  
  // Write the new record
  // record size is -1 for name element, +1 for terminating NULL
  recH = DmNewRecord(gFavsDB, &iRec, sizeof(rec) + len);
  ErrFatalDisplayIf(!recH, "Out of memory");
  recP = MemHandleLock(recH);
  DmWrite(recP, 0, &rec, sizeof(rec) - 1);
  DmWrite(recP, offsetof(bt_discovery_dbentry_t, name), 
	  gDevices[iDevice].name, len + 1);
  MemHandleUnlock(recH);
  DmReleaseRecord(gFavsDB, iRec, true);
  
  // Add the favorites attribute
  gDevices[iDevice].state |= DEV_FAVORITE;   
}

static Int16 DevDBCompareAddrs(BtLibDeviceAddressType *addr1P, 
			       BtLibDeviceAddressType *addr2P) {
  UInt8 i;
  UInt16 retval;
  
  for(i = 0; i<btLibDeviceAddressSize; i++) {
    retval = (Int16)(addr1P->address[i]) - (Int16)(addr2P->address[i]);
    if (0 != retval) return retval;
  }
  return 0;
}

static UInt16 DevDBFindFavoriteRecord(BtLibDeviceAddressType *bdaddrP) {
  UInt16 iRec;
  UInt16 numRecs;
  MemHandle recH;
  bt_discovery_dbentry_t *recP;
  Int16 result;

  numRecs = DmNumRecords(gFavsDB);
  for (iRec = 0; iRec < numRecs; iRec++) {
    recH = DmQueryRecord(gFavsDB, iRec);
    recP = (bt_discovery_dbentry_t*)MemHandleLock(recH);
    result = DevDBCompareAddrs(bdaddrP, &(recP->bdaddr));
    MemHandleUnlock(recH);
    if (0 == result) {
      return iRec;
    }
  }
  
  ErrFatalDisplay("Favorites record not found");
  return iRec;
}


void DevDBSetName(UInt16 iDevice, Char *name, UInt16 len) {
  Err error;
  UInt16 iRec;
  
  // Update the name in the device list
  if (gDevices[iDevice].name) {
    MemPtrFree(gDevices[iDevice].name);
  }
  gDevices[iDevice].name = MemPtrNew(len+1);
  ErrFatalDisplayIf(!gDevices[iDevice].name, "Out of memory");
  MemMove(gDevices[iDevice].name, name, len);
  gDevices[iDevice].name[len] = '\0';
  
  if (DevDBIsNamed(name, len)) {
    gDevices[iDevice].state |= DEV_NAMED;
  } else {
    gDevices[iDevice].state &= ~DEV_NAMED;
  }
  
  // Now update the name in the favorites database
  if ((gDevices[iDevice].state & DEV_FAVORITE) == DEV_FAVORITE) {
    iRec = DevDBFindFavoriteRecord(&(gDevices[iDevice].bdaddr));
    error = DmRemoveRecord(gFavsDB, iRec);
    ErrFatalDisplayIf(error, "Couldn't delete favorites record");
    gDevices[iDevice].state &= ~DEV_FAVORITE;
    DevDBAddToFavorites(iDevice, iRec);
  }
}

Boolean DevDBInit(void) {
  Err error;
  LocalID lid;
  UInt16 attr;
  UInt16 version;
  UInt32 type;
  UInt32 creator;
  UInt16 numRecs;
  MemHandle recH;
  bt_discovery_dbentry_t *recP;
  UInt16 i;
  UInt16 len;
  
  if (0 == (lid = DmFindDatabase(0, DBNAME))) {
    DEBUG_BLUE_PRINTF("creating new \""DBNAME"\"\n");

    error = DmCreateDatabase(0, DBNAME, CREATOR, DBTYPE, false);
    ErrFatalDisplayIf(error, "Couldn't create favorites database");
    lid = DmFindDatabase(0, DBNAME);
    attr = dmHdrAttrBackup;
    version = DBVER;
    error = DmSetDatabaseInfo(0, lid, NULL, &attr, &version, NULL,
			      NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    ErrFatalDisplayIf(error, "Couldn't set favorites database info");
  }
  
  DEBUG_BLUE_PRINTF("getting database info\n");
  error = DmDatabaseInfo(0, lid, NULL, NULL, &version, NULL, NULL,
			 NULL, NULL, NULL, NULL, &type, &creator);

  if((version != DBVER) || (type != DBTYPE) || (creator != CREATOR)) {
    // Delete the invalid database and try again
    error = DmDeleteDatabase(0, lid);
    ErrFatalDisplayIf(error, "Couldn't delete invalid favorites database");
    return DevDBInit();
  }
  
   gFavsDB = DmOpenDatabase(0, lid, dmModeReadWrite);
   ErrFatalDisplayIf(!gFavsDB, "Couldn't open favorites database");
   
   // Initialize the device list
   numRecs = DmNumRecords(gFavsDB);
   if (0 == numRecs) {
     return true;
   }
   
   for(i=0; i < numRecs; i++) {
//     gSuccessfulDiscovery = true;
     recH = DmQueryRecord(gFavsDB, i);
     recP = MemHandleLock(recH);
     // Don't set the DEV_FAVORITE flag because it's already in
     // there.
     DevDBAdd(&(recP->bdaddr), recP->classOfDevice, 0);
     
     // Now we have one name from the favorites database and one from
     // the Bluetooth stack's name cache. Determine which one is
     // accurate and update the other.
     len = StrLen((Char *)gDevices[gcDevices-1].name);
     if (DevDBIsNamed((Char *)gDevices[gcDevices-1].name, len)) {
       MemHandleUnlock(recH);
       error = DmRemoveRecord(gFavsDB, i);
       ErrFatalDisplayIf(error, "Couldn't delete favorites record");
       DevDBAddToFavorites(gcDevices-1, i);
     } else {
       len = StrLen((Char *)&recP->name);
       if (DevDBIsNamed((Char *)&recP->name, len)) {
	 DevDBSetName(gcDevices-1, (Char *)&recP->name, len);
       }
       MemHandleUnlock(recH);
       gDevices[gcDevices-1].state |= DEV_FAVORITE;
     }
   }
   return true;
}


/* add device to list */
void DevDBAdd(BtLibDeviceAddressType *bdaddrP,
	      BtLibClassOfDeviceType classOfDevice, UInt8 state) {
  Err error;
  Boolean bDuplicate;  // Is it already in the device list?
  Boolean bSame;       // Is the current device the same as the one being added?
  Boolean bUpdateUI;   // Does the UI need to be updated?

  bt_discovery_device_t *tempList;
  UInt16 i;
  UInt16 j;
  UInt16 len;
  Char *name;

  DEBUG_BLUE_PRINTF("DevDBAdd()\n");

  if(!gFavsDB) {
    DEBUG_BLUE_PRINTF("favorite database not open\n");    
    return;
  }

  // *******
  // Determine the name
  // *******
  name = bt_get_name(bdaddrP, btLibCachedOnly);
  len = StrLen(name);

  /* check if device has a name */
  if (DevDBIsNamed(name, len)) state |=  DEV_NAMED;
  else                         state &= ~DEV_NAMED;

  if(state & DEV_NAMED) {
    DEBUG_BLUE_PRINTF("device is named %s\n", name);
  }

  // *******
  // Check if the entry is a duplicate
  // *******
  bDuplicate = false;
  bUpdateUI = true;
  for(i=0; !bDuplicate && (i < gcDevices); i++) {
    bSame = true;
    for(j=0; bSame && (j < btLibDeviceAddressSize); j++) {
      if (gDevices[i].bdaddr.address[j] != bdaddrP->address[j]) {
	bSame = false;
	break;
      }
    }
    if (bSame) {
      bDuplicate = true;
      
      // Add any new attributes
      if ((gDevices[i].state & state)  == state) 
	bUpdateUI = false;

      gDevices[i].state |= state;
      break;
    }
  }
  
  // *******
  // Add it to the list of devices
  // *******
  if (!bDuplicate) {
    // Enlarge the array
    tempList = (bt_discovery_device_t *)MemPtrNew((gcDevices+1)*sizeof(*gDevices));
    ErrFatalDisplayIf(!tempList, "Out of memory!");
    
    if (gcDevices) {
      MemMove(tempList, gDevices, gcDevices*sizeof(*gDevices));
      MemPtrFree(gDevices);
    }
    gDevices = tempList;
    
    // Add the new record
    for(j=0; j < btLibDeviceAddressSize; j++) {
      gDevices[gcDevices].bdaddr.address[j] = bdaddrP->address[j];
    }
    gDevices[gcDevices].classOfDevice = classOfDevice;
    gDevices[gcDevices].state = state;
    
    // Save the name
    gDevices[gcDevices].name = MemPtrNew(len+1);
    ErrFatalDisplayIf(!gDevices[gcDevices].name, "Out of memory");
    MemMove(gDevices[gcDevices].name, name, len);
    gDevices[gcDevices].name[len] = '\0';
    
    // device has been added to end of list */
    bt_discovery_ui_add(name, gcDevices);

    // Update the total
    gcDevices++;

  }
  
  // *******
  // Update the UI, if appropriate
  // *******
  if (bUpdateUI) {

//    bt_update_form();
  }
}

/* close device database and free memory */
void DevDBShutdown(void) {
  UInt16 curDevice;

  DEBUG_BLUE_PRINTF("freeing database\n");
  
  if(gFavsDB) {
    DmCloseDatabase(gFavsDB);
    gFavsDB = NULL;
 }
  
  // Deallocate globals
  for (curDevice = 0; curDevice < gcDevices; curDevice++) {
    if (gDevices[curDevice].name) {
      MemPtrFree(gDevices[curDevice].name);
    }
  }
  if (gDevices) {
    MemPtrFree(gDevices);
    gDevices = NULL;
    gcDevices = 0;
  }   
}
