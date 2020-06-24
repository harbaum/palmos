#ifndef RUMBLE_H
#define RUMBLE_H

extern UInt16 rumble_cnt;

Bool rumble_open(void) SEC3D;
void rumble_close(void) SEC3D;
void rumble(Bool on) SEC3D;

#endif // RUMBLE_H
