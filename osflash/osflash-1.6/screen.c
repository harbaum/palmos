/* 
   screen.c - simple text mode screen routines to avoid 
              OS access during flash 

*/


#define SCREEN_BASE (*(long*)0xfffffa00)

void scr_scrollup(void) {
  int i;
  long *p;

  p=(long*)SCREEN_BASE;
  for(i=0;i<(152*5);i++,p++)
    *p=*(p+40);  
}

void scr_draw_char_pos(int x, int y, char chr) {
  int i;
  char *p,*s;
  extern unsigned char font[];

  p=(char*)SCREEN_BASE + x + y*20*8;
  s=&font[8*chr];

  for(i=0;i<8;i++) {
    *p=*s++;
    p+=20;
  }
}

int scr_x=0, scr_y=0;
void scr_draw_char(char chr) {
  if((chr!='\r')&&(chr!='\n'))
    scr_draw_char_pos(scr_x, scr_y, chr);

  if(chr=='\r') { scr_x=0; return; }

  scr_x++;
  if((scr_x==20)||(chr=='\n')) {
    scr_x=0;
    scr_y++;

    if(scr_y==20) {
      scr_y=19;
      scr_scrollup();
    }
  }
}

void scr_home(void) {
  scr_x=0;
  scr_y=0;
}

void scr_clear(void) { 
  int i;
  long *p;

  p=(long*)SCREEN_BASE;
  for(i=0;i<(160*5);i++)
    *p++=0;

  scr_home();
}

void scr_draw_string(char *str) {
  while(*str) {
    scr_draw_char(*str++);
  }
}
