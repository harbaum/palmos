/*
  smartmedia.c

  SmartMedia IO routines - (c) 1999 Till Harbaum

  Possible enhancements / ToDos:
  - recovery on correctable ECC errors

  Remarks:
  - current support is limited to cards up to 128 MBytes (8 zones)
*/

/* SmartMedia

   SmartMedia uses blocks to store information.
   Each block contains several sectors a 512 bytes with 16 bytes
   additional data each. A block can only be erased at once.
   
   Each sector uses the 16 additional bytes to store information
   about the logical block that is stored in a particular physical
   block, ECC bits etc.
*/

#ifdef OS35HDR
#include <PalmOS.h>
#include <PalmCompatibility.h>
#else
#include <Pilot.h>
#include <System/SysEvtMgr.h>
#endif

#include "axxPacDrv.h"
#include "../api/axxPac.h"

#include "smartmedia.h"
#include "errors.h"

#define noWAIT0            // define to enable 0 wait states (otherwise 1)
#define noFLASH_MODE       // define to use flash io mode
#define DISABLE_IRQS     // define to disable IRQs during media access
#define WRITE_VERIFY     // define this for read-after-write/copy-verify 
#define ERASE_VERIFY     // define this for erase verify
#define noDEBUG_ERASE
#define noDEBUG_SCAN

#define SKIP_WORKAROUND  // 0xff skipping because i don't knwo how to get 
                         // writing to extension-only working 

#define IMR         (*(volatile unsigned long*)0xfffff304l)

#ifdef PALM3X
#define BASE_ADDR 0x18000000l

#define DATA_REG_W  (*(volatile unsigned char*)(BASE_ADDR+0l))
#define DATA_REG_R  (*(volatile unsigned char*)(BASE_ADDR+1l))
#define CTRL_REG_W  (*(volatile unsigned char*)(BASE_ADDR+2l))
#define CTRL_REG_R  (*(volatile unsigned char*)(BASE_ADDR+3l))

#define DATA_OUT_PORT ((volatile unsigned char*)(BASE_ADDR+0l))
#define DATA_IN_PORT  ((volatile unsigned char*)(BASE_ADDR+1l))

#define CSGBB       (*(volatile unsigned short*)0xfffff102l)
#define CSB         (*(volatile unsigned short*)0xfffff112l)
#define PBSEL       (*(volatile unsigned short*)0xfffff40al)
#define PGSEL       (*(volatile unsigned short*)0xfffff432l)
#else
/* defines for PalmPro prototype */
#define DATA_REG_R  (*(volatile unsigned char*)0x10e00000l)
#define DATA_REG_W  (*(volatile unsigned char*)0x10e00000l)
#define CTRL_REG_R  (*(volatile unsigned char*)0x10e00001l)
#define CTRL_REG_W  (*(volatile unsigned char*)0x10e00001l)

#define DATA_IN_PORT  ((volatile unsigned char*)0x10e00000l)
#define DATA_OUT_PORT ((volatile unsigned char*)0x10e00000l)

#define CSA2        (*(volatile unsigned long*)0xfffff118l)
#define CSA3        (*(volatile unsigned long*)0xfffff11cl)

#endif

/* control port */
/* read */
#define CTRL_BUSY   (0x01)  /* card busy                                */
#define CTRL_INS    (0x02)  /* card insert detect      (final hw only)  */

/* write */
#ifdef PALM3X
#define VDD         (0x01)  /* VDD (slot pin 12/22)    (final hw only)  */
#else
#define VDD         (0)     /* prototype doesn't switch VDD             */
#endif

#define VSS         (0x02)  /* VSS (slot pin 1/10/11/18)                */
#define nCE         (0x04)  /* /CE (slot pin 21)                        */
#define CLE         (0x08)  /* CLE (slot pin 2)                         */
#define ALE         (0x10)  /* ALE (slot pin 3)                         */
#define WPOW        (0x20)  /* write prot pullup                        */
#define INSPOW      (0x40)  /* insert detection pullup (final hw only)  */
#define OEN         (0x80)  /* driver output enable    (final hw only)  */

#ifdef DEBUG
#define noDEBUG_TABLE   /* translation table creation debugging */
#define noDEBUG_TABLEX  /* extended translation table debugging */
#define noDEBUG_WRITE   /* sector write debugging               */
#define noDEBUG_SM_READ /* debug sector read                    */
#define noDEBUG_FREE    /* debug freeing of blocks              */
#define noDEBUG_XFREE   /* debug init free                      */
#endif

#define LBLK2ZONE(a) (a/1000) /* convert logical block number to zone id        */
#define LBLK2ZBLK(a) (a%1000) /* convert logical block number of block in zone  */
#define PBLK2ZONE(a) (a/1024) /* convert physical block number to zone id       */
#define PBLK2ZBLK(a) (a%1024) /* convert physical block number of block in zone */

#define LBLK2INDEX(a)  (1024*LBLK2ZONE(a)+LBLK2ZBLK(a))
#define PBLK2INDEX(a)  (a)  /* identical to (1024*PBLK2ZONE(a)+PBLK2ZBLK(a)) */

/* 256 * bits per byte for fast ecc/parity */
const char par_table[]={
 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5, /*   0- 31 */
 1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6, /*  32- 63 */
 1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6, /*  64- 95 */
 2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7, /*  96-127 */
 1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6, /* 128-159 */
 2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7, /* 160-191 */
 2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7, /* 192-223 */
 3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8  /* 224-255 */
};

void smartmedia_init_globals(Lib_globals *gl) {
  int i,j;

  /* pointer to translation table physical->logical */
  gl->smartmedia.used_table = NULL; 
  /* pointer to table of free sectors               */
  gl->smartmedia.free_table = NULL;
  /* true if media does not contain a valid fs      */
  gl->smartmedia.unusable = false;
  /* assume no card inserted                    */
  gl->smartmedia.type = axxPacCardNone;
  gl->smartmedia.use_cnt = 0;
  gl->smartmedia.change  = true;

#ifdef STORE_TAB_IN_DB
  /* lib is not open */
  gl->smartmedia.dbRef = 0;
#endif

#ifdef DELAYED_CLOSE
  gl->smartmedia.close_timeout = 0;  
#endif

  /* init ecc tables */
  for(i=0;i<256;i++) {
    /* extended parity table */
    if(par_table[i]&1) gl->smartmedia.par_tab[i]=0xffff;
    else               gl->smartmedia.par_tab[i]=0x0000;

    /* extended ecc table */
    gl->smartmedia.ecc_tab[i]=0;
    for(j=0;j<8;j++)
      if(i&(1<<j)) gl->smartmedia.ecc_tab[i]|=(2<<(2*j));
      else         gl->smartmedia.ecc_tab[i]|=(1<<(2*j));
  }
}

/* typical values for device are: */
/* 6b/e3/e5 for 4 MBytes */
/* e6 for 8 MBytes       */
/* 73 for 16 MBytes      */
/* 75 for 32 MBytes      */
/* 76 for 64 MBytes      */
/* 79 for 128 MBytes     */

const char devname[][12] = { 
  "unknown",
  "TC58V32ADC", "TC58V64DC", "TH58V128DC", "TC58V256DC", "TH58512DC", "TH58NS100DC", 
  "SMFV004",    "SMFV008",   "SMFV016",    "SMFV032"   , "SMFV064",   "K9Q1G08V0A" };

/* physical layout of different cards */
const struct card_layout card_layout[]={
  /* size, spb, zones, cyl, heads, spt,  lsec,  psec, spf  */
  /* starthd, startsec, startcyl, ptype, endhd, endsec, endcyl, pbr, size  */

  /* type 0:  4 mb card */
  {     4,  16, 1, 250,     4,   8,  8000,  8192,   2, 
       { 0x80, 0x03, 0x04, 0x00, 0x01, 0x03, 0x08, 0xf9, 
	     27,0,0,0, 0x25,0x1f,0x00,0x00 }},

  /* type 1:  8 mb card */
  {     8,  16, 1, 250,     4,  16, 16000, 16384,   3, 
       { 0x80, 0x01, 0x0a, 0x00, 0x01, 0x03, 0x10, 0xf9, 
	     25,0,0,0, 0x67,0x3e,0x00,0x00 }},

  /* type 2: 16 mb card */  
  {    16,  32, 1, 500,     4,  16, 32000, 32768,   3, 
       { 0x80, 0x02, 0x0a, 0x00, 0x01, 0x03, 0x50, 0xf3, 
	     41,0,0,0, 0xd7,0x7c,0x00,0x00 }},

  /* type 3: 32 mb card */
  {    32,  32, 2, 500,     8,  16, 64000, 65536,   6, 
       { 0x80, 0x02, 0x04, 0x00, 0x01, 0x07, 0x50, 0xf3, 
	     35,0,0,0, 0xdd,0xf9,0x00,0x00 }},

  /* type 4: 64 mb card */
  {    64,  32, 4, 500,     8,  32, 128000, 131072,  12, 
       { 0x80, 0x01, 0x18, 0x00, 0x01, 0x07, 0x60, 0xf3, 
	     55,0,0,0, 0xc9,0xf3,0x01,0x00 }},

  /* type 5: 128 mb card */
  {   128,  32, 8, 500,    16,  32, 256000, 262144,  32,
       { 0x80, 0x01, 0x10, 0x00, 0x06, 0x0f, 0x60, 0xf3, 
	     47,0,0,0, 0xd1,0xe7,0x03,0x00 }}
};

