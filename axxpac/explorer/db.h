/*

  database header file
*/

#ifndef __DB_H__
#define __DB_H__

#define BAR_HEIGHT  40
#define BAR_WIDTH  120

#define HANDLE_OLD_FORMAT   // define this to try to cope with the old format

#include "../api/axxPac.h"
#include "explorer.h"

typedef unsigned char ubyte;
typedef signed char sbyte;
typedef unsigned short uword;
typedef short word;
typedef unsigned long udword;
typedef long dword;

/* palm pdb/prc database header */
struct db_header {
  ubyte name[32];
  uword fileAttributes;
  uword version;
  udword creationDate;
  udword modificationDate;
  udword lastBackupDate;
  udword modificationNumber;
  udword appInfoArea;
  udword sortInfoArea;
  udword type;
  udword creator;
  udword uniqueIDSeed;
  udword nextRecordListID;
  uword numberOfRecords;
} __attribute__ ((packed));

/* resource entry header in palm resource database (prc) */
struct db_rsrc_header {
  udword type;
  uword  id;
  udword offset;
} __attribute__ ((packed));

/* record entry header in palm record database (pdb) */
struct db_record_header {
  udword offset;
  ubyte  attributes;
  ubyte  unique_id[3];
} __attribute__ ((packed));

struct read_info {
  axxPacFD fd;
  ULong flen;
  ULong cnt;
  Boolean force;
  UInt lib;
  int pos;
  int *lp;

#ifdef HANDLE_OLD_FORMAT
  int   skip;
  int   state;
  struct db_header header;
  struct db_record_header rec1st;
  struct db_rsrc_header rsrc1st;
#endif
};

struct write_info {
  axxPacFD fd;
  ULong flen;
  ULong cnt;
  Boolean quiet;
  UInt lib;
  int pos;
  int *lp;
};

extern Boolean db_read(char *name, long length, int pos, Boolean force);
extern Boolean db_write(UInt lib, char *db_name, long length, int pos,
			Boolean force, Boolean quiet);
extern Boolean db_delete(char *db_name);

extern void db_progress_bar(Boolean init, char *str, int percent, int *lp, int pos);

#endif

