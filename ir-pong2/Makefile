#
# Makefile for IR Pong 2
#
# (c) 2000 by Till Harbaum
#

APPNAME = IR-Pong2
VERSION = 2.1.1
CREATOR = IPn2

CSRCS = $(APPNAME).c msg_queue.c ir.c beam.c\
	levels.c textbox.c playlist.c bt.c

ASMSRCS = $(APPNAME)-sections.s

OBJS = $(CSRCS:.c=.o) $(ASMSRCS:.s=.o)

# Tool locations
CC = m68k-palmos-gcc
AS = m68k-palmos-as
MULTIGEN = m68k-palmos-multigen
BUILDPRC = build-prc
PILRC = pilrc
CONVERT = convert
CONVERT_OPTS = -compress none -depth 8

RESFILE = Rsc.rcp
PRC = $(APPNAME).prc

CSFLAGS = -O2 -S

# -fomit-frame-pointer
CFLAGS = -palmos4 -DVERSION=\"$(VERSION)\" -DCREATOR=\'$(CREATOR)\' -DAPPNAME=\"$(APPNAME)\" -O2


%.bmp: %.gif
	$(CONVERT) $< $(CONVERT_OPTS) $@

%d.bmp: %.gif
	$(CONVERT) $< $(CONVERT_OPTS) -flip $@

%r.bmp: %.gif
	$(CONVERT) $< $(CONVERT_OPTS) -rotate 90 $@

GR = graphics/

ICONS      = $(GR)icon.1.bmp    $(GR)icon.2.bmp    $(GR)icon.4.bmp    $(GR)icon.8.bmp\
	     $(GR)icon_sm.1.bmp $(GR)icon_sm.2.bmp $(GR)icon_sm.4.bmp $(GR)icon_sm.8.bmp

PADDLE0    = $(GR)paddle0.1.bmp $(GR)paddle0.2.bmp $(GR)paddle0.4.bmp $(GR)paddle0.8.bmp
PADDLE1    = $(GR)paddle1.1.bmp $(GR)paddle1.2.bmp $(GR)paddle1.4.bmp $(GR)paddle1.8.bmp
BALL       = $(GR)ball.1.bmp    $(GR)ball.2.bmp    $(GR)ball.4.bmp    $(GR)ball.8.bmp
BORDER     = $(GR)border.1.bmp  $(GR)border.2.bmp  $(GR)border.4.bmp  $(GR)border.8.bmp
BORDER_R   = $(GR)border.1r.bmp $(GR)border.2r.bmp $(GR)border.4r.bmp $(GR)border.8r.bmp
PBAR0      = $(GR)pbar0.1.bmp   $(GR)pbar0.2.bmp   $(GR)pbar0.4.bmp   $(GR)pbar0.8.bmp
PBAR1      = $(GR)pbar1.1.bmp   $(GR)pbar1.2.bmp   $(GR)pbar1.4.bmp   $(GR)pbar1.8.bmp
PBAR2      = $(GR)pbar2.1.bmp   $(GR)pbar2.2.bmp   $(GR)pbar2.4.bmp   $(GR)pbar2.8.bmp
SHOT_UP    = $(GR)shot.1.bmp    $(GR)shot.2.bmp    $(GR)shot.4.bmp    $(GR)shot.8.bmp
SHOT_DOWN  = $(GR)shot.1d.bmp   $(GR)shot.2d.bmp   $(GR)shot.4d.bmp   $(GR)shot.8d.bmp

PBAR       = $(PBAR0) $(PBAR1) $(PBAR2)
SHOTS      = $(SHOT_UP) $(SHOT_DOWN)
PADDLES    = $(PADDLE0) $(PADDLE1)

TILES      = $(PADDLES) $(BALL) $(BORDER) $(BORDER_R) $(PBAR) $(SHOTS)

# standard bricks
BRICKS_STD = $(GR)brick0.%.bmp  $(GR)brick1.%.bmp  $(GR)brick2.%.bmp  $(GR)brick3.%.bmp\
	     $(GR)brick4.%.bmp  $(GR)brick5.%.bmp  $(GR)brick6.%.bmp

# custom bricks
BRICKS_CST = $(GR)brickA.%.bmp  $(GR)brickv.%.bmp  $(GR)bricka.%.bmp  $(GR)brickn.%.bmp\
	     $(GR)brickt.%.bmp  $(GR)brickG.%.bmp  $(GR)bricko.%.bmp

BRICKS_ALL = $(BRICKS_STD) # $(BRICKS_CST)

BRICKS     = $(BRICKS1) $(BRICKS2) $(BRICKS4) $(BRICKS8)

