/*

  memopad interface header file

*/

#ifndef __MEMO_H__
#define __MEMO_H__

#define MEMODB_NAME "MemoDB"
#define MAX_MEMO_LEN 32768   /* max length of files to be installed in the memo pad */
#define MAX_MEMO_NAME 32

#define XLINES 10
#define LENWID 30
#define NUMWID 12

struct memo_info {
  char name[MAX_MEMO_NAME+1];
  char len[6];
  int size,record;
};

extern void memo_import(char *name, long flen);
extern void memo_export(void);

#endif  // __MEMO_H__
