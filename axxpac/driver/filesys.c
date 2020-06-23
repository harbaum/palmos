/* 

   SmartMedia DOS VFAT12 filesystem driver for axxPac

   (c) 2000 by Till Harbaum

*/

/* show detailed card info required to set up format parameters */
#define noALL_INFO

#ifdef OS35HDR
#include <PalmOS.h>
#include <PalmCompatibility.h>
#else
#include <Pilot.h>
#endif

#include "axxPacDrv.h"
#include "../api/axxPac.h"

#include "smartmedia.h"
#include "filesys.h"

#define noDEBUG_FAT

#define CLUSTER2SECTOR(a) ((long)glf->first_dsec+(long)glf->spc*((long)a-2l))

void fs_init_globals(Lib_globals *gl) {
  filesys_globals *glf = &(gl->filesys);

  glf->fs_ok = false;          /* file system isn't valid         */
  glf->file_list = NULL;       /* no open files                   */
  glf->fat_buffer = NULL;      /* no file allocation table loaded */
  glf->path_str[0] = 0;        /* path string is empty            */

#ifdef STORE_FAT_IN_DB
  glf->dbRef = 0;
#endif
}

void fs_release_fat(Lib_globals *gl) {
  filesys_globals *glf = &(gl->filesys);

#ifndef STORE_FAT_IN_DB
  /* release memory allocated for FAT */
  if(glf->fat_buffer != NULL) {
    DEBUG_MSG_MEM("free fat: %lx\n", glf->fat_buffer);
    MemPtrFree(glf->fat_buffer);
  }
#else
  LocalID dbid;

  /* close, unlock and remove database */
  if(glf->dbRef != 0) {
    /* unlock FAT buffer */
    MemPtrUnlock(glf->fat_buffer);
    /* release FAT buffer record */
    DmReleaseRecord(glf->dbRef, 0, false);
    /* close the database */
    DmCloseDatabase(glf->dbRef); 
    glf->dbRef = 0;

    DEBUG_MSG_MEM("free fat: %lx\n", glf->fat_buffer);
  }
    
  /* and finally try to delete an existing database */
  if((dbid = DmFindDatabase(0, FAT_DB_NAME))!=0)
    DmDeleteDatabase(0, dbid);
#endif
}  

void fs_release(Lib_globals *gl) {
  file_state *fstate, *p;
  filesys_globals *glf = &(gl->filesys);

//  DEBUG_MSG("filesystem shutdown\n");

  /* remove open files (lib closed while files still open???) */
  fstate = glf->file_list;
  while(fstate != NULL) {
//    DEBUG_MSG("file still open: fd=%d\n", fstate->fd);

    /* close file */
    /* ... */

    p = fstate;
    DEBUG_MSG_MEM("free fstate: %lx\n", fstate);
    MemPtrFree(fstate);
    fstate = p->next;
  }

  fs_release_fat(gl);
}

/* initialze file system in partitions part of the card */
Err fs_init(Lib_globals *gl) {
  int base = 0x1be;
  int err;
  file_state *fstate, *p;
  filesys_globals *glf = &(gl->filesys);
#ifdef STORE_FAT_IN_DB
  LocalID dbid;
  VoidHand record;
#endif
#ifdef ALL_INFO
  unsigned long nhid;
  unsigned int nsides, spt, media;
  int i; 
#endif

  DEBUG_MSG("init filesystem\n");

  /* close all open files */
  fstate = glf->file_list;
  while(fstate != NULL) {
    p = fstate;
    DEBUG_MSG_MEM("free fstate: %lx\n", fstate);
    MemPtrFree(fstate);
    fstate = p->next;
  }
  glf->file_list = NULL;               /* no open files */  

  fs_release_fat(gl);
  
  glf->fs_ok = false;                  /* file system isn't valid */
  glf->dir_sector.sec_no = -1;         /* no sector in directory buffer */
  glf->current_dir_cluster = ROOTDIR; /* current dir is root directory */
  glf->fat_dirty = false;             /* fat needn't to be written */

  /* read master boot record */
  if(smartmedia_read(gl, glf->dir_sector.buffer, 0, 1, false)) {
    DEBUG_MSG("read error on MBR\n");
    error(gl, false, ErrFsRdMBR);
    return -ErrFsRdMBR;
  }

  /* check validity of mbr */
  if((glf->dir_sector.buffer[0x1fe]!=0x55)||
     (glf->dir_sector.buffer[0x1ff]!=0xaa)) {
    DEBUG_MSG("invalid MBR\n");
    error(gl, false, ErrFsInvMBR);
    return -ErrFsInvMBR;
  }

#ifdef ALL_INFO
  for(i=0;i<16;i++) {
    DEBUG_MSG("buffer %x = %d (%x)\n", 0x1be + i, 
	      (int)glf->dir_sector.buffer[0x1be + i],
	      (int)glf->dir_sector.buffer[0x1be + i]);
  }
#endif

  /* check partition type */
  if((glf->dir_sector.buffer[0x1be + 4]!=1)&&
     (glf->dir_sector.buffer[0x1be + 4]!=6)) {
    DEBUG_MSG("no FAT 12/16\n");
    error(gl, false, ErrFsNoFAT12);
    return -ErrFsNoFAT12;
  }

  /* remember fat encoding */
  glf->is_fat12 = (glf->dir_sector.buffer[0x1be + 4]==1);

 /* preceeding sectors */
  glf->p_start  =       glf->dir_sector.buffer[ 0x1be + 0x08] + 
                  256 * glf->dir_sector.buffer[ 0x1be + 0x09];

  /* ok, now read the partition boot sector */
  if(smartmedia_read(gl, glf->dir_sector.buffer, 
		     glf->p_start, 1, false)) {
    DEBUG_MSG("read error on partition boot sector\n");
    error(gl, false, ErrFsRdPBS);
    return -ErrFsRdPBS;
  }

  /* get disk parameters */
  glf->bps    =       glf->dir_sector.buffer[0x0b] + 
               256u * glf->dir_sector.buffer[0x0c];
  glf->spc    =       glf->dir_sector.buffer[0x0d];
  glf->res    =       glf->dir_sector.buffer[0x0e] + 
               256u * glf->dir_sector.buffer[0x0f];
  glf->nfats  =       glf->dir_sector.buffer[0x10];
  glf->ndirs  =       glf->dir_sector.buffer[0x11] + 
               256u * glf->dir_sector.buffer[0x12];
  glf->nsects =       glf->dir_sector.buffer[0x13] + 
               256u * glf->dir_sector.buffer[0x14];
#ifdef ALL_INFO
  media       =       glf->dir_sector.buffer[0x15];
#endif
  glf->spf    =       glf->dir_sector.buffer[0x16] + 
               256u * glf->dir_sector.buffer[0x17];
#ifdef ALL_INFO
  spt         =       glf->dir_sector.buffer[0x18] + 
               256u * glf->dir_sector.buffer[0x19];
  nsides      =       glf->dir_sector.buffer[0x1a] + 
               256u * glf->dir_sector.buffer[0x1b];
  nhid        =       glf->dir_sector.buffer[0x1c] + 
               256u * glf->dir_sector.buffer[0x1d];
#endif

  /* huge format for 64MBytes media and up */
  if(glf->nsects == 0) {
#ifdef ALL_INFO
    nhid        =             (long)glf->dir_sector.buffer[0x1c] +      
                      256ul * (long)glf->dir_sector.buffer[0x1d] +    
                    65536ul * (long)glf->dir_sector.buffer[0x1e] + 
                 16777216ul * (long)glf->dir_sector.buffer[0x1f];    
#endif
    glf->nsects =             (long)glf->dir_sector.buffer[0x20] +      
                      256ul * (long)glf->dir_sector.buffer[0x21] +
                    65536ul * (long)glf->dir_sector.buffer[0x22] + 
                 16777216ul * (long)glf->dir_sector.buffer[0x23];
  }

#ifdef ALL_INFO
  DEBUG_MSG("nsects: %ld\n", glf->nsects);
  DEBUG_MSG("spt: %d\n", spt);
  DEBUG_MSG("heads: %d\n", nsides);
  DEBUG_MSG("nhid: %ld\n", nhid);

  DEBUG_MSG("pstart: %ld\n", (long)glf->p_start);
  DEBUG_MSG("res: %ld\n", (long)glf->res);
  DEBUG_MSG("nfats: %ld\n", (long)glf->nfats);
  DEBUG_MSG("spf: %ld\n", (long)glf->spf);
  DEBUG_MSG("ndirs: %ld\n", (long)glf->ndirs);
#endif

  /* calculate position of first data sector */
  glf->first_dsec = (long)glf->p_start + (long)glf->res + 
     (long)glf->spf * (long)glf->nfats + (long)glf->ndirs/16;

  /* total number of clusters */
  glf->total_cl = (glf->nsects - (glf->ndirs/16) -
		   (glf->spf * glf->nfats)) /glf->spc;

  /* some checks ... */
  if(glf->bps != 512) {
    error(gl, false, ErrFsSecSiz);
    return -ErrFsSecSiz;
  }

#ifndef STORE_FAT_IN_DB  
  /* (re)load FAT */
  if((glf->fat_buffer = 
      (char*)MemPtrNew(512 * glf->spf))==NULL) {
    error(gl, false, ErrFsFATOOM);
    return -ErrFsFATOOM;
  }
  DEBUG_MSG_MEM("alloc fat memory %ld = %lx\n", 512l * glf->spf, glf->fat_buffer);

  /* finally load fat */
  if(smartmedia_read(gl, glf->fat_buffer, 
		     glf->p_start+glf->res, glf->spf, false)) {
    error(gl, false, ErrFsRdFAT);
    return -ErrFsRdFAT;
  }

#else
  /* create a database */
  DmCreateDatabase(0, FAT_DB_NAME, CREATOR, FAT_DB_TYPE, false);
 
  if((dbid = DmFindDatabase(0, FAT_DB_NAME))!=NULL) {
    UInt16 at=0;

    glf->dbRef = DmOpenDatabase(0, dbid, dmModeReadWrite);

    /* create a new record */
    if((record = DmNewRecord(glf->dbRef, &at, 512 * glf->spf))==0) {
      /* close database */
      DmCloseDatabase(glf->dbRef);
      glf->dbRef = 0;

      error(gl, false, ErrFsFATOOM);
      return -ErrFsFATOOM;
    }

    glf->fat_buffer = MemHandleLock(record);  
  }

  DEBUG_MSG_MEM("alloc fat database %ld = %lx\n", 512l * glf->spf, glf->fat_buffer);

  /* finally load fat */
  {
    int j;

    for(j=0; j < glf->spf; j++) {
      /* read single FAT sector */
      if(smartmedia_read(gl, glf->dir_sector.buffer, 
			 glf->p_start+glf->res+j, 1, false)) {

	error(gl, false, ErrFsRdFAT);
	return -ErrFsRdFAT;
      }

      /* and write it into the database */
      DmWrite(glf->fat_buffer, j*512, glf->dir_sector.buffer, 512);
    }
  }
#endif

  /* reset to root dir */
  StrCopy(glf->path_str, DELIMITER_STR);

  glf->fs_ok = true;

  return -ErrNone;
}

/* replacement for missing strrchr in PalmOS */
char *fs_strrchr(char *str, char chr) {
  char* ret=NULL;

  /* scan whole string */
  while(*str) {
    if(*str==chr) ret=str;
    str++;
  }
  return ret;
}

