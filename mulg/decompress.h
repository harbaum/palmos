#ifndef __DECOMPRESS_H__
#define __DECOMPRESS_H__

#define DECOMPRESS_DB_NAME  "TH Decompress Temp."
#define DECOMPRESS_TYPE     'Dcmp'

extern void *decompress_rsc(unsigned long type, int id);
extern void decompress_free(void *);

#endif
