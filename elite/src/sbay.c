/*
  Subspace trading system

  sbay.c
*/

#ifdef SBAY

#include <PalmOS.h>
#include "sections.h"

#include "config.h"
#include "elite.h"
#include "planet.h"
#include "debug.h"
#include "trade.h"
#include "gfx.h"
#include "options.h"
#include "sound.h"

#include "sbay.h"

static void display_price_n_quantity(void) SECDK;
static void IrCallback(IrConnect *connect, IrCallBackParms *parms) SECDK;

UInt16 ir_ref=0;
IrConnect con;

IrIasObject ias_object;
char ias_name[]="Elite";       /* Class name */
IrIasAttribute attribs;        /* A pointer to an array of attributes */
char attr_name[]="IrDA:IrLMP:LsapSel";
char attr_value[]={ IAS_ATTRIB_INTEGER, 0,0,0,42};

IrIasQuery query;
char rlsap_query[]="\005Elite\022IrDA:IrLMP:LsapSel";

/* Ir stuff: */
char dev_name[]={IR_HINT_PDA, 0, 'P','a','l','m','E'};

#define RESULT_SIZE 32
char result[RESULT_SIZE];

IrPacket packet;
char ir_data[128];

volatile int max_tx=0, pkts=0;

Boolean sbay_enabled = false;

static void IrCallback(IrConnect *connect, IrCallBackParms *parms) {
  IrStatus ret;
  int i, j;
  
  switch(parms->event) {
    case LEVENT_STATUS_IND:
      break;
      
    case LEVENT_DISCOVERY_CNF:
      if(parms->deviceList->nItems == 0) {
	/* ERR: no other trader */
      } else {
	/* search for matching device */
	for(j=-1,i=0;i<parms->deviceList->nItems;i++)
	  if(StrNCompare(parms->deviceList->dev[i].xid, dev_name, 7)==0)
	    j=i;
	
	if(j>=0) {
	  if(IrConnectIrLap(ir_ref, parms->deviceList->dev[j].hDevice) !=
	     IR_STATUS_PENDING) {
	    /* ERR: connect failed */
	  } 
	} else {
	  /* ERR: no other trader */
	}
      }
      break;
      
    case LEVENT_LM_CON_IND:
      /* parms->rxBuff, connection est.,  */
      max_tx = IrMaxTxSize(ir_ref, &con);
      
      if((ret=IrConnectRsp(ir_ref, &con, &packet, 0x7f))!=IR_STATUS_PENDING) {
	/* unable to respond */
      } else {
	/* client ok */
      }
      break;
      
    case LEVENT_LAP_DISCON_IND:
    case LEVENT_LM_DISCON_IND:
      /* connection */
      break;
      
    case LEVENT_LAP_CON_CNF:
      // get rLsap
      query.queryBuf = rlsap_query;
      query.queryLen = StrLen(query.queryBuf);
      query.resultBufSize = RESULT_SIZE;
      query.result = result;
//    query.callBack = IrIasCallback;  // FIXME
      
      if((ret=IrIAS_Query(ir_ref, &query))!=IR_STATUS_PENDING) {
	/* inquiry failed */
      }
      break;
      
    case LEVENT_DATA_IND:
      /* parms->rxBuff */
    break;
    
    case LEVENT_LM_CON_CNF:
      /* parms->rxBuff von client */
      max_tx = IrMaxTxSize(ir_ref, &con);
      /* continue with PACKET_HANDLED */
      
    case LEVENT_PACKET_HANDLED:
#if 0
      packet.buff = (char*)&master;
      packet.len = sizeof(master_state);
      
      /* send it! */ 
      IrDataReq(ir_ref, &con, &packet);
#endif
      break;
      
    default:
      /* panic here */
      break;    
  }
}

