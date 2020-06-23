/*

  ftp.c  - (c) 2000 by Till Harbaum

  ftp client

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 */

#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "axxPacFTP.h"
#include "ftpRsc.h"
#include "fselRsc.h"
#include "ftp.h"
#include "filio.h"

#define RECV_BUFFER 4096
#define MAX_FILES   512  // allocate buffer space for these files

/* control port infos */
static Int LibRef = 0;
static Long timeoutticks;
static NetSocketRef sock = -1;
static NetSocketAddrINType remote;

static NetHostInfoBufType hostinfo;

/* passive port infos */
static Int passive_port;
static NetSocketRef passive_sock = -1;
static NetSocketAddrINType passive_remote;

static char msg_buffer[256];

static Boolean is_connected = false;

static int files = 0;
static struct file_info *file = NULL;

static int current_disp_offset;

static char buf[RECV_BUFFER+1];  // receiver buffer

const char error_net_timeout[] = "Timeout";
const char error_net_socket_not_open[] = "Socket not open";
const char error_net_socket_busy[] = "Socket busy";
const char error_net_name_too_long[] = "DNS: Name too long";
const char error_net_bad_name[] = "DNS: Bad name";
const char error_net_sock_closed_remote[] = "Socket closed by remote";
const char error_net_error_num[] = "Error #";

char errname[256];
static char *net_error(Err err) {
  switch (err) {
  case netErrTimeout: StrCopy(errname, error_net_timeout); break;
  case netErrSocketNotOpen: StrCopy(errname, error_net_socket_not_open); break;
  case netErrSocketBusy : StrCopy(errname, error_net_socket_busy); break;
  case netErrDNSNameTooLong: StrCopy(errname, error_net_name_too_long); break;
  case netErrDNSBadName: StrCopy(errname, error_net_bad_name); break;
  case netErrSocketClosedByRemote: StrCopy(errname, error_net_sock_closed_remote); break;

  default:
    StrCopy(errname, error_net_error_num);
    StrIToA(&errname[StrLen(errname)], (Long) err);
    StrCat(errname, " (");
    StrIToA(&errname[StrLen(errname)], (Long) err - 0x1200);
    StrCat(errname, ")");
  }
  return errname;
}

void busy(Boolean on) {
  Handle resH;
  BitmapPtr resP;
  RectangleType rect;

  if(!on) {
    /* clear bitmap */
    rect.topLeft.x = 150; rect.topLeft.y = 0; 
    rect.extent.x =   10; rect.extent.y  = 12;
    WinEraseRectangle(&rect,0);
  } else {
    /* draw bitmap */
    resH = (Handle)DmGetResource( bitmapRsc, IconBusy);
    resP = (BitmapPtr)MemHandleLock((VoidHand)resH);
    WinDrawBitmap(resP, 150, 1);
    MemPtrUnlock(resP);
    DmReleaseResource((VoidHand) resH );
  }
}

/* draw some kind of progess bar */
void progress_bar(Boolean init, char *str, int percent) {
#define BAR_HEIGHT  40
#define BAR_WIDTH  120

  RectangleType rect;
  static int lp;
  char pstr[5];

  if(percent<0)   percent=0;
  if(percent>100) percent=100;

  if(init) {
    rect.topLeft.x = 80-(BAR_WIDTH/2);
    rect.topLeft.y = 80-(BAR_HEIGHT/2);
    rect.extent.x  = BAR_WIDTH;
    rect.extent.y  = BAR_HEIGHT;

    WinEraseRectangle(&rect, 3);
    WinDrawRectangleFrame(dialogFrame, &rect);

    WinDrawChars(str, StrLen(str), 80-(BAR_WIDTH/2)+10, 80-(BAR_HEIGHT/2)+5);

    /* erase bar */
    rect.topLeft.x = 80-(BAR_WIDTH/2)+10;
    rect.topLeft.y = 80+(BAR_HEIGHT/2)-15;
    rect.extent.x = BAR_WIDTH-20;
    rect.extent.y = 11;
    WinDrawRectangleFrame(rectangleFrame, &rect);
  } else {
    /* nothing to do? */
    if(percent==lp) return;

    /* remove old percentage */
    StrPrintF(pstr, "%d%%", lp);
    WinInvertChars(pstr, StrLen(pstr), 
                   80-(FntLineWidth(pstr, StrLen(pstr))/2),
                   80+(BAR_HEIGHT/2)-15);
  }

  /* draw bar */
  rect.topLeft.x = 80-(BAR_WIDTH/2)+11;
  rect.topLeft.y = 80+(BAR_HEIGHT/2)-14;
  rect.extent.x = (percent*(BAR_WIDTH-22))/100;
  rect.extent.y = 9;
  WinDrawRectangle(&rect, 0);

  /* new percentage */
  StrPrintF(pstr, "%d%%", percent);
  WinInvertChars(pstr, StrLen(pstr), 
                 80-(FntLineWidth(pstr, StrLen(pstr))/2),
                 80+(BAR_HEIGHT/2)-15);

  lp = percent;
}

