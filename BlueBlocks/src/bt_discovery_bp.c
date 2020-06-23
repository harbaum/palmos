/*
  bt_discovery_pb.c

  a more user friendly discovery dialog
  
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

#define BAR_WIDTH   100
#define BAR_HEIGHT   20

/* draw some kind of progess bar */
void bt_progress_bar(Boolean init, char *str, int percent) {
  RectangleType rect;
  char pstr[5];
  int  x,y;
  static int last_perc = 0;

  if(percent<0)   percent=0;
  if(percent>100) percent=100;

  x = 80; y = 80-(65/2)+34;

  if(init) {
    /* draw border */
    rect.topLeft.x = 80-(BAR_WIDTH/2);
    rect.topLeft.y = 80-(BAR_HEIGHT/2);
    rect.extent.x = BAR_WIDTH;
    rect.extent.y = BAR_HEIGHT;

    WinEraseRectangle(&rect, 3);
    WinDrawRectangleFrame(dialogFrame, &rect);

    WinDrawChars(str, StrLen(str), 80-(BAR_WIDTH/2)+10, 80-(BAR_HEIGHT/2)+5);

    /* draw box */
    rect.topLeft.x = x-(BAR_WIDTH/2)+10;
    rect.topLeft.y = y;
    rect.extent.x = BAR_WIDTH-20;
    rect.extent.y = 11;
    WinEraseRectangle(&rect, 0);
    WinDrawRectangleFrame(rectangleFrame, &rect);
  }

  /* bar has to be drawn if percentage changes or the bar is new */
  if((percent != *lp)||(init)) {

    /* erase inards */
    rect.topLeft.x = x-(BAR_WIDTH/2)+11;
    rect.topLeft.y = y+1;
    rect.extent.x = BAR_WIDTH-22;
    rect.extent.y = 9;
    WinEraseRectangle(&rect, 0);
    
    /* draw bar */
    rect.topLeft.x = x-(BAR_WIDTH/2)+11;
    rect.topLeft.y = y+1;
    rect.extent.x = (percent*(BAR_WIDTH-22))/100;
    rect.extent.y = 9;
    WinDrawRectangle(&rect, 0);

    /* new percentage */
    StrPrintF(pstr, "%d%%", percent);
    WinInvertChars(pstr, StrLen(pstr), 
		   x-(FntLineWidth(pstr, StrLen(pstr))/2),y);
  
    last_perc = percent;
  }
}
