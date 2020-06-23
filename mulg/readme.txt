Mulg II - a puzzle game for the palm pilot and compatibles
Copyright (C) 1998 - 2001 by Till Harbaum, Pat Kane and Tomoto Shimizu

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

The sources for mulg are available at
  http://www.ibr.cs.tu-bs.de/~harbaum/pilot/mulg.html

Installation:
  The archive contains three binaries:
  - mulg.prc works with 2bpp grayscale graphics (black, white and two
    shades of grey) and works on all black'n white palms
  - g_mulg.prc uses the 4bpp grayscale graphics (16 shades of grey) 
    available on EZ-cpu based machines. This program runs on all
    EZ-CPU based black'n white palms (e.g. Palm IIIx, Palm IIIe,
    Palm IIIxe, Palm V, Palm Vx, the newer Palm VII models, the
    Visor and the TRG Pro). This version looks ugly on some Palm IIIe,
    Palm IIIx and Palm IIIxe (and perhaps other devices). This is
    due to a hardware problem of the display of the III series.
    Please try the 2bpp version instead.
    Since Mulg IIo the 4bpp version should run on any current Palm
    device (Visor Prism and Platinum and the Palm IIIc as well).
  - c_mulg.prc is the color version of mulg and runs on all 
    color palm devices based on the SED1375 video unit (Palm IIIc and
    Visor Prism).
  
  HotSync the required mulg binary and at least one level database 
  (mulg.pdb, mulg2.pdb or mulg3.pdb) to your pilot. For tilt sensor 
  support optionally install the appropriate tilt driver lib.

Flashbuilder/FlashPro:
  Since version IIe, the binary and the level databases can be stored in 
  flash.

Instructions:
  There isn't very much to say here, just start the game and play the first 
  level. Once you've completed one level, you are allowed to play the next 
  one, too. It's recommanded to start with the levels in 'Classic Mulg'
  (mulg.pdb), then play 'The Story goes on' (mulg2.pdb) and then 'The next
  chapter' (mulg3.pdb). You may also install third party level databases
  from the mulg homepage.

Hardware addon:
  Instructions to add a tilt sensor to the pilot to play the game by tilting 
  the pilot can be found under:
     http://www.ibr.cs.tu-bs.de/~harbaum/pilot/adxl202.html

  Since version IIe, mulg uses a seperate tilt driver. Without tilt hardware
  installed on your pilot, you don't need to install any driver.

  If you are interested in purchasing a tilt add-on for your Palm,
  please contact the author.

Graphic level editors:
  Visit the mulg homepage http://www.ibr.cs.tu-bs.de/~harbaum/pilot/mulg.html
  for links to graphic level editors.

More levels:
  More level databases are available at the mulg homepage.

Compilation:
  The mulg source can be compiled under Linux using the prc-tools-2.0 available
  at 3com developers pages. Additionally you'll need the PalmOS 3.5 header 
  files (OS35 is required to compile the color version, if you only need the
  greyscale versions, you might be able to use the OS 3.1 headers supplied
  with the sdk. Please remove the '-D OS35HDR' from the Makefile when using
  the old header files).

Version history:
  1.0  initial release
  1.1  fixed exception when entering border area (happened in level 5)
  1.1b fixed problem with document in level 15
  1.1c system game sound preferences are used
  II   level compiler/levels from databases
       div. bug fixes
       new tiles (flip/groove/bomb disp/radio button ...)
       support for adxl202 tilt sensor
  IIa  fixed auto power off when using the tilt sensor
  IIb  tilt sensor support switched off for OS version > 3.02 (for Palm IIIx 
       and Palm V) and fixed little display bug
  IIc  fixed some problem with low batt warnings, changed the sensor
       support to be disabled with EZ CPUs (patch from Goeff Richmond)
       and a little fix in greyscale memory handling.
       (the low batt stuff still has some problems which causes the
       screen sometimes to switch for a second to BW even when there 
       is no real low batt warning displayed, i have seen other 
       applications like the TinyViewer having the same problem but
       don't know a solution. I think this is better than to ignore
       the low batt warning at all like i did in prior versions)
  IId  minor bug fixes
  IIe  changed tilt sensor interfacing to use sensor library 
       added support for rom based level databases
       added gif image based tiles
       fix screen flicker with OS 3.3 or the visor
       4 bit per pixel graphics engine (still needs graphics)
  IIf  PEK added "light weight" blocks and "switch spaces"
  IIg  finished 4bpp graphics engine
       added color support
       and switched to prc-tools-2.0
       removed limitation to 16 level databases
  IIh  memo tiles
  IIi  scarab bug fix
       tilt interface doesn't automatically comply about missing dongle
  IIj  beam level databases
  IIk  fixed bug in code for breaking light boxes
  IIl  skipped, because version numbering looked ugly
  IIm  fixed bug introduced with IIk bug fix
  IIn  new tiles: hanoi pyramid, walker and swapped
  IIo  fixed walker bug, new 4bpp greyscale code (works
       on Visor Platinum, Prism and Palm IIIc, font fixes
       for multibyte fonts (e.g. japanese)

Have fun,
  Tomoto, Pat & Till




