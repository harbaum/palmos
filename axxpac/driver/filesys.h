#ifndef __FILESYS_H__
#define __FILESYS_H__

#include "../api/axxPac.h"

/* store FAT in database and save memory */
#define STORE_FAT_IN_DB
#define FAT_DB_NAME "axxPacFAT"
#define FAT_DB_TYPE 'FETT'

/* use unix style path delimiters */
#define DELIMITER     '/'
#define DELIMITER_STR "/"

/* debug settings */
#define noDEBUG_PATH
#define noDEBUG_DELETE
#define noDEBUG_READ
#define noDEBUG_SEEK
#define noDEBUG_NEWDIR
#define noDEBUG_CHDIR
#define DEBUG_FWRITE
#define noDEBUG_OPEN
#define noDEBUG_MATCH
#define noDEBUG_FIND
#define noDEBUG_RENAME
#define noDEBUG_SETWP
#define noDEBUG_STAT

#ifdef DEBUG_FIND
#define DEBUG_MSG_FIND(args...) DEBUG_MSG(##args)
#else
#define DEBUG_MSG_FIND(args...)
#endif

#ifdef DEBUG_CHDIR
#define DEBUG_MSG_CHDIR(args...) DEBUG_MSG(##args)
#else
#define DEBUG_MSG_CHDIR(args...)
#endif

#ifdef DEBUG_RENAME
#define DEBUG_MSG_RENAME(args...) DEBUG_MSG(##args)
#else
#define DEBUG_MSG_RENAME(args...)
#endif

#ifdef DEBUG_SETWP
#define DEBUG_MSG_SETWP(args...) DEBUG_MSG(##args)
#else
#define DEBUG_MSG_SETWP(args...)
#endif

#ifdef DEBUG_MATCH
#define DEBUG_MSG_MATCH(args...) DEBUG_MSG(##args)
#else
#define DEBUG_MSG_MATCH(args...)
#endif

#ifdef DEBUG_PATH
#define DEBUG_MSG_PATH(args...) DEBUG_MSG(##args)
#else
#define DEBUG_MSG_PATH(args...)
#endif

#ifdef DEBUG_SEEK
#define DEBUG_MSG_SEEK(args...) DEBUG_MSG(##args)
#else
#define DEBUG_MSG_SEEK(args...)
#endif

#ifdef DEBUG_STAT
#define DEBUG_MSG_STAT(args...) DEBUG_MSG(##args)
#else
#define DEBUG_MSG_STAT(args...)
#endif

#ifdef DEBUG_READ
#define DEBUG_MSG_READ(args...) DEBUG_MSG(##args)
#else
#define DEBUG_MSG_READ(args...)
#endif

#ifdef DEBUG_DELETE
#define DEBUG_MSG_DELETE(args...) DEBUG_MSG(##args)
#else
#define DEBUG_MSG_DELETE(args...)
#endif

#ifdef DEBUG_NEWDIR
#define DEBUG_MSG_NEWDIR(args...) DEBUG_MSG(##args)
#else
#define DEBUG_MSG_NEWDIR(args...)
#endif

#ifdef DEBUG_FWRITE
#define DEBUG_MSG_WRITE(args...) DEBUG_MSG(##args)
#else
#define DEBUG_MSG_WRITE(args...)
#endif

#ifdef DEBUG_OPEN
#define DEBUG_MSG_OPEN(args...) DEBUG_MSG(##args)
#else
#define DEBUG_MSG_OPEN(args...)
#endif

#define PALM_MAX_NAMELEN  37  /* 32 + extension + \000   */

#define MAX_FILENAME_LEN 256
#define MAX_FIND_PATTERN (MAX_FILENAME_LEN)
#define MAX_PATH_LEN     512

#define FLUSH_BUFFER -1       /* fs_buffer_sec */

typedef struct {
  unsigned char buffer[512];  /* sector buffer */
  long sec_no;                /* sector no */
  Boolean dirty;              /* true if buffer dirty */
} sector_buffer;

struct file_state_struct {
  axxPacFD fd;                /* fd of this entry */

  sector_buffer sector;       /* file sector buffer */
  long pos;                   /* curent file position */
  int cluster;                /* current cluster */
  int last_cluster;           /* for elonging chain during write */
  int sec;                    /* sector offset within cluster */
  int byte;                   /* byte offset within sector */

  long flen;                  /* current file length */

  Boolean fast;               /* fast read (no ECC verification) */

  Boolean writeable;          /* this file may be written */
  Boolean update_dir;         /* dir entry needs to be updated */
  int stcl;                   /* new start cluster */

  unsigned long dir_sec;      /* sector with dir entry */
  unsigned int dir_offset;    /* offset of dir entry */

  struct file_state_struct *next; /* chain ... */
};

typedef struct file_state_struct file_state;

/* status for fs_findfirst/next */
typedef struct {
  int  cluster, cnt, mode;
  int  offset, ns_offset, bk_offset;
  long sec, ns_sec, bk_sec;
} find_state;

/* extended status for fs_findfirst/next */
typedef struct {
  find_state find;
  char pattern[MAX_FIND_PATTERN];
} find_pstate;

typedef struct {
  /* temporary buffer */
  char tmp[MAX_FILENAME_LEN];

  /* ascii path buffer */
  char path_str[MAX_PATH_LEN];

  /* fat buffer */
  unsigned char *fat_buffer;

  /* partition / disk parameter */
  int p_start;
  unsigned short bps, spc, res, nfats, ndirs, spf;
  unsigned long  nsects, first_dsec;
  unsigned short total_cl;

  /* information needed for read/write file */
  struct file_state_struct *file_list;

  /* directory stuff */
  int current_dir_cluster;
  sector_buffer dir_sector;
  Boolean fat_dirty;

  Boolean is_fat12;

  Boolean fs_ok;

#ifdef STORE_FAT_IN_DB
  /* database handle for storing FAT in database */
  DmOpenRef dbRef;
#endif
} filesys_globals;

/* normal DOS dir stuff */
#define ROOTDIR 0

#define DOS2USHORT(a)  ( ((a&0xff)<<8)|((a&0xff00)>>8) )
#define DOS2ULONG(a)   ( ((a&0xff)<<24)|((a&0xff00)<<8)|((a&0xff0000)>>8)|((a&0xff000000)>>24) )

struct dos_dir_entry {
  unsigned char name[11]; /* filename */
  unsigned char attr;     /* file attributes */
  char dummy[10];         /* filler (unused) */
  unsigned short time;    /* DOS time */
  unsigned short date;    /* DOS date */
  unsigned short stcl;    /* start cluster */
  unsigned long  flen;    /* file length */
};

/* special vfat stuff */
#define VSE_NAMELEN 13
#define VSE_MASK 0x1f
#define VSE_LAST 0x40

#define VSE1SIZE 5
#define VSE2SIZE 6
#define VSE3SIZE 2

#define PACKED __attribute__ ((packed))

struct unicode_char {
  char lchar;
  char uchar;
};

struct vfat_dir_entry {
  unsigned char id;               /* 0x40 = last; & 0x1f = VSE ID */
  struct unicode_char text1[VSE1SIZE] PACKED;
  unsigned char attribute;        /* 0x0f for VFAT */
  unsigned char hash1;            /* Always 0? */
  unsigned char sum;              /* Checksum of short name */
  struct unicode_char text2[VSE2SIZE] PACKED;
  unsigned char sector_l;         /* 0 for VFAT */
  unsigned char sector_u;         /* 0 for VFAT */
  struct unicode_char text3[VSE3SIZE] PACKED;
};

#endif



