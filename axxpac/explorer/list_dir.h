/*
  list_dir.h - 2000 Till Harbaum

  list view for axxPac explorer
*/

#ifndef __LIST_DIR_H__
#define __LIST_DIR_H__

#include "explorer.h"

#define ICON_ERR    0   // card state icon: card unusable
#define ICON_INS    1   // card inserted and ok
#define ICON_BUSY   2   // card inserted and busy

#define ICON_DIR    3   // icons for list view
#define ICON_PRC    4
#define ICON_TXT    5
#define ICON_DB     6

extern Handle    resH[];
extern BitmapPtr resP[];

/* interface to plugins */
typedef struct {
  char extension[4];    /* file extension (jpg, gif, ...) */
  char icon[36];
  char name[32];
} ExplorerPlugin;

Err PluginGetInfo(UInt refNum, ExplorerPlugin *plugin) \
               SYS_TRAP(sysLibTrapCustom+0);
Err PluginExec(UInt refNum, UInt axxPacRef, axxPacFD, char *, long) \
               SYS_TRAP(sysLibTrapCustom+1);
Err PluginConfig(UInt refNum) \
               SYS_TRAP(sysLibTrapCustom+2);

#define PLUGINS 8
extern ExplorerPlugin plugin[PLUGINS];

// updated
extern void  list_initialize(void);

// not updated
extern void  list_dir(void);
extern void  list_refresh(Boolean scan);
extern void  list_scroll(Boolean abs, int pos);
extern void  list_select(int x, int y, int sel);
extern void  list_set_disp_mode(int mode, Boolean redraw);
extern void  list_set_click_mode(int mode);
extern int   list_get_disp_mode(void);
extern int   list_get_click_mode(void);

extern void  list_error(int id);

// this is also used in the memo export list
extern void length2ascii(char *p, long k);

extern int current_dir_cluster;
extern Boolean redraw_free_mem;

extern char saved_path[];

/* some caching stuff */
#ifdef USE_DATABASE_CACHE
void list_cache_release(void);
#endif

/* list mode */
#define LIST_PALM 0
#define LIST_CARD 1

/* click mode */
#define SEL_INFO  0
#define SEL_COPY  1
#define SEL_MOVE  2
#define SEL_DEL   3

/* file display columns */
#define NAME_POS   10
#define LEN_POS    80
#define DATE_POS   110
#define LAST_POS   152


#define SHOW_ENTRIES 12

struct db_entry {
  char name[32];                /* database name */
  char len_str[6];              /* string for ascii file length */
  Boolean is_rsrc;              /* resource db */
  ULong length;                 /* database length in bytes */
  ULong recs;                   /* number of records */
  LocalID dbId;                 /* database Id */
  ULong date;                   /* modification date */

  int cathegory;                /* link to cathegory */
  ULong creator, type;          /* creator of this cathegory */
};

struct db_cathegory {
  char name[32];                /* cathegory name */
  ULong creator, type;          /* creator of this cathegory */
  int first_entry;              /* id of first entry within this cathegory */
  int entries;                  /* number of entries in this cathegory */
};

#define LTYPE_SUBDIR  1
#define LTYPE_PALMPRC 2
#define LTYPE_PALMPDB 3
#define LTYPE_TEXT    4

#define LTYPE_PLUG0  32
#define LTYPE_PLUG1  33
#define LTYPE_PLUG2  34
#define LTYPE_PLUG3  35
#define LTYPE_PLUG4  36
#define LTYPE_PLUG5  37
#define LTYPE_PLUG6  38
#define LTYPE_PLUG7  39
#define LTYPE_PLUG8  40
#define LTYPE_PLUG9  41

/* directory entry as shown to the pilot */
#define MAX_NAMELEN   39

struct card_entry {
  char name[MAX_NAMELEN+1];        /* max 39 chars filename */
  char len_str[6];                 /* string for ascii file length */
  char date_str[10];               /* string for file date */
  unsigned long length, pdate;     /* file length, date in palm format */
  unsigned char type;              /* local attributes (LTYPE) */
};

extern void win_draw_chars_len(int x, int y, SWord max, char *str);
extern char *my_strrchr(char *str, char chr);

#endif
