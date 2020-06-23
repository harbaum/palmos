/* Uncomment the next line if you have two flash chips, and want to write to the second one...*/
//#define SECOND_FLASH

/* Uncomment the next line if you want to write the serial number sector.  This is not possible on most devices */
//#define FLASH_SN

/* don't do any OS calls during the flash access (no debug output while */
/* accessing the rom), because this would require array access to the flash */
/* which is not available during advanced flash operations */

/* interrupt mask register */
#define IMR  (*(unsigned long*)0xfffff304)

/* chip select A3 needed to correct memory access on 8MB boards */
#define CSA3 (*(unsigned long*)0xfffff11c)

#ifndef SECOND_FLASH
#ifdef DO_WARNING
#warning Compiling for first (OS) ROM
#endif

/* chip select A0 */
#define CSA0 (*(unsigned long*)0xfffff110)

/* Flash base address */
#define FLASHBASE     (0x10c00000)
#else
#ifdef DO_WARNING
#warning Compiling for second ROM
#endif

/* chip select second flash */
#define CSA0 (*(unsigned long*)0xfffff118)

/* second Flash base address */
#define FLASHBASE     (0x10e00000)
#endif

/* internal use only */
extern void flash_open(void);
extern void flash_close(void);
extern int  flash_command_set(int);
extern int  flash_write_word(unsigned short data, long addr);

/* public functions */
extern int  flash_is_present(void);
extern void flash_get_info(char *buffer);
extern long flash_last_word_used(void);
extern int  flash_is_empty(long start, long size);
extern int  flash_sector_erase(long start);
extern int  flash_sector_checkprotect(long addr);
extern int  flash_erase(void);
extern long flash_write(unsigned short *src, long dest, long size);
extern int  flash_read(unsigned short *dest, long src, long size);
extern int  flash_verify(unsigned short *src, long dest, long size);