/* write length limited string followed by dots at pos */
static void 
win_draw_chars_len(int x, int y, SWord max, char *str) {
  SWord lenp;
  Boolean fit;
  const char dots[]="...";

  lenp = StrLen(str);
  fit = true;
  FntCharsInWidth(str, &max, &lenp, &fit);

  /* something cut off? */
  if(lenp != StrLen(str)) {
    max = max-FntLineWidth((char*)dots,3);
    FntCharsInWidth(str, &max, &lenp, &fit);
    WinDrawChars(str, lenp, x, y);
    WinDrawChars((char*)dots, 3, x+max, y);
  } else
    WinDrawChars(str, lenp, x, y);
}

/* list an card entry */
static void 
list_entry(BitmapPtr resP, int index, int y) {
  char str[32];
  SWord widp, lenp;
  Boolean fit;

  if(file[index].is_dir) WinDrawBitmap(resP, 0, 15+y*11);

  FntSetFont(0);

  /* draw name (max 70 pixel) */
  win_draw_chars_len(NAME_POS, 15+y*11, LEN_POS-NAME_POS, file[index].name);

  if(!file[index].is_dir) {
  
    /* draw length */
    StrIToA(str, file[index].length);
    WinDrawChars(str, StrLen(str), LAST_POS-
		 FntLineWidth(str, StrLen(str)), 15+y*11);
  }
}

int NPNetworkLayerUp(void) {
  NetMasterPBType pbP;
  Word nl;
  Err err;
  UInt32  ifCreator;
  UInt16  ifInstance;

  // Maybe network has not been initialized yet so find netlib in either case.
  err = SysLibFind("Net.lib", &nl);
  if (err) {
    FrmCustomAlert(alt_err, "Net Library not found, exiting ...",0,0);
    return false;
  }

  // If there are no interfaces attached, print error message
  err = NetLibIFGet(nl, 0, &ifCreator, &ifInstance);
  if (err) {
    FrmCustomAlert(alt_err, "Net internet service setup.",0,0);
    return false;
  }

  return true;
} /* NPNetworkLayerUp */

void close_netlib(void) {
  Err err;

  /* free buffer */
  if(file != NULL) {
    files = 0;
    MemPtrFree(file);
    file = NULL;
  }

  if (sock != -1) {
    NetLibSocketClose(LibRef, sock, timeoutticks, &err);
    sock = -1;
  }

  if (passive_sock != -1) {
    NetLibSocketClose(LibRef, passive_sock, timeoutticks, &err);
    passive_sock = -1;
  }

  if (LibRef != 0) err = NetLibClose(LibRef, prefs.close);
}

SWord Send(NetSocketRef sock, NetSocketAddrINType *remote, 
	   char *buffer, Word size, Err *err) {
  SWord num;

  if (size == 0) return 1;

  while(size > 0) {

    num = NetLibSend(LibRef, sock, (VoidPtr) buffer, size, 0, 
		     (VoidPtr)remote, sizeof(*remote), timeoutticks, err);
    if (num == -1) {
      FrmCustomAlert(alt_err2, "Sending data", net_error(*err), 0);
      size = 0;
    } else if (num == 0) {
      FrmCustomAlert(alt_err, (char*)error_net_sock_closed_remote, 0,0);
    } else {
      buffer += num;
      size -= num;
    }
  }

  return num;
}

