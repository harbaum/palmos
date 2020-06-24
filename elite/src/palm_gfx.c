/*
 * Elite - The New Kind, PalmOS port
 *
 * Reverse engineered from the BBC disk version of Elite.
 * Additional material by C.J.Pinder and T.Harbaum
 *
 * The original Elite code is (C) I.Bell & D.Braben 1984.
 * This version re-engineered in C by C.J.Pinder 1999-2001.
 *
 * email: <christian@newkind.co.uk>
 *
 * PalmOS port (c) 2002 by Till Harbaum <till@harbaum.org>
 *
 */

#include <PalmOS.h>
#include <DLServer.h>

#include "config.h"
#include "gfx.h"
#include "elite.h"
#include "shipdata.h"
#include "docked.h"
#include "debug.h"
#include "Rsc.h"
#include "splash.h"
#include "decompress.h"
#include "space.h"
#include "sound.h"
#include "options.h"

/* grayscale support */
UInt8 *graytab = NULL;
static const char graymask[2] = { 0xf0, 0x0f };
int pixperbyte;

static char *GetVidbase (void);

static UInt32 last_tick;
UInt16 ticks;

/* sinus table to be exported to 3d part */
Int16 *sintab = NULL;

// #define SHOW_FPS

#define MAX_POLYS	100

#define SPACE_WIDTH     3

/* pointer to offscreen video buffer */
char *vidbuff = NULL;

static int start_poly;
static int total_polys;

static Boolean clipping = 0;
static int clip_bx, clip_by, clip_tx, clip_ty;

static UInt8 *fontPtr = NULL;
static char *textPtr = NULL;

UInt8 engine_idx = 0;

/* global pointer to video memory */
char *vidbase = NULL;

/* enable easter egg */
Bool fluffy_dices;

/* enable gerhard fun stuff */
Bool gerhard;

/* hardware allows direct video access */
Bool direct_video;
BitmapType *offscreen_bitmap;

/* sprite stuff */
char *sprite_data[SPRITE_DECOMP];
const UInt8 sprite_size[SPRITE_NUM][2] = {
  {160,40}, {210,10}, {40,40}, {48,64}, {48,64}, {24,10} };

UInt32 GetRomVersion(void) {
  UInt32 romVersion=0;
  
  FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
  return(romVersion);
}

static char *
GetVidbase (void) {
  if(GetRomVersion() < PalmOS35)
    return WinGetDrawWindow ()->displayAddrV20;

  return BmpGetBits(WinGetBitmap(WinGetDrawWindow()));
}

#ifdef DEBUG
static Boolean gfx_running = true;

void gfx_toggle_update(void) {
  DEBUG_PRINTF("stop/start video update\n");
  gfx_running = !gfx_running;
}
#endif

