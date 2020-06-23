##########################################################################
##
## Makefile for BlueBlocks
##
###########################################################################

APPNAME = BlueBlocks
ICONTEXT = "BlueBlocks"
APPID = bBlk
VERSION = 1.0

SRCS = $(APPNAME).c
OBJS = $(SRCS:.c=.o) tetris.o graphics.o debug.o bt.o \
	bt_error.o bt_discovery.o bt_discovery_db.o bt_discovery_ui.o \
	bt_discovery_pb.o

# Tool locations
CC = m68k-palmos-gcc
BUILDPRC = build-prc
PILRC = pilrc
CONVERT = convert
COPTS = -compress none -depth 8 

RESOURCE = Rsc
PRC = ../$(APPNAME).prc

# Compile options
DEFINES = -DDEBUG
CFLAGS = -O2 -palmos4 -DAPP=\"$(APPNAME)\" -DCREATOR=\'$(APPID)\' $(DEFINES)

# the icon path
ICONP  = icons

# normal icons
ICON   = $(ICONP)/icon.8.bmp $(ICONP)/icon.4.bmp \
	 $(ICONP)/icon.1.bmp \
# hires icon
ICOND  = $(ICONP)/icon_d.8.bmp
# small icons
ICONS  = $(ICONP)/icon_sm.8.bmp $(ICONP)/icon_sm.4.bmp \
	 $(ICONP)/icon_sm.1.bmp \
# small hires icon
ICONSD = $(ICONP)/icon_sm_d.8.bmp

GRAPHICS = $(ICON) $(ICOND) $(ICONS) $(ICONSD)

all: $(PRC)

$(SRCS): $(RESOURCE).h

%.bmp: %.gif
	$(CONVERT) $< $(COPTS) $@

%.4.bmp: %.8.bmp
	convert +dither -colorspace GRAY $< $(COPTS) $@

.S.o:
	$(CC) $(TARGETFLAGS) -c $<

.c.s:
	$(CC) $(CSFLAGS) $<

%.d:%.c
	set -e; $(CC) -MM $(CFLAGS) $< \
	| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@ ; \
	[ -s $@ ] || rm -f $@

$(PRC): $(APPNAME) Rsc.h
	$(BUILDPRC) $@ $(ICONTEXT) $(APPID) $(APPNAME) *.bin

$(RESOURCE).h: $(RESOURCE).rcp $(GRAPHICS)
	cd graphics; make
	$(PILRC) -H $@ $(RESOURCE).rcp
	perl -e 'print "$(VERSION)\0"' >tver0001.bin

$(APPNAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

backup_pen:
	make clean
	cd ../..; tar cvfz $(APPNAME).tar.gz $(APPNAME)
	mcopy -o ../../$(APPNAME).tar.gz c:
	rm -f ../../$(APPNAME).tar.gz

restore_pen:
	cd ../..; rm -rf $(APPNAME).bak
	cd ../..; cp -a $(APPNAME) $(APPNAME).bak
	rm -rf *
	mcopy c:$(APPNAME).tar.gz ../..
	cd ../..; tar xvfz $(APPNAME).tar.gz
	rm -f ../../$(APPNAME).tar.gz

clean:
	cd graphics; make clean
	rm -rf */*.bmp *.[oad] $(APPNAME) *.bin Rsc.h *~

include $(OBJS:.o=.d)
