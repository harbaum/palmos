/*---------------------------------------------------------------------
| Win2.c
|
|	Win2 2bpp greyscale replacement for Win* Palm routines
|	To use, replace all calls to Win* palm APIs with WinX*
|
|	See readme.wn2 for usage information
|
|	(c) 1997-1998 ScumbySoft - wesc@ricochet.net
-----------------------------------------------------------------------*/

/* 
   WARNING! This is a modified version for Mulg!
   Please read the file readme.wn2, which is included with this
   source distribution. 

   Search your favorite palm download site for 'Win2' for the
   original version by Wes.

   Till Harbaum
*/

#define NON_PORTABLE
#ifdef OS35HDR
#include <PalmOS.h>
#include <PalmCompatibility.h>
#else
#include <Pilot.h>
typedef signed   char  Int8;
typedef unsigned char  UInt8; 
typedef VoidHand MemHandle;
#endif

#include "winx.h"
#include "mulg.h"
#include "tiles.h"
#include "rsc.h"

// Define auxillary routines dependent on WinXopts settings
#if defined(WINXDRAWCHARS)
#define WINXDRAWBITMAPEX
#endif

#if defined(WINXDRAWBITMAP)
#define WINXDRAWBITMAPEX
#endif


/* defines */
#define VPW   ((unsigned char  *)0xFFFFFA05)
#define LSSA  ((unsigned long  *)0xFFFFFA00)
#define PICF  ((unsigned char  *)0xFFFFFA20)
#define LPXCD ((unsigned char  *)0xFFFFFA25)
#define CKCON ((unsigned char  *)0xFFFFFA27)
#define LBAR  ((unsigned char  *)0xFFFFFA29)
#define FRCM  ((unsigned char  *)0xFFFFFA31)
#define LGPMR ((unsigned short *)0xFFFFFA32)

/* vars */
void _WinXSwitchDisplayModeGrey();
void _WinXSwitchDisplayModeBW();

typedef struct {
  WinXColor clr;
  WinXColor clrBackground;
  UInt8 *pbGreyScreenBase, *base;
  UInt8 *pbMonoScreenBase;
  short fGreyScale;
  Boolean flip;
  UInt8 lpxcd;
} WinXType;

WinXType vWinX = {
  clrBlack,	// clr
  clrWhite,	// clrBackground
};

#define cbppGrey 2		// bits per pixel grey
#define cppbuGrey 8		// pixels per BltUnit grey
#define cbpbu 16			// bits per BltUnit
#define shppbuGrey 2		// shift count pixels->BltUnit
#define buAllOnes 0xFFFF
#define BltUnit UInt16

#define dxScreen 160
#define dyScreen 160
#define cbGreyScreen (dxScreen*cbppGrey/8*dyScreen)	
#define MonoBToBltUnit(b) ((mpNibbleGreyUInt8[(b)>>4]<<8)|mpNibbleGreyUInt8[b&0xf])
#define WriteMask(buDest,buSrc,buMask)  (buDest=((buDest)&~(buMask))|((buSrc)&(buMask)))
	
BltUnit mpclrbuRep[] = { 0x0000, 0x5555, 0xAAAA, 0xFFFF };	

int _WinXFAllocScreen() {
  vWinX.flip = true;
  vWinX.base = vWinX.pbGreyScreenBase = MemPtrNew(2*cbGreyScreen);
  return vWinX.base != NULL;
}

void _WinXFreeScreen() {
  if (vWinX.base != NULL) {
    MemPtrFree(vWinX.base);
    vWinX.base = NULL;
  }
}
	
MemHandle      TileResH;
unsigned long *TileResP;

/*-----------------------------------------------------------------------------
|	WinXSetGreyscale
|	
|		Switches Pilots display into greyscale mode
|	Note, this allocates memory in a variety of ways.  See WinXopts.h
|	for more information
|	
|	Returns: 0 if successful, nonzero if not
-------------------------------------------------------wesc@ricochet.net-----*/
int WinXSetGreyscale() {

  if (vWinX.fGreyScale==0) {	/* only chage if req. */

    if (!_WinXFAllocScreen()) {
      FrmCustomAlert(alt_err, "Out of memory!",0,0);
      return(1);
    }
	
    vWinX.pbMonoScreenBase=(char*)*LSSA;

    MemSet(vWinX.base, 2*cbGreyScreen, 0);

    _WinXSwitchDisplayModeGrey();
    
    *LSSA=(long)(vWinX.base+cbGreyScreen);

    vWinX.fGreyScale=1;
  }
  
  /* get access to tile data */
  TileResH = (MemHandle)DmGetResource('tile', 0);
  TileResP = MemHandleLock((MemHandle)TileResH);

  return(0);
}

