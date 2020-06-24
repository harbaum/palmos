/*
  silkjoy.h

  interface to the silkscreen joystick control panel 

  (c) 2002 by Till Harbaum <palm@harbaum.org>

  For more details visit http://www.harbaum.org/palm
*/

#ifndef SILKJOY_H
#define SILKJOY_H

#include <PalmOS.h>

#define SilkJoyPrefsVersion10 1
#define SilkJoyCreator 'SkJy'

typedef struct {
  PointType center;
  UInt16 radius;
} SilkJoyPrefsType;


#endif // SILKJOY_H