Boolean open_netlib(void) {
  Word open_err;
  Err err;

  timeoutticks = 10 * sysTicksPerSecond;

  /* open netlib */
  if (SysLibFind("Net.lib", &LibRef)) {
    FrmCustomAlert(alt_err, "Unable to find network library.", 0,0);
    LibRef = 0;  /* failed ... */
    return false;
  }

  if (!NPNetworkLayerUp()) {
    FrmCustomAlert(alt_err, "Network is shut down, trying to reopen it.", 0,0);
    close_netlib();
  }

  //	NPSetInterfaceSpeed(LibRef, 57600); // uncomment to speed-up

  err = NetLibOpen(LibRef, &open_err);
  if (open_err || (err && (err != netErrAlreadyOpen))) {
    FrmCustomAlert(alt_err, "Unable to open network library.", 0, 0);
    if (open_err) {
      NetLibClose(LibRef, 1);
    }
    return false;
  }
  return true;
}

Boolean ftp_send_str(char *str) {
  Err err;

  Send(sock, &remote, str, StrLen(str), &err);
  return true;
}

char cmd_buf[RECV_BUFFER];
int cmd_index = 0;

Boolean ftp_cmd(char *cmd, char *parms, int *expect) {
  int num=0;
  NetSocketAddrType addr;
  Word size = sizeof(NetSocketAddrType);
  Err err;
  Boolean end=false;
  int id, *b;
  char *p, *s;
  Boolean ret = true;

  if(cmd != 0) {
    ftp_send_str(cmd);     /* send command */

    if(parms != 0) {
      ftp_send_str(" ");   /* send space */
      ftp_send_str(parms); /* send optional parameter */
    }

    ftp_send_str("\r\n");  /* send crlf */
  }

  while(!end) {
    /* only reload if no lf in buffer */
    if(StrChr(cmd_buf, '\n')==0) {
      num = NetLibReceive(LibRef, sock, (VoidPtr) &cmd_buf[cmd_index], 
			  RECV_BUFFER - cmd_index, 
			  0, &addr, &size, timeoutticks, &err);

      if (num == -1) {
	FrmCustomAlert(alt_err2, "Receiving data", net_error(err), 0);
	return false;
      } else if (num == 0) {
	FrmCustomAlert(alt_err, "Connection closed by remote server.", 0,0);
	return false;
      }
    }

    /* extend current buffer */
    cmd_index += num;
    cmd_buf[cmd_index]=0;

    /* handle all lines in buffer */
    while(((p=StrChr(cmd_buf, '\n'))!=0)&&(!end)) {

      /* remove carriage return */
      if((s=StrChr(cmd_buf, '\r')) != 0) *s = 0;

      /* buffer contains end of line, replace with terminator */
      *p=0;
      StrCopy(msg_buffer, cmd_buf);   /* save for further analysis */
      
      /* buffer starts with digit */
      if((cmd_buf[0] >= '0')&&(cmd_buf[0] <= '9')) {
	/* try to extract id number */
	id = StrAToI(cmd_buf);
	
	/* serach for matching entry */
	b = expect;
	while((*b != -1)&&(*b != id)) b++;
	
	DEBUG_MSG("(%d) %s\n", id, cmd_buf);
	
	if(*b == -1) {
	  /* save unexpected message */
	  FrmCustomAlert(alt_err2, "Server reply", cmd_buf, 0);
	  ret = false;
	}
	
	/* stop */
	if(cmd_buf[3] != '-') end = true;
      }
      
      /* skip this line */
      MemMove(cmd_buf, p+1, RECV_BUFFER-(p-cmd_buf));
      cmd_index -= (p-cmd_buf+1);
    }
  }
  return ret;
}

