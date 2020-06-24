/*
 * sin.c - sine/cosine table generator for Elite for PalmOS
 */


#include <stdio.h>
#include <math.h>

#define  SCALE  16384

main(int argc, char **argv) {
  int j, i, steps, total;

  steps = atoi(argv[1]);
  total = (5*steps)/4;

  fprintf(stderr, "creating sinus table of resolution %d (%d total)\n", 
	  steps, total);

  for(i=0;i<total;i++) {
    j = 0xffff & (int)(SCALE*sin((float)i/(steps/2)*M_PI));

    /* write highbyte first */
    putchar(j>>8); putchar(j&0xff);
  }

  fprintf(stderr, "size of sinus table: %d bytes\n", 
	  total * sizeof(short));

  return 0;
}
