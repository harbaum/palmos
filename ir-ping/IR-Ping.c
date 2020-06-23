/*
    IR-Ping.c V0.1  -  http://www.ibr.cs.tu-bs.de/~harbaum/pilot/ir_ping.html

    PalmOS3 IrDA stack based IR analyser. This source is intended (despite the fact that
    it is very bad commented) as a base for other IrDA applications. Just rip off the code
    you need and make your own IrDA based application. If you distribute your application
    with source code (i would recomment this, since it helps other people devoloping 
    new applications) state the source of this code somewhere in your source.

    This code is free, but i would like receive a copy of programs based on my code. 

    Till Harbaum

    t.harbaum@tu-bs.de
    http://www.ibr.cs.tu-bs.de/
*/


#include <Pilot.h>
#include <ctype.h>
#include <irlib.h>
#include "callback.h"
#include "IR-PingRsc.h"

#define IAS_DEBUG

#define DEVICE_NAME "TestName"

char *ir_status_str[]={
  "Successful and complete",
  "Operation failed",
  "Successfully started but pending",
  "Link disconnected",
  "No IrLAP Connection exists",
  "IR Media is busy",
  "IR Media is not busy",
  "IrLAP not making progress",
  "No progress condition cleared"
};

char *ir_ias_encoding[]={
  "ASCII","ISO 8859 1","ISO 8859 2","ISO 8859 3","ISO 8859 4",
  "ISO 8859 5","ISO 8859 6","ISO 8859 7","ISO 8859 8","ISO 8859 9", "Unicode"
};

char *ir_event_str[]={
  "LM_CON_IND","LM_DISCON_IND","DATA_IND","PACKET_HANDLED","LAP_CON_IND","LAP_DISCON_IND",
  "DISCOVERY_CNF","LAP_CON_CNF","LM_CON_CNF","STATUS_IND","TEST_IND","TEST_CNF"
};

char *ir_ias_type_str[]={
  "IAS_ATTRIB_MISSING","IAS_ATTRIB_INTEGER","IAS_ATTRIB_OCTET_STRING",
  "IAS_ATTRIB_USER_STRING","IAS_ATTRIB_UNDEFINED" /* 0xff */
};

char *ir_ias_return_str[]={
  "Operation successful","No such class","No such attribute","Operation unsupported" /* 0xff */
};

#define IrPongAppType 'IRPn'

/////////////////////////
// Function Prototypes
/////////////////////////

static void    StartApplication(void);
static Boolean MainFormHandleEvent(EventPtr event);
static Boolean ApplicationHandleEvent(EventPtr event);
static void    EventLoop(void);

/////////////////////////
// StartApplication
/////////////////////////


UInt ir_ref=0;

static MenuBarPtr CurrentMenu = NULL;

/* Ir stuff: */
char dev_name[]={IR_HINT_PDA, 0, 'P','a','l','m','E'};

IrConnect con;
IrIasObject ias_object;
char ias_name[]="Ping";                      /* local Class name */
IrIasAttribute attribs;                      /* A pointer to an array of attributes */
char attr_name[]="IrDA:TinyTP:LsapSel";
char attr_value[]={ IAS_ATTRIB_INTEGER, 0,0,0,42};

#define RESULT_SIZE 128
IrIasQuery query;
char rlsap_dev_query[]="\006Device\012DeviceName";
char rlsap_tp_query[] ="\013IrDA:IrCOMM\023IrDA:TinyTP:LsapSel";
char rlsap_lmp_query[]="\013IrDA:IrCOMM\022IrDA:IrLMP:LsapSel";
char result[RESULT_SIZE];
int  rlsap=-1,rtype=0;

