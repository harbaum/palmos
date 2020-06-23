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

#define BAR_WIDTH   (148)
#define BAR_X       (78)
#define BAR_Y       (158-32)

/* draw some kind of progess bar */
void bt_discovery_pb(Boolean init, int percent) {
  RectangleType rect;
  char pstr[5];
  static int last_perc = 0;
  static UInt32 gStartSeconds;

  FntSetFont(1);

  if(init) {
    // Kickstart the progress bar
    gStartSeconds = TimGetSeconds();

    /* draw box */
    rect.topLeft.x = BAR_X-(BAR_WIDTH/2);
    rect.topLeft.y = BAR_Y;
    rect.extent.x = BAR_WIDTH;
    rect.extent.y = 11;
    WinEraseRectangle(&rect, 0);
    WinDrawRectangleFrame(rectangleFrame, &rect);
  }

  if(percent == BT_PB_TIME) {
    /* make percent from time */
    percent = 10 * (TimGetSeconds()- gStartSeconds);

    DEBUG_GREEN_PRINTF("timed percent: %d\n", percent);   
  }

  if(percent < 0)   percent = 0;
  if(percent > 100) percent = 100;

  /* bar has to be drawn if percentage changes or the bar is new */
  if((percent != last_perc)||(init)) {

    /* erase inards */
    rect.topLeft.x = BAR_X-(BAR_WIDTH/2)+1;
    rect.topLeft.y = BAR_Y+1;
    rect.extent.x = BAR_WIDTH-2;
    rect.extent.y = 9;
    WinEraseRectangle(&rect, 0);
    
    /* draw bar */
    rect.topLeft.x = BAR_X-(BAR_WIDTH/2)+1;
    rect.topLeft.y = BAR_Y+1;
    rect.extent.x = (percent*(BAR_WIDTH-2))/100;
    rect.extent.y = 9;
    WinDrawRectangle(&rect, 0);

    /* new percentage */
    StrPrintF(pstr, "%d%%", percent);
    WinInvertChars(pstr, StrLen(pstr), 
		   BAR_X-(FntLineWidth(pstr, StrLen(pstr))/2),BAR_Y);
  
    last_perc = percent;
  }
}
