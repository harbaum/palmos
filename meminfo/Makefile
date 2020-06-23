OBJS = meminfo.o

CC = m68k-palmos-coff-gcc

CSFLAGS = -O2 -S

CFLAGS = -O2 -fomit-frame-pointer

PILRC = pilrc
OBJRES = m68k-palmos-coff-obj-res
BUILDPRC = build-prc

ICONTEXT = "Mem Info"
APPID = MemI

all: meminfo.prc

meminfo.o: meminfo.c meminfoRsc.h

.S.o:
	$(CC) $(TARGETFLAGS) -c $<

.c.s:
	$(CC) $(CSFLAGS) $<

meminfo.prc: res.stamp obj.stamp
	$(BUILDPRC) meminfo.prc $(ICONTEXT) $(APPID) *.grc *.bin
	pilot-xfer -i meminfo.prc 

obj.stamp: meminfo
	$(OBJRES) meminfo
	touch obj.stamp

res.stamp: meminfo.rcp meminfoRsc.h meminfo.pbm
	$(PILRC) meminfo.rcp .
	touch res.stamp

meminfo: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o meminfo

clean:
	rm -rf *.[oa] meminfo *.bin *.stamp *.grc *~ meminfo.zip

dist:
	make meminfo.prc
	zip  /usr/home/harbaum/WWW/pilot/meminfo_prc.zip meminfo.prc
	make clean
	cd ..; zip /usr/home/harbaum/WWW/pilot/meminfo.zip meminfo/*
