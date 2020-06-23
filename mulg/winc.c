/*
  Winc.c  -  (c) 2000 by Till Harbaum

  color mulg graphics

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
   
*/

#define NON_PORTABLE
#ifdef OS35HDR
#include <PalmOS.h>
#else
#include <Pilot.h>
#include <UI/ScrDriverNew.h>
typedef signed   char  Int8;
typedef unsigned char  UInt8; 
typedef VoidHand MemHandle;
#endif

#include "winx.h"
#include "mulg.h"
#include "tiles.h"
#include "rsc.h"

#include "decompress.h"

// Define auxillary routines dependent on WinXopts settings
#if defined(WINXDRAWCHARS)
#define WINXDRAWBITMAPEX
#endif

#if defined(WINXDRAWBITMAP)
#define WINXDRAWBITMAPEX
#endif

#define dxScreen 160
#define dyScreen 160
#define cbColorScreen (dxScreen*dyScreen)	

#define  WITHIN_MULG
#include "tiles/coltab.h"

typedef struct {
  WinXColor clr;
  WinXColor clrBackground;
  UInt8 *pbDrawBase, *vidbase, *save;
#ifndef USE_SYSTEM_MAP
  RGBColorType *palette;
#endif
} WinXType;

WinXType vWinX = {
  clrBlack,	// clr
  clrWhite,	// clrBackground
};

Boolean on = false;

unsigned long *TileResP;

int WinXSetGreyscale() {
  UInt32  depth;
  Boolean color;
  Err err;

  if (!on) {	     /* only chage if req. */

    /* allocate backup memory */
    vWinX.pbDrawBase  = MemPtrNew(cbColorScreen);

    if (vWinX.pbDrawBase == 0) {
      FrmCustomAlert(alt_err, "Out of memory!",0,0);
      return(1);
    }

    depth = 8;      // 8bit - 256 different combinations with color
    color = true;   

    // try to change to the correct screen mode
    err = WinScreenMode(winScreenModeSet, NULL, NULL, &depth, &color);

    // screen mode supported, continue
    if (err) {
      FrmCustomAlert(alt_err, "This color version of Mulg "
		     "requires a color Palm! Please "
		     "install the greyscale version.",0,0);
      return 1;
    }

    /* get access to tile data */
    if((TileResP = decompress_rsc('tile', 0)) == NULL)
      return 1;

#ifndef USE_SYSTEM_MAP
    {
      RGBColorType *palette;
      int i;
      palette  = (RGBColorType *)MemPtrNew(256 * sizeof(RGBColorType));

      vWinX.palette = (RGBColorType *)MemPtrNew(256 * sizeof(RGBColorType));
      if (vWinX.palette == 0) {
	FrmCustomAlert(alt_err, "Out of memory!",0,0);
	return(1);
      }

      /* save system palette */
      WinPalette(winPaletteGet, 0, 256, vWinX.palette);

      /* set up new color table */
      for(i=0;i<256;i++) {
	palette[i].index = (UInt8)i;
	palette[i].r     = PalmPalette256[i][0];
	palette[i].g     = PalmPalette256[i][1];
	palette[i].b     = PalmPalette256[i][2];
      }
      WinPalette(winPaletteSet, 0, 256, palette);
      MemPtrFree(palette);
    }
#endif

    /* erase buffer, save screen */
    vWinX.vidbase = (void*)(WinGetDrawWindow()->displayAddrV20);
    MemSet(vWinX.pbDrawBase, cbColorScreen, 0);

    on = true;
  }

  return(0);
}

extern void fill_form(void);
extern void draw_sound_tilt_indication(int son, int ton);
extern short tilt_on, sound_on;
#define ACC_ON       0x01