Boolean
gfx_graphics_startup (void)
{
  int i,j;
  const char *oom="Out of memory.";
  UInt8 *s;
  char user_name[dlkUserNameBufSize];
  UInt32 *d, col;
  Err error;

  /* check if user is named gerhard */
  DlkGetSyncInfo(0, 0, 0, user_name, 0, 0);
  gerhard = (StrNCaselessCompare(user_name, "gerhard", 6) == 0);

  fluffy_dices = PrefGetPreference(prefAllowEasterEggs);

  /* decompress texts */
  textPtr = (char*)decompress_rsc('text', 0);

  /* fetch font data */
  fontPtr = (UInt8*)decompress_rsc('font', 0);

  /* fetch ship data */
  init_ship_data ();

  /* load sinus table */
  sintab = (UInt16*)decompress_rsc('sine', 0);

  /* grayscale has 2 pixels per byte */
  pixperbyte = color_gfx?1:2;

  /* get screen base and erase screen */
  vidbase = GetVidbase ();

  /* let's assume direct video is posible */
  direct_video = true;

  /* WinSetForeColor is only present on machines >= OS 3.5 */
  if(GetRomVersion() >= PalmOS35) {

    /* draw pixel with OS and verify directly to verify video buffer layout */
    WinSetForeColor(1); WinDrawPixel(0,0);
    WinSetForeColor(2); WinDrawPixel(1,0);
    WinSetForeColor(3); WinDrawPixel(0,1);
    WinSetForeColor(4); WinDrawPixel(1,1);
    
    /* verify, that pixels show up at the right position in video memory */
    if(((color_gfx)&&
	((vidbase[0]   != 1)||(vidbase[1]   != 2)||
	 (vidbase[160] != 3)||(vidbase[161] != 4)))||
       ((!color_gfx)&&
	((vidbase[0] != 0x12)||(vidbase[80] != 0x34))))
      
      direct_video = false;
  }

#if 0
  /* force direct video to false */
  direct_video = false;
  FrmCustomAlert(alt_err, "Testing direct video.",0,0);
#endif

  if(direct_video) {
    /* allocate offscreen buffer */
    vidbuff = MemPtrNew(DSP_HEIGHT * DSP_WIDTH / pixperbyte );
  } else {
    /* create a big offscreen bitmap */
    offscreen_bitmap = BmpCreate(160, 160, 8, NULL, &error);

    if(!offscreen_bitmap) {
      FrmCustomAlert(alt_err, "Error creating offscreen bitmap.",0,0);
      return false;
    }

    /* get address of offscreen bitmap */
    vidbase = BmpGetBits(offscreen_bitmap);
    vidbuff = vidbase;
  }

  if((vidbuff == NULL)||(sintab == NULL)||
     (textPtr == NULL)||(fontPtr == NULL)) {
    FrmCustomAlert(alt_err, oom, 0, 0);
    return false;
  }

  /* clear buffer (black) */
  for (d = (long *) vidbuff, i = 0; 
       i < DSP_HEIGHT * DSP_WIDTH / (4*pixperbyte) ; i++)
    *d++ = -1;

  /* decompress grayscale conversion table if requiered */
  if(!color_gfx) {
    graytab = decompress_rsc('gray',0);
    if(graytab == NULL) {
      FrmCustomAlert(alt_err, oom, 0, 0);
      return false;
    }

    /* if invert, adjust pointer to inverted map */
    if(options.bool[OPTION_INVERT]) {
      DEBUG_PRINTF("inverse video\n");
      graytab += 256;
    }
  }
  
  /* clear screen (black) */
  if(color_gfx) col = 0xffffffff;
  else          col = COL_EXPAND_32(graytab[0xff]);

  for (d = (long *) vidbase, i = 0; i < 160 * 160 / (4*pixperbyte); i++)
    *d++ = col;

#ifdef SPLASH
  /* show plash screen */
  do_splash();
  if(finish) return 0;
#endif

  /* clear screen (black) */
  for (d = (long *) vidbase, i = 0; i < 160 * 160 / (4*pixperbyte); i++)
    *d++ = col;

  /* decompress sprites */
  for(i=0;i<SPRITE_DECOMP;i++) {
    if((sprite_data[i] = decompress_rsc('sprt', i)) == NULL) {
      FrmCustomAlert(alt_err, "Error accessing internal ressources.", 0, 0);
      return false;
    }

    /* convert grayscale sprites */
    if(!color_gfx) {
      s = (UInt8*)sprite_data[i];
      for(j=0;j<sprite_size[i][0]*sprite_size[i][1];j++,s++)
	*s = graytab[*s];
    }
  }

  /* draw scanner and buttons */
  gfx_draw_scanner();

  /* init game timer to faked 1/20 sec */
  last_tick = TimGetTicks() - i*(SysTicksPerSecond()/20);
  ticks = 50;

  return true;
}

char *get_text(int offset) {
  return textPtr + offset;
}

void
gfx_graphics_shutdown (void)
{
  int i;

  /* free texts */
  if(textPtr) MemPtrFree(textPtr);

  /* free grayscale map */
  if(graytab) {
    /* correct graytab pointer */
    if(options.bool[OPTION_INVERT])
      graytab -= 256;

    MemPtrFree(graytab);  
  }

  /* free sinus table */
  if(sintab)
    MemPtrFree(sintab);

  if(direct_video) {
    /* free video buffer */
    if (vidbuff)
      MemPtrFree (vidbuff);
  } else {
    /* free offscreen bitmap */
    if(offscreen_bitmap)
      BmpDelete(offscreen_bitmap);
  }
  
  /* free sprites */
  for(i=0;i<SPRITE_DECOMP;i++) {
    if(sprite_data[i] != NULL)
      MemPtrFree(sprite_data[i]);
  }

  /* free ship data */
  free_ship_data ();

  /* free font data */
  if(fontPtr != NULL)
    MemPtrFree(fontPtr);
}

