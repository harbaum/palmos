/*
  bluetooth communication routines 
*/

#include <PalmOS.h>
#include <System/DLServer.h>

#include <BtLib.h>
#include <BtLibTypes.h>

#include "Rsc.h"
#include "IR-Pong2.h"

/* internal state to react on disconnects */
#define DISCONNECTED   0
#define CONNECTED      1
#define DISC_REQUESTED 2
#define LISTENING      3

static UInt16 btLibRefNum=0;

static BtLibSocketRef socketSdp=-1;
static BtLibSocketRef socketL2cap=-1;

static BtLibSdpRecordHandle recH = 0;

static UInt16 l2state = DISCONNECTED;         // possible L2CAP disc is on

Boolean ACLcon = false;                // ACL connection is establi

static const char appname[]=APPNAME;

static BtLibDeviceAddressType device;         // address of bt device

/* generate by linux uuidgen */
static const UInt8 irPong2Uuid[16] = { 
  0x57, 0xd2, 0x03, 0x34, 
  0xa1, 0xc9,
  0x42, 0x1d,
  0xaa, 0x6f,
  0x71, 0x55, 0xc6, 0x60, 0xd2, 0xf7
};

static unsigned short message_buffer = 0;

static BtLibSdpUuidType serviceList[1];

static Boolean send_complete = false;

typedef struct {
  Err err;
  char *str;
} err_entry;