int WinXSetMono() {
  FormType *pfrm;

  if (!on) return 1; /* already mono */
  on = false;
	
  /* restore system palette */
#ifndef USE_SYSTEM_MAP
  WinPalette(winPaletteSet, 0, 256, vWinX.palette);
  MemPtrFree(vWinX.palette);
#endif

  MemPtrFree(vWinX.pbDrawBase);

  decompress_free(TileResP);

  WinScreenMode(winScreenModeSetToDefaults, NULL, NULL, NULL, NULL);

  /* redraw main form */
  pfrm = FrmGetActiveForm();
  FrmDrawForm(pfrm);
  fill_form();  
  draw_sound_tilt_indication(sound_on, tilt_on&ACC_ON);

  return 0;
}

Boolean WinXPreFilterEvent(const EventType *pevt) {
  return 0;
}		

unsigned long colors[]={
  0x00000000,0x01010101,0x01010101,0x01010101
};

unsigned long mask0[]={ 
  0xffffffff,0x00ffffff,0x0000ffff,0x000000ff
};

unsigned long mask1[]={ 
  0x00000000,0xff000000,0xffff0000,0xffffff00
};

void WinXFillHorizLine(short xDest, short dx, short y, WinXColor clr) {
  unsigned long *dst0, *dst1;
  int off0, off1;

  if (!on) return;

  dst0 = ((unsigned long *)vWinX.pbDrawBase) + (xDest/4)    + 40*y;
  dst1 = ((unsigned long *)vWinX.pbDrawBase) + (xDest+dx)/4 + 40*y;

  off0 = xDest&3;
  off1 = (xDest+dx)&3;

  /* whole line within one long word? */
  if(dst0 == dst1) {
    *dst0 = ((*dst0) & ~(mask0[off0]&mask1[off1])) | 
             (colors[clr]&(mask0[off0]&mask1[off1]));
  } else {
    *dst0++ = ((*dst0) & ~mask0[off0]) | (colors[clr] & mask0[off0]);
    while(dst0 != dst1) *dst0++ = colors[clr];
    *dst1 = ((*dst1) & ~mask1[off1]) | (colors[clr] & mask1[off1]);
  }
}

int WinXFillRect(RectangleType *prc, int clr) {
  RectangleType rc;
  int y;
  
  rc = *prc;
  for (y = 0; y < rc.extent.y; y++)
    WinXFillHorizLine(rc.topLeft.x, rc.extent.x, rc.topLeft.y+y, clr);
  return 1;			
}
	
WinXColor WinXSetColor(WinXColor clr) {
  WinXColor clrOld;
	
  clrOld = vWinX.clr;	
  vWinX.clr = clr;
  return clrOld;
}
	
WinXColor WinXSetBackgroundColor(WinXColor clr) {
  WinXColor clrOld;
	
  clrOld = vWinX.clrBackground;	
  vWinX.clrBackground = clr;
  return clrOld;
}

void WinXDrawBitmapEx(UInt8 *pbBits, short xSrc, short ySrc, 
		     short xDest, short yDest, short dxDest, short dyDest, 
		     short rowBytes, Boolean fDrawChar) {
  unsigned char *d, *dst = ((unsigned char *)vWinX.pbDrawBase) + 
                          xDest + 160*yDest;
  unsigned short *src = ((unsigned short*)pbBits) + xSrc/16 + 
                          (rowBytes/2)*ySrc;

  unsigned long mask, b, *s;
  int i, j, soff = xSrc&15;

  if(!on) return;

  for(i=0;i<dyDest;i++) {
    mask = 0x80000000l;

    d = (unsigned char*)dst;
    s = (unsigned long*)src;

    /* get min 16 pixels (enough for character) */
    b = (*s << soff);

    for(j=0;j<dxDest;j++, mask>>=1 )
      if(b & mask)
	*d++ = 0xff;
      else
    	*d++ = 0x00;

    dst += 160;  /* next row */
    src += rowBytes/2;
  }
}

void WinXDrawChars(char *chars, UInt16 len, Int16 x, Int16 y) {
  char *pch;
  char *pchMac;
  FontPtr pfnt;
  UInt16 *plocTbl;
  UInt16 *ploc;
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
    
    ch = *pch;
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
      WinXDrawBitmapEx((UInt8 *)(pfnt+1), xCharImage, 0,  
		       x, y, dxCharImage, pfnt->fRectHeight, 
		       pfnt->rowWords*2, 1);
    }