Err
fs_check(Lib_globals *gl) {
  filesys_globals *glf = &(gl->filesys);

  if(!glf->fs_ok) {
    /* read of status forces readback */
    smartmedia_card_status(gl);

    if(!glf->fs_ok) {
      DEBUG_MSG("fs_check failed!\n");
      
      error(gl, false, ErrFsNoSys);
      return -ErrFsNoSys;  
    }
  }
  return ErrNone;
}

/* find cluster following the current one */
unsigned short
fs_get_next_fat_entry(Lib_globals *gl, int no) {
  filesys_globals *glf = &(gl->filesys);

  //  DEBUG_MSG("get next fat entry: %d\n", no);

  if(glf->is_fat12) {
    unsigned char fat_0 = glf->fat_buffer[no * 3 / 2];
    unsigned char fat_1 = glf->fat_buffer[no * 3 / 2 +1];

    if(no&1) return ((fat_1 & 0xff) << 4) | ((fat_0 & 0xf0) >> 4);
    else     return ((fat_1 & 0x0f) << 8) | (fat_0 & 0xff);  
  } else {
    unsigned short *entry = (unsigned short*) &(glf->fat_buffer[2*no]);
    return DOS2USHORT(*entry);
  }
}

/* find cluster preceeding the current one */
unsigned short
fs_get_prev_fat_entry(Lib_globals *gl, int no) {
  unsigned int i;

  for(i=0;i<gl->filesys.total_cl;i++)
    if(fs_get_next_fat_entry(gl, i+2) == no)
      return i+2;

  return 0;
}

void 
fs_set_fat_entry(Lib_globals *gl, int no, unsigned short code) {
  filesys_globals *glf = &(gl->filesys);
  glf->fat_dirty = true;

  if(glf->is_fat12) {
    unsigned char *fat_0 = &glf->fat_buffer[no * 3 / 2];
    unsigned char *fat_1 = &glf->fat_buffer[no * 3 / 2 +1];

#ifndef STORE_FAT_IN_DB
    if (no & 1) {
      /* (odd) not on byte boundary */
      *fat_0 = (*fat_0 & 0x0f) | ((code << 4) & 0xf0);
      *fat_1 = (code >> 4) & 0xff;
    } else {
      /* (even) on byte boundary */
      *fat_0 = code & 0xff;
      *fat_1 = (*fat_1 & 0xf0) | ((code >> 8) & 0x0f);
    }
#else
    unsigned char dat[2];
  
    if (no & 1) {
      /* (odd) not on byte boundary */
      dat[0] = (*fat_0 & 0x0f) | ((code << 4) & 0xf0);
      dat[1] = (code >> 4) & 0xff;
    } else {
    /* (even) on byte boundary */
      dat[0] = code & 0xff;
      dat[1] = (*fat_1 & 0xf0) | ((code >> 8) & 0x0f);
    }
    
    /* write changed entry to database */
    DmWrite(glf->fat_buffer, no*3/2, &dat, 2);
#endif
  } else {
#ifndef STORE_FAT_IN_DB
    unsigned short *fat = (unsigned short*) &glf->fat_buffer[2*no];
    *fat = DOS2USHORT(code);    
#else
    code = DOS2USHORT(code);
    DmWrite(glf->fat_buffer, 2*no, &code, 2);
#endif
  }
}

unsigned short 
fs_get_free_cluster(Lib_globals *gl) {
  filesys_globals *glf = &(gl->filesys);
  unsigned int i,k;

  for(i=0;i<glf->total_cl;i++) {
    if(fs_get_next_fat_entry(gl, i+2) == 0) {
      /* erase new cluster (works without erasing, */
      /* but this speeds things up)                */
      smartmedia_free_lblock(gl, CLUSTER2SECTOR(i+2));
      return i+2;
    }
  }

  return 0;
}

Err 
fs_get_diskspace(Lib_globals *gl, DWord *free) {
  filesys_globals *glf = &(gl->filesys);
  unsigned int i, k, empty=0, defective=0, used=0, other=0;
  Boolean close_me=false;
  Err err;

  if(err = fs_check(gl)) return err;

  if(smartmedia_open(gl, true)) {
    error(gl, false, ErrFsNOpen);
    return -ErrFsNOpen;
  }

  /* check FAT type */
#ifdef DEBUG
  if(glf->is_fat12) {
    DEBUG_MSG("scan FAT 12\n");
  } else {
    DEBUG_MSG("scan FAT 16\n");
  }
#endif

  for(i=0;i<glf->total_cl;i++) {
    k = fs_get_next_fat_entry(gl, i+2);

    if(glf->is_fat12) {
      /* scan fat 12 */
      if(k==0) empty++;
      else if(((k>=0x002)&&(k<=0x0fef))||((k>=0x0ff8)&&(k<=0x0fff))) used++;
      else if((k>=0x0ff0)&&(k<0x0ff7)) defective++;
      else other++;
    } else {
      /* scan fat 16 */
      if(k==0) empty++;
      else if(((k>=0x0002)&&(k<=0x7fff))||((k>=0xfff8)&&(k<=0xffff))) used++;
      else if((k>=0xfff0)&&(k<0xfff7)) defective++;
      else other++;
    }
  }

  DEBUG_MSG("FAT: %u used, %u def\n     %u empty, %u other\n", 
	    used, defective, empty, other);

  if(other!=0) {
    smartmedia_close(gl);
    error(gl, false, ErrFsDefFAT);
    return -ErrFsDefFAT;
  }

  smartmedia_close(gl);

  *free = (DWord)empty*(DWord)(glf->spc/2);
  return -ErrNone;
}

int 
fs_unicode_read(struct unicode_char *in, char *out, int num) {
  int j;

  for (j=0; j<num; ++j) {
    if (in->uchar)
      *out = '_';
    else
      *out = in->lchar;
    ++out;
    ++in;
  }
  return num;
}

int 
fs_unicode_write(char *in, struct unicode_char *out, int num, int *end_p) {
  int j;

  for (j=0; j<num; ++j) {
    out->uchar = '\0';	/* Hard coded to ASCII */
    if (*end_p)
      /* Fill with 0xff */
      out->uchar = out->lchar = (char) 0xff;
    else {
      out->lchar = *in;
      if (! *in) {
	*end_p = VSE_LAST;
      }
    }
    
    ++out;
    ++in;
  }
  return num;
}

/* sum short name checksum */
unsigned char 
fs_sum_shortname(char *name) {
  unsigned char sum;
  int j;

  for (j=sum=0; j<11; ++j)
    sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1)
      + (name[j] ? name[j] : ' ');
  return(sum);
}

const static int day_n[] = { 
  0,31,59,90,120,151,181,212,243,273,304,334,0,0,0,0 };
/*  JanFebMarApr May Jun Jul Aug Sep Oct Nov Dec */

/* Convert a MS-DOS time/date pair to a PalmOS date (seconds since 1 1 1904). */
unsigned long 
fs_date_dos2palm(unsigned short time, unsigned short date) {
  unsigned long month,year,secs;

  month = ((date >> 5)-1) & 15;

  year = date >> 9;
  secs = 2082844800 + (time & 31)*2l+60l*((time >> 5) & 63)+(time >> 11)*3600l+86400l*
    ((date & 31)-1+day_n[month]+(year/4)+year*365-((year & 3) == 0 &&
						   month < 2 ? 1 : 0)+3653);
  /* days since 1.1.1904 plus 80's leap day */
  return secs;
}

/* Convert linear PalmOS date to a MS-DOS time/date pair. */
void 
fs_date_palm2dos(unsigned long palm_date,unsigned short *time, unsigned short *date) {
  int day,year,nl_day,month;

  palm_date -= 2082844800;

  *time = (palm_date % 60)/2+(((palm_date/60) % 60) << 5)+
    (((palm_date/3600) % 24) << 11);
  day = palm_date/86400-3652;
  year = day/365;
  if ((year+3)/4+365*year > day) year--;
  day -= (year+3)/4+365*year;
  if (day == 59 && !(year & 3)) {
    nl_day = day;
    month = 2;
  }
  else {
    nl_day = (year & 3) || day <= 59 ? day : day-1;
    for (month = 0; month < 12; month++)
      if (day_n[month] > nl_day) break;
  }
  *date = nl_day-day_n[month-1]+1+(month << 5)+(year << 9);
}

/* load a sector into buffer if needed (sec_no == -1 -> flush) */
Err 
fs_buffer_sec(Lib_globals *gl, sector_buffer *sector, 
	      long sec_no, Boolean fast) {
  filesys_globals *glf = &(gl->filesys);

  DEBUG_MSG("fs_buffer_sec: %ld\n", sec_no);

  if(sec_no != sector->sec_no) {
    if(smartmedia_open(gl, true)) {
      error(gl, false, ErrFsNOpen);
      return -ErrFsNOpen;
    }

    /* save dirty sector? */
    if((sector->sec_no != FLUSH_BUFFER)&&(sector->dirty)&&
       (sector->sec_no != sec_no)) {

      DEBUG_MSG("writing dirty sector %ld\n", sector->sec_no);

      /* write sector */
      if(smartmedia_write(gl, sector->buffer, sector->sec_no, 1)) {
	DEBUG_MSG("buffer write error at %ld\n", sector->sec_no);
	smartmedia_close(gl);
	error(gl, false, ErrFsWrSec);
	return -ErrFsWrSec;
      }
      sector->dirty = false;
    }

    /* load new sector? */
    if((sec_no != FLUSH_BUFFER)&&(sec_no != sector->sec_no)) {
      
      if(smartmedia_read(gl, sector->buffer, sec_no, 1, fast)) {
	if(smartmedia_read(gl, sector->buffer, sec_no, 1, fast)) {
	  // DEBUG_MSG("buffer reload error at %ld\n", sec_no);
	  smartmedia_close(gl);
	  sector->sec_no = -1;       /* no valid sector in buffer */
	  error(gl, false, ErrFsRdSec);
	  return -ErrFsRdSec;
	} 
#ifdef DEBUG
	else
	  DEBUG_MSG("successful retry %ld\n", sec_no);
#endif
      }
      sector->dirty = false;
      sector->sec_no = sec_no;
    }
    smartmedia_close(gl);
  }
  return -ErrNone;
}

Boolean is_empty(filesys_globals *glf, int cluster) {
  if(glf->is_fat12) return((cluster&0x0ff8)==0x0ff8);

  return((cluster&0xfff8)==0xfff8);
}

