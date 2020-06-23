OBJS = Mensa.o mampf.o beethoven.o

CC = m68k-palmos-coff-gcc

CSFLAGS = -O2 -S

CFLAGS = -O2 -fomit-frame-pointer

PILRC = pilrc
OBJRES = m68k-palmos-coff-obj-res
BUILDPRC = build-prc

ICONTEXT = "Mensa"
APPID = Mnsa

all: Mensa.prc

Mensa.o: Mensa.c MensaRsc.h

beethoven.o: beethoven.c

mampf.o: mampf.c

.S.o:
	$(CC) $(TARGETFLAGS) -c $<

.c.s:
	$(CC) $(CSFLAGS) $<

Mensa.prc: res.stamp obj.stamp
	$(BUILDPRC) Mensa.prc $(ICONTEXT) $(APPID) *.grc *.bin
	pilot-xfer -i Mensa.prc 

obj.stamp: Mensa
	$(OBJRES) Mensa
	touch obj.stamp

res.stamp: Mensa.rcp MensaRsc.h mensa.pbm
	$(PILRC) Mensa.rcp .
	touch res.stamp

Mensa: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o Mensa

clean:
	rm -rf *.[oa] Mensa *.bin *.stamp *.[pg]rc *~ mampf.c conv

mampf.c: conv mensa
	conv mensa > mampf.c

conv: conv.c
	gcc -o conv conv.c

dist:
	make Mensa.prc
	zip /usr/home/harbaum/WWW/pilot/Mensa_prc.zip Mensa.prc
	make clean
	cd ..; tar cvfz /usr/home/harbaum/WWW/pilot/Mensa.tgz mensa
