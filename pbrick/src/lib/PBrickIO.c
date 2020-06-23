/*
   PBrickIO.c

   Part of the PBrick Library, a PalmOS shared library for bidirectional
   infrared communication between a PalmOS device and the Lego Mindstorms
   RCX controller.

   Copyright (C) 2002 Till Harbaum <t.harbaum@web.de>

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
#include "PBrickIO.h"

/* check for cpu type (ez/vz or 328 based) */
void check_cpu(PBrickGlobals *gl) {
  UInt32 id, chip;
  Err err;

  err = FtrGet(sysFtrCreator, sysFtrNumProcessorID, &id);
  if (err)  chip = sysFtrNumProcessor328;
  else      chip = id & sysFtrNumProcessorMask;
  
  gl->cpu_is_ez = ((chip == sysFtrNumProcessorVZ) || 
		   (chip == sysFtrNumProcessorEZ)); 
}

/* setup hardware specific stuff */
void hw_setup(PBrickGlobals *gl) {

  if(gl->cpu_is_ez) {
    /* VZ or EZ cpu */

    /* RxD, TxD, RTS and CTS do GPIO */
    PESEL |= 0xf0;
  
    /* CTS and RxD are inputs, TxD and RTS outputs */
    PEDIR = (PEDIR & ~0x90)|0x60;     
  } else {

    /* ordinary 328 cpu */

    /* RxD, TxD do GPIO */
    PGSEL |= 0x03;  /* TxD, RxD */

    /* RxD is input, TxD is output */
    PGDIR = (PGDIR & ~0x02) | 0x01;
  }
}

void hw_restore(PBrickGlobals *gl) {
  if(gl->cpu_is_ez) {
    /* RxD, TxD normal operation */
    PESEL &= ~0xf0;
  } else {
    PGSEL &= ~0x03;
  }
}

Err open_pbrick(PBrickGlobals *gl) {
  UInt32 value;
  UInt32 flags=srmSettingsFlagBitsPerChar8 | srmSettingsFlagStopBits1 | 
    srmSettingsFlagParityOnM;
  UInt16 paramSize = sizeof(flags);

  /* check for new serial library */
  if(FtrGet(sysFileCSerialMgr, sysFtrNewSerialPresent, &value))
    return errNoMgr;

  /* open library */
  if(SrmOpen(serPortIrPort, 2400, &gl->portId) != errNone)
      return errOpenSer;

  if(SrmControl(gl->portId, srmCtlSetFlags, &flags, &paramSize)!=errNone)
      return errSetPort;

  if(SrmControl(gl->portId, srmCtlIrDAEnable, NULL, NULL)!=errNone)
      return errNoIR;

  /* do some hw specific stuff */
  check_cpu(gl);
  hw_setup(gl);

  return errNone;
}

Err close_pbrick(PBrickGlobals *gl) {
  hw_restore(gl);

  if(gl->portId != 0)
    SrmClose(gl->portId);

  return errNone;
}

/* assembler routines for fast signal generation */
void one_bit_ez(UInt16 timer_val);
void zero_bit_ez(UInt16 timer_val);
void one_bit_328(UInt16 timer_val);
void zero_bit_328(UInt16 timer_val);