BRICKS1    = $(subst %,1,$(BRICKS_ALL))
BRICKS2    = $(subst %,2,$(BRICKS_ALL))
BRICKS4    = $(subst %,4,$(BRICKS_ALL))
BRICKS8    = $(subst %,8,$(BRICKS_ALL))

$(GR)pbar0.%.bmp $(GR)pbar1.%.bmp $(GR)pbar2.%.bmp: $(GR)pbar.%.gif
	$(CONVERT) -crop 32x8+0+0  $(CONVERT_OPTS) $< $(GR)pbar0.$*.bmp
	$(CONVERT) -crop 32x8+0+8  $(CONVERT_OPTS) $< $(GR)pbar1.$*.bmp
	$(CONVERT) -crop 32x8+0+16 $(CONVERT_OPTS) $< $(GR)pbar2.$*.bmp

$(BRICKS_STD): $(GR)bricks.%.gif
	$(CONVERT) -crop 16x8+0+8  $(CONVERT_OPTS) $< $(GR)brick0.$*.bmp
	$(CONVERT) -crop 16x8+16+8 $(CONVERT_OPTS) $< $(GR)brick1.$*.bmp
	$(CONVERT) -crop 16x8+32+8 $(CONVERT_OPTS) $< $(GR)brick2.$*.bmp
	$(CONVERT) -crop 16x8+48+8 $(CONVERT_OPTS) $< $(GR)brick3.$*.bmp
	$(CONVERT) -crop 16x8+64+8 $(CONVERT_OPTS) $< $(GR)brick4.$*.bmp
	$(CONVERT) -crop 16x8+80+8 $(CONVERT_OPTS) $< $(GR)brick5.$*.bmp
	$(CONVERT) -crop 16x8+96+8 $(CONVERT_OPTS) $< $(GR)brick6.$*.bmp

$(BRICKS_CST): $(GR)bricks.%.gif
	$(CONVERT) -crop 16x8+0+0  $(CONVERT_OPTS) $< $(GR)brickA.$*.bmp
	$(CONVERT) -crop 16x8+16+0 $(CONVERT_OPTS) $< $(GR)brickv.$*.bmp
	$(CONVERT) -crop 16x8+32+0 $(CONVERT_OPTS) $< $(GR)bricka.$*.bmp
	$(CONVERT) -crop 16x8+48+0 $(CONVERT_OPTS) $< $(GR)brickn.$*.bmp
	$(CONVERT) -crop 16x8+64+0 $(CONVERT_OPTS) $< $(GR)brickt.$*.bmp
	$(CONVERT) -crop 16x8+80+0 $(CONVERT_OPTS) $< $(GR)brickG.$*.bmp
	$(CONVERT) -crop 16x8+96+0 $(CONVERT_OPTS) $< $(GR)bricko.$*.bmp

all: $(PRC)

.S.o:
	$(CC) $(TARGETFLAGS) -c $<

.c.s:
	$(CC) $(CSFLAGS) $<

$(PRC): $(APPNAME) tver0001.bin
	$(BUILDPRC) -o $@ $(APPNAME).def $(APPNAME) *.bin

$(APPNAME)-sections.s $(APPNAME)-sections.ld: $(APPNAME).def
	$(MULTIGEN) $(APPNAME).def

dep:
	makedepend -Y $(CSRCS)

tver0001.bin: Rsc.rcp Rsc.h font_num.txt $(ICONS) $(TILES) $(BRICKS)
	$(PILRC) Rsc.rcp .
	perl -e 'print "$(VERSION)\0"' >$@

$(APPNAME): $(OBJS) $(APPNAME)-sections.ld
	$(CC) $(CFLAGS) $(OBJS) -o $@

clean:
	rm -rf *.[oa] $(APPNAME) *.bak *.bin *.s *.ld *~ */*.bmp

dist:	$(PRC)
	make clean
	rm -f ../IR-Pong2*.zip
	cd ..; zip -r IR-Pong2_src.zip ir-pong2
	zip ../IR-Pong2.zip IR-Pong2.prc readme.txt

disk:
	make dist
	mcopy -o ../IR-Pong2*.zip

backup:
	make dist
	scp ../IR-Pong2.zip harbaum@phobos:~/palm/projekte/ir-pong

# DO NOT DELETE

IR-Pong2.o: Rsc.h IR-Pong2.h
msg_queue.o: IR-Pong2.h
ir.o: Rsc.h IR-Pong2.h
beam.o: Rsc.h IR-Pong2.h
levels.o: IR-Pong2.h
textbox.o: Rsc.h IR-Pong2.h
playlist.o: Rsc.h IR-Pong2.h
