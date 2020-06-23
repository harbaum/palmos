
/* internal use only */
extern void flash_open(void);
extern void flash_close(void);
extern int  flash_write_word(unsigned short data, long addr);

/* public functions */
extern int  flash_is_present(void);
extern void flash_get_info(char *buffer);
extern long flash_last_word_used(void);
extern int  flash_is_empty(long start, long size);
extern int  flash_erase_sector(long start);
extern long flash_write(unsigned short *src, long dest, long size);
extern int  flash_read(unsigned short *dest, long src, long size);
extern int  flash_verify(unsigned short *src, long dest, long size);