/* increment offset during directory search */
Err 
inc_offset(Lib_globals *gl, find_state *state, Boolean extend) {
  filesys_globals *glf = &(gl->filesys);
  Err err;
  unsigned int new_cluster, next_cluster;
  unsigned long new_sec;
  int sector;

  /* read next sector if neccessary */
  if(++state->offset == 16) {
    state->offset = 0;

    DEBUG_MSG("load next sector %ld\n", state->sec+1);
    /* still sectors in this cluster to read? */
    if(--state->cnt > 0) {
      DEBUG_MSG("sector is in same cluster\n");
      state->sec++;
      if(err = fs_buffer_sec(gl, &(glf->dir_sector), state->sec, false))
	return err;
    } else {
      /* no more clusters for ROOTDIR */
      if(state->cluster == ROOTDIR) {
	/* always return error, since root dir */
	/* cannot be extended */
	DEBUG_MSG("root dir sector not found\n");

	if(!extend) {
	  error(gl, false, ErrFsNotFound);
	  return -ErrFsNotFound;
	} else {
	  error(gl, false, ErrFsRtFull);
	  return -ErrFsRtFull;
	}
      } 

      /* get number of next cluster */
      next_cluster = fs_get_next_fat_entry(gl, state->cluster);

      /* last sector in chain? */
      if( is_empty(glf, next_cluster)){

	/* try to extend dir */
	if(extend) {
	  DEBUG_MSG("try to extend dir\n");
	  
	  if((new_cluster = fs_get_free_cluster(gl))==NULL) {
	    DEBUG_MSG_NEWDIR("extend dir: disk is full\n");
	    error(gl, false, ErrFsDskFull);
	    return -ErrFsDskFull;
	  }

	  /* mark old cluster as being follwed by new one */
	  fs_set_fat_entry(gl, state->cluster, new_cluster);

	  /* mark new cluster as last cluster */
	  fs_set_fat_entry(gl, new_cluster, 0xfff8);

	  new_sec = CLUSTER2SECTOR(new_cluster);

	  /* init all sectors of this cluster */
	  for(sector=0; sector<glf->spc; sector++, new_sec++) {
	    /* flush buffer */
	    if(err = fs_buffer_sec(gl, &glf->dir_sector, FLUSH_BUFFER, false))
	      return err;

	    /* setup new buffer */
	    MemSet(glf->dir_sector.buffer, 512, 0);
	    glf->dir_sector.sec_no = new_sec;
	    glf->dir_sector.dirty = true;
	  }
	  DEBUG_MSG("dir successfully extended\n");

	  state->cluster = new_cluster;
	  goto inc_load;
	} else {
	  DEBUG_MSG("reached end of subdir\n");
	  error(gl, false, ErrFsNotFound);
	  return -ErrFsNotFound;
	}
      } 
      
      state->cluster = next_cluster;

inc_load:
      /* load first sector of next cluster */
      state->sec = CLUSTER2SECTOR(state->cluster);
      state->cnt = glf->spc;

      if(err = fs_buffer_sec(gl, &(glf->dir_sector), state->sec, false)) 
	return err;
    }
  }
  return -ErrNone;
}

#define EXIST_QUANTOR '?'
#define ALL_QUANTOR   '*'

/* match pattern with filename */
Boolean 
fs_match(char *pattern, char *filename) {
  int ii, star;

  DEBUG_MSG_MATCH("compare\n   %s\n   %s\n", pattern, filename);

new_segment:
  star = 0;

  while (pattern[0] == ALL_QUANTOR) {
    star = 1;
    pattern++;
  }
  
test_match:
  for (ii = 0; (pattern[ii]!=DELIMITER) && 
	 pattern[ii] && (pattern[ii] != ALL_QUANTOR); ii++) {
    if (pattern[ii] != filename[ii]) {
      if (!filename[ii]) return 0;
      if (pattern[ii] == EXIST_QUANTOR) continue;
      if (!star) return 0;
      filename++;
      goto test_match;
    }
  }
  
  if (pattern[ii] == ALL_QUANTOR) {
    filename += ii;
    pattern += ii;
    goto new_segment;
  }

  if (!filename[ii]) return true;
  if (ii && pattern[ii-1] == ALL_QUANTOR) return true;
  if (!star) return false;
  filename++;
  goto test_match;
}

/* modes 0 and 1 are defined in axxPac.h */
#define FIND_SHORT        3  /* find short (8+3) file name */
#define FIND_REMOVE_ENTRY 4 
#define FIND_SPACE        5 
#define FIND_NEW_ENTRY    6 

Err 
fs_findnext(Lib_globals *gl, char *pattern, 
	    find_state *state, axxPacFileInfo *info,
	    unsigned char sum) {
  filesys_globals *glf = &(gl->filesys);
  Err err;
  Boolean quit=false, delete=false;
  char *c;
  int  k, vsum = -1;
  struct dos_dir_entry *dir;
  struct vfat_dir_entry *vdir;
  int mode = state->mode & 0xff, cnt=0;
  int req = state->mode >> 8, end;
  char *p;

  /* make sure that sector is in buffer */
  if(err = fs_buffer_sec(gl, &(glf->dir_sector), state->sec, false)) 
    return err;

  do {
    dir = (struct dos_dir_entry*)&glf->dir_sector.buffer[32*state->offset];

    /* found last entry? */
    if(dir->name[0]==0x00) {
      if((mode != FIND_SPACE)&&(mode != FIND_NEW_ENTRY)) {
	error(gl, false, ErrFsNotFound);
	return -ErrFsNotFound; 
      }
      cnt++;
    } else {
      /* ok, valid entry */
      if(dir->name[0] != 0xe5) {
	cnt = 0;  /* reset space counter */
	if((mode != FIND_SPACE)&&(mode != FIND_NEW_ENTRY)) {
	  if(mode == FIND_REMOVE_ENTRY) {
	    if((state->ns_offset == state->offset) &&
	       (state->ns_sec    == state->sec)) {
	      delete = true;
	    }
	  }
	  
	  /* vfat entry? */
	  if(dir->attr==0x0f) {
	    vdir = (struct vfat_dir_entry*)
	      &glf->dir_sector.buffer[32*state->offset];
	    vsum = vdir->sum;
	    
	    /* save offset of first entry */
	    if(mode != FIND_REMOVE_ENTRY) {
	      
	      if(vdir->id & VSE_LAST) {
		state->ns_offset = state->offset;
		state->ns_sec    = state->sec;
	      }
	    }
	    
	    c = &(info->name[VSE_NAMELEN * ((vdir->id & VSE_MASK)-1)]);
	    c += fs_unicode_read(vdir->text1, c, VSE1SIZE);
	    c += fs_unicode_read(vdir->text2, c, VSE2SIZE);
	    c += fs_unicode_read(vdir->text3, c, VSE3SIZE);
	    
	    /* terminate string */
	    if(vdir->id & VSE_LAST) {
	      info->name[VSE_NAMELEN * (vdir->id & VSE_MASK)]=0;
	    }
	    
	    if(delete) {
	      dir->name[0] = 0xe5; 
	      glf->dir_sector.dirty = true;  /* this sector is now dirty */
	    }
	  } else {
	  
	    /* valid long name checksum? */
	    if((vsum == fs_sum_shortname(dir->name))&&(mode != FIND_SHORT)) {
	      /* remove trailing spaces */
	      c = &(info->name[StrLen(info->name)-1]);
	      while(*c==' ') *c--=0;
	    } else {

	      /* save offset of first entry */
	      if(mode != FIND_REMOVE_ENTRY) {
//		DEBUG_MSG("save short state\n");
		state->ns_offset = state->offset;
		state->ns_sec    = state->sec;
	      }
	    
	      /* if find short, then use unmodified name */
	      if(mode == FIND_SHORT) {
		MemMove(info->name, dir->name, 11);
		info->name[11]=0;
	      } else {
		/* save normal DOS name */
		for(k=0;k<8;k++) info->name[k]=dir->name[k];
		while(info->name[k-1]==' ') k--;
	      
		/* add optional extension */
		if(dir->name[8] != ' ') {
		  info->name[k++] = '.';
		  info->name[k++] = dir->name[8];
		  if(dir->name[9]  != ' ') info->name[k++] = dir->name[9];
		  if(dir->name[10] != ' ') info->name[k++] = dir->name[10];
		}
		info->name[k]=0;
	      }

	      /* reset fvat sum */
	      vsum = -1;  
	    }
	    
	    if(delete) {
	      dir->name[0] = 0xe5; 
	      glf->dir_sector.dirty = true;  /* this sector is now dirty */
	    }
	    
	    /* skip volume name */
	    if(!(dir->attr & FA_VOLUME)) {
	      
	      /* don't search for directories? */
	      if(!((dir->attr & FA_SUBDIR)&&(mode == FIND_FILES_ONLY))) {

		/* valid filename, no pattern at all or SUBDIR */
		/* for FIND_FILES_N_ALL_DIRS */
		if((pattern[0] == '\0')||(fs_match(pattern,info->name))||
		   ((mode == FIND_FILES_N_ALL_DIRS)&&(dir->attr&FA_SUBDIR))) {

		  /* save state infos */
		  state->bk_sec    = state->sec;
		  state->bk_offset = state->offset;
		  
		  /* return this entry */
		  info->stcl     = DOS2USHORT(dir->stcl);
		  info->attrib   = dir->attr;
		  info->size     = DOS2ULONG(dir->flen);
		  info->date     = fs_date_dos2palm(DOS2USHORT(dir->time), 
						    DOS2USHORT(dir->date));
		  
		  DEBUG_MSG_MATCH("Match: %s %ld\n", info->name, info->size);
		  
		  /* skip to next entry */
		  inc_offset(gl, state, false);
		  return -ErrNone;
		}
	      }
	    }
	  }
	}
      } else {
	cnt++;
      }
    }

    if(mode == FIND_NEW_ENTRY) {
      if((state->ns_offset == state->offset) &&
	 (state->ns_sec    == state->sec)) 
	delete = true;

      if(delete) {
	DEBUG_MSG_NEWDIR("new entry %d in %ld/%d\n", 
			 cnt, state->sec, state->offset);

	end = 0;

	/* first to forelast entry */
	vdir = (struct vfat_dir_entry*)
	  &glf->dir_sector.buffer[32*state->offset];

	/* ok, setup vfat entry */
	vdir->id = (cnt==1)?((req-1) | VSE_LAST):req-cnt;
    
	vdir->attribute = 0x0f;
	vdir->hash1 = 0;
	vdir->sector_l = vdir->sector_u = 0;
	vdir->sum = sum;      
	
	/* setup long name */
	p = pattern + VSE_NAMELEN * (req-cnt-1);
	p += fs_unicode_write(p, vdir->text1, VSE1SIZE, &end);
	p += fs_unicode_write(p, vdir->text2, VSE2SIZE, &end);
	p += fs_unicode_write(p, vdir->text3, VSE3SIZE, &end);

	/* sector is now dirty */
	glf->dir_sector.dirty = true;
      }
    }

    if(mode == FIND_SPACE) {
      if(cnt == 1) {
	state->ns_offset = state->offset;
	state->ns_sec    = state->sec;
      }
    }
    
    if((mode == FIND_SPACE)||(mode == FIND_NEW_ENTRY))
      if(cnt == req)
	return -ErrNone;

    /* skip to next entry */
    if(err=inc_offset(gl, state, mode==FIND_SPACE)) return err;
  } while(1);

  /* this will never be reached */
  return -ErrNone;
}

Err 
fs_findfirst(Lib_globals *gl, int cluster, char *pattern, find_state *state, 
	     axxPacFileInfo *info, int mode, unsigned char sum) {
  filesys_globals *glf = &(gl->filesys);
  Err err;
  int i=0;
  char *p;

  DEBUG_MSG_FIND("cluster: %d\n", cluster);

  /* make sure filesystem is initialized */
  if(err = fs_check(gl)) return err;

  state->cluster = cluster;

  if(cluster == ROOTDIR) {
    state->sec = glf->p_start + glf->res + glf->spf * glf->nfats;
    state->cnt = glf->ndirs/16;  /* 16 entries per sector */

    DEBUG_MSG_FIND("read rootdir at %ld\n", state->sec);
  } else {
    state->sec = CLUSTER2SECTOR(state->cluster);
    state->cnt = glf->spc;       /* get a whole cluster */

    DEBUG_MSG_FIND("read subdir %d at %ld\n", state->cluster, state->sec);
  }

  state->offset = 0;                 /* start at start of sector */
  state->mode = mode;

  err = fs_findnext(gl, pattern, state, info, sum);
  if(err < 0) {
    DEBUG_MSG_FIND("'%s' not found\n", pattern);
    return err;
  }

  return -ErrNone;
}

