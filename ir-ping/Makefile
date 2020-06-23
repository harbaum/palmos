OBJS = IR-Ping.o

CC = m68k-palmos-coff-gcc

CSFLAGS = -O2 -S

CFLAGS = -O2 -fomit-frame-pointer

PILRC = pilrc
OBJRES = m68k-palmos-coff-obj-res
BUILDPRC = build-prc

ICONTEXT = "IR Ping"
APPID = IRAn

all: IR-Ping.prc

IR-Ping.o: IR-Ping.c IR-PingRsc.h

.S.o:
	$(CC) $(TARGETFLAGS) -c $<

.c.s:
	$(CC) $(CSFLAGS) $<

IR-Ping.prc: res.stamp obj.stamp
	$(BUILDPRC) IR-Ping.prc $(ICONTEXT) $(APPID) *.grc *.bin

obj.stamp: IR-Ping
	$(OBJRES) IR-Ping
	touch obj.stamp

res.stamp: IR-Ping.rcp IR-PingRsc.h IR-Ping.pbm
	$(PILRC) IR-Ping.rcp .
	touch res.stamp

IR-Ping: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o IR-Ping

clean:
	rm -rf *.[oa] IR-Ping *.bin *.stamp *.grc *~

dist:
	make IR-Ping.prc
	zip  /usr/home/harbaum/WWW/pilot/IR-Ping_prc.zip IR-Ping.prc
	make clean
	cd ..; zip /usr/home/harbaum/WWW/pilot/IR-Ping.zip ir-ping/*