static const err_entry err_entries[] = {
  { btLibErrNoError,                     "btLibErrNoError" },
  { btLibErrError,                       "ErrError"},
  { btLibErrNotOpen,                     "ErrNotOpen"},
  { btLibErrBluetoothOff,                "ErrBluetoothOff"},
  { btLibErrNoPrefs,                     "ErrNoPrefs"},
  { btLibErrAlreadyOpen,                 "ErrAlreadyOpen"},
  { btLibErrOutOfMemory,                 "ErrOutOfMemory"},
  { btLibErrFailed,                      "ErrFailed"},
  { btLibErrInProgress,                  "ErrInProgress"},
  { btLibErrParamError,                  "ErrParamError"},
  { btLibErrTooMany,                     "ErrTooMany"},
  { btLibErrPending,                     "ErrPending"},
  { btLibErrNotInProgress,               "ErrNotInProgress"},
  { btLibErrRadioInitFailed,             "ErrRadioInitFailed"},
  { btLibErrRadioFatal,                  "ErrRadioFatal"},
  { btLibErrRadioInitialized,            "ErrRadioInitialized"},
  { btLibErrRadioSleepWake,              "ErrRadioSleepWake"},
  { btLibErrNoConnection,                "ErrNoConnection"},
  { btLibErrAlreadyRegistered,           "ErrAlreadyRegistered"},
  { btLibErrNoAclLink,                   "ErrNoAclLink"},
  { btLibErrSdpRemoteRecord,             "ErrSdpRemoteRecord"},
  { btLibErrSdpAdvertised,               "ErrSdpAdvertised"},
  { btLibErrSdpFormat,                   "ErrSdpFormat"},
  { btLibErrSdpNotAdvertised,            "ErrSdpNotAdvertised"},
  { btLibErrSdpQueryVersion,             "ErrSdpQueryVersion"},
  { btLibErrSdpQueryHandle,              "ErrSdpQueryHandle"},
  { btLibErrSdpQuerySyntax,              "ErrSdpQuerySyntax"},
  { btLibErrSdpQueryPduSize,             "ErrSdpQueryPduSize"},
  { btLibErrSdpQueryContinuation,        "ErrSdpQueryContinuation"},
  { btLibErrSdpQueryResources,           "ErrSdpQueryResources"},
  { btLibErrSdpQueryDisconnect,          "ErrSdpQueryDisconnect"},
  { btLibErrSdpInvalidResponse,          "ErrSdpInvalidResponse"},
  { btLibErrSdpAttributeNotSet,          "ErrSdpAttributeNotSet"},
  { btLibErrSdpMapped,                   "ErrSdpMapped"},
  { btLibErrSocket,                      "ErrSocket"},
  { btLibErrSocketProtocol,              "ErrSocketProtocol"},
  { btLibErrSocketRole,                  "ErrSocketRole"},
  { btLibErrSocketPsmUnavailable,        "ErrSocketPsmUnavailable"},
  { btLibErrSocketChannelUnavailable,    "ErrSocketChannelUnavailable"},
  { btLibErrSocketUserDisconnect,        "ErrSocketUserDisconnect"},
  { btLibErrCanceled,                    "ErrCanceled"},
  { btLibErrBusy,                        "ErrBusy"},
  { btLibMeStatusUnknownHciCommand,      "MeStatusUnknownHciCommand"},
  { btLibMeStatusNoConnection,           "MeStatusNoConnection"},
  { btLibMeStatusHardwareFailure,	 "MeStatusHardwareFailure"}, 
  { btLibMeStatusPageTimeout,            "MeStatusPageTimeout"},
  { btLibMeStatusAuthenticateFailure,    "MeStatusAuthenticateFailure"},
  { btLibMeStatusMissingKey,             "MeStatusMissingKey"},
  { btLibMeStatusMemoryFull,             "MeStatusMemoryFull"},
  { btLibMeStatusConnnectionTimeout,     "MeStatusConnnectionTimeout"},
  { btLibMeStatusMaxConnections,         "MeStatusMaxConnections"},
  { btLibMeStatusMaxScoConnections,      "MeStatusMaxScoConnections"},
  { btLibMeStatusMaxAclConnections,      "MeStatusMaxAclConnections"},
  { btLibMeStatusCommandDisallowed,      "MeStatusCommandDisallowed"},
  { btLibMeStatusLimitedResources,       "MeStatusLimitedResources"},
  { btLibMeStatusSecurityError,          "MeStatusSecurityError"},
  { btLibMeStatusPersonalDevice,         "MeStatusPersonalDevice"},
  { btLibMeStatusHostTimeout,            "MeStatusHostTimeout"},
  { btLibMeStatusUnsupportedFeature,     "MeStatusUnsupportedFeature"},
  { btLibMeStatusInvalidHciParam,        "MeStatusInvalidHciParam"},
  { btLibMeStatusUserTerminated,         "MeStatusUserTerminated"},
  { btLibMeStatusLowResources,           "MeStatusLowResources"},
  { btLibMeStatusPowerOff,               "MeStatusPowerOff"},
  { btLibMeStatusLocalTerminated,        "MeStatusLocalTerminated"},
  { btLibMeStatusRepeatedAttempts,       "MeStatusRepeatedAttempts"},
  { btLibMeStatusPairingNotAllowed,      "MeStatusPairingNotAllowed"},
  { btLibMeStatusUnknownLmpPDU,          "MeStatusUnknownLmpPDU"},
  { btLibMeStatusUnsupportedRemote,      "MeStatusUnsupportedRemote"},
  { btLibMeStatusScoOffsetRejected,      "MeStatusScoOffsetRejected"},
  { btLibMeStatusScoIntervalRejected,    "MeStatusScoIntervalRejected"},
  { btLibMeStatusScoAirModeRejected,     "MeStatusScoAirModeRejected"},
  { btLibMeStatusInvalidLmpParam,        "MeStatusInvalidLmpParam"},
  { btLibMeStatusUnspecifiedError,       "MeStatusUnspecifiedError"},
  { btLibMeStatusUnsupportedLmpParam,    "MeStatusUnsupportedLmpParam"},
  { btLibMeStatusRoleChangeNotAllowed,   "MeStatusRoleChangeNotAllowed"},
  { btLibMeStatusLmpResponseTimeout,     "MeStatusLmpResponseTimeout"},
  { btLibMeStatusLmpTransdCollision,     "MeStatusLmpTransdCollision"},
  { btLibMeStatusLmpPduNotAllowed,       "MeStatusLmpPduNotAllowed"},
  { btLibL2DiscReasonUnknown,            "L2DiscReasonUnknown"},
  { btLibL2DiscUserRequest,              "L2DiscUserRequest"},
  { btLibL2DiscRequestTimeout,           "L2DiscRequestTimeout"},
  { btLibL2DiscLinkDisc,                 "L2DiscLinkDisc"},
  { btLibL2DiscQosViolation,             "L2DiscQosViolation"},
  { btLibL2DiscSecurityBlock,            "L2DiscSecurityBlock"},
  { btLibL2DiscConnPsmUnsupported,       "L2DiscConnPsmUnsupported"},
  { btLibL2DiscConnSecurityBlock,        "L2DiscConnSecurityBlock"},
  { btLibL2DiscConnNoResources,          "L2DiscConnNoResources"},
  { btLibL2DiscConfigUnacceptable,       "L2DiscConfigUnacceptable"},
  { btLibL2DiscConfigReject,             "L2DiscConfigReject"},
  { btLibL2DiscConfigOptions,            "L2DiscConfigOptions"},
  { btLibServiceShutdownAppUse,          "ServiceShutdownAppUse"},
  { btLibServiceShutdownPowerCycled,     "ServiceShutdownPowerCycled"},
  { btLibServiceShutdownAclDrop,         "ServiceShutdownAclDrop"},
  { btLibServiceShutdownTimeout,         "ServiceShutdownTimeout"},
  { btLibServiceShutdownDetached,        "ServiceShutdownDetached"},
  { btLibErrInUseByService,              "ErrInUseByService"},
  { btLibErrNoPiconet,                   "ErrNoPiconet"},
  { btLibErrRoleChange,                  "ErrRoleChange"},
  { btLibNotYetSupported,                "NotYetSupported"},
  { btLibErrSdpNotMapped,                "ErrSdpNotMapped"},
  { btLibErrAlreadyConnected,            "ErrAlreadyConnected"},
  { -1,                                  "unknown Error"},
};
  
