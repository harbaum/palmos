#ifndef DIALOG_H
#define DIALOG_H

/* dialog positions */
#define  DLG_X1   (32)
#define  DLG_Y1    (0)
#define  DLG_X2  (127)
#define  DLG_Y2   (39)
#define  DLG_BRD   (3)

/* currently supported types of dialog */
#define DLG_ALERT     0
#define DLG_QUESTION  1
#define DLG_TXTINPUT  2

#define DLG_MAX_LEN (12)
#define PLANET_LEN  (8)

struct dialog {
  UInt8 type, id;  // see above

  char  *title;    // title message
  char  *msg;      // the message itself
  char  *init;     // text to init txt box

  char  *but1;     // the label of button 1
  char  *but2;     // the label of button 2
};

#define DLG_BTN1  1
#define DLG_BTN2  2

// types of dialogs used
#define DLG_FIND_PLANET  0 
#define DLG_ESCAPE       1
#define DLG_NOTHING      2
#define DLG_RENAME       3
#define DLG_EBOMB        4
#define DLG_NAME         5
#define DLG_NEW          6

extern Bool dlg_shaded;
extern Int8 dialog_btn_active, txt_pos;
extern void dialog_draw(const struct dialog *dlg);
extern void dialog_handle_tap(int x, int y);
extern void dialog_handle_char(UInt8 chr);

#endif // DIALOG_H