void 
sbay_init(void) {

  /* open IR library */
  if(SysLibFind(irLibName, &ir_ref)!=0) {
    return;
  } else {
    /* ok, so far, so good, now open the library */
    if(IrOpen(ir_ref, 0)!=0) {
      return;
    } else {
      /* hey things are working ... */
      if(IrBind(ir_ref, &con, IrCallback)!=IR_STATUS_SUCCESS) {
	return;
      } else {
	if(IrSetDeviceInfo(ir_ref, dev_name, 7)!=IR_STATUS_SUCCESS) {
	  return;
	} else {
	  IrSetConTypeLMP(&con);  /* set connection type to IrLMP */

	  ias_object.name = ias_name;
	  ias_object.len  = StrLen(ias_name);
	  ias_object.nAttribs = 1;
	  ias_object.attribs = &attribs;
	  
	  attribs.name=attr_name;            /* Pointer to name of attribute */
	  attribs.len=StrLen(attr_name);     /* Length of attribute name */
	  attribs.value=attr_value;          /* Hardcode value */
	  attribs.valLen=5;                  /* Length of the value. */
	  attr_value[4]=con.lLsap;
	  
	  // set up our service in IAS db
	  if(IrIAS_Add(ir_ref, &ias_object)!=IR_STATUS_SUCCESS)
	    return;
	}
      }
    }
  }
  sbay_enabled = true;
}

void 
sbay_free(void) {
  if (ir_ref!=0) {
    /* close connection (don't warn if that fails, we finish anyway) */
    if(IrIsIrLapConnected(ir_ref))
      IrDisconnectIrLap(ir_ref);
    
    /* close ir library */
    IrClose(ir_ref);
  }
}

/* states of sbay transfer */
#define SBAY_PASSIVE          0
#define SBAY_PRICE_N_QUANTITY 0

static UInt8 sbay_state = SBAY_PASSIVE;
static UInt16 trade_item, trade_price, trade_quant;

#define SBAY_PNQ_X         (4)
#define SBAY_PNQ_W     (30)
#define SBAY_PNQ_X2    (DSP_WIDTH-1-SBAY_PNQ_X-SBAY_PNQ_W)
#define SBAY_PNQ_X1    (SBAY_PNQ_X2-5-SBAY_PNQ_W)

#define SBAY_PNQ_SKIP  4
#define SBAY_PNQ_Y_I   (TITLE_LINE_Y+4+0*SBAY_PNQ_SKIP+GFX_CELLS(0,2))
#define SBAY_PNQ_Y_Q1  (TITLE_LINE_Y+4+1*SBAY_PNQ_SKIP+GFX_CELLS(1,2))
#define SBAY_PNQ_Y_Q2  (TITLE_LINE_Y+3+1*SBAY_PNQ_SKIP+GFX_CELLS(2,2))
#define SBAY_PNQ_Y_P1  (TITLE_LINE_Y+4+2*SBAY_PNQ_SKIP+GFX_CELLS(3,2))
#define SBAY_PNQ_Y_P2  (TITLE_LINE_Y+3+2*SBAY_PNQ_SKIP+GFX_CELLS(4,2))
#define SBAY_PNQ_Y_P3  (TITLE_LINE_Y+3+2*SBAY_PNQ_SKIP+GFX_CELLS(5,2))
#define SBAY_PNQ_Y_T   (TITLE_LINE_Y+4+3*SBAY_PNQ_SKIP+GFX_CELLS(6,2))
#define SBAY_PNQ_Y_F   (TITLE_LINE_Y+6+3*SBAY_PNQ_SKIP+GFX_CELLS(7,2))
#define SBAY_PNQ_Y_S   (TITLE_LINE_Y+8+3*SBAY_PNQ_SKIP+GFX_CELLS(8,2))

#define SBAY_PNQ_XE    (DSP_WIDTH-SBAY_PNQ_X-1)