Boolean
connect(char *host, int port, char *user, char *pass) {
  Err err;
  SWord num;
  UInt address_pos = 0;
  FormPtr frm;

  int init_ret[] = {220, -1};
  int user_ret[] = {331, -1};
  int pass_ret[] = {230, -1};
  int type_ret[] = {200, -1};

  /* draw busy box */
  frm = FrmInitForm(frm_ServerCon);
  CtlSetLabel(FrmGetObjectPtr (frm, FrmGetObjectIndex(frm, lbl_SName)), host);
  FrmDrawForm(frm);

  if(is_connected) {
    FrmEraseForm(frm);
    FrmDeleteForm(frm);
    FrmCustomAlert(alt_err, "Already connected to FTP server.", 0,0);
    return false;
  }
    
  sock = NetLibSocketOpen(LibRef, netSocketAddrINET, netSocketTypeStream, 
			  NetHToNS(6), timeoutticks, &err);
  if (sock == -1) {
    FrmEraseForm(frm);
    FrmDeleteForm(frm);
    FrmCustomAlert(alt_err, "Unable to open socket.",0 ,0);
    return false;
  }

  if (0 != NetLibGetHostByName(LibRef, host, &hostinfo, 
			       timeoutticks, &err)) {
    remote.addr = hostinfo.address[address_pos];
  } else {
    FrmEraseForm(frm);
    FrmDeleteForm(frm);
    FrmCustomAlert(alt_err, "Unable to resolve server name.", 0, 0);
    return false;
  }
  
  remote.family = netSocketAddrINET;
  remote.port = port;

  num = NetLibSocketConnect(LibRef, sock, (NetSocketAddrType *)&remote, 
			    sizeof(remote), timeoutticks, &err);

  if (num == -1) {
    FrmEraseForm(frm);
    FrmDeleteForm(frm);
    FrmCustomAlert(alt_err, "Unable to connect to server.", 0,0);
    return false;
  }

  /* read welcome message */
  if(!ftp_cmd(NULL, NULL, init_ret)) {
    FrmEraseForm(frm);
    FrmDeleteForm(frm);
    sock = -1;
    NetLibSocketClose(LibRef, sock, timeoutticks, &err);
    return false;  // reply: connected
  }

  /* log on */
  if(!ftp_cmd("USER", user, user_ret)) {
    FrmEraseForm(frm);
    FrmDeleteForm(frm);
    sock = -1;
    NetLibSocketClose(LibRef, sock, timeoutticks, &err);
    return false;  // reply: passwd req
  }

  /* send passwd */
  if(!ftp_cmd("PASS", pass, pass_ret)) {
    FrmEraseForm(frm);
    FrmDeleteForm(frm);
    sock = -1;
    NetLibSocketClose(LibRef, sock, timeoutticks, &err);
    return false;  // reply: logged in
  }

  if(!ftp_cmd("TYPE", "i", type_ret)) {
    FrmEraseForm(frm);
    FrmDeleteForm(frm);
    return false;
  }

    FrmEraseForm(frm);
  FrmDeleteForm(frm);
  is_connected = true;
  return true;
}

void disconnect(Boolean interactive) {
  Err err;
  int quit_ret[] = {221, -1};

  if(is_connected) {

    /* send disconnect message */
    ftp_cmd("QUIT", NULL, quit_ret);
  
    /* finally close socket */
    NetLibSocketClose(LibRef, sock, timeoutticks, &err);

    sock = -1;
    is_connected = false;
  } else if(interactive) {
    FrmCustomAlert(alt_err, "Not connected to FTP server." ,0,0);  }
}

Boolean open_passive(void) {
  char *s;
  int i;
  SWord num;
  Err err;
  int pasv_ret[] = {227, -1};

  if(!ftp_cmd("PASV", NULL, pasv_ret)) return false;

  /* skip to port number */
  s = msg_buffer;
  while(*s != '(') s++;
  for(i=0;i<4;i++) {
    while(*s != ',') s++;
    s++;
  }

  /* extract port number */
  passive_port = StrAToI(s);
  while(*s != ',') s++;
  passive_port = 256*passive_port + StrAToI(s+1);

  DEBUG_MSG("passive port: %u\n", passive_port);
  passive_sock = NetLibSocketOpen(LibRef, netSocketAddrINET, 
				  netSocketTypeStream, 
				  NetHToNS(6), timeoutticks, &err);
  if (passive_sock == -1) {
    FrmCustomAlert(alt_err, "Unable to open data socket.",0 ,0);
    return false;
  }

  passive_remote.addr = hostinfo.address[0];
  passive_remote.family = netSocketAddrINET;
  passive_remote.port = passive_port;

  num = NetLibSocketConnect(LibRef, passive_sock, 
			    (NetSocketAddrType *)&passive_remote, 
			    sizeof(passive_remote), timeoutticks, &err);

  if (num == -1) {
    FrmCustomAlert(alt_err, "Unable to connect to server.", 0,0);
    return false;
  }

  DEBUG_MSG("passive connection is open\n");

  busy(true);

  return true;
} 