/* table of SmartMedia cards supported */
const struct card_type card_type[]={
  /* vnd,  dev, type, dname */

  /* supported unknown cards (vendor0 == any) */
  { 0x00,           0x6b,  0,  0 },
  { 0x00,           0xe3,  0,  0 },
  { 0x00,           0xe5,  0,  0 },
  { 0x00,           0xe6,  1,  0 },
  { 0x00,           0x73,  2,  0 },
  { 0x00,           0x75,  3,  0 },
  { 0x00,           0x76,  4,  0 },
  { 0x00,           0x79,  5,  0 },

  /* supported Toshiba cards */
  { VENDOR_TOSHIBA, 0xe5,  0,  1 },
  { VENDOR_TOSHIBA, 0xe6,  1,  2 },
  { VENDOR_TOSHIBA, 0x73,  2,  3 },
  { VENDOR_TOSHIBA, 0x75,  3,  4 },
  { VENDOR_TOSHIBA, 0x76,  4,  5 },
  { VENDOR_TOSHIBA, 0x79,  5,  6 },

  /* supported Samsung cards */
  { VENDOR_SAMSUNG, 0xe3,  0,  7 },
  { VENDOR_SAMSUNG, 0xe6,  1,  8 },
  { VENDOR_SAMSUNG, 0x73,  2,  9 },
  { VENDOR_SAMSUNG, 0x75,  3, 10 },
  { VENDOR_SAMSUNG, 0x76,  4, 11 },
  { VENDOR_SAMSUNG, 0x79,  5, 12 },

  { 0x00,              0,  0, 255 }
};

/* smartmedia commands */
#define SM_READ  0x00
#define SM_EXT   0x50
#define SM_WRITE 0x80
#define SM_RESET 0xff

void smartmedia_cmd(unsigned char cmd) {
  CTRL_REG_W = VDD | OEN | WPOW | CLE;  /* activate   CE/CLE */
  DATA_REG_W = cmd;
  CTRL_REG_W = VDD | OEN | WPOW | nCE;  /* deactivate CE/CLE */
}

void smartmedia_write_addr(unsigned char addr) {
  CTRL_REG_W = VDD | OEN | WPOW | ALE;  /* activate   CE/ALE */
  DATA_REG_W = addr;
  CTRL_REG_W = VDD | OEN | WPOW | nCE;  /* deactivate CE/ALE */
}

#define CMD_FUNCTION
#ifdef CMD_FUNCTION
void smartmedia_cmd_sector(unsigned char cmd, unsigned long sector) {
  unsigned char *c = ((unsigned char*)&sector)+3;

  CTRL_REG_W = VDD | OEN | WPOW | CLE;  /* activate   CE/CLE */
  CTRL_REG_W = VDD | OEN | WPOW | CLE;  /* activate   CE/CLE */
  DATA_REG_W = cmd;                     /* command */
  CTRL_REG_W = VDD | OEN | WPOW | ALE;  /* activate   ALE */
  DATA_REG_W = 0x00;                    /* intra sector offset */
  DATA_REG_W = *c--;
  DATA_REG_W = *c--;
  DATA_REG_W = *c--;
  CTRL_REG_W = VDD | OEN | WPOW;        /* deactivate ALE */
}
#else
#define smartmedia_cmd_sector(cmd, sector) {\
  CTRL_REG_W = VDD | OEN | WPOW | CLE;  /* activate   CE/CLE */\
  DATA_REG_W = cmd;                     /* command */\
  CTRL_REG_W = VDD | OEN | WPOW | ALE;  /* activate   ALE */\
  DATA_REG_W = 0x00;                    /* intra sector offset */\
  DATA_REG_W =  sector        & 0xff;\
  DATA_REG_W = (sector >> 8)  & 0xff;\
  DATA_REG_W = (sector >> 16) & 0xff;\
  CTRL_REG_W = VDD | OEN | WPOW;        /* deactivate ALE */\
}
#endif

/* this timeout is very long, but that doesn't really matter ... */
#define WAIT_TIMEOUT 100000

Boolean wait_while_busy(void) {
  unsigned long timeout = WAIT_TIMEOUT;
  while(timeout && (CTRL_REG_R & CTRL_BUSY)==0) timeout--;
  return(timeout);
}

/* shut down chip select */
#define SMARTMEDIA_OFF   { CTRL_REG_W = VDD | OEN | WPOW | nCE; /* deactivate CE */ }

unsigned char smartmedia_status(void) {
  unsigned char s;

  smartmedia_cmd(0x70);                       /* get status */
  CTRL_REG_W = VDD | OEN | WPOW;              /* activate CE */
  s = DATA_REG_R;                             /* read status byte */
  CTRL_REG_W = VDD | OEN | WPOW | nCE;        /* deactivate CE */
  return s;
}

#define BLOCK_DEFECT -1
#define BLOCK_FREE   -2

int smartmedia_get_parity(unsigned int value) {
  int par;

  par  = par_table[(value&0x00ff)];
  par ^= par_table[(value&0x0700)>>8];
  /* upper five bits are always 00010 */

  return par&1;
}

/* calculate 22 bit ecc for 256 bytes array src */
/* and store it in three bytes ecc */
void smartmedia_ecc(unsigned short *par_tab, unsigned short *ecc_tab, 
		    unsigned char *ecc, unsigned char *src) {

  int cnt;
  unsigned short xecc;
  unsigned char  becc;

  /* a little bit unrolled ... */
  for(becc=0xff,xecc=0xffff,cnt=127;cnt>=0;cnt--) {
    becc ^= *src;
    xecc ^= ((par_tab[*src++])&(*ecc_tab++));

    becc ^= *src;
    xecc ^= ((par_tab[*src++])&(*ecc_tab++));
  }
  
  ecc[2] = 0xff;
  /* P1/P1' */
  if(par_tab[becc&0x55]) ecc[2] &= ~0x04;
  if(par_tab[becc&0xaa]) ecc[2] &= ~0x08;
  /* P2/P2' */
  if(par_tab[becc&0x33]) ecc[2] &= ~0x10;
  if(par_tab[becc&0xcc]) ecc[2] &= ~0x20;
  /* P4/P4' */
  if(par_tab[becc&0x0f]) ecc[2] &= ~0x40;
  if(par_tab[becc&0xf0]) ecc[2] &= ~0x80;

  /* store ecc */
  ecc[0] = xecc&0xff;
  ecc[1] = (xecc>>8);
}

/* extract block status/id from redundant area */
int smartmedia_check_block_id(unsigned char *buffer) {
  struct extension *ext = (struct extension *)buffer; /* interpret buffer as extension */
  unsigned int block;

  /* invalid block if bit > 2 set */
  if((ext->block_status&0xfc)==0xfc) {

    /* both block numbers must be equal */
    if(ext->block_address_1 == ext->block_address_2) {

      block = ext->block_address_1;

      /* this is an unused block */
      if(block==0xffff) return BLOCK_FREE;
    
      /* upper five bits must be 00010 */
      if((block&0xf800)==0x1000) {
	
	/* valid parity set? */
	if(smartmedia_get_parity(block))
	  return (block&0x7ff)>>1;  /* block id 0..1023 */
      }
    }
  }

  return BLOCK_DEFECT;  /* don't use this block at all */
}

Boolean smartmedia_alloc_tables(Lib_globals *gl, int blocks) {
#ifndef STORE_TAB_IN_DB

  /* allocate used table */
  gl->smartmedia.used_table = MemPtrNew(sizeof(int)*blocks);
  DEBUG_MSG_MEM("alloc RAM used_table %ld = %lx\n", 
		sizeof(int)*blocks, gl->smartmedia.used_table);
  if(gl->smartmedia.used_table == NULL) return false;

  /* allocate free table */
  gl->smartmedia.free_table = MemPtrNew(sizeof(int)*blocks);
  DEBUG_MSG_MEM("alloc RAM free_table %ld = %lx\n", 
		sizeof(int)*blocks, gl->smartmedia.free_table);
  if(gl->smartmedia.free_table == NULL) {
    MemPtrFree(gl->smartmedia.used_table);
    gl->smartmedia.used_table = NULL;
    return false;
  }

  /* initialize used_table with -1 */
  MemSet(gl->smartmedia.used_table, sizeof(int)*blocks, 0xff);

  return true;
#else
  LocalID dbid;
  VoidHand used_record, free_record;

  /* create a database */
  DmCreateDatabase(0, TAB_DB_NAME, CREATOR, TAB_DB_TYPE, false);
 
  if((dbid = DmFindDatabase(0, TAB_DB_NAME))!=NULL) {
    UInt16 at;

    gl->smartmedia.dbRef = DmOpenDatabase(0, dbid, dmModeReadWrite);

    /* create two new new records */

    at = 0;
    if((used_record = DmNewRecord(gl->smartmedia.dbRef, &at, sizeof(int)*blocks))!=0) {
      at = 1;
      if((free_record = DmNewRecord(gl->smartmedia.dbRef, &at, sizeof(int)*blocks))!=0) {

	/* lock records in memory */
	gl->smartmedia.used_table = MemHandleLock(used_record);
	gl->smartmedia.free_table = MemHandleLock(free_record);

	DEBUG_MSG_MEM("alloc DB used_table %ld = %lx\n", 
		      sizeof(int)*blocks, gl->smartmedia.used_table);
	DEBUG_MSG_MEM("alloc DB free_table %ld = %lx\n", 
		      sizeof(int)*blocks, gl->smartmedia.free_table);

	/* initialize used_table with -1 */
	DmSet(gl->smartmedia.used_table, 0, sizeof(int)*blocks, 0xff);
	return true;
      }
    }
  }

  /* error! */

  /* close database */
  DmCloseDatabase(gl->smartmedia.dbRef);
  gl->smartmedia.dbRef = 0;

  /* and finally try to delete an existing database */
  if((dbid = DmFindDatabase(0, TAB_DB_NAME))!=0)
    DmDeleteDatabase(0, dbid);

  return false;
#endif
}

