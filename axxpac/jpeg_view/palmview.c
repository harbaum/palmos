
#ifdef OS35HDR
#include <PalmOS.h>
#include <PalmCompatibility.h>
#else
#include <Pilot.h>
#endif


#include "../api/axxPac.h"

#include "jinclude.h"
#include "cdjpeg.h"		/* Common decls for cjpeg/djpeg applications */

#ifdef PPM_SUPPORTED


/*
 * For 12-bit JPEG data, we either downscale the values to 8 bits
 * (to write standard byte-per-sample PPM/PGM files), or output
 * nonstandard word-per-sample PPM/PGM files.  Downscaling is done
 * if PPM_NORAWWORD is defined (this can be done in the Makefile
 * or in jconfig.h).
 * (When the core library supports data precision reduction, a cleaner
 * implementation will be to ask for that instead.)
 */

#if BITS_IN_JSAMPLE == 8
#define PUTPPMSAMPLE(ptr,v)  *ptr++ = (char) (v)
#define BYTESPERSAMPLE 1
#define PPM_MAXVAL 255
#else
#ifdef PPM_NORAWWORD
#define PUTPPMSAMPLE(ptr,v)  *ptr++ = (char) ((v) >> (BITS_IN_JSAMPLE-8))
#define BYTESPERSAMPLE 1
#define PPM_MAXVAL 255
#else
/* The word-per-sample format always puts the LSB first. */
#define PUTPPMSAMPLE(ptr,v)			\
	{ register int val_ = v;		\
	  *ptr++ = (char) (val_ & 0xFF);	\
	  *ptr++ = (char) ((val_ >> 8) & 0xFF);	\
	}
#define BYTESPERSAMPLE 2
#define PPM_MAXVAL ((1<<BITS_IN_JSAMPLE)-1)
#endif
#endif


/*
 * When JSAMPLE is the same size as char, we can just fwrite() the
 * decompressed data to the PPM or PGM file.  On PCs, in order to make this
 * work the output buffer must be allocated in near data space, because we are
 * assuming small-data memory model wherein fwrite() can't reach far memory.
 * If you need to process very wide images on a PC, you might have to compile
 * in large-memory model, or else replace fwrite() with a putc() loop ---
 * which will be much slower.
 */


/* Private version of data destination object */

typedef struct {
  struct djpeg_dest_struct pub;	/* public fields */

  /* Usually these two pointers point to the same place: */
  char *iobuffer;		/* fwrite's I/O buffer */
  JSAMPROW pixrow;		/* decompressor output buffer */
  size_t buffer_width;		/* width of I/O buffer */
  JDIMENSION samples_per_row;	/* JSAMPLEs per output row */
} ppm_dest_struct;

typedef ppm_dest_struct * ppm_dest_ptr;

//const unsigned char contrast[]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
const unsigned char contrast[]={ 5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,12 };

/*
 * Write some pixel data.
 * In this module rows_supplied will always be 1.
 *
 * put_pixel_rows handles the "normal" 8-bit case where the decompressor
 * output buffer is physically the same as the fwrite buffer.
 */
METHODDEF(void)
put_pixel_rows (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
		JDIMENSION rows_supplied)
{
  int i,w,j,k,m,t,u;
  ppm_dest_ptr dest = (ppm_dest_ptr) dinfo;
  char *d;

  if(dest->buffer_width>160) {
    j = 160;
    k = 0;
    m = (dest->buffer_width-160)/4;
  } else {
    j = dest->buffer_width;
    k = (160-dest->buffer_width)/2;
    m = 0;
  }

  d = cinfo->vWinX.pbGreyScreenBase + 80*cinfo->line + k/2;

  u = (t = cinfo->line & 1)^1;
  if((cinfo->line>=0)&&(cinfo->line<160)) {
    for(i=0;i<j;i+=2) {
      *d++ = ~( (contrast[u+((dest->iobuffer[m+i  ]>>4)&0x0f)]<<4) + 
		(contrast[t+((dest->iobuffer[m+i+1]>>4)&0x0f)]));
    }
  }
  cinfo->line++;
}

/*
 * Startup: write the file header.
 */

METHODDEF(void)
start_output_ppm (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo)
{
  ppm_dest_ptr dest = (ppm_dest_ptr) dinfo;

  DEBUG_MSG("image size %d x %d\n", 
	    (int)cinfo->output_width, (int)cinfo->output_height);

  cinfo->line = 74-(cinfo->output_height/2);
  cinfo->width  = 8*cinfo->output_width;
  cinfo->height = 8*cinfo->output_height;
}

GLOBAL(djpeg_dest_ptr)
jinit_write_ppm (j_decompress_ptr cinfo)
{
  ppm_dest_ptr dest;

  /* Create module interface object, fill in method pointers */
  dest = (ppm_dest_ptr)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(ppm_dest_struct));
  dest->pub.start_output = start_output_ppm;

  /* Calculate output image dimensions so we can allocate space */
  jpeg_calc_output_dimensions(cinfo);

  /* Create physical I/O buffer.  Note we make this near on a PC. */
  dest->samples_per_row = cinfo->output_width * cinfo->out_color_components;
  dest->buffer_width = dest->samples_per_row * (BYTESPERSAMPLE * SIZEOF(char));
  dest->iobuffer = (char *) (*cinfo->mem->alloc_small)
    ((j_common_ptr) cinfo, JPOOL_IMAGE, dest->buffer_width);

  /* We will fwrite() directly from decompressor output buffer. */
  /* Synthesize a JSAMPARRAY pointer structure */
  /* Cast here implies near->far pointer conversion on PCs */
  dest->pixrow = (JSAMPROW) dest->iobuffer;
  dest->pub.buffer = & dest->pixrow;
  dest->pub.buffer_height = 1;
  dest->pub.put_pixel_rows = put_pixel_rows;

  return (djpeg_dest_ptr) dest;
}

#endif /* PPM_SUPPORTED */
