#include <stdio.h>
#include <math.h>

main(void) {
  int i;

  for(i=0;i<80;i++) {
    printf("0x%02x,", 0xff&(int)(63*sin((float)i/32*M_PI)));
  }
}
