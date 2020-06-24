#include <stdio.h>

main(void) {
    int i,j,n;

    while((i=fgetc(stdin))!=EOF) {
	n=0;

	for(j=0;j<8;j++)
	    if(i&(0x80>>j))
		n |= 0x01<<j;

	putchar(n);
    }

    return 0;
}
