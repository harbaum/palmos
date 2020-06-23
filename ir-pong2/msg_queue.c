#include <PalmOS.h>

#include "IR-Pong2.h"

/* msg: */
/* XTTT NNNN DDDD DDDD */
/* X = toggle, T = type, N = unused, D = data */

/* ack: */
/* XTTT NNNN */
/* X = toggle, T = type */

/* msg system */
#define MSG_BUFFER_SIZE 8
UInt16 msg_queue[MSG_BUFFER_SIZE];
int msg_buffer_len = 0;
Boolean msg_toggle = 0;

/* add a message to the queue */
Boolean enqueue_msg(int type, int data) {
  if(msg_buffer_len < (MSG_BUFFER_SIZE-1)) {
    msg_queue[msg_buffer_len] = (msg_toggle<<15) | (type << 12) | data;
    msg_buffer_len++;
    msg_toggle ^= 1;
  }
}

/* get message to send */
unsigned short current_msg(void) {
  if(msg_buffer_len>0)
    return msg_queue[0];

  return 0;
}

/* process acknowledge */
void update_queue(unsigned char ack) {
  int i;

  /* not idle? */
  if(((ack & 0x70) != 0)&&(msg_buffer_len>0)) {
    /* correct ack */
    if(ack == (msg_queue[0]>>8)) {

      /* remove msg from queue */
      for(i=1;i<MSG_BUFFER_SIZE;i++)
	msg_queue[i-1] = msg_queue[i];

      msg_queue[MSG_BUFFER_SIZE-1]=0;
      msg_buffer_len--;
    }
  }
}