Boolean smartmedia_free_tables(Lib_globals *gl) {
#ifndef STORE_TAB_IN_DB
  if(gl->smartmedia.used_table != NULL) {
    MemPtrFree(gl->smartmedia.used_table);
    gl->smartmedia.used_table = NULL;
  }  

  if(gl->smartmedia.free_table != NULL) {
    MemPtrFree(gl->smartmedia.free_table);
    gl->smartmedia.free_table = NULL;
  }

  DEBUG_MSG("both RAM tables freed\n");
  return true;
#else
  LocalID dbid;

  /* close, unlock and remove database */
  if(gl->smartmedia.dbRef != 0) {
    /* unlock tables */
    MemPtrUnlock(gl->smartmedia.used_table);
    MemPtrUnlock(gl->smartmedia.free_table);

    /* release table records */
    DmReleaseRecord(gl->smartmedia.dbRef, 0, false);
    DmReleaseRecord(gl->smartmedia.dbRef, 1, false);

    /* close the database */
    DmCloseDatabase(gl->smartmedia.dbRef); 
    gl->smartmedia.dbRef = 0;

    /* tables aren't allocated anymore */
    gl->smartmedia.used_table = NULL;
    gl->smartmedia.free_table = NULL;

    DEBUG_MSG("both DB tables freed\n");
  }
    
  /* and finally try to delete an existing database */
  if((dbid = DmFindDatabase(0, TAB_DB_NAME))!=0)
    DmDeleteDatabase(0, dbid);

  return true;
#endif
}

void smartmedia_change_used_table(Lib_globals *gl, unsigned int entry, int value) {
#ifndef STORE_TAB_IN_DB
  gl->smartmedia.used_table[entry] = value;
#else
  DmWrite(gl->smartmedia.used_table, sizeof(int)*entry,&value,sizeof(int));
#endif
}

void smartmedia_change_free_table(Lib_globals *gl, unsigned int entry, int value) {
#ifndef STORE_TAB_IN_DB
  gl->smartmedia.free_table[entry] = value;
#else
  DmWrite(gl->smartmedia.free_table, sizeof(int)*entry,&value,sizeof(int));
#endif
}

/* read redundant area of first sector in each block, */
/* extract block nummer and store it in table         */
int smartmedia_scan_blocks(Lib_globals *gl,
			   int pblocks, 
			   int lblocks, 
			   int spb, int zones) {
  int i, no, id, zone, zblk;
  unsigned char buffer[16];
#ifdef DISABLE_IRQS
  unsigned long  irq_save;
#endif

#ifdef DEBUG_TABLE
  DEBUG_MSG("%d zones, %d pblocks, %d lblocks, %d spb\n", 
	    zones, pblocks, lblocks, spb);
#endif

  /* allocate block used/free tables */
  if(!smartmedia_alloc_tables(gl, pblocks)) return 1;

  gl->smartmedia.pblocks = pblocks;  /* save table length */
  gl->smartmedia.lblocks = lblocks;  /* save table length */
  gl->smartmedia.spb     = spb;      /* save number of sectors per block */
  gl->smartmedia.zones   = zones;    /* number of zones */

  for(i=0;i<zones;i++) {
    gl->smartmedia.free[i] = 0;      /* number of free blocks in this zone */
    gl->smartmedia.used[i] = 0;      /* number of used blocks in this zone */
  }

  DEBUG_MSG("sectors per block: %d\n", spb);

#ifdef DISABLE_IRQS
  irq_save = IMR;
  IMR = 0x00ffffff;
#endif

#ifdef DIRECT_WRITE
  MemSemaphoreReserve(true);
#endif

  /* scan all physical blocks (start at the end to behave like olympus 1400XL camera) */
  for(no=pblocks-1;no>=0;no--) {

    zone = PBLK2ZONE(no);   /* zone, block is in       */
    zblk = PBLK2ZBLK(no);   /* number of block in zone */

    /* read extension area */
    smartmedia_cmd_sector(SM_EXT, (long)no*(long)spb);

    wait_while_busy();

    /* now read redundant area */
    for(i=0;i<16;i++)  buffer[i] = DATA_REG_R;

    SMARTMEDIA_OFF;

    /* extract block no */
    id = smartmedia_check_block_id(buffer);

    /* valid block id */
    if(id >= 0 ) {
      // DEBUG_MSG("zone %d block %d allocated by %d\n", zone, no, id); 

      gl->smartmedia.used[zone]++;

#ifdef DEBUG
      if(gl->smartmedia.used_table[1024*zone + id] != -1)
	DEBUG_MSG("zone %d lblock %d allocated by %d and %d\n", zone, id, 
		  gl->smartmedia.used_table[1024*zone + id], no); 
#endif

#ifndef DIRECT_WRITE
      smartmedia_change_used_table(gl, 1024*zone+id, no);
#else
      gl->smartmedia.used_table[1024*zone+id] = no;
#endif

#ifdef DEBUG_SCAN
      DEBUG_MSG("found block %d at %d (zone=%d, zblk=%d)\n", no, id, zone, zblk);
#endif
    } else {
      /* free block -> store in table of free sectors */
      if(id == BLOCK_FREE) {
#ifdef DEBUG_XFREE
	DEBUG_MSG("index 1 %d/%d/%d marked free\n", zone, 1024*zone + gl->smartmedia.free[zone], no);
#endif

#ifndef DIRECT_WRITE
	smartmedia_change_free_table(gl, 1024*zone + gl->smartmedia.free[zone]++, no);
#else
	gl->smartmedia.free_table[1024*zone + gl->smartmedia.free[zone]++] = no;
#endif

#ifdef DEBUG_SCAN
      } else {
	DEBUG_MSG("block %d state: %d\n", no, id);
#endif
      }
    }
  }

#ifdef DIRECT_WRITE
  MemSemaphoreRelease(true);
#endif

#ifdef DISABLE_IRQS
  IMR = irq_save;
#endif

#ifdef DEBUG_TABLE
  DEBUG_MSG("Blocks total: P:%d/L:%d\n", gl->smartmedia.pblocks, gl->smartmedia.lblocks);
  for(i=0;i<zones;i++) {
    DEBUG_MSG("Zone %d ok, ", i);
    DEBUG_MSG("used: %d, free: %d\n", gl->smartmedia.used[i], gl->smartmedia.free[i]);
  }
#endif

  return 0;
}

int  smartmedia_init(Lib_globals *gl) {
  char msg[64];
  int  i,j;

  smartmedia_cmd(0x90);   /* get id */
  smartmedia_write_addr(0x00);  /* addr 0 */

  CTRL_REG_W = VDD | OEN | WPOW;            /* activate CE */
  gl->smartmedia.vendor = DATA_REG_R;       /* read vendor byte */
  gl->smartmedia.device = DATA_REG_R;       /* read device byte */
  CTRL_REG_W = VDD | OEN | WPOW | nCE;      /* deactivate CE */

  /* find card in table */
  i=0, j=-1;
  while(card_type[i].dname != 255) {
    if( (card_type[i].device == gl->smartmedia.device) &&
	((card_type[i].vendor == gl->smartmedia.vendor)||
	 (card_type[i].vendor == 0)))  j=i;
    i++;
  }

  return j;
}

static Boolean 
smartmedia_read_sec(Lib_globals *gl, char *data, 
		    long sector, long num, Boolean fast) {
  int i,k;
  volatile unsigned char *in=DATA_IN_PORT;  
  unsigned char ecc[3];
#ifdef DISABLE_IRQS
  unsigned long  irq_save;
#endif

  struct extension ext;
  char   *extp, *datap;

#ifdef DEBUG_SM_READ
  DEBUG_MSG("Read sec P:%ld/%ld\n", sector, num);
#endif

#ifdef DISABLE_IRQS
  irq_save = IMR;
  IMR = 0x00ffffff;
#endif

  smartmedia_cmd_sector(SM_READ, sector);

  /* optionally read several sectors at once */
  for(k=0;k<num;k++) {
    extp  = (char*)&ext;
    datap = data;

    wait_while_busy();

    /* now read whole sector (little bit unrolled) */
    for(i=0;i<128;i++) {
      *data++ = *in;
      *data++ = *in;
      *data++ = *in;
      *data++ = *in;
    }
    
    /* and read extension */
    for(i=0;i<4;i++) {
      *extp++ = *in;  
      *extp++ = *in;  
      *extp++ = *in;  
      *extp++ = *in;  
    }
    
    /* verify ecc */
    if(!fast) {
      smartmedia_ecc(gl->smartmedia.par_tab, gl->smartmedia.ecc_tab, 
		     ecc, datap);
    
      /* check first 256 byte block */
      if((ext.ecc1[0]!=ecc[0])||(ext.ecc1[1]!=ecc[1])||(ext.ecc1[2]!=ecc[2])) {
	SMARTMEDIA_OFF;
#ifdef DISABLE_IRQS
	IMR = irq_save;
#endif
	DEBUG_MSG("Wrong ecc1: %d/%d/%d %d/%d/%d\n", 
		  ext.ecc1[0], ext.ecc1[1], ext.ecc1[2],
		  ecc[0], ecc[1], ecc[2]);
	return false;
      }
    
      /* check second 256 byte block */
      smartmedia_ecc(gl->smartmedia.par_tab, gl->smartmedia.ecc_tab, 
		     ecc, &datap[256]);
      
      if((ext.ecc2[0]!=ecc[0])||(ext.ecc2[1]!=ecc[1])||(ext.ecc2[2]!=ecc[2])) {
	SMARTMEDIA_OFF;
#ifdef DISABLE_IRQS
	IMR = irq_save;
#endif
	DEBUG_MSG("Wrong ecc2: %d/%d/%d %d/%d/%d\n", 
		  ext.ecc2[0], ext.ecc2[1], ext.ecc2[2],
		  ecc[0], ecc[1], ecc[2]);
	return false;
      }
    }
  }

  SMARTMEDIA_OFF;

#ifdef DISABLE_IRQS
  IMR = irq_save;
#endif

  return true;
}

