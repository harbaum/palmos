#define noMECKER

/*
 * djpeg.c
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains a command-line user interface for the JPEG decompressor.
 * It should work on any system with Unix- or MS-DOS-style command lines.
 *
 * Two different command line styles are permitted, depending on the
 * compile-time switch TWO_FILE_COMMANDLINE:
 *	djpeg [options]  inputfile outputfile
 *	djpeg [options]  [inputfile]
 * In the second style, output is always to standard output, which you'd
 * normally redirect to a file or pipe to some other program.  Input is
 * either from a named file or from standard input (typically redirected).
 * The second style is convenient on Unix but is unhelpful on systems that
 * don't support pipes.  Also, you MUST use the first style if your system
 * doesn't do binary I/O to stdin/stdout.
 * To simplify script writing, the "-outfile" switch is provided.  The syntax
 *	djpeg [options]  -outfile outputfile  inputfile
 * works regardless of which command line style is used.
 */

#include "jinclude.h"
#include "cdjpeg.h"		/* Common decls for cjpeg/djpeg applications */
#include "jversion.h"		/* for version message */

// #include <ctype.h>		/* to declare isprint() */

#ifdef USE_CCOMMAND		/* command-line reader for Macintosh */
#ifdef __MWERKS__
#include <SIOUX.h>              /* Metrowerks needs this */
#include <console.h>		/* ... and this */
#endif
#ifdef THINK_C
#include <console.h>		/* Think declares it here */
#endif
#endif

#ifdef PALM
/* defines */
#define VPW   ((unsigned char  *)0xFFFFFA05)
#define LSSA  ((unsigned long  *)0xFFFFFA00)
#define PICF  ((unsigned char  *)0xFFFFFA20)
#define LPXCD ((unsigned char  *)0xFFFFFA25)
#define CKCON ((unsigned char  *)0xFFFFFA27)
#define LBAR  ((unsigned char  *)0xFFFFFA29)
#define FRCM  ((unsigned char  *)0xFFFFFA31)
#define LGPMR ((unsigned short *)0xFFFFFA32)

#define dxScreen 160
#define dyScreen 160
#define cbGreyScreen (dxScreen*dyScreen/2)	

const unsigned long mask0[]={ 
  0xffffffff,0x0fffffff,0x00ffffff,0x000fffff,
  0x0000ffff,0x00000fff,0x000000ff,0x0000000f
};

const unsigned long mask1[]={ 
  0x00000000,0xf0000000,0xff000000,0xfff00000,
  0xffff0000,0xfffff000,0xffffff00,0xfffffff0
};

const unsigned short tr[]={
  0x0000, 0x000f, 0x00f0, 0x00ff, 0x0f00, 0x0f0f, 0x0ff0, 0x0fff,
  0xf000, 0xf00f, 0xf0f0, 0xf0ff, 0xff00, 0xff0f, 0xfff0, 0xffff
};

void WinXDrawBitmapEx(j_decompress_ptr cinfo,
		      unsigned char *pbBits, short xSrc, short ySrc, 
		      short xDest, short yDest, short dxDest, short dyDest, 
		      short rowBytes, Boolean fDrawChar) {
  unsigned short *dst = ((unsigned short *)cinfo->vWinX.pbGreyScreenBase) + 
    xDest/4 + 40*yDest;
  unsigned short *src = ((unsigned short*)pbBits) + xSrc/16 + 
    (rowBytes/2)*ySrc;
  unsigned long *s, *d, mask, b, smask;

  int i, doff = (xDest&3)<<2, soff = xSrc&15;

  smask = 0xffff0000 >> dxDest;

  for(i=0;i<dyDest;i++) {
    d = (unsigned long*)dst;
    s = (unsigned long*)src;

    /* get min 16 pixels (enough for character) */
    b = (*s >> (16-soff)) & smask;

    mask = mask1[(dxDest>4)?4:dxDest] >> doff;
    *d = (*d & ~mask) | ((unsigned long)tr[(b>>12)&15])<<(16-doff); 

    /* more than 4 pixels? */
    if(dxDest>4) {
      mask = mask1[(dxDest>8)?4:(dxDest-4)] >> doff;
            d = (unsigned long*)(dst+1);
      *d = (*d & ~mask) | ((unsigned long)tr[(b>> 8)&15])<<(16-doff); 
    }

    dst += 40;  /* next row */
    src += rowBytes/2;
  }
}

