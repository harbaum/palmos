#
#  Makefile for Mulg II
#

OBJS = mulg.o hole.o scarab.o

CC = m68k-palmos-gcc

CSFLAGS = -O2 -S

CFLAGS = -palmos3.5 -O2 -DOS35HDR $(XINCS) -Wall

PILRC = pilrc
OBJRES = m68k-palmos-obj-res
BUILDPRC = build-prc

ICONTEXT = "Mulg"
APPID = Mulg

PALM = /dev/palmv

LEVELDB = test.pdb mulg.pdb mulg2.pdb mulg3.pdb

all: mulg.prc g_mulg.prc c_mulg.prc $(LEVELDB)

compress: compress.c
	gcc -o compress compress.c

makelevel.c: makelevel.y ml_lex.c
	bison -o makelevel.c makelevel.y

ml_lex.c: makelevel.lex
	flex -oml_lex.c makelevel.lex

makelevel: makelevel.c mulg.h
	gcc -Wno-multichar -g -o makelevel makelevel.c

# the demo level contains custom images
test.pdb: custom_1_gray2.raw custom_1_gray4.raw custom_1_color.raw

%_gray2.raw: %_gray2.gif
	convert $< GRAY:$@

%_gray4.raw: %_gray4.gif
	convert $< GRAY:$@

%_color.raw: %_color.gif
	convert -map tiles/rgb.gif $< RGB:$@

%.pdb: %.lev makelevel tiles.h level.h
	makelevel $<

mulg.o: mulg.c rsc.h tiles.h mulg.h tilt.h scarab.c

win2.o: win2.c

win4.o: win4.c

tiles/gray2/tile0000.bin:
	cd tiles; make gray2/tile0000.bin

tiles/gray4/tile0000.bin:
	cd tiles; make gray4/tile0000.bin

tiles/color/tile0000.raw:
	cd tiles; make color/tile0000.raw

# color version uses compressed tile resource
tiles/color/tile0000.bin: compress tiles/color/tile0000.raw
	./compress tiles/color/tile0000.raw tiles/color/tile0000.bin

level.o: level.c tiles.h hole.h

.S.o:
	$(CC) $(TARGETFLAGS) -c $<

.c.s:
	$(CC) $(CSFLAGS) $<

installv: mulg.prc
	pilot-xfer -p /dev/palmv -i mulg.prc

install: mulg.prc
	pilot-xfer -p /dev/pilot -i mulg.prc

mulg.prc: res.stamp obj.stamp tiles/gray2/tile0000.bin
	$(BUILDPRC) mulg.prc $(ICONTEXT) $(APPID) *.mulg.grc *.bin tiles/gray2/tile0000.bin

g_mulg.prc: res.stamp g_obj.stamp tiles/gray4/tile0000.bin
	$(BUILDPRC) g_mulg.prc $(ICONTEXT) $(APPID) *.g_mulg.grc *.bin tiles/gray4/tile0000.bin

c_mulg.prc: res.stamp c_obj.stamp tiles/color/tile0000.bin
	$(BUILDPRC) c_mulg.prc $(ICONTEXT) $(APPID) *.c_mulg.grc *.bin tiles/color/tile0000.bin

obj.stamp: mulg
	$(OBJRES) mulg
	touch obj.stamp

g_obj.stamp: g_mulg
	$(OBJRES) g_mulg
	touch g_obj.stamp

c_obj.stamp: c_mulg
	$(OBJRES) c_mulg
	touch c_obj.stamp

res.stamp: mulg.rcp rsc.h
	$(PILRC) mulg.rcp
	touch res.stamp

mulg: $(OBJS) win2.o
	$(CC) $(CFLAGS) $(OBJS) win2.o -o mulg

g_mulg: $(OBJS) win4.o
	$(CC) $(CFLAGS) $(OBJS) win4.o -o g_mulg

winc.o:  tiles/coltab.h

c_mulg: $(OBJS) winc.o decompress.o
	$(CC) $(CFLAGS) $(OBJS) winc.o decompress.o -o c_mulg

clean:
	rm -rf *.[oa] mulg c_mulg g_mulg *.bin *.stamp *.grc *~ 
	rm -rf convbin ml_lex.c makelevel makelevel.c core .xvpics
	rm -rf compress
	cd tiles; make clean

dist:
	rm -rf ../mulg*.zip
	make mulg.prc g_mulg.prc c_mulg.prc
	zip  ../mulg_prc.zip c_mulg.prc g_mulg.prc mulg.prc mulg.pdb mulg2.pdb mulg3.pdb readme.txt COPYING
	make clean
	cd ..; zip mulg2.zip -r mulg

disk:
	make dist
	mcopy -o ../mulg*.zip a:

www:
	make dist
	mv ../mulg*.zip /usr/home/harbaum/WWW/pilot

srcdisk:
	make clean
	cd ..; zip mulg2.zip -r mulg
	mcopy -o ../mulg2.zip a:


