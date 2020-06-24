/*

  decompress.c

  decompressor for compressed resources - (c) 2001 by Till Harbaum

*/

#include <PalmOS.h>

#include "Rsc.h"
#include "decompress.h"
#include "debug.h"

#define EXP2_DIC_MAX   12   // max word counter
#define INDEX_MAX (1<<EXP2_DIC_MAX)

/* global variables */
static const UInt16 mask_table[]={0,1,3,7,15,31,63,127,255,511,1023,2047,4095};

/* one single dictionary entry */
typedef struct dic_entry_struct { 
  UInt8 entry;
  struct dic_entry_struct *prec;
} dic_entry;

/* temporary decompressor variables */
typedef struct {
  UInt8 *source_ptr, *dest_ptr;
  UInt16 *src_processed, written;
  UInt16 bits_processed;

  dic_entry dictionary[INDEX_MAX];
} decompress_globals;

/* write a byte */
static void write_output(decompress_globals *gl, UInt16 value) {
  *gl->dest_ptr++ = value;
  gl->written++;
}

#define BUFFERSIZE 16
static UInt8 write_chain(decompress_globals *gl, dic_entry *chain) { 
  UInt8 entry, buffer[BUFFERSIZE];
  UInt16 i;
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

static UInt8 write_string(decompress_globals *gl, UInt16 prec_code, 
			  UInt16 current_code, UInt8 first_entry, 
			  UInt16 index_dic) { 
  UInt8 entry;

  if(current_code < index_dic) {
    entry = write_chain(gl, &gl->dictionary[current_code]);
  } else { 
    entry = write_chain(gl, &gl->dictionary[prec_code]);
    write_output(gl, first_entry);
  }
  return entry;
}

static UInt16 read_code(decompress_globals *gl, UInt16 bits) {
  UInt32 code;

  code = ((*((UInt32*)gl->src_processed)) >> 
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

void *decompress_rsc(UInt32 rsc, int id) {
  MemHandle resH;
  UInt16 prec_code, current_code, bits_req, index_dic;
  UInt8 first_entry=0;
  UInt16 bit_counter_limit, size;
  UInt16 at;
  LocalID dbid;
  MemHandle record;
  decompress_globals *gl = NULL;
  Int16 i;
  void *dest;

  /* get globals (don't bust stack with locals) */
  if((gl = MemPtrNew(sizeof(decompress_globals)))==NULL) {
    DEBUG_PRINTF("out of memory in decompression\n");
    return NULL;
  }

  /* init globals to zero */
  MemSet((void*)gl, sizeof(decompress_globals), 0);

  /* open source ressource */
  resH = (MemHandle)DmGetResource(rsc, id);

  gl->source_ptr = MemHandleLock(resH);
  gl->src_processed = (UInt16*)(&gl->source_ptr[2]);

  size = 256u*(UInt16)gl->source_ptr[0] + gl->source_ptr[1];

  /* allocate enough memory */
  DEBUG_PRINTF("allocate %d bytes for %s/%d\n", size, &rsc, id);
  dest = gl->dest_ptr = MemPtrNew(size);
  if(dest == NULL) {
    DEBUG_PRINTF("out of memory in decompression\n");
    return NULL;
  }

  /* init dictionary values */
  for (i=0;i<256;i++)       (gl->dictionary)[i].entry = i;

  index_dic = 256;
  bits_req = 9;
  bit_counter_limit = 511;
  prec_code = current_code = read_code(gl, 9);

  first_entry = write_string(gl, prec_code, current_code, 
			     first_entry, index_dic);

  /* still bytes to create */
  while(gl->written < size) {
    current_code = read_code(gl, bits_req);

    /* internal failure (corrupted binary) */
    if(current_code > index_dic) {
      DEBUG_PRINTF("decompression failed\n");
      free_all(gl, resH, false);
      return NULL;
    }

    first_entry = write_string(gl, prec_code, current_code, 
			       first_entry, index_dic);
    
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
	first_entry = write_string(gl, prec_code,current_code,
				   first_entry,index_dic);
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

  free_all(gl, resH, false);

  return dest;
}

