#ifndef __GNUC__
#pragma pack(2)
#endif

typedef unsigned char ubyte;
typedef signed char sbyte;
typedef unsigned short uword;
typedef short word;
typedef unsigned long udword;
typedef long dword;

typedef struct {
  ubyte name[32];
  uword fileAttributes;
  uword version;
  udword creationDate;
  udword modificationDate;
  udword lastBackupDate;
  udword modificationNumber;
  udword appInfoArea;
  udword sortInfoArea;
  ubyte databaseType[4];        
  ubyte creatorID[4];
  udword uniqueIDSeed;
  udword nextRecordListID;
  uword numberOfRecords;
} 
#ifdef __GNUC__
__attribute__ ((packed))
#endif
PdbHeader;

typedef struct {
  udword recordDataOffset;
  ubyte recordAttributes;
  ubyte uniqueID[3];
}
#ifdef __GNUC__
__attribute__ ((packed))
#endif
RecHeader;









