/* 

   list_dir.c - directory listing / gui for SmartSync

   (c) 1999 by Till Harbaum


   ToDo:
   - save database infos in database for cache

*/

#ifdef OS35HDR
#include <PalmOS.h>
#include <PalmCompatibility.h>
#else
#include <Pilot.h>
#endif

#include "../api/axxPac.h"
#include "explorerRsc.h"

#include "db.h"
#include "list_dir.h"
#include "explorer.h"
#include "memo.h"
#include "beam.h"
#include "callback.h"

#define CARD 0  /* only work with card 0 */

/* plugins */
ExplorerPlugin plugin[PLUGINS];

/* icon stuff */
#define ICONS 7
int icons[]={ IconError, IconInserted, IconBusy, IconFolder, 
	      IconSync, IconText, IconDB };
Handle    resH[ICONS];
BitmapPtr resP[ICONS];
Boolean icons_ok = false;

/* current date format */
DateFormatType dateformat;

/* current directory list mode (Palm/Card) */
Boolean disp_mode = LIST_PALM;

/* current open mode (Info/Open) */
int click_mode = SEL_COPY;

/* current palm list view hirarchy */
int current_cath = -1, current_cath_crc;
int cathegories, total_entries;

char saved_path[512];

#define CATH_UNKNOWN 4

/* creators of the system files to create pseudo hierarchy */
ULong fixed_creator_system[]={
  /* real system files */
  'netl', 'graf', 'gdem', 'hpad', 'palm', 'irda', 'psys', 
  'slip', 'ppp_', 'loop', 'digi', 'bttn', 'shct', 'ownr', 
  'netw', 'modm', 'gnrl', 'frmt', 'ping', 'u328', 'u650',
  'ircm', 'swrp', 'htcp', 'nett', 'u8EZ',

  /* and axxPac driver and plugin's */
  'axPC', 'axP0', 'axP1', 'axP2', 'axP3', 'axP4', 'axP5', 
  'axP6', 'axP7', 'axP8', 'axP9', 0 };

ULong fixed_creator_builtin[]={
  'todo', 'netw', 'memo', 'mail', 'lnch', 'addr', 'setp', 
  'secr', 'pref', 'exps', 'date', 'calc', 'sync', 0, };

/* find creator id in list of ids */
Boolean list_find_creator(ULong *table, ULong id) {
  /* search to end of table */
  while(*table!=0)
    if(*table++ == id)
      return true;

  return false;
}

#ifdef USE_DATABASE_CACHE
/* some caching stuff */
char list_cache_db_name[]="axxPac cache";

void list_cache_release(void) {
  LocalID dbId;

  /* delete database if existent */
  if((dbId = DmFindDatabase(0, list_cache_db_name))!=0)
    DmDeleteDatabase(0, dbId);
}

Boolean list_write_data(DmOpenRef dbRef, UInt rec, void *wdata, int length) {
  VoidHand hand;
  UInt at;
  VoidPtr data;

  /* write cache records */
  if((hand = DmNewRecord(dbRef, &rec, length))==NULL)
    return false;

  /* lock resource */
  data = MemHandleLock(hand);

  /* finally copy data */
  DmWrite(data, 0, wdata, length);

  /* unlock resource */
  MemPtrUnlock(data);

  /* release record */
  DmReleaseRecord(dbRef, rec, true); 

  return true;
}

/* save some data in a cache database */
void list_cache_create(void *data0, unsigned int length0,
		       void *data1, unsigned int length1) {
  LocalID dbId;
  DmOpenRef dbRef;

  /* database not already existent? */
  if((dbId = DmFindDatabase(0, list_cache_db_name))==0) {

    /* create new db, but don't complain if impossible */
    if(DmCreateDatabase(0, list_cache_db_name, CREATOR, CACHE_TYPE, false)!=0)
      return;

    /* find it now */
    if((dbId = DmFindDatabase(0, list_cache_db_name))==0)
      return;
  }

  /* dbID now points to a valid database */
  if((dbRef = DmOpenDatabase(0, dbId, dmModeReadWrite))==0) return;

  /* remove existing records */
  while(DmNumRecords(dbRef)>0) DmRemoveRecord(dbRef, 0);

  if((!list_write_data(dbRef, 0, data0, length0))||
     (!list_write_data(dbRef, 1, data1, length1))) {

    /* something went wrong -> delete whole database */
    DmCloseDatabase(dbRef);
    list_cache_release();
    return;
  }

  /* and close it */
  DmCloseDatabase(dbRef);
}

Boolean list_read_data(DmOpenRef dbRef, UInt rec, void **data, int *length) {
  LocalID chunk;
  VoidPtr ptr;
  long    len;

  DmRecordInfo(dbRef, rec, NULL, NULL, &chunk);
  ptr = MemLocalIDToLockedPtr(chunk, 0);
  len = MemPtrSize(ptr);

  *data = MemPtrNew(len);
  if(*data == NULL) {
    MemPtrUnlock(ptr);
    return false;
  }

  MemMove(*data, ptr, len);
  MemPtrUnlock(ptr);
  *length = len;
	    
  return true;
}

/* retrieve data from the cache database */
Boolean list_cache_retrieve(void **data0, int *length0,
			    void **data1, int *length1) {
  LocalID dbId;
  DmOpenRef dbRef;

  /* database not already existent? */
  if((dbId = DmFindDatabase(0, list_cache_db_name))==0) 
    return false;

  /* dbID now points to a valid database */
  if((dbRef = DmOpenDatabase(0, dbId, dmModeReadWrite))==0) return;

  if((!list_read_data(dbRef, 0, data0, length0))||
     (!list_read_data(dbRef, 1, data1, length1))) {

    /* something went wrong -> delete whole database */
    DmCloseDatabase(dbRef);
    list_cache_release();
    return false;
  }

  /* and close it */
  DmCloseDatabase(dbRef);

  return true;
}
#endif

/* names of the fixed palm cathegories */
char *fixed_cathegory_name[]={ "[System]", "[Built-In]", 
		      "[Hacks/DA's]", "[Documents]", 
		      "[unknown]" };

/* show alert box with error message in string id */
void list_error(int id) {
  char *msg=MemHandleLock(DmGetResource(strRsc, id));
  FrmCustomAlert(alt_err, msg, 0,0);
  MemPtrUnlock(msg);
}