void close_passive(void) {
  Err err;

  busy(false);

  NetLibSocketClose(LibRef, passive_sock, timeoutticks, &err);
  passive_sock = -1;

  DEBUG_MSG("passive connection is closed\n");
}

Boolean change_dir(char *name) {
  int cwd_ret[]={250, -1};

  if(is_connected) {
    DEBUG_MSG("change dir %s\n", name);

    /* send change dir command */
    ftp_cmd("CWD", name, cwd_ret);
  }
  return true;
}

void list_files(Boolean sb) { 
  RectangleType rect;
  FormPtr frm;
  ScrollBarPtr bar;  
  Handle resH;
  BitmapPtr resP;
  int i, n;

  /* clear screen */
  rect.topLeft.x=0; rect.topLeft.y=15; 
  rect.extent.x=LAST_POS; rect.extent.y=SHOW_ENTRIES*11;
  WinEraseRectangle(&rect,0);

  resH = (Handle)DmGetResource( bitmapRsc, IconFolder);
  resP = (BitmapPtr)MemHandleLock((VoidHand)resH);

  frm = FrmGetActiveForm();
  bar = FrmGetObjectPtr (frm, FrmGetObjectIndex (frm, scl_Main));

  i = files - SHOW_ENTRIES;
  if(i<0) i=0;

  if(sb) {
    if(files>SHOW_ENTRIES) 
      SclSetScrollBar(bar, 0/* offset */, 0, i, SHOW_ENTRIES-1);
    else 
      SclSetScrollBar(bar, 0/* offset */, 0, 0, 0);
  }

  for(i=0;i<((files>SHOW_ENTRIES)?SHOW_ENTRIES:files);i++)
    list_entry(resP, i+current_disp_offset, i);

  MemPtrUnlock(resP);
  DmReleaseResource((VoidHand) resH );
}

const char *month[]={
  " Jan ", " Feb ", " Mar ", " Apr ", " May ", " Jun ", 
  " Jul ", " Aug ", " Sep ", " Oct ", " Nov ", " Dec "
};

Boolean list(void) {
  Err err;
  NetFDSetType readFDS;
  Boolean done = false;
  char *p, *s, *d;
  SWord num;
  NetSocketAddrType addr;
  Word size = sizeof(NetSocketAddrType);
  int index = 0, offset = -1, i, j;
  RectangleType rect;

  int list_ret[] = {150, 125, -1};
  int compl_ret[] = {226, -1};
  int pwd_ret[] = {257, -1};

  if(is_connected) {

    if(file == NULL) {
      file = MemPtrNew(MAX_FILES * sizeof(struct file_info));
      if(file == NULL) {
	FrmCustomAlert(alt_err, "Out of memory.", 0,0);
	return false;
      }
    }

    /* Listen does not really work with this single tasking PalmOS */
    /* so let's use passive mode */
    open_passive();

    if(!ftp_cmd("LIST", NULL, list_ret)) {
      close_passive();
      return false;
    }

    files = 0;
    current_disp_offset = 0;

    do {
      num = NetLibReceive(LibRef, passive_sock, (VoidPtr) &buf[index], 
		     RECV_BUFFER-index, 0, &addr, &size, timeoutticks, &err);
  
      if (num == -1) {
	FrmCustomAlert(alt_err2, "Receiving data", net_error(err), 0);
	close_passive();
	return false;
      } else if(num>0) {
	/* extend current buffer */
	index += num;
	buf[index]=0;

	/* handle all lines in buffer */
	while((p=StrChr(buf, '\n'))!=0) {

	  /* remove carriage return */
	  if((s=StrChr(buf, '\r')) != 0) *s = 0;

	  /* buffer contains end of line, replace with terminator */
	  *p=0;

	  if(files<MAX_FILES) {
	    /* parse buffer line */

	    /* ignore links */
	    if((StrLen(buf)>20)&&((buf[0] == 'd')||(buf[0] == '-'))) {

	      if(offset = -1) {

		/* try to find name of month */
		for(i=0;i<12;i++) {
		  if(StrStr(buf, month[i]) != NULL)
		    offset = StrStr(buf, month[i]) - buf;
		}

		/* one step back to length */
		if(offset != -1) {
		  while(buf[offset] == ' ') offset--;
		  while(buf[offset] != ' ') offset--;
		  offset++;
		}
	      }
	      
	      if(offset != -1) {
		file[files].is_dir = (buf[0] == 'd');

		s = &buf[offset];
		file[files].length = StrAToI(s);
		while(*s != ' ') s++; while(*s == ' ') s++; /* skip length */
		while(*s != ' ') s++; while(*s == ' ') s++; /* skip month  */
		while(*s != ' ') s++; while(*s == ' ') s++; /* skip day    */
		while(*s != ' ') s++; while(*s == ' ') s++; /* skip year   */
		
		/* copy name */
		d = file[files].name;
		while((*s != ' ')&&(*s != '\n')&&
		      (*s != '\r')&&(*s != 0)) *d++ = *s++;
		*d++ = 0;
		
		files++;
	      }
	    }
	  }

	  MemMove(buf, p+1, RECV_BUFFER-(p-buf));
	  index -= (p-buf+1);
	}
      }
    } while(num > 0);  /* read until server closes connection (num==0) */
    
    close_passive();

    /* wait for completion code */
    if(!ftp_cmd(NULL, NULL, compl_ret)) return false;
  }

  DEBUG_MSG("scanned %d files\n", files);
  list_files(true);

  /* draw path */
  if(!ftp_cmd("PWD", NULL, pwd_ret)) return false;

  WinDrawLine(0, 160-13, 116, 160-13);

  if(StrChr(msg_buffer, '\"') != NULL) {
    p = s = msg_buffer; while(*s != '\"') { s++; p++; }
    s++; p++; while(*p != '\"') p++; *p=0;
  }
  
  FntSetFont(0);

  /* clear path */
  rect.topLeft.x=15; rect.topLeft.y=160-12; 
  rect.extent.x=116-15; rect.extent.y=11;
  WinEraseRectangle(&rect,0);

  win_draw_chars_len(15, 160-12, 116-15, s);

  return true;
}

