/* bumpy.c */
/* bump mapper for mulg color tiles */
/* (c) 2000 by Till Harbaum */

#include <stdio.h>

main(int argc, char **argv) {
  FILE *in, *map, *out;
  int pix,r,s,c,scale;

  if(argc!=5) {
    puts("usage: bumpy scale_percent in map out");
    return 1;
  }

  scale = atoi(argv[1]);
  in  = fopen(argv[2], "rb");
  map = fopen(argv[3], "rb");
  out = fopen(argv[4], "wb");

  if((in==NULL)||(map==NULL)||(out==NULL)) {
    puts("file open error");
    return 1;
  }

  printf("converting with scale factor %d%%\n", scale);

  for(pix=0;pix<256;pix++) {
    s = ((((fgetc(map) + fgetc(map) + fgetc(map))/3)-127)*scale)/100;

    for(r=0;r<3;r++) {
      c = fgetc(in) + s;
      if(c<0) c=0;
      if(c>255) c=255;
      
      fputc(c, out);
    }
  }

  fclose(in);
  fclose(map);
  fclose(out);

  return 0;
}