static void display_price_n_quantity(void) {
  const char *unit_name[] = { "t", "kg", "g" };
  char str[32];
  UInt16 sum;

  /* draw setup screen */
  gfx_draw_title("PREPARE SUBSPACE TRADING", 1);

  /* display item type */
  gfx_display_text(SBAY_PNQ_X, SBAY_PNQ_Y_I, ALIGN_LEFT, 
		   "Item to trade:", GFX_COL_GREEN_1);
  gfx_display_text(SBAY_PNQ_XE, SBAY_PNQ_Y_I, ALIGN_RIGHT, 
		   stock_market[trade_item].name, GFX_COL_WHITE);
  
  /* display quantity */
  gfx_display_text(SBAY_PNQ_X, SBAY_PNQ_Y_Q1, ALIGN_LEFT, 
		   "Quantity:", GFX_COL_GREEN_1);
  StrPrintF(str, "%d of %d%s", trade_quant, cmdr.current_cargo[trade_item], 
	    unit_name[stock_market[trade_item].units]);
  gfx_display_text(SBAY_PNQ_XE, SBAY_PNQ_Y_Q1, ALIGN_RIGHT, 
		   str, GFX_COL_WHITE);

  /* draw up/down buttons if required */
  draw_option_button(SBAY_PNQ_X1, SBAY_PNQ_Y_Q2, SBAY_PNQ_X1+SBAY_PNQ_W, 
		     "-1", GFX_COL_RED, GFX_COL_DARK_RED);
  
  draw_option_button(SBAY_PNQ_X2, SBAY_PNQ_Y_Q2, SBAY_PNQ_X2+SBAY_PNQ_W, 
		     "+1", GFX_COL_GREEN_2, GFX_COL_GREEN_4);

  /* display price */
  gfx_display_text(SBAY_PNQ_X, SBAY_PNQ_Y_P1, ALIGN_LEFT, 
		   "Price:", GFX_COL_GREEN_1);
  StrPrintF(str, "%d.%d" CURRENCY "\014%s", 
	    trade_price/10, trade_price%10,
	    unit_name[stock_market[trade_item].units]);

  /* display red, when the price is lower then the current price */
  gfx_display_text(SBAY_PNQ_XE, SBAY_PNQ_Y_P1, ALIGN_RIGHT, str, 
		   (trade_price < stock_market[trade_item].current_price)?
		   GFX_COL_RED:GFX_COL_WHITE);

  /* add local market price */
  StrPrintF(str, "(local: %d.%d" CURRENCY "\014%s)", 
	    stock_market[trade_item].current_price/10,
	    stock_market[trade_item].current_price%10,
	    unit_name[stock_market[trade_item].units]);
  gfx_display_text(SBAY_PNQ_X, SBAY_PNQ_Y_P2, ALIGN_LEFT, 
		   str, GFX_COL_GREEN_1);

  /* draw up/down buttons if required */
  draw_option_button(SBAY_PNQ_X1, SBAY_PNQ_Y_P2, SBAY_PNQ_X1+SBAY_PNQ_W, 
		     "-0.1",  GFX_COL_RED, GFX_COL_DARK_RED);

  draw_option_button(SBAY_PNQ_X1, SBAY_PNQ_Y_P3, SBAY_PNQ_X1+SBAY_PNQ_W, 
		     "-1.0",  GFX_COL_RED, GFX_COL_DARK_RED);

  draw_option_button(SBAY_PNQ_X2, SBAY_PNQ_Y_P2, SBAY_PNQ_X2+SBAY_PNQ_W, 
		     "+0.1", GFX_COL_GREEN_2, GFX_COL_GREEN_4);

  draw_option_button(SBAY_PNQ_X2, SBAY_PNQ_Y_P3, SBAY_PNQ_X2+SBAY_PNQ_W, 
		     "+1.0", GFX_COL_GREEN_2, GFX_COL_GREEN_4);

  /* summ up the overall price */
  sum = trade_price * trade_quant;

  /* display total price */
  gfx_draw_line(SBAY_PNQ_X, SBAY_PNQ_Y_T-2, SBAY_PNQ_XE, SBAY_PNQ_Y_T-2);
  gfx_display_text(SBAY_PNQ_X, SBAY_PNQ_Y_T, ALIGN_LEFT, 
		   "Total:", GFX_COL_GREEN_1);  
  StrPrintF(str, "%d.%d" CURRENCY, sum/10, sum%10);
  gfx_display_text(SBAY_PNQ_XE, SBAY_PNQ_Y_T, ALIGN_RIGHT, 
		   str, GFX_COL_WHITE);  

  /* fee is 1%, 0.5 min */
  sum/=100; if(sum<5) sum = 5;

  /* display fee */
  gfx_display_text(SBAY_PNQ_X, SBAY_PNQ_Y_F, ALIGN_LEFT, 
		   "Offer fee:", GFX_COL_GREEN_1);
  StrPrintF(str, "%d.%d" CURRENCY , sum/10, sum%10);
  gfx_display_text(SBAY_PNQ_XE, SBAY_PNQ_Y_F, ALIGN_RIGHT, str, GFX_COL_WHITE);

  /* draw start button */
  draw_option_button((DSP_WIDTH-80)/2, SBAY_PNQ_Y_S, (DSP_WIDTH+80)/2, 
		     "Transmit offer", 
		     (cmdr.credits >= sum)?GFX_COL_GREEN_2:GFX_COL_RED, 
		     (cmdr.credits >= sum)?GFX_COL_GREEN_4:GFX_COL_DARK_RED);
  
  /* draw sbay logo */
  gfx_draw_sprite(SPRITE_SBAY, SBAY_PNQ_X, SBAY_PNQ_Y_S);
}