SkipChar:			
    x += pfci->width;
  }
}	

/* mulg game routines */

unsigned long save_sprite[MAX_SPRITES];
int save_hx[MAX_SPRITES], save_hy[MAX_SPRITES];
void create_sprite(int no, int sprite, int hx, int hy) {
  save_hx[no]=hx; save_hy[no]=hy;
  save_sprite[no]=sprite;
}

void init_sprites(void) {
}

int disp_grey(void) {
  return on;
}

void draw_sprite(int no, int x, int y) {
  int a,b;
  unsigned char *m, *s, *d = (unsigned char *)(vWinX.pbDrawBase+ 
		          (x-save_hx[no]) + 160*(y-save_hy[no]) );

  if (!on) return;

  s = (char*)&TileResP[(save_sprite[no]  )*64];
  m = (char*)&TileResP[(save_sprite[no]+1)*64];

  for(a=0;a<16;a++) {
    for(b=0;b<16;b++)
      if(*m++)
	*d++ = *s++;
      else {
	d++;
	s++;
      }

    d+=144;
  }
}

void draw_tile(int tile_num, int x, int y) {
  int a,b;
  unsigned long *screen=(unsigned long *)vWinX.pbDrawBase;

  if (!on) return;

  tile_num &= 0xff;

  for(a=0;a<16;a++)
    for(b=0;b<4;b++)
      *(screen + y*640 + 4*x+b+ a*40) = TileResP[tile_num*64+a*4+b];
}

/* buffer with level data */
extern unsigned char level_buffer[MAX_HEIGHT][MAX_WIDTH];

void draw_level(int xp, int yp) {
  int x,y,b;
  unsigned long *screen=(unsigned long *)vWinX.pbDrawBase;
  unsigned long *d, *s;
  
  if (!on) return;

  d = screen;
  for(y=0;y<9;y++) {

    for(x=0;x<10;x++) { 

      b = level_buffer[y+yp][x+xp];

      if(b<255-16)
	s = &TileResP[b*64];
      else
	s = (unsigned long*)(custom_tiles[255-b].color);

      *d++=*s++; *d++=*s++; *d++=*s++; *d++=*s++; d+=36;
      *d++=*s++; *d++=*s++; *d++=*s++; *d++=*s++; d+=36;
      *d++=*s++; *d++=*s++; *d++=*s++; *d++=*s++; d+=36;
      *d++=*s++; *d++=*s++; *d++=*s++; *d++=*s++; d+=36;

      *d++=*s++; *d++=*s++; *d++=*s++; *d++=*s++; d+=36;
      *d++=*s++; *d++=*s++; *d++=*s++; *d++=*s++; d+=36;
      *d++=*s++; *d++=*s++; *d++=*s++; *d++=*s++; d+=36;
      *d++=*s++; *d++=*s++; *d++=*s++; *d++=*s++; d+=36;

      *d++=*s++; *d++=*s++; *d++=*s++; *d++=*s++; d+=36;
      *d++=*s++; *d++=*s++; *d++=*s++; *d++=*s++; d+=36;
      *d++=*s++; *d++=*s++; *d++=*s++; *d++=*s++; d+=36;
      *d++=*s++; *d++=*s++; *d++=*s++; *d++=*s++; d+=36;

      *d++=*s++; *d++=*s++; *d++=*s++; *d++=*s++; d+=36;
      *d++=*s++; *d++=*s++; *d++=*s++; *d++=*s++; d+=36;
      *d++=*s++; *d++=*s++; *d++=*s++; *d++=*s++; d+=36;
      *d++=*s++; *d++=*s++; *d++=*s++; *d++=*s++; d+=36;

      d -= (640-4);
    }
    d += 600;
  }
}

void WinFlip(void) {
  MemMove(vWinX.vidbase, vWinX.pbDrawBase, cbColorScreen);
}