void WinXDrawChars(j_decompress_ptr cinfo,
		   char *chars, unsigned short len, 
		   short x, short y, int offset) {
  char *pch;
  char *pchMac;
  FontPtr pfnt;
  unsigned short *plocTbl;
  unsigned short *ploc;
  FontCharInfoType *pfciTbl;
  FontCharInfoType *pfci;
  RectangleType rc;
  int xRight;
  
  rc.topLeft.x = x;
  rc.topLeft.y = y;
  rc.extent.x = dxScreen-x;
  rc.extent.y = FntCharHeight();
  
  pfnt = FntGetFontPtr();
  
  xRight = rc.topLeft.x+rc.extent.x;
  pchMac = chars+len;
  plocTbl = ((UInt16 *)(pfnt+1))+pfnt->rowWords*pfnt->fRectHeight;
  pfciTbl = (FontCharInfoType *) (((UInt16 *)&pfnt->owTLoc)+pfnt->owTLoc);
  pchMac = chars+len;
  
  for (pch = chars; pch < pchMac && x < xRight; pch++) {
    int ch;
    int dch;
    int dxCharImage;
    int xCharImage;	
    
    ch = *pch + offset;
    if (ch < pfnt->firstChar || ch > pfnt->lastChar) {
Missing:		
      ch = pfnt->lastChar+1;
    }

    dch = ch-pfnt->firstChar;
    pfci = pfciTbl+dch;
    if (*(UInt16 *)pfci == 0xffff)
      goto Missing;
    ploc = plocTbl+dch;
    xCharImage = *ploc;
    dxCharImage = *(ploc+1)-xCharImage;
    if (dxCharImage > 0) {
      if (x < rc.topLeft.x) {
	// check for partial on left
	if (x + dxCharImage >= rc.topLeft.x)
	  xCharImage+=rc.topLeft.x-x;
	else
	  goto SkipChar;
      }
      if (x + dxCharImage >= xRight) {
	// partial on right
	dxCharImage = xRight-x;
      }
      WinXDrawBitmapEx(cinfo,
		       (unsigned char *)(pfnt+1), xCharImage, 0,  
		       x, y, dxCharImage, pfnt->fRectHeight, 
		       pfnt->rowWords*2, 1);
    }
SkipChar:			
    x += pfci->width;
  }
}	

int WinXSetGreyscale(j_decompress_ptr cinfo) {
  DWord mode;

  cinfo->vWinX.pbGreyScreenBase = MemPtrNew(cbGreyScreen);

  cinfo->vWinX.pbMonoScreenBase=(char*)*LSSA;
  MemSet(cinfo->vWinX.pbGreyScreenBase, cbGreyScreen, 0);

  /* set pixel clock devider to full speed */
  cinfo->vWinX.lpxcd = *LPXCD;

  /* try to get current refresh settings */
  if(FtrGet(CREATOR, 0, &mode)) mode = 0;  // use system settings 

  if(mode != 0) *LPXCD = mode-1;

  *CKCON=*CKCON & 0x7F;	/* display off*/
	
  /*virtual page width now 80 bytes (160 pixels)*/
  *VPW=40;
  *PICF=*PICF | 0x02; /*switch to grayscale mode*/
  *LBAR=20; /*line buffer now 40 bytes*/
  
  /*register to control grayscale pixel oscillations*/
  *FRCM=0xB9;
  
  /*let the LCD get to a 2 new frames (40ms delay) */
  SysTaskDelay(4);
  
  /*switch LCD back on */
  *CKCON=*CKCON | 0x80;
   
  *LSSA=(long)cinfo->vWinX.pbGreyScreenBase;

  return(0);
}

int WinXSetMono(j_decompress_ptr cinfo) {

  *LSSA=(long)cinfo->vWinX.pbMonoScreenBase;

  //switch off LCD update temporarily
  *CKCON=*CKCON & 0x7F;
  
  //virtual page width now 20 bytes (160 pixels)
  *VPW=10;
  *PICF=*PICF & 0xFC; //switch to black and white mode
  *LBAR=10; // line buffer now 20 bytes
  
  //let the LCD get to a new frame (20ms delay)
  SysTaskDelay(4);
  
  //switch LCD back on in new mode
  *CKCON=*CKCON | 0x80;

  // restore original pixel clock devider state
  *LPXCD = cinfo->vWinX.lpxcd;
	
  MemPtrFree(cinfo->vWinX.pbGreyScreenBase);

  return 0;
}
#endif

/* Create the add-on message string table. */
#if 0
#define JMESSAGE(code,string)	string ,

static const char * const cdjpeg_message_table[] = {
#include "cderror.h"
  NULL
};
#endif
/*
 * Argument-parsing code.
 * The switch parser is designed to be useful with DOS-style command line
 * syntax, ie, intermixed switches and file names, where only the switches
 * to the left of a given file name affect processing of that file.
 * The main program in this file doesn't actually use this capability...
 */


/*
 * Marker processor for COM and interesting APPn markers.
 * This replaces the library's built-in processor, which just skips the marker.
 * We want to print out the marker as text, to the extent possible.
 * Note this code relies on a non-suspending data source.
 */

LOCAL(unsigned int)
jpeg_getc (j_decompress_ptr cinfo)
/* Read next byte */
{
  struct jpeg_source_mgr * datasrc = cinfo->src;

  if (datasrc->bytes_in_buffer == 0) {
    if (! (*datasrc->fill_input_buffer) (cinfo))
      ERREXIT(cinfo, JERR_CANT_SUSPEND);
  }
  datasrc->bytes_in_buffer--;
  return GETJOCTET(*datasrc->next_input_byte++);
}


/*
 * The main program.
 */

#ifdef MECKER
#define T 33
const char msg1[]={
  'T'-T,'h'-T,'i'-T,'s'-T,' '-T,
  'v'-T,'e'-T,'r'-T,'s'-T,'i'-T,'o'-T,'n'-T,' '-T,
  'i'-T,'s'-T,0};
