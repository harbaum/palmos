/*
  bt_discovery.h
*/

#ifndef BT_DISCOVERY_H
#define BT_DISCOVERY_H

#include <PalmOS.h>
#include <BtLib.h>
#include <BtLibTypes.h>

extern Boolean gInquiring;

/* structure used to save info about currently seen devices */
typedef struct {
  BtLibDeviceAddressType bdaddr;
  BtLibClassOfDeviceType classOfDevice;
  UInt8 state;
  Char *name;
} bt_discovery_device_t;

// Device state flags
#define DEV_VISIBLE    0x01   // device has actually been seen
#define DEV_FAVORITE   0x02   // device is a favority from database
#define DEV_NAMED      0x04   // devices name has been read
#define DEV_WRONG_COD  0x08   // devices class of device does not match

extern void bt_discovery_start(void);

/* internally used interfaces */
extern Char *bt_get_name(BtLibDeviceAddressType *bdaddr, BtLibGetNameEnum);

#endif // BT_DISCOVERY_H