asm("
one_bit_ez:
        clr.w  %d1
	move.l #0xfffff421,%a1
	move.w 4(%sp),%a0

ob_main_ez:
        move.w %a0,%d0
	or.b #0x20,(%a1)
ob_lp0_ez: 
        dbra %d0, ob_lp0_ez

        move.w %a0,%d0
	and.b #~0x20,(%a1)
ob_lp1_ez: 
        dbra %d0, ob_lp1_ez

        add.w #1,%d1
	cmp.w #15,%d1
	jble ob_main_ez
	rts

zero_bit_ez:
        clr.w  %d1
	move.l #0xfffff421,%a1
	move.w 4(%sp),%a0

zb_main_ez:
        move.w %a0,%d0
	and.b #~0x20,(%a1)
zb_lp0_ez: 
        dbra %d0, zb_lp0_ez

        move.w %a0,%d0
	and.b #~0x20,(%a1)
zb_lp1_ez: 
        dbra %d0, zb_lp1_ez

        add.w #1,%d1
	cmp.w #15,%d1
	jble zb_main_ez
	rts

one_bit_328:
        clr.w  %d1
	move.l #0xfffff431,%a1
	move.w 4(%sp),%a0

ob_main_328:
        move.w %a0,%d0
	or.b #0x01,(%a1)
ob_lp0_328: 
        dbra %d0, ob_lp0_328

        move.w %a0,%d0
	and.b #~0x01,(%a1)
ob_lp1_328: 
        dbra %d0, ob_lp1_328

        add.w #1,%d1
	cmp.w #15,%d1
	jble ob_main_328
	rts

zero_bit_328:
        clr.w  %d1
	move.l #0xfffff431,%a1
	move.w 4(%sp),%a0

zb_main_328:
        move.w %a0,%d0
	and.b #~0x01,(%a1)
zb_lp0_328: 
        dbra %d0, zb_lp0_328

        move.w %a0,%d0
	and.b #~0x01,(%a1)
zb_lp1_328: 
        dbra %d0, zb_lp1_328

        add.w #1,%d1
	cmp.w #15,%d1
	jble zb_main_328
	rts
");

short check_val(PBrickGlobals *gl) {
  unsigned short t0;
  int i;
  
  t0 = TCN; 

  if(gl->cpu_is_ez) 
    for(i=0;i<(2400/CALI_REZ);i++) 
      zero_bit_ez(gl->timer_val);  
  else              
    for(i=0;i<(2400/CALI_REZ);i++) 
      zero_bit_328(gl->timer_val);  

  return(TCN-t0);
}

#define MY_ABS(a)  (((a)>0)?(a):-(a))

/* try to calibrate active waiting loop */
void calibrate(PBrickGlobals *gl) {
  long imr_save,j;
  int k,i,ticks, t, decr = 16;
  unsigned short tctl_save, tprer_save;

  /* just guess an initial timer_val */
  gl->timer_val = 32;

  /* disable all irqs */
  imr_save = IMR;
  IMR = 0x00ffffff;

  /* put counter into free run 32khz mode */
  tctl_save = TCTL;
  tprer_save = TPRER;
  TCTL |= 0x108;
  TPRER = 0;

  do {
    gl->timer_val -= decr;  /* try to decrease val */
    t = check_val(gl);
    if(t<(32768/CALI_REZ)) gl->timer_val += decr;
    decr>>=1;
  } while(decr>0);

  /* we are still > required value */
  /* check if further dec. vy one is even better */
  gl->timer_val--; 

  decr = MY_ABS((32768/CALI_REZ)-t);
  t = check_val(gl);
  if((MY_ABS(32768/CALI_REZ)-t)>decr)
    gl->timer_val++;

  /* ok, one final test to calculate error */
  t = check_val(gl);
  gl->error = (1000l * ((long)t-(32768l/CALI_REZ)))/(32768l/CALI_REZ);

  /* restore counter mode */
  TCTL = tctl_save;
  TPRER = tprer_save;

  IMR = imr_save;
}

/* send a single byte */
void send_byte(PBrickGlobals *gl, Int8 byte) {
  int i,bits[12], p=0;
  long j;

  bits[0] = 1;  // start bit

  for(i=0;i<8;i++) {
    if(byte&(1<<i)) {
      bits[1+i] = 0;
      p ^= 1;
    } else
      bits[1+i] = 1;
  }

  bits[9] = p;  // parity
  bits[10] = 0; // stop bit

  if(gl->cpu_is_ez) {
    for(i=0;i<11;i++)
      if(bits[i]) one_bit_ez(gl->timer_val); else zero_bit_ez(gl->timer_val);
  } else {
    for(i=0;i<11;i++)
      if(bits[i]) one_bit_328(gl->timer_val); else zero_bit_328(gl->timer_val);
  }
}

UInt16 rcv_msg(UInt16, UInt16, UInt16*);

asm("
rcv_msg:
        movm.l %d2-%d6,-(%sp)

        tst.w  24(%sp)

        beq.s  rcv_328
        move.w #4,%d3
	move.l #0xfffff421,%a0       | port e reg
        bra.s  rcv_start
rcv_328:
        move.w #1,%d3
	move.l #0xfffff431,%a0       | port g reg

rcv_start:
        move.w #764,%d1              | ir event counter (BUFFER_SIZE-4)/2
        
        move.w 26(%sp),%d5           | get timer limit
        lsl.w  #1,%d5
        neg.w  %d5

	move.l 28(%sp),%a1
        clr.w  %d6                    | clear 1-event counter

        move.w #0xffff,%d2

        move.w %d2,%d0
rcv_lp0:
        btst   %d3,(%a0)              | RxD
        dbeq   %d0,rcv_lp0

rcv_main:
        move.w %d2,%d0
rcv_lp1:
        btst   %d3,(%a0)
        dbne   %d0,rcv_lp1

rcv_lp2:
        btst   %d3,(%a0)
        dbeq   %d0,rcv_lp2

        | try to sum up 38kHz peaks
        cmp    %d5,%d0
        bcs.s  rcv_store             | looks like a 0 -> store it

        add.w  %d0,%d6               | add event length to 1's counter 

        cmp.w  %d2,%d0
        bne.s  rcv_main              | no timeout
        bra.s  rcv_quit

rcv_store:
        move.w %d6,(%a1)+            | store previous 1-events
        clr.w  %d6                   | clear 1-event counter
        move.w %d0,(%a1)+            | store 0-event
       
        cmp.w  %d2,%d0
        dbeq   %d1,rcv_main          | neither timeout nor buffer full

rcv_quit:
        move.w %d6,(%a1)+            | store previous 1-events
        move.w %d0,(%a1)+            | store current 0-event

        move.w #1532,%d0             | BUFFER_SIZE-4
        lsl.w  #1,%d1                | double number of events
        sub.w  %d1,%d0

        movm.l (%sp)+,%d2-%d6

	rts
");

/* decode reply message */
UInt16 decode_buffer(PBrickGlobals *gl, UInt16 elements) {
  Int16 i=0,n,j,parity, bytes=0;
  rcv_state state = s_start;
  UInt8 byte;
  Int16 len, bit;
  Boolean error = false;
  Int8 *data=(char*)gl->rcv_buffer;
  UInt16 bitsum[2]={0,0}, bitcnt[2]={0,0}, average;

  /* we need something to work with */
  if(elements<16) return 0;

  /* some preprocessing to gain info about average bit lengths */
  for(average=0,i=0;i<5;i++)
    average -= gl->rcv_buffer[2*i+1];

  /* some more preprocessing to gain info about bit lengths */
  for(i=8;i<16;i++) {
    len = -gl->rcv_buffer[i];
    bit = !(i&1);

    /* we know, that every message starts with 0x55,0xff,0x00 */
    /* so there's a 10 zero bit sequence and a 9 one bit sequence */
    if((bitsum[0]==0)&&(bit==0)&&(len>average))
      bitsum[0]=len/10;

    if((bitsum[1]==0)&&(bit==1)&&(len>average))
      bitsum[1]=len/9;
  }

  /* anable to find correct byte lengths */
  if((!bitsum[0])||(!bitsum[1]))
    return 0;

  for(i=0;i<elements;i++) {

    len = -gl->rcv_buffer[i];
    bit = !(i&1);

    /* parse all bits */
    for(j=0;j<(len+bitsum[bit]/2)/bitsum[bit];j++) {

      /* verify start/stop bits */
      if(state == s_start) {
	parity = 0;
	if(bit != 1) error = true;
      }

      if(state == s_stop) {
	if(bit != 0) error = true;

	/* drop remaining 0-bits */
	j=(len+bitsum[bit]/2)/bitsum[bit];
      }

      /* handle data bits */
      if((state>=s_d0)&&(state<=s_d7)) {
	if(state == s_d0) byte = 0;

	if(bit == 0) {
	  parity ^= 1;
	  byte |= (1<<(state-s_d0));
	}

	if(state == s_d7) {
	  if(bytes < 2*BUFFER_SIZE)
	    data[bytes] = byte;

	  bytes++;
	}
      }

      if(state == s_p)
	if(bit!=parity) error = true;

      /* increase state counter */
      if(++state == s_last) state = s_start;
    }
  }

  return bytes;
}

/* parse reply message */
UInt16 parse_buffer(PBrickGlobals *gl, UInt16 len, UInt8 *rcv_data, UInt16 rcv_len) {
  UInt16 i,n=0;
  UInt8 *data=(char*)gl->rcv_buffer, sum=0;

  /* check header */
  if(*data++ != 0x55) return 0;
  if(*data++ != 0xff) return 0;
  if(*data++ != 0x00) return 0;

  len -= 3;
  if((len&1)||(len<4)) return 0;  /* odd remaining buffer size? */

  /* verify all payload bytes */
  for(i=0;i<(len/2)-1;i++,data+=2) {
    if(*data != ((~*(data+1))&0xff)) return 0;

    if(n++<rcv_len) *rcv_data++ = data[2*i];
    sum += *data;
  }  

  /* verify checksum */
  if(*data != ((~*(data+1))&0xff)) return 0;
  if(sum != *data) return 0;

  return n;
}

/* send a message to the brick and receive reply */
Int16 send_msg(PBrickGlobals *gl, Int8 *data, UInt16 len, Int8 *rcv_dst, UInt16 *rcv_len) {
  int i,j;
  char check=0;
  long imr_save;

  data[0] &= ~8;
  data[0] |= gl->toggle;
  gl->toggle ^= 8;

  /* disable all irqs */
  imr_save = IMR;
  IMR = 0x00ffffff;

  /* send start sequence */
  send_byte(gl, 0x55);
  send_byte(gl, 0xff);
  send_byte(gl, 0x00);

  /* send data */
  for(i=0;i<len;i++) {
    send_byte(gl, data[i]);
    send_byte(gl, ~data[i]);
    check += data[i];
  }

  /* send checksum */
  send_byte(gl, check);
  send_byte(gl, ~check);

  /* receive answer */
  if(rcv_len != NULL) {

    i = rcv_msg(gl->cpu_is_ez, gl->timer_val, (UInt16*)gl->rcv_buffer);

    /* restore interrupt settings */
    IMR = imr_save;
    
    /* try to decode buffer */
    if(!(i = decode_buffer(gl, i)))
      return errLevel0;
    
    if(!(i=parse_buffer(gl, i, rcv_dst, *rcv_len)))
      return errLevel1;
    
    *rcv_len = i;
    
    return errNone;
  }
  else 
    IMR = imr_save;
  
  return errNone;
}
  
Err check_alive(PBrickGlobals *gl) {
  UInt8 msg[]={0x10}, reply[2];
  Int16 rlen=2, cnt;
  Err err;
  
  for(cnt=0;cnt<gl->retries;cnt++) {

    /* send Alive message and get reply */
    err = send_msg(gl, msg, 1, reply, &rlen);
    if((err == errNone)&&(rlen==1)&&((reply[0]&0xf7)==0xe7)) 
      return errNone;

    if(err == errNone) {
      if(rlen != 1) err = errReplyLen;
      else          err = errReply;
    }
    SysTaskDelay(10);
  }  
  return err;
}

Err play_sound(PBrickGlobals *gl, UInt16 snd) {
  UInt8 msg[]={0x51,snd}, reply[2];
  Int16 rlen=2, cnt;
  Err err;

  for(cnt=0;cnt<gl->retries;cnt++) {

    /* send SoundCommand and get reply */
    err = send_msg(gl, msg, 2, reply, &rlen);

    /* check reply message */
    if((err == errNone)&&(rlen == 1)&&((reply[0]&0xf7)==0xa6)) 
      return errNone;

    /* set error according to reply message */
    if(err == errNone) {
      if(rlen != 1) err = errReplyLen;
      else          err = errReply;
    }
    SysTaskDelay(10);
  }  
  return err;
}

Err get_versions(PBrickGlobals *gl, UInt16 *rom, UInt16 *fw) {
  UInt8 msg[]={0x15,1,3,5,7,11}, reply[10];
  Int16 rlen=10, cnt;
  Err err;

  for(cnt=0;cnt<gl->retries;cnt++) {

    /* send GetVersions and get reply */
    err = send_msg(gl, msg, 6, reply, &rlen);

    /* check reply message */
    if((err == errNone)&&(rlen > 4)&&((reply[0]&0xf7)==0xe2)) {

      *rom = 256*reply[1]+reply[2];
      *fw  = 256*reply[3]+reply[4];
      return errNone;
    }

    /* set error according to reply message */
    if(err == errNone) {
      if(rlen <= 4) err = errReplyLen;
      else          err = errReply;
    }
    SysTaskDelay(10);
  }  
  return err;
}

Err get_battery(PBrickGlobals *gl, UInt16 *bat) {
  UInt8 msg[]={0x30}, reply[5];
  Int16 rlen=5, cnt;
  Err err;

  for(cnt=0;cnt<gl->retries;cnt++) {

    /* send GetBattery and get reply */
    err = send_msg(gl, msg, 1, reply, &rlen);

    /* check reply message */
    if((err == errNone)&&(rlen > 2)&&((reply[0]&0xf7)==0xc7)) {

      *bat = 256*reply[1]+reply[2];
      return errNone;
    }

    /* set error according to reply message */
    if(err == errNone) {
      if(rlen <= 2) err = errReplyLen;
      else          err = errReply;
    }
    SysTaskDelay(10);
  }  
  return err;
}







