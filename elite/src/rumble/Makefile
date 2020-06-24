#  Rumble lib for M50x

LIBNAME = RumbleLibM50x

OBJECTS = $(LIBNAME).o
RESFILE = Rsc.rcp

CC = m68k-palmos-gcc -palmos4 -nostartfiles
XTRALIBS =
BUILDPRC = build-prc
CSFLAGS = -O2 -S
PILRC = pilrc

.S.o:
	$(CC) $(TARGETFLAGS) -c $<

../../rumble/$(LIBNAME).prc: $(LIBNAME)
	$(BUILDPRC) -o $@ driver.def $(LIBNAME) tver0001.bin
	m68k-palmos-objdump --section-headers $(LIBNAME)

$(LIBNAME): $(OBJECTS) tver0001.bin
	$(CC) -o $(LIBNAME) $(OBJECTS) $(XTRALIBS) driver.ld

tver0001.bin: $(RESFILE)
	$(PILRC) $(RESFILE)

$(LIBNAME).o: $(LIBNAME).c
	$(CC) -O5 -c $(LIBNAME).c

clean:
	rm -f *.grc *.bin *.o $(LIBNAME) *~



