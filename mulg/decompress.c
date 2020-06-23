/*

  decompress.c

  decompressor for compressed resources - (c) 2001 by Till Harbaum

*/

#include <PalmOS.h>
#include "decompress.h"
#include "mulg.h"

#define VERBOSE             // define this for extended and penible error checking
#define BUFFER_SIZE 512

#define MAX_DECOMPRESS 8    // max ressources to be decompressed

#define EXP2_DIC_MAX   12   // max word counter
#define INDEX_MAX (1<<EXP2_DIC_MAX)

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;

/* global variables */
static DmOpenRef dbRef=NULL;     /* database for expanded resources */
static void *address[MAX_DECOMPRESS];

static const uint mask_table[]={0,1,3,7,15,31,63,127,255,511,1023,2047,4095};

/* one single dictionary entry */
typedef struct dic_entry_struct { 
  uchar entry;
  struct dic_entry_struct *prec;
} dic_entry;

/* temporary decompressor variables */
typedef struct {
  uchar *source_ptr, *dest_ptr;
  ushort dest_offset, written, *src_processed;
  uint bits_processed;

  uchar output_buffer[BUFFER_SIZE];
  int in_buffer;
 
  dic_entry dictionary[INDEX_MAX];
} decompress_globals;

static void write_output(decompress_globals *gl, uint value) {
  gl->written++;

  /* store byte in buffer */
  (gl->output_buffer)[gl->in_buffer++] = value;

  /* and see whether we have to flush the buffer */
  if(gl->in_buffer == BUFFER_SIZE) {
    DmWrite(gl->dest_ptr, gl->dest_offset, gl->output_buffer, gl->in_buffer);
    gl->dest_offset += gl->in_buffer;
    gl->in_buffer = 0;
  }
}

static void flush_output(decompress_globals *gl) {
  DmWrite(gl->dest_ptr, gl->dest_offset, gl->output_buffer, gl->in_buffer);
}

#define BUFFERSIZE 16
static uchar write_chain(decompress_globals *gl, dic_entry *chain) { 
  uchar entry, buffer[BUFFERSIZE];
  uint i;
  dic_entry *mchain = chain->prec;

  if (mchain != NULL) { 
    i=0; mchain = chain->prec;
    while((mchain != NULL)&&(i<BUFFERSIZE)) {
      entry = buffer[i++] = mchain->entry; 
      mchain = mchain->prec;
    }

    /* still stuff to do ... recurse */
    if(mchain != NULL)
      entry = write_chain(gl, mchain);

    while(i>0) write_output(gl, buffer[--i]);
    write_output(gl, chain->entry);
  } else { 
    write_output(gl, chain->entry);
    entry = chain->entry;
  }
  return entry;
}

static uchar write_string(decompress_globals *gl, uint prec_code, 
			 uint current_code, uchar first_entry, uint index_dic) { 
  uchar entry;

  if(current_code < index_dic) {
    entry = write_chain(gl, &gl->dictionary[current_code]);
  } else { 
    entry = write_chain(gl, &gl->dictionary[prec_code]);
    write_output(gl, first_entry);
  }
  return entry;
}

static uint read_code(decompress_globals *gl, uint bits) {
  ulong code;

  code = ((*((ulong*)gl->src_processed)) >> 
	  (32-bits-gl->bits_processed)) & mask_table[bits];

  /* skip processed bits */
  gl->bits_processed += bits;
  if(gl->bits_processed > 16) {
    gl->bits_processed  -= 16;
    gl->src_processed++;
  }

  return code;
}

/* free all resources occupied by this decompressor */
static void free_all(decompress_globals *gl, MemHandle resH, Boolean fail) {
  if(gl != NULL) {
    if(gl->source_ptr != NULL) {
      MemPtrUnlock(gl->source_ptr);
      DmReleaseResource(resH);
    }
    MemPtrFree(gl);
  }
}

