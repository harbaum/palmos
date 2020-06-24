/*
 * Elite - The New Kind.
 *
 * Palm version of the keyboard routines.
 *
 * The code in this file has not been derived from the original Elite code.
 * Written by C.J.Pinder 1999-2001.
 * email: <christian@newkind.co.uk>
 *
 */

/*
 * keyboard.h
 *
 * Code to handle keyboard input.
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

/* screen tap */
#define KBD_TAP_X  0
#define KBD_TAP_Y  1
extern UInt8   kbd_tap[2];

Boolean kbd_keyboard_startup (void);
Boolean kbd_keyboard_shutdown (void);
void kbd_poll_keyboard (void);

/* updated key driver */
#define KBD_F1          0x0001
#define KBD_F2          0x0002
#define KBD_F3          0x0004
#define KBD_F4          0x0008
#define KBD_F5          0x0010
#define KBD_F6          0x0020
#define KBD_F7          0x0040
#define KBD_F8          0x0080
#define KBD_F9          0x0100
#define KBD_F10         0x0200
#define KBD_F11         0x0400
#define KBD_F12         0x0800
#define KBD_F13         0x1000
#define KBD_F14         0x2000
#define KBD_F15         0x4000
#define KBD_F16         0x8000

#define KBD_UP          0x0001
#define KBD_DOWN        0x0002
#define KBD_LEFT        0x0004
#define KBD_RIGHT       0x0008
#define KBD_SPEED       0x0010
#define KBD_FIRE        0x0020
// #define KBD_TAP_HOLD    0x0040
#define KBD_TAP         0x0080

#define KBD_ANY         (KBD_UP|KBD_DOWN|KBD_LEFT|KBD_RIGHT|KBD_ENTER|KBD_FIRE|KBD_TAP )

#define KBD_SPINC       0x0100
#define KBD_SPDEC       0x0200
#define KBD_MSLE        0x0400
#define KBD_TOGGLE      0x0800

#define KBD_TAP_REPEAT  0x4000
#define KBD_MENU        0x8000

extern UInt16 kbd_button_map;
extern UInt16 kbd_funkey_map;
extern UInt8  kbd_gamepad;

#ifdef SONY
#define SONY_STEP     8

#define KBD_SONY_UP   0x01
#define KBD_SONY_DOWN 0x02
#define KBD_SONY_PUSH 0x04
#define KBD_SONY_BACK 0x08

extern UInt8  kbd_sony;
#endif

#endif