/*-----------------------------------------------------------------------------
|	WinXSetMono
|	
|		Switches back to mono mode.
|
|	Returns: 0 if successful, nonzero if not
-------------------------------------------------------wesc@ricochet.net-----*/
extern void fill_form(void);
extern void draw_sound_tilt_indication(int son, int ton);
extern short tilt_on, sound_on;
#define ACC_ON       0x01

int WinXSetMono() {
  FormType *pfrm;
  unsigned long depth, newDepth;
  
  if (vWinX.fGreyScale!=1)
    return 1;	/* already mono */
	
  MemPtrUnlock(TileResP);
  DmReleaseResource((MemHandle)TileResH );

  *LSSA=(long)vWinX.pbMonoScreenBase;
  _WinXSwitchDisplayModeBW();
	
  _WinXFreeScreen();
  vWinX.fGreyScale=0;

  if(has_newscr) {
    /* return to default video mode */
    ScrDisplayMode(scrDisplayModeGetSupportedDepths, NULL, NULL, &depth, NULL);
	
    if(depth & ((1<<1)|(1<<3))) {
      
      /* is 4bpp or 2bpp supported? */
      if (depth & (1<<3))     newDepth = 4;
      else if(depth & (1<<1)) newDepth = 2;
      
      ScrDisplayMode(scrDisplayModeSet, NULL, NULL, &newDepth, NULL);
    } 
  }

  /* redraw main form */
  pfrm = FrmGetActiveForm();
  FrmDrawForm(pfrm);

  fill_form();
  draw_sound_tilt_indication(sound_on, tilt_on&ACC_ON);

  return 0;
}

/*-----------------------------------------------------------------------------
|	WinXPreFilterEvent
|	
|		Helper -- put in your main event loop to handle common cases
|	where the pilot should be put back into mono mode
|	
|	Arguments:
|		const EventType *pevt
|	
|	Returns: true if event is processed
-------------------------------------------------------wesc@ricochet.net-----*/
Boolean WinXPreFilterEvent(const EventType *pevt) {
  if (pevt->eType == keyDownEvent && (pevt->data.keyDown.modifiers & 
				      commandKeyMask)) {
      switch (pevt->data.keyDown.chr) {
      case alarmChr:                       // back to bw for alarm events
	WinXSetMono();
	break;
      case lowBatteryChr:                  // dito for low battery warnings
      case keyboardChr:
      case graffitiReferenceChr:           // popup the Graffiti reference
      case keyboardAlphaChr:               // popup the keyboard in alpha mode
      case keyboardNumericChr:             // popup the keyboard in number mode
	return 1;	                   // eat, don't switch to mono
      case hardPowerChr:
      case backlightChr:	
	// don't switch to mono, but don't eat either
	break;
      }
  }
  return 0;
}		

const unsigned char mpNibbleGreyUInt8[]={0x00,0x03,0x0c,0x0f,
					0x30,0x33,0x3c,0x3f,
					0xc0,0xc3,0xcc,0xcf,
					0xf0,0xf3,0xfc,0xff};

void _WinXSwitchDisplayModeGrey() {

  /* set pixel clock devider to full speed */
  vWinX.lpxcd = *LPXCD;
  *LPXCD = 0;

  *CKCON=*CKCON & 0x7F;	/* display off*/
	
  /*virtual page width now 40 bytes (160 pixels)*/
  *VPW=20;
  *PICF=*PICF | 0x01; /*switch to grayscale mode*/
  *LBAR=20; /*line buffer now 40 bytes*/
  
  /*register to control grayscale pixel oscillations*/
  *FRCM=0xB9;
  
  /*let the LCD get to a 2 new frames (40ms delay) */
  SysTaskDelay(4);
  
  /*switch LCD back on */
  *CKCON=*CKCON | 0x80;
  
  *LGPMR = 0x3074;
}