void bt_error(char *msg, Err err) {
  int i = 0;

  while((err_entries[i].err != err)&&(err_entries[i].err != -1))
    i++;
  
  FrmCustomAlert(alt_bt_err, msg, err_entries[i].str, 0);
}

void l2cap_connect(int psm) {
  Err error;
  BtLibSocketConnectInfoType connectInfo;
  void BtLibL2CAPCallbackProc(BtLibSocketEventType *sEventP, UInt32 refCon);
  
  error = BtLibSocketCreate( btLibRefNum, &socketL2cap, 
			     BtLibL2CAPCallbackProc, 0, btLibL2CapProtocol);

  if( error != btLibErrNoError) {
    bt_error("Unable to create L2cap data socket", error);
    return;
  }
  
  /* use remote PSM to establish L2Cap connection */	
  connectInfo.data.L2Cap.remotePsm = psm;
  
  /* connecting machine sends master_state packets */
  connectInfo.data.L2Cap.localMtu = 128; // sizeof(master_state);
  
  /* client machine sends client_state packets */
  connectInfo.data.L2Cap.minRemoteMtu = 128;  //sizeof(slave_state);
  
  /* device to connect to */
  connectInfo.remoteDeviceP = &device;
  
  error = BtLibSocketConnect(btLibRefNum, socketL2cap, &connectInfo);
  if(error != btLibErrPending) {    
    bt_error("Error connecting L2CAP", error);
    return;
  }
}

Boolean l2cap_listen(void) {
  Err error;
  BtLibSocketListenInfoType listenInfo;
  void BtLibL2CAPCallbackProc(BtLibSocketEventType *sEventP, UInt32 refCon);

  // create L2CAP listen socket
  if((error = BtLibSocketCreate(btLibRefNum, &socketL2cap, 
				BtLibL2CAPCallbackProc, 0, 
				btLibL2CapProtocol)) != btLibErrNoError) {
    bt_error("Unable to create l2cap socket", error);
    socketL2cap = -1;
    return false;
  }
    
  /* listen on socket */
  listenInfo.data.L2Cap.localPsm = BT_L2CAP_RANDOM_PSM; /* random psm */
  listenInfo.data.L2Cap.localMtu = 128; // sizeof(slave_state);
  listenInfo.data.L2Cap.minRemoteMtu = 128; // sizeof(master_state);

  if((error = BtLibSocketListen(btLibRefNum, socketL2cap, &listenInfo)) != 
      btLibErrNoError) {
    bt_error("Unable to create listen socket", error);
    return false;
  }

  /* create a record to announce */
  if((error = BtLibSdpServiceRecordCreate(btLibRefNum, &recH)) != 
     btLibErrNoError) {
    bt_error("Unable to create sdp record", error);
    return false;
  }
  
  /* setup record */
  BtLibSdpUuidInitialize(serviceList[0], irPong2Uuid, btLibUuidSize128);
  
  if((error = BtLibSdpServiceRecordSetAttributesForSocket(btLibRefNum,
	  socketL2cap, serviceList, 1, appname, StrLen(appname), recH)) != 
     btLibErrNoError) {
    bt_error("Unable to setup sdp record", error);
    return false;
  }

  /* make record accessible */
  if((error = BtLibSdpServiceRecordStartAdvertising(btLibRefNum, recH)) !=
      btLibErrNoError) {
    bt_error("Unable to advertise sdp record", error);
    return false;
  }

  l2state = LISTENING;
     
  return true;
}