/* preprocess path */
Err
fs_process_path(Lib_globals *gl, char *name, int *cluster, char **base) {
  filesys_globals *glf = &(gl->filesys);
  int i,k;
  int vsum = -1;
  struct dos_dir_entry *dir;
  struct vfat_dir_entry *vdir;
  char dir_name[64];
  find_state state;
  Err err;
  char *c;
  Boolean found;

  DEBUG_MSG_PATH("try to process %s\n", name);

  /* strip base name */
  if((*base = fs_strrchr(name, DELIMITER)) == NULL)  *base = name;
  else *base = *base + 1;

  state.offset = 0;

  if(name[0] == DELIMITER) {
    state.cluster = ROOTDIR;
    i=1;
  } else {
    state.cluster = glf->current_dir_cluster;
    i=0;
  }

  if(state.cluster == ROOTDIR) {
    state.sec = glf->p_start + glf->res + glf->spf * glf->nfats;
    state.cnt = glf->ndirs/16;  /* 16 entries per sector */
    DEBUG_MSG_PATH("processing root at %ld\n", state.sec);
  } else {
    state.sec = CLUSTER2SECTOR(state.cluster);
    state.cnt = glf->spc;       /* get a whole cluster */
    DEBUG_MSG_PATH("processing subdir at %ld\n", state.sec);
  }

  do {
    /* skip additional path delimiter */
    while(name[i] == DELIMITER) i++;

    /* finished if no path delimiters left in path */
    if(StrChr(&name[i], DELIMITER)==0) {
      DEBUG_MSG_PATH("search finished in cluster %d\n", state.cluster);
      *cluster = state.cluster;
      return ErrNone;
    }

    DEBUG_MSG_PATH("still to process: %s (%d)\n", &name[i], state.cluster);
    found = false;

    /* make sure that sector is in buffer */
    if(err = fs_buffer_sec(gl, &(glf->dir_sector), state.sec, false)) 
      return err;

    do {
      dir = (struct dos_dir_entry*)&glf->dir_sector.buffer[32*state.offset];

      /* found last entry? -> path not found */
      if(dir->name[0]==0x00) {
	error(gl, false, ErrFsNoPath);
	return -ErrFsNoPath; 

      } else {
      /* ok, valid entry */
	if(dir->name[0] != 0xe5) {
	  /* vfat entry? */
	  if(dir->attr==0x0f) {
	    vdir = (struct vfat_dir_entry*)
                  &glf->dir_sector.buffer[32*state.offset];
	    vsum = vdir->sum;
	    
	    c = &(dir_name[VSE_NAMELEN * ((vdir->id & VSE_MASK)-1)]);
	    c += fs_unicode_read(vdir->text1, c, VSE1SIZE);
	    c += fs_unicode_read(vdir->text2, c, VSE2SIZE);
	    c += fs_unicode_read(vdir->text3, c, VSE3SIZE);
	    
	    /* terminate string */
	    if(vdir->id & VSE_LAST)
	      dir_name[VSE_NAMELEN * (vdir->id & VSE_MASK)]=0;
	  } else {	  
	    /* valid long name checksum? */
	    if(vsum == fs_sum_shortname(dir->name)) {
	      /* remove trailing spaces */
	      c = &(dir_name[StrLen(dir_name)-1]);
	      while(*c==' ') *c--=0;
	    } else {
	      /* save normal DOS name */
	      for(k=0;k<8;k++) dir_name[k]=dir->name[k];
	      while(dir_name[k-1]==' ') k--;
	      
	      /* add optional extension */
	      if(dir->name[8] != ' ') {
		dir_name[k++] = '.';
		dir_name[k++] = dir->name[8];
		if(dir->name[9]  != ' ') dir_name[k++] = dir->name[9];
		if(dir->name[10] != ' ') dir_name[k++] = dir->name[10];
	      }
	      dir_name[k]=0;
	    }
	    
	    /* reset fvat sum */
	    vsum = -1;  
	  }
	    
	  /* skip volume name and search for subdirs */
	  if((!(dir->attr & FA_VOLUME))&&(dir->attr & FA_SUBDIR)) {

	    if(fs_match(&name[i], dir_name)) {
	      /* continue search in this entry */
	      state.cluster = DOS2USHORT(dir->stcl);

	      /* compatibility to error in axxPac 1.0 */
	      if(state.cluster == -1) state.cluster = ROOTDIR;

	      if(state.cluster == ROOTDIR) {
		state.sec = glf->p_start + glf->res + glf->spf * glf->nfats;
		state.cnt = glf->ndirs/16;  /* 16 entries per sector */
	      } else {
		state.sec = CLUSTER2SECTOR(state.cluster);
		state.cnt = glf->spc;       /* get a whole cluster */
	      }
	      state.offset = 0;

	      DEBUG_MSG_PATH("found entry for %s (%d)\n", 
			     dir_name, state.cluster);
	      found = true;
	      /* skip to end of entry */
	      while((name[i] != DELIMITER) && (name[i] != 0)) i++;
	    }
	  }
	}
      }
      /* skip to next entry */
      if(!found)
	if(err=inc_offset(gl, &state, false)) return err;

    } while(!found);
  } while(1);

  return ErrNone;
}

Err
fs_changedir(Lib_globals *gl, char *dirname) {
  filesys_globals *glf = &(gl->filesys);
  Err err;
  find_state state;
  int i, j, cluster;
  char *basename;

  /* this creates a axxPacFileInfo without the space for 'RESERVED' */
  short buffer[(sizeof(axxPacFileInfo)-RESERVED_SIZE+1)/2];

  /* copy name to delete into buffer and cut off trailing '/' */
  StrCopy(glf->tmp, dirname);
  i = StrLen(dirname)-1;

  if(i!=0)
    while(glf->tmp[i] == DELIMITER) 
      glf->tmp[i--]=0;

  /* preprocess path */
  if((err = fs_process_path(gl, glf->tmp, &cluster, &basename))<0) return err;

  DEBUG_MSG_CHDIR("search for dir in cluster %d (%s)\n", cluster, basename);

  /* empty basename ('/') */
  if(basename[0]!=0) {

    /* search for entry */
    if(err = fs_findfirst(gl, cluster, basename, &state, 
			  (axxPacFileInfo*)buffer, FIND_FILES_N_DIRS, 0))
      return err;

    DEBUG_MSG_CHDIR("found dir at %d\n", ((axxPacFileInfo*)buffer)->stcl); 

    /* change dir */
    glf->current_dir_cluster = ((axxPacFileInfo*)buffer)->stcl;

    /* compatibility to old axxPac 1.0 error */
    if(glf->current_dir_cluster == -1) glf->current_dir_cluster = ROOTDIR;
  } else {
    DEBUG_MSG_CHDIR("found red. dir at %d\n", cluster); 
    glf->current_dir_cluster = cluster;
  }
      
  /* parse path for getcwd (e.g. honor '.' and '..') */
  i = 0;

  /* absolute path? */
  if(glf->tmp[0] == DELIMITER) {
    StrCopy(glf->path_str, DELIMITER_STR);
    i++;
  }
  
  do {
    /* remove additional DELIMITER */
    while(glf->tmp[i] == DELIMITER) i++;

    if(glf->tmp[i] != 0) {
      j = StrLen(glf->path_str);
      
      /* the '.' dir (current dir) */
      if((glf->tmp[i] == '.')&&
	 ((glf->tmp[i+1] == 0)||(glf->tmp[i+1] == DELIMITER)))
	i += 2;
      
      /* the '..' dir (parent dir) */
      else if((glf->tmp[i] == '.')&&(glf->tmp[i+1] == '.')&&
	      ((glf->tmp[i+2] == 0)||(glf->tmp[i+2] == DELIMITER))) {
	
	/* remove entry */
	j -= 2;  /* skip last delimiter */
	
	while((j>0)&&(glf->path_str[j]!=DELIMITER)) j--;
	if(glf->path_str[j]==DELIMITER) glf->path_str[j+1]='\0';
	
	i += 2;
      } 
      
      /* else just attach dir string */
      else {
	while((glf->tmp[i] != 0)&&(glf->tmp[i] != DELIMITER))
	  glf->path_str[j++] = glf->tmp[i++];
	
	glf->path_str[j++] = DELIMITER;
	glf->path_str[j++] = 0;
      }
    }
  } while(glf->tmp[i] != 0);
  
  DEBUG_MSG_CHDIR("dir now %s\n", glf->path_str);
  
  return err;
}

char *
fs_getcwd(Lib_globals *gl) {
  filesys_globals *glf = &(gl->filesys);
  DEBUG_MSG_PATH("Current dir: %s\n", glf->path_str);
  return(glf->path_str);
}

/* find file discriptor in chain */
file_state* 
fs_find_fd(Lib_globals *gl, axxPacFD fd) {
  filesys_globals *glf = &(gl->filesys);
  file_state *entry=glf->file_list;

  while(entry!=NULL) {
    if(entry->fd == fd) 
      return entry;

    entry = entry->next; /* follow chain */
  }
  
  return NULL;
}

/* free chain of clusters */
void 
fs_freechain(Lib_globals *gl, int cluster) {
  filesys_globals *glf = &(gl->filesys);
  int next;

  if(cluster != 0) {

    DEBUG_MSG_DELETE("delete chain starting at cluster %d\n", cluster);

    while(!is_empty(glf, cluster)) {

      next = fs_get_next_fat_entry(gl, cluster);
      DEBUG_MSG_DELETE("delete cluster %d\n", cluster);

      /* mark cluster as free */
      fs_set_fat_entry(gl, cluster, 0);

      /* erase cluster */
      smartmedia_free_lblock(gl, CLUSTER2SECTOR(cluster));

      /* get next cluster */
      cluster = next;
    }
  }
}

const char short_illegals[] =";+=[]',\"*\\<>/?:|";
const char long_illegals[]  = "\"*\\<>?:|\005";

/* Automatically derive a new name */
static void autorename(char *name,
		       char tilda, char dot, const char *illegals,
		       int limit, int bump)
{
  int tildapos, dotpos;
  unsigned int seqnum=0, maxseq=0;
  char tmp;
  char *p;
	
  tildapos = -1;

  for(p=name; *p ; p++) {
    if((*p < ' ' && *p != '\005') || StrChr(illegals, *p) || (*p&0x80) ) {
      *p = '_';
      bump = 0;
    }
  }

  for(dotpos=0;
      name[dotpos] && dotpos < limit && name[dotpos] != dot ;
      dotpos++) {
    if(name[dotpos] == tilda) {
      tildapos = dotpos;
      seqnum = 0;
      maxseq = 1;
    } else if (name[dotpos] >= '0' && name[dotpos] <= '9') {
      seqnum = seqnum * 10 + name[dotpos] - '0';
      maxseq = maxseq * 10;
    } else
      tildapos = -1; /* sequence number interrupted */
  }

  if(tildapos == -1) {
    /* no sequence number yet */
    if(dotpos > limit - 2) {
      tildapos = limit - 2;
      dotpos = limit;
    } else {
      tildapos = dotpos;
      dotpos += 2;
    }
    seqnum = 1;
  } else {
    if(bump)
      seqnum++;
    if(seqnum > 999999) {
      seqnum = 1;
      tildapos = dotpos - 2;
      /* this matches Win95's behavior, and also guarantees
       * us that the sequence numbers never get shorter */
    }
    if (seqnum == maxseq) {
      if(dotpos >= limit)
	tildapos--;
      else
	dotpos++;
    }
  }
  
  tmp = name[dotpos];
  if((bump && seqnum == 1) || seqnum > 1 )
    StrPrintF(name+tildapos,"%c%d",tilda, seqnum);
  if(dot)
    name[dotpos]=tmp;
  /* replace the character if it wasn't a space */
}

/* generate short dos file name */
void fs_long2dos(char *src, char *dst, int len) {
  int i=0,j=0;
  unsigned char c=-1;

  /* translate at least 8 chars */
  while(i<len) {
    if(c==0) dst[i++]=' ';  /* no more source chars, fill with spaces */
    else {
      c = src[j++];
      
      if((c != ' ')&&(c != '.')&&(c != 0)) { /* skip dot or space */
	if(c<' ') dst[i++] = '_';  /* chars < space get to underscore */
	else {
	  if((c>='a')&&(c<='z')) dst[i++]=(c-'a')+'A';
	  else dst[i++] = c;
	}
      }
    } 
  }
}

Err fs_create_dir_entry(Lib_globals *gl, int cluster, char *name, 
			long pdate, long length, 
			int start_cluster, unsigned char attrib,
			unsigned long *sec, unsigned int *offset) {
  filesys_globals *glf = &(gl->filesys);
  char short_name[12], *p;
  int i, req, old_cluster, next;
  struct dos_dir_entry *dir;
  unsigned short time, date;
  unsigned char sum;
  find_state state;
  Err err;

  /* this creates a axxPacFileInfo without the space */
  /* for the 'RESERVED' field */
  short buffer[(sizeof(axxPacFileInfo)-RESERVED_SIZE+1)/2];

  DEBUG_MSG_NEWDIR("create dir entry '%s'\n", name);

  /* erase buffer for short name */
  for(i=0;i<12;i++) short_name[i]=' ';
  short_name[11]=0;

  /* long file name does not exist, create short DOS file name */

  /* does the filename has an extension? */
  if((p=fs_strrchr(name, '.'))!=NULL) {
    /* copy it */
    fs_long2dos(p, &short_name[8], 3);
    /* temporarily truncate full name */
    *p=0;
  }

  /* copy filename */
  fs_long2dos(name, short_name, 8);

  /* mangle it until we find an unused short DOS name */
  do {
    autorename(short_name, '~', ' ', short_illegals, 8, 1);
    DEBUG_MSG_NEWDIR("verify short name '%s'\n", short_name);
  } while(!fs_findfirst(gl, cluster, short_name, &state, 
			(axxPacFileInfo*)buffer, FIND_SHORT, 0));

  if(p!=NULL) *p='.';  /* 'untruncate' long file name */
  
  DEBUG_MSG_NEWDIR("short name '%s'\n", short_name);
  
  /* ok, create new directory entry */

  /* determine number of required directory entries */
  req = ((StrLen(name)-1) / VSE_NAMELEN) + 2;

  DEBUG_MSG_NEWDIR("req %d entries\n", req);

  /* search for space for new entry */
  if(err = fs_findfirst(gl, cluster, name, &state, 
		  (axxPacFileInfo*)buffer, FIND_SPACE+(req<<8) ,0)) {
    return err;
  }
  
  DEBUG_MSG_NEWDIR("enough space for new file %ld/%d\n",
		   state.ns_sec, state.ns_offset);

  sum = fs_sum_shortname(short_name);

  /* create new entry */
  fs_findfirst(gl, cluster, name, &state, (axxPacFileInfo*)buffer, 
	       FIND_NEW_ENTRY+(req<<8), sum);

  /* short entry is still in buffer */
  dir = (struct dos_dir_entry*)&glf->dir_sector.buffer[32*state.offset];

  if(sec    != NULL) *sec    = state.sec;
  if(offset != NULL) *offset = state.offset;

  /* ok, everything is fine, fill the entry */
  MemMove(dir->name, short_name, 11);
  dir->flen = DOS2ULONG(length);
  dir->stcl = DOS2USHORT(start_cluster);

  /* get current time/date in dos format */
  if(pdate == 0) fs_date_palm2dos(TimGetSeconds(), &time, &date);
  else           fs_date_palm2dos(pdate, &time, &date);

  dir->time = DOS2USHORT(time);
  dir->date = DOS2USHORT(date);
  dir->attr = attrib;

  /* dir sector is now dirty */
  glf->dir_sector.dirty = true;

  return ErrNone;
}

/* update FAT if necessary */
Err fs_update_fat(Lib_globals *gl) {
  filesys_globals *glf = &(gl->filesys);

  DEBUG_MSG("update FAT\n");

  if(glf->fat_dirty) {
    DEBUG_MSG("FAT is dirty\n");

    /* update fats */
    if(smartmedia_write(gl, glf->fat_buffer, 
			glf->p_start+glf->res, 
			glf->spf) < 0) {
      
      error(gl, false, ErrFsWrFAT1);
      return -ErrFsWrFAT1;
    }
    
    if(smartmedia_write(gl, glf->fat_buffer, 
			glf->p_start+glf->res+glf->spf, 
			glf->spf) < 0) {
      
      error(gl, false, ErrFsWrFAT2);
      return -ErrFsWrFAT2;
    }

    /* FAT is in sync again */
    glf->fat_dirty = false;
  }
  DEBUG_MSG("done\n");

  return ErrNone;
}
  
/* open file */
axxPacFD
fs_open(Lib_globals *gl, char *filename, axxPacMode mode) {
  filesys_globals *glf = &(gl->filesys);
  Err err;
  find_state state;
  file_state *fstate;
  axxPacFD fd;
  struct dos_dir_entry *dir;
  int dcluster;
  char *basename;

  /* this creates a axxPacFileInfo without the space */
  /* for the 'RESERVED' field */
  short buffer[(sizeof(axxPacFileInfo)-RESERVED_SIZE+1)/2];

  /* catch empty name */
  if((filename == NULL)||(filename[0] == 0)) {
    /* just return 'file not found' */
    error(gl, false, ErrFsIllegal);
    return -ErrFsIllegal; 
  }

  StrCopy(glf->tmp, filename);

  /* preprocess path */
  if((err = fs_process_path(gl, glf->tmp, &dcluster, &basename))<0) return err;
  
  /* search for entry */
  err = fs_findfirst(gl, dcluster, basename, &state, 
		     (axxPacFileInfo*)buffer, FIND_FILES_ONLY, 0);
  if(!err) {
    DEBUG_MSG_OPEN("found file at %d\n", ((axxPacFileInfo*)buffer)->stcl ); 

#ifdef DEBUG
    /* print cluster chain */
    {
      int j=0, i = ((axxPacFileInfo*)buffer)->stcl;
      while ((i != 0) && (! is_empty(glf, i))) {
	DEBUG_MSG_OPEN("cluster %d = %d\n", j++, i);
	i = fs_get_next_fat_entry(gl, i);      
      }
      DEBUG_MSG_OPEN("cluster end = %d (%x)\n", i, i);
    }
#endif

    /* write access to existing file */
    if((mode != O_RdOnly)&&(mode != O_RdFast)) {
      if(((axxPacFileInfo*)buffer)->attrib & FA_RDONLY) {
	DEBUG_MSG_OPEN("file is write protected\n");
	error(gl, false, ErrFsFileProt);
	return -ErrFsFileProt;
      }
    }

    if(mode == O_Creat) {

      /* read directory sector */
      if(err = fs_buffer_sec(gl, (void*)glf->dir_sector.buffer, 
			     state.bk_sec, false)) 
	return err;

      /* erase old file, create new one */
      DEBUG_MSG_OPEN("O_Creat: file already exists, erase old chain\n");
      fs_freechain(gl, ((axxPacFileInfo*)buffer)->stcl);
      
      /* get address of directory entry */
      dir = (struct dos_dir_entry*)&glf->dir_sector.buffer[32*state.bk_offset];

      /* and mark it empty (in info buffer and directory sector) */
      ((axxPacFileInfo*)buffer)->size = dir->flen = 0; 
      ((axxPacFileInfo*)buffer)->stcl = dir->stcl = 0; 

      /* the directory entry now is dirty */
      glf->dir_sector.dirty = true;
    }
  } else {
    DEBUG_MSG_OPEN("file %s does not exist\n", basename);

    if(mode == O_Creat) {
      DEBUG_MSG_OPEN("O_Creat: trying to create new entry\n");

      /* ok, create dir entry (insert size and the like) */
      err = fs_create_dir_entry(gl, dcluster, basename, 0, 0, 0, 0, 
				&state.bk_sec, &state.bk_offset);

      /* search for entry */
      err = fs_findfirst(gl, dcluster, basename, &state, 
			 (axxPacFileInfo*)buffer, FIND_FILES_N_DIRS, 0);
    }
  }

  /* exit now if file could not be opened */
  if(err) return err;

  /* file is open, allocate file state information */
  if((fstate = (file_state*)MemPtrNew(sizeof(file_state)))==NULL) {
    error(gl, true, ErrSmMemBuf);   /* low level error */
    error(gl, false, ErrFsOpenF);   /* high level error */
    return -ErrFsOpenF;
  }

  DEBUG_MSG_MEM("alloc fstate %ld = %lx\n", sizeof(file_state), fstate);

  /* find free file descriptor (no error */
  /* handling, they will never be _all_ in use) */
  for(fd=0;fs_find_fd(gl, fd)!=NULL; fd++);

  DEBUG_MSG_OPEN("New fd=%d\n", fd);

  fstate->fast    = (mode==O_RdFast);      /* fast read mode (no ecc) */ 

  /* buffer is valid, fill it */
  fstate->fd      = fd;                    /* set file descriptor */
  fstate->flen    = ((axxPacFileInfo*)buffer)->size; 
  fstate->pos     = 0;                     /* start at beginning of file */
  fstate->cluster = ((axxPacFileInfo*)buffer)->stcl; 
                                           /* start  with first cluster */
  fstate->sec     = 0;                     /* -''- first sector of cluster */
  fstate->byte    = 0;                     /* -''- first byte of setcor */
  fstate->sector.sec_no = -1;              /* sector buffer is empty */

  fstate->writeable = (mode != O_RdOnly);  /* this file may be written */
  fstate->update_dir = false;              /* dir entry needs to be updated */
  fstate->stcl = 0;                        /* no new start cluster */

  DEBUG_MSG_OPEN("directory in %ld/%d\n", state.bk_sec, state.bk_offset);
  fstate->dir_sec = state.bk_sec;          /* position of directory entry */
  fstate->dir_offset = state.bk_offset;

  /* insert entry into chain */
  fstate->next    = glf->file_list;
  glf->file_list = fstate;
  
  /* flush directory buffer */
  if(err = fs_buffer_sec(gl, &glf->dir_sector, FLUSH_BUFFER, false))
    return err;

  /* update FAT */
  if((err = fs_update_fat(gl)) < 0) return err;

  return fd;
}