const char msg2[]={
  'n'-T,'o'-T,'t'-T,' '-T,
  'f'-T,'o'-T,'r'-T,0};
const char msg3[]={
  'd'-T,'i'-T,'s'-T,'t'-T,'r'-T,'i'-T,'b'-T,'u'-T,'t'-T,'i'-T,'o'-T,'n'-T,0};
#endif

#ifdef PALM
void jpeg_view(UInt LibRef, axxPacFD fd, char *name, long length)
#else
void main (void)
#endif
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
#ifdef PROGRESS_REPORT
  struct cdjpeg_progress_mgr progress;
#endif
  int file_index;
  djpeg_dest_ptr dest_mgr = NULL;
  char msg[32];
  Boolean quit = false;

  FILE * input_file;

#ifdef PALM
  EventType e;
#else
  FILE * output_file;
#endif
  JDIMENSION num_scanlines;

  DEBUG_MSG("sizeof cinfo = %ld\n", sizeof(struct jpeg_decompress_struct));
  DEBUG_MSG("sizeof jerr = %ld\n", sizeof(struct jpeg_error_mgr));

#ifdef PALM
  cinfo.fd = fd;
  cinfo.LibRef = LibRef;

  WinXSetGreyscale(&cinfo);
#endif

#ifdef MECKER
  WinXDrawChars(&cinfo, msg1, StrLen(msg1), 53, 60, T);
  WinXDrawChars(&cinfo, msg2, StrLen(msg2), 68, 72, T);
  WinXDrawChars(&cinfo, msg3, StrLen(msg3), 58, 84, T);
#endif

  /* Initialize the JPEG decompression object with default error handling. */
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  /* Add some application-specific error messages (from cderror.h) */

  /* Now safe to enable signal catcher. */
#ifdef NEED_SIGNAL_CATCHER
  enable_signal_catcher((j_common_ptr) &cinfo);
#endif

  /* Scan command line to find file names. */
  /* It is convenient to use just one switch-parsing routine, but the switch
   * values read here are ignored; we will rescan the switches after opening
   * the input file.
   * (Exception: tracing level set here controls verbosity for COM markers
   * found during jpeg_read_header...)
   */

  /* die hier koennen alle fix reincodiert werden */

#ifndef PALM
  /* default input file is stdin */
  input_file = stdin; // read_stdin();

  /* default output file is stdout */
  output_file = stdout; // write_stdout();
#endif

#ifdef PROGRESS_REPORT
  start_progress_monitor((j_common_ptr) &cinfo, &progress);
#endif

  /* Specify data source for decompression */
  jpeg_stdio_src(&cinfo, input_file);

  /* Read file header, set default decompression parameters */
  (void) jpeg_read_header(&cinfo, TRUE);

  /* die hier koennen alle fix reincodiert werden */

  /* Initialize the output module now to let it override any crucial
   * option settings (for instance, GIF wants to force color quantization).
   */
  dest_mgr = jinit_write_ppm(&cinfo);
  //  dest_mgr->output_file = output_file;

  /* Start decompressor */
  (void) jpeg_start_decompress(&cinfo);

  /* Write output file header */
  (*dest_mgr->start_output) (&cinfo, dest_mgr);

  /* Process data */
  while ((cinfo.output_scanline < cinfo.output_height)&&(!quit)) {

    num_scanlines = jpeg_read_scanlines(&cinfo, dest_mgr->buffer,
    					dest_mgr->buffer_height);
    (*dest_mgr->put_pixel_rows) (&cinfo, dest_mgr, num_scanlines);

    EvtGetEvent(&e, 0);
    SysHandleEvent(&e);

    if(e.eType == penDownEvent) quit = true;
  }

  /* Finish decompression and release memory.
   * I must do it in this order because output module has allocated memory
   * of lifespan JPOOL_IMAGE; it needs to finish before releasing memory.
   */
  if(!quit)
    jpeg_finish_decompress(&cinfo);

  jpeg_destroy_decompress(&cinfo);

  /* close files ... */

#ifdef PROGRESS_REPORT
  end_progress_monitor((j_common_ptr) &cinfo);
#endif

#ifdef PALM
  if(!quit) {
    WinXDrawChars(&cinfo, name, StrLen(name), 0, 160-11,0);
    StrPrintF(msg, "%dx%d", cinfo.width, cinfo.height);
    WinXDrawChars(&cinfo, msg, StrLen(msg), 160-
		  FntLineWidth(msg, StrLen(msg)), 160-11,0);

    do {
      EvtGetEvent(&e, evtWaitForever);
      SysHandleEvent(&e);
    } while(e.eType != penDownEvent);
  }

appStopEvent:

  WinXSetMono(&cinfo);
#endif
}

#ifdef PALM
long palmread(j_decompress_ptr cinfo, char *buf, long sizeofbuf) {
  DEBUG_MSG("read %ld bytes\n", sizeofbuf);

  return axxPacRead(cinfo->LibRef, cinfo->fd, buf, sizeofbuf);
}

#endif
