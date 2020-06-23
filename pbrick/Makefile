#
#   Makefile
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

all: 
	cd src/lib ; make
	cd src/demo ; make
	cd src/remote ; make

clean: 
	rm -f *~
	cd src/lib ; make clean
	cd src/demo ; make clean
	cd src/remote ; make clean

disk:
	make clean
	rm -f ../pbrick.zip
	cd ..; zip -r pbrick.zip pbrick
	mcopy -o ../pbrick.zip a:
