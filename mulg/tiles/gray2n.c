#include <stdio.h>
#include <netinet/in.h>  /* for htonl */

int main(int argc, char *argv[]) {
  unsigned int bits,i,k,m,step, max,v,maxt;
  FILE *out, *in;
  unsigned long q;

  bits = atoi(argv[1]);

  if((bits!=2)&&(bits!=4)) {
    printf("usage: gray2n bits outfile infiles ...\n");
    exit(1);
  }

  max  = (1<<bits)-1;
  step = 255/max;

  out = fopen(argv[2], "wb");

  for(i=3;i<argc;i++) {
    in = fopen(argv[i], "rb");

    /* each tile has 256 bits */
    for(q=0,k=0;k<256;k++) {
      m = fgetc(in);
      v = max - ((m+step/2)/step);

      q <<= bits;
      q |= v;

      if((k&((32/bits)-1))==((32/bits)-1)) {
	q = htonl(q);
	fwrite(&q, sizeof(unsigned long), 1, out);
	q=0;
      }
    }

    fclose(in);
  }

  fclose(out);
  return 0;
}
