OBJS = IR-Pong.o

CC = m68k-palmos-coff-gcc

CSFLAGS = -O2 -S

CFLAGS = -O2 -fomit-frame-pointer

PILRC = pilrc
OBJRES = m68k-palmos-coff-obj-res
BUILDPRC = build-prc

ICONTEXT = "IR Pong"
APPID = IRPn

ICONS = ball.pbm paddle.pbm ir-ind.pbm brick0.pbm brick1.pbm brick2.pbm IR-Pong.pbm 

all: IR-Pong.prc

IR-Pong.o: IR-Pong.c IR-PongRsc.h

.S.o:
	$(CC) $(TARGETFLAGS) -c $<

.c.s:
	$(CC) $(CSFLAGS) $<

# copy file to PIII and PV
install: IR-Pong.prc
	pilot-xfer -i IR-Pong.prc 
	pilot-xfer -p /dev/cua1 -i IR-Pong.prc 

IR-Pong.prc: res.stamp obj.stamp
	$(BUILDPRC) IR-Pong.prc $(ICONTEXT) $(APPID) *.grc *.bin

obj.stamp: IR-Pong
	$(OBJRES) IR-Pong
	touch obj.stamp

res.stamp: IR-Pong.rcp IR-PongRsc.h font.txt $(ICONS)
	$(PILRC) IR-Pong.rcp .
	touch res.stamp

IR-Pong: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o IR-Pong

clean:
	rm -rf *.[oa] IR-Pong *.bin *.stamp *.[pg]rc *~

dist:
	make IR-Pong.prc
	rm -f /usr/home/harbaum/WWW/pilot/ir_pong.zip
	zip /usr/home/harbaum/WWW/pilot/ir_pong.zip IR-Pong.prc README
	binhex -d IR-Pong.prc README >/usr/home/harbaum/WWW/pilot/ir_pong.hqx 
	make clean
	rm -f /usr/home/harbaum/WWW/pilot/ir_pong_source.zip
	cd ..; zip -r /usr/home/harbaum/WWW/pilot/ir_pong_source.zip ir-pong