/* draw character in 8 bit color */
static int
gfx_draw_char_c256(unsigned char chr, int x, int y, UInt32 col) {
  const UInt32 mask[]={
    0x00000000, 0x000000ff, 0x0000ff00, 0x0000ffff,
    0x00ff0000, 0x00ff00ff, 0x00ffff00, 0x00ffffff,
    0xff000000, 0xff0000ff, 0xff00ff00, 0xff00ffff,
    0xffff0000, 0xffff00ff, 0xffffff00, 0xffffffff };

  UInt32 *dst = (UInt32*)(vidbuff + 160 * y + (x&~1));
  UInt16 *src = ((UInt16*)fontPtr) + 64 + ((chr * 6)>>4);
  UInt16 m;
  int doff = x&1;
  int soff2, soff = (chr * 6) & 15;  // bit offset to char in src
  int j,i,n,w;
  Bool more, m2;

  if (chr == ' ')
    return SPACE_WIDTH;

  w = fontPtr[chr];  // char width

  /* we need bits from a second source byte */
  soff2 = 16 - soff;
  more  = soff2 - w < 0;
  
  m2 = (fontPtr[chr] + doff) > 4;

  for (j = 0; j < 6; j++) {  // six lines
    m = src[0] << soff;  // get source bits

    /* need to load a second byte?? */
    if(more) m |= src[1] >> soff2;  // get source bits

    m &=  0xfc00;
    m >>= doff;  // one bit shift
    
    n = m>>12;
    dst[0] = (dst[0] & ~mask[n]) | (mask[n] & col);
    
    /* need to write a second long word? */
    if(m2) {
      n = (m>>8)&0x0f;
      dst[1] = (dst[1] & ~mask[n]) | (mask[n] & col);
    }
    
    dst += 40;
    src += 48;
  }

  return w + 1;
}

/* draw character in 4 bit grayscale */
static int
gfx_draw_char_g16(unsigned char chr, int x, int y, UInt16 gray)
{
  const UInt16 mask[]={
    0x0000, 0x000f, 0x00f0, 0x00ff, 0x0f00, 0x0f0f, 0x0ff0, 0x0fff,
    0xf000, 0xf00f, 0xf0f0, 0xf0ff, 0xff00, 0xff0f, 0xfff0, 0xffff };

  UInt16 *dst = (UInt16*)(vidbuff + 80 * y + ((x>>1)&~1));
  UInt16 *src = ((UInt16*)fontPtr) + 64 + ((chr * 6)>>4);
  UInt16 m;
  int doff = x&3;
  int soff2, soff = (chr * 6) & 15;  // bit offset to char in src
  int j,i,n,w;
  Bool more, m2;

  if (chr == ' ')
    return SPACE_WIDTH;

  w = fontPtr[chr];  // char width

  /* we need bits from a second source byte */
  soff2 = 16 - soff;
  more  = soff2 - w < 0;
  
  m2 = (fontPtr[chr] + doff) > 4;

  for (j = 0; j < 6; j++) {  // six lines
    m = src[0] << soff;  // get source bits

    /* need to load a second byte?? */
    if(more) m |= src[1] >> soff2;  // get source bits

    m &=  0xfc00;
    m >>= doff;  // one bit shift

#ifdef DEBUG
    if(m & 0xff) DEBUG_PRINTF("char overflow %d at %d\n", chr, x);
#endif

    n = m>>12;
    dst[0] = (dst[0] & ~mask[n]) | (mask[n] & gray);
    
    /* need to write a second word? */
    if(m2) {
      n = (m>>8)&0x0f;
      dst[1] = (dst[1] & ~mask[n]) | (mask[n] & gray);
    }
    
    dst += 40;
    src += 48;
  }

  return w + 1;
}

