/* replace.c */
/* image mapper for mulg color tiles */
/* (c) 2000 by Till Harbaum */

#include <stdio.h>

main(int argc, char **argv) {
  FILE *in, *map, *out;
  int pix,r,s,c,color, cnt=0;

  if(argc!=5) {
    puts("usage: replace color in map out");
    puts("       with color = rrggbb");
    return 1;
  }

  sscanf(argv[1], "%x", &color); 
  printf("replacing color #%06x\n", color);

  map = fopen(argv[2], "rb");
  in  = fopen(argv[3], "rb");
  out = fopen(argv[4], "wb");

  if((in==NULL)||(map==NULL)||(out==NULL)) {
    puts("file open error");
    return 1;
  }

  for(pix=0;pix<256;pix++) {
    s = (((int)fgetc(map))<<16) + (((int)fgetc(map))<<8) + (((int)fgetc(map)));
    c = (((int)fgetc(in))<<16) + (((int)fgetc(in))<<8) + (((int)fgetc(in)));

    if(s==color) {
      fputc(c>>16, out);
      fputc(c>>8, out);
      fputc(c, out);
      cnt++;
    } else {
      fputc(s>>16, out);
      fputc(s>>8, out);
      fputc(s, out);
    }
  }

  fclose(in);
  fclose(map);
  fclose(out);

  printf("replaced %d pixels (%d%%)\n", cnt, (cnt*100)/256);

  return 0;
}
