/*
*/

#pragma pack(2)
#include <Pilot.h>
#include <System/KeyMgr.h>

Boolean eventhandle(EventPtr event)
{
  FormPtr form = FrmGetActiveForm();
  DWord mask;
  Err err;
  long i;
  int n,j;

  /* get button mask */
  err = FtrGet(CREATOR, 1001, &mask);
  if(err) mask = 0;  
  
  switch (event->eType) {

  case frmOpenEvent:
    FrmDrawForm(form);

    /* set speed rate */
    for(n=0,j=0,i=0x100;i<0x10000;i<<=1,n++) {
      if((j==0)&&(mask & i)) {
	CtlSetValue(FrmGetObjectPtr (form, FrmGetObjectIndex (form, 5110+n)), 1);
	j=1;
      } else
	CtlSetValue(FrmGetObjectPtr (form, FrmGetObjectIndex (form, 5110+n)), 0);
    }

    if(j==0)
	CtlSetValue(FrmGetObjectPtr (form, FrmGetObjectIndex (form, 5110)), 1);

    /* set buttons */
    CtlSetValue(FrmGetObjectPtr (form, FrmGetObjectIndex (form, 5100)), mask & keyBitPageUp);
    CtlSetValue(FrmGetObjectPtr (form, FrmGetObjectIndex (form, 5101)), mask & keyBitHard1);
    CtlSetValue(FrmGetObjectPtr (form, FrmGetObjectIndex (form, 5102)), mask & keyBitHard2);
    CtlSetValue(FrmGetObjectPtr (form, FrmGetObjectIndex (form, 5103)), mask & keyBitHard3);
    CtlSetValue(FrmGetObjectPtr (form, FrmGetObjectIndex (form, 5104)), mask & keyBitHard4);
    CtlSetValue(FrmGetObjectPtr (form, FrmGetObjectIndex (form, 5105)), mask & keyBitPageDown);

    return 1;

  case ctlSelectEvent:
    switch(event->data.ctlEnter.controlID) {
      
    case 5001:
      /* get gui settings */
      mask = 0;
      if(CtlGetValue(FrmGetObjectPtr (form, FrmGetObjectIndex (form, 5100)))) mask |= keyBitPageUp;
      if(CtlGetValue(FrmGetObjectPtr (form, FrmGetObjectIndex (form, 5101)))) mask |= keyBitHard1;
      if(CtlGetValue(FrmGetObjectPtr (form, FrmGetObjectIndex (form, 5102)))) mask |= keyBitHard2;
      if(CtlGetValue(FrmGetObjectPtr (form, FrmGetObjectIndex (form, 5103)))) mask |= keyBitHard3;
      if(CtlGetValue(FrmGetObjectPtr (form, FrmGetObjectIndex (form, 5104)))) mask |= keyBitHard4;
      if(CtlGetValue(FrmGetObjectPtr (form, FrmGetObjectIndex (form, 5105)))) mask |= keyBitPageDown;
 
      /* get speed rate */
      for(n=0,i=0x100;i<0x10000;i<<=1,n++) {
	if(CtlGetValue(FrmGetObjectPtr (form, FrmGetObjectIndex (form, 5110+n))))
	  mask |= i;
      }

      /* set feature */
      FtrSet(CREATOR, 1001, mask);

      /* return to hackmaster */
      FrmGotoForm(9000);
      return 1;
      break;

    }

  default:
  }

  FrmHandleEvent(form, event);
  return 1;
}
