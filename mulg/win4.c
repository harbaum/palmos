/*
   Win4.c  -  (c) 1999 by Till Harbaum

   four bit per pixel (16 greyscale) mulg graphics

   some major improvements contributed by Daniel Morais <pilotto@online.fr>
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

typedef	signed char	SBYTE;			// signed   byte      (  8 bits )
typedef	unsigned char	UBYTE;			// unsigned byte      (  8 bits )

typedef	signed short	SWORD;			// signed   word      ( 16 bits )
typedef	unsigned short	UWORD;  		// unsigned word      ( 16 bits )

typedef	signed long	SLONG;			// signed   long word ( 32 bits )
typedef	unsigned long	ULONG;			// unsigned long word ( 32 bits )

//===========================================================================
//	DEFINES
//===========================================================================
// The following informations was found using :
// - The Motorola MC68EZ328 User's Manual ( http://www.motorola.com/SPS/WIRELESS/pda/index.html )
// - The Article "Hacking the pilot : Bypassing the PalmOS" by Edward Keyes ( Handeld Systems archives )
// - Some source codes from Till Harbaum ( http://bodotill.suburbia.com.au )

#define LSSA  *((void **)0xFFFFFA00) // LCD Screen Starting Address Register
#define LVPW  *((UBYTE *)0xFFFFFA05) // LCD Virtual Page Width Register
#define LXMAX *((UWORD *)0xFFFFFA08) // LCD Screen Width Register
#define LYMAX *((UWORD *)0xFFFFFA0A) // LCD Screen Height Register
#define LCXP  *((UWORD *)0xFFFFFA18) // LCD Cursor X Position Register
#define LCYP  *((UWORD *)0xFFFFFA1A) // LCD Cursor Y Position Register
#define LCWCH *((UWORD *)0xFFFFFA1C) // LCD Cursor Width and Height Register
#define LBLCK *((UBYTE *)0xFFFFFA1F) // LCD Blink Control Register
#define LPICF *((UBYTE *)0xFFFFFA20) // LCD Panel Interface Configuration Register
#define LPOLCF *((UBYTE *)0xFFFFFA21)// LCD Polarity Configuration Register
#define LACDRC *((UBYTE *)0xFFFFFA23)// LACD Rate Control Register
#define LPXCD *((UBYTE *)0xFFFFFA25) // LCD Pixel Clock Divider Register
#define LCKCON *((UBYTE *)0xFFFFFA27)// LCD Clocking Control Register
#define LRRA  *((UBYTE *)0xFFFFFA29) // LCD Refresh Rate Adjustment Register
#define LPOSR *((UBYTE *)0xFFFFFA2D) // LCD Panning Offset Register
#define LFRCM *((UBYTE *)0xFFFFFA31) // LCD Frame Rate Control Modulation Register
#define LGPMR *(UBYTE *)(0xFFFFFA33) // LCD Gray Palette Mapping Register
#define LPWMR *(UWORD *)(0xFFFFFA36) // PWM Contrast Control Register

#define SCREEN_WIDTH  160
#define SCREEN_HEIGHT 160

// Define auxillary routines dependent on WinXopts settings
#if defined(WINXDRAWCHARS)
#define WINXDRAWBITMAPEX
#endif

#if defined(WINXDRAWBITMAP)
#define WINXDRAWBITMAPEX
#endif

typedef struct {
  WinXColor clr;
  WinXColor clrBackground;
  UInt8 *GreyScreenBase;
  UInt8 *DisplayBase;
  short fGreyScale;
} WinXType;

WinXType vWinX = {
  clrBlack,	// clr
  clrWhite,	// clrBackground
  NULL, NULL, 0
};

void *OldLSSA=NULL;
UBYTE OldLVPW;
UWORD OldLXMAX;
UWORD OldLYMAX;
UWORD OldLCXP;
UWORD OldLCYP;
UWORD OldLCWCH;
UBYTE OldLBLCK;
UBYTE OldLPICF;
UBYTE OldLPOLCF;
UBYTE OldLACDRC;
UBYTE OldLPXCD;
UBYTE OldLCKCON;
UBYTE OldLRRA;
UBYTE OldLPOSR;
UBYTE OldLFRCM;
UBYTE OldLGPMR;
UWORD OldLPWMR;

#define cbppGrey 4		// bits per pixel grey
#define cppbuGrey 8		// pixels per BltUnit grey
#define cbpbu 16		// bits per BltUnit
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
  vWinX.GreyScreenBase = MemPtrNew(160*160/2);
  return vWinX.GreyScreenBase != NULL;
}

void _WinXFreeScreen() {
  if (vWinX.GreyScreenBase != NULL) {
    MemPtrFree(vWinX.GreyScreenBase);
    vWinX.GreyScreenBase = NULL;
  }
}
	
MemHandle      TileResH;
unsigned long *TileResP;

static Boolean VideoDirect(void) {
  unsigned long id, chip;
  Err err;

  /* CPU type detection code from Geoff Richmond */
  err = FtrGet(sysFtrCreator, sysFtrNumProcessorID, &id);
  if (err) return false;
  chip = id & sysFtrNumProcessorMask;
  if(chip != sysFtrNumProcessorEZ) return false;

  /* set new video base address */
  if((vWinX.DisplayBase = MemPtrNew(160*160/2)) == NULL) {
    FrmCustomAlert(alt_err, "Out of memory allocating video buffer.",0,0);
    return false;
  }

  // Save ALL the registers, to be able to restore them
  OldLSSA = LSSA;
  OldLVPW = LVPW;
  OldLXMAX = LXMAX;
  OldLYMAX = LYMAX;
  OldLCXP = LCXP;
  OldLCYP = LCYP;
  OldLCWCH = LCWCH;
  OldLBLCK = LBLCK;
  OldLPICF = LPICF;
  OldLPOLCF = LPOLCF;
  OldLACDRC = LACDRC;
  OldLPXCD = LPXCD;
  OldLCKCON = LCKCON & 0x7F;
  OldLRRA = LRRA;
  OldLPOSR = LPOSR;
  OldLFRCM = LFRCM;
  OldLGPMR = LGPMR;
  OldLPWMR = LPWMR;
  
  // First, put the display off !
  LCKCON &= 0x7F;
  
  // Set the address of the visible screen  
  LSSA = vWinX.DisplayBase;
  
