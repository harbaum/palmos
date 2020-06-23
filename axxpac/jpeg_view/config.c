/*
  config.c - axxpac JPEG plugin

*/


#ifdef OS35HDR
#include <PalmOS.h>
#include <PalmCompatibility.h>
#else
#include <Pilot.h>
#endif


#include "../api/axxPac.h"

#include "jinclude.h"
#include "cdjpeg.h"	
#include "config.h"

void *jpeg_GetObjectPtr(FormPtr frm, Word object) {
  return FrmGetObjectPtr(frm, FrmGetObjectIndex (frm, object));
}

void jpeg_config(void) {

  FormPtr frm;
  LocalID dbId;
  DmOpenRef dbRef=0;
  int i;
  DWord mode;
  
  /* find lib and open it to get access to resources */
  if(((dbId = DmFindDatabase(0, "axxPac JPEG plugin"))==0)||
     ((dbRef = DmOpenDatabase(0, dbId, dmModeReadOnly))==0)) {
    goto cfg_quit;
  }

  frm = FrmInitForm(rsc_config);

  /* try to get current settings */
  if(FtrGet(CREATOR, 0, &mode)) mode = 0;  // use system settings 
  for(i=0;i<4;i++)
    if(mode==i) 
      CtlSetValue(jpeg_GetObjectPtr(frm, rsc_sys+i), 1);

  i = FrmDoDialog(frm);
  FrmDeleteForm(frm);

  if(i==rsc_ok) {
    /* set settings */
    for(i=0;i<4;i++)
      if(CtlGetValue(jpeg_GetObjectPtr(frm, rsc_sys+i)))
	mode = i;

    FtrSet(CREATOR, 0, mode);
  }

cfg_quit:
  if(dbRef != 0) DmCloseDatabase(dbRef);
}

