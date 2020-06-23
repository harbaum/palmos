/*
    flash_io.c  -  low level flash routines for Palm III

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
*/

#include "flash_io.h"

/* don't do any OS calls during the flash access (no debug output while */
/* accessing the rom), because this would require array access to the flash */
/* which is not available during advanced flash operations */

/* interrupt mask register */
#define IMR  (*(unsigned long*)0xfffff304)

/* chip select A0 */
#define CSA0 (*(unsigned long*)0xfffff110)

/* chip select A3 */
#define CSA3 (*(unsigned long*)0xfffff11c)

/* Flash base address */
#define FLASHBASE     (0x10c00000)
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
  int i;
  int ok=0;

  flash_open();

  /* verify manufactorer id (AMD and Fujitsu) */
  FLASH(0x555)=0xaa;
  FLASH(0x2aa)=0x55;
  FLASH(0x555)=0x90;
  i=FLASH(0x00);
  if((i==0x01)||(i==0x04)) {

    /* verify device id (AM29LV160BB or BT) */
    FLASH(0x555)=0xaa;
    FLASH(0x2aa)=0x55;
    FLASH(0x555)=0x90;
    i=FLASH(0x01);
    if((i==0x2249)||(i==0x22c4))  ok=1;
  }

  /* reset flash to normal operation */
  FLASH(0x00) = 0xf0;

  flash_close();

  return(ok);
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

  /* for security reasons, this code only allows */
  /* to write to addresses 0xa0000-0xfffff, which */
  /* does not contain the OS, you should verify this */
  /* by reading the last byte != 0xffff with the */
  /* unmodified flash. This _MUST_ be < 0xa0000. */
  /* (last line of the flashy info screen) */

  /* never write to OS */
  if((addr<0xa0000)||(addr>0xfffff))
    return 0;

  /* look if word already there */
  if(FLASH(addr)==data)
    return 1;

  /* initiate write */
  FLASH(0x555)=0xaa;
  FLASH(0x2aa)=0x55;
  FLASH(0x555)=0xa0;

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

  if(!flash_is_present())
    return(dest);

  /* make sure the data is readeable by reading it */
  /* if there is a fatal exception due to illegal memory */
  /* better crash now than during the write operation */
  for(i=0;i<size;i++)
    dummy=FLASH(dest+i);  

  flash_open();

  for(i=0;i<size;i++) {
    /* write as long as everything is fine */
    if(!flash_write_word(*src++, dest+i)) {
      /* oops, error -> return address */
      flash_close();
      return(dest+i);
    }
  }

  flash_close();

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

  if(!flash_is_present())
    return 3;

  /* never write to OS */
  if((addr<0xa0000)||(addr>0xfffff))
    return 2;

  flash_open();
  
  /* initiate sector erase */
  FLASH(0x555)=0xaa;
  FLASH(0x2aa)=0x55;
  FLASH(0x555)=0x80;
  FLASH(0x555)=0xaa;
  FLASH(0x2aa)=0x55;

  /* ok, erase ... */
  FLASH(addr)=0x30;

  do {
    /* read status */
    status=FLASH(addr);

    /* test poll bit */
    if((status&0x80)==0x80) {
      flash_close();
      return 0;   /* successfully erased! */
    }

    /* loop until timeout */
  } while((status&0x20)!=0x20);
  
  /* read status */
  status=FLASH(addr);
  
  flash_close();

  if((status&0x80)==0x80)
    return 0;   /* successfully erased! */
  
  return 1;
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

