/*
   axxPac.h  -  for axxPac library version 1.0x

   (c) 2000 Till Harbaum / AMS Software GmbH
*/

#ifndef __AXXPAC_H__
#define __AXXPAC_H__

#define AXXPAC_LIB_NAME     "axxPac driver"
#define AXXPAC_LIB_CREATOR  'axPC'

typedef short axxPacErr;

typedef enum { O_RdOnly, O_RdWr, O_Creat, O_RdFast } axxPacMode; /* open modes */
typedef short axxPacFD;                                /* file descriptor */ 

/* fileselector stuff */
#define AXXPAC_FSEL_BUFFERSIZE 256

/* DOS file attributes */
#define FA_RDONLY     0x01      /* read only         */
#define FA_HIDDEN     0x02      /* hidden file       */
#define FA_SYSTEM     0x04      /* system file       */
#define FA_VOLUME     0x08      /* disk name         */
#define FA_SUBDIR     0x10      /* subdirectory      */
#define FA_ARCH       0x20      /* archive           */

/* seek modes */
#define SEEK_SET   0
#define SEEK_CUR   1
#define SEEK_END   2

/* search modes for findfirst/findnext */
#define FIND_FILES_ONLY   0     /* return matching files only                */
#define FIND_FILES_N_DIRS 1     /* return matching files and directories     */
#define FIND_FILES_N_ALL_DIRS 2 /* return matching files and all directories */

#define RESERVED_SIZE 290
typedef struct {
  unsigned char attrib;     /* DOS file attributes                     */
  UInt32   date;            /* creation time/date in palmos format     */
  Int32   size;             /* file size                               */
  short   stcl;             /* start cluster                           */
  char    name[256];        /* file name                               */
  char    reserved[RESERVED_SIZE];  /* used by system -- don't modify! */
} axxPacFileInfo;

/* stat info */
typedef struct {
  Int32 size;             /* file size                               */
  UInt32 date;            /* creation time/date in palmos format     */
  unsigned char attrib;   /* DOS file attributes                     */
} axxPacStatType;

/* library info */
typedef struct {
  char  version[16];      /* library version string         */
  char  creator[16];      /* name of creator of the library */
  char  revision;         /* library interface revision     */
} axxPacLibInfo;

#define AXXPAC_MAX_ZONES 16  /* 16 zones = 16*16MBytes = 256 MBytes */

/* info on smartmedia zone (16 MBytes) */
typedef struct {
  int used;               /* number of used blocks in zone */
  int free;               /* number of free blocks in zone */
  int unusable;           /* number of unusable blocks in zone */
} axxPacZoneInfo;

/* card info */
typedef struct {
  Boolean  inserted;              /* card in slot? */
  
  unsigned char  vendor_id;       /* vendor infos */
           char  vendor_name[16];

  unsigned char  device_id;       /* device infos */
           char  device_name[16];

  unsigned char  size;            /* size in MBytes */
  unsigned char  spb;             /* sectors (a 512 bytes) per block */

  unsigned char  zones;           /* number of zones */
  axxPacZoneInfo zone_info[AXXPAC_MAX_ZONES];
} axxPacCardInfo;

/* state of card for callback function */
typedef enum {
  axxPacCardNone=0,     /* no card inserted                 */
  axxPacCardUnusable,   /* card inserted, but unusable      */
  axxPacCardOk,         /* card inserted and usable         */
  axxPacCardBusy        /* card inserted, usable and in use */
} axxPacCardState;

#ifdef AXXPAC_NOTRAPDEFS
#define AXXPAC_SYS_TRAP(trap)
#else
#define AXXPAC_SYS_TRAP(trap) SYS_TRAP(trap)
#endif

/* lib functions */
axxPacErr      axxPacLibOpen(UInt refNum) \
                        AXXPAC_SYS_TRAP(sysLibTrapOpen);
axxPacErr      axxPacLibClose(UInt refNum, UIntPtr numappsP) \
                        AXXPAC_SYS_TRAP(sysLibTrapClose);

char*          axxPacStrError(UInt refnum) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+0);
axxPacErr      axxPacGetLibInfo(UInt refnum, axxPacLibInfo*) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+1);

/* card functions */
axxPacErr      axxPacInstallCallback(UInt refnum, void(*callback)(axxPacCardState)) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+2);
axxPacErr      axxPacGetCardInfo(UInt refnum, axxPacCardInfo*) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+3);
axxPacErr      axxPacFormat(UInt refnum, void(*callback)(unsigned char)) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+4);
axxPacErr      axxPacDiskspace(UInt refnum, DWord*) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+5);

/* directory functions */
axxPacErr      axxPacExists(UInt refnum, char *) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+6);
axxPacErr      axxPacFindFirst(UInt refnum, char *, axxPacFileInfo*, int) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+7);
axxPacErr      axxPacFindNext(UInt refnum, axxPacFileInfo*) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+8);
axxPacErr      axxPacNewDir(UInt refnum, char *) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+9);
axxPacErr      axxPacChangeDir(UInt refnum, char *dir) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+10);
char*          axxPacGetCwd(UInt refnum) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+11);
axxPacErr      axxPacDelete(UInt refnum, char *) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+12);
axxPacErr      axxPacRename(UInt refnum, char*, char*) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+18);
axxPacErr      axxPacSetWP(UInt refnum, char*, Boolean) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+19);
axxPacErr      axxPacStat(UInt refnum, char*, axxPacStatType*) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+20);
axxPacErr      axxPacSetAttrib(UInt refnum, char*, unsigned char) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+23);
axxPacErr      axxPacSetDate(UInt refnum, char*, UInt32) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+24);

/* file io */
axxPacFD       axxPacOpen(UInt refNum, char *name, axxPacMode) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+13);
axxPacErr      axxPacClose(UInt refNum, axxPacFD) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+14);
long           axxPacRead(UInt refnum, axxPacFD, void*, long) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+15);
long           axxPacWrite(UInt refnum, axxPacFD, void*, long) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+16);
long           axxPacTell(UInt refnum, axxPacFD) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+21);
axxPacErr      axxPacSeek(UInt refnum, axxPacFD, long, int) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+17);

/* file selector */
axxPacErr      axxPacFsel(UInt refnum, char *, char *, char *, Boolean) \
                        AXXPAC_SYS_TRAP(sysLibTrapCustom+22);

#endif // __AXXPAC_H__
