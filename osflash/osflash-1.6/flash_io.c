/*
    flash_io.c  -  low level flash routines for Palm III
    (modified version for osflash)

    This code is free. Written for use with gcc/pilrc. It is not optimized
    for speed, but for readability.

    BE VERY, VERY, VERY CAREFUL WITH THIS. YOU MIGHT ERASE THE OPERATING SYSTEM
    OF YOUR PALM CAUSING IT TO BE UNRECOVERABLE DEAD!!!!!

    DON'T USE THIS PROGRAM IF YOU ARE NOT REALLY SURE ABOUT THE POSSIBLE
    CONSEQUENCES!!!

    Till Harbaum

    t.harbaum@tu-bs.de
    http://www.ibr.cs.tu-bs.de/

    Version 27.4.98 - fixed crash on 8MB machines
            16.6.99 (<Tim Charron> tcharron@interlog.com)
                    - Added detection of Fujitsu MBM29LV160B chip (flash_is_present())
					- Added flash_sector_checkprotect() to determine the protection
					  status of a single sector
                    - Added messages indicating chip type, which chip being flashed
*/

#define DO_WARNING
#include <PalmOS.h>
#include "flash_io.h"
#include "osflashRsc.h"

#define FLASH(a)      (*(((volatile unsigned short*)FLASHBASE)+a))

unsigned long  irq_save, save_csa3;

/* enable write access to flash rom */
void flash_open(void) {
  /* disable irqs to prevent access to rom during flash operation */
  irq_save = IMR;
  IMR = 0x00ffffff;

  /* allow write access to rom (CSA0) */
  CSA0 &= ~8;

  /* reduce area of second RAM to 4MB (for 8MB machines) */
  save_csa3 = CSA3;
  CSA3 &= ~0xc000l;
}

/* disable write access to flash rom */
void flash_close(void) {
  /* restore csa3 state */
  CSA3 = save_csa3;

  /* disable write access to rom (CSA0) */
  CSA0 |=  8;    

  /* enable irqs */
  IMR = irq_save;
}

/* check if appropriate flash rom is present */
int  flash_is_present(void) {
  int ok=0;
  char str[100];
  int mfgid=0, devid=0;

  flash_open();

  /* verify manufacturer id (AMD) */
  FLASH(0x555)=0xaa;
  FLASH(0x2aa)=0x55;
  FLASH(0x555)=0x90;
  if(FLASH(0x00)==0x01) {

    /* verify device id (AMD & Fujitsu both use 2249 for Bottom Boot Block) */
    FLASH(0x555)=0xaa;
    FLASH(0x2aa)=0x55;
    FLASH(0x555)=0x90;
    if(FLASH(0x01)==0x2249)  {mfgid=ok=1; devid=0x2249; flash_command_set(1);}
  }

  /* verify manufacturer id (Fujitsu MBM29LV160B "29LV160B-90PFTN") */
  FLASH(0x555)=0xaa;
  FLASH(0x2aa)=0x55;
  FLASH(0x555)=0x90;
  if(FLASH(0x00)==0x04) {

    /* verify device id (AMD & Fujitsu both use 2249 for Bottom Boot Block) */
    FLASH(0x555)=0xaa;
    FLASH(0x2aa)=0x55;
    FLASH(0x555)=0x90;
    if(FLASH(0x01)==0x2249)  {mfgid=ok=4; devid=0x2249; flash_command_set(1);}
  }

  /* verify manufacturer id (Toshiba TC58FVB160) */
  FLASH(0x555)=0xaa;
  FLASH(0x2aa)=0x55;
  FLASH(0x555)=0x90;
  if(FLASH(0x00)==0x98) {

    /* verify device id (Toshiba Bottom boot block is 0x0043) */
    FLASH(0x555)=0xaa;
    FLASH(0x2aa)=0x55;
    FLASH(0x555)=0x90;
    if(FLASH(0x01)==0x0043)  {mfgid=ok=0x98; devid=0x0043; flash_command_set(1);}
  }

  /* verify manufacturer id (SST SST39VF160) */
  FLASH(0x5555)=0xaa;
  FLASH(0x2aaa)=0x55;
  FLASH(0x5555)=0x90;
  if(FLASH(0x00)==0xbf) {

    /* verify device id (SST SST39LF160 / SST39VF160) */
/*    FLASH(0x5555)=0xaa; */
/*    FLASH(0x2aaa)=0x55; */
/*    FLASH(0x5555)=0x90; */
/*    if(FLASH(0x01)==0x2782)  {mfgid=ok=0xbf; devid=0x2782; flash_command_set(2);} */
  }

  if (ok==0) {
    // No known flash found
    FLASH(0x555)=0xaa;
    FLASH(0x2aa)=0x55;
    FLASH(0x555)=0x90;
    mfgid=FLASH(0x00);

    FLASH(0x555)=0xaa;
    FLASH(0x2aa)=0x55;
    FLASH(0x555)=0x90;
    devid=FLASH(0x01);

    /* if mfgid is still 0 then we try to read manufacturer and device id differently */
    if (!mfgid) {
      FLASH(0x5555)=0xaa;
      FLASH(0x2aaa)=0x55;
      FLASH(0x5555)=0x90;
      mfgid=FLASH(0x00);

      FLASH(0x5555)=0xaa;
      FLASH(0x2aaa)=0x55;
      FLASH(0x5555)=0x90;
      devid=FLASH(0x01);
    }

  }

  /* reset flash to normal operation */
  FLASH(0x00) = 0xf0;

  flash_close();

  if (ok==0) {
      StrPrintF(str, "Unknown flash: mfg=%x device=%x", mfgid, devid);
      FrmCustomAlert(alt_err, str,0,0);

      // Note: AM29DL323DB is 0x2253.  4M Bottom boot block AMD chip.  Sector
      // alignment differs from other chips above.
  }
  
  return(ok);
}