void _WinXSwitchDisplayModeBW() {
  //switch off LCD update temporarily
  *CKCON=*CKCON & 0x7F;
  
  //virtual page width now 20 bytes (160 pixels)
  *VPW=10;
  *PICF=*PICF & 0xFE; //switch to black and white mode
  *LBAR=10; // line buffer now 20 bytes
  
  //let the LCD get to a new frame (20ms delay)
  SysTaskDelay(4);
  
  //switch LCD back on in new mode
  *CKCON=*CKCON | 0x80;

  // restore original pixel clock devider state
  *LPXCD = vWinX.lpxcd;
}

#if defined(WINXDRAWBITMAPEX)
int WinXDrawBitmapEx(UInt8 *pbBits, short xSrc, short ySrc, 
		     short xDest, short yDest, short dxDest, short dyDest, 
		     short rowBytes, Boolean fDrawChar) {
  int xDestRight;
  int yDestBot;
  BltUnit *pbuBaseDest;
  BltUnit *pbuBaseSrc;
  int yDestCur;
  int xDestCur;
  int cPelsLeft;
  BltUnit  buMaskLeft;
  int shSrc;	
  int cPelsRight;
  BltUnit buMaskRight;
  BltUnit buClrMask;
  BltUnit buClrMaskBackground;
  BltUnit rgbuSrc[8];	// handles up to 64 pixel wide font...otta be enough
  int dxSrcTranslate;
  int   cPelsNextSrc;
  
  if(vWinX.base == 0) return 0;

  shSrc = ((xSrc + cppbuGrey - (xDest % cppbuGrey)) % cppbuGrey) * cbppGrey;

  pbuBaseDest = ((BltUnit *)vWinX.pbGreyScreenBase) +
                (dxScreen/cppbuGrey)*yDest + xDest/(cppbuGrey);

  xDestRight = xDest + dxDest;	// pbm->width;
  yDestBot   = yDest + dyDest;	// pbm->height;  

  if (fDrawChar) {
    dxSrcTranslate = (xSrc%8+dxDest+7)/8;
    // get mask for current color
    buClrMask = mpclrbuRep[vWinX.clr];
    buClrMaskBackground = mpclrbuRep[vWinX.clrBackground];
    pbuBaseSrc = (BltUnit *) (pbBits + rowBytes*ySrc + xSrc/8);
  } else
    pbuBaseSrc = (BltUnit *)(pbBits);
   
  // precompute masks and stuff
  cPelsLeft = 0;		
  if(xDest % cppbuGrey) {
    int dwRightShift;
    
    cPelsLeft = cppbuGrey - (xDest % cppbuGrey);
    dwRightShift = (xDest * cbppGrey) % cbpbu;
    buMaskLeft = buAllOnes >> dwRightShift;
    if(xDest + cPelsLeft > xDestRight) {
      int cPelsRightDontTouch;
      
      cPelsRightDontTouch = xDest + cPelsLeft - xDestRight;
      buMaskLeft &= buAllOnes << (cPelsRightDontTouch * cbppGrey);
      cPelsLeft -= cPelsRightDontTouch;
    }
    cPelsNextSrc = cPelsLeft - (cppbuGrey - (xSrc % cppbuGrey));
  }

  // precompute right mask		
  if ((cPelsRight = (xDestRight % cppbuGrey))) {
    buMaskRight = buAllOnes << (cppbuGrey - cPelsRight) * cbppGrey;
  }
  
  for(yDestCur = yDest; yDestCur < yDestBot; yDestCur++) {
    BltUnit  *pbuDest;
    BltUnit  *pbuSrc;
    BltUnit  buSrc;				  
    BltUnit  buSrcData, buSrcNext;  
    
    pbuDest = pbuBaseDest;		
    xDestCur = xDest;
    if (fDrawChar) {
      UInt8 *pbSrcT;
      int x;
      
      pbSrcT = (UInt8 *)pbuBaseSrc;
      for (x = 0; x < dxSrcTranslate; x++) {
	BltUnit bu;
	
	bu  = MonoBToBltUnit(*pbSrcT);
	// make all 1 bits be current color
	// make all 0 bits be current bkgnd color
	bu = (bu & buClrMask) | ((~bu) & buClrMaskBackground);
	rgbuSrc[x] = bu;
	pbSrcT++;
      }
      pbuSrc = rgbuSrc;
    }
    else
      pbuSrc = pbuBaseSrc;
        

    buSrc = *pbuSrc;
    
    //  unaligned bits on left side
    if (cPelsLeft != 0) {
      xDestCur += cPelsLeft;
      
      // do we need another Bltunit?
      if (cPelsNextSrc > 0 ||			   
	  (cPelsNextSrc == 0 && xDestCur < xDestRight)) { 
	buSrcNext = *(++pbuSrc);
	buSrcData = buSrc << shSrc;
	buSrcData |= shSrc ? (buSrcNext >> (cbpbu - shSrc)) : 0;
	buSrc = buSrcNext;
      } 
      else if((((xDest * cbppGrey) % cbpbu) + shSrc) < cbpbu)
	buSrcData = buSrc << shSrc;
      else
	buSrcData = buSrc >> (cbpbu - shSrc);
      WriteMask(*pbuDest, buSrcData, buMaskLeft);
      pbuDest++;
    }

    // src aligned
    if(shSrc == 0) {	   
      if(xDestCur + cppbuGrey <= xDestRight) {
	do {
	  xDestCur = xDestCur + cppbuGrey;
	  
	  *pbuDest++ = *pbuSrc++;
	} while(xDestCur + cppbuGrey <= xDestRight);
	if(xDestCur < xDestRight)
	  buSrc = *pbuSrc;
      }
    }
    else				// src not aligned
      while(xDestCur + cppbuGrey <= xDestRight) {
	xDestCur = xDestCur + cppbuGrey;
	
	buSrcNext = *(++pbuSrc);
	buSrcData = (buSrc << shSrc) | (buSrcNext >> (cbpbu - shSrc));
	*pbuDest = buSrcData;
	pbuDest++;
	buSrc = buSrcNext;
      }
    //  unaligned bits on right side
    if(xDestCur < xDestRight) {
      if(cPelsRight * cbppGrey > cbpbu - shSrc) {
	buSrcNext = *(++pbuSrc);
	buSrcData = (buSrc << shSrc) | (buSrcNext >> (cbpbu - shSrc));
      } else
	buSrcData = buSrc << shSrc;
      
      WriteMask(*pbuDest, buSrcData, buMaskRight);
    }

    pbuBaseDest += (dxScreen/cppbuGrey);
    pbuBaseSrc += rowBytes/(cbpbu/8);
  }

  return 1;
}
#endif // WINXDRAWBITMAPEX

