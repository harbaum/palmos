/*
 *  dialog.c - dialogs in elites scanner area
 */

#include <PalmOS.h>

#include "config.h"
#include "elite.h"
#include "debug.h"
#include "dialog.h"
#include "gfx.h"
#include "space.h"
#include "docked.h"
#include "sequences.h"
#include "file.h"
#include "options.h"
#include "sound.h"

#define  DLG_BUT_W  (40)
#define  DLG_BUT_X1 (DLG_X1+DLG_BRD)
#define  DLG_BUT_X2 (DLG_X2-DLG_BRD-DLG_BUT_W)
#define  DLG_BUT_Y  (DLG_Y2-GFX_CELL_HEIGHT-5)
#define  DLG_BUT_E  (DLG_BUT_Y+GFX_CELL_HEIGHT+2)

#define  DLG_INP_Y  (DLG_Y1+4+DLG_BRD+2*GFX_CELL_HEIGHT)

#define  DLG_CURSOR 9

static void dialog_undraw(void);
static void draw_button(int x, char *text, int color, int color_bg);

Int8 dialog_btn_active=0;
Int8 dialog_id;

Int8 txt_pos=-1;
char txt_buffer[DLG_MAX_LEN+2];

void
dialog_handle_char(UInt8 chr) {
  void *tmp;
  int  pos, dpos;

  /* text input required? */
  if(txt_pos < 0) return;

  /* only allow upper and lower case characters and numbers */
  if((chr!=0)&&(chr!=8)&&                    // 0,bs
     ((chr<' ')||(chr>128))) return;

  /* nothing to delete: return immediately */
  if((chr == 8)&&(txt_pos == 0)) return;

  if((txt_pos >= DLG_MAX_LEN)&&(chr != 8)) return;

  /* force graphic routines to work directly on video buffer */
  tmp = vidbuff;
  vidbuff = vidbase+(DSP_HEIGHT*160/pixperbyte);
  
  dpos = txt_pos-((chr == 8)?1:0);
  
  if(chr != 0) {
    /* make chr uppercase */
    chr = my_toupper(chr);

    /* fetch x pos of cursor char */
    pos = gfx_str_width(txt_buffer);
    if(pos > 0) {
      pos -= gfx_str_width(&txt_buffer[dpos]);
    }
    
    /* undraw cursor */
    gfx_display_text(DLG_X1+DLG_BRD+pos, DLG_INP_Y, ALIGN_LEFT, 
		     &txt_buffer[dpos], GFX_COL_BLACK); 
    
    /* attach char to buffer */
    if(chr != 8)
      txt_buffer[txt_pos++] = chr;
    else
      txt_pos--;
  }
  
  /* attach cursor */
  txt_buffer[txt_pos]   = DLG_CURSOR;
  txt_buffer[txt_pos+1] = 0;
  
  if(chr != 0) {
    /* draw new char and cursor */
    gfx_display_text(DLG_X1+DLG_BRD+pos, DLG_INP_Y, ALIGN_LEFT, 
		     &txt_buffer[(txt_pos>0)?txt_pos-((chr!=8)?1:0):0], 
		     GFX_COL_WHITE); 
  } else
    gfx_display_text(DLG_X1+DLG_BRD, DLG_INP_Y, ALIGN_LEFT, 
		     txt_buffer, GFX_COL_WHITE); 
  
  vidbuff = tmp;
}

void 
dialog_handle_tap(int x, int y) {
  int hit=0;

  /* check vertically */
  y-=DSP_HEIGHT;
  if((y>=DLG_BUT_Y)&&(y<=DLG_BUT_E)) {
    if((dialog_btn_active & DLG_BTN1)&&
       (x>=DLG_BUT_X1)&&(x<=DLG_BUT_X1+DLG_BUT_W)) hit = 1;
    if((dialog_btn_active & DLG_BTN2)&&
       (x>=DLG_BUT_X2)&&(x<=DLG_BUT_X2+DLG_BUT_W)) hit = 2;
  }

  /* hit some button */
  if(hit) {
    /* remove last char from text */
    if(txt_pos>=0) txt_buffer[txt_pos]=0;

    snd_click();
    dialog_undraw();

    switch(dialog_id) {
      case DLG_FIND_PLANET:
	if(hit == 1) {
	  cross_x = old_cross_x = -100;
	  display_galactic_chart();
	  find_planet_by_name(txt_buffer);
	}
	break;

      case DLG_EBOMB:
	if(hit == 1) {
	  detonate_bomb = 1;
	  cmdr.energy_bomb = 0;
	  gfx_update_button_map(SAME_MARKER); // remove bomb icon
	}
	break;

      case DLG_ESCAPE:
	if(hit == 1)
	  run_escape_sequence();
	break;

      case DLG_NOTHING:
	break;

      case DLG_RENAME:
	if(hit == 1) {
	  rename_commander(rename_slot, txt_buffer);
	  draw_save_slot(rename_slot); 
	}
	break;

      case DLG_NAME:
	DEBUG_PRINTF("new commander name: %s\n", txt_buffer);
	StrCopy(cmdr.name, txt_buffer);
	break;

      case DLG_NEW:
	if(hit == 1) {
	  restore_jameson_data();

	  DEBUG_PRINTF("new commander name: %s\n", txt_buffer);
	  StrCopy(cmdr.name, txt_buffer);

	  docked_planet = find_planet (cmdr.ship_x, cmdr.ship_y);
	  hyperspace_planet = docked_planet;
  
	  generate_planet_data (&current_planet_data, docked_planet);
	  generate_stock_market ();
	  set_stock_quantities (cmdr.station_stock);

	  dock_player();
	  gfx_draw_scanner();
	  update_console(1);
	  
	  display_commander_status ();
	}
	break;

      default:
	DEBUG_PRINTF("unknown dialog event\n");
	break;
    }
  }
}

