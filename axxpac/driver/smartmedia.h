/*
  smartmedia.h

  (c) 1999-2001 Till Harbaum
*/

#ifndef __SMARTMEDIA_H__
#define __SMARTMEDIA_H__

#define STORE_TAB_IN_DB  /* store allocation tables in a database */
#define DIRECT_WRITE

#define FORMAT_PANORAMA   // always create an olympus panorama card

#define TAB_DB_NAME  "axxPacTab"
#define TAB_DB_TYPE  'xTAB'

#define TAB_USED     0
#define TAB_FREE     1

#define noPALM3X      // final Palm IIIx hardware (now set by Makefile)

#define HW_VERSION 0

#define CARD_NONE    -2
#define CARD_UNKNOWN -1

#define MAX_ZONES     8    /* number of currently supported zones */
                           /* (1 zone = 16MBytes) */

#define noSMARTMEDIA_DEBUG
#define noDEBUG_SCAN

#define VENDOR_TOSHIBA 0x98
#define VENDOR_SAMSUNG 0xec

#define SMARTMEDIA_PASS (!(smartmedia_status()&0x01)) 
#define SMARTMEDIA_BUSY (!(smartmedia_status()&0x40)) 
#define SMARTMEDIA_PROT (!(smartmedia_status()&0x80)) 

/* format of 16 bytes additional data per sector */
struct extension {
  unsigned char  user_data[4];       /* bytes 512-515 (always 0xff) */
  unsigned char  user_status;        /* byte  516 (always 0xff) */
  unsigned char  block_status;       /* byte  517 (0xff if valid) */
  unsigned short block_address_1;    /* bytes 518/519 */
  unsigned char  ecc2[3];            /* bytes 520-522 */
  unsigned short block_address_2;    /* bytes 523/524 */
  unsigned char  ecc1[3];            /* bytes 525-527 */
} __attribute__ ((packed));

struct card_layout {
  unsigned char  size;    /* size in MBytes */
  unsigned char  spb;     /* sectors (a 512 bytes) per block */
  unsigned char  zones;   /* number of zones */

  /* fake harddisk parameters (needed for formatting the device) */
  unsigned short cyl;      /* cylinders */
  unsigned char  heads;    /* heads */
  unsigned char  sec;      /* sectors per track */
  unsigned long  lsec;     /* number of total logical sectors */
  unsigned long  psec;     /* number of total physical sectors */
  unsigned char  spf;      /* sectors per FAT */

  unsigned char  fmt[16];  /* 0: partition boot flag */
                           /* 1: format parameter start_head */
                           /* 2: format parameter start_sector */
                           /* 3: format parameter start_cylinder */
                           /* 4: partition type */
                           /* 5: format parameter end_head */
                           /* 6: format parameter end_sector */
                           /* 7: format parameter end_cylinder */  
                           /* 8,9,10,11: offset to partition boot record */
                           /* 12,13,14,15: total sectors */
};

struct card_type {
  unsigned char vendor;  /* vendor id    */
  unsigned char device;  /* device id    */
  unsigned char layout;  /* layout index */
  unsigned char dname;   /* device name  */
};

typedef struct {
#ifdef DELAYED_CLOSE
  ULong close_timeout;
#endif

  unsigned short ecc_tab[256];  /* tables for new ecc code */
  unsigned short par_tab[256];

  int use_cnt;

  int type;             /* force reload by erasing this one ...             */

  int *used_table;      /* pointer to translation table physical -> logical */
  int *free_table;      /* pointer to table of free sectors                 */

#ifdef STORE_TAB_IN_DB
  DmOpenRef dbRef;      /* reference of database to hold tables             */
#endif

  /* some additional info                                                   */
  int used[MAX_ZONES];  /* number of used blocks in zone                    */
  int free[MAX_ZONES];  /* number of free blocks in zone                    */

  int pblocks;          /* number of physical blocks on medium              */
  int lblocks;          /* number of logical blocks on medium               */
  int spb;              /* number of sectors per block                      */
  int zones;            /* number of zones                                  */

  int vendor, device;   /* current vendor and device                        */
  Boolean unusable;     /* true if media does not contain a valid fs        */

  unsigned char tmp[528];  /* buffer for sector copy                        */

  Boolean change;       /* card has just been changed                       */
} smartmedia_globals;

#endif