Err smartmedia_read(Lib_globals *gl, void *data, long sec_no, 
		    int len, Boolean fast) {

  int  sec, lblk_index, offset, run;
  long med_sec;

#ifdef DEBUG_SM_READ
  DEBUG_MSG("smartmedia_read(L:%ld/%d)\n", sec_no, len);
#endif

  if((gl->smartmedia.used_table == NULL)||
     (gl->smartmedia.free_table == NULL)) {
    error(gl, true, ErrNoTable);
    return -ErrNoTable;
  }

#ifdef NO_BURST_READ
  for(sec=0;sec<len;sec++) {
    /* exceed logical blocks? -> illegal block number */
    if( (sec_no/gl->smartmedia.spb) >= gl->smartmedia.lblocks) {
      error(gl, true, ErrSmNoSec);
      return -ErrSmNoSec;
    }

    lblk_index = LBLK2INDEX(sec_no/gl->smartmedia.spb);

    if( gl->smartmedia.used_table[lblk_index] == -1) {
      DEBUG_MSG("no such sector\n");
      error(gl, true, ErrSmNoSec);
      return -ErrSmNoSec;
    }

    /* get translated sector address */
    med_sec = ((long)gl->smartmedia.used_table[lblk_index] * 
	       (long)gl->smartmedia.spb) | (sec_no % gl->smartmedia.spb);

    if(!smartmedia_read_sec(gl, data, med_sec, 1, fast)) {
      error(gl, true, ErrSmECC);
      return -ErrSmECC;
    }
    data+=512;
    
    sec_no++;
  }  
#else
  /* exceed logical blocks? -> illegal block number */
  if( (sec_no/gl->smartmedia.spb) >= gl->smartmedia.lblocks) {
    error(gl, true, ErrSmNoSec);
    return -ErrSmNoSec;
  }

  /* offset within block */
  offset = sec_no % gl->smartmedia.spb;

  while(len>0) {
    lblk_index = LBLK2INDEX(sec_no/gl->smartmedia.spb);

    if( gl->smartmedia.used_table[lblk_index] == -1) {
      DEBUG_MSG("no such sector\n");
      error(gl, true, ErrSmNoSec);
      return -ErrSmNoSec;
    }

    /* limit run to one block */
    if(offset+len > gl->smartmedia.spb)  run = gl->smartmedia.spb - offset;
    else run = len;

#ifdef DEBUG_SM_READ
    DEBUG_MSG("run: %ld/%d sectors\n", sec_no, run);
#endif

    /* get translated sector address */
    med_sec = ((long)gl->smartmedia.used_table[lblk_index] * 
	       (long)gl->smartmedia.spb) | (long)offset;

    if(!smartmedia_read_sec(gl, data, med_sec, run, fast)) {
      error(gl, true, ErrSmECC);
      return -ErrSmECC;
    }

    /* jump to next block */
    len    -= run;
    sec_no += run;
    offset  = 0;
    data   += 512*run;
  }

#endif
  return -ErrNone;
}

/* create 16 byte extension for write sector */
void
smartmedia_create_extension(Lib_globals *gl, struct extension *ext, 
			    int log_block, char *data) {
  int block;

  /* set all bytes in ext area to 0xff */
  MemSet(ext, sizeof(struct extension), 0xff);

  /* create block number and add parity */
  if(log_block>=0) {
    block = 0x1000 + (log_block<<1);
    if( (par_table[block&0xff]^par_table[(block>>8)&0xff])&1 ) block|=1;
  } else
    block = 0;

  /* write block numbers */
  ext->block_address_1 = ext->block_address_2 = block;

  /* only initialize ecc for valid data */
  if(data != NULL) {
    /* create ecc's */
    smartmedia_ecc(gl->smartmedia.par_tab, gl->smartmedia.ecc_tab,
		   ext->ecc1, data);
    smartmedia_ecc(gl->smartmedia.par_tab, gl->smartmedia.ecc_tab,
		   ext->ecc2, &data[256]);
  }
}

static Boolean
smartmedia_erase_block(Lib_globals *gl, long psec) {
  int i;
#ifdef ERASE_VERIFY
  int sector, err;
  volatile unsigned char *in=DATA_IN_PORT;
#endif
#ifdef DISABLE_IRQS
  unsigned long  irq_save;
#endif

  /* calculate sector block starts with */
  psec  = (psec/gl->smartmedia.spb)*gl->smartmedia.spb;

#ifdef DEBUG_ERASE
  DEBUG_MSG("erase psec %ld\n", psec);
#endif

  /* now verify the data */  
  CTRL_REG_W = VDD | OEN | WPOW | CLE;  /* activate   CE/CLE */
  DATA_REG_W = 0x60;                    /* auto erase command */
  CTRL_REG_W = VDD | OEN | WPOW;        /* deactivate CLE */

  CTRL_REG_W = VDD | OEN | WPOW | ALE;  /* activate   ALE */
  DATA_REG_W =  psec        & 0xff;
  DATA_REG_W = (psec >> 8)  & 0xff;
  DATA_REG_W = (psec >> 16) & 0xff;
  CTRL_REG_W = VDD | OEN | WPOW;        /* deactivate ALE */

  /* start erase command */
  CTRL_REG_W = VDD | OEN | WPOW | CLE;  /* activate   CE/CLE */
  DATA_REG_W = 0xd0;

  wait_while_busy();

  /* read status */
  CTRL_REG_W = VDD | OEN | WPOW | CLE;        /* activate   CE/CLE */
  DATA_REG_W = 0x70;                          /* get status command */
  CTRL_REG_W = VDD | OEN | WPOW | nCE;        /* deactivate CLE */
  
  CTRL_REG_W = VDD | OEN | WPOW;              /* activate CE */
  i = DATA_REG_R;                             /* read status byte */
  CTRL_REG_W = VDD | OEN | WPOW | nCE;        /* deactivate CE */

  /* test 'passed' bit (bit 0) */
  if(i&0x01) {
    DEBUG_MSG("erase failed\n");
    return false;
  }

#ifdef ERASE_VERIFY
#ifdef DISABLE_IRQS
  irq_save = IMR;
  IMR = 0x00ffffff;
#endif

  /* verify erase */

  /* check all sectors of block */
  for(err=0,sector=0; (!err) && (sector<gl->smartmedia.spb);sector++) {
      
    /* now verify the data */  
    smartmedia_cmd_sector(SM_READ,  psec+sector);

    wait_while_busy();

    /* verify whole sector incl red. data area */
    for(i=0;i<528;i++) if(*in != 0xff) err=1;

    SMARTMEDIA_OFF;
  }
  
#ifdef DISABLE_IRQS
  IMR = irq_save;
#endif
  
  if(err) {
    DEBUG_MSG("erase verify error\n");
    return false;
  }
#endif
  
  return true;
}

/* erase logical block containing sector sec */
Boolean
smartmedia_free_lblock(Lib_globals *gl, long sec) {
  long psec;
  int  lblock_idx = LBLK2INDEX(sec/gl->smartmedia.spb), pblock;

#ifdef DEBUG_FREE
  DEBUG_MSG("free lblock %ld/%d\n", sec, lblock_idx);
#endif

  if((gl->smartmedia.used_table==NULL)||
     (gl->smartmedia.free_table==NULL)) {
    error(gl, true, ErrNoTable);
    return false;
  }

  /* look logical block up in translation table -> not there means free -> fine */
  if((pblock = gl->smartmedia.used_table[lblock_idx]) == -1) return true;

  /* physical block -> first physical sector */
  psec = (long)pblock * (long)gl->smartmedia.spb;  

  /* remove block from used_table */
  smartmedia_change_used_table(gl, lblock_idx, -1);

  if(!smartmedia_erase_block(gl, psec)) {
    /* erasure failed, don't attach block to free list */
    return false;
  }

  /* add free block to table */
#ifdef DEBUG_FREE
  DEBUG_MSG("index 2 %d/%d marked free\n", 1024 * PBLK2ZONE(pblock) + 
	  gl->smartmedia.free[PBLK2ZONE(pblock)], pblock);
#endif

  //WFT  gl->smartmedia.free_table[1024 * PBLK2ZONE(pblock) + 
  //	  gl->smartmedia.free[PBLK2ZONE(pblock)]++ ] = pblock;
  smartmedia_change_free_table(gl, 1024 * PBLK2ZONE(pblock) + 
			  gl->smartmedia.free[PBLK2ZONE(pblock)]++, pblock);

  gl->smartmedia.used[PBLK2ZONE(pblock)]--;

  return true;
}

static Boolean
smartmedia_copy_sec(Lib_globals *gl, long src_sec, long dst_sec) {
  int i, err;
  unsigned char *d;
  volatile unsigned char *in=DATA_IN_PORT, *out=DATA_OUT_PORT;
#ifdef DISABLE_IRQS
  unsigned long  irq_save;
#endif

#ifdef DISABLE_IRQS
  irq_save = IMR;
  IMR = 0x00ffffff;
#endif

  /* read the data */  
  smartmedia_cmd_sector(SM_READ, src_sec);

  wait_while_busy();

  /* read data */
  for(d=gl->smartmedia.tmp,i=0;i<528;i++)  *d++ = *in;

  SMARTMEDIA_OFF;
  SMARTMEDIA_OFF;

  /* write the data */  
  smartmedia_cmd_sector(SM_WRITE, dst_sec);

  /* write data */
  for(d=gl->smartmedia.tmp,i=0;i<528;i++)  *out = *d++;
  
  /* start write command */
  CTRL_REG_W = VDD | OEN | WPOW | CLE;  /* activate   CE/CLE */
  DATA_REG_W = 0x10;

  wait_while_busy();

  SMARTMEDIA_OFF;

  /* read status */
  i = smartmedia_status();

#ifdef DISABLE_IRQS
  IMR = irq_save;
#endif

  /* test 'passed' bit (bit 0) */
  if(i&0x01) {
    DEBUG_MSG("copy error\n");
    return false;
  }

#ifdef WRITE_VERIFY
#ifdef DISABLE_IRQS
  irq_save = IMR;
  IMR = 0x00ffffff;
#endif

  /* now verify the data */  
  smartmedia_cmd_sector(SM_READ, dst_sec);

  wait_while_busy();

  /* verify whole sector incl red. data area */
  for(err=0,d=gl->smartmedia.tmp,i=0;i<528;i++) if(*d++ != *in) err=1;

  SMARTMEDIA_OFF;

#ifdef DISABLE_IRQS
  IMR = irq_save;
#endif

  if(err) {
    DEBUG_MSG("copy verify error\n");
    return false;
  }
#endif

  return true;
}