/* close file */
Err
fs_close(Lib_globals *gl, axxPacFD fd) {
  filesys_globals *glf = &(gl->filesys);
  file_state *tmp, **entry = &(glf->file_list);
  long sec;
  struct dos_dir_entry *dir;
  Err err;
  unsigned short time, date;

  while((*entry) != NULL) {
    if((*entry)->fd == fd) {

//      DEBUG_MSG("remove %d from chain\n", fd);

      if((*entry)->update_dir) {
	DEBUG_MSG_WRITE("update dir entry %ld/%d\n",
		  (*entry)->dir_sec, (*entry)->dir_offset);

	if(err = fs_buffer_sec(gl,&glf->dir_sector,(*entry)->dir_sec, false))
	  return err;

	/* update entry */
	/* get address of directory entry */
	dir = (struct dos_dir_entry*)
	  &glf->dir_sector.buffer[32*(*entry)->dir_offset];

	DEBUG_MSG_WRITE("name: %s\n", dir->name);
	
	/* update length and stcl */
	dir->flen = DOS2ULONG((*entry)->flen);
	dir->stcl = DOS2USHORT((*entry)->stcl); 

	/* update time */
	fs_date_palm2dos(TimGetSeconds(), &time, &date);
	dir->time = DOS2USHORT(time);
	dir->date = DOS2USHORT(date);

	glf->dir_sector.dirty = true;

	if(err = fs_buffer_sec(gl,&glf->dir_sector, FLUSH_BUFFER, false))
	  return err;
      }

      DEBUG_MSG("close: flush last sector\n");
      /* flush current sector if neccessary */
      sec = CLUSTER2SECTOR((*entry)->cluster) + (*entry)->sec;

      if(err = fs_buffer_sec(gl, &((*entry)->sector), FLUSH_BUFFER, false)) 
	return err;

      /* remove entry from chain */
      tmp = *entry;
      *entry = (*entry)->next;

      DEBUG_MSG_MEM("free fstate: %lx\n", tmp);
      MemPtrFree(tmp);

      /* update FAT */
      if((err = fs_update_fat(gl)) < 0) return err;

      return -ErrNone;
    }

    entry = &((*entry)->next); /* follow chain */      
  }
  
//  DEBUG_MSG("close: unknown file descriptor %d\n", fd);
  error(gl, false, ErrFsUnkFD);
  return -ErrFsUnkFD;
}

long 
fs_read(Lib_globals *gl, axxPacFD fd, void *data, long len) {
  filesys_globals *glf = &(gl->filesys);
  file_state *fstate = fs_find_fd(gl, fd);
  long sec, cnt=0;
  int burst;
  Err err;

  if(fstate == NULL) {
    DEBUG_MSG("read: unknown file descriptor %d\n", fd);
    error(gl, false, ErrFsUnkFD);
    return -ErrFsUnkFD;
  }

  /* user requests more bytes than available */
  if(len > fstate->flen - fstate->pos) {
    len = fstate->flen - fstate->pos;
    DEBUG_MSG("read limit %ld bytes\n", len);
  }

  /* still bytes to return */
  while(len>0) {
    if((fstate->cluster == 0)||               /* 0 bytes file */
       (is_empty(glf, fstate->cluster))) {    /* end of file */
      DEBUG_MSG("PANIC: reached last cluster\n");
      return cnt;
    }

    /* make sure right sector is in buffer */
    sec = CLUSTER2SECTOR(fstate->cluster) + fstate->sec;

    DEBUG_MSG_READ("cluster %d/%d -> %ld\n", 
		   fstate->cluster, fstate->sec, sec);

    /* is this sector all we still need? */
    if(len < 512 - fstate->byte) {

      if(err = fs_buffer_sec(gl, &(fstate->sector), sec, fstate->fast)) 
	return err;

      MemMove(((char*)data)+cnt, fstate->sector.buffer + fstate->byte, len);
      fstate->byte += len;
      fstate->pos  += len;
      return cnt+len;
    }

    /* at start of sector and more than one sector to copy? */
    if((fstate->byte == 0)&&(len>=512)) {
      burst = len/512;

      /* how many of these blocks are in this cluster */
      if((glf->spc - fstate->sec) < burst) burst = glf->spc - fstate->sec;

//      DEBUG_MSG("burst %ld/%d!\n", sec, burst);

      if(smartmedia_read(gl, ((char*)data)+cnt, sec, burst, fstate->fast)) {
	/* one retry */
	if(smartmedia_read(gl, ((char*)data)+cnt, sec, burst, fstate->fast)) {
//	  DEBUG_MSG("burst read error\n");
	  error(gl, false, ErrFsRdSec);
	  return -ErrFsRdSec;
	}
      }

      fstate->pos += 512 * burst;
      fstate->sec += (burst-1);   /* will later be incremented by 1  */
      cnt += 512 * burst;
      len -= 512 * burst;

    } else {

      /* else buffer sector and copy remaining bytes */
      if(err = fs_buffer_sec(gl, &(fstate->sector), sec, fstate->fast)) 
	return err;

      MemMove(((char*)data)+cnt, fstate->sector.buffer + 
	      fstate->byte, 512 - fstate->byte);

      /* move file/sector/byte/cluster pointers */
      fstate->pos += (512 - fstate->byte);
      cnt += (512 - fstate->byte);
      len -= (512 - fstate->byte);
    }

    /* this sector is 'emptied' */
    fstate->byte = 0;

    /* switch to next sector */
    fstate->sec++;

    /* reached last sector of cluster? */
    if(fstate->sec == glf->spc) {
      fstate->sec = 0;
      
      /* get next cluster */
      fstate->cluster = fs_get_next_fat_entry(gl, fstate->cluster);      
    }
  }
  return cnt;
}

long 
fs_write(Lib_globals *gl, axxPacFD fd, void *data, long len) {
  filesys_globals *glf = &(gl->filesys);
  file_state *fstate = fs_find_fd(gl, fd);
  long sec;
  long cnt=0;
  Err err;
  int new_cluster;

  DEBUG_MSG("fs_write: %d\n", fstate->cluster);

  /* write may change: start cluster (stcl) and file length (length) */
  /* -> update dir entry */

  if(fstate == NULL) {
    DEBUG_MSG_WRITE("write: unknown file descriptor %d\n", fd);
    error(gl, false, ErrFsUnkFD);
    return -ErrFsUnkFD;
  }

  /* still bytes to write */
  while(len>0) {

    DEBUG_MSG_WRITE("bytes to write: %ld\n", len);

    /* new cluster needed? */
    if((fstate->cluster == 0)||(is_empty(glf, fstate->cluster))) {
      DEBUG_MSG_WRITE("need new cluster\n");

      if((new_cluster = fs_get_free_cluster(gl))==NULL) {
	DEBUG_MSG_WRITE("disk is full\n");
	error(gl, false, ErrFsDskFull);
	return -ErrFsDskFull;
      }

      if(fstate->cluster == 0) {
	DEBUG_MSG_WRITE("attaching first cluster %d\n", new_cluster);	
	fstate->stcl = new_cluster;
	fstate->update_dir = true;
	fs_set_fat_entry(gl, new_cluster, 0xfff8);
      } else {
	DEBUG_MSG_WRITE("attaching cluster %d to %d\n", 
			new_cluster, fstate->last_cluster);
	fs_set_fat_entry(gl, fstate->last_cluster, new_cluster);
	fs_set_fat_entry(gl, new_cluster, 0xfff8);
      }

      /* use new cluster */
      fstate->cluster = new_cluster;

      /* flush buffer */
      if(err = fs_buffer_sec(gl, &(fstate->sector), FLUSH_BUFFER, false)) 
	return err;

      sec = CLUSTER2SECTOR(fstate->cluster) + fstate->sec;

      /* setup new buffer */
      MemSet(fstate->sector.buffer, 512, 0xff);
      fstate->sector.sec_no = sec;
      fstate->sector.dirty = true;

      DEBUG_MSG_WRITE("sector %ld in new cluster %d\n",
		      sec, fstate->cluster);
    } else {
      /* make sure right sector is in buffer */
      DEBUG_MSG("cluster: %d/%d\n", fstate->cluster, fstate->sec);

      sec = CLUSTER2SECTOR(fstate->cluster) + fstate->sec;
      DEBUG_MSG("XM\n");
      if(err = fs_buffer_sec(gl, &(fstate->sector), sec, false)) return err;

      DEBUG_MSG_WRITE("sector %ld in exist. cluster %d\n",
		      sec, fstate->cluster);
    }

    /* is this sector all we still need? */
    if(len < 512 - fstate->byte) {

      /* write data */
      MemMove(fstate->sector.buffer + fstate->byte, ((char*)data)+cnt, len);

      /* buffer is now dirty */
      fstate->sector.dirty = true;

      fstate->byte += len;
      fstate->pos  += len;

      /* file size increased */
      if(fstate->pos > fstate->flen) {
	DEBUG_MSG_WRITE("file size %ld -> %ld\n", fstate->flen, fstate->pos);

	fstate->update_dir = true;
	fstate->flen = fstate->pos;
      }

      return cnt+len;
    }

    /* else copy whole remaining sector */
    MemMove(fstate->sector.buffer + fstate->byte, 
	    ((char*)data)+cnt, 512 - fstate->byte);

    /* buffer is now dirty */
    fstate->sector.dirty = true;

    /* move file/sector/byte/cluster pointers */
    fstate->pos += (512 - fstate->byte);
    cnt += (512 - fstate->byte);
    len -= (512 - fstate->byte);

    /* this sector is 'emptied' */
    fstate->byte = 0;

    /* switch to next sector */
    fstate->sec++;

    /* reached last sector of cluster? */
    if(fstate->sec == glf->spc) {
      fstate->sec = 0;

      /* save current cluster */
      fstate->last_cluster = fstate->cluster;

      /* get next cluster */
      DEBUG_MSG("A get next cluster %d\n", fstate->cluster);
      fstate->cluster = fs_get_next_fat_entry(gl, fstate->cluster);      
      DEBUG_MSG("B get next cluster %d\n", fstate->cluster);
    }
  }

  /* file size increased */
  if(fstate->pos > fstate->flen) {
    DEBUG_MSG_WRITE("file size %ld -> %ld\n", fstate->flen, fstate->pos);
    fstate->flen = fstate->pos;
    fstate->update_dir = true;
  }

  return cnt;
}

long 
fs_tell(Lib_globals *gl, axxPacFD fd) {
  file_state *fstate = fs_find_fd(gl, fd);

  if(fstate == NULL) {
    DEBUG_MSG_SEEK("seek: unknown file descriptor %d\n", fd);
    error(gl, false, ErrFsUnkFD);
    return -ErrFsUnkFD;
  }

  return fstate->pos;  /* just return current position */
}