/* close l2cap socket and ACL link */
Boolean bt_discon(Boolean complain) {
  Err error;

  /* stop advertising psm */
  if(recH != 0) {
    BtLibSdpServiceRecordStopAdvertising(btLibRefNum, recH);
    BtLibSdpServiceRecordDestroy(btLibRefNum, recH);
    recH = 0;
  }
  
  if(socketL2cap != -1) {
    
    /* close listen/data socket */
    error = BtLibSocketClose(btLibRefNum, socketL2cap);
    if((error != btLibErrPending) && (error != btLibErrNoError)) {
      bt_error("L2Cap listen close failed", error);
      return false;
    }
    
    if(error != btLibErrNoError)
      FrmCustomAlert(alt_err, "Unhandled, delayed close",0,0);
    
    l2state = DISCONNECTED;
    socketL2cap = -1;
    
    /* don't send further data */
    send_complete = false;
  }
  
  /* disconnect ACL connection if required */
  BtLibLinkDisconnect(btLibRefNum, &device);
  
  return true;
}

/* l2cap callback function */
void BtLibSdpCallbackProc(BtLibSocketEventType *sEventP, UInt32 refCon) {
  Err error;
  int i;

  if (sEventP->status != errNone) {

    /* unable to find service */
    if(sEventP->status != btLibErrSdpAttributeNotSet)
      bt_error("Sdp callback error", sEventP->status);
      
    /* close sdp socket */
    error = BtLibSocketClose(btLibRefNum, socketSdp);
    if((error != btLibErrPending) && (error != btLibErrNoError)) {
      bt_error("Sdp socket close failed", error);
      return;
    }
    
    if(error == btLibErrNoError)
      socketSdp = -1;
    
    bt_discon(true);
    
    if(sEventP->status == btLibErrSdpAttributeNotSet) {
      i = FrmAlert(alt_irnoother);
      if(i==1) { /* retry */
	bt_start_discovery();
      } else if(i==2) {
	beam_app();
      } else if(i==3) {
	send_app();
      }
    }
    
    return;
  }
  
  switch(sEventP->event) {

    /* sdp socket has been disconnected asynchronously */
    case btLibSocketEventDisconnected:
      FrmCustomAlert(alt_err, "async sdp sock disc",0,0);
      socketSdp = -1;
      break;

    case btLibSocketEventSdpGetPsmByUuid:
      error = BtLibSocketClose(btLibRefNum, socketSdp);
      if((error != btLibErrPending) && (error != btLibErrNoError)) {
	bt_error("Sdp socket close failed", error);
	return;
      }
      
      if(error == btLibErrNoError)
	socketSdp = -1;
      
      /* stop listening on socket */
      if(!bt_discon(true)) return;

      /* try to connect */
      l2cap_connect(sEventP->eventData.sdpByUuid.param.psm);
      
      break;
  }
}