static void
smartmedia_block_mark_bad(Lib_globals *gl, int pblock) {
  volatile unsigned char *out=DATA_OUT_PORT;
  struct extension ext;
  long psec;
  int i;
  unsigned char *d;
#ifdef DISABLE_IRQS
  unsigned long  irq_save;
#endif

  MemSet(&ext, sizeof(struct extension), 0xff);
  ext.block_status = 0;   /* bad block marking */

#ifdef DISABLE_IRQS
  irq_save = IMR;
  IMR = 0x00ffffff;
#endif

  for(psec=(long)pblock * gl->smartmedia.spb ; 
      psec< ((long)pblock+1l) * gl->smartmedia.spb; psec++) {

    smartmedia_cmd(SM_READ);  /* reset pointer to data */

    /* initiate write */
    smartmedia_cmd_sector(SM_WRITE, psec);

    /* write data */
    for(i=0;i<512;i++)  *out = 0xff;
  
    /* write extension */
    for(d=(char*)&ext, i=0;i<16;i++)  *out = *d++;  

    /* start write command */
    CTRL_REG_W = VDD | OEN | WPOW | CLE;  /* activate   CE/CLE */
    DATA_REG_W = 0x10;

    wait_while_busy();

    SMARTMEDIA_OFF;
  }

#ifdef DISABLE_IRQS
  IMR = irq_save;
#endif
}

/* write physical sector (data == NULL -> only write extension) */
static Boolean
smartmedia_write_sec(Lib_globals *gl, long lsec, long psec, 
		     unsigned char *data) {
  int i;
  unsigned char *d;
  volatile unsigned char *out=DATA_OUT_PORT;
  struct extension ext;
#ifdef DISABLE_IRQS
  unsigned long  irq_save;
#endif

#ifdef WRITE_VERIFY
  int err;
  volatile unsigned char *in=DATA_IN_PORT;
#endif

#ifdef DEBUG_WRITE
  DEBUG_MSG("Write sec L:%ld/P:%ld\n", lsec, psec);
#endif
  
#ifdef DISABLE_IRQS
  irq_save = IMR;
  IMR = 0x00ffffff;
#endif

  /* create 16 byte extension */
  smartmedia_create_extension(gl, &ext, lsec/gl->smartmedia.spb, data);

#ifdef SKIP_WORKAROUND
  smartmedia_cmd(SM_READ);  /* reset pointer to data */
#else
  /* write extension only? */
  if(data == NULL) {
    smartmedia_cmd(SM_EXT);               /* reset pointer to extension */
    CTRL_REG_W = VDD | OEN | WPOW | CLE;  /* activate   CE/CLE */
    DATA_REG_W = SM_EXT;
  } else {
    smartmedia_cmd(SM_READ);              /* reset pointer to data */
    CTRL_REG_W = VDD | OEN | WPOW | CLE;  /* activate   CE/CLE */
    DATA_REG_W = SM_READ;
  }
#endif

  /* initiate write */
  smartmedia_cmd_sector(SM_WRITE, psec);

  /* write data */
  if(data != NULL) for(d=data,i=0;i<512;i++)  *out = *d++;
#ifdef SKIP_WORKAROUND
  else             for(d=data,i=0;i<512;i++)  *out = 0xff;
#endif
  
  /* write extension */
  for(d=(char*)&ext, i=0;i<16;i++)  *out = *d++;  

  /* start write command */
  CTRL_REG_W = VDD | OEN | WPOW | CLE;  /* activate   CE/CLE */
  DATA_REG_W = 0x10;

  wait_while_busy();

  SMARTMEDIA_OFF;

  /* read status */
  i = smartmedia_status();

#ifdef DISABLE_IRQS
  IMR = irq_save;
#endif

  /* test 'passed' bit (bit 0) */
  if(i&0x01) {
    DEBUG_MSG("write error\n");
    return false;
  }

#ifdef WRITE_VERIFY
#ifdef DISABLE_IRQS
  irq_save = IMR;
  IMR = 0x00ffffff;
#endif

  /* now verify the data */  
  if(data == NULL) {
    smartmedia_cmd_sector(SM_EXT, psec);

    wait_while_busy();

    /* verify extension */
    for(err=0,d=(char*)&ext,i=0;i<16;i++) if(*d++ != *in) err=1;
  } else {
    smartmedia_cmd_sector(SM_READ, psec);

    wait_while_busy();

    /* verify sector data */
    for(err=0,d=data,i=0;i<512;i++) if(*d++ != *in) err=1;

    /* verify extension */
    for(d=(char*)&ext,i=0;i<16;i++) if(*d++ != *in) err=1;
  }
 
  SMARTMEDIA_OFF;

#ifdef DISABLE_IRQS
  IMR = irq_save;
#endif

  if(err) {
    DEBUG_MSG("write verify error\n");
    return false;
  }
#endif

  return true;
}

void smartmedia_get_info(Lib_globals *gl, axxPacCardInfo *info) {
  int i,r;
  static const char hexdigit[]="0123456789ABCDEF";

  /* return card infos */
  info->inserted = (gl->smartmedia.type != CARD_NONE);

  /* try to determine vendor name */
  info->vendor_id = gl->smartmedia.vendor;
  if(gl->smartmedia.vendor == VENDOR_TOSHIBA)
    StrCopy(info->vendor_name, "Toshiba");
  else if(gl->smartmedia.vendor == VENDOR_SAMSUNG)
    StrCopy(info->vendor_name, "Samsung");
  else
    StrPrintF(info->vendor_name, "unknown $%c%c", 
	      hexdigit[(gl->smartmedia.vendor)>>4],
	      hexdigit[(gl->smartmedia.vendor)&0x0f]);

  /* try to determine device name */
  info->device_id = gl->smartmedia.device;
  if(gl->smartmedia.type >= 0) {
    StrCopy(info->device_name, devname[card_type[gl->smartmedia.type].dname]);

    /* misc card infos */
    info->size  = card_layout[card_type[gl->smartmedia.type].layout].size;  /* size in MBytes */
    info->spb   = card_layout[card_type[gl->smartmedia.type].layout].spb;   /* sectors (a 512 bytes) per block */
    info->zones = card_layout[card_type[gl->smartmedia.type].layout].zones; /* number of zones */
    
    /* zone allocation info */
    r = gl->smartmedia.pblocks;
    for(i=0;i<info->zones;i++) {
      info->zone_info[i].used = gl->smartmedia.used[i]; /* number of used blocks in zone */
      info->zone_info[i].free = gl->smartmedia.free[i]; /* number of free blocks in zone */
      info->zone_info[i].unusable = ((r>1024)?1024:r)-(gl->smartmedia.free[i]+gl->smartmedia.used[i]); 
    }
  } else {
    StrPrintF(info->device_name, "unknown $%c%c", 
	      hexdigit[(gl->smartmedia.device)>>4],
	      hexdigit[(gl->smartmedia.device)&0x0f]);    

    /* not a valid card */
    info->size  = 0;
    info->zones = 0;
    info->spb   = 0;
  }
}

static Boolean 
smartmedia_check_free(int lblock, long psec) {
  volatile unsigned char *in=DATA_IN_PORT;
  int err=0,i;
  unsigned char d;
#ifdef DISABLE_IRQS
  unsigned long  irq_save;
#endif

#ifdef DISABLE_IRQS
  irq_save = IMR;
  IMR = 0x00ffffff;
#endif

  smartmedia_cmd_sector(SM_READ, psec);

  wait_while_busy();

  /* now read whole sector */
  for(i=0;i<518;i++)  if(0xff != *in) err=1;

  /* check extension */
  d = *in; d = *in;  /* skip id 0 */
  for(i=0;i<3;i++)  if(0xff != *in) err=1;
  d = *in; d = *in;  /* skip id 1 */
  for(i=0;i<3;i++)  if(0xff != *in) err=1;

  SMARTMEDIA_OFF;

#ifdef DISABLE_IRQS
  IMR = irq_save;
#endif

  return !err;
}