// LCD Pixel Clock Divider Register : use PIX clock directly, bypassing the divider circuit
  LPXCD = 0;  
  
  // LCD Virtual Page Width Register : 40 ( Till use 48, for clipping purpose ?? )
  LVPW = SCREEN_WIDTH/4;  

  // LCD Panel Interface Configuration Register : 4 bits panel bus, 4 bits grayscale
  LPICF = 0x08 | 0x02;    
  
  // Special Warning !!!
  // The register at 0xFFFFFA29 was named LLBAR on the MC68328 and is now LRRA on the MC68EZ328
  // They have a completely different mapping and meaning
  // So I think that Till did a mistake, here, maybe infuenced by Edward initialisation code for 2 Bpp
  // So, don't put 40 here, but the correct value !
  
  LRRA = 254;             // LCD Refresh Rate Adjustment Register ( taken from results given by PalmOS 3.5 )
  LFRCM = 185;            // LCD Frame Rate Control Modulation Register ( also taken from PalmOS3.5 )
  
  // Now, we will initialize the other registers, which was not done by Till nor Edward
  // Don't know if it is necessary, but it shouldn't hurt ! :-)
  
  LXMAX = SCREEN_WIDTH;   // LCD Screen Width Register ( should be a multiple of 16 )
  LYMAX = SCREEN_HEIGHT-1;// LCD Screen Height Register ( don't forget the minus 1 !! )

  LCXP = 0;               // LCD Cursor X and Y Position Register
  LCYP = 0;

  LCWCH = (4L << 8) | 4;  // Hardware cursor width & height = 4 ( taken from results given by PalmOS3.5 )

  LBLCK = (1 << 7) | 63;  // Blink is enabled, Blink divisor = 63 ( taken from results given by PalmOS3.5 )
 
  LGPMR = (8 << 4) | 4;   // Two intermediate gray scale shading ( taken from results given by PalmOS3.5 )

  // The following registers are all initialized with 0

  LPOLCF = 0;             // LCD Polarity Configuration Register
  LACDRC = 0;             // LACD Rate Control Register
  LPOSR = 0;              // LCD Panning Offset Register
	
  // Don't modify the next register, else you will see nothing on screen !!
  //LPWMR = 0;              // PWM Contrast Control Register
  
  // Wait some frames time, before re enabling the display
  // 1 tick should be 1/100e of second, but use SysTicksPerSecond() to be sure
  // The delay should be around 20 ms, which correspond to a refresh of 50 Hz...
  
  SysTaskDelay( SysTicksPerSecond() / 50 );
	
  // Enable the display
  LCKCON = 0x80 | 6;   // Display enable, 16 bits bus width, 6 clocks wait state ( Taken from OS 3.5 )
  			  

  return true;
}