void BtLibMECallbackProc(BtLibManagementEventType *mEventP, UInt32 refCon) {
  Err error;

  switch(mEventP->event) {

    /* someone connected us via acl */
    case btLibManagementEventACLConnectInbound:
      if(mEventP->status == btLibErrNoError)
	ACLcon = true;
      break;
		
    case btLibManagementEventACLConnectOutbound:
      if(mEventP->status == btLibErrNoError) {
	ACLcon = true;

	/* ACL connection is there, open sdp ... */

	// create Sdp socket
	if((error = BtLibSocketCreate(btLibRefNum, &socketSdp, 
				      BtLibSdpCallbackProc, 0, 
				      btLibSdpProtocol)) != btLibErrNoError) {
	  bt_error("Unable to create Sdp socket", error);
	  socketSdp = -1;
	  return;
	}
	
	/* setup record */
	BtLibSdpUuidInitialize(serviceList[0], irPong2Uuid, btLibUuidSize128);
	
	/* request it */
	if((error = BtLibSdpGetPsmByUuid(btLibRefNum, socketSdp, 
			   &device, serviceList, 1)) != btLibErrPending) {
	  bt_error("Unable to query PSM", error);
	  return;
	}
      } else
	bt_error("ACL link failed", mEventP->status);
      break;
			
    case btLibManagementEventACLDisconnect:
      ACLcon = false;

      /* we don't care about success or failure here, just end the program */
      stop_game();

      /* try to re-establish listen socket */
      l2cap_listen();

      break;	
  }
}

#define CONNECTED_OUTBOUND 0x01
#define CONNECTED_INBOUND  0x02
#define CONNECTED_NAME     0x04  // expecting name

UInt16 connected=0;
static char bt_buffer[256];  // buffer to store bt packet

static void send_name(void) {
  Err error;

  /* build my first IrPacket and buffer, containing the HotSync name */
  DlkGetSyncInfo(0, 0, 0, bt_buffer, 0, 0);

  /* send name + 0-byte */
  send_complete = false;
  error = BtLibSocketSend(btLibRefNum, socketL2cap, bt_buffer, 
			  StrLen(bt_buffer)+1);

  if(error != btLibErrPending) 
    bt_error("sending name", error);
}

void bt_send_state(void) {
  Err error;
  int i;

  if(!send_complete) return;

  /* create packet to send */
  if(!i_am_master) {
    if(state == SLAVE_ACK) {
      slave.buttons = 0xff;
      slave.paddle_pos = COMM_VERSION;
    } else {
      slave.buttons = slave_buttons;
      slave.paddle_pos = paddle_pos[ME];  /* send own paddle position */
    }
    
    /* reply to message */
    slave.msg_ack = message_buffer>>8;
    
    /* send message to master */
    send_complete = false;
    MemMove(bt_buffer, &slave, sizeof(slave_state));
    error = BtLibSocketSend(btLibRefNum, socketL2cap, bt_buffer, 
			    sizeof(slave_state));
    
    if(error != btLibErrPending) 
      bt_error("slave send", error);
    
  } else {
    /* normal playing state */
    if((state == RUNNING)||(state == SLEEPING)) {
      master.type = MSTR_BALL;
      master.paddle_pos = paddle_pos[ME];
      
      /* transfer balls position */
      for(i=0;i<MAX_BALLS;i++) {
	if(ball_state[i] >= BALL_MOVE) {
	  master.ball_p[i][AX] = 
	    BALL_MIRROR_X(ball_p[i][AX] >> BALL_SHIFT);
	  master.ball_p[i][AY] = 
	    BALL_MIRROR_Y(ball_p[i][AY] >> BALL_SHIFT);
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
    
    /* send message to slave */
    send_complete = false;
    MemMove(bt_buffer, &master, sizeof(master_state));
    error = BtLibSocketSend(btLibRefNum, socketL2cap, bt_buffer, 
			    sizeof(master_state));
	  
    if(error != btLibErrPending) 
      bt_error("master send", error);
  }
}

/* l2cap callback function */
void BtLibL2CAPCallbackProc(BtLibSocketEventType *sEventP, UInt32 refCon) {
  Err error;
  int i;

  switch(sEventP->event) {

    case btLibSocketEventData:
      /* reveived some data */
//      if(sEventP->eventData.data.dataLen == sizeof(img_block_col)) {

      /* expecting name? */
      if(connected & CONNECTED_NAME) {
	connected &= ~CONNECTED_NAME;

	/* save opponents name */
	StrCopy(opponent_name, sEventP->eventData.data.data);

	/* master connects outbound */
	if(connected & CONNECTED_OUTBOUND) {
	  start_game(1);  /* start game as master */
	} else {
	  for(i=0;i<MAX_BALLS;i++)
	    ball_s[i][AX] = 0xff;
	  
	  start_game(0); /* start game as slave */
	  single_game = false;
	}
      } else {
	/* received ordinary game data */
	panic = 0;  /* reset panic counter */

	if(state != SETUP) {
	  if(i_am_master) {
	    MemMove(&slave, sEventP->eventData.data.data, 
		    sizeof(slave_state)); 

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
		/* slave ack with wrong comm version */
		bt_discon(true);
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
	    MemMove(&master, sEventP->eventData.data.data, 
		    sizeof(master_state)); 
	    if((master.type>>4) != 0xf) do_sound(master.type>>4);
	
	    if((master.type&0x0f) == MSTR_BALL) {
	      state = SLAVE_RUNNING;
	      KeySetMask(~(keyBitHard1|keyBitHard2|keyBitHard3|keyBitHard4));
	  
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
      }
      break;
      
    case btLibSocketEventSendComplete:
      /* packet has successfully been sent */
      if (sEventP->status != errNone)
	bt_error("sending", sEventP->status);

      send_complete = true;
      break;
      
    case btLibSocketEventConnectedOutbound:
      // Master just connected to a slave
      if (sEventP->status == errNone) {
	l2state = CONNECTED;
	socketL2cap = sEventP->socket;

	connected = CONNECTED_OUTBOUND | CONNECTED_NAME;
	send_name();
      }
      break;

    case btLibSocketEventConnectedInbound:

      // Master just connected to a slave
      if (sEventP->status == errNone) {
	/* close listen socket */
	bt_discon(true);

	/* save socket */
	socketL2cap = sEventP->eventData.newSocket;

	l2state = CONNECTED;

	connected = CONNECTED_INBOUND | CONNECTED_NAME;
	send_name();

//	FrmCustomAlert(alt_err, "connected inbound",0,0);
      }
      break;
      
    case btLibSocketEventDisconnected:

      // Master or slave lost a connection. Close socket and disconnect ACL
      if(l2state == DISCONNECTED)
	FrmCustomAlert(alt_err, "Connection refused!",0,0);
      
      l2state = DISCONNECTED;

      /* close ACL connection */
      bt_discon(false);

      /* don't send further data */
      send_complete = false;
      
      break;

      /* someone wants to connect */
    case btLibSocketEventConnectRequest:
      /* answer request positive */
      error = BtLibSocketRespondToConnection(btLibRefNum, socketL2cap, true);
      if(error != btLibErrPending) {
	bt_error("Error accepting connection", error);
	return;
      }

      break;
  }
}

void bt_start_discovery(void) {
  Err error;

  /* start discovery of a single device */
  error = BtLibDiscoverSingleDevice(btLibRefNum,"Select Someone to Play with",
				    NULL, 0, &device, false, false);
    
  if(error == btLibErrCanceled)  return;

  if(error != btLibErrNoError) {
    bt_error("Discovery failed", error);
    return;
  }

  error = BtLibLinkConnect(btLibRefNum, &device);
  if((error != btLibErrPending)&&(error != btLibErrNoError)) {
    bt_error("ACL connect failed", error);
    return;
  }
}

Boolean bt_open(void) {
  Err error;

  // Find the Library
  if( SysLibFind( btLibName, &btLibRefNum) ) {
    // load it if it can't be found	
    error = SysLibLoad( sysFileTLibrary , sysFileCBtLib, &btLibRefNum) ;
    btLibRefNum = 0;
    if (error) return false;
  }

  // Open the library, but ensure, that no error message pops up
  if((error = BtLibOpen(btLibRefNum, true)) != btLibErrNoError) {
    if(error != btLibErrRadioInitFailed) btLibRefNum = 0;
    return false;
  }

  // Register the management callback function
  BtLibRegisterManagementNotification(btLibRefNum, BtLibMECallbackProc, 0);
  
  // prepare for incoming connections
  if(!l2cap_listen()) return;

  return !error;
}

void bt_close(void) {
  Err error;

  if (btLibRefNum!=0) {
    BtLibRegisterManagementNotification(btLibRefNum, 0, 0);
    BtLibClose(btLibRefNum);
    btLibRefNum = 0;
  }
}
