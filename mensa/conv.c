#include <stdio.h>
#include <string.h>

void output(char *str) {
  while(*str) {
    while((*str==' ')&&(*(str+1)==' ')) str++; 

    if(*str=='&') {
      if     (*(str+1)=='a') { str+=5; printf("\\%03o",0xe4); }
      else if(*(str+1)=='o') { str+=5; printf("\\%03o",0xf6); }
      else if(*(str+1)=='u') { str+=5; printf("\\%03o",0xfc); }
      else if(*(str+1)=='s') { str+=6; printf("\\%03o",0xdf); }
      else if(*(str+1)=='A') { str+=5; printf("\\%03o",0xc4); }
      else if(*(str+1)=='O') { str+=5; printf("\\%03o",0xd6); }
      else if(*(str+1)=='U') { str+=6; printf("\\%03o",0xdc); }
      else putchar('&');
    } else {
      if(*str=='"')
	putchar('\\');

      putchar(*str);

      if((*str==',')&&(*(str+1)!=' '))
	putchar(' ');
    }
    str++;
  }
}

main(int argc, char *argv[]) {
  FILE *in;
  char str[128];
  int i,day=0,week=0;
  int first=1, last_was_essen=0,quiet=0;

  in=fopen(argv[1],"r");
  if(in==NULL) {
    perror("reading input file");
    exit(1);
  }

  do
    fgets(str, 128, in);
  while(strncmp(str, "__END__", 7)!=0);

  printf("char *mampf[]={\n");

  while(fgets(str, 128, in)!=NULL) {
    /* remove return */
    str[strlen(str)-1]=0;

    if((strncmp(str, "---", 3)!=0)&&(strncmp(str, "===", 3)!=0)) {
      if((strncmp(str, "Essen", 5)==0)||(strncmp(str, "EXTRA", 5)==0)) {
	if(first)
	  printf("\"");
	else
	  printf("\",\n\"");
	
	first=0;

	if(strncmp(str, "EXTRA", 5)==0)
	  output(&str[7]);
	else {
	  output(&str[9]);
	  last_was_essen=1;
	}

	quiet=0;
      } else {
	if(!quiet) {
	  if(last_was_essen) putchar(',');
	  output(str);
	  last_was_essen=0;
	}
      }
    } else
      quiet=1;
  }

  printf("\"\n};\n");

  fclose(in);

  return 0;
}