static void
gfx_draw_str (char *str, int x, int y, UInt8 col) {
#ifdef DEBUG
  /* don't write if offscreen */
  if (y > DSP_HEIGHT - GFX_CELL_HEIGHT) {
    DEBUG_PRINTF ("gfx_draw_str() out of screen: %s\n", str);
    return;
  }
#endif

  if(color_gfx) {
    UInt32 color = COL_EXPAND_32(col);

    while (*str)
      x += gfx_draw_char_c256 (*str++, x, y, color);
  } else {
    UInt16 gray;
    UInt8  gray_map_l[] = { 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x22, 
	0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xff };
    UInt8  gray_map_d[] = { 
	0x00, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 
	0xee, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

    if(options.bool[OPTION_INVERT]) 
      /* darken text */
      gray = COL_EXPAND_16(gray_map_d[0x0f & graytab[col]]);
    else
      /* brighten text */
      gray = COL_EXPAND_16(gray_map_l[0x0f & graytab[col]]);

    while (*str)
      x += gfx_draw_char_g16 (*str++, x, y, gray);
  }
}

int
gfx_str_width (unsigned char *str)
{
  int width = 0;

  while (*str) {
    if (*str == ' ')
      width += SPACE_WIDTH;
    else
      width += (fontPtr[*str] & 0x0f) + 1;

    str++;
  }
  
  if(width>0) width--;  // no spacing after last char

  return width;
}

static void
hline (int x, int y, int x2, int col)
{
  UInt8 *d;

  if (clipping) {
    if ((y < clip_ty) || (y > clip_by))   return;
    if ((x > clip_bx) || (x2 < clip_tx))  return;
    
    if (x < clip_tx)   x = clip_tx;
    if (x2 > clip_bx) x2 = clip_bx;
  }

  if(!color_gfx) {
    int n = x&1;
    col = graytab[col];   // convert to grayscale

    /* grayscale */
    d = (UInt8*)(vidbuff + DSP_WIDTH/2 * y + x/2);
    while (x2 >= x) {
      *d = (*d & ~graymask[n]) | (graymask[n] & col);
      if(n) { n=0; d++; } 
      else    n=1;
      x2--;
    }
  } else {
    /* color */
    d = (UInt8*)(vidbuff + DSP_WIDTH * y + x);
    while (x2 >= x) {
      *d++ = col;
      x2--;
    }
  }
}

static void
vline (int x, int y, int y2, int col)
{
  char *d;

  if (clipping) {
    if ((x < clip_tx) || (x > clip_bx))
      return;
    if ((y > clip_by) || (y2 < clip_ty))
      return;

    if (y < clip_ty)
      y = clip_ty;
    if (y2 > clip_by)
      y2 = clip_by;
  }

  if(!color_gfx) {
    /* grayscale */
    int n = x&1;
    col = graytab[col];   // convert to grayscale

    d = (UInt8*)(vidbuff + DSP_WIDTH/2 * y + x/2);
    while (y2 >= y) {
      *d = (*d & ~graymask[n]) | (graymask[n] & col);
      d += DSP_WIDTH/2;
      y2--;
    }
  } else {
    /* color */
    d = vidbuff + DSP_WIDTH * y + x;
    while (y2 >= y) {
      *d = col;
      d += DSP_WIDTH;
      y2--;
    }
  }
}

/*
 * Blit the back buffer to the screen.
 */

void
gfx_update_screen (void)
{
  int i;
  long *d = (long *) vidbase, *s = (long *) vidbuff;
  UInt32 cticks = TimGetTicks();

#ifdef SHOW_FPS
#define SAMPLES 100l

  static UInt32 lasttick = 0;
  static UInt16 count = 0, lastfps = 0;
  UInt32 curtick;
  char str[32];
#endif

  /* update tick counter */

  /* calculate current tick */
  ticks = 1000l*(cticks - last_tick)/SysTicksPerSecond();
  if(!ticks) ticks=1;
  last_tick = cticks;
  
//  DEBUG_PRINTF("ticks: %d\n", ticks);
  
  snd_update_sound ();

#ifdef DEBUG
  if(!gfx_running) {
    SysTaskDelay(10);
    return;
  }
#endif

  if ((current_screen & SCR_FLAG_3D)&&(fluffy_dices))
    gfx_draw_sprite(SPRITE_DICE, 80-20, 0);
  
#ifdef SHOW_FPS
  if (++count == SAMPLES) {
    curtick = TimGetTicks ();

    if (lasttick != 0)
      lastfps = (SAMPLES * 10l * SysTicksPerSecond ()) / (curtick - lasttick);

    lasttick = curtick;
    count = 0;
  }

  gfx_clear_area (1, 1, 40, 6);
  StrPrintF (str, "FPS: %d.%d", lastfps / 10, lastfps % 10);
  gfx_display_text(1, 1, ALIGN_LEFT, str, GFX_COL_WHITE);

#endif

  /* increase engine color index */
  engine_idx = (engine_idx + 1) & 7;

  if(direct_video) {
    /* copy the data directly to the screen */
    for (i = 0; i < DSP_WIDTH * DSP_HEIGHT / (16 * pixperbyte); i++) {
      *d++ = *s++;
      *d++ = *s++;
      *d++ = *s++;
      *d++ = *s++;
    }
  } else
    /* or let the palm os do the job */
    WinDrawBitmap(offscreen_bitmap, 0, 0);
}

void
gfx_indirect_update(void) {
  if(!direct_video)
    WinDrawBitmap(offscreen_bitmap, 0, 0);    
}

void
gfx_draw_title(char *title, Bool erase) {
  if(erase) gfx_clear_display ();
  else      gfx_clear_area(0, TITLE_Y, 159, TITLE_LINE_Y);

  if(color_gfx)
    gfx_display_text(DSP_CENTER+1,TITLE_Y+1,ALIGN_CENTER,title, GFX_COL_BROWN);

  gfx_display_text(DSP_CENTER, TITLE_Y, ALIGN_CENTER, title, GFX_COL_GOLD);
  gfx_draw_line (0, TITLE_LINE_Y, 159, TITLE_LINE_Y);
}

void
gfx_plot_pixel (int x, int y, int col)
{
#ifdef DEBUG
  if ((x < 0) || (x > DSP_WIDTH - 1))
    DEBUG_PRINTF ("pixel x = %ld out\n", (long) x);
  if ((y < 0) || (y > DSP_HEIGHT - 1))
    DEBUG_PRINTF ("pixel y = %ld out\n", (long) y);
#endif

  if(color_gfx)
    vidbuff[DSP_WIDTH * y + x] = col;
  else {
    vidbuff[DSP_WIDTH/2 * y + x/2] &= ~graymask[x&1]; 
    vidbuff[DSP_WIDTH/2 * y + x/2] |=  graymask[x&1] & graytab[col]; 
  }
}

void
gfx_draw_filled_circle(int cx, int cy, int radius, int col)
{
  int counter = radius, xval = 0, yval = radius, line;
  int y0 = cy-radius, y1 = cy+radius;
  
  do {
    if (counter < 0) {
      counter += 2 * --yval;

      hline(cx-xval+2, ++y0, cx+xval-2, col);
      hline(cx-xval+2, --y1, cx+xval-2, col);
    } else
      counter -= 2 * xval++;
  }
  while (yval > 1);

  hline(cx-xval+2, ++y0, cx+xval-2, col);
}

void
gfx_draw_circle(int cx, int cy, int radius, int col)
{
  int counter = radius, xval = 0, yval = radius, line;
  int y0 = cy-radius, y1 = cy+radius, last=0;
  
  do {
    if (counter < 0) {
      counter += 2 * --yval;

      hline(cx+last-1, ++y0, cx+xval-2, col);
      hline(cx-xval+2,   y0, cx-last+1, col);

      hline(cx+last-1, --y1, cx+xval-2, col);
      hline(cx-xval+2,   y1, cx-last+1, col);

      last = xval - 1;
    } else
      counter -= 2 * xval++;
  }
  while (yval > 1);

  hline(cx+last-1, ++y0, cx+xval-2, col);
  hline(cx-xval+2,   y0, cx-last+1, col);
}


void
gfx_draw_colour_line (int x1, int y1, int x2, int y2, int line_colour)
{
  int t;

#ifdef DEBUG
  /* verify overflow */
  if ((y1 < 0) || (y1 > DSP_HEIGHT - 1) ||
      (y2 < 0) || (y2 > DSP_HEIGHT - 1) ||
      (x1 < 0) || (x1 > DSP_WIDTH - 1) || (x2 < 0) || (x2 > DSP_WIDTH - 1)) {

    DEBUG_PRINTF ("gfx_draw_colour_line() out of limits\n");
    return;
  }
#endif

  if (y1 == y2) {
    if (x2 < x1) {
      t = x2;
      x2 = x1;
      x1 = t;
    }
    hline (x1, y1, x2, line_colour);
    return;
  }

  if (x1 == x2) {
    if (y2 < y1) {
      t = y2;
      y2 = y1;
      y1 = t;
    }
    vline (x1, y1, y2, line_colour);
    return;
  }

  DEBUG_PRINTF("drawing non-vertical/horizontal line!\n");
}

void
gfx_draw_line (int x1, int y1, int x2, int y2) {
  gfx_draw_colour_line (x1, y1, x2, y2, GFX_COL_WHITE);
}

static void
xor_hline (int x, int y, int x2, int col)
{
  char *d;

  if (clipping) {
    if ((y < clip_ty) || (y > clip_by))
      return;
    if ((x > clip_bx) || (x2 < clip_tx))
      return;

    if (x < clip_tx)
      x = clip_tx;
    if (x2 > clip_bx)
      x2 = clip_bx;
  }

  if(!color_gfx) {
    int n = x&1;

    /* grayscale */
    d = (UInt8*)(vidbuff + DSP_WIDTH/2 * y + x/2);
    while (x2 >= x) {
      *d ^= (graymask[n] & col);
      if(n) { n=0; d++; } 
      else    n=1;
      x2--;
    }
  } else {
    /* color */
    d = (UInt8*)(vidbuff + DSP_WIDTH * y + x);
    while (x2 >= x) {
      *d++ ^= col;
      x2--;
    }
  }
}

static void
xor_vline (int x, int y, int y2, int col)
{
  char *d;

  if (clipping) {
    if ((x < clip_tx) || (x > clip_bx))
      return;
    if ((y > clip_by) || (y2 < clip_ty))
      return;

    if (y < clip_ty)
      y = clip_ty;
    if (y2 > clip_by)
      y2 = clip_by;
  }

  if(!color_gfx) {
    /* grayscale */
    int n = x&1;

    d = (UInt8*)(vidbuff + DSP_WIDTH/2 * y + x/2);
    while (y2 >= y) {
      *d ^= (graymask[n] & col);
      d += DSP_WIDTH/2;
      y2--;
    }
  } else {
    /* color */
    d = vidbuff + DSP_WIDTH * y + x;
    while (y2 >= y) {
      *d ^= col;
      d += DSP_WIDTH;
      y2--;
    }
  }
}

void
gfx_xor_colour_line (int x1, int y1, int x2, int y2, int line_colour)
{
  int t;

  if (y1 == y2) {
    if (x2 < x1) {
      t = x2;
      x2 = x1;
      x1 = t;
    }
    xor_hline (x1, y1, x2, line_colour);
    return;
  }

  if (x1 == x2) {
    if (y2 < y1) {
      t = y2;
      y2 = y1;
      y1 = t;
    }
    xor_vline (x1, y1, y2, line_colour);
    return;
  }

  DEBUG_PRINTF("drawing xor non-vertical/horizontal line!\n");
}

void
gfx_display_text (int x, int y, int align, char *txt, int col) {
  if(align == ALIGN_CENTER)
    x -= gfx_str_width(txt)/2;
  else if(align == ALIGN_RIGHT)
    x -= gfx_str_width(txt);

  gfx_draw_str (txt, x, y, col);
}

void
gfx_clear_display (void) {
  int i;
  UInt32 *dst = (long *) vidbuff;

  if((!color_gfx) && options.bool[OPTION_INVERT]) {
    for (i = 0; i < DSP_WIDTH * DSP_HEIGHT / (16*pixperbyte); i++) {
      *dst++ = 0; *dst++ = 0; *dst++ = 0; *dst++ = 0;
    }
  } else {
    for (i = 0; i < DSP_WIDTH * DSP_HEIGHT / (16*pixperbyte); i++) {
      *dst++ = -1; *dst++ = -1; *dst++ = -1; *dst++ = -1;
    }
  }
}

void
gfx_clear_area (int tx, int ty, int bx, int by)
{
  int y;

  for (y = ty; y <= by; y++)
    hline (tx, y, bx, GFX_COL_BLACK);
}

void
gfx_draw_rectangle (int tx, int ty, int bx, int by, int col)
{
  gfx_draw_colour_line(tx, ty, tx, by, col);
  gfx_draw_colour_line(bx, ty, bx, by, col);
  gfx_draw_colour_line(tx, ty, bx, ty, col);
  gfx_draw_colour_line(tx, by, bx, by, col);
}

void
gfx_fill_rectangle (int tx, int ty, int bx, int by, int col)
{
  int y;

  for (y = ty; y <= by; y++)
    hline (tx, y, bx, col);
}

#define MAX_CHR (32)		/* max 32 chars per word */
void
gfx_display_pretty_text (int tx, int ty, int bx, int by, int spc, char *txt)
{
  int x = tx, y = ty;
  char word[MAX_CHR + 1], n = 0;

  /* fetch a word and try to print it, wrap if necessary */
  do {
    /* print whole word */
    if ((*txt == 0) || (*txt == ' ')) {

      /* remove multiple spaces */
      if (*txt == ' ')
	while (*(txt + 1) == ' ')
	  txt++;

      word[n] = 0;

      /* do we need to wrap? */
      if (x + gfx_str_width (word) > bx) {
	x = tx;
	y += GFX_CELL_HEIGHT+spc;
      }

      if (y <= by - GFX_CELL_HEIGHT) {
	gfx_draw_str (word, x, y, GFX_COL_WHITE);
	x += gfx_str_width (word) + SPACE_WIDTH + 1;
      }

      n = 0;
    }
    else if (n < MAX_CHR)
      word[n++] = *txt;		/* fetch char */

  } while (*txt++);
}

static int get_button_index(Bool on, int i) {
  int hd, n = i;
  const char disp[]={16,17,17,18};

  if(!on) n = 17;
  else if(!is_docked) {
    switch(i) {
      case 10:
	/* is the ecm module installed? */
	if(!cmdr.ecm) n = 17;
	/* gerhard special feature */
	else if(gerhard) n = 19;	  
	break;
	
      case 11:
	/* is the escape capsule or energy bomb installed? */
	if((!cmdr.escape_pod)&&(!cmdr.energy_bomb)) n = 17;
	break;
	
      case 12:
	/* is there a galactic hyperdrive installed? */
	if(!cmdr.galactic_hyperdrive) n = 17;
	break;
	
      case 13:
	/* is the hyperjump impossible */
	hd = calc_distance_to_planet (docked_planet, hyperspace_planet);
	if ((hd == 0) || (hd > cmdr.fuel)) n = 17;
	break;
	
      case 15:
	/* is a no auto pilot installed */
	if(!cmdr.docking_computer) n = 17;
	break;
    }
  } else {
    if(i>8) n = 17;       // no nav buttons
    if(i<4) n = disp[i]; 
  }
  
  return n;
}

static void draw_button(int pos, int id, Bool mask) {
  UInt8 *s, *d, bit_mask, mask_col;
  int x,y;

  if(color_gfx || !options.bool[OPTION_INVERT])  mask_col = 0xff;
  else                                           mask_col = 0x00;

  /* destination address */
  d = (UInt8*)(vidbase+(160*(160-FUNKEY_HEIGHT)+10*pos)/pixperbyte);
  s = (UInt8*)sprite_data[SPRITE_BUTTONS] + 10*id;

  if(color_gfx) {
    /* draw color button */
    for(y=0;y<10;y++,d+=(160-10),s+=(210-10))
      for(x=0;x<10;x++,d++,s++)
	if((!mask)||(*s != mask_col))
	  *d = *s;
  } else {
    /* draw grayscale button */
    for(y=0;y<10;y++,d+=(80-5),s+=(210-10)) {
      for(x=0;x<5;x++,s+=2) {
	if(mask) bit_mask = ((s[0]!=mask_col)?0xf0:0x00)|
		            ((s[1]!=mask_col)?0x0f:0x00);
	else     bit_mask = 0xff;

	*d++ = (*d & ~bit_mask) | (bit_mask & ((0xf0 & *s) | 
					       (0x0f & *(s+1))));
      }
    }
  }
}

void 
gfx_update_button_map(int marked) { 
  static int marker = -1;
  int i;

#ifdef DEBUG
  if(!gfx_running) return;
#endif

  /* draw buttons */
  for (i = 0; i < 16; i++)
    draw_button(i, get_button_index(marked != NO_BUTTONS, i), false);

  /* redraw marker if required */
  if(marked != SAME_MARKER)
    marker = marked;

  if(marker >= 0)
    draw_button(marker, 20, true);    
}

void
gfx_draw_scanner (void) {
  UInt8 *s = (UInt8*)sprite_data[SPRITE_SCANNER];
  UInt8 *d = (UInt8*)(vidbase+(160*(160-SCANNER_HEIGHT-FUNKEY_HEIGHT))/pixperbyte);
  int i;

#ifdef DEBUG
  if(!gfx_running) return;
#endif

  if(color_gfx) {
    /* draw color scanner */
    for(i=0;i<160*40;i++) 
      *d++ = *s++;
  } else {
    /* draw grayscale scanner */
    for(i=0;i<80*40;i++,s+=2) 
      *d++ = (0xf0 & *(s+0)) | 
             (0x0f & *(s+1));
  }

  scanner_clear_buffer();
  gfx_update_button_map(NO_BUTTONS);
}

void
gfx_set_clip_region (int tx, int ty, int bx, int by)
{
  if (tx == bx)
    clipping = 0;
  else {
    clipping = 1;
    clip_tx = tx;
    clip_ty = ty;
    clip_bx = bx;
    clip_by = by;
  }
}

void 
gfx_draw_sprite(Int16 bmp, int xpos, int ypos) 
{
  UInt8 *g, *s, *d = vidbuff + (DSP_WIDTH*ypos + xpos)/pixperbyte;
  int x,y,w,h;

  /* use already decompressed sprites */
  if(bmp >= SPRITE_DECOMP) {
    if((g = decompress_rsc('sprt', bmp)) == NULL)
	return;

    /* convert grayscale sprites */
    if(!color_gfx) {
      for(s=g,x=0;x<sprite_size[bmp][0]*sprite_size[bmp][1];x++)
	*s++ = graytab[*s];
    }

    s = g;
  } else
    s = sprite_data[bmp];
  
  if(s != NULL) {
    w = sprite_size[bmp][0]/pixperbyte;
    h = sprite_size[bmp][1];

    if(color_gfx) {
      /* draw color sprite */
      for(y=0;y<h;y++,d+=160-w)
	for(x=0;x<w;x++,d++,s++)
	  if(*s != 0xff)
	    *d=*s;
    } else {
      /* draw grayscale sprite */
      for(y=0;y<h;y++,d+=80-w) {
	for(x=0;x<w;x++,d++) {
	  if(*s != 0xff) *d = (*d & 0x0f) | (*s & 0xf0);
	  s++;
	  if(*s != 0xff) *d = (*d & 0xf0) | (*s & 0x0f);	  
	  s++;
	}
      }
    }
  }

  if(bmp >= SPRITE_DECOMP)
    MemPtrFree(g);
}

/* in greyscale mode invert the screen */
void gfx_invert_screen(void) {
  int i, j;
  UInt8 *s;

  /* first invert the screen itself */
  for (s = (UInt8*)vidbase, i = 0; i < 160*160/2; i++, s++) 
    *s = ~*s;

  /* then invert the offscreen buffer */
  for (s = (UInt8*)vidbuff, i = 0; i < DSP_WIDTH*DSP_HEIGHT/2; i++, s++) 
    *s = ~*s;

  /* and finally invert all buffered bitmaps as well */
  for(i=0;i<SPRITE_DECOMP;i++) {
    s = (UInt8*)sprite_data[i];
    for(j=0;j<sprite_size[i][0]*sprite_size[i][1];j++,s++)
      *s = ~*s;
  }
}