/* show infos on file */
void
file_info(char *fname, struct card_entry *cdir) {
  FormPtr frm;
  FieldPtr fld;
  VoidHand hand;
  char *ptr;
  axxPacFileInfo info;
  char size_str[256];
  SystemPreferencesType sysPrefs;
  DateTimeType date;
  char date_str[longDateStrLength];
  char time_str[timeStringLength];
  Boolean set;
  extern void hide(FormPtr frm, Word object);
  int i,j;
  long plugin_name = 'axP0';
  UInt PluginRef;
  axxPacFD fd;

  /* get system preferences for date format */
  PrefGetPreferences(&sysPrefs);

  frm = FrmInitForm(frm_FileInfo);

  /* hide plugin button */
  if(!((cdir->type >= LTYPE_PLUG0)&&
       (cdir->type <= LTYPE_PLUG9))) {
    CtlSetUsable(GetObjectPtr(frm, FileInfoPlugin), false);
  }

  /* hide beam button */
  if((cdir->type != LTYPE_PALMPRC)&&
     (cdir->type != LTYPE_PALMPDB)) {
    CtlSetUsable(GetObjectPtr(frm, FileInfoBeam), false);
  }

  /* get more detailed info on file */
  if(axxPacFindFirst(LibRef, fname, &info, FIND_FILES_N_DIRS)) {
    list_error(StrFindFile);
    return;
  }

  /* get a new chunk for text */
  if((hand = MemHandleNew(StrLen(info.name)+1)) == 0) {
    list_error(StrMemBuf);
    return;
  }

  /* fill in filename */
  ptr = MemHandleLock(hand);
  StrCopy(ptr, info.name);
  MemHandleUnlock(hand);
  
  /* set date and time */
  TimSecondsToDateTime(info.date, &date);
  DateToAscii(date.month, date.day, date.year, 
	      sysPrefs.longDateFormat, date_str);
  CtlSetLabel(GetObjectPtr(frm, FileInfoDate), date_str);
  TimeToAscii(date.hour, date.minute, sysPrefs.timeFormat, time_str);
  CtlSetLabel(GetObjectPtr(frm, FileInfoTime), time_str);
  
  /* set flags */
  if(info.attrib & FA_SUBDIR) {
    StrCopy(size_str, "---");
    CtlSetValue(FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,FileInfoDir)),1);
  } else {
    StrPrintF(size_str, "%ld bytes", info.size);
    CtlSetEnabled(FrmGetObjectPtr(frm,FrmGetObjectIndex(frm,FileInfoRO)),1);
  }
  
  /* set file size */
  CtlSetLabel(GetObjectPtr(frm, FileInfoSize), size_str);
  
  if(info.attrib & FA_RDONLY) 
    CtlSetValue(FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,FileInfoRO)),1);
  
  /* set name */
  fld = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, FileInfoName));
  FldSetTextHandle(fld, (Handle)hand);

  /* set focus on name field */
  FrmSetFocus(frm, FrmGetObjectIndex (frm, FileInfoName));

  /* and run the dialog */
  switch(FrmDoDialog(frm)) {

  case FileInfoMove:
    FrmDeleteForm(frm);

    /* move file on media */
    StrCopy(size_str, cdir->name);  /* copy name */
    if((i = axxPacFsel(LibRef, "select destination", size_str, "", false))<0) {
      FrmCustomAlert(alt_err, axxPacStrError(LibRef),0,0);
      list_dir();
      return;
    }

    /* hit the ok button */
    if(i==1) {
      /* remove everything up to last slash */
      j = StrLen(size_str)-1;
      while((j>0)&&(size_str[j] != '/')) size_str[j--] = 0;
      StrCat(size_str, cdir->name);

      if(axxPacRename(LibRef, cdir->name, size_str) < 0)
	FrmCustomAlert(alt_err, axxPacStrError(LibRef),0,0);
    }
    /* rescan dir */
    list_dir();
    break;

  case FileInfoPlugin:
    FrmDeleteForm(frm);

    /* create name 'axP?' */
    i = LTYPE_PLUG0 - cdir->type;
    plugin_name = (plugin_name & 0xffffff00)|('0'+i); 
	    
    if(!SysLibLoad('libr', plugin_name, &PluginRef)) {
#ifdef DEBUG_PLUGIN
      DEBUG_MSG("running plugin %d\n", i);
#endif
      
      fd = axxPacOpen(LibRef, cdir->name, O_RdOnly);
      PluginExec(PluginRef, LibRef, fd, cdir->name, cdir->length);
      axxPacClose(LibRef, fd);
      SysLibRemove(PluginRef);
    }
    break;

  case FileInfoBeam:
    FrmDeleteForm(frm);
    file_beam(cdir->name, cdir->length);
    break;

  case FileInfoOK:  
    /* did name change? */
    if(StrCaselessCompare(info.name,
			  FldGetTextPtr(FrmGetObjectPtr(frm, 
			  FrmGetObjectIndex (frm, FileInfoName)))) != 0) {

      /* name did change */
      if(axxPacRename(LibRef, info.name,
		      FldGetTextPtr(FrmGetObjectPtr(frm, 
		      FrmGetObjectIndex (frm, FileInfoName)))) < 0) {
	FrmDeleteForm(frm);
	FrmCustomAlert(alt_err, axxPacStrError(LibRef),0,0);
	list_dir();
	return;
      }
    }	  

    /* ok, set values */
    if(!(info.attrib & FA_SUBDIR)) {

      /* get state of write protect marker */
      set = CtlGetValue(FrmGetObjectPtr(frm, 
	    FrmGetObjectIndex(frm,FileInfoRO)));

      if(( set && !(info.attrib & FA_RDONLY))||
	 (!set &&  (info.attrib & FA_RDONLY))) {
	/* ok, set write protect bit */
	if(axxPacSetWP(LibRef, info.name, set) < 0) {
	  FrmDeleteForm(frm);
	  FrmCustomAlert(alt_err, axxPacStrError(LibRef),0,0);
	  list_dir();
	  return;
	}
      }
    }

    FrmDeleteForm(frm);

    /* free mem on card may have changed (directory may be longer now) */
    redraw_free_mem = true;

    /* rescan entry */
    list_dir();
    break;

  default:
    FrmDeleteForm(frm);
    break;
  }
}