IrPacket packet;
const char test_data[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

int max_tx=0, pkts=0;
int dev_addr_valid=0;
IrDeviceAddr dev_addr;

#define TEXT_TOP   20
#define TEXT_LINES 11

#define MAX_LEN   80
#define MAX_LINES 80

/* sorry you CW guys, i really don't know, how to do this for your compiler */
#define PRINTF(format, args...) { char tmp[MAX_LEN]; StrPrintF(tmp, format, ## args); debug_msg(tmp); }

char text_buffer[MAX_LINES+1][MAX_LEN+1];
int  disp_line, text_height;
WinHandle draw_win;
int  win_dirty;

void init_buffer(void) {
  int i;

  disp_line=win_dirty=0;
  text_height=FntLineHeight()*TEXT_LINES;

  /* clear buffer */
  for(i=0;i<MAX_LINES;i++)
    text_buffer[i][0]=0;
}

static void debug_msg(char *msg) {
  RectangleType rect; 
  int i;

  /* scroll buffer up one line */
  for(i=MAX_LINES;i>0;i--)
    MemMove(text_buffer[i], text_buffer[i-1], MAX_LEN+1);

  /* store new line */
  text_buffer[0][0]=(StrLen(msg)>MAX_LEN)?MAX_LEN:StrLen(msg);
  MemMove(&text_buffer[0][1], msg, text_buffer[0][0]);

  if(draw_win==WinGetActiveWindow()) {
    /* scroll text up */
    rect.topLeft.x=0;  rect.topLeft.y=TEXT_TOP+FntLineHeight(); 
    rect.extent.x=160; rect.extent.y=text_height-FntLineHeight();
    WinCopyRectangle(draw_win, draw_win, &rect, 0, TEXT_TOP, scrCopy);
  
    /* clear new text area */
    rect.topLeft.x=0;  rect.topLeft.y=TEXT_TOP+text_height-FntLineHeight(); 
    rect.extent.x=160; rect.extent.y=FntLineHeight();
    WinEraseRectangle(&rect,0);

    /* draw string */
    WinDrawChars(&text_buffer[disp_line][1], text_buffer[disp_line][0], 0, TEXT_TOP+text_height-FntLineHeight());
  } else
    win_dirty=1;
}

char *arrows[]={"\001","\002","\003","\004"};

void update_disp_line(int offset) {
  FormPtr frm;
  int n;

  n=disp_line+offset;

  if(n<0)  n=0;
  if(n>(MAX_LINES-TEXT_LINES))
    n=MAX_LINES-TEXT_LINES;

  if((n!=disp_line)||(offset==0)) {
    frm = FrmGetActiveForm ();

    if(n==0) {
      CtlSetLabel(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, ScrollDown)), arrows[3]);
      CtlSetEnabled(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, ScrollDown)), false);
    }

    if((n==1)&&(offset>0)) {
      CtlSetLabel(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, ScrollDown)), arrows[1]);
      CtlSetEnabled(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, ScrollDown)), true);
    }

    if(n==(MAX_LINES-TEXT_LINES)) {
      CtlSetLabel(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, ScrollUp)), arrows[2]);
      CtlSetEnabled(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, ScrollUp)), false);
    }

    if((n==(MAX_LINES-TEXT_LINES-1))&&(offset<0)) {
      CtlSetLabel(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, ScrollUp)), arrows[0]);
      CtlSetEnabled(FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, ScrollUp)), true);
    }

    disp_line=n;
  }
}

void scroll_up(void) {
  RectangleType rect; 
  int i;

  update_disp_line(-1);

  if(draw_win==WinGetActiveWindow()) {
    /* scroll text up */
    rect.topLeft.x=0;  rect.topLeft.y=TEXT_TOP+FntLineHeight(); 
    rect.extent.x=160; rect.extent.y=text_height-FntLineHeight();
    WinCopyRectangle(WinGetActiveWindow(),WinGetActiveWindow(), &rect, 0, TEXT_TOP, scrCopy);
    
    /* clear new text area */
    rect.topLeft.x=0;  rect.topLeft.y=TEXT_TOP+text_height-FntLineHeight(); 
    rect.extent.x=160; rect.extent.y=FntLineHeight();
    WinEraseRectangle(&rect,0);
    
    /* draw string */
    WinDrawChars(&text_buffer[disp_line][1], text_buffer[disp_line][0], 0, TEXT_TOP+text_height-FntLineHeight());
  } else
    win_dirty=1;
}