void sbay_sell_stock(int item) {
  snd_click();

  if ((!is_docked) || (cmdr.current_cargo[item] == 0)) return;

  /* start trading at these values */
  trade_item  = item;
  trade_quant = cmdr.current_cargo[item];
  trade_price = stock_market[item].current_price;

  /* draw sbay main screen */
  current_screen = SCR_SBAY;

  /* reset sbay state */
  sbay_state = SBAY_PRICE_N_QUANTITY;

  display_price_n_quantity();
}

/* handle screen taps while in sbay mode */
void sbay_handle_tap(UInt8 x, UInt8 y) {
  UInt16 sum;

  switch(sbay_state) {
    case SBAY_PRICE_N_QUANTITY:
 
      /* tapped into quantity line */
      if((y>=SBAY_PNQ_Y_Q2)&&(y<=SBAY_PNQ_Y_Q2+GFX_CELL_HEIGHT)) {
	/* the down button */
	if((x>=SBAY_PNQ_X1)&&(x<=SBAY_PNQ_X1+SBAY_PNQ_W)) {
	  snd_click();

	  if(trade_quant>1) {
	    trade_quant--;
	    display_price_n_quantity();
	  }
	  return;
	}
	/* the up button */
	if((x>=SBAY_PNQ_X2)&&(x<=SBAY_PNQ_X2+SBAY_PNQ_W)) {
	  snd_click();

	  if(trade_quant < cmdr.current_cargo[trade_item]) {
	    trade_quant++;
	    display_price_n_quantity();
	  }
	  return;
	}
      }

      /* tapped into price line 1 */
      if((y>=SBAY_PNQ_Y_P2)&&(y<=SBAY_PNQ_Y_P2+GFX_CELL_HEIGHT)) {
	/* the down 1 button */
	if((x>=SBAY_PNQ_X1)&&(x<=SBAY_PNQ_X1+SBAY_PNQ_W)) {
	  snd_click();

	  if(trade_price > 1) {
	    trade_price--;
	    display_price_n_quantity();
	  }
	  return;
	}
	/* the up 1 button */
	if((x>=SBAY_PNQ_X2)&&(x<=SBAY_PNQ_X2+SBAY_PNQ_W)) {
	  snd_click();

	  if(trade_price < 999) {
	    trade_price++;
	    display_price_n_quantity();
	  }
	  return;
	}
      }

      /* tapped into price line 10 */
      if((y>=SBAY_PNQ_Y_P3)&&(y<=SBAY_PNQ_Y_P3+GFX_CELL_HEIGHT)) {
	/* the down 10 button */
	if((x>=SBAY_PNQ_X1)&&(x<=SBAY_PNQ_X1+SBAY_PNQ_W)) {
	  snd_click();

	  if(trade_price > 10) {
	    trade_price-=10;
	    display_price_n_quantity();
	  }
	  return;
	}
	/* the up 10 button */
	if((x>=SBAY_PNQ_X2)&&(x<=SBAY_PNQ_X2+SBAY_PNQ_W)) {
	  snd_click();

	  if(trade_price < 909) {
	    trade_price+=10;
	    display_price_n_quantity();
	  }
	  return;
	}
      }

      /* tapped start button */
      if((y>=SBAY_PNQ_Y_S)&&(y<=SBAY_PNQ_Y_S+GFX_CELL_HEIGHT)&&
	 (x>=(DSP_WIDTH-80)/2)&&(x<=(DSP_WIDTH+80)/2)) {
	snd_click();

	sum = (trade_price * trade_quant)/100;
	if(sum<5) sum = 5;

	if(cmdr.credits >= sum) {
	  DEBUG_PRINTF("start\n");
	}
      }

      break;
  }
}

