/*---------------------------------------------------------------------
| winxopts.h
|
|	Define global options for the WIN2 2bpp greyscale library
|
|	(c) 1997-1998 ScumbySoft - wesc@ricochet.net
-----------------------------------------------------------------------*/


/*---------------------------------------------------------------------
| Global options for the WinX library.  Undefine these to elminate
| APIs which you don't call to minimize code size
-----------------------------------------------------------------------*/
// #define WINXDRAWBITMAP 1
#define WINXDRAWCHARS     1
#define WINXFILLRECT      1

//#define WINXDRAWLINE   1 // #defining WINXFILLRECT too makes horiz lines much faster

#define WINXALLOCPTR 1

