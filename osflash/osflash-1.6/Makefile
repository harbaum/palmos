OBJS = osflash.o font.o screen.o flash_io.o

CC = m68k-palmos-gcc

CSFLAGS = -O2 -S

CFLAGS = -palmos3.5 -O2 -fomit-frame-pointer

PILRC = pilrc
OBJRES = m68k-palmos-obj-res
BUILDPRC = build-prc

ICONTEXT = "OS Flash"
APPID = OSFl

all: osflash.prc

osflash.o: osflash.c osflashRsc.h

.S.o:
	$(CC) $(TARGETFLAGS) -c $<

.c.s:
	$(CC) $(CSFLAGS) $<

osflash.prc: res.stamp obj.stamp
	$(BUILDPRC) osflash.prc $(ICONTEXT) $(APPID) *.grc *.bin

install: osflash.prc
	pilot-xfer -i osflash.prc 

obj.stamp: osflash
	$(OBJRES) osflash
	touch obj.stamp

res.stamp: osflash.rcp osflashRsc.h osflash.pbm
	$(PILRC) osflash.rcp .
	touch res.stamp

osflash: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o osflash

os2pdb: os2pdb.c
	gcc -o os2pdb os2pdb.c

clean:
	rm -rf *.[oa] osflash os2pdb *.bin *.stamp *.grc *~ osflash.zip

dist:
	make clean
	rm -f /usr/home/harbaum/WWW/pilot/osflash_prc.zip
	zip /usr/home/harbaum/WWW/pilot/osflash_prc.zip README osflash.prc os2pdb.exe
	rm -f /usr/home/harbaum/WWW/pilot/osflash.zip
	cd ..; zip /usr/home/harbaum/WWW/pilot/osflash.zip osflash/*