/* show infos on database */
void
db_info(char *db_name) {
  FormPtr frm;
  char size_str[32], entry_str[32];
  SystemPreferencesType sysPrefs;
  DateTimeType date;
  char date_str[longDateStrLength];
  char time_str[timeStringLength];
  LocalID dbId;
  DmOpenRef dbRef=0;
  ULong type[2]={0,0}, creator[2]={0,0}, cdate, length, entries;
  uword attributes, version;
  char version_str[32];
  VoidHand hand;
  VoidPtr p;
  Int index;

  /* get system preferences for date format */
  PrefGetPreferences(&sysPrefs);
  frm = FrmInitForm(frm_DbInfo);

  /* set name */
  CtlSetLabel(GetObjectPtr(frm, DbName), db_name);

 /* find database */
  if((dbId = DmFindDatabase(0, db_name))==0) {
    list_error(StrFindDB);
    return;
  }
  
  /* and open it */
  if((dbRef = DmOpenDatabase(0, dbId, dmModeReadOnly))==0) {
    list_error(StrOpenDB);
    return;
  }

  /* get database info */
  DmDatabaseInfo(CARD, dbId, NULL, &attributes, &version, NULL, 
		 &cdate, NULL, NULL, NULL, NULL, type, creator);

  /* get size of database */
  DmDatabaseSize(CARD, dbId, &entries, &length, NULL);
 
  StrCopy(version_str, "---");
  /* resource / rec db */
  if(attributes & dmHdrAttrResDB) {

    /* find first type resource */
    if((index = DmFindResourceType(dbRef, 'tver', 0))>=0) {

      /* get version resource */
      hand = DmGetResourceIndex(dbRef, index);
      if(hand != NULL) {
	p = MemHandleLock(hand);
	StrCopy(version_str, p);
	MemPtrUnlock(p);
	DmReleaseResource(hand);
      }
    }
  }

  /* and close it again */
  DmCloseDatabase(dbRef);

  /* resource / rec db */
  if(attributes & dmHdrAttrResDB)
    CtlSetValue(FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,DbRsrc)),1);
  else
    CtlSetValue(FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,DbRec)),1);

  /* read only flag */
  if(attributes & dmHdrAttrReadOnly)
    CtlSetValue(FrmGetObjectPtr(frm, FrmGetObjectIndex(frm,DbRO)),1);

  /* set date and time */
  TimSecondsToDateTime(cdate, &date);
  DateToAscii(date.month, date.day, date.year, 
	      sysPrefs.longDateFormat, date_str);
  CtlSetLabel(GetObjectPtr(frm, DbDate), date_str);
  TimeToAscii(date.hour, date.minute, sysPrefs.timeFormat, time_str);
  CtlSetLabel(GetObjectPtr(frm, DbTime), time_str);
  
  /* set file size */
  StrPrintF(entry_str, "%ld entries", entries);
  CtlSetLabel(GetObjectPtr(frm, DbEntries), entry_str);
  StrPrintF(size_str, "%ld bytes", length);
  CtlSetLabel(GetObjectPtr(frm, DbSize), size_str);

  /* set version string */
  CtlSetLabel(GetObjectPtr(frm, DbVersion), version_str);  

  /* set creator string */
  CtlSetLabel(GetObjectPtr(frm, DbCreator), (char*)creator);
  CtlSetLabel(GetObjectPtr(frm, DbType), (char*)type);

  /* and run the dialog */
  FrmDoDialog(frm);

  /* and remove it */
  FrmDeleteForm(frm);
}

/* write length limited string followed by dots at pos */
void 
win_draw_chars_len(int x, int y, SWord max, char *str) {
  SWord lenp;
  Boolean fit;
  const char dots[]="...";

  lenp = StrLen(str);
  fit = true;
  FntCharsInWidth(str, &max, &lenp, &fit);

  /* something cut off? */
  if(lenp != StrLen(str)) {
    max = max-FntLineWidth((char*)dots,3);
    FntCharsInWidth(str, &max, &lenp, &fit);
    WinDrawChars(str, lenp, x, y);
    WinDrawChars((char*)dots, 3, x+max, y);
  } else
    WinDrawChars(str, lenp, x, y);
}

/* list database entry */
static void 
palm_print_dir_entry(struct db_entry *db, int y) {
  DateType date;
  char str[32];

  if(db==NULL) {
    WinDrawBitmap (resP[ICON_DIR], 0, 15+y*11);   
    win_draw_chars_len(NAME_POS, 15+y*11, LAST_POS-NAME_POS, "..");
  } else {
    WinDrawBitmap (resP[db->is_rsrc?ICON_DB:ICON_PRC], 0, 15+y*11);

    /* draw name (max 80 pixel) */
    win_draw_chars_len(NAME_POS, 15+y*11, LEN_POS-NAME_POS, db->name);

    /* draw length */
    WinDrawChars(db->len_str, StrLen(db->len_str),
		 DATE_POS-FntLineWidth(db->len_str, StrLen(db->len_str)), 15+y*11);
  
    /* draw date */
    DateSecondsToDate(db->date, &date);
    DateToAscii(date.month, date.day, date.year + firstYear, dateformat, str);
    WinDrawChars(str, StrLen(str), LAST_POS-FntLineWidth(str, StrLen(str)), 15+y*11);  
  }
}

/* list cathegory */
static void 
palm_print_dir_cathegory(struct db_cathegory *db, int y) {
  DateType date;
  char str[32];

  WinDrawBitmap(resP[ICON_DIR], 0, 15+y*11);   

  /* draw name (max 80 pixel) */
  win_draw_chars_len(NAME_POS, 15+y*11, DATE_POS-NAME_POS, db->name);

  /* draw contents */
  StrPrintF(str, "%d", db->entries);
  WinDrawChars(str, StrLen(str), LAST_POS-FntLineWidth(str, StrLen(str)), 15+y*11);  
}

/* list an card entry */
static void 
card_print_dir_entry(struct card_entry *dir, int y) {
  char str[32];
  SWord widp, lenp;
  Boolean fit;
  short icons[]={ICON_DIR, ICON_DB, ICON_PRC, ICON_TXT };

  /* draw icon */
  if((dir->type >= LTYPE_PLUG0)&&(dir->type <= LTYPE_PLUG9))
    WinDrawBitmap((BitmapPtr)(plugin[dir->type - LTYPE_PLUG0].icon), 
		  0, 15+y*11);
  else if(dir->type)
    WinDrawBitmap(resP[icons[dir->type-1]], 0, 15+y*11);

  /* draw name (max 70 pixel) */
  win_draw_chars_len(NAME_POS, 15+y*11, LEN_POS-NAME_POS, dir->name);

  /* draw length */
  WinDrawChars(dir->len_str, StrLen(dir->len_str), 
	       DATE_POS-FntLineWidth(dir->len_str, 
		    StrLen(dir->len_str)), 15+y*11);

  /* draw date */
  WinDrawChars(dir->date_str, StrLen(dir->date_str), 
	       LAST_POS-FntLineWidth(dir->date_str, 
		    StrLen(dir->date_str)), 15+y*11);  
}

/* number of dir entries in the currently displayed directory */
int current_entries=0, current_disp_offset=0;
int parent_disp_offset;

static struct card_entry   *card_dirs = NULL;  /* table of card entries */
static struct db_entry     *palm_dirs = NULL;  /* table of palm entries */
static struct db_cathegory *palm_cath = NULL;  /* table of palm cathegories */

