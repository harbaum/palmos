/*

  gray hack  -  (c) 2000 by Till Harbaum

  adjust the grayscale levels when setting grayscale video mode using 
  the system functions

*/

#include <Pilot.h>
#include <ScrDriverNew.h>

DWord get_cpu(void) {
  DWord id, chip;
  Err err;

  err = FtrGet(sysFtrCreator, sysFtrNumProcessorID, &id);
  if (err) {
    // can't get that feature; we must be on an old unit
    // without it defined, thus we're on a DragonBall.
    chip = sysFtrNumProcessor328;
  } else
    chip = id & sysFtrNumProcessorMask;

  return(chip);
}

void set_gray(DWord cpu, short gray1, short gray2) {
  DWord value;
  
  if(cpu == sysFtrNumProcessor328) {
    /* set hardware register */
    *((unsigned short*)0xfffffa32) = 0x70 | (gray1<<12) | gray2;
  
    /* save value */
    value = 0x00000000 | 0x70 | (gray1<<12) | gray2;
    FtrSet(CREATOR, 1001, value);
  } else {
    /* set hardware register */
    *((unsigned char*)0xfffffa33) = gray1 | (gray2<<4);
    
    /* save value */
    value = 0x00010000 | gray1 | (gray2<<4);
    FtrSet(CREATOR, 1001, value);
  }
}

Boolean eventhandle(EventPtr event)
{
  FormPtr form = FrmGetActiveForm();
  DWord value;
  Err err;
  int wPalmOSVerMajor;
  long dwVer, cpu;
  DWord newDepth;
  Boolean col;
  short gray1, gray2, max;
  
  /* get cpu type */
  cpu = get_cpu();
  
  /* get current pixel oscillator value */
  err = FtrGet(CREATOR, 1001, &value);
  if(err) {
    if(cpu == sysFtrNumProcessor328)
      value = *((unsigned short*)0xfffffa32);
    else
      value = *((unsigned  char*)0xfffffa33);
  }

  /* get values */
  if(cpu == sysFtrNumProcessor328) {
    gray1 = (value>>12)&0x07;
    gray2 = (value>>0)&0x07;
    max   = 7;
  } else {
    gray1 = (value>>0)&0x0f;
    gray2 = (value>>4)&0x0f;
    max   = 15;
  }

  switch (event->eType) {

  case frmOpenEvent:

    /* get OS version */
    FtrGet(sysFtrCreator, sysFtrNumROMVersion, &dwVer);
    wPalmOSVerMajor = sysGetROMVerMajor(dwVer);
    if (wPalmOSVerMajor < 3) {
      FrmAlert(3100);
      FrmGotoForm(9000);
      return 0;
    }
      
    /* see whether machine supports color modes */
    ScrDisplayMode(scrDisplayModeGetSupportsColor, NULL, NULL, NULL, &col);
    if(col) {
      FrmAlert(3100);
      FrmGotoForm(9000);
      return 0;
    }
      
    /* check for 68328 or 68EZ328 CPU */
    if((cpu != sysFtrNumProcessor328)&&(cpu != sysFtrNumProcessorEZ)) {
      FrmAlert(3100);
      FrmGotoForm(9000);
      return 0;
    }

    /* enable 2bpp grayscale */
    newDepth = 2;
    ScrDisplayMode(scrDisplayModeSet, NULL, NULL, &newDepth, NULL);

    FrmDrawForm(form);
    set_gray(cpu, gray1, gray2);
    return 1;
    break;

  case ctlRepeatEvent:
    switch(event->data.ctlEnter.controlID) {

      /* light gray */
    case 5020:
      gray1--;
      if(gray1 < 0) gray1 = 0;
      set_gray(cpu, gray1, gray2);
      break;

    case 5021:
      gray1++;
      if(gray1 > max) gray1 = max;
      set_gray(cpu, gray1, gray2);
      break;

      /* dark gray */
    case 5030:
      gray2--;
      if(gray2 < 0) gray2 = 0;
      set_gray(cpu, gray1, gray2);
      break;

    case 5031:
      gray2++;
      if(gray2 > max) gray2 = max;
      set_gray(cpu, gray1, gray2);
      break;
    }
    break;
      
  case ctlSelectEvent:
    switch(event->data.ctlEnter.controlID) {
    case 5001:
      /* return to hackmaster */
      ScrDisplayMode(scrDisplayModeSetToDefaults, NULL, NULL, NULL, NULL);
      FrmGotoForm(9000);
      return 1;
      break;

    }

  default:
  }

  FrmHandleEvent(form, event);
  return 1;
}