/*-----------------------------------------------------------------------------
|	WinXSetGreyscale
|	
|		Switches Pilots display into greyscale mode
|	
|	Returns: 0 if successful, nonzero if not
-------------------------------------------------------wesc@ricochet.net-----*/
int WinXSetGreyscale() {
  int i,j;
  int   wPalmOSVerMajor;
  long  dwVer;
  unsigned long newDepth, depth;

  if (vWinX.fGreyScale==0) {	/* only chage if req. */

    /* this game requires at least OS3.0 */
    FtrGet(sysFtrCreator, sysFtrNumROMVersion, &dwVer);
    wPalmOSVerMajor = sysGetROMVerMajor(dwVer);
    if (wPalmOSVerMajor < 3) {
      FrmCustomAlert(alt_err,"This version of Mulg requires at least PalmOS 3.0.",0,0);
      return (1);
    }

    newDepth = 4;
    if(ScrDisplayMode(scrDisplayModeSet, NULL, NULL, &newDepth, NULL)) {
      /* try to access hardware directly */
      if(!VideoDirect()) {
	FrmCustomAlert(alt_err,"Grayscale video not supported on this machine.",0,0);
	return (1);
      }
    }

    /* if not direct hardware access get video buffer address from OS */
    if(OldLSSA == NULL)
      vWinX.DisplayBase = WinGetDrawWindow()->displayAddrV20;
    
    if (!_WinXFAllocScreen()) {
      FrmCustomAlert(alt_err, "Out of memory!",0,0);
      return(1);
    }
    
    /* erase offscreen buffer */
    MemSet(vWinX.GreyScreenBase, cbGreyScreen, 0);
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
  DWord depth, newDepth;
  
  if (vWinX.fGreyScale!=1) return 1; /* already mono */

  MemPtrUnlock(TileResP);
  DmReleaseResource((MemHandle)TileResH );

  if(OldLSSA != NULL) {

    MemPtrFree(vWinX.DisplayBase);
    vWinX.DisplayBase = NULL;

    // First, put the display off !

    LCKCON &= 0x7F;

    // Restore the address of the initial visible screen
    
    LSSA = OldLSSA;
    
    LVPW = OldLVPW;
    LXMAX = OldLXMAX;
    LYMAX = OldLYMAX;
    LCXP = OldLCXP;
    LCYP = OldLCYP;
    LCWCH = OldLCWCH;
    LBLCK = OldLBLCK;
    LPICF = OldLPICF;
    LPOLCF = OldLPOLCF;
    LACDRC = OldLACDRC;
    LPXCD = OldLPXCD;
    LRRA = OldLRRA;
    LPOSR = OldLPOSR;
    LFRCM = OldLFRCM;
    LGPMR = OldLGPMR;
    LPWMR = OldLPWMR;

    // Wait some frames time, before re enabling the display
    // 1 tick should be 1/100e of second, but use SysTicksPerSecond() to be sure
    // The delay should be around 20 ms, which correspond to a refresh of 50 Hz...
    
    SysTaskDelay( SysTicksPerSecond() / 50 );
    
    // Enable the display
    
    LCKCON = OldLCKCON | 0x80;
  } else {
    RectangleType rect={{0,0},{160,160}};

    /* OS 3.3 requires the screen to be erased */
    WinEraseRectangle(&rect, 0);

    /* return to default video mode */
    ScrDisplayMode(scrDisplayModeGetSupportedDepths, NULL, NULL, &depth, NULL);
	
    if(depth & ((1<<1)|(1<<3)|(1<<7))) {
	  
      /* is 4bpp or 2bpp supported? */
      if (depth & (1<<7))     newDepth = 8;
      else if(depth & (1<<3)) newDepth = 4;
      else if(depth & (1<<1)) newDepth = 2;
	
      ScrDisplayMode(scrDisplayModeSet, NULL, NULL, &newDepth, NULL);
    }
    vWinX.DisplayBase = NULL;
  }

  _WinXFreeScreen();
  vWinX.fGreyScale=0;

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

  /* only filter when using direct hardware access */
  if(OldLSSA != NULL) {

    if (pevt->eType == keyDownEvent && (pevt->data.keyDown.modifiers & 
				      commandKeyMask)) {
      switch (pevt->data.keyDown.chr) {
      case alarmChr:                       // back to bw for alarm events
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
  }
  return 0;
}		

const unsigned char mpNibbleGreyUInt8[]={
  0x00,0x03,0x0c,0x0f,0x30,0x33,0x3c,0x3f,
  0xc0,0xc3,0xcc,0xcf,0xf0,0xf3,0xfc,0xff};

unsigned long colors[]={
  0x00000000,0x55555555,0xaaaaaaaa,0xffffffff
};

unsigned long mask0[]={ 
  0xffffffff,0x0fffffff,0x00ffffff,0x000fffff,
  0x0000ffff,0x00000fff,0x000000ff,0x0000000f
};

unsigned long mask1[]={ 
  0x00000000,0xf0000000,0xff000000,0xfff00000,
  0xffff0000,0xfffff000,0xffffff00,0xfffffff0
};

unsigned short tr[]={
  0x0000, 0x000f, 0x00f0, 0x00ff, 0x0f00, 0x0f0f, 0x0ff0, 0x0fff,
  0xf000, 0xf00f, 0xf0f0, 0xf0ff, 0xff00, 0xff0f, 0xfff0, 0xffff
};

void WinXDrawBitmapEx(UInt8 *pbBits, short xSrc, short ySrc, 
		     short xDest, short yDest, short dxDest, short dyDest, 
		     short rowBytes, Boolean fDrawChar) {
  unsigned short *dst = ((unsigned short *)vWinX.GreyScreenBase) + 
    xDest/4 + 40*yDest;
  unsigned short *src = ((unsigned short*)pbBits) + xSrc/16 + 
    (rowBytes/2)*ySrc;
  unsigned long *s, *d, mask, b, smask;

  int i, doff = (xDest&3)<<2, soff = xSrc&15;

  if(vWinX.GreyScreenBase == 0) return;

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

void WinXFillHorizLine(short xDest, short dx, short y, WinXColor clr) {
  unsigned long *dst0, *dst1;
  int off0, off1;

  dst0 = ((unsigned long *)vWinX.GreyScreenBase) + (xDest/8)    + 20*y;
  dst1 = ((unsigned long *)vWinX.GreyScreenBase) + (xDest+dx)/8 + 20*y;

  off0 = xDest&7;
  off1 = (xDest+dx)&7;

  if (vWinX.GreyScreenBase == 0) return;

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
WinXColor WinXSetColor(WinXColor clr) {
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
WinXColor WinXSetBackgroundColor(WinXColor clr) {
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
  return(vWinX.GreyScreenBase!=0);
}

void draw_sprite(int no, int x, int y) {
  int a,n,m;
  unsigned long *save = (unsigned long *)(vWinX.GreyScreenBase+ 
		        4*((x-save_hx[no])/8) + 80*(y-save_hy[no]) );

  if (vWinX.GreyScreenBase == 0) return;

  /* draw sprite (mask, shift) */
  n=4*((x-save_hx[no])&0x07);
  m=32-n;

  for(a=0;a<16;a++) {

    save[a*20]   = (save[a*20]   & 
		    ~(TileResP[(save_sprite[no]+1)*32+2*a+0]>>n)) | 
                     (TileResP[(save_sprite[no]  )*32+2*a+0]>>n);

    save[a*20+1] = (((save[a*20+1] & 
		     ~(TileResP[(save_sprite[no]+1)*32+2*a+1]>>n)) | 
		      (TileResP[(save_sprite[no]  )*32+2*a+1]>>n)) &
		     ~(TileResP[(save_sprite[no]+1)*32+2*a+0]<<m)) | 
		      (TileResP[(save_sprite[no]  )*32+2*a+0]<<m);

    save[a*20+2] = (save[a*20+2] & 
		    ~(TileResP[(save_sprite[no]+1)*32+2*a+1]<<m)) | 
                     (TileResP[(save_sprite[no]  )*32+2*a+1]<<m);
  }
}

void draw_tile(int tile_num, int x, int y) {
  int a,i;
  unsigned long *screen=(unsigned long *)vWinX.GreyScreenBase;

  if (vWinX.GreyScreenBase == 0) return;

  tile_num &= 0xff;

  for(a=0;a<16;a++) {
    *(screen + y*320 + 2*x+   a*20)=TileResP[tile_num*32+2*a  ];
    *(screen + y*320 + 2*x+1+ a*20)=TileResP[tile_num*32+2*a+1];
  }
}

/* buffer with level data */
extern unsigned char level_buffer[MAX_HEIGHT][MAX_WIDTH];

void draw_level(int xp, int yp) {
  int x,y,b;
  unsigned long *screen=(unsigned long *)vWinX.GreyScreenBase;
  unsigned long *d, *s;
  
  if (vWinX.GreyScreenBase == 0) return;

  d = screen;
  for(y=0;y<9;y++) {

    for(x=0;x<10;x++) { 
      b = level_buffer[y+yp][x+xp];

      if(b<255-16)
	s = &TileResP[b*32];
      else
	s = (unsigned long*)(custom_tiles[255-b].gray4);

      *d++=*s++; *d=*s++; d+=19; *d++=*s++; *d=*s++; d+=19;
      *d++=*s++; *d=*s++; d+=19; *d++=*s++; *d=*s++; d+=19;
      *d++=*s++; *d=*s++; d+=19; *d++=*s++; *d=*s++; d+=19;
      *d++=*s++; *d=*s++; d+=19; *d++=*s++; *d=*s++; d+=19;

      *d++=*s++; *d=*s++; d+=19; *d++=*s++; *d=*s++; d+=19;
      *d++=*s++; *d=*s++; d+=19; *d++=*s++; *d=*s++; d+=19;
      *d++=*s++; *d=*s++; d+=19; *d++=*s++; *d=*s++; d+=19;
      *d++=*s++; *d=*s++; d+=19; *d++=*s++; *d=*s++; d+=19;

      d -= 318;
    }
    d += 300;
  }
}

void WinFlip(void) {
  int i;
  unsigned long *src = (unsigned long*)vWinX.GreyScreenBase;
  unsigned long *dst = (unsigned long*)vWinX.DisplayBase;

  /* copy screen */
  for(i=0;i<(((160*160)/2)/16);i++) {
    *dst++ = *src++;
    *dst++ = *src++;
    *dst++ = *src++;
    *dst++ = *src++;
  }
}