static Boolean
smartmedia_write_block(Lib_globals *gl, void *data, int sector, int len, int lblock, Boolean mark_bad) {
  int pblock, sec,i, xblock;
  Boolean new = false;
  int zone = LBLK2ZONE(lblock);
  int lblock_idx = LBLK2INDEX(lblock);

  /* look logical block up in translation table */
  if(gl->smartmedia.used_table[lblock_idx] == -1) {
    /* there is currently no physical block reserved for this logical block */
    
    /* get one physical block from the pool of free blocks */
    if(gl->smartmedia.free[zone] == 0) return false;  /* no free blocks available ... */

    /* allocate new block */
    pblock = gl->smartmedia.free_table[1024*zone + gl->smartmedia.free[zone] -1];

    if(pblock == -1) {
      DEBUG_MSG("Internal error 0: free table corrupted\n");
    }
      
#ifdef DEBUG_WRITE
    DEBUG_MSG("got free lblock %d, pblock %d, sec %d, len %d\n", 
	   lblock, pblock, sector, len);
#endif

    /* mark all extensions in the new block with the new logical block number */
    for(sec=0;sec<gl->smartmedia.spb;sec++) {
#ifdef DEBUG_WRITE
      DEBUG_MSG("init sec Z:%d/L:%ld/P:%ld\n", zone, 
	     ((long)LBLK2ZBLK(lblock)*gl->smartmedia.spb)+sec, 
	     ((long)pblock*gl->smartmedia.spb)+sec);
#endif

      if(!smartmedia_write_sec(gl, ((long)LBLK2ZBLK(lblock)*gl->smartmedia.spb)+sec, 
			       ((long)pblock * (long)gl->smartmedia.spb)+sec, NULL)) {

	/* write error within block? erase and return */ 
	/* (retry is handled by calling function) */
	smartmedia_erase_block(gl, (long)pblock * (long)gl->smartmedia.spb);

	/* mark defective block */
	if(mark_bad) {
	  smartmedia_block_mark_bad(gl, pblock);
	  gl->smartmedia.free[zone]--;
	}
	
#ifdef DEBUG_WRITE
	DEBUG_MSG("failed to write sec z%d, b%ld/ l%ld\n", zone, 
		  ((long)LBLK2ZBLK(lblock)*gl->smartmedia.spb)+sec, 
		  ((long)pblock*gl->smartmedia.spb)+sec);
#endif
	
	return false;
      }
    }

#ifdef DEBUG_WRITE
    DEBUG_MSG("creating new sector in block %d\n", pblock);
#endif

    new = true;  /* a new sector was allocated */
  } else {
    pblock = gl->smartmedia.used_table[lblock_idx];

#ifdef DEBUG_WRITE
    DEBUG_MSG("block %d (idx: %d) is in %d (%d/%d)\n", lblock, lblock_idx, pblock, sector, len);
#endif

    /* see if the sectors are all writeable (empty) */
    for(i=true, sec=sector; sec<sector+len; sec++)
      if(!smartmedia_check_free(lblock, ((long)pblock*gl->smartmedia.spb)+sec))
	i = false;

    /* block can not be written completely (already contains data) */
    if(!i) {

      /* free blocks available? */
      if(gl->smartmedia.free[zone] == 0) return false;
      
      /* allocate new block */
      xblock = gl->smartmedia.free_table[1024*zone + gl->smartmedia.free[zone]-1];

#ifdef DEBUG_WRITE
      if(xblock == -1) {
	DEBUG_MSG("Internal error 1: free table corrupted\n");
      }

      DEBUG_MSG("unwritable, getting new block = %d\n", xblock);
#endif

      /* copy all sectors from lblock to xblock */
      for(sec=0; sec<gl->smartmedia.spb; sec++) {

	/* only copy sectors not to be filled later */
	if((sec<sector)||(sec>=(sector+len))) {

#ifdef DEBUG_WRITE
	  DEBUG_MSG("copy %ld to new %ld\n", 
		 ((long)pblock*gl->smartmedia.spb)+sec,
		 ((long)xblock*gl->smartmedia.spb)+sec);
#endif

	  /* try to copy sectors */
	  if(!smartmedia_copy_sec(gl, ((long)pblock*gl->smartmedia.spb)+sec, 
				      ((long)xblock*gl->smartmedia.spb)+sec)) {

#ifdef DEBUG_WRITE
	    DEBUG_MSG("failed copy %ld to new %ld\n", 
		   ((long)pblock*gl->smartmedia.spb)+sec,
		   ((long)xblock*gl->smartmedia.spb)+sec);
#endif

	    /* write error within block? erase and return */
	    /* (retry is handled by calling function) */
	    smartmedia_erase_block(gl, xblock*gl->smartmedia.spb);
	  
	    /* mark defective block */
	    if(mark_bad) {
#ifdef DEBUG_WRITE
	      DEBUG_MSG("mark bad %d\n", xblock);
#endif
	      smartmedia_block_mark_bad(gl, xblock);
	      gl->smartmedia.free[zone]--;
	    }

	    return false;
	  }
	}
      }

      /* erase old block */
      if(smartmedia_erase_block(gl, (long)pblock * (long)gl->smartmedia.spb)) {
#ifdef DEBUG_FREE
	DEBUG_MSG("index 3 %d/%d marked free\n", 1024*zone + gl->smartmedia.free[zone]-1, pblock);
#endif

	/* erase worked -> add to table of free blocks */
	// WFT gl->smartmedia.free_table[1024*zone + gl->smartmedia.free[zone]-1] = pblock;
	smartmedia_change_free_table(gl, 1024*zone + gl->smartmedia.free[zone]-1, pblock);
      } else {
	/* retry once */
	if(smartmedia_erase_block(gl, (long)pblock * (long)gl->smartmedia.spb)) {
	  /* erase worked -> add to table of free blocks */
	  // WFT gl->smartmedia.free_table[1024*zone + gl->smartmedia.free[zone]-1] = pblock;
	  smartmedia_change_free_table(gl, 1024*zone + gl->smartmedia.free[zone]-1, pblock);
	} else {
	  /* erasure failed, mark as bad and shorten free list */
	  smartmedia_block_mark_bad(gl, pblock);
	  gl->smartmedia.free[zone]--;
	}
      }

#ifdef DEBUG_WRITE
      DEBUG_MSG("replaced block %d with %d to get write access\n", pblock, xblock);
#endif

      /* enter new block to table of used blocks and use it */      
      smartmedia_change_used_table(gl, lblock_idx, xblock);
      pblock = xblock;
    }

#ifdef DEBUG_WRITE
    if(i) { DEBUG_MSG("all sectors can be written\n"); }
    else  { DEBUG_MSG("not all sectors can be written\n"); }
#endif
  }

#ifdef DEBUG_WRITE
  DEBUG_MSG("writing data to block %d\n", pblock);
#endif
  
  /* the block definitely exists and is writeable */
  /* (was already there or just created) */
  for(sec=sector; sec<sector+len; sec++) {
    if(!smartmedia_write_sec(gl, ((long)LBLK2ZBLK(lblock)*gl->smartmedia.spb)+sec,
			         ((long)pblock*gl->smartmedia.spb)+sec, data)) {
      /* write error? just return it, calling function will cause */
      /* retry and hence replacement */

#ifdef DEBUG_WRITE
      DEBUG_MSG("write error sec %ld\n", ((long)pblock*gl->smartmedia.spb)+sec);
#endif

      return false;
    }
    data += 512;
  }

  /* update tables if needed */
  if(new) {
    gl->smartmedia.free[zone]--;                      /* one free block less */
    gl->smartmedia.used[zone]++;                      /* one used block more */

    /* add sector to sector table */
    smartmedia_change_used_table(gl, lblock_idx, pblock);
  }

  return true;
}

Err 
smartmedia_write(Lib_globals *gl, void *data, long start, int len) {
  int  lblock, i, consec, ret, lblock_idx;
  long sector;

#ifdef DEBUG_WRITE
  DEBUG_MSG("smartmedia_write(%ld, %d)\n", start, len);
#endif
    
  if((gl->smartmedia.used_table==NULL)||
     (gl->smartmedia.free_table==NULL)) {
    error(gl, true, ErrNoTable);
    return -ErrNoTable;
  }

  /* is even the last sector number valid? */
  if(((start+len-1)/gl->smartmedia.spb) >= gl->smartmedia.lblocks) {
    error(gl, true, ErrSmNoSec);
    return -ErrSmNoSec;
  }

  /* scan for sectors in the same block */
  for(sector=0; sector<len; sector++) {
    /* first sector in current block */
    //    first = ((start+sector)/gl->smartmedia.spb) * gl->smartmedia.spb;                 
    lblock = (start+sector)/gl->smartmedia.spb; /* current logical block */
    lblock_idx = LBLK2INDEX(lblock);            /* table index of log. block */

    consec = 0;
    while( (((start+sector+consec+1)/gl->smartmedia.spb) == lblock)&&
	   (sector+consec+1<len)) consec++;

    /* retry while blocks available */
    do {
      /* write block but don't mark bad on error */
      ret = smartmedia_write_block(gl, data, 
				   (start+sector)%gl->smartmedia.spb, 
				   consec+1, lblock, false);

      /* retry once with same block, now mark bad on error */
      if(!ret) {
	ret = smartmedia_write_block(gl, data, 
				     (start+sector)%gl->smartmedia.spb, 
				     consec+1, lblock, true);
      }

#ifdef PALM3X
      if(!ret) {
	/* check if card has been removed */
	CTRL_REG_W = VDD | INSPOW | nCE;      /* driver insert pullup                  */
	SysTaskDelay(1);                      /* wait 10ms for proper power up         */

	/* see whether card has been removed */
	if(!(CTRL_REG_R & CTRL_INS)) {
	  error(gl, true, ErrSmNoCard);
	  return -ErrSmNoCard;  /* too much defective sectors */
	}
      }
#endif

      /* retry while there are still free blocks in this zone */
    } while((!ret)&&(gl->smartmedia.free[LBLK2ZONE(lblock)] != 0));

    /* only possible error is that there are no more free sectors, */
    /* all other errors can be repaired */
    if(!ret) {
      error(gl, true, ErrSmTooM);
      return -ErrSmTooM;  /* too much defective sectors */
    }

    /* skip all consecutive sectors */
    sector+=consec;
    
    data += 512 * (consec + 1);
  }

  return ErrNone;
}

void smartmedia_close(Lib_globals *gl) {

  /* card isn't open? */
  if(gl->smartmedia.use_cnt == 0) {
    DEBUG_MSG("close: card is not open!\n");
    return;
  }

  /* card isn't in use anymore */
  if(--gl->smartmedia.use_cnt==0) {
    /* toggle auto off timer */
    /* to provent auto power off after long operations */
    EvtResetAutoOffTimer();

#ifdef DELAYED_CLOSE
    /* set timout to ten ticks (1sec) */
    gl->smartmedia.close_timeout = TimGetTicks() + DELAYED_TIMEOUT;
#else

#ifdef PALM3X
    CTRL_REG_W = nCE;   /* everything off */
#else
    CTRL_REG_W = 0x3f;  /* everything off */
#endif

    if(gl->callback != NULL) {
      if(gl->filesys.fs_ok) gl->callback(axxPacCardOk);
      else                  gl->callback(axxPacCardUnusable);
    }
#endif
  }
}

#ifdef DELAYED_CLOSE
void smartmedia_force_close(Lib_globals *gl) {
  DEBUG_MSG("card force close\n");

  gl->smartmedia.close_timeout = 0;

  if(gl->callback != NULL) {
    if(gl->filesys.fs_ok) gl->callback(axxPacCardOk);
    else                  gl->callback(axxPacCardUnusable);
  }

#ifdef PALM3X
  CTRL_REG_W = nCE;   /* everything off */
#else
  CTRL_REG_W = 0x3f;  /* everything off */
#endif
}
#endif

/* if we're not using a new include file, define things appropriately */
#ifndef sysFtrNumProcessorID
#define sysFtrNumProcessorID    2
/* Product id */
/* 0xMMMMRRRR, where MMMM is the processor model */
/* and RRRR is the revision. */
#define sysFtrNumProcessorMask  0xFFFF0000  // Mask to obtain processor model
#define sysFtrNumProcessor328   0x00010000  // Motorola 68328 (Dragonball)
#define sysFtrNumProcessorEZ    0x00020000  // Motorola 68EZ328 (Dragonball EZ)
#endif

