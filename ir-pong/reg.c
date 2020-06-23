#include <stdio.h>

int main(int argc, char **argv) {
  int j,i, id, chr;

  printf("IR Pong Keygen\n");

  if(argc!=2) {
    printf("usage: reg name\n");
    return 1;
  }

  id=j=i=0;
  while(argv[1][i]!=0) {
    chr = argv[1][i];

    if((chr>32)&&(chr<127)) {

      if((chr^j) & 1) id ^= chr;
      else            id ^= (chr<<8);

      id ^= (0x1111<<(chr&3));

      id &= 0x7fff;
      j++;
    }

    i++;
  }

  printf("Code for '%s': %d\n", argv[1], id);

  return 0;
}