/* used for systems sort function for directory entries */
static Int 
card_compare_directory(VoidPtr object1, VoidPtr object2, Long sort) {
  struct card_entry *d1=object1, *d2=object2;
  int ret;

  /* both same type (DIR or FILE?) */
  if((d1->type == LTYPE_SUBDIR)&&(d2->type == LTYPE_SUBDIR)) {
    /* sort directories always alphabetically */
    ret = StrCaselessCompare(d1->name, d2->name);
  } else if((d1->type != LTYPE_SUBDIR)&&(d2->type != LTYPE_SUBDIR)) {

    switch(sort) {

    case 0: /* sort by name */
      ret = StrCaselessCompare(d1->name, d2->name);
      break;
      
    case 1: /* sort by date/time */
      if     (d2->pdate > d1->pdate) ret =  1;
      else if(d2->pdate < d1->pdate) ret = -1;
      else                           ret =  0;
      break;

    case 2: /* sort by size */
      if     (d2->length > d1->length) ret =  1;
      else if(d2->length < d1->length) ret = -1;
      else                             ret =  0;
      break;
    }
  } else if(d1->type == LTYPE_SUBDIR)
    ret = -1;
  else
    ret =  1;

  return ret;
}

/* used for systems sort function for directory entries */
static Int 
palm_compare_directory(VoidPtr object1, VoidPtr object2, Long sort) {
  struct db_entry *d1=object1, *d2=object2;
  int ret;
  
  if(d1->cathegory != d2->cathegory)
    ret = d1->cathegory - d2->cathegory;
  else {

    switch(sort) {

    case 0: /* sort by name */
      ret = StrCaselessCompare(d1->name, d2->name);
      break;
      
    case 1: /* sort by date/time */
      if     (d2->date > d1->date) ret =  1;
      else if(d2->date < d1->date) ret = -1;
      else                         ret =  0;
      break;
      
    case 2: /* sort by size */
      if     (d2->length > d1->length) ret =  1;
      else if(d2->length < d1->length) ret = -1;
      else                             ret =  0;
      break; 
    }
  }
  return ret;
}

/* used for systems sort function for cathegory entries */
static Int 
palm_compare_cathegory(VoidPtr object1, VoidPtr object2, Long other) {
  struct db_cathegory *d1=object1, *d2=object2;

  if((d1->entries == 1)&&(d2->entries != 1 )) return  1;
  if((d1->entries != 1)&&(d2->entries == 1 )) return -1;

  return StrCaselessCompare(d1->name, d2->name);
}

/* redisplay complete directory */
void display_dir_entries(void) {
  RectangleType rect;
  int n,i;
  FormPtr frm;
  ScrollBarPtr bar;

#ifndef SELECT_ICON
  ListPtr lst;
#endif

  /* update scrollbar */
  i = current_entries - SHOW_ENTRIES;
  if(i<0) i=0;

  /* get scrollbar object address */
  frm = FrmGetActiveForm();
  bar = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, scl_Main));

  /* update scrollbar */
  SclSetScrollBar(bar, current_disp_offset, 0, i, SHOW_ENTRIES-1);
  
  /* redraw select buttons */
  CtlSetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, DispPalm)), 
	      disp_mode == LIST_PALM);
  CtlSetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, DispCard)), 
	      disp_mode == LIST_CARD);

#ifdef SELECT_ICON
  /* redraw open mode buttons */
  CtlSetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, SelInfo)),   
	      click_mode == SEL_INFO);
  CtlSetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, SelCopy)),   
	      click_mode == SEL_COPY);
  CtlSetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, SelMove)),   
	      click_mode == SEL_MOVE);
  CtlSetValue(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, SelDelete)), 
	      click_mode == SEL_DEL);
#else
  lst = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, SelList));

  LstSetSelection(lst, click_mode-1);
  CtlSetLabel(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, SelPopup)), 
	      LstGetSelectionText(lst, click_mode-1));
#endif

  rect.topLeft.x=0; rect.topLeft.y=15; 
  rect.extent.x=LAST_POS; rect.extent.y=SHOW_ENTRIES*11;
  WinEraseRectangle(&rect,0);

  /* display at most SHOW_ENTRIES entries */
  if(current_entries>SHOW_ENTRIES) n=SHOW_ENTRIES;
  else                             n=current_entries;

  if(current_disp_offset>(current_entries - n)) 
    current_disp_offset = current_entries - n;
    
  if(disp_mode == LIST_CARD)
    for(i=0;i<n;i++)
      card_print_dir_entry(&card_dirs[i+current_disp_offset],i);
  else {
    if(current_cath == -1) {
      for(i=0;i<n;i++) {

	/* display single app file in cathegory view (not with predefined cathegories) */
	if((palm_cath[i+current_disp_offset].entries == 1)&&(i+current_disp_offset>=5))
	  palm_print_dir_entry(&palm_dirs[palm_cath[i+current_disp_offset].first_entry],i);
	else
	  /* display multiple app/datbases as directory */
	  palm_print_dir_cathegory(&palm_cath[i+current_disp_offset],i);
      }
    } else {
      for(i=0;i<n;i++) {
	if((i==0)&&(current_disp_offset==0))
	  palm_print_dir_entry(NULL, i);
	else
	  palm_print_dir_entry(&palm_dirs[i-1+current_disp_offset+ 
			  palm_cath[current_cath].first_entry],i);
      }
    }
  }
}

/* set scroll position */
void list_scroll(Boolean abs, int pos) {
  if(abs) current_disp_offset  = pos;
  else    current_disp_offset += pos*(SHOW_ENTRIES-1);

  if(current_disp_offset>(current_entries - SHOW_ENTRIES)) 
    current_disp_offset = current_entries - SHOW_ENTRIES;

  if(current_disp_offset<0) current_disp_offset=0;

  display_dir_entries();
}

void list_MemPtrFree(void **data) {
  if(*data != NULL) {
    MemPtrFree(*data);
    *data = NULL;
  }
}

/* free all memory */
static void 
list_free(void) {
  list_MemPtrFree((void**)&card_dirs);
  list_MemPtrFree((void**)&palm_dirs);
  list_MemPtrFree((void**)&palm_cath);
}

void length2ascii(char *p, long k) {
  char *d;
  int j,l;

  d = p;

  if(k!=0) { 
    if(k>999999)   { k = (10*k)/1048576; p[5]='m'; }
    else if(k>999) { k = (10*k)/1024;    p[5]='k'; }
    else           { k =  10*k;          p[5]=0; }
	  
    l=1000; while(l>k) l/=10;
    if(l==1) { *p++='0'; *p++='.'; }
	  
    for(j=0;((j<3)&&(l>0)&&((l!=1)||(k!=0)));j++) {
      *p++ = '0'+k/l;
      k  = k%l;
      l /= 10;
      
      if((l==1)&&(k!=0)&&(j!=2)) *p++='.';
    }
  } else {
    d[5]=0;
    *p++ = '0';
  }

  /* add m/k qualifier */
  *p++ = d[5];
  *p++ = 0;
}