/* initialize hardware (once at startup) */
Err smartmedia_initialize(Lib_globals *gl) {
  DWord id;

#ifndef PALM3X
  /* this version only runs on non-EZ machines */
  if (!FtrGet(sysFtrCreator, sysFtrNumProcessorID, &id)) {

    /* can get that feature -> determine cpu type */
    if((id & sysFtrNumProcessorMask) != sysFtrNumProcessor328) {
      error(gl, true, ErrWrongCPU);
      return -ErrWrongCPU;
    }
  }

  CSA2  = 0xe0000001l;    /* csa2 (rom1) decode 64k, 8Bit, 1 wait state */
  CSA3 &= ~0xc000l;       /* reduce area of second RAM to at most 4MB */
                          /* (for 8MB/12MB machines) */
#else
  unsigned char hw_id;
  char Err1[4], Err2[4];

  /* this version only runs on EZ machines */
  if(!FtrGet(sysFtrCreator, sysFtrNumProcessorID, &id)) {
    /* no feature found -> old machine -> no EZ cpu */
    if((id & sysFtrNumProcessorMask) == sysFtrNumProcessor328) {
      error(gl, true, ErrWrongCPU);
      return -ErrWrongCPU;
    }
  } else {
    error(gl, true, ErrWrongCPU);
    return -ErrWrongCPU;
  }

  CSGBB  = 0xc000;         /* map group csb to 0x18000000      */ 
#ifdef WAIT0
#ifdef FLASH_MODE
  CSB    = 0x0101;         /* csb decode 128k, 8bit, 0wait, rw, flash */
#else
  CSB    = 0x0001;         /* csb decode 128k, 8bit, 0wait, rw */
#endif
#else
#ifdef FLASH_MODE
  CSB    = 0x0111;         /* csb decode 128k, 8bit, 1wait, rw, flash */
#else
  CSB    = 0x0011;         /* csb decode 128k, 8bit, 1wait, rw */
#endif
#endif
  PBSEL &= ~1;             /* enable CSB0 for chip select      */
  PGSEL &= ~2;             /* enable A0                        */

  /* read magic word and rev id from hardware */
  hw_id = CTRL_REG_R;
  if((hw_id & 0xf0) != 0xa0) {
    error(gl, true, ErrNoHw);
    return -ErrNoHw;
  }

  if(((hw_id >> 2) & 3)!= HW_VERSION) {
    error(gl, true, ErrHwVersion);
    return -ErrHwVersion;
  }
#endif

  return -ErrNone;
}

Err smartmedia_release(Lib_globals *gl) {
  smartmedia_close(gl);  /* if forgotten */

  smartmedia_free_tables(gl);

  return -ErrNone;
}

/* frequently check card state */
axxPacCardState smartmedia_card_status(Lib_globals *gl) {
  int type;
  axxPacCardState state;

#ifdef PALM3X
  /* final hardware checks insert switch */
  CTRL_REG_W = VDD | INSPOW | nCE;      /* driver insert pullup                  */

  SysTaskDelay(1);                /* wait 10ms for proper power up         */
  if(!(CTRL_REG_R & CTRL_INS)) {  /* no card? (switch open, pullup active) */
    CTRL_REG_W = nCE;             /* disable insert pullup                 */
    type = CARD_NONE;             /* removed means something changed       */
  } else
#endif
  {
    CTRL_REG_W = VDD | OEN | 0x1e;  /* drive write protect pull down and LED */
    CTRL_REG_W = VDD | OEN | nCE;   /* power chip, CE inactive               */
    CTRL_REG_W = VDD | OEN | WPOW | nCE;  /* write protect pull up */
                                          /* CE inactive    */

    SysTaskDelay(1);                /* wait 10ms for proper power up         */

    type = smartmedia_init(gl);     /* see if type changes -> */
                                    /* media is removed/changed */
  }

  if(type != gl->smartmedia.type) {
    gl->smartmedia.type = type;
    gl->smartmedia.change = true;

    if(type < 0) {
      gl->filesys.fs_ok = false;
      if(type == CARD_NONE) state = axxPacCardNone;  /* insert switch is off */
      else                  state = axxPacCardUnusable;  /* unknown card */
    } else {
      DEBUG_MSG("New card (%s), initialize ...\n", 
		devname[card_type[type].dname]);
      if(!smartmedia_open(gl, false)) {
	DEBUG_MSG("card initialization ok!\n");
	if(!gl->filesys.fs_ok)               state = axxPacCardUnusable;
	else if(gl->smartmedia.use_cnt != 0) state = axxPacCardBusy;
	else                                 state = axxPacCardOk;
	smartmedia_close(gl);
      } else {
	DEBUG_MSG("card initialization failed!\n");
	state = axxPacCardUnusable;
      }
    }
  } else {
    /* card insert/remove state didn't change */
    if(type == CARD_NONE)         
      state = axxPacCardNone;      /* insert switch is off */
    else if((type == CARD_UNKNOWN) || (!gl->filesys.fs_ok)) 
      state = axxPacCardUnusable;  /* unknown card */
#ifndef DELAYED_CLOSE
    else if(gl->smartmedia.use_cnt != 0)
#else
    else if((gl->smartmedia.close_timeout > 0) || 
	    (gl->smartmedia.use_cnt != 0))
#endif
      state = axxPacCardBusy;
    else
      state = axxPacCardOk;
  }

#ifndef DELAYED_CLOSE
  if(gl->smartmedia.use_cnt == 0)
#else
  if((gl->smartmedia.use_cnt == 0)&&(gl->smartmedia.close_timeout == 0))
#endif
  {
#ifdef PALM3X
    CTRL_REG_W = nCE;   /* everything off */
#else
    CTRL_REG_W = 0x3f;  /* everything off */
#endif
  }
  return state;
}

Boolean smartmedia_open(Lib_globals *gl, Boolean complain) {
  int type, err;

  if(gl->smartmedia.use_cnt == 0) {

#ifdef DELAYED_CLOSE
    if(gl->smartmedia.close_timeout == 0) {
#endif

      DEBUG_MSG("card open\n");

#ifdef PALM3X
      /* final hardware checks insert switch */
      CTRL_REG_W = VDD | INSPOW | nCE;      /* driver insert pullup */
      SysTaskDelay(1);                /* wait 10ms for proper power up */

      if(!(CTRL_REG_R & CTRL_INS)) {  /* no card? (switch open, pullup active) */
	smartmedia_close(gl);         /* disable insert pullup */
	if(complain) error(gl, true, ErrSmNoDev);
	DEBUG_MSG("no device found\n");
	return false;
      }
#endif

      CTRL_REG_W = VDD | OEN | 0x1e;        /* drive write protect pull down and LED */
      CTRL_REG_W = VDD | OEN | nCE;         /* power chip, CE inactive */
      CTRL_REG_W = VDD | OEN | WPOW | nCE;  /* write protect pull up power chip, CE inactive */
    
      SysTaskDelay(1);          /* wait 10ms for proper power up */

      type = smartmedia_init(gl);
    
      if(type==-1) {
	smartmedia_close(gl);
	if(complain) error(gl, true, ErrSmUnkDev);
	DEBUG_MSG("unknown device\n");
	return -ErrSmUnkDev;   /* unknown card type */
      }
      
      if((gl->smartmedia.change)&&(type != CARD_NONE)) {
	gl->smartmedia.unusable = false;  /* assume media is fine */
	gl->smartmedia.change = false;
	gl->smartmedia.type = type;
      
	smartmedia_free_tables(gl);
	
	/* MByte/sec size -> 1048576/512 = 2048 sec/MB */
	/* -> physical blocks = 2048 / sectors_per_block * size_in_megabytes */ 
	/* -> logical blocks = 2000 ... */
	if(smartmedia_scan_blocks( gl,
				   card_layout[card_type[type].layout].psec/
				   card_layout[card_type[type].layout].spb, 
				   card_layout[card_type[type].layout].lsec/
				   card_layout[card_type[type].layout].spb,
				   card_layout[card_type[type].layout].spb,
				   card_layout[card_type[type].layout].zones) != 0) {
	  smartmedia_close(gl);
	  error(gl, true, ErrSmOOM);
	  DEBUG_MSG("out of memory\n");
	  return -ErrSmOOM;  /* error retrieving block table */
	}
	
	/* card is fine, init file system */
	if(fs_init(gl)) {
	  /* this card can't be used for normal operations */
	  gl->smartmedia.unusable = true;   
	  error(gl, true, ErrFsError);
	  DEBUG_MSG("filesystem init failed\n");
	  return -ErrFsError;
	}
      } else {
	if(gl->smartmedia.unusable) {
	  error(gl, true, ErrSmUnusable);      
	  return -ErrSmUnusable;
	}
      }
#ifdef DELAYED_CLOSE
    }
#endif    
    if(gl->callback != NULL) gl->callback(axxPacCardBusy);
    gl->smartmedia.use_cnt++;
  } else {
    DEBUG_MSG("card is already open\n");
  }
  return -ErrNone;
}

/* CIS block */
const unsigned char cis_init[]={
  0x01, 0x03, 0xd9, 0x01, 0xff, 0x18, 0x02, 0xdf, 
  0x01, 0x20, 0x04, 0x00, 0x00, 0x00, 0x00, 0x21,
  0x02, 0x04, 0x01, 0x22, 0x02, 0x01, 0x01, 0x22, 
  0x03, 0x02, 0x04, 0x07, 0x1a, 0x05, 0x01, 0x03,

  0x00, 0x02, 0x0f, 0x1b, 0x08, 0xc0, 0xc0, 0xa1, 
  0x01, 0x55, 0x08, 0x00, 0x20, 0x1b, 0x0a, 0xc1,
  0x41, 0x99, 0x01, 0x55, 0x64, 0xf0, 0xff, 0xff, 
  0x20, 0x1b, 0x0c, 0x82, 0x41, 0x18, 0xea, 0x61,

  0xf0, 0x01, 0x07, 0xf6, 0x03, 0x01, 0xee, 0x1b, 
  0x0c, 0x83, 0x41, 0x18, 0xea, 0x61, 0x70, 0x01,
  0x07, 0x76, 0x03, 0x01, 0xee, 0x15, 0x14, 0x05, 

#ifndef FORMAT_PANORAMA
  0x00,  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',  ' ',
  0x00,  ' ',  ' ',  ' ',  ' ', 0x00,  '0',  '.',
   '0', 0x00, 0xff, 0x14, 0x00, 0xff, 0x00, 0x00
#else
  0x00,  'O',  'L',  'Y',  'M',  'P',  'U',  'S',
  0x00,  'P',  'A',  'N',  ' ', 0x00,  '1',  '.',
   '0', 0x00, 0xff, 0x14, 0x00, 0xff, 0x00, 0x00
#endif
};

