/*

  text box with scrollbar

*/

#include <PalmOS.h>

#include "Rsc.h"
#include "IR-Pong2.h"


#define VIEW_LINES         11

static void set_scrollbar(FormPtr form, FieldPtr field) {
  ScrollBarPtr scrollbar;
  UInt16 scroll_pos, text_height, field_height, max_value;

  scrollbar = FrmGetObjectPtr(form, FrmGetObjectIndex(form, ViewScrollbar));

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
  field = FrmGetObjectPtr(form, FrmGetObjectIndex(form, ViewField));
  
  if (delta < 0) FldScrollField(field, - delta, 0);
  else           FldScrollField(field, delta, 1);
  
  if (update_scrollbar) set_scrollbar(form, field);
}

static Boolean handle_read_form_event(EventPtr event) {
  Boolean handled = false;

  switch (event->eType) {
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

/* view some STRING ressource as text msg */
void show_text(char *title, UInt16 rsc, MemHandle hand) {
  FormPtr frm;
  FieldPtr fld;

  frm = FrmInitForm(TextView);
  FrmSetTitle(frm, title);
  fld = FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, ViewField));
  if(hand==NULL) FldSetTextHandle(fld, DmGetResource(strRsc, rsc));
  else           FldSetTextHandle(fld, hand);

  set_scrollbar(frm, fld);
  FrmSetEventHandler(frm, handle_read_form_event);
  FrmDoDialog(frm);

  /* this frees the textbuffer, too */
  FldSetTextHandle(fld, (MemHandle)NULL);
  FrmDeleteForm(frm);
}