/* replacement for missing strrchr in PalmOS */
char *my_strrchr(char *str, char chr) {
  char* ret=NULL;

  /* scan whole string */
  while(*str) {
    if(*str==chr) ret=str;
    str++;
  }
  return ret;
}

/* list current directory (may take some time, therefore progress bar) */
static void 
palm_list_dir(void) {
  int i, j, k, l, total, cath;
  ScrollBarPtr bar;
  FormPtr frm;
  UInt attrib;
  int lp;

  /* free current list */
  list_free();

  /* check if cache database exists */
#ifdef USE_DATABASE_CACHE

  if(list_cache_retrieve((void**)&palm_cath, &cath, (void**)&palm_dirs, &total)) {
    total_entries = total / sizeof(struct db_entry);
    cathegories   = cath  / sizeof(struct db_cathegory);    

    /* check if cathegory is still valid */
    if(current_cath != -1) {
      /* generate crc */
      for(j=0,i=0;palm_cath[current_cath].name[i] != 0;i++) 
	j ^= palm_cath[current_cath].name[i];
      
      /* display all entries + '..' */
      current_entries = palm_cath[current_cath].entries+1;
      
      if(j != current_cath_crc) {
	current_cath = -1;
	current_entries = cathegories;
	current_disp_offset = 0;
      }
    } else {
      /* start with cathegory overview */
      current_entries = cathegories;
      current_disp_offset = 0;
    }
  } else
#endif

  {
    total = DmNumDatabases(CARD);

    /* allocate directory buffer */
    palm_dirs = MemPtrNew(total * sizeof(struct db_entry));
    if(palm_dirs == NULL) {
//      DEBUG_MSG("unable to allocate buffer for %d entries\n", total);
      list_error(StrLstOOMDir);
      total_entries = 0;
      return;
    }
    
    db_progress_bar(true, "reading databases ...", 0, &lp, 0);
    
    /* scan names and length of all databases */
    for(cath=5,total_entries=0,i=0;i<total;i++) {
      
      palm_dirs[total_entries].dbId = DmGetDatabase(CARD, i);
      
      /* get size of database */
      DmDatabaseSize(CARD, palm_dirs[total_entries].dbId, 
		     &palm_dirs[total_entries].recs, 
		     &palm_dirs[total_entries].length, 
		     NULL);
      
      /* get name of database */
      DmDatabaseInfo(CARD, palm_dirs[total_entries].dbId,
		     palm_dirs[total_entries].name,
		     &attrib, 
		     NULL, NULL, 
		     &palm_dirs[total_entries].date, 
		     NULL, NULL, NULL, NULL, 
		     &palm_dirs[total_entries].type, 
		     &palm_dirs[total_entries].creator);

      palm_dirs[total_entries].is_rsrc = (attrib & dmHdrAttrResDB);
      
      /* mark cathegory as unknown */
      palm_dirs[total_entries].cathegory = CATH_UNKNOWN;
      
      /* don't show read/only stuff */
      if((prefs.flags & PREF_SHOWROM)||(!(attrib & dmHdrAttrReadOnly))) {
	
	/* only applications make new cathegories */
	if((palm_dirs[total_entries].type == 'appl')&&
	   (!list_find_creator(fixed_creator_system, 
			       palm_dirs[total_entries].creator))&&
	   (!list_find_creator(fixed_creator_builtin, 
			       palm_dirs[total_entries].creator))) {
	  
	  cath++;
	}
	
	length2ascii(palm_dirs[total_entries].len_str, 
		     palm_dirs[total_entries].length);
	
	total_entries++;
      }
      
      db_progress_bar(false, NULL, (100l*i)/total, &lp, 0);
    }
    
    /* create palm database cathegories */
    /* number of cathegories = apps + 5 (system, builtin, hacks, docs, unknown) */
    palm_cath = MemPtrNew(sizeof(struct db_cathegory) * cath); 
    
    /* error allocating buffer memory */
    if(palm_cath == NULL) {
      list_error(StrLstOOMDir);
      list_MemPtrFree((void**)&palm_cath);
      current_entries = 0;
      return;
    }
    
    /* create all cathegories */
    for(j=0,i=0;i<cath;i++) {
      palm_cath[i].entries = 0;
      palm_cath[i].first_entry = -1;
      
      /* first 5 cathegories are fixed */
      if(i<CATH_UNKNOWN) {
	/* setup name of fixed cathegory */
	StrCopy(palm_cath[i].name, fixed_cathegory_name[i]);
	
	/* search all databases if they match this cathegory */
	for(k=0;k<total_entries;k++) {
	  
	  /* only search unassigned databases */
	  if(palm_dirs[k].cathegory == CATH_UNKNOWN) {
	    
	    /* cathegory system, builtin, hacks & TEXt */
	    if((i<2)?list_find_creator((i==0)?
			      fixed_creator_system:fixed_creator_builtin, 
			      palm_dirs[k].creator):
	                      (palm_dirs[k].type == ((i==2)?'HACK':'TEXt'))||
	                      (palm_dirs[k].type == ((i==2)?'DAcc':'TEXt'))) {
	      
	      palm_dirs[k].cathegory = i;
	      palm_cath[i].entries++;
	    }
	  }
	}
	
	/* just skip the unknown cathegory (neither fixed cathegory nor matching app) */
      } else if(i == CATH_UNKNOWN) {
	/* setup name of fixed cathegory */
	StrCopy(palm_cath[CATH_UNKNOWN].name, fixed_cathegory_name[CATH_UNKNOWN]);
	
	/* the app cathegories */
      } else if(i>CATH_UNKNOWN) {
	/* scan all apps */
	
	/* search all databases if they match this cathegory */
	palm_cath[i].creator = 0;
	
	/* first search for an unassigned application */
	for(k=0;(palm_cath[i].creator == 0)&&(k<total_entries);k++) {
	  
	  /* a unassigned application */
	  if((palm_dirs[k].cathegory == CATH_UNKNOWN)&&
	     (palm_dirs[k].type == 'appl')) {
	    
	    /* ok, set up new cathegory */
	    StrCopy(palm_cath[i].name, palm_dirs[k].name);
	    palm_cath[i].creator = palm_dirs[k].creator;  
	  }
	}
	
	if(palm_cath[i].creator != 0) {
	  /* now search for files of the same cathegory */
	  for(k=0;k<total_entries;k++) {
	    /* unassigned? */
	    if(palm_dirs[k].cathegory == CATH_UNKNOWN) {
	      if(palm_dirs[k].creator == palm_cath[i].creator) {
		palm_dirs[k].cathegory = i;
		palm_cath[i].entries++;
	      }
	    }
	  }
	}
      }
    }
    
    /* search for unused cathegories */
    for(k=5,i=5;i<cath;i++)
      if(palm_cath[i].entries != 0)
	k++;
    
    cathegories = cath = k;
    
    /* search for unknown databases */
    for(k=0;k<total_entries;k++)
      if(palm_dirs[k].cathegory == CATH_UNKNOWN)
	palm_cath[CATH_UNKNOWN].entries++;
    
    /* ok, now sort the databases */
    SysQSort( (Byte*)palm_dirs, total_entries, sizeof(struct db_entry), 
	      palm_compare_directory, (prefs.flags & PREF_SORT)>>4);
    
    /* and finally assign first database to each cathegory */
    for(i=0;i<cath;i++) {
      for(k=0;(k<total_entries)&&(palm_cath[i].first_entry < 0);k++) {
	if(palm_dirs[k].cathegory == i)
	  palm_cath[i].first_entry = k;
      }
    }
    
    /* and sort the cathegories */
    if(cath > 5) SysQSort( (Byte*)&palm_cath[5], cath-5, 
			   sizeof(struct db_cathegory), palm_compare_cathegory, 0);
    
    /* check if cathegory is still valid */
    if(current_cath != -1) {
      /* generate crc */
      for(j=0,i=0;palm_cath[current_cath].name[i] != 0;i++) 
	j ^= palm_cath[current_cath].name[i];
      
      /* display all entries + '..' */
      current_entries = palm_cath[current_cath].entries+1;
      
      if(j != current_cath_crc) {
	current_cath = -1;
	current_entries = cathegories;
	current_disp_offset = 0;
      }
    } else {
      /* start with cathegory overview */
      current_entries = cathegories;
      current_disp_offset = 0;
    }

#ifdef USE_DATABASE_CACHE
    /* write palm database info to cache */
    list_cache_create(palm_cath, cathegories   * sizeof(struct db_cathegory),
		      palm_dirs, total_entries * sizeof(struct db_entry));
#endif
  }

  display_dir_entries();    
}

