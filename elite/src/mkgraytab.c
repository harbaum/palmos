/*
  mkgraytab

  make grayscale mapping table for palm 8 bit color map
*/

#include <stdio.h>
#include <math.h>
#include "coltab.h"

#define WGREEN (0.6)
#define WRED   (0.5)
#define WBLUE  (0.3)
#define BRIGHT (1.3)

int main(void) {
  int i,c;

  /* normal graytab */
  for(i=0;i<256;i++) {
    c = 15 - (((WGREEN * PalmPalette256[i][0] + 
		WRED   * PalmPalette256[i][1] + 
		WBLUE  * PalmPalette256[i][2])/
	       ((WGREEN+WRED+WBLUE)/BRIGHT))*15)/255;

    if(c<0)  c = 0;
    if(c>15) c = 15;

    putchar(((c&0xf)<<4) | (c&0xf)); 
  }

  /* reverse graytab */
  for(i=0;i<256;i++) {
    c =  (((WGREEN * PalmPalette256[i][0] + 
	    WRED   * PalmPalette256[i][1] + 
	    WBLUE  * PalmPalette256[i][2])/
	   ((WGREEN+WRED+WBLUE)/BRIGHT))*15)/255;

    if(c<0)  c = 0;
    if(c>15) c = 15;

    putchar(((c&0xf)<<4) | (c&0xf)); 
  }
	
  return 0;
}