void scroll_down(void) {
  RectangleType rect; 
  int i;

  update_disp_line(+1);

  if(draw_win==WinGetActiveWindow()) {
    /* scroll text down */
    rect.topLeft.x=0;  rect.topLeft.y=TEXT_TOP; 
    rect.extent.x=160; rect.extent.y=text_height-FntLineHeight();
    WinCopyRectangle(WinGetActiveWindow(),WinGetActiveWindow(), &rect, 0, TEXT_TOP+FntLineHeight(), scrCopy);
    
    /* clear new text area */
    rect.topLeft.x=0;  rect.topLeft.y=TEXT_TOP; 
    rect.extent.x=160; rect.extent.y=FntLineHeight();
    WinEraseRectangle(&rect,0);
    
    /* draw string */
    WinDrawChars(&text_buffer[disp_line+TEXT_LINES-1][1], text_buffer[disp_line+TEXT_LINES-1][0], 0, TEXT_TOP);
  } else
    win_dirty=1;
}

void update(void) {
  RectangleType rect; 
  int i;

  if(draw_win==WinGetActiveWindow()) {
    /* clear text area */
    rect.topLeft.x=0;  rect.topLeft.y=TEXT_TOP; 
    rect.extent.x=160; rect.extent.y=text_height;
    WinEraseRectangle(&rect,0);
    
    /* draw text */
    for(i=0;i<TEXT_LINES;i++)
      WinDrawChars(&text_buffer[disp_line+i][1], text_buffer[disp_line+i][0], 0, 
		   TEXT_TOP+text_height-(i+1)*FntLineHeight());

    win_dirty=0;
  }
}

char hexdigits[]="0123456789abcdef";
void hex_dump(char *bytes, int len) {
  int i,j,k;
  char hexstr[64];

  for(i=0; i<1+len/8; i++) {
    for(j=0;(j<8)&&((8*i+j)<len);j++) {
      hexstr[3*j]   = hexdigits[(bytes[8*i+j]>>4)&0x0f];
      hexstr[3*j+1] = hexdigits[bytes[8*i+j]&0x0f];
      hexstr[3*j+2] = ' ';  
    }    
    hexstr[3*j]=0;
    StrCat(hexstr,"    ");

    for(j=0;(j<8)&&((8*i+j)<len);j++) {

      k=StrLen(hexstr);
      if(bytes[8*i+j]>31)
	hexstr[k]=bytes[8*i+j]; 
      else
	hexstr[k]='.';

      hexstr[k+1]=0;
    }    

    debug_msg(hexstr);
  }
}