/* weird extra ASCII block */
const char xtra_init[19] = "AXXPAC/TH  " "FAT12   ";

/* format smartmedia, erase everything and rewrite every sector */
Err
smartmedia_format(Lib_globals *gl, void(*callback)(unsigned char)) {
  Boolean ret = false;
  int type;
  const struct card_layout* card;
  int block, i, zone;
  unsigned char buffer[512];

#ifdef PALM3X
  /* final hardware checks insert switch */
  CTRL_REG_W = VDD | INSPOW | nCE; /* driver insert pullup */

  SysTaskDelay(1);                 /* wait 10ms for proper power up */

  if(!(CTRL_REG_R & CTRL_INS)) {   /* no card? (switch open, pullup active) */
    smartmedia_close(gl);          /* disable insert pullup */
    error(gl, true, ErrSmNoDev);
    return -ErrSmNoDev;
  }
#endif

  CTRL_REG_W = VDD | OEN | 0x1e;        /* write protect pull down and LED */
  CTRL_REG_W = VDD | OEN | nCE;         /* power chip, CE inactive */
  CTRL_REG_W = VDD | OEN | WPOW | nCE;  /* write protect pull up power chip */
                                        /* CE inactive */
  SysTaskDelay(1);

  if((type = smartmedia_init(gl))==-1) {
    smartmedia_close(gl);
    error(gl, true, ErrSmUnkDev);
    return -ErrSmUnkDev;   /* unknown card type */
  }

  if(SMARTMEDIA_PROT) {
    smartmedia_close(gl);
    error(gl, false, ErrFsWrProt);
    return -ErrFsWrProt;
  }

  card = &card_layout[card_type[type].layout];

  if(gl->callback) gl->callback(axxPacCardBusy);

  /* init desired parameters */
  gl->smartmedia.spb     = card->spb;
  gl->smartmedia.pblocks = (card->psec/card->spb);  /* save table length */
  gl->smartmedia.lblocks = (card->lsec/card->spb);  /* save table length */
  gl->smartmedia.spb     = card->spb;   /* save number of sectors per block */
  gl->smartmedia.zones   = card->zones;

  for(i=0;i<card->zones;i++) {
    gl->smartmedia.free[i] = 0;  /* number of free blocks in this zone */
    gl->smartmedia.used[i] = 0;  /* number of used blocks in this zone */
  }

  smartmedia_free_tables(gl);

  /* allocate block used/free tables */
  if(!smartmedia_alloc_tables(gl, gl->smartmedia.pblocks)) {

    smartmedia_close(gl);
    error(gl, true, ErrSmMemBuf);
    return -ErrSmMemBuf;
  }

  /* erase all blocks */
  for(block=gl->smartmedia.pblocks-1;block>=0;block--) {
    zone = PBLK2ZONE(block);

    /* draw progress bar */
    if(callback != NULL)
      callback((100l*(gl->smartmedia.pblocks-block-1))/gl->smartmedia.pblocks);

    if(!smartmedia_erase_block(gl, (long)block*(long)gl->smartmedia.spb)) {

#ifdef PALM3X
      /* check if card has been removed */
      CTRL_REG_W = VDD | INSPOW | nCE;      /* driver insert pullup                  */
      SysTaskDelay(1);                      /* wait 10ms for proper power up         */

      if(!(CTRL_REG_R & CTRL_INS)) {
	smartmedia_close(gl);
	error(gl, true, ErrSmNoCard);
	error(gl, false, ErrFsFormat);
	return -ErrFsFormat;
      }
#endif

      DEBUG_MSG("block %d is defective\n", block);

      /* error erasing block */
      if(block<2) {
	/* first two blocks must be ok for format */
	smartmedia_close(gl);
	error(gl, true, ErrSmFormat);
	error(gl, false, ErrFsFormat);
	return -ErrFsFormat;
      } else {
	/* defective data block */
	smartmedia_block_mark_bad(gl, block);
      }
    } else {
      /* block is fine */

      /* don't mark the first 2 blocks as free */
      if(block>=2) {
	// WFT	gl->smartmedia.free_table[1024*zone + 
	//	 gl->smartmedia.free[zone]++] = block;      
	smartmedia_change_free_table(gl, 1024*zone + gl->smartmedia.free[zone]++, block);
      }
    }
  }

  /* not enough free blocks om each zone */
  if(card->zones == 1) {
    if(gl->smartmedia.free[0] < gl->smartmedia.lblocks) {
      smartmedia_close(gl);
      error(gl, true, ErrSmFormat);
      error(gl, false, ErrFsFormat);
      return -ErrFsFormat;
    }
  } else {
    for(i=0;i<card->zones;i++) {
      /* at least 1000 free blocks per zone */
      if(gl->smartmedia.free[i] < 1000) {
	smartmedia_close(gl);
	error(gl, true, ErrSmFormat);
	error(gl, false, ErrFsFormat);
	return -ErrFsFormat;
      }
    }
  }

  /* set up disk structures ... */

  /* mark first block bad */
  smartmedia_block_mark_bad(gl, 0);

  /* fill second block with CIS */
  MemSet(buffer, 512, 0x00);
  MemMove(buffer,     cis_init, 224);
  MemMove(buffer+256, cis_init, 224);
  smartmedia_write_sec(gl, -128, gl->smartmedia.spb, buffer);
  MemSet(buffer, 512, 0xff);
  for(i=gl->smartmedia.spb+1; i<(2*gl->smartmedia.spb);i++)
    smartmedia_write_sec(gl, -1, i, buffer);

  /* this should go into filesys.c */

  /* setup master boot record, set up one partition (first real sector) */
  MemSet(buffer, 512, 0x00);
  for(i=0;i<16;i++) buffer[0x1be + i] = card->fmt[i];
  buffer[0x1fe] = 0x55; buffer[0x1ff] = 0xaa;  /* signature */
  smartmedia_write(gl, buffer, 0, 1);

  /* setup partition boot record */
  MemSet(buffer, 512, 0x00);
  buffer[0]=0xe9;                            /* silly DOS jump */
  MemMove(&buffer[3], &cis_init[0x59], 8);   /* same name as in cis block */
  /* buffer[0x0c]=0; due to clear */         /* always 512 bytes per sector */
  buffer[0x0c] = 2;                          /* -"- high byte */
  buffer[0x0d] = card->spb;                  /* block is always = cluster */
  buffer[0x0e] = 1;                          /* reserved sector (boot sector)*/
  /* buffer[0x0f]=0; due to clear */         /* two fats */  
  buffer[0x10] = 2;                          /* -"- high byte */
  /* buffer[0x11]=0; due to clear */         /* 256 root directory entries */ 
  buffer[0x12] = 1;                          /* -"- high byte */

  /* card <= 32 MBytes */
  if(card->fmt[14]==0) {
    buffer[0x13] = card->fmt[12];            /* logical number sectors */
    buffer[0x14] = card->fmt[13];            /* -"- high byte */
  } else {
    buffer[0x20] = card->fmt[12];            /* logical number sectors */
    buffer[0x21] = card->fmt[13];
    buffer[0x22] = card->fmt[14];
    buffer[0x23] = card->fmt[15];
  }

  buffer[0x15] = 0xf8;                       /* media byte */
  buffer[0x16] = card->spf;                  /* sectors per fat */
  /* buffer[0x17]=0; due to clear */         /* -"- high byte */
  buffer[0x18] = card->sec;                  /* sectors per track */
  /* buffer[0x19]=0; due to clear */         /* -"- high byte */
  buffer[0x1a] = card->heads;                /* sectors per track */
  /* buffer[0x1b]=0; due to clear */         /* -"- high byte */
  buffer[0x1c] = card->fmt[8];               /* number of hidden sectors */
  /* buffer[0x1d]=0; due to clear */         /* -"- byte 1 */
  /* buffer[0x1e]=0; due to clear */         /* -"- byte 2 */
  /* buffer[0x1f]=0; due to clear */         /* -"- byte 3 */

  /* setup weird extra ASCII fields */
  MemMove(&buffer[0x2b], xtra_init, 19);
  if(card->fmt[4] == 0x06) buffer[0x2b+15] = '6'; /* "FAT12" -> "FAT16" */

  buffer[0x1fe] = 0x55; buffer[0x1ff] = 0xaa;  /* signature */

  /* 8: low byte of offset to partition boot sector */
  smartmedia_write(gl, buffer, card->fmt[8],1);

  /* setup empty FATs/root directory (always 16 sec) */
  for(i=0; i< 2*card->spf + 16 ; i++) {
    MemSet(buffer, 512, 0x00);


    /* first sector of one of the fats? */
    if((i==0)||(i==card->spf)) {
      buffer[0] = 0xf8;             /* media byte */
      buffer[1] = buffer[2] = 0xff;

      /* for fat 16 set one additional byte */
      if(card->fmt[4] == 0x06)
	buffer[3] = 0xff;
    }
    smartmedia_write(gl, buffer, card->fmt[8]+1+i,1);
  }

  smartmedia_close(gl);

  /* force rescan of media */
  gl->smartmedia.change = true;
  gl->smartmedia.type = CARD_NONE;  /* force reload of card info */

  if(gl->callback != NULL)          /* tell caller about this */
    gl->callback(axxPacCardNone);

  return ret;
}