#if 0
  /* display quantity */
  gfx_display_text(SBAY_PNQ_X, SBAY_PNQ_Y_Q, ALIGN_LEFT, "Quantity:", GFX_COL_GREEN_1);
  StrPrintF(str, "%d of %d%s", trade_quant, cmdr.current_cargo[trade_item], 
	    unit_name[stock_market[trade_item].units]);
  gfx_display_text(SBAY_PNQ_X, SBAY_PNQ_Y_Q, ALIGN_RIGHT, str, GFX_COL_WHITE);

  /* draw up/down buttons if required */
  if(trade_quant > 1) 
    gfx_display_text(SBAY_PNQ_X_D, SBAY_PNQ_Y_Q, ALIGN_LEFT, "\004", GFX_COL_RED);

    if(trade_quant < cmdr.current_cargo[trade_item])
gfx_display_text(SBAY_PNQ_X_U, SBAY_PNQ_Y_Q, ALIGN_LEFT, 
		 "\003", GFX_COL_GREEN_1);

  /* display price */
  gfx_display_text(SBAY_PNQ_X, SBAY_PNQ_Y_P, ALIGN_LEFT, 
		   "Price:", GFX_COL_GREEN_1);
  StrPrintF(str, "%d.%d" CURRENCY "\014%s", 
	    trade_price/10, trade_price%10,
	    unit_name[stock_market[trade_item].units]);

  /* display red, when the price is lower then the current price */
  if(trade_price >= stock_market[trade_item].current_price)
    gfx_display_text(SBAY_PNQ_X, SBAY_PNQ_Y_P, ALIGN_RIGHT, str, GFX_COL_WHITE);
  else
    gfx_display_text(SBAY_PNQ_X, SBAY_PNQ_Y_P, ALIGN_RIGHT, str, GFX_COL_RED);

  /* draw up/down buttons if required */
  if(trade_price > 0) 
    gfx_display_text(SBAY_PNQ_X_D, SBAY_PNQ_Y_P, ALIGN_LEFT, "\004", GFX_COL_RED);

  if(trade_price > 9) 
    gfx_display_text(SBAY_PNQ_X_FD, SBAY_PNQ_Y_P, ALIGN_LEFT, "\004", GFX_COL_RED);

  if(trade_price < 999) // up to 99.9 per unit
    gfx_display_text(SBAY_PNQ_X_U, SBAY_PNQ_Y_P, ALIGN_LEFT, "\003", GFX_COL_GREEN_1);

  if(trade_price < 909) // up to 99.9 per unit
    gfx_display_text(SBAY_PNQ_X_FU, SBAY_PNQ_Y_P, ALIGN_LEFT, "\003", GFX_COL_GREEN_1);

  /* summ up the overall price */
  sum = trade_price * trade_quant;

  gfx_draw_line(100, TITLE_LINE_Y+2+3*GFX_CELL_HEIGHT+1, 
		SBAY_PNQ_X, TITLE_LINE_Y+2+3*GFX_CELL_HEIGHT+1);
  StrPrintF(str, "%d.%d" CURRENCY, sum/10, sum%10);
  gfx_display_text(SBAY_PNQ_X,TITLE_LINE_Y+2+3*GFX_CELL_HEIGHT+3, 
		   ALIGN_RIGHT, str, GFX_COL_WHITE);  

#endif

#endif