/* draw empty list area */
void list_empty(void) {
  ScrollBarPtr bar;
  FormPtr frm;

  /* just display nothing */
  current_disp_offset = 0;
  current_entries = 0;
  
  /* update scrollbar */
  frm = FrmGetActiveForm();
  bar = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, scl_Main));
  SclSetScrollBar(bar, current_disp_offset, 0, 0, 0);
  
  display_dir_entries();
}

/* replacement for missing strrchr in PalmOS */
char *StrRChr(char *str, char chr) {
  char* ret=NULL;

  /* scan whole string */
  while(*str) {
    if(*str==chr) ret=str;
    str++;
  }
  return ret;
}

/* file types that can be installed in the memo pad */
const char *text_types[]={ ".txt", ".c", ".h", ".doc", ".htm", ".html", "" };

/* list current directory */
static void 
card_list_dir(Boolean scan) {
#define PATTERN ""   // empty pattern == all files

  Err err;
  axxPacFileInfo info;
  int i;
  char *ext;

  ScrollBarPtr bar;
  FormPtr frm;
  DateType date;

  /* if buffer allocated free it */
  list_free();
  current_entries = 0;

  if(scan) {
    /* scan number of files */
    err = axxPacFindFirst(LibRef, PATTERN, &info, FIND_FILES_N_ALL_DIRS);
    while(!err) {
      current_entries++;
      err = axxPacFindNext(LibRef, &info);
    }
  }

  /* get scrollbar object address */
  frm = FrmGetActiveForm();
  bar = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, scl_Main));

  /* perhaps an empty directory? */
  if(current_entries!=0) {

    /* allocate a buffer chunk */
    if((card_dirs = MemPtrNew(current_entries *
			      sizeof(struct card_entry))) == NULL){
      DEBUG_MSG("unable to allocate buffer for %d entries\n", current_entries);
      list_error(StrLstOOMDir);
      return;
    }

    /* rescan file info */
    current_entries = 0;
    err = axxPacFindFirst(LibRef, PATTERN, &info, FIND_FILES_N_ALL_DIRS);
    while(!err) {
      card_dirs[current_entries].type = 0;

      /* limit name length */
      StrNCopy(card_dirs[current_entries].name, info.name, MAX_NAMELEN);

      if(info.attrib & FA_SUBDIR) {
	card_dirs[current_entries].type = LTYPE_SUBDIR;
	card_dirs[current_entries].len_str[0] = '\0';
	card_dirs[current_entries].length = 0;	
      } else {
	/* valid extension? */
	if((ext = StrRChr(card_dirs[current_entries].name, '.'))!=NULL) {

	  /* scan for plugins */
	  for(i=0;i<PLUGINS;i++) {
	    /* valid plugin entry? */
	    if(plugin[i].extension[0] != 0) {
	      if(StrCaselessCompare(ext+1, plugin[i].extension)==0)
		card_dirs[current_entries].type = LTYPE_PLUG0 + i;
	    }
	  }

	  /* no plugin? try builtin */
	  if(card_dirs[current_entries].type == 0) {
	    /* palm installable db? */
	    if(StrCaselessCompare(ext, ".pdb")==0)
	      card_dirs[current_entries].type = LTYPE_PALMPDB;

	    else if(StrCaselessCompare(ext, ".prc")==0)
	      card_dirs[current_entries].type = LTYPE_PALMPRC;

	    else {
	      if(info.size <= MAX_MEMO_LEN) {

		for(i=0;text_types[i][0]!=0;i++)
		  if(StrCaselessCompare(ext, text_types[i])==0)
		    card_dirs[current_entries].type = LTYPE_TEXT;
	      }
	    }
	  }
	}

	length2ascii(card_dirs[current_entries].len_str, info.size);
	card_dirs[current_entries].length = info.size;	
      }
      
      DateSecondsToDate(info.date, &date);
      DateToAscii(date.month, date.day, date.year + firstYear, 
		  dateformat, card_dirs[current_entries].date_str);
      card_dirs[current_entries].pdate = info.date;

      current_entries++;
      err = axxPacFindNext(LibRef, &info);
    }

    if(err<0) FrmCustomAlert(alt_err, axxPacStrError(LibRef),0,0);

    /* sort directory */
    SysQSort( (Byte*)card_dirs, current_entries, 
	      sizeof(struct card_entry), card_compare_directory, 
	      (prefs.flags & PREF_SORT)>>4);

    i=current_entries-SHOW_ENTRIES;
    if(i<0) i=0;

    /* update scrollbar */
    SclSetScrollBar(bar, current_disp_offset, 0, i, SHOW_ENTRIES-1);

  } else {
    /* remove scrollbar */
    SclSetScrollBar(bar, 0, 0, 0, 0);
  }
  display_dir_entries();
}