Boolean get_file(char *name, long fsize) {
  Err err;
  int retr_ret[]  = {150, 125, -1};
  int compl_ret[] = {226, -1};
  SWord num;
  NetSocketAddrType addr;
  Word size = sizeof(NetSocketAddrType);
  Long cnt=0;
  char fname[128];
  int i;

  if(is_connected) {
    /* Listen does not really work with this single tasking PalmOS */
    /* so let's use passive mode */

    /* select destination directory */
    StrCopy(fname, prefs.path);
    i=StrLen(fname)-1;
    while((fname[i] != '/')&&(i>=0)) fname[i--]=0;
    StrCat(fname, name);

    if((i=fs_fsel("Select destination:", fname, true))<0)
      return false;

    /* cancelled */
    if(i == 0) return true;

    StrCopy(prefs.path, fname);

    /* see whether file has a filename attached */
    if(fname[StrLen(fname)-1]=='/') {
      StrCat(fname, name);
    }

    open_passive();

    if(!ftp_cmd("RETR", name, retr_ret)) {
      close_passive();
      return false;
    }

    /* open file */
    if(!fs_open(fname, FS_CREAT)) {
      close_passive();
      return false;
    }

    progress_bar(true, "Download file", 0);

    do {
      num = NetLibReceive(LibRef, passive_sock, (VoidPtr) buf, 
		     RECV_BUFFER, 0, &addr, &size, timeoutticks, &err);
  
      if (num == -1) {
	FrmCustomAlert(alt_err2, "Receiving data", net_error(err), 0);

	/* close file */
	fs_close();

	/* remove this file */
	fs_delete(fname);

	close_passive();
	return false;
      } else if(num>0) {
	cnt+=num;
	fs_write(buf, num);
	progress_bar(false, NULL, 100l*cnt/fsize);
      }
    } while(num > 0);  /* read until server closes connection (num==0) */
    
    DEBUG_MSG("received %ld bytes\n", cnt);

    /* close file */
    fs_close();

    close_passive();

    if(!ftp_cmd(NULL, NULL, compl_ret)) {
      return false;
    }
  }
  return true;
}

