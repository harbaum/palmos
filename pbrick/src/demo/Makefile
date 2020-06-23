#
#   Makefile for PBrick Demo
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

APPNAME = PBrickDemo
ICONTEXT = "PBDemo"
APPID = PBDm

SRCS = $(APPNAME).c AppStdIO.c
OBJS = $(SRCS:.c=.o)

# Tool locations
CC = m68k-palmos-gcc
BUILDPRC = build-prc
PILRC = pilrc
CONVERT = convert
COPTS = -compress none -depth 8 

RESFILE = Rsc.rcp
PRC = ../../$(APPNAME).prc

# Compile options
DEFINES = -DDEBUG
CSFLAGS = -O2 -S -DCREATOR=\'$(APPID)\' $(DEFINES)
CFLAGS = -O2 -palmos4 -I../include -DCREATOR=\'$(APPID)\' $(DEFINES)

ICONP = icons
ICON  = $(ICONP)/icon.8.bmp $(ICONP)/icon.4.bmp \
	$(ICONP)/icon.2.bmp $(ICONP)/icon.1.bmp 
ICONS = $(ICONP)/icon_sm.8.bmp $(ICONP)/icon_sm.4.bmp \
	$(ICONP)/icon_sm.2.bmp $(ICONP)/icon_sm.1.bmp 
GRAPHICS = $(ICON) $(ICONS)

all: $(PRC)

.S.o:
	$(CC) $(TARGETFLAGS) -c $<

.c.s:
	$(CC) $(CSFLAGS) $<

$(PRC): $(APPNAME) tver0001.bin
	$(BUILDPRC) $@ $(ICONTEXT) $(APPID) $(APPNAME) *.bin

tver0001.bin: $(RESFILE) $(GRAPHICS)
	$(PILRC) $(RESFILE)

$(APPNAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

clean:
	rm -rf *.[oa] $(APPNAME) *.bin *~




