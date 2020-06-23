OBJS = flashy.o flash_io.o

CC = m68k-palmos-coff-gcc

CSFLAGS = -O2 -S

CFLAGS = -O2 -fomit-frame-pointer

PILRC = pilrc
OBJRES = m68k-palmos-coff-obj-res
BUILDPRC = build-prc

ICONTEXT = "Flashy"
APPID = Flsh

all: flashy.prc

flashy.o: flashy.c flashyRsc.h

.S.o:
	$(CC) $(TARGETFLAGS) -c $<

.c.s:
	$(CC) $(CSFLAGS) $<

flashy.prc: res.stamp obj.stamp
	$(BUILDPRC) flashy.prc $(ICONTEXT) $(APPID) *.grc *.bin

install: flashy.prc
	pilot-xfer -i flashy.prc 

obj.stamp: flashy
	$(OBJRES) flashy
	touch obj.stamp

res.stamp: flashy.rcp flashyRsc.h flashy.pbm
	$(PILRC) flashy.rcp .
	touch res.stamp

flashy: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o flashy

clean:
	rm -rf *.[oa] flashy *.bin *.stamp *.grc *~ flashy.zip

dist:
	make clean
	cd ..; zip /usr/home/harbaum/WWW/pilot/flashy.zip flashy/*