void WinXFillHorizLine(short xDest, short dx, short y, WinXColor clr) {
  BltUnit buSrc;
  BltUnit buMskData;			 
  int xDestCur;
  int xDestRight;
  BltUnit *pbuDest;

  xDestRight = xDest+dx;
  buSrc	= mpclrbuRep[clr & 0x3];
  pbuDest = ((BltUnit *)vWinX.pbGreyScreenBase)+
            (dxScreen/cppbuGrey)*y + xDest/(cppbuGrey);
  xDestCur = xDest;

  if (xDest % cppbuGrey) {
    int cPels;
    int cPelsRightShift;
    BltUnit buMask;
		
    cPels = cppbuGrey-(xDest%cppbuGrey);
    cPelsRightShift = (xDest*cbppGrey)%cbpbu;
    buMask = buAllOnes >> cPelsRightShift;
    if(xDest + cPels > xDestRight) {
      BltUnit cPelsRightDontTouch;
			 
      cPelsRightDontTouch = xDest + cPels - xDestRight;
      buMask &= buAllOnes << (cPelsRightDontTouch * cbppGrey);
      cPels -= cPelsRightDontTouch;
    }
    xDestCur += cPels;
    WriteMask(*pbuDest,buSrc,buMask);
    pbuDest++;
  }

  if(xDestCur + cppbuGrey <= xDestRight) {
    do {
      xDestCur = xDestCur + cppbuGrey;
      
      *pbuDest++ = buSrc;
    } while(xDestCur + cppbuGrey <= xDestRight);
  }

  // now do right partial bltunit if necessary
  if(xDestCur < xDestRight) {
    int cPels;
		  
    cPels = xDestRight - xDestCur;
    buMskData = buAllOnes << (cppbuGrey - cPels) * cbppGrey;
    WriteMask(*pbuDest,buSrc,buMskData);
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
	
/*-----------------------------------------------------------------------------
|	WinXSetColor
|	
|		Sets the current drawing color.  All Fill, Line and Text
|	operations will use this color
|	
|	Arguments:
|		WinXColor clr
|	
|	Returns: old color
-------------------------------------------------------wesc@ricochet.net-----*/
WinXColor WinXSetColor(WinXColor clr)
	{
	WinXColor clrOld;
	
	clrOld = vWinX.clr;	
	vWinX.clr = clr;
	return clrOld;
	}
	
/*-----------------------------------------------------------------------------
|	WinXSetBackgroundColor
|	
|		Sets the background color for WinXDrawChars
|	
|	Arguments:
|		WinXColor clr
|	
|	Returns: old color
-------------------------------------------------------wesc@ricochet.net-----*/
WinXColor WinXSetBackgroundColor(WinXColor clr)
	{
	WinXColor clrOld;
	
	clrOld = vWinX.clrBackground;	
	vWinX.clrBackground = clr;
	return clrOld;
	}
	

#if defined(WINXDRAWCHARS)
/*-----------------------------------------------------------------------------
|	WinXDrawChars
|	
|		Draws chars on the screen.  Uses current color and background color
|	Note that Underline mode is not supported!
|	
|	Arguments:
|		CharPtr chars
|		UInt16 len
|		SUInt16 x
|		SUInt16 y
-------------------------------------------------------wesc@ricochet.net-----*/
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
#endif // WINXDRAWCHARS	

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
  return(vWinX.base!=0);
}

void draw_sprite(int no, int x, int y) {
  int a,n,m;
  unsigned long *save=(unsigned long *)(vWinX.pbGreyScreenBase+ 
                    4*((x-save_hx[no])/16) + 40*(y-save_hy[no]) );

  if (vWinX.pbGreyScreenBase == 0) return;

  /* draw sprite (mask, shift) */
  n=2*((x-save_hx[no])&0x0f);
  m=32-n;

  for(a=0;a<16;a++) {
    save[a*10]   = (save[a*10]   & 
		    ~(TileResP[(save_sprite[no]+1)*16+a]>>n)) | 
                     (TileResP[(save_sprite[no]  )*16+a]>>n);

    save[a*10+1] = (save[a*10+1] & 
		    ~(TileResP[(save_sprite[no]+1)*16+a]<<m)) | 
                     (TileResP[(save_sprite[no]  )*16+a]<<m);
  }
}

void draw_tile(int tile_num, int x, int y) {
  int b,i;
  unsigned long *screen=(unsigned long *)vWinX.pbGreyScreenBase;
  unsigned long *d, *s;

  if (vWinX.pbGreyScreenBase == 0) return;

  tile_num &= 0xff;

  d = screen + y*160 + x;
  s = &TileResP[tile_num*16];

  for(b=0;b<16;b++) {
    *d = *s++; d += 10;
  }
}

/* buffer with level data */
extern unsigned char level_buffer[MAX_HEIGHT][MAX_WIDTH];

void draw_level(int xp, int yp) {
  int x,y,b;
  unsigned long *screen=(unsigned long *)vWinX.pbGreyScreenBase;
  unsigned long *d, *s;
  
  if (vWinX.pbGreyScreenBase == 0) return;

  d = screen;
  for(y=0;y<9;y++) {

    for(x=0;x<10;x++) { 
      b = level_buffer[y+yp][x+xp];

      if(b<255-16)
	s = &TileResP[b*16];
      else
	s = (unsigned long*)(custom_tiles[255-b].gray2);

      *d=*s++; d+=10; *d=*s++; d+=10; *d=*s++; d+=10; *d=*s++; d+=10;
      *d=*s++; d+=10; *d=*s++; d+=10; *d=*s++; d+=10; *d=*s++; d+=10;
      *d=*s++; d+=10; *d=*s++; d+=10; *d=*s++; d+=10; *d=*s++; d+=10;
      *d=*s++; d+=10; *d=*s++; d+=10; *d=*s++; d+=10; *d=*s++; d+=10;

      d -= 159;
    }
    d += 150;
  }
}

void WinFlip(void) {
  if(vWinX.flip) {
    vWinX.pbGreyScreenBase = vWinX.base;
    *LSSA = (long)(vWinX.base+cbGreyScreen);
  } else {
    vWinX.pbGreyScreenBase = vWinX.base+cbGreyScreen;
    *LSSA = (long)vWinX.base;
  }
  vWinX.flip = !vWinX.flip;
}