static void IrIasCallback(IrStatus status) {
  IrStatus ret;
  Byte b;
  int i;

  CALLBACK_PROLOGUE;

  PRINTF("IAS callback: %s", ir_status_str[status]);

  if((query.retCode)==IAS_RET_SUCCESS) {

    i=IrIAS_GetType(&query);
    switch(i) {
    case IAS_ATTRIB_MISSING:
      debug_msg("reply: attrib not found");  
      break;

    case IAS_ATTRIB_INTEGER:
      if(rtype!=0) {
	rlsap = con.rLsap = IrIAS_GetIntLsap(&query);
	PRINTF("got rLsap: %d", con.rLsap);
    
	/* build my first IrPacket and buffer */
	packet.buff = (char*)test_data;
	packet.len = 0;
    
	/* open session */ 
	if((ret=IrConnectReq(ir_ref, &con, &packet, 0x7f))!=IR_STATUS_PENDING)
	  PRINTF("IrConnectReq() failed: %s", ir_status_str[ret]);
	
	rtype=0;
      } else
	PRINTF("received integer: %d", IrIAS_GetInteger(&query));
      
      break;

    case IAS_ATTRIB_USER_STRING:
      b=IrIAS_GetUserStringCharSet(&query);
      PRINTF("recv. %d bytes user str, enc: %s", 
	     IrIAS_GetUserStringLen(&query),
	     ((b>9)&&(b!=0xff))?"unknown":ir_ias_encoding[b]);
      hex_dump(IrIAS_GetUserString(&query),IrIAS_GetUserStringLen(&query));
      break;
      
    case IAS_ATTRIB_OCTET_STRING:
      PRINTF("received %d bytes octet sequence", IrIAS_GetOctetStringLen(&query));
      hex_dump(IrIAS_GetOctetString(&query),IrIAS_GetOctetStringLen(&query));
      break;
      
    default:
      PRINTF("received unknow IAS reply (%d)", i);
      PRINTF("      (%s)", ir_ias_return_str[(query.retCode>3)?3:query.retCode]);
      break;
    }
  } else 
    PRINTF("IAS reply: %s\n", ir_ias_return_str[(query.retCode>3)?3:query.retCode]);

  CALLBACK_EPILOGUE;
}


static void IrCallback(IrConnect *connect, IrCallBackParms *parms) {
  IrStatus ret;
  int i;
  CALLBACK_PROLOGUE;

  PRINTF("Event: %s", ir_event_str[parms->event]);

  switch(parms->event) {
  case LEVENT_STATUS_IND:
    PRINTF("Status = %s", ir_status_str[parms->status]);

    break;

 case LEVENT_DISCOVERY_CNF:
   if(parms->deviceList->nItems > 0) {
     for(i=0;i<parms->deviceList->nItems;i++)
       PRINTF("%d: %lx.%d.%d: %s",i, 
	      parms->deviceList->dev[i].hDevice.u32,
	      parms->deviceList->dev[i].xid[0],
	      parms->deviceList->dev[i].xid[1],
	      &parms->deviceList->dev[i].xid[2]);

     dev_addr.u32=parms->deviceList->dev[0].hDevice.u32;
     dev_addr_valid=1;
   } else {
     debug_msg("no IrDA devices in range");
   }
   break;

  case LEVENT_LM_CON_IND:
    debug_msg("accepting...");

    /* accept connection */
    packet.buff = (char*)test_data;
    packet.len = 0;

    if((ret=IrConnectRsp(ir_ref, &con, &packet, 0x7f))!=IR_STATUS_PENDING)
      PRINTF("IrConnectRsp() failed: %s",ir_status_str[ret]);
    break;

  case LEVENT_LAP_CON_CNF:
    break;
    
  case LEVENT_DATA_IND:
    PRINTF("received %d bytes", parms->rxLen);
    hex_dump(parms->rxBuff, parms->rxLen);
    break;

  case LEVENT_LM_CON_CNF:
    max_tx = IrMaxTxSize(ir_ref, &con);
    PRINTF("max tx size: %d ", max_tx);
    /* continue with PACKET_HANDLED */

  case LEVENT_PACKET_HANDLED:
    break;
  }
  CALLBACK_EPILOGUE;
}

static void StartApplication(void)
{
  /* initialize text buffer */
  init_buffer();

  FrmGotoForm(frm_Main);
}

