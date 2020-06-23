/*
  configurable refresh rate for NoStreak

  (c) 1999 Till Harbaum

  this is freeware/open source, read the file COPYING for details
*/

#pragma pack(2)
#include <Pilot.h>

#define CREATOR 'NStr'
#define DEFAULT 2

Boolean eventhandle(EventPtr event) {
  Word i, but;
  DWord val;
  FormPtr form = FrmGetActiveForm();

  switch (event->eType) {

  case frmOpenEvent:
    FrmDrawForm(form);

    /* get current value from features */
    if(FtrGet(CREATOR, 0, &val) != 0) val = DEFAULT;
    
    /* highlight button */
    for(i=0;i<6;i++)
      CtlSetValue(FrmGetObjectPtr (form, FrmGetObjectIndex (form, 2002+i)), i == (val&0x0f));

    /* greyscale button */
    CtlSetValue(FrmGetObjectPtr (form, FrmGetObjectIndex (form, 2010)), val&0x10);    

    /* draw some lines to show streaking */
    #define WIDTH     60  // half width
    #define HEIGHT    33  // half height
    #define CENTER_X  78
    #define CENTER_Y  96
    #define STEPS_X    5
    #define STEPS_Y    4

    /* draw vertical lines */
    for(i=0;i<STEPS_X;i++) {
      WinDrawLine(CENTER_X+(WIDTH/(STEPS_X-1))*i, CENTER_Y-HEIGHT, CENTER_X+(WIDTH/(STEPS_X-1))*i, CENTER_Y+HEIGHT);
      WinDrawLine(CENTER_X-(WIDTH/(STEPS_X-1))*i, CENTER_Y-HEIGHT, CENTER_X-(WIDTH/(STEPS_X-1))*i, CENTER_Y+HEIGHT);
    }
      
    /* draw horizontal lines */
    for(i=0;i<STEPS_Y;i++) {
      WinDrawLine(CENTER_X-WIDTH, CENTER_Y+(HEIGHT/(STEPS_Y-1))*i, CENTER_X+WIDTH, CENTER_Y+(HEIGHT/(STEPS_Y-1))*i);
      WinDrawLine(CENTER_X-WIDTH, CENTER_Y-(HEIGHT/(STEPS_Y-1))*i, CENTER_X+WIDTH, CENTER_Y-(HEIGHT/(STEPS_Y-1))*i);
    }

    /* save original value */
    FtrSet(CREATOR, 1, val);

    return 1;

  case ctlSelectEvent:
    but = event->data.ctlEnter.controlID;

    /* scan buttons and set feature */
    for(i=0;i<6;i++)
      if(CtlGetValue(FrmGetObjectPtr (form, FrmGetObjectIndex (form, 2002+i))))
	val = i;

    if(CtlGetValue(FrmGetObjectPtr (form, FrmGetObjectIndex (form, 2010))))
      val |= 0x10;

    FtrSet(CREATOR, 0, val);

    /* ok or cancel button */
    if((but == 2008)||(but == 2009)) {

      if(but==2009) {
	/* 'cancel' -> reload original value */
	FtrGet(CREATOR, 1, &val);
	FtrSet(CREATOR, 0,  val);
      }

      FrmGotoForm(9000);
      return 1;
    }

    default:
  }

  FrmHandleEvent(form, event);
  return 1;
}