/* select particular entry */
static void 
select_entry_int(int entry, int on) {
  RectangleType rect;
  static cur_on=-1;
  int i;

#ifdef DEBUG_SELECT
  DEBUG_MSG("%sselect entry %d\n", on?(on==2)?"":"re":"de", entry);
#endif

  /* set entry size */
  rect.topLeft.x = 0; 
  rect.extent.x  = LAST_POS; 
  rect.extent.y  = 11;    

  /* deinvert selected entry on desel or if new to be selected */
  if((cur_on>=0)&&(cur_on<files)&&
     ((!on)||(cur_on!=entry)||(entry<0))) { 
#ifdef DEBUG_SELECT
    DEBUG_MSG("Off: %d\n", cur_on);
#endif

    rect.topLeft.y=15+11*cur_on;       
    if(!on) SysTaskDelay(10);
    WinInvertRectangle(&rect,0);
  }

  /* select new entry if existing and not already selected */
  if((on)&&(entry<files)&&(entry>=0)&&(cur_on!=entry)) { 
#ifdef DEBUG_SELECT
    DEBUG_MSG("On: %d\n", entry);
#endif

    rect.topLeft.y=15+11*entry;
    WinInvertRectangle(&rect,0);
  }

  /* cur_on may be <0, because release events are reported */
  /* even if they're not inside the display area */
  if((!on)&&(entry<files)&&(cur_on != -1)) {

    /* make some sound ... */
    SndPlaySystemSound(sndClick);

    if(file[entry+current_disp_offset].is_dir) {
      change_dir(file[entry+current_disp_offset].name);
      list();
    } else {
      get_file(file[entry+current_disp_offset].name, 
	       file[entry+current_disp_offset].length);
      list();
    }
  }

  if(!on) cur_on=-1;
  else    cur_on=entry;

#ifdef DEBUG_SELECT
  DEBUG_MSG("new cur on = %d\n", cur_on);
#endif
}

void list_select(int x, int y, int sel) {
  /* in valid click area? */
  if((y>15)&&(y<(15+SHOW_ENTRIES*11))&&(x<LAST_POS))
    select_entry_int((y-15)/11, sel);
  else
    select_entry_int(-1, sel);
}

/* set scroll position */
void list_scroll(Boolean abs, int pos) {
  if(abs) current_disp_offset  = pos;
  else    current_disp_offset += pos*(SHOW_ENTRIES-1);

  if(current_disp_offset>(files - SHOW_ENTRIES)) 
    current_disp_offset = files - SHOW_ENTRIES;

  if(current_disp_offset<0) current_disp_offset=0;

  list_files(false);
}

Boolean upload(void) {
  Err err;
  int stor_ret[]  = {150, 125, -1};
  int compl_ret[] = {226, -1};
  SWord num;
  NetSocketAddrType addr;
  Word size = sizeof(NetSocketAddrType);
  Long cnt=0;
  char fname[128];
  int i;
  char *p;
  long flength;

  if(is_connected) {
    /* Listen does not really work with this single tasking PalmOS */
    /* so let's use passive mode */

    /* select destination directory */
    StrCopy(fname, prefs.path);
    i=StrLen(fname)-1;
    while((fname[i] != '/')&&(i>=0)) fname[i--]=0;

    if((i=fs_fsel("Select file to upload:", fname, false))<0)
      return false;

    /* cancelled */
    if(i == 0) return true;

    StrCopy(prefs.path, fname);

    open_passive();

    DEBUG_MSG("A: %s\n", fname);

    /* find last '/' */
    p = fname;
    if(StrChr(fname, '/') != NULL) {
      i = StrLen(fname)-1;
      while(fname[i]!='/') p = &fname[i--];
    }

    DEBUG_MSG("B\n");

    DEBUG_MSG("store '%s'\n", p);

    if(!ftp_cmd("STOR", p, stor_ret)) {
      close_passive();
      return false;
    }

    /* get file size */
    flength = fs_tell(fname);
    if(flength < 0) {
      close_passive();
      return false;
    }

    /* open file */
    if(!fs_open(fname, FS_RDONLY))
      return false;

    progress_bar(true, "Upload file", 0);

    do {
      num = fs_read((VoidPtr) buf, RECV_BUFFER);

      if(num>0) {
	Send(passive_sock, &passive_remote, (VoidPtr)buf, num, &err);
  
	cnt+=num;
	progress_bar(false, NULL, 100l*cnt/flength);
      }
    } while(num == RECV_BUFFER);  /* read until whole file read */
    
    DEBUG_MSG("sent %ld bytes\n", cnt);

    /* close file */
    fs_close();

    close_passive();

    if(!ftp_cmd(NULL, NULL, compl_ret)) {
      return false;
    }
  }

  return true;
}

