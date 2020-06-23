/*
  bt_discovery_ui.c

  user interface routines for more user friendly discovery dialog
  
  (c) 2003 by Till Harbaum, BeeCon GmbH
*/

#include <PalmOS.h>
#include <BtLib.h>
#include <BtLibTypes.h>

#include "Rsc.h"
#include "bt.h"
#include "bt_error.h"
#include "bt_discovery.h"
#include "bt_discovery_db.h"
#include "bt_discovery_ui.h"
#include "bt_discovery_pb.h"

#include "debug.h"

#define VIEW_LINES         11

static int scroll_offset = 0;

static void set_scrollbar(FormPtr form, FieldPtr field) {
  ScrollBarPtr scrollbar;
  UInt16 scroll_pos, text_height, field_height, max_value;

  scrollbar = FrmGetObjectPtr(form, FrmGetObjectIndex(form, scl_devices));

  FldGetScrollValues(field, &scroll_pos, &text_height, &field_height);
  if (text_height > field_height) {
    max_value = text_height - field_height;
  } else if (scroll_pos > 0) {
    max_value = scroll_pos;
  } else {
    max_value = 0;
  }
  SclSetScrollBar(scrollbar, scroll_pos, 0, max_value, field_height);
}

static void scroll_field(int delta, Boolean update_scrollbar) {
  FormPtr form;
  FieldPtr field;

  form = FrmGetActiveForm();
  field = FrmGetObjectPtr(form, FrmGetObjectIndex(form, fld_devices));
  
  if (delta < 0) FldScrollField(field, - delta, 0);
  else           FldScrollField(field, delta, 1);
  
  if (update_scrollbar) set_scrollbar(form, field);
}

static Boolean handle_discovery_form_event(EventPtr event, UInt16 *quit) {
  Boolean handled = false;

  switch (event->eType) {
    case ctlSelectEvent:
      DEBUG_RED_PRINTF("ctlSelectEvent(), id = %d\n",
		       event->data.ctlEnter.controlID);

      if (event->data.ctlEnter.controlID == btn_Ok) {
	DEBUG_RED_PRINTF("clicked btn_Ok\n");
	handled = true;
	*quit = true;

      }
      break;

    case keyDownEvent:
      if (event->data.keyDown.chr == pageUpChr) {
	scroll_field(- VIEW_LINES + 1, true);
	handled = true;
      } else if (event->data.keyDown.chr == pageDownChr) {
	scroll_field(VIEW_LINES - 1, true);
	handled = true;
      }
      break;
      
    case fldChangedEvent:
      scroll_field(0, true);
      handled = true;
      break;
      
    case sclRepeatEvent:
      scroll_field(event->data.sclRepeat.newValue - 
		   event->data.sclRepeat.value, false);
      break;
  }
  
  return handled;
}

#define NAMES_X  4
#define NAMES_Y  14

/* write length limited string followed by dots at pos */
void 
bt_draw_chars_len(int x, int y, Int16 max, char *str) {
  Int16 lenp;
  Boolean fit;
  const char dots[]="...";

  lenp = StrLen(str);
  fit = true;
  FntCharsInWidth(str, &max, &lenp, &fit);

  /* something cut off? */
  if(lenp != StrLen(str)) {
    max = max-FntLineWidth((char*)dots,3);
    FntCharsInWidth(str, &max, &lenp, &fit);
    WinDrawChars(str, lenp, x, y);
    WinDrawChars((char*)dots, 3, x+max, y);
  } else
    WinDrawChars(str, lenp, x, y);
}

/* draw entry on screen */
void bt_discovery_ui_add(char *name, int index) {
  int y = NAMES_Y + 12 * (index - scroll_offset);

  FntSetFont(0);   // standard font
  bt_draw_chars_len(NAMES_X, y, 140, name);
}

void bt_discovery_ui_draw(void) {
  FormPtr frm, origfrm;
  EventType event;
  UInt16    error, quit;

  origfrm = FrmGetActiveForm();

  /* load the discovery form */
  frm = FrmInitForm(frm_Discovery);
  FrmSetActiveForm(frm);
  FrmDrawForm(frm);
 
//  set_scrollbar(frm, fld);

  /* stop inquiry if its still in progress */
  bt_discovery_pb(true, 0);
 
  /* the event handler */
  do {
    EvtGetEvent(&event, SysTicksPerSecond()/2);

    if((event.eType) == appStopEvent) {
      /* put event back into queue */
      EvtAddEventToQueue(&event);
      quit=true;
    } else {
      quit = false;
      if(!(handle_discovery_form_event(&event, &quit))) {
	
	if(!(SysHandleEvent(&event)))
	  MenuHandleEvent(NULL, &event, &error);
      }
    }

    FrmDispatchEvent(&event);

    if(gInquiring)
      bt_discovery_pb(false, BT_PB_TIME);

  } while(!quit);

  FrmEraseForm(frm);
  FrmDeleteForm(frm);

  FrmSetActiveForm(origfrm);
}
