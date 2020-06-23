#include <PalmOS.h>
#include <System/DLServer.h>

#include "Rsc.h"
#include "IR-Pong2.h"

char opponent_name[dlkUserNameBufSize];

typedef struct {
  char name[dlkUserNameBufSize];
  unsigned long score[2];
} game;

void *GetObjectPtr(FormPtr frm, UInt16 object) {
  return FrmGetObjectPtr(frm, FrmGetObjectIndex (frm, object));
}

game *game_results;
int saved_results;

#define DB_NAME  APPNAME ".score"
#define DB_SCORE 'data'

void list_init(void) {
  DmOpenRef dbRef;
  LocalID dbID, chunk;
  char *data;  
  int i;
  long size;

  /* try to load game_db */
  if((dbID = DmFindDatabase(0, DB_NAME))!=NULL) {

    dbRef = DmOpenDatabase(0, dbID, dmModeReadOnly);

    /* get record info (type, id, memory chunk) */
    DmRecordInfo(dbRef, 0, NULL, NULL, &chunk);
	  
    data = MemLocalIDToLockedPtr(chunk, 0);
    size = MemPtrSize(data);

    saved_results = size/sizeof(game);
    game_results = MemPtrNew(size);

    /* read data */
    MemMove(game_results, data, size);

    MemPtrUnlock(data);

    DmCloseDatabase(dbRef);
  } else {
    saved_results = 0;
    game_results = NULL;
  }
}

/* used for systems sort function */
static Int16
compare_game(void *object1, void *object2, Int32 sort) {
  game *d1=object1, *d2=object2;

  return ((d2->score[0]+d2->score[1])-(d1->score[0]+d1->score[1]));
}

void list_add(Boolean winner) {
  unsigned long sum;
  Boolean found;
  int i;
  game *new;

  /* empty opponent name? */
  if(opponent_name[0]==0)
    StrCopy(opponent_name, "NoName");

  /* search if opponent exists in table */
  for(found=false,i=0;(i<saved_results)&&(!found);i++) {
    if(StrCompare(game_results[i].name, opponent_name)==0) {
      /* just add scores */
      game_results[i].score[winner?1:0]++;
      found = true;
    }
  }

  /* append a new entry */
  if(!found) {
    /* create new result buffer */
    new = MemPtrNew((saved_results+1) * sizeof(game));
    if(new != NULL) {

      /* copy old entries */
      if(game_results != NULL)
	MemMove(new, game_results, saved_results*sizeof(game));

      /* and add a new entry */
      StrCopy(new[saved_results].name, opponent_name);
      new[saved_results].score[winner?1:0] = 1;
      new[saved_results].score[winner?0:1] = 0;
      saved_results += 1;

      /* now use new buffer */
      if(game_results != NULL)
	MemPtrFree(game_results);

      game_results = new;
    }
  }

  /* ok, now sort the entries */
  if(saved_results != 0)
    SysQSort( game_results, saved_results, sizeof(game), compare_game, 0);
}
 
void list_show(void) {
  MemHandle hand;
  char str[64], score[10], *ptr;
  int i, slen;

  if(saved_results == 0) {
    hand = MemHandleNew(100);
    ptr = MemHandleLock(hand);
    StrCopy(ptr, "no games played");
  } else {
    hand = MemHandleNew(100*saved_results+1);
    ptr = MemHandleLock(hand);
    ptr[0]=0;

    FntSetFont(1);

    for(i=0;i<saved_results;i++) {
      StrPrintF(score, "%lu:%lu", 
		game_results[i].score[0],
		game_results[i].score[1]);

      slen = 140 - FntLineWidth(score, StrLen(score));

      StrPrintF(str, "%d. %s", i+1, game_results[i].name);

      while(FntLineWidth(str, StrLen(str))<slen)
	StrCat(str," ");
      
      StrCat(ptr, str);
      StrCat(ptr, score);
      if(i!=saved_results-1) StrCat(ptr, "\n");
    }
  }

  MemHandleUnlock(hand);
  show_text("Score table", 0, hand);
}

void list_save(void) {
  DmOpenRef dbRef;
  LocalID dbID;
  MemHandle rec;
  char *data;  
  UInt16 at;

  /* delete existing database */
  if((dbID = DmFindDatabase(0, DB_NAME))!=0)
    DmDeleteDatabase(0, dbID);

  if(saved_results != 0) {
    DmCreateDatabase(0, DB_NAME, CREATOR, DB_SCORE, false);
    dbID = DmFindDatabase(0, DB_NAME);
    dbRef = DmOpenDatabase(0, dbID, dmModeReadWrite);

    at = 0; rec = DmNewRecord(dbRef, &at, saved_results * sizeof(game));
    data = MemHandleLock(rec);

    /* write data */
    DmWrite(data, 0, game_results, saved_results * sizeof(game));
    
    MemPtrUnlock(data);
    DmReleaseRecord(dbRef, 0, false);
    
    DmCloseDatabase(dbRef);
  }

  if(game_results != NULL)
    MemPtrFree(game_results);
}
