/*

  splash.c
  
  Splash screen code - simple version, since video setup 
  is done by the main application

*/

#ifdef SPLASH
#include <PalmOS.h>

#include "config.h"
#include "splash.h"
#include "elite.h"
#include "keyboard.h"
#include "Rsc.h"
#include "decompress.h"
#include "gfx.h"
#include "debug.h"
#include "options.h"

/* show splash screen */
void
do_splash (void)
{
  UInt32 wait;
  EventType e;
  void *midiP;
  Err ret = sndErrInterrupted;
  void *tmp;
  UInt8 *d,*s;
  int x,y;

  current_screen = SCR_INTRO_DANUBE;

  /* decompress image from resource */
  if((tmp = s = (UInt8*)decompress_rsc('spls', 0)) != NULL) {
    d = (UInt8*)(vidbase + (40 + 40*80)/pixperbyte );

    if(color_gfx) {
      /* copy image to screen */
      for(y=0;y<80;y++,d+=80)
	for(x=0;x<80;x++)
	  *d++ = *s++;
    } else {
      /* copy image to screen */
      for(y=0;y<80;y++,d+=40) {
	for(x=0;x<80/2;x++, s+=2)
	  *d++ = (0xf0 & graytab[*(s+0)]) | 
	         (0x0f & graytab[*(s+1)]);
      }
    }
    MemPtrFree(tmp);
  }

  /* force text routines to work directly on video buffer */
  tmp = vidbuff;

#ifdef INTERNAL_TEXT
  vidbuff = vidbase;
  gfx_display_text(DSP_CENTER, 6, ALIGN_CENTER,
		   get_text(TXT_INTERN), GFX_COL_RED);

#else
#ifdef LIMIT
  vidbuff = vidbase;
#ifdef LIMIT_NAME
  gfx_display_text(DSP_CENTER, 6, ALIGN_CENTER,
		   get_text(TXT_LIM_USR_MSG), GFX_COL_RED);
#endif
#ifdef LIMIT_TIME
  gfx_display_text(DSP_CENTER, 6, ALIGN_CENTER,
		   get_text(TXT_LIM_TIM_MSG), GFX_COL_RED);
#endif
#endif
#endif

  vidbuff = vidbase+(80*160/pixperbyte);

  gfx_display_text(DSP_CENTER, 20, ALIGN_CENTER,
		   get_text(TXT_VERSION), GFX_COL_GOLD);
  gfx_display_text(DSP_CENTER, 28, ALIGN_CENTER,
		   get_text(TXT_COBRA_POCKET), GFX_COL_WHITE);

  gfx_display_text(DSP_CENTER, 80-36, ALIGN_CENTER,
		   get_text(TXT_COPYRIGHT1), GFX_COL_GOLD);
  gfx_display_text(DSP_CENTER, 80-28, ALIGN_CENTER,
		   get_text(TXT_COPYRIGHT2), GFX_COL_GOLD);
  gfx_display_text(DSP_CENTER, 80-20, ALIGN_CENTER,
		   get_text(TXT_COPYRIGHT3), GFX_COL_GOLD);

  gfx_display_text(DSP_CENTER, 80-8, ALIGN_CENTER, 
		   get_text(TXT_URL), GFX_COL_WHITE);

  /* force update when using palmos draw routines */
  gfx_indirect_update();

  if(options.bool[OPTION_SOUND]) {
    /* try to play some music */
    if((midiP = decompress_rsc('midi', 0)) != NULL) {
      ret = SndPlaySmf(NULL, sndSmfCmdPlay, midiP, NULL, NULL, NULL, 0);
      MemPtrFree(midiP);
    }
  } else
    ret = sndErrInterrupted;

  /* sound was probably interrupted by screen tap -> eat it! */
  if(ret == sndErrInterrupted) {
    do 
      kbd_poll_keyboard ();
    while( !(kbd_button_map & KBD_TAP) && !(finish));
  }

  /* restore video setting */
  vidbuff = tmp;
}
#endif //SPLASH