void open_ir(void) {
  IrStatus ret;

  /* open IR library */
  if(SysLibFind(irLibName, &ir_ref)!=0) {
    PRINTF("SysLibFind(%s) failed", irLibName);
  } else {
    PRINTF("got IR library reference: %d", ir_ref);

    /* ok, so far, so good, now open the library */
    if(IrOpen(ir_ref, 0)!=0) {
      debug_msg("IrOpen() failed");
    } else {
      /* hey things are working ... */
      if(IrBind(ir_ref, &con, IrCallback)!=IR_STATUS_SUCCESS) {
	debug_msg("IrBind() failed");
      } else {
	if(IrSetDeviceInfo(ir_ref, dev_name, 7)!=IR_STATUS_SUCCESS) {
	  debug_msg("IrSetDeviceInfo() failed");
	} else {
//	  IrSetConTypeTTP(&con);  /* set connection type to TinyTP */

	  // not sure if this works to override main name
#ifdef OVERRIDE_MAIN
	  if((ret=IrIAS_SetDeviceName(ir_ref, DEVICE_NAME, StrLen(DEVICE_NAME)))!=IR_STATUS_SUCCESS) {
	    PRINTF("IrIAS_SetDeviceName() failed: %s", ir_status_str[ret]);
	  } else {
#endif
	    ias_object.name = ias_name;
	    ias_object.len  = StrLen(ias_name);
	    ias_object.nAttribs = 1;
	    ias_object.attribs = &attribs;

	    attribs.name=attr_name;            /* Pointer to name of attribute */
            attribs.len=StrLen(attr_name);     /* Length of attribute name */
            attribs.value=attr_value;          /* Hardcode value (see below) */
            attribs.valLen=5;                  /* Length of the value. */
	    attr_value[4]=con.lLsap;

	    // set up our service in IAS db
	    if((ret=IrIAS_Add(ir_ref, &ias_object))!=IR_STATUS_SUCCESS)
	      PRINTF("IrIAS_Add() failed: %s", ir_status_str[ret]);
#ifdef OVERRIDE_MAIN
	  }
#endif
	}
      }
    }
  }
}

int start_discovery(void) {
  IrStatus ret;

  if(IrIsMediaBusy(ir_ref))
    debug_msg("IR media is busy!");
  else {
    /* search for someone */
    if((ret=IrDiscoverReq(ir_ref, &con))!=IR_STATUS_PENDING)
      PRINTF("IrDiscoverReq() failed: %s", ir_status_str[ret]);
  }
}