Err
fs_seek(Lib_globals *gl, axxPacFD fd, long offset, int whence) {
  filesys_globals *glf = &(gl->filesys);
  file_state *fstate = fs_find_fd(gl, fd);
  long sec;
  long cnt=0;
  Err err;

  if(fstate == NULL) {
    DEBUG_MSG_SEEK("seek: unknown file descriptor %d\n", fd);
    error(gl, false, ErrFsUnkFD);
    return -ErrFsUnkFD;
  }

  /* convert offset to relative position */
  if(whence == SEEK_SET)
    offset = offset - fstate->pos;                     // absolute from start
  else if(whence == SEEK_END)
    offset = fstate->flen - 1 - offset - fstate->pos;  // absolute from end

  DEBUG_MSG_SEEK("seek rel %ld (to %ld)\n", offset, fstate->pos+offset);

  /* not within file? */
  if(((fstate->pos+offset)<0)||((fstate->pos+offset)>=fstate->flen)) {
    DEBUG_MSG_SEEK("offset out of range: %ld\n", fstate->pos+offset);
    error(gl, false, ErrFsOffset);
    return -ErrFsOffset;
  }

  /* still to seek forward ... */
  while(offset > 0) {
#ifdef DEBUG
    if((fstate->cluster == 0)||                /* 0 bytes file */
       (is_empty(glf, fstate->cluster))) {     /* end of file */
//      DEBUG_MSG("internal seek error!!!\n");
      error(gl, false, ErrFsOffset);
      return -ErrFsOffset;
    }
#endif
    
    /* is new position within current sector? */
    if(offset < 512 - fstate->byte) {
      fstate->byte += offset;
      fstate->pos  += offset;
      return -ErrNone;
    }

    /* move file/sector/byte/cluster to next sector pointers */
    fstate->pos += (512 - fstate->byte);
    offset -= (512 - fstate->byte);
    fstate->byte = 0;                 /* this sector is 'emptied' */    
    fstate->sec++;                    /* switch to next sector */

    /* reached last sector of cluster? */
    if(fstate->sec == glf->spc) {
      fstate->sec = 0;

      /* get next cluster */
      fstate->cluster = fs_get_next_fat_entry(gl, fstate->cluster);      
    }
  }

  /* still to seek backward ... */
  while(offset < 0) {
//    DEBUG_MSG_SEEK("seek backwards %d %ld\n", fstate->byte, offset);

#ifdef DEBUG_SEEK
    if((fstate->cluster == 0)||                /* 0 bytes file */
       (is_empty(glf, fstate->cluster))) {     /* end of file */
//      DEBUG_MSG("internal seek error!!!\n");
      error(gl, false, ErrFsOffset);
      return -ErrFsOffset;
    }
#endif
    
    /* is new position within current sector? */
    if((fstate->byte + offset) >= 0 ) {
//      DEBUG_MSG_SEEK("within sector!\n");

      fstate->byte += offset;
      fstate->pos  += offset;
      return -ErrNone;
    }

    /* move file/sector/byte/cluster to previous sector pointers */
    fstate->pos -= (fstate->byte+1);
    offset += (fstate->byte+1);
    fstate->byte = 511;     /* now in last byte of previous sector */    
    fstate->sec--;          /* switch to previous sector           */

    /* leaving first sector of cluster? */
    if(fstate->sec == -1) {
      fstate->sec = glf->spc-1;

      /* get previous cluster */
      fstate->cluster = fs_get_prev_fat_entry(gl, fstate->cluster);      
#ifdef DEBUG_SEEK
      if(fstate->cluster == 0) {
//	DEBUG_MSG("internal error in backward seek\n");
	return -ErrNone;
      }
#endif
    }
  }

  /* everything is fine ... */
  return -ErrNone;
}

/* check whether name is '.' or '..' */
Boolean
fs_isdotdir(char *name) {
  /* error if filename is '.' or '..' */
  if((name[0]=='.') && ((name[1]==0)|| ((name[1]=='.')&&(name[2]==0))))
    return true;

  return false;
}

/* delete file described by directory entry db in directory dir_cluster*/ 
Err
fs_delete(Lib_globals *gl, char *filename) {
  filesys_globals *glf = &(gl->filesys);
  Err err;
  find_state state;
  int n, dcluster;
  char *basename;

  unsigned long nsec;
  unsigned int noffset;

  /* this creates a axxPacFileInfo without the space */
  /* for the 'RESERVED' field */
  short buffer[(sizeof(axxPacFileInfo)-RESERVED_SIZE+1)/2];

  /* catch empty name */
  if((filename == NULL)||(filename[0] == 0)) {
    /* just return 'file not found' */
    error(gl, false, ErrFsIllegal);
    return -ErrFsIllegal; 
  }

  /* error if filename is '.' or '..' */
  if(fs_isdotdir(filename)) {
    DEBUG_MSG_DELETE("tried to delete '..' or '.'\n");

    /* just return 'file not found' */
    error(gl, false, ErrFsNotFound);
    return -ErrFsNotFound; 
  }

  /* preprocess path */
  if((err = fs_process_path(gl, filename, &dcluster, &basename))<0) return err;

  /* search for entry */
  if(err = fs_findfirst(gl, dcluster, basename, &state, 
			(axxPacFileInfo*)buffer, FIND_FILES_N_DIRS, 0)) {
    DEBUG_MSG_DELETE("delete: file %s not found\n", basename);
    return err;
  }

  /* check if file is write protected */
  if(((axxPacFileInfo*)buffer)->attrib & FA_RDONLY) {
    DEBUG_MSG_OPEN("file is write protected\n");
    error(gl, false, ErrFsFileProt);
    return -ErrFsFileProt;
  }

  /* save sector and offset of found entry */
  nsec = state.ns_sec;
  noffset = state.ns_offset;

  DEBUG_MSG_DELETE("delete %s\n", ((axxPacFileInfo*)buffer)->name);
  DEBUG_MSG_DELETE("       at %ld/%d\n", nsec, noffset);

  if(((axxPacFileInfo*)buffer)->attrib & FA_SUBDIR) {
    /* test if dir is empty */
    n=0;
    err = fs_findfirst(gl, ((axxPacFileInfo*)buffer)->stcl, "", &state, 
		       (axxPacFileInfo*)buffer, FIND_FILES_N_DIRS, 0);
    while(err>=0) {
      n++;
      err = fs_findnext(gl, "", &state, (axxPacFileInfo*)buffer, 0);
    }

    /* empty directory contains two entries '.' and '..' */
    if(n!=2) {
      DEBUG_MSG_DELETE("dir is not empty\n", n);    
      error(gl, false, ErrFsDirNE);
      return -ErrFsDirNE;
    }
  }

  /* restore info one saved sector and offset */
  state.ns_sec = nsec;
  state.ns_offset = noffset;

  /* erase dir entry */
  if(err = fs_findfirst(gl, dcluster, filename, &state, 
			(axxPacFileInfo*)buffer, FIND_REMOVE_ENTRY, 0)) {
    if(err>0) err = -err;
    return err;
  }

  /* free the data chain */
  fs_freechain(gl, ((axxPacFileInfo*)buffer)->stcl);

  /* flush directory buffer */
  if(err = fs_buffer_sec(gl, &glf->dir_sector, FLUSH_BUFFER, false))
    return err;

  /* update FAT */
  if((err = fs_update_fat(gl)) < 0) return err;

  return -ErrNone;
}

Err
fs_stat(Lib_globals *gl, char *name, axxPacStatType *stat) {
  filesys_globals *glf = &(gl->filesys);
  Err err;
  find_state state;
  int cluster;
  char *base;

  /* this creates a axxPacFileInfo without the space */
  /* for the 'RESERVED' field */
  short buffer[(sizeof(axxPacFileInfo)-RESERVED_SIZE+1)/2];

  /* catch empty name */
  if((name == NULL)||(name[0] == 0)) {
    /* just return 'file not found' */
    error(gl, false, ErrFsIllegal);
    return -ErrFsIllegal; 
  }

  /* preprocess path */
  if((err = fs_process_path(gl, name, &cluster, &base))<0) return err;

  /* search for entry */
  if(err = fs_findfirst(gl, cluster, base, &state, 
			(axxPacFileInfo*)buffer, FIND_FILES_N_DIRS, 0)) {
    DEBUG_MSG_STAT("stat: file %s not found\n", basename);
    return err;
  }

  /* set file infos */
  stat->size   = ((axxPacFileInfo*)buffer)->size;
  stat->date   = ((axxPacFileInfo*)buffer)->date;
  stat->attrib = ((axxPacFileInfo*)buffer)->attrib;

  return ErrNone;
}

Err fs_check_name(Lib_globals *gl, char *name) {
  /* this creates a axxPacFileInfo without the space */
  /* for the 'RESERVED' field */
  short buffer[(sizeof(axxPacFileInfo)-RESERVED_SIZE+1)/2];

  find_state state;
  int dcluster;
  char *basename;
  Err err;

  /* remove illegals from long name (replace with '_') */
  autorename(name, '-', '\0', long_illegals, PALM_MAX_NAMELEN, 0);

  /* name doesn't contain any chars anymore */
  if((name[0]==0)||(
     ((name[0]<'A')||(name[0]>'Z'))&&
     ((name[0]<'a')||(name[0]>'z'))&&
     ((name[0]<'0')||(name[0]>'9'))
     )) {

    error(gl, false, ErrFsIllName);
    return -ErrFsIllName;
  }

  /* preprocess path */
  if((err = fs_process_path(gl, name, &dcluster, &basename))<0) return err;

  /* search for entry */
  if(!fs_findfirst(gl, dcluster, basename, &state, 
		   (axxPacFileInfo*)buffer, FIND_FILES_N_DIRS, 0)) {
    DEBUG_MSG_NEWDIR("filename %s already exists\n", name);

    /* just return 'filename already exists' */
    error(gl, false, ErrFsNamExists);
    return -ErrFsNamExists; 
  }

  return ErrNone;
}

/* create a new directory */
Err fs_newdir(Lib_globals *gl, char *name) {
  filesys_globals *glf = &(gl->filesys);
  Err err;
  int sector;
  struct dos_dir_entry *dir;
  unsigned short time, date;

  int new_cluster, dcluster;
  unsigned long new_sec;
  char *basename;

  /* catch empty name */
  if((name == NULL)||(name[0] == 0)) {
    /* just return 'file not found' */
    error(gl, false, ErrFsIllegal);
    return -ErrFsIllegal; 
  }

  /* copy name into buffer */
  StrCopy(glf->tmp, name);

  /* check if this is a valid file to create */
  if((err = fs_check_name(gl, glf->tmp))<0) return err;

  DEBUG_MSG_NEWDIR("create new directory '%s'\n", glf->tmp);

  /* preprocess path */
  if((err = fs_process_path(gl, name, &dcluster, &basename))<0) return err;

  /* get space for new directory */
  if((new_cluster = fs_get_free_cluster(gl))==NULL) {
    DEBUG_MSG_NEWDIR("get subdir space: disk is full\n");
    error(gl, false, ErrFsDskFull);
    return -ErrFsDskFull;
  }

  new_sec = CLUSTER2SECTOR(new_cluster);

  /* init all sectors of this cluster */
  for(sector=0; sector<glf->spc; sector++, new_sec++) {

    /* flush buffer */
    if(err = fs_buffer_sec(gl, &glf->dir_sector, FLUSH_BUFFER, false))
      return err;

    /* setup new buffer */
    MemSet(glf->dir_sector.buffer, 512, 0);
    glf->dir_sector.sec_no = new_sec;
    glf->dir_sector.dirty = true;

    /* setup directory pointers */
    if(sector == 0) {
      /* setup '.' entry */
      dir = (struct dos_dir_entry*)&glf->dir_sector.buffer[0];
      MemSet(dir->name, 11, ' ');  /* erase name */
      dir->name[0]='.';

      /* set pointer to current directory */
      dir->stcl = DOS2USHORT(new_cluster);
      fs_date_palm2dos(TimGetSeconds(), &time, &date);

      dir->time = DOS2USHORT(time);
      dir->date = DOS2USHORT(date);
      dir->attr = FA_SUBDIR;

      /* set up '..' entry */
      dir = (struct dos_dir_entry*)&glf->dir_sector.buffer[32];

      /* copy first entry */
      MemMove(&glf->dir_sector.buffer[32], &glf->dir_sector.buffer[0], 32); 

      /* extend '.' to '..' */
      dir->name[1]='.';

      DEBUG_MSG_NEWDIR("parent cluster = %d\n", dcluster);
      /* set pointer to the parent directory */
      dir->stcl = DOS2USHORT(dcluster);
    }
  }

  /* finally allocate cluster */
  fs_set_fat_entry(gl, new_cluster, 0xfff8);
  DEBUG_MSG_NEWDIR("sub dir created in cluster %d\n", new_cluster);

  /* ok, create dir entry (insert size and the like) */
  if((err = fs_create_dir_entry(gl, dcluster, basename, 0, 0, new_cluster, 
				FA_SUBDIR, NULL, NULL)) < 0) {

    /* deallocate entry */
    fs_set_fat_entry(gl, new_cluster, 0);
    return err;
  }
  
  DEBUG_MSG_NEWDIR("flush dir buffer ...\n");
  /* flush directory buffer */
  if(err = fs_buffer_sec(gl, &glf->dir_sector, FLUSH_BUFFER, false))
    return err;
  
  /* update FAT */
  if((err = fs_update_fat(gl)) < 0) return err;

  return ErrNone;
}