void list_dir(void) {
  if(disp_mode == LIST_CARD) card_list_dir(true);
  else                       palm_list_dir();
}

void list_refresh(Boolean scan) {
  /* only refresh card view */
  if(disp_mode == LIST_CARD)
    card_list_dir(scan);
}

/* change listing mode between palm and card */
void list_set_disp_mode(int mode, Boolean redraw) {
  if(disp_mode != mode) current_disp_offset = 0;

  disp_mode = mode;

  if(redraw) list_dir();
}

/* change click mode between copy, move, delete and info */
void list_set_click_mode(int mode) {
  click_mode = mode;
}

/* listing mode (palm or card) */
int list_get_disp_mode(void) {
  return(disp_mode);
}

/* copy, move, delete or info */
int list_get_click_mode(void) {
  return(click_mode);
}

void list_process_db(int index) {
  char name[32];
  long len;

  /* palm db listing */
  MemMove(name, palm_dirs[index].name,32);
  len = palm_dirs[index].length;

  if(click_mode == SEL_INFO) {
    db_info(name);
  }

  if((click_mode == SEL_COPY)||(click_mode == SEL_MOVE)) {
    list_free();

    if(db_write(LibRef, name, len, 0, false, false)) {
      /* write successful -> delete if moved */
      if(click_mode == SEL_MOVE) {
#ifdef USE_DATABASE_CACHE
	list_cache_release();
#endif
	db_delete(name);
      }
    }
    /* free mem on card may have changed */
    redraw_free_mem = true;

    /* update dir view */
    list_dir();
  }
  
  if(click_mode == SEL_DEL) {
#ifdef USE_DATABASE_CACHE
    list_cache_release();
#endif
    db_delete(name);
    
    /* rescan databases */
    list_dir();
  }
}
 
void file_delete(char *filename) {
  Err err;
  axxPacStatType stat;
  axxPacFileInfo info;
  long cnt, total;
  int lp;

  /* check whether file is directory */
  if(axxPacStat(LibRef, filename, &stat)) {
    FrmCustomAlert(alt_err, axxPacStrError(LibRef),0,0);
    goto delete_quit;
  }

  if(stat.attrib & FA_SUBDIR) {
    /* erase a whole dir */

    if(prefs.flags & PREF_DELETE)
      if(FrmCustomAlert(alt_delsub, filename, 0,0)==1)
	return;

    /* change into dir */
    if(axxPacChangeDir(LibRef, filename) < 0) {
      FrmCustomAlert(alt_err, axxPacStrError(LibRef),0,0);
      goto delete_quit;
    }

    /* check whether dir contains other dirs */
    total = 0; err = axxPacFindFirst(LibRef, "*", &info, FIND_FILES_N_ALL_DIRS);
    while(!err) {
      
      /* this is a real entry */
      if(total>1) {
	if(info.attrib & FA_SUBDIR) {
	  FrmCustomAlert(alt_sdfull, filename,0,0);
	  axxPacChangeDir(LibRef, "..");
	  return;
	}
      }

      total++;
      err = axxPacFindNext(LibRef, &info);
    }

    db_progress_bar(true, "Deleting ...", 0, &lp, 0);

    /* delete all files within */
    cnt = 0; err = axxPacFindFirst(LibRef, "*", &info, FIND_FILES_N_ALL_DIRS);
    while(!err) {
      
      if(cnt>1) {
	if(axxPacDelete(LibRef, info.name)) {
	  FrmCustomAlert(alt_err, axxPacStrError(LibRef),0,0);
	  axxPacChangeDir(LibRef, "..");
	  goto delete_quit;
	}

	db_progress_bar(false, NULL, (cnt*100l)/total, &lp, 0);
      }

      cnt++;
      err = axxPacFindNext(LibRef, &info);
    }

    db_progress_bar(false, NULL, 100, &lp, 0);
    axxPacChangeDir(LibRef, "..");
  } else {
    /* confirm deletion of single file */
    if(prefs.flags & PREF_DELETE)
      if(FrmCustomAlert(alt_delete, filename, 0,0)==1)
	return;
  }

  /* erase just a file/directory */
  if(axxPacDelete(LibRef, filename))
    FrmCustomAlert(alt_err, axxPacStrError(LibRef),0,0);

delete_quit:
  /* free mem on card may have changed */
  redraw_free_mem = true;

  /* rescan entry */
//  current_disp_offset = 0;
  list_dir();
}

