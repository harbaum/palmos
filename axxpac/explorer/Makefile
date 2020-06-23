OBJS = explorer.o list_dir.o db.o memo.o beam.o backup.o

CC = m68k-palmos-gcc

CSFLAGS = -O2 -S

PILRC = pilrc
BUILDPRC = build-prc

ICONTEXT = "axxPac"
CREATOR  = axPE

#DEBUG = -DDEBUG
CFLAGS = -palmos3.1 -O2 -DCREATOR=\'$(CREATOR)\' $(DEBUG)

PALM = /dev/ttyS3

all: ../axxPacShell.prc

explorer.o: explorer.c explorerRsc.h

.S.o:
	$(CC) $(TARGETFLAGS) $<

.c.s:
	$(CC) $(CSFLAGS) $<

../axxPacShell.prc: res.stamp explorer
	$(BUILDPRC) -o ../axxPacShell.prc explorer.def explorer *.bin

obj.stamp: explorer
	$(OBJRES) explorer
	touch obj.stamp

res.stamp: explorer.rcp explorerRsc.h explorer.bmp
	$(PILRC) explorer.rcp .
	touch res.stamp

explorer: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o explorer explorer.ld

clean::
	rm -rf core *.[oa] explorer *.bak *.bin *.stamp *.grc *~

readback::
	pilot-xfer $(PALM) -f SmartMediaImage

restore::
	pilot-xfer $(PALM) -i ../xtra_smartmedia/image/4m/SmartMediaImage.pdb

depend::
	makedepend *.c

# DO NOT DELETE


backup.o: ../api/axxPac.h callback.h db.h explorer.h explorerRsc.h list_dir.h
beam.o: ../api/axxPac.h beam.h callback.h db.h explorer.h explorerRsc.h
db.o: ../api/axxPac.h callback.h db.h explorer.h explorerRsc.h
explorer.o: ../api/axxPac.h backup.h callback.h db.h explorer.h explorerRsc.h
explorer.o: list_dir.h
list_dir.o: ../api/axxPac.h beam.h callback.h db.h explorer.h explorerRsc.h
list_dir.o: list_dir.h memo.h
memo.o: ../api/axxPac.h db.h explorer.h explorerRsc.h list_dir.h memo.h
