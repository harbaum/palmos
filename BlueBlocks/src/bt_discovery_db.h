/*
  
*/


#ifndef BT_DISCOVERY_DB_H
#define BT_DISCOVERY_DB_H

#include <PalmOS.h>
#include <BtLib.h>
#include <BtLibTypes.h>

#include "bt_discovery.h"

// name of database used to store the favorites
#define DBTYPE 'favs'
#define DBNAME APP " Bluetooth Favorites"
#define DBVER  1

#define offsetof(struct, member) ((UInt16)&(((struct *)0)->member))

/* structure to be used to store a device in the favorites database */
typedef struct {
  BtLibDeviceAddressType bdaddr;
  BtLibClassOfDeviceType classOfDevice;
  Char name[0];
} bt_discovery_dbentry_t;

// Array of discovered devices
extern bt_discovery_device_t *gDevices;
// Number of entries in gDevices[] array
extern UInt16 gcDevices;

extern Boolean DevDBInit(void);
extern void    DevDBShutdown(void);
extern void    DevDBAdd(BtLibDeviceAddressType *,
			BtLibClassOfDeviceType, UInt8);
extern void    DevDBSetName(UInt16 iDevice, Char *name, UInt16 len);


#endif // BT_DISCOVERY_DB_H