static Boolean MainFormHandleEvent(EventPtr event)
{
  FormPtr frm;
  IrStatus ret;

  Boolean handled = false;
  
  switch (event->eType) {
  case popSelectEvent: 
    if(event->data.ctlEnter.controlID == CmdPopup) {
      switch(event->data.popSelect.selection) {
      case 0:
	start_discovery();
	break; 

      case 1:
	if(!dev_addr_valid) 
	  debug_msg("No address in discovery buffer!");
	else {
	  PRINTF("connecting IrLap to %lx", dev_addr.u32);
	  if((ret=IrConnectIrLap(ir_ref, dev_addr))!=IR_STATUS_PENDING)
	    PRINTF("IrConnectIrLap() failed: %s", ir_status_str[ret]);
	}
	break;

      case 2:
	if(!IrIsIrLapConnected(ir_ref)) 
	  debug_msg("IrLap is not connected!");
	else {
	  debug_msg("disconnecting IrLap");
	  if((ret=IrDisconnectIrLap(ir_ref))!=IR_STATUS_PENDING)
	    PRINTF("IrDisconnectIrLap() failed: %s", ir_status_str[ret]);	  
	}
	break;

      case 3:
	IrSetConTypeTTP(&con);
	rtype=1;

	// get rLsap
	query.queryBuf = rlsap_tp_query;
	query.queryLen = 32;
	query.resultBufSize = RESULT_SIZE;
	query.result = result;
	query.callBack = IrIasCallback;

	if((ret=IrIAS_Query(ir_ref, &query))!=IR_STATUS_PENDING)
	  PRINTF("IrIAS_Query() failed: %s", ir_status_str[ret]);
	break;
	
      case 4:
	IrSetConTypeLMP(&con);
	rtype=1;
	
	// get rLsap
	query.queryBuf = rlsap_lmp_query;
	query.queryLen = 31;
	query.resultBufSize = RESULT_SIZE;
	query.result = result;
	query.callBack = IrIasCallback;
	
	if((ret=IrIAS_Query(ir_ref, &query))!=IR_STATUS_PENDING)
	  PRINTF("IrIAS_Query() failed: %s", ir_status_str[ret]);
	break;

      case 5:
	if(!dev_addr_valid) 
	  debug_msg("No address in discovery buffer!");
	else {
	  /* build packet with test data */
	  packet.buff = (char*)test_data;
	  packet.len = 16;

	  if((ret=IrTestReq(ir_ref, dev_addr, &con, &packet))!=IR_STATUS_PENDING)
	    PRINTF("IrTestReq() failed: %s", ir_status_str[ret]);
	}
	break;

      case 6:
	packet.buff = (char*)test_data;
	packet.len = 16;
      
	/* send it! */ 
	if((ret=IrDataReq(ir_ref, &con, &packet))!=IR_STATUS_PENDING)
	  PRINTF("IrDataReq() failed: %s", ir_status_str[ret]);    
	break;

      case 7:
	PRINTF("max. rx size: %d bytes",IrMaxRxSize(ir_ref, &con));
	break;

      case 8:
	PRINTF("max. tx size: %d bytes",IrMaxTxSize(ir_ref, &con));
	break;

      case 9:
	// get Device Name
	query.queryBuf = rlsap_dev_query;
	query.queryLen = 18;
	query.resultBufSize = RESULT_SIZE;
	query.result = result;
	query.callBack = IrIasCallback;
	
	if((ret=IrIAS_Query(ir_ref, &query))!=IR_STATUS_PENDING)
	  PRINTF("IrIAS_Query() failed: %s", ir_status_str[ret]);
	break;
      }
      handled=true;
    }
    break;
    
  case ctlRepeatEvent:
    if(event->data.ctlEnter.controlID == ScrollUp)
      scroll_down();

    if(event->data.ctlEnter.controlID == ScrollDown)
      scroll_up();
    break;

  case frmOpenEvent:
    FrmDrawForm(FrmGetActiveForm());
    draw_win=WinGetDrawWindow();
    update_disp_line(0);

    debug_msg("IR Ping V0.1 (c)'98 Till Harbaum");
    debug_msg("www.ibr.cs.tu-bs.de/~harbaum/pilot");
    debug_msg("");

    open_ir();

    handled = true;
    break;

    case nilEvent:
      if(win_dirty)
	update();
      handled = true;
      break;
  }
  
  return(handled);
}

/////////////////////////
// ApplicationHandleEvent
/////////////////////////

static Boolean ApplicationHandleEvent(EventPtr event)
{
  FormPtr form;
  Word    formId;
  Boolean handled = false;
  
  if (event->eType == frmLoadEvent) {
    formId = event->data.frmLoad.formID;
    form = FrmInitForm(formId);
    FrmSetActiveForm(form);
    
    switch (formId) {
    case frm_Main:
      FrmSetEventHandler(form, MainFormHandleEvent);
      break;
    }
    handled = true;
  }

  if ((event->eType == appStopEvent)&&(ir_ref!=0)) {
    /* close ir library */
    IrClose(ir_ref);
  }

  return handled;
}

/////////////////////////
// EventLoop
/////////////////////////

static void EventLoop(void)
{
    EventType event;
    Word      error;

    do
    {
        EvtGetEvent(&event, 100);
        if (SysHandleEvent(&event))
            continue;
        if (MenuHandleEvent(CurrentMenu, &event, &error))
            continue;
        if (ApplicationHandleEvent(&event))
            continue;
        FrmDispatchEvent(&event);
    }
    while (event.eType != appStopEvent);
}

/////////////////////////
// PilotMain
/////////////////////////

DWord PilotMain(Word cmd, Ptr cmdBPB, Word launchFlags)
{
    if (cmd == sysAppLaunchCmdNormalLaunch)
    {
        StartApplication();
        EventLoop();
        return(0);
    }
    return(0);
}