/* remember what type of command set the flash chip uses */
int flash_command_set(int command) {
  static int command_set=0;

  if (command) {
    command_set=command;
  }
  return (command_set);
}

/* read internal flash registers into 256 byte buffer */
void flash_get_info(char *buffer) {
  int i;

  flash_open();

  /* write CFI query command to flash rom (the flash is in word mode) */
  FLASH(0x55) = 0x98;  // write 0x98 to address 0x55

  /* read CFI information */
  for(i=0;i<256;i++)
    buffer[i]=FLASH(i);

  /* reset flash to normal operation */
  FLASH(0x00) = 0xf0;

  flash_close();
}

/* return offset of last used word (!=0xffff) */
long flash_last_word_used(void) {
  long i=0xfffff;

  while(FLASH(i)==0xffff) i--;

  return i;
}

/* write a single word to the flash rom */
int flash_write_word(unsigned short data, long addr) {
  unsigned short status;

  /* look if word already there */
  if(FLASH(addr)==data)
    return 1;

  /* initiate write */
  switch ( flash_command_set(0) ) {
    case 1:
      FLASH(0x555)=0xaa;
      FLASH(0x2aa)=0x55;
      FLASH(0x555)=0xa0;
      break;
    case 2:
      FLASH(0x5555)=0xaa;
      FLASH(0x2aaa)=0x55;
      FLASH(0x5555)=0xa0;
      break;
    default:
      return 0;  /* something is wrong :-( */
  }

  /* ok, write ... */
  FLASH(addr)=data;

  do {
    /* read status */
    status=FLASH(addr);

    /* test poll bit */
    if((status&0x80)==(data&0x80))
      return 1;   /* successfully written!!! */

    /* loop until timeout */
  } while((status&0x20)!=0x20);
  
  /* read status */
  status=FLASH(addr);

  if((status&0x80)==(data&0x80))
    return 1;   /* successfully written!!! */
  
  return 0;  /* operation failed :-( */
}

/* write data to flash rom (this is the dangerous part) */
/* this returns 0 on success, otherwise the address that */
/* caused the error */
long  flash_write(unsigned short *src, long dest, long size) {
  long i;
  unsigned short dummy;

  for(i=0;i<size;i++) {
    /* write as long as everything is fine */
    if(!flash_write_word(*src++, dest+i)) {
      /* oops, error -> return address */
      return(dest+i);
    }
  }

  return 0;  /* everything is fine */
}


/* read data from flash */
int  flash_read(unsigned short *dest, long src, long size) {
  long i;

  for(i=0; i<size; i++)
    dest[i]=FLASH(i+src);

  return 0;
}

/* erase sector */
int flash_sector_erase(long addr) {
  unsigned short status;

  /* initiate sector erase */
  switch ( flash_command_set(0) ) {
    case 1:
      FLASH(0x555)=0xaa;
      FLASH(0x2aa)=0x55;
      FLASH(0x555)=0x80;
      FLASH(0x555)=0xaa;
      FLASH(0x2aa)=0x55;
      break;
    case 2:
      FLASH(0x5555)=0xaa;
      FLASH(0x2aaa)=0x55;
      FLASH(0x5555)=0x80;
      FLASH(0x5555)=0xaa;
      FLASH(0x2aaa)=0x55;
      break;
    default:
      return 1;  /* something is wrong :-( */
  }

  /* ok, erase ... */
  FLASH(addr)=0x30;

  do {
    /* read status */
    status=FLASH(addr);

    /* test poll bit */
    if((status&0x80)==0x80) {
      return 0;   /* successfully erased! */
    }

    /* loop until timeout */
  } while((status&0x20)!=0x20);
  
  /* read status */
  status=FLASH(addr);
  
  if((status&0x80)==0x80)
    return 0;   /* successfully erased! */
  
  return 1;
}

// Check if sector is protected or not
int flash_sector_checkprotect(long addr) {
  int prot=0;
  long addr2;

  addr2 = addr & 0xFFFFFF00;
  addr2 +=       0x00000002;

  flash_open();

  /* verify manufacturer id (amd) */
  FLASH(0x555)=0xaa;
  FLASH(0x2aa)=0x55;
  FLASH(0x555)=0x90;
  if(FLASH(addr2)==0x01) prot=1;

  /* reset flash to normal operation */
  FLASH(0x00) = 0xf0;

  flash_close();

  return(prot);
}

/* check if area contains data != 0xffff */
int flash_is_empty(long start, long size) {
  long i;

  for(i=0;i<size;i++)
    if(FLASH(start+i)!=0xffff)
      return 0;

  return 1;
}

/* check if flash contents equal buffer */
int  flash_verify(unsigned short *src, long dest, long size) {
  long i;
  
  for(i=0; i<size; i++)
    if(src[i]!=FLASH(i+dest))
      return 0;

  return 1;
}

