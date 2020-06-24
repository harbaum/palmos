/*
  mkfnttab.c

*/

#include <stdio.h>

#define FONT_SPC (6)
#define FONT_LEN (FONT_SPC * FONT_SPC * 128/8)

//#define VIEW

char buffer[FONT_LEN];

static void draw_char(char chr) {
  char *src = &buffer[chr*6/8];
  int  i,j,pb, left=100, right=0;

  for(j=0;j<FONT_SPC;j++) {
    pb=(chr*FONT_SPC)%8;
    for(i=0;i<FONT_SPC;i++) {
      if(src[(i+pb)/8] & (0x01<<((i+pb)%8))) {
#ifdef VIEW
	putchar('*');
#endif
	if(left>i)  left=i;
	if(right<i) right=i;
      } 
#ifdef VIEW
      else
	putchar('.');
#endif
    }
#ifdef VIEW
    putchar('\n');
#endif

    src+=96;
  }

  if((left != 0)&&(left != 100))
      fprintf(stderr, "left offset of char %d != 0 (%d)\n", 
	      (int)chr&0xff, left);

#ifdef VIEW
  printf("Left: %d, Right: %d, Width: %d\n", left, right, right+1-left);
#else
  if(left == 100) putchar(0);
  else            putchar( (left<<4) | (right+1-left));
#endif
}

main(int argc, char **argv) {
  FILE *fnt;
  int i;

  if((fnt = fopen(argv[1], "rb")) == NULL) {
    perror("fopen");
    return 1;
  }

  if(fread(buffer, 1l, FONT_LEN, fnt) != FONT_LEN) {
    fprintf(stderr, "font read error\n");
    return 1;
  }
  
#ifdef VIEW
  draw_char('M');
  draw_char('i');
  draw_char('l');
  draw_char('c');
  draw_char('h');
  draw_char(0);
  draw_char(1);
  draw_char(5);
#else
  for(i=0;i<128;i++)
    draw_char(i);
#endif  

  fclose(fnt);
  return 0;
}