/* select particular entry */
static void 
select_entry_int(int entry, int on, Boolean card, Boolean on_icon) {
  RectangleType rect;
  static cur_on=-1;
  struct card_entry cdir;
  int i;
  long plugin_name = 'axP0';
  UInt PluginRef;
  axxPacFD fd;

#ifdef DEBUG_SELECT
  DEBUG_MSG("%sselect entry %d\n", on?(on==2)?"":"re":"de", entry);
#endif

  /* set entry size */
  rect.topLeft.x = 0; 
  rect.extent.x  = LAST_POS; 
  rect.extent.y  = 11;    

  /* deinvert selected entry on desel or if new to be selected */
  if((cur_on>=0)&&(cur_on<current_entries)&&
     ((!on)||(cur_on!=entry)||(entry<0))) { 
#ifdef DEBUG_SELECT
    DEBUG_MSG("Off: %d\n", cur_on);
#endif

    rect.topLeft.y=15+11*cur_on;       
    if(!on) SysTaskDelay(10);
    WinInvertRectangle(&rect,0);
  }

  /* select new entry if existing and not already selected */
  if((on)&&(entry<current_entries)&&(entry>=0)&&(cur_on!=entry)) { 
#ifdef DEBUG_SELECT
    DEBUG_MSG("On: %d\n", entry);
#endif

    rect.topLeft.y=15+11*entry;
    WinInvertRectangle(&rect,0);
  }

  /* cur_on may be <0, because release events are reported */
  /* even if they're not inside the display area */
  if((!on)&&(entry<current_entries)&&(cur_on != -1)) {
    if(card) {
      /* memory card listing */

      /* copy current directory entry */
      MemMove(&cdir, &card_dirs[entry+current_disp_offset],
	      sizeof(struct card_entry));
      
      /* clicked on sub directory */
      if(card_dirs[entry+current_disp_offset].type == LTYPE_SUBDIR) {
	/* either icon click or open mode or '.' or '..' directory */
	if((on_icon)||((click_mode != SEL_DEL)&&(click_mode != SEL_INFO))||
	   (card_dirs[entry+current_disp_offset].name[0]=='.')) {
	  
	  SndPlaySystemSound(sndClick);
	  axxPacChangeDir(LibRef, card_dirs[entry+current_disp_offset].name);
	  StrCopy(saved_path, axxPacGetCwd(LibRef));
	  list_dir();
	} else {
	  if(click_mode == SEL_DEL) {
	    /* delete subdir */
	    file_delete(card_dirs[entry+current_disp_offset].name);	  
	    list_dir();
	  } else {
	    /* show file info */
	    file_info(card_dirs[entry+current_disp_offset].name,
		      &card_dirs[entry+current_disp_offset]);
	  }
	}
      } else {
	/* no click on subdir */
	if((on_icon)||(click_mode == SEL_COPY)||(click_mode == SEL_MOVE)) {
	  
	  if((cdir.type >= LTYPE_PLUG0)&&
	     (cdir.type <= LTYPE_PLUG9)) {
	    
	    /* if buffer allocated free it */
	    list_free();
	    
	    /* try to access plugin */
	    
	    /* create name 'axP?' */
	    i = LTYPE_PLUG0 - cdir.type;
	    plugin_name = (plugin_name & 0xffffff00)|('0'+i); 
	    
	    if(!SysLibLoad('libr', plugin_name, &PluginRef)) {
#ifdef DEBUG_PLUGIN
	      DEBUG_MSG("running plugin %d\n", i);
#endif
	      
	      fd = axxPacOpen(LibRef, cdir.name, O_RdOnly);
	      PluginExec(PluginRef, LibRef, fd, cdir.name, cdir.length);
	      axxPacClose(LibRef, fd);
	      SysLibRemove(PluginRef);
	    }
	    
	    /* don't remove file in 'move' mode */
	    
	    /* re-display directory afterwards */
	    list_dir();
	  } else if((cdir.type == LTYPE_PALMPDB)||
		    (cdir.type == LTYPE_PALMPRC)) {
	    
	    /* if buffer allocated free it */
	    list_free();
	    
	    /* read database from smartmedia and install it on the pilot */
	    SndPlaySystemSound(sndClick);
#ifdef USE_DATABASE_CACHE
	    list_cache_release();
#endif
	    db_read(cdir.name, cdir.length, 0, false);
	    
	    /* delete source at move */
	    if(click_mode == SEL_MOVE) {
	      file_delete(cdir.name);
	    }

	    /* re-display directory afterwards */
	    list_dir();
	    
	  } else if(cdir.type == LTYPE_TEXT) {   
	    SndPlaySystemSound(sndClick);
	    memo_import(cdir.name, cdir.length);
	    
	    /* delete source at move */
	    if(click_mode == SEL_MOVE) {
	      file_delete(cdir.name);
	      list_dir();	    
	    }
	  } else
	    list_error(StrNoPrcPdb);
	}
	
	else if(click_mode == SEL_DEL)
	  file_delete(cdir.name);

	else if(click_mode == SEL_INFO) file_info(cdir.name, &cdir);
      }
    } else {
      /* make some sound ... */
      SndPlaySystemSound(sndClick);
      
      if(current_cath < 0) {
	/* only one app? */
	if((palm_cath[entry+current_disp_offset].entries == 1)&&
	   (entry+current_disp_offset>=5))
	  list_process_db(palm_cath[entry+current_disp_offset].first_entry);
	else {
	  
	  /* enter cathegory */
	  current_entries = palm_cath[entry+current_disp_offset].entries + 1;
	  current_cath = entry+current_disp_offset;
	  parent_disp_offset = current_disp_offset;
	  current_disp_offset = 0;
	  
	  /* generate cathegory crc */
	  for(current_cath_crc=0,i=0;i<palm_cath[current_cath].name[i] != 0;i++) 
	    current_cath_crc ^= palm_cath[current_cath].name[i];
	  
	  display_dir_entries();
	}
      } else {
	
	/* cathegory view */
	if(entry+current_disp_offset == 0) {
	  current_disp_offset = parent_disp_offset;
	  current_entries = cathegories;
	  current_cath = -1;
	  
	  display_dir_entries();
	} else { 
	  list_process_db(palm_cath[current_cath].first_entry-
			  1+entry+current_disp_offset);
	}
      }
    }
  }

  if(!on) cur_on=-1;
  else    cur_on=entry;

#ifdef DEBUG_SELECT
  DEBUG_MSG("new cur on = %d\n", cur_on);
#endif
}

void list_select(int x, int y, int sel) {
  /* in valid click area? */
  if((y>15)&&(y<(15+SHOW_ENTRIES*11))&&(x<LAST_POS))
    select_entry_int((y-15)/11, sel, disp_mode==LIST_CARD, x < NAME_POS);
  else
    select_entry_int(-1, sel, disp_mode==LIST_CARD, x < NAME_POS);
}

void list_initialize(void) {
  int i;
  Err err;
  UInt PluginRef;
  long plugin_name = 'axP0';

  SystemPreferencesType sysPrefs;

  /* get system preferences for short date format */
  PrefGetPreferences(&sysPrefs);
  dateformat = sysPrefs.dateFormat;

  /* set up icons */
  for(i=0;i<ICONS;i++) {
    resH[i] = (Handle)DmGetResource( bitmapRsc, icons[i]);
    ErrFatalDisplayIf( !resH[i], "Missing bitmap" );
    resP[i] = MemHandleLock((VoidHand)resH[i]);
  }
  icons_ok = true;

  /* try to scan for plugins */
  for(i=0;i<PLUGINS;i++) {
    plugin[i].extension[0] = 0;   /* unused plugin */

    /* try to access plugin */
    plugin_name = (plugin_name & 0xffffff00)|('0'+i); /* create name 'axP?' */

    if(!SysLibLoad('libr', plugin_name, &PluginRef)) {
      PluginGetInfo(PluginRef, &plugin[i]);
      SysLibRemove(PluginRef);
#ifdef DEBUG_PLUGIN
      DEBUG_MSG("found plugin %d for %s\n", i, plugin[i].extension);
#endif
    }
  }

  /* we start in the root directory */
  StrCopy(saved_path, "/");
}

void list_release(void) {
  int i;

#ifdef USE_DATABASE_CACHE
  /* free cache */
  list_cache_release();
#endif

  /* free allocated structures */
  list_free();

  /* free icons */
  if(icons_ok) {
    for(i=0;i<ICONS;i++) {
      MemPtrUnlock(resP[i]);
      DmReleaseResource((VoidHand) resH[i] );
    }
  }
}

