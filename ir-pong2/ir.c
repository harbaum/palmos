#include <PalmOS.h>
#include <System/DLServer.h>

#include "Rsc.h"
#include "IR-Pong2.h"

UInt16 ir_ref=0;
IrConnect con;

const char *ir_status_str[]={
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

const char *ir_ias_type_str[]={
  "IAS_ATTRIB_MISSING",
  "IAS_ATTRIB_INTEGER",
  "IAS_ATTRIB_OCTET_STRING",
  "IAS_ATTRIB_USER_STRING",
  "IAS_ATTRIB_UNDEFINED" /* 0xff */
};

const char *ir_ias_return_str[]={
  "Operation successful",
  "No such class",
  "No such attribute",
  "Operation unsupported" /* 0xff */
};

IrIasObject ias_object;
char ias_name[]="Pong2";                     /* Class name */
IrIasAttribute attribs;                      /* A pointer to an array of attributes */
char attr_name[]="IrDA:IrLMP:LsapSel";
char attr_value[]={ IAS_ATTRIB_INTEGER, 0,0,0,42};

IrIasQuery query;
char rlsap_query[]="\005Pong2\022IrDA:IrLMP:LsapSel";

/* Ir stuff: */
char dev_name[]={IR_HINT_PDA, 0, 'P','a','l','m','E'};

#define RESULT_SIZE 32
char result[RESULT_SIZE];

IrPacket packet;
char ir_data[128];

volatile int max_tx=0, pkts=0;

void ir_discon(Boolean complain) {
  IrStatus ret;

  if((ret=IrDisconnectIrLap(ir_ref))!=IR_STATUS_PENDING) {
    if(complain)
      FrmCustomAlert(alt_irdiscon, ir_status_str[ret],0,0);
  }
}

void ir_start_discovery(void) {
  if(IrIsMediaBusy(ir_ref))
    FrmAlert(alt_irbusy);
  else {
    /* search for someone */
    if(IrDiscoverReq(ir_ref, &con)!=IR_STATUS_PENDING)
      FrmAlert(alt_irdiscover);
    else
      btn_set(0);  // disable buttons
  }
}

static void IrIasCallback(IrStatus status) {
  IrStatus ret;

  if(((ret=IrIAS_GetType(&query)) == IAS_ATTRIB_INTEGER)&&
     (query.retCode == IAS_RET_SUCCESS)) {
    con.rLsap = IrIAS_GetIntLsap(&query);
    
    /* build my first IrPacket and buffer, containing the HotSync name */
    DlkGetSyncInfo(0, 0, 0, ir_data, 0, 0);
    packet.buff = ir_data;
    packet.len = dlkUserNameBufSize;
    
    /* open TinyTP/IrLMP session */ 
    if(IrConnectReq(ir_ref, &con, &packet, 0x7f)!=IR_STATUS_PENDING)
      FrmAlert(alt_irconnectl);
  } else {
    if(query.retCode!=IAS_RET_SUCCESS) {
      FrmCustomAlert( alt_iriaserr, ir_ias_return_str[(query.retCode>2)?2:query.retCode], 0,0);

      /* disconnect if needed */
      if(IrIsIrLapConnected(ir_ref))
	if((ret=IrDisconnectIrLap(ir_ref))!=IR_STATUS_PENDING)
	  FrmCustomAlert(alt_irdiscon, ir_status_str[ret],0,0);
    } else
      FrmCustomAlert( alt_iriaserr, ir_ias_type_str[(ret>3)?3:ret],0,0);
  }
}

static void IrCallback(IrConnect *connect, IrCallBackParms *parms) {
  IrStatus ret;
  int i,j;
  static unsigned short message_buffer = 0;

  switch(parms->event) {
  case LEVENT_STATUS_IND:
    break;

 case LEVENT_DISCOVERY_CNF:
   btn_set(1);   // enable buttons

   if(parms->deviceList->nItems == 0) {
     i = FrmAlert(alt_irnoother);
     if(i==1) { /* retry */
       ir_start_discovery();
     } else if(i==2) {
       beam_app();
     } else if(i==3) {
       send_app();
     }
   } else {
     for(j=-1,i=0;i<parms->deviceList->nItems;i++)
       if(StrNCompare(parms->deviceList->dev[i].xid, dev_name, 7)==0)
	 j=i;

     if(j>=0) {
       if(IrConnectIrLap(ir_ref, parms->deviceList->dev[j].hDevice)!=IR_STATUS_PENDING)
	 FrmAlert(alt_irconnect);
     } else {
       i = FrmAlert(alt_irnoother);
       if(i==1) { /* retry */
	 ir_start_discovery();
       } else if(i==2) {
	 beam_app();
       }
     }
   }
   break;

  case LEVENT_LM_CON_IND:
    StrCopy(opponent_name, parms->rxBuff);

    /* accept connection */
    DlkGetSyncInfo(0, 0, 0, ir_data, 0, 0);
    packet.buff = ir_data;
    packet.len = dlkUserNameBufSize;

    max_tx = IrMaxTxSize(ir_ref, &con);

    if((ret=IrConnectRsp(ir_ref, &con, &packet, 0x7f))!=IR_STATUS_PENDING)
      FrmCustomAlert(alt_irrespond, ir_status_str[ret],0,0);
    else {
      for(i=0;i<MAX_BALLS;i++)
	ball_s[i][AX] = 0xff;
      
      start_game(0); /* start game as slave */
      single_game = false;
    }
    break;

  case LEVENT_LAP_DISCON_IND:
  case LEVENT_LM_DISCON_IND:
    stop_game();
    break;

  case LEVENT_LAP_CON_CNF:
    // get rLsap
    query.queryBuf = rlsap_query;
    query.queryLen = StrLen(query.queryBuf);
    query.resultBufSize = RESULT_SIZE;
    query.result = result;
    query.callBack = IrIasCallback;

    if((ret=IrIAS_Query(ir_ref, &query))!=IR_STATUS_PENDING)
      FrmCustomAlert(alt_irquery, ir_status_str[ret], 0,0);      
    break;
    
  case LEVENT_DATA_IND:
    panic = 0;  /* reset panic counter */

    if(state != SETUP) {
      if(i_am_master) {
	MemMove(&slave, parms->rxBuff, sizeof(slave_state)); 

	/* handle message ack */
	update_queue(slave.msg_ack);

	/* normal slave packet */
	if(slave.buttons!=0xff) {
	  /* get paddle_pos etc. from slave */
	  move_paddle(YOU,  PADDLE_MIRROR(slave.paddle_pos)); 
	  
	  slave_buttons = slave.buttons;
	} else if(slave.buttons == 0xff) {
	  
	  /* verify comm version */
	  if(slave.paddle_pos != COMM_VERSION) {
	    /* slave ack with comm version */
	    IrDisconnectIrLap(ir_ref);
	    state = WRONG_VERSION;
	    
	    wrong_version = slave.paddle_pos;
	  } else {
	    for(i=0;i<MAX_BALLS;i++) 
	      ball_state[i] = BALL_NONE;
	    
	    ball_state[0] = BALL_STICK;

	    if(levels[level].dual_ball)
	      ball_state[1] = BALL_STICK;

	    state = RUNNING;

	    KeySetMask(~(keyBitHard1 | keyBitHard2 | keyBitHard3 | keyBitHard4));
	  }
	}
      } else {
	MemMove(&master, parms->rxBuff, sizeof(master_state)); 
	if((master.type>>4) != 0xf) do_sound(master.type>>4);
	
	if((master.type&0x0f) == MSTR_BALL) {
	  state = SLAVE_RUNNING;
	  KeySetMask(~(keyBitHard1 | keyBitHard2 | keyBitHard3 | keyBitHard4));
	  
	  /* ball/paddle coordinates */
	  move_paddle(YOU,  PADDLE_MIRROR(master.paddle_pos)); 	
	  move_ball_slave(master.ball_p, master.shot);
	  
	  /* process messages */
	  if(master.msg != message_buffer) {
	    message_buffer = master.msg;
	    
	    switch((message_buffer>>12)&0x7) {
	    case MSG_REMOVE:
	      check_n_remove_brick(BRICK_ROWS   +1-(message_buffer>>4)&0x0f, 
				   BRICK_COLUMNS-1- message_buffer    &0x0f,0);
	      break;
	    }
	  }
	  
	} else if((master.type&0x0f) == MSTR_END) {
	  if(state != SLAVE_ACK) {
	    /* win/loose information */
	    if(master.paddle_pos != 2) {
	      score[(master.paddle_pos)?ME:YOU]++;
	      loc_score[(master.paddle_pos)?ME:YOU]++;
	      list_add(!master.paddle_pos);
	      draw_score();
	    }
	    
	    slave_level = master.ball_p[0][AX];
	    
	    paddle_pos[ME] = PADDLE_CENTER;
	  }
	  state = SLAVE_ACK;
	} 
      }
    }
    break;

  case LEVENT_LM_CON_CNF:
    StrCopy(opponent_name, parms->rxBuff);

    start_game(1);  /* start game as master */

    max_tx = IrMaxTxSize(ir_ref, &con);
    /* continue with PACKET_HANDLED */

  case LEVENT_PACKET_HANDLED:

    if(!i_am_master) {
      if(max_tx>=sizeof(slave_state)) {
	if(state == SLAVE_ACK) {
	  slave.buttons = 0xff;
	  slave.paddle_pos = COMM_VERSION;
	} else {
	  slave.buttons = slave_buttons;
	  slave.paddle_pos = paddle_pos[ME];  /* send own paddle position */
	}
    
	/* reply to message */
	slave.msg_ack = message_buffer>>8;

	packet.buff = (char*)&slave;
	packet.len = sizeof(slave_state);
      
	/* send it! */ 
	IrDataReq(ir_ref, &con, &packet);
      }
    } else {
      if(max_tx>=sizeof(master_state)) {
	
	/* normal playing state */
	if((state == RUNNING)||(state == SLEEPING)) {
	  master.type = MSTR_BALL;
	  master.paddle_pos = paddle_pos[ME];

	  /* transfer balls position */
	  for(i=0;i<MAX_BALLS;i++) {
	    if(ball_state[i] >= BALL_MOVE) {
	      master.ball_p[i][AX] = BALL_MIRROR_X(ball_p[i][AX] >> BALL_SHIFT);
	      master.ball_p[i][AY] = BALL_MIRROR_Y(ball_p[i][AY] >> BALL_SHIFT);
	    } else
	      master.ball_p[i][AX] = 0xff;
	  }

	  /* transfer shot position */
	  if(shot[0][0]!=0) {
	    master.shot[1][0] = SHOT_MIRROR_X(shot[0][0]);
	    master.shot[1][1] = SHOT_MIRROR_Y(shot[0][1]);
	  } else
	    master.shot[1][0] = 0;

	  if(shot[1][0]!=0) {
	    master.shot[0][0] = SHOT_MIRROR_X(shot[1][0]);
	    master.shot[0][1] = SHOT_MIRROR_Y(shot[1][1]);
	  } else
	    master.shot[0][0] = 0;

	  /* slave shall not draw the balls */
	  if(state == SLEEPING) {
	    for(i=0;i<MAX_BALLS;i++)
	      master.ball_p[i][AX] = 0xff;  
	  }

	  /* end of one game */
	} else if(state == NEW_GAME) {
	  master.type = MSTR_END;
	  master.paddle_pos = winner;
	  master.ball_p[0][AX] = level;
	}

	/* add message */
	master.msg = current_msg();

	/* send sound to slave */
	master.type |= (last_sound<<4);
	last_sound = 0xff;
	
	packet.buff = (char*)&master;
	packet.len = sizeof(master_state);
	
	/* send it! */ 
	IrDataReq(ir_ref, &con, &packet);
      }
    }

    break;

  default:
    /* panic here */
    break;    
  }
}

Boolean ir_open(void) {

  /* open IR library */
  if(SysLibFind(irLibName, &ir_ref)!=0) {
    FrmAlert(alt_libfind);
    return false;
  } else {
    /* ok, so far, so good, now open the library */
    if(IrOpen(ir_ref, 0)!=0) {
      FrmAlert(alt_iropen);
      return false;
    } else {
      /* hey things are working ... */
      if(IrBind(ir_ref, &con, IrCallback)!=IR_STATUS_SUCCESS) {
	FrmAlert(alt_irbind);
	return false;
      } else {
	if(IrSetDeviceInfo(ir_ref, dev_name, 7)!=IR_STATUS_SUCCESS) {
	  FrmAlert(alt_irsetdev);
	  return false;
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
	  if(IrIAS_Add(ir_ref, &ias_object)!=IR_STATUS_SUCCESS) {
	    FrmAlert(alt_iriasadd);
	    return false;
	  }
	}
      }
    }
  }
  return true;
}

void ir_close(void) {

  if (ir_ref!=0) {
    /* close connection (don't warn if that fails, we finish anyway) */
    if(IrIsIrLapConnected(ir_ref))
      IrDisconnectIrLap(ir_ref);

    /* close ir library */
    IrClose(ir_ref);
  }
}