void *decompress_rsc(ulong rsc, int id) {
  MemHandle resH;
  uint prec_code, current_code, bits_req, index_dic;
  uchar first_entry=0;
  uint bit_counter_limit, size;
  UInt16 at;
  LocalID dbid;
  MemHandle record;
  decompress_globals *gl = NULL;
  int i;

  /* get globals (don't bust stack with locals) */
  gl = MemPtrNew(sizeof(decompress_globals));

  /* init globals to zero */
  MemSet((void*)gl, sizeof(decompress_globals), 0);

  /* open source ressource */
  resH = (MemHandle)DmGetResource(rsc, id);
#ifdef VERBOSE
  ErrFatalDisplayIf( !resH, "Missing resource!" );
#endif

  gl->source_ptr = MemHandleLock(resH);
  gl->src_processed = (ushort*)(&gl->source_ptr[2]);

  /* create a dummy database for the new record */
  if(dbRef == NULL) {

    /* clear decoder infos */
    for(i=0;i<MAX_DECOMPRESS;i++)
      address[i] = NULL;

    DmCreateDatabase(0, DECOMPRESS_DB_NAME, CREATOR, DECOMPRESS_TYPE, false);
 
    if((dbid = DmFindDatabase(0, DECOMPRESS_DB_NAME))==NULL)
      ErrFatalDisplayIf( true, "Unable to create buffer db");

    dbRef = DmOpenDatabase(0, dbid, dmModeReadWrite);
#ifdef VERBOSE
    ErrFatalDisplayIf(dbRef==NULL, "Unable to open buffer db");
#endif
  }

  /* find free decoder slot */
  for(at = -1,i=MAX_DECOMPRESS-1;i>=0;i--) {
    if(address[i] == NULL) at = i;
  }

#ifdef VERBOSE
  ErrFatalDisplayIf(at == -1, "Out of decoder slots");
#endif

  size = 256u*(uint)gl->source_ptr[0] + gl->source_ptr[1];
  if((record = DmNewRecord(dbRef, &at, size))==0) {
    /* out of memory */
    free_all(gl, resH, true);
    return NULL;
  }

  /* lock record in memory */
  address[at] = gl->dest_ptr = MemHandleLock(record);
  gl->dest_offset = 0;

  /* init dictionary values */
  for (i=0;i<256;i++)       (gl->dictionary)[i].entry = i;

  index_dic = 256;
  bits_req = 9;
  bit_counter_limit = 511;
  prec_code = current_code = read_code(gl, 9);

  first_entry = write_string(gl, prec_code, current_code, first_entry, index_dic);

  /* still bytes to create */
  while(gl->written < size) {
    current_code = read_code(gl, bits_req);

#ifdef VERBOSE    
    /* internal failure (corrupted binary) */
    if(current_code > index_dic)
      ErrFatalDisplayIf(true, "Decompression failed!");
#endif

    first_entry = write_string(gl, prec_code, current_code, first_entry, index_dic);
    
    /* prepare for new dictionary */
    if (index_dic == INDEX_MAX-2) {

      /* re-init dictionary values */
      for(i=0;i<INDEX_MAX;i++) (gl->dictionary)[i].prec = NULL;
      index_dic = 256;
      bits_req = 9;
      bit_counter_limit = 511;

      /* if still bytes to extract */
      if(gl->written < size) {
	prec_code = current_code = read_code(gl, bits_req);
	first_entry = write_string(gl, prec_code,current_code,first_entry,index_dic);
      }
    } else {
      /* add string to dictionary */
      (gl->dictionary)[index_dic].entry = first_entry;
      (gl->dictionary)[index_dic].prec  = &(gl->dictionary)[prec_code];

      if (++index_dic == bit_counter_limit) {
	bits_req++;
	bit_counter_limit = (bit_counter_limit<<1)+1;
      }
    }
    prec_code = current_code;
  }

  /* flush output buffers */
  flush_output(gl);

  free_all(gl, resH, false);

  return address[at];
}

void decompress_free(void *addr) {
  int i, at;
  LocalID dbid;

#ifdef VERBOSE
  ErrFatalDisplayIf(dbRef == NULL, "Database not open!");
#else
  if(dbRef == NULL) return;
#endif

  /* find matching entry */
  for(at=-1, i=0;i<MAX_DECOMPRESS;i++)
    if(address[i]==addr) at=i;

#ifdef VERBOSE
  ErrFatalDisplayIf(at == -1, "Unused decoder slot!");
#else
  if(at == -1) return;
#endif

  /* unlock, release and remove record */
  MemPtrUnlock(addr);   
  address[at] = NULL;
    
  /* release record */
  DmReleaseRecord(dbRef, at, true); 
  DmDeleteRecord(dbRef, at);

  /* count used slots */
  for(at=0, i=0;i<MAX_DECOMPRESS;i++)
    if(address[i]!=NULL) at++;

  /* if no record left close database and remove */
  if(at == 0) {
    DmCloseDatabase(dbRef);
    dbid = DmFindDatabase(0, DECOMPRESS_DB_NAME);
    DmDeleteDatabase(0, dbid);
    dbRef = NULL;
  }
}