Err      
fs_prepare_access(Lib_globals *gl, Boolean write) {
  error_reset(gl);

  if(gl->smartmedia.type == CARD_NONE) {
    error(gl, false, ErrSmNoDev);
    return -ErrSmNoDev;
  }

  if(smartmedia_open(gl, true)) {
    error(gl, false, ErrFsNoSys);
    return -ErrFsNoSys;
  }

  /* check write protection */
  if((write)&&(SMARTMEDIA_PROT)) {
    error(gl, false, ErrFsWrProt);
    smartmedia_close(gl);
    return -ErrFsWrProt;
  }

  smartmedia_close(gl);

  return ErrNone;
}

/* check whether scluster is a parent cluster of dcluster */
/* both clusters must be directory clusters */
/* make sure, that dcluster points to a directory! */
Boolean
fs_checkparent(Lib_globals *gl, int scluster, int dcluster) {
  filesys_globals *glf = &(gl->filesys);
  Err err;
  struct dos_dir_entry *dir;

  DEBUG_MSG("scluster: %d, dcluster: %d\n", scluster, dcluster);

  /* scan all parent dirs (-1 for compatibility with bug in axxPac V1.0) */
  while((dcluster != ROOTDIR)&&(dcluster != -1)) {

    /* dcluster is in child of scluster! */
    if(dcluster == scluster) return true;

    /* load first directory sector */
    if(err = fs_buffer_sec(gl, &(glf->dir_sector), CLUSTER2SECTOR(dcluster), false))
      return err;

    /* read start cluster of second entry (..) */
    dir = (struct dos_dir_entry*)&glf->dir_sector.buffer[32];
    dcluster = DOS2USHORT(dir->stcl);

    DEBUG_MSG("dcluster -> %d\n", dcluster);
  }

  return false;
}

Err
fs_rename(Lib_globals *gl, char *src_name, char *dst_name) {
  filesys_globals *glf = &(gl->filesys);
  int scluster, dcluster;
  char *sbase, *dbase;
  Err err;
  find_state state;
  unsigned long nsec;
  unsigned int noffset;
  struct dos_dir_entry *dir;

  /* this creates a axxPacFileInfo without the space for 'RESERVED' */
  short buffer[(sizeof(axxPacFileInfo)-RESERVED_SIZE+1)/2];

  /* catch empty dest name */
  if((dst_name == NULL)||(dst_name[0] == 0)) {
    /* just return 'file not found' */
    error(gl, false, ErrFsIllegal);
    return -ErrFsIllegal; 
  }

  /* destination name needs to be in writable buffer */
  StrCopy(glf->tmp, dst_name);

  /* preprocess both paths */
  if((err = fs_process_path(gl, src_name, &scluster, &sbase))<0) return err;
  if((err = fs_process_path(gl, glf->tmp, &dcluster, &dbase))<0) return err;

  /* see whether dest entry already exists */
  if(!fs_findfirst(gl, dcluster, dbase, &state, 
		   (axxPacFileInfo*)buffer, FIND_FILES_N_DIRS, 0)) {
    DEBUG_MSG_RENAME("file '%s' already exists\n", dbase);
    error(gl, false, ErrFsNamExists);
    return -ErrFsNamExists;
  }

  /* check if trying to rename '.' or '..' */
  if(fs_isdotdir(sbase) || fs_isdotdir(dbase)) {
    error(gl, false, ErrFsIllegal);
    return -ErrFsIllegal;
  }

  DEBUG_MSG_RENAME("rename '%s' (%d)\n", src_name, scluster);
  DEBUG_MSG_RENAME("    to '%s' (%d)\n", dst_name, dcluster);

  /* find src entry */
  if((err = fs_findfirst(gl, scluster, sbase, &state, 
			 (axxPacFileInfo*)buffer, FIND_FILES_N_DIRS, 0))<0) {
    DEBUG_MSG_RENAME("unable to find '%s'\n", sbase);
    return err;
  }

  DEBUG_MSG_RENAME("found '%s'\n", sbase);

  if(((axxPacFileInfo*)buffer)->attrib & FA_SUBDIR) {
    DEBUG_MSG("source is dir!\n");

    if(fs_checkparent(gl, ((axxPacFileInfo*)buffer)->stcl, dcluster)) {
      DEBUG_MSG_RENAME("destination is subdir of source!\n");
      error(gl, false, ErrFsMoveDir);
      return -ErrFsMoveDir;
    }
  }

  /* save sector and offset of found entry */
  nsec = state.ns_sec;
  noffset = state.ns_offset;

  /* ok, create dir entry (insert size and the like) */
  if((err = fs_create_dir_entry(gl, dcluster, dbase, 
	((axxPacFileInfo*)buffer)->date, 
	((axxPacFileInfo*)buffer)->size, 
	((axxPacFileInfo*)buffer)->stcl, 
	((axxPacFileInfo*)buffer)->attrib, NULL, NULL)) < 0) {
    DEBUG_MSG_RENAME("error creating new entry\n");
    return err;
  }

  DEBUG_MSG_RENAME("created '%s'\n", dbase);

  /* if subdir was moved into another dir, setup it's '..' entry */
  if((((axxPacFileInfo*)buffer)->attrib & FA_SUBDIR)&&(dcluster != scluster)) {
    DEBUG_MSG_RENAME("update '..' in subdir '%s'\n", dbase);

    /* load first sector of subdir into buffer */
    if(err = fs_buffer_sec(gl, &glf->dir_sector, 
			   CLUSTER2SECTOR(((axxPacFileInfo*)buffer)->stcl),
			   false))
      return err;

    dir = (struct dos_dir_entry*)&glf->dir_sector.buffer[32];
    dir->stcl = DOS2USHORT(dcluster);

    glf->dir_sector.dirty = true;   /* buffer is now dirty */
  }

  /* restore info one saved sector and offset */
  state.ns_sec = nsec;
  state.ns_offset = noffset;

  /* erase dir entry */
  if((err = fs_findfirst(gl, scluster, sbase, &state, 
	      (axxPacFileInfo*)buffer, FIND_REMOVE_ENTRY, 0))<0) {
    DEBUG_MSG_RENAME("error removing src entry\n");
    return err;
  }

  DEBUG_MSG_RENAME("flush dir buffer ...\n");
  /* flush directory buffer */
  if(err = fs_buffer_sec(gl, &glf->dir_sector, FLUSH_BUFFER, false))
    return err;
  
  /* update FAT */
  if((err = fs_update_fat(gl)) < 0) return err;

  return ErrNone;
}

Err
fs_setattr(Lib_globals *gl, char *name, unsigned char attr, Boolean set_all) {
  filesys_globals *glf = &(gl->filesys);
  int cluster;
  char *base;
  Err err;
  find_state state;
  struct dos_dir_entry *dir;

  /* this creates a axxPacFileInfo without the space for 'RESERVED' */
  short buffer[(sizeof(axxPacFileInfo)-RESERVED_SIZE+1)/2];

  /* preprocess path */
  if((err = fs_process_path(gl, name, &cluster, &base))<0) return err;

  DEBUG_MSG_SETWP("setattrib %s(%d)\n", name, cluster);

  /* find entry */
  if((err = fs_findfirst(gl, cluster, base, &state, 
			 (axxPacFileInfo*)buffer, FIND_FILES_N_DIRS, 0))<0) {
    DEBUG_MSG_SETWP("unable to find '%s'\n", sbase);
    return err;
  }

  /* make sure sector is in buffer */
  if(err = fs_buffer_sec(gl, &(glf->dir_sector), state.bk_sec, false))
    return err;  

  /* entry is now in buffer */
  dir = (struct dos_dir_entry*)&glf->dir_sector.buffer[32*state.bk_offset];

  if(set_all) {
    /* update - don't update all attributes */
    dir->attr = (dir->attr & (FA_VOLUME | FA_SUBDIR)) | 
                (     attr & (FA_RDONLY | FA_HIDDEN | FA_SYSTEM | FA_ARCH));
  } else {
    /* set (unset) protection bit */
    if(attr & FA_RDONLY) dir->attr |=  FA_RDONLY;
    else                 dir->attr &= ~FA_RDONLY;
  }

  glf->dir_sector.dirty = true;

  /* flush directory buffer */
  if(err = fs_buffer_sec(gl, &glf->dir_sector, FLUSH_BUFFER, false))
    return err;

  return ErrNone;
}

Err
fs_setdate(Lib_globals *gl, char *name, UInt32 date) {
  filesys_globals *glf = &(gl->filesys);
  int cluster;
  char *base;
  Err err;
  find_state state;
  struct dos_dir_entry *dir;
  unsigned short ddate, dtime;

  /* this creates a axxPacFileInfo without the space for 'RESERVED' */
  short buffer[(sizeof(axxPacFileInfo)-RESERVED_SIZE+1)/2];

  /* preprocess path */
  if((err = fs_process_path(gl, name, &cluster, &base))<0) return err;

  DEBUG_MSG_SETWP("setdate %s(%d)\n", name, cluster);

  /* find entry */
  if((err = fs_findfirst(gl, cluster, base, &state, 
			 (axxPacFileInfo*)buffer, FIND_FILES_N_DIRS, 0))<0) {
    DEBUG_MSG_SETWP("unable to find '%s'\n", sbase);
    return err;
  }

  /* make sure sector is in buffer */
  if(err = fs_buffer_sec(gl, &(glf->dir_sector), state.bk_sec, false))
    return err;  

  /* entry is now in buffer */
  dir = (struct dos_dir_entry*)&glf->dir_sector.buffer[32*state.bk_offset];

  /* set date */
  fs_date_palm2dos(date, &dtime, &ddate);
  dir->time = DOS2USHORT(dtime);
  dir->date = DOS2USHORT(ddate);

  glf->dir_sector.dirty = true;

  /* flush directory buffer */
  if(err = fs_buffer_sec(gl, &glf->dir_sector, FLUSH_BUFFER, false))
    return err;

  return ErrNone;
}