static void
draw_button(int xb, char *text, int color, int color_bg) {
  int x = (xb==0)?DLG_BUT_X1:DLG_BUT_X2;

  if(text == NULL) return;

  gfx_fill_rectangle(x, DLG_BUT_Y, x+DLG_BUT_W, DLG_BUT_E, color_bg);
  gfx_draw_rectangle(x, DLG_BUT_Y, x+DLG_BUT_W, DLG_BUT_E, color);

  gfx_display_text(x+(DLG_BUT_W/2), DLG_BUT_Y+2, ALIGN_CENTER, 
		   text, GFX_COL_WHITE);

  /* remember, that a button is being displayed */
  dialog_btn_active |= (1<<xb);
}

void 
dialog_draw(const struct dialog *dlg) {
  void *tmp;
  int color, color_bg;

  /* never draw a dialog over a dialog */
  if(dialog_btn_active) {
    DEBUG_PRINTF("ERROR, dialog over dialog!\n");
    return;
  }

  /* disable funkey inpout */
  gfx_update_button_map(NO_MARKER);

  /* force graphic routines to work directly on video buffer */
  tmp = vidbuff;
  vidbuff = vidbase+DSP_HEIGHT*(160/pixperbyte);

  /* save id of this dialog, since this is somehow handled asychronously */
  dialog_id = dlg->id;

  /* and now draw the dialog itself */

  /* erase scanner */
  gfx_fill_rectangle(DLG_X1, DLG_Y1, DLG_X2, DLG_Y2, GFX_COL_BLACK);

  if(dlg->type == DLG_ALERT) {
    color     = GFX_COL_RED;
    color_bg  = GFX_COL_DARK_RED;
  } else {
    color     = GFX_COL_GREEN_2;
    color_bg  = GFX_COL_GREEN_4;
  }

  /* draw border */
  gfx_draw_rectangle(DLG_X1, DLG_Y1, DLG_X2, DLG_Y2, color);

  /* draw title */
  gfx_fill_rectangle(DLG_X1, DLG_Y1, DLG_X2, DLG_Y1+GFX_CELL_HEIGHT+2, color);
  gfx_display_text(DSP_CENTER, 2, ALIGN_CENTER, dlg->title, GFX_COL_WHITE);

  gfx_display_pretty_text(DLG_X1+DLG_BRD, DLG_Y1+GFX_CELL_HEIGHT+3+DLG_BRD,
 			  DLG_X2-DLG_BRD, DLG_Y2-DLG_BRD, 1, dlg->msg);

  /* finally draw some buttons */
  draw_button(0, dlg->but1, color, color_bg);
  draw_button(1, dlg->but2, color, color_bg);

  /* create text input if required */
  if(dlg->type == DLG_TXTINPUT) {
    gfx_draw_colour_line(DLG_X1+DLG_BRD, DLG_INP_Y+GFX_CELL_HEIGHT, 
			 DLG_X2-DLG_BRD, DLG_INP_Y+GFX_CELL_HEIGHT, color);

    if(dlg->init == NULL) {
      txt_pos = 0;
      txt_buffer[0] = 0;
    } else {
      StrCopy(txt_buffer, dlg->init);
      txt_pos = StrLen(txt_buffer);
    }

    dialog_handle_char(0);
  }

  /* and return to double buffered mode */
  vidbuff = tmp;
}

static void 
dialog_undraw(void) {
  UInt8 *s = (UInt8*)(sprite_data[SPRITE_SCANNER]+32);
  UInt8 *d = (UInt8*)(vidbase+(160*(160-SCANNER_HEIGHT-FUNKEY_HEIGHT)+32)/pixperbyte);
  int x,y;

  if(color_gfx) {
    /* draw color center of scanner */
    for(y=0;y<40;y++) {
      for(x=0;x<96;x++) *d++ = *s++;
      d+=64; s+=64;
    }
  } else {
    /* draw grayscale center of scanner */
    for(y=0;y<40;y++) {
      for(x=0;x<48;x++,s+=2) 
	*d++ = (0xf0 & *(s+0))|(0x0f & *(s+1));

      d+=32; s+=64;
    }
  }
  
  scanner_clear_buffer();

  dialog_btn_active=0;
  txt_pos = -1;

  update_console(1);

  gfx_update_button_map(SAME_MARKER);
}
