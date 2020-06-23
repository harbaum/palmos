/*---------------------------------------------------------------------
| WinX.h
|
|	WinX 2bpp greyscale replacement for Win* Palm routines
|	To use, replace all calls to Win* palm APIs with WinX*
|
|	See readme.txt for usage information
|
|	(c) 1997-1998 ScumbySoft - wesc@ricochet.net
|
|       This whole grayscale stuff has been modified to 
|       support 2 bpp, 4 bpp and color modes, so the naming
|       my be somwhat inappropriate now.
|
-----------------------------------------------------------------------*/

#include "winxopts.h"


/*---------------------------------------------------------------------
| WinX Colors
-----------------------------------------------------------------------*/
enum WinXColor {
  clrWhite,
  clrLtGrey,
  clrDkGrey,
  clrBlack,
};
	
typedef enum WinXColor WinXColor;	

/*-----------------------------------------------------------------------------
|	WinXSetGreyscale
|	
|		Switches Pilots display into greyscale mode
|	Note, this allocates memory in a variety of ways.  See WinXopts.h
|	for more information
|	
|	Returns: 0 if successful, nonzero if not
-------------------------------------------------------wesc@ricochet.net-----*/
int WinXSetGreyscale();
/*-----------------------------------------------------------------------------
|	WinXSetMono
|	
|		Switches back to mono mode.
|
|	Returns: 0 if successful, nonzero if not
-------------------------------------------------------wesc@ricochet.net-----*/
int  WinXSetMono();
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
WinXColor WinXSetColor(WinXColor clr);
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
WinXColor WinXSetBackgroundColor(WinXColor clr);

/*-----------------------------------------------------------------------------
|	WinXCopyMonoToGrey
|	
|		Copies the contents of the monochrome screen to the grey screen
|	
|	Arguments:
|		Boolean fOrMode:  if true then or's mono bits with dest grey
-------------------------------------------------------wesc@ricochet.net-----*/
void WinXCopyMonoToGrey(Boolean fOrMode);
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
Boolean WinXPreFilterEvent(const EventType *pevt);


/*-----------------------------------------------------------------------------
|	WinXDrawLine
|	
|		Draws a line from x1,y1 to x2,y2
|	
|	Arguments:
|		Int16 x1
|		Int16 y1
|		Int16 x2
|		Int16 y2
-------------------------------------------------------wesc@ricochet.net-----*/
void WinXDrawLine(Int16 x1, Int16 y1, Int16 x2, Int16 y2);

/*-----------------------------------------------------------------------------
|	WinXDrawChars
|	
|		Draws chars on the screen.  Uses current color and background color
|	Note that Underline mode is not supported!
|	
|	Arguments:
|		CharPtr chars
|		UInt16 len
|		Int16 x
|		Int16 y
-------------------------------------------------------wesc@ricochet.net-----*/
void WinXDrawChars(char* chars, UInt16 len, Int16 x, Int16 y);

int WinXFillRect(RectangleType *prc, int clr);

void WinFlip(void);
