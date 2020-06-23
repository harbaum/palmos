#
#   Makefile for PBrick Library
#
#   Part of the PBrick Library, a PalmOS shared library for bidirectional
#   infrared communication between a PalmOS device and the Lego Mindstorms
#   RCX controller.
#
#   Copyright (C) 2002 Till Harbaum <t.harbaum@web.de>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

LIBNAME   = PBrickLib

OBJECTS   = $(LIBNAME).o PBrickIO.o

TOOLBASE = m68k-palmos-
CC = $(TOOLBASE)gcc -nostartfiles
LD = m68k-palmos-gcc -nostartfiles
BUILDPRC = build-prc
PILRC = pilrc

CSFLAGS = -O2 -S
CFLAGS = -palmos4 -O2 -I../include
CFLAGSOPT = $(CFLAGS) -fomit-frame-pointer

all: ../../$(LIBNAME).prc

PBrickIO.o: PBrickIO.c
	$(CC) $(CFLAGSOPT) -o $@ -c $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

../../$(LIBNAME).prc: $(LIBNAME) tver0001.bin
	$(BUILDPRC) -o $@ library.def $(LIBNAME) *.bin
	m68k-palmos-objdump --section-headers $(LIBNAME)

tver0001.bin: Rsc.rcp
	$(PILRC) $< .

$(LIBNAME): $(OBJECTS)
	$(LD) -o $(LIBNAME) $(OBJECTS) library.ld

clean:
	rm -f *.bin *.stamp *.grc *.o $(LIBNAME) *~

depend:
	makedepend *.c
