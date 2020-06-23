#include <stdio.h>
#include <netinet/in.h>  /* for htonl */

#include "coltab.h"

int ColorIndex(int r, int g, int b) {
  int index, lowValue, i, *diffArray;

  // generate the color "differences" for all colors in the palette
  diffArray = (int *)malloc(256 * sizeof(int));
  for (i=0; i < 256; i++) {
    diffArray[i] = ((PalmPalette256[i][0] - r)*(PalmPalette256[i][0] - r)) +
                   ((PalmPalette256[i][1] - g)*(PalmPalette256[i][1] - g)) +
                   ((PalmPalette256[i][2] - b)*(PalmPalette256[i][2] - b));
  }

  // find the palette index that has the smallest color "difference"
  index    = 0;
  lowValue = diffArray[0];
  for (i=1; i<256; i++) {
    if (diffArray[i] < lowValue) {
      lowValue = diffArray[i];
      index    = i;
    }
  }

  // clean up
  free(diffArray);

  return index;
}

int main(int argc, char *argv[]) {
  FILE *out, *in;
  int i,k,n;
  unsigned char r,g,b;

  if(argc<3) {
    out = fopen(argv[1], "wb");
    for(i=0;i<256;i++) {
      fputc(PalmPalette256[i][0], out);
      fputc(PalmPalette256[i][1], out);
      fputc(PalmPalette256[i][2], out);
    }
    fclose(out);
    return 0;
  }

  out = fopen(argv[1], "wb");

  for(i=2;i<argc;i++) {
    in = fopen(argv[i], "rb");

    /* each tile has 256 pixels */
    for(k=0;k<256;k++) {
      r = fgetc(in);
      g = fgetc(in);
      b = fgetc(in);

      fputc(ColorIndex(r,g,b), out);
    }
    fclose(in);
  }

  fclose(out);
  return 0;
}
