#include <stdio.h>

char *c_names[]={ 
  "red",     "green",   "blue",  "yellow",  "magenta", "cyan", "grey" 
};

/* color weighting */
unsigned int  c_weight[] = { 
  200, 200, 200, 150, 150, 150, 100
};

/* calor values to be activated for color */
int  c_act[][3] = {
  { 1,0,0 }, { 0,1,0 }, { 0,0,1 }, { 1,1,0 }, { 1,0,1 }, { 0,1,1 }, { 1,1,1 }
};

int main(int argc, char **argv) {
  int i, color, c;
  int size, plane;
  FILE *file;
  unsigned char *buffer;

  if(argc!= 4) {
    printf("Usage: %s color infile outfile\n", argv[0]);
    return 1;
  }

  /* search for color name */
  for(color=-1,i=0;i<7;i++)
    if(strcmp(argv[1], c_names[i]) == 0)
      color = i;

  if(color < 0) {
    printf("Not a valid color: %s\n", argv[1]);
    return 1;
  }

  if(!(file = fopen(argv[2], "rb"))) {
    printf("Unable to open input file %s\n", argv[2]);
    return 1;
  }
  
  /* get input file size */
  fseek(file, 0, SEEK_END);
  size = ftell(file);
  fseek(file, 0, SEEK_SET);
  printf("image size = %d bytes\n", size);

  /* read input file */
  buffer = (unsigned char*)malloc(size);
  fread(buffer, 1, size, file);

  fclose(file);

  if(!(file = fopen(argv[3], "wb"))) {
    printf("Unable to open output file %s\n", argv[3]);
    return 1;
  }
  
  /* write three planes */
  for(plane=0;plane<3;plane++) {
    for(i=0;i<size;i++) {
      /* get new color weight in percent */
      c = (c_weight[color] * ((unsigned int)buffer[i])) / 100;

      /* active color */
      if(c_act[color][plane]) {
	if(c >= 256)    fputc(0xff, file);
	else            fputc(c, file);
      } else {
	if(c >= 256)    fputc(c-256, file);
	else            fputc(0, file);
      }
    }
  }

  fclose(file);
  free(buffer);

  return 0;
}
