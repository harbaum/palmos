/*
  errors.h - 2000 Till Harbaum

  axxPac error messages
 */

#ifndef AXXPAC_ERROR
#ifndef ERRORS_H
/* First time through, define the enum list */
#define AXXPAC_ENUM_LIST
#else
/* Repeated inclusions of this file are no-ops unless AXXPAC_ERROR is defined */
#define AXXPAC_ERROR(code,string)
#endif /* ERRORS_H */
#endif /* AXXPAC_ERROR */

#ifdef AXXPAC_ENUM_LIST

typedef enum {

#define AXXPAC_ERROR(code,string)	code ,

#endif /* AXXPAC_ENUM_LIST */

/* non errors go north of here with negative numbers */
AXXPAC_ERROR(ErrNone=0,     "No error")

/* smartmedia errors */
AXXPAC_ERROR(ErrNoTable,    "No block translation table")

AXXPAC_ERROR(ErrWrongCPU,   "This program does not run on this CPU")
AXXPAC_ERROR(ErrHwVersion,  "Unexpected hardware version installed")
AXXPAC_ERROR(ErrNoHw,       "No axxPac hardware installed")
AXXPAC_ERROR(ErrSmNoDev,    "No card inserted")

/* file system errors */
AXXPAC_ERROR(ErrFsNotFound, "No such file or directory")
AXXPAC_ERROR(ErrFsInvMBR,   "Invalid Master Boot Record")
AXXPAC_ERROR(ErrFsNoFAT12,  "No FAT12 filesystem")
AXXPAC_ERROR(ErrFsSecSiz,   "Illegal sector size")
AXXPAC_ERROR(ErrFsFATOOM,   "Out of memory allocating FAT")
AXXPAC_ERROR(ErrFsPat2Long, "Search pattern too long")
AXXPAC_ERROR(ErrFsWrFAT1,   "Unable to write FAT 1")
AXXPAC_ERROR(ErrFsWrFAT2,   "Unable to write FAT 2")
AXXPAC_ERROR(ErrFsRdFAT,    "Unable to read FAT")
AXXPAC_ERROR(ErrFsWrProt,   "Media is write protected")
AXXPAC_ERROR(ErrFsRdPBS,    "Unable to read partition boot sector")
AXXPAC_ERROR(ErrFsRdMBR,    "Unable to read master boot record")
AXXPAC_ERROR(ErrFsDefFAT,   "FAT is defective")
AXXPAC_ERROR(ErrFsRdSec,    "Unable to read sector")
AXXPAC_ERROR(ErrFsWrSec,    "Unable to write sector")
AXXPAC_ERROR(ErrFsRtFull,   "The root directory is full")
AXXPAC_ERROR(ErrFsDskFull,  "The media is full")
AXXPAC_ERROR(ErrFsIllName,  "A file or directory name must start with a character or digit")
AXXPAC_ERROR(ErrFsError,    "Unable to initialize file system")
AXXPAC_ERROR(ErrFsNoFile,   "File or directory not found")
AXXPAC_ERROR(ErrFsNoSys,    "No valid filesystem")
AXXPAC_ERROR(ErrFsNOpen,    "Unable to open card")
AXXPAC_ERROR(ErrFsDirNE,    "Unable to delete directory because it is not empty")
AXXPAC_ERROR(ErrFsOpenF,    "Unable to open file")
AXXPAC_ERROR(ErrFsUnkFD,    "Unknown file descriptor")
AXXPAC_ERROR(ErrFsOffset,   "File offset out of range")
AXXPAC_ERROR(ErrFsNamExists,"Filename already exists")
AXXPAC_ERROR(ErrFsIllegal,  "Illegal file or directory name")
AXXPAC_ERROR(ErrFsNoPath,   "Path not found")
AXXPAC_ERROR(ErrFsFileProt, "File is write protected")
AXXPAC_ERROR(ErrFsel,       "Unable to open fileselector")
AXXPAC_ERROR(ErrFsFormat,   "Format error")
AXXPAC_ERROR(ErrFsMoveDir,  "Unable to move a directory to a subdirectory of itself")

/* smartmedia errors */
AXXPAC_ERROR(ErrSmOOM,      "Out of memory initializing media")
AXXPAC_ERROR(ErrSmWrVer,    "Write verify error")
AXXPAC_ERROR(ErrSmNoSec,    "No such sector")
AXXPAC_ERROR(ErrSmECC,      "Wrong ECC")
AXXPAC_ERROR(ErrSmTooM,     "Too many defective blocks")
AXXPAC_ERROR(ErrSmUnkDev,   "Unknown smartmedia device")
AXXPAC_ERROR(ErrSmFormat,   "Media is unusable")
AXXPAC_ERROR(ErrSmUnusable, "The current media is unusable. Please replace or format it")
AXXPAC_ERROR(ErrSmMemBuf,   "Out of memory allocating buffer")
AXXPAC_ERROR(ErrSmNoCard,   "Media removed")

/* lib errors */
AXXPAC_ERROR(ErrLibOpen,    "Unable to open hardware")
AXXPAC_ERROR(ErrNoGlobals,  "Globals not valid.")

#ifdef AXXPAC_ENUM_LIST
  ErrLast
} AXXPAC_ERROR_CODE;

#undef AXXPAC_ENUM_LIST
#endif /* AXXPAC_ENUM_LIST */

/* Zap AXXPAC_ERROR macro so that future re-inclusions do nothing by default */
#undef AXXPAC_ERROR

#ifndef ERRORS_H
#define ERRORS_H

#define MAX_ERRLEN  256

typedef struct {
  Err smartmedia_err; /* last error code from smartmedia layer */
  Err filesys_err;    /* last error code from filesystem/lib internals */
  
  char msg_buffer[MAX_ERRLEN];  /* buffer for error messages */
} error_globals;

#endif /* ERRORS_H */
