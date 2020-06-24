##########################################################################
##
## Makefile for Elite for PalmOS
##
###########################################################################

APPNAME = Elite
ICONTEXT = "Elite"
APPID = Elit
VERSION = 0.9.2

CSRCS = palm_main.c palm_debug.c\
	palm_gfx.c palm_snd.c palm_key.c docked.c elite.c\
	planet.c random.c sequences.c pilot.c decompress.c\
	trade.c options.c missions.c palm_file.c space.c\
	intro.c stars.c swat.c vector.c threed.c splash.c\
	rumble.c dialog.c sbay.c

ASMSRCS = elite-sections.s

OBJS = $(CSRCS:.c=.o) $(ASMSRCS:.s=.o) 

# Tool locations
CC = m68k-palmos-gcc
AS = m68k-palmos-as
MULTIGEN = m68k-palmos-multigen
OBJCOPY = m68k-palmos-objcopy
BUILDPRC = build-prc
PILRC = pilrc
CONVERT = convert
COPTS = -compress none -depth 8 

RESFILE = Rsc.rcp
PRC = ../$(APPNAME).prc

OPTIONS += -DVERSION=\"$(VERSION)\"

#### Compile options, uncomment desired options
#### show all ships on start screen (e.g. cougar)
# OPTIONS += -DSHOW_ALL_SHIPS
#### do some console debug output
# OPTIONS += -DDEBUG
#### don't show launch sequence and the like
# OPTIONS += -DNO_SEQUENCES
#### enable intros/splash screen
OPTIONS += -DSPLASH # splash screen with danube melody
# OPTIONS += -DINTRO1 # cobra intro
OPTIONS += -DINTRO2 # ship parade intro
#### cheat for testing purposes
# OPTIONS += -DCHEAT
#### enable support for sony specific hardware
OPTIONS += -DSONY
#### add sbay support
# OPTIONS += -DSBAY

#### enable limitations (NAME/TIME)
# LIMIT = NAME
OPTIONS += -DINTERNAL_TEXT

# limit game to user
#OPTIONS += -DUSER_NAME="\"Ian Bell\""
#OPTIONS += -DUSER_NAME="\"Kara Shallenberg\""
#OPTIONS += -DUSER_NAME="\"Till Harbaum\""
#OPTIONS += -DUSER_NAME="\"wl\""
#OPTIONS += -DUSER_NAME="\"Ilan Tayary\""
#OPTIONS += -DUSER_NAME="\"glitch\""
#OPTIONS += -DUSER_NAME="\"Andrew Robertson\""
#OPTIONS += -DUSER_NAME="\"Joshua S. Weisbuch\""
#OPTIONS += -DUSER_NAME="\"Howard Samuels\""

# limit game by time
EXPIRE_AT = 15.11.2002 00:00:00

ifdef LIMIT
OPTIONS += -DLIMIT
OPTIONS += -DLIMIT_$(LIMIT)
endif


# resolution of sinus table
SINREZ = 512

CSFLAGS = -O2 -S -DCREATOR=\'$(APPID)\' $(OPTIONS)
CFLAGS = -O2 -palmos4 -DCREATOR=\'$(APPID)\' $(OPTIONS) -Wimplicit -Wmissing-declarations

GRAPH = graphics
ICON  = $(GRAPH)/icon.8.bmp $(GRAPH)/icon.4.bmp \
	$(GRAPH)/icon.2.bmp $(GRAPH)/icon.1.bmp 
ICONS = $(GRAPH)/icon_sm.8.bmp $(GRAPH)/icon_sm.4.bmp \
	$(GRAPH)/icon_sm.2.bmp $(GRAPH)/icon_sm.1.bmp 

COMPRESSED = spls0000.bin sprt0000.bin sprt0001.bin \
	     sprt0002.bin sprt0003.bin sprt0004.bin \
	     sprt0005.bin

GRAPHICS = $(ICON) $(ICONS) $(BUTTONS) $(COMPRESSED)

all: $(PRC)
	cd rumble; make

midi0000.bin: sound/danube.mid compress
	./compress $< $@

# compress splash screen, scanner and sprites
spls0000.bin: splash_sm.8.map compress
	./compress $< $@

# the scanner image
sprt0000.bin: scanner.8.map compress
	./compress $< $@

# button map
sprt0001.bin: buttons.8.map compress
	./compress $< $@

# the dice sprite
sprt0002.bin: dice.8.map compress
	./compress $< $@

# commander blake
sprt0003.bin: blake.8.map compress
	./compress $< $@

# commander fortesque
sprt0004.bin: fort.8.map compress
	./compress $< $@

# spacebay logo
sprt0005.bin: sbaylogo.8.map compress
	./compress $< $@

# convert 3*8 bit rgb into palm map color mapped image
%.8.map: %.8.rgb bitmapper
	./bitmapper $< $@

# convert gif image into 3*8 bit rgb
%.8.rgb: $(GRAPH)/%.8.gif
	convert $< RGB:$@

bitmapper: bitmapper.c coltab.h
	gcc -o $@ $<

%.bmp: %.gif
	$(CONVERT) $< $(COPTS) $@

gray0000.bin: mkgraytab
	./mkgraytab > gray0000.raw
	./compress gray0000.raw $@

mkgraytab: mkgraytab.c coltab.h
	gcc -o $@ $< -lm

space.o: sintab.h

sintab.h: sin.c compress
	gcc -o sin sin.c -lm
	./sin $(SINREZ) > sine0000.raw
	./compress sine0000.raw sine0000.bin
	echo -n "#define SIN_STEPS " > sintab.h
	echo -n $(SINREZ) >> sintab.h

ifdef LIMIT
dategen: dategen.c
	gcc -o $@ $<

limit.h: dategen Makefile
	./dategen $(EXPIRE_AT) >limit.h

limit.o: limit.c limit.h Makefile

limit.raw: limit.o
	$(OBJCOPY) -O binary limit.o limit.raw

texts.h: mktexts.c compress limit.raw limit.h
	gcc -o mktexts $(OPTIONS) mktexts.c
	./mktexts texts.h text0000.raw limit.raw
	./compress text0000.raw text0000.bin
else
texts.h: mktexts.c compress
	gcc -o mktexts $(OPTIONS) mktexts.c
	./mktexts texts.h text0000.raw
	./compress text0000.raw text0000.bin
endif

gfx.h: texts.h

.S.o:
	$(CC) $(TARGETFLAGS) -c $<

.c.s:
	$(CC) $(CSFLAGS) $<

$(PRC): $(APPNAME) ship0000.bin tver0001.bin font/font0000.bin midi0000.bin gray0000.bin text0000.bin
	$(BUILDPRC) -o $@ elite.def $(APPNAME) *.bin font/*.bin

font/font0000.raw:
	cd font; make

font/font0000.bin: font/font0000.raw compress
	./compress $< $@

tver0001.bin: $(RESFILE) $(GRAPHICS)
	$(PILRC) $(RESFILE)
	perl -e 'print "$(VERSION)\0"' >$@

threed.o: 3d_limit.h

3d_limit.h ship0000.bin: mkshipdata
	./mkshipdata

compress: compress.c
	gcc -o $@ $<

mkshipdata: mkshipdata.c shipdata.h
	gcc $(DEFINES) -DMKSHIPDATA -o mkshipdata mkshipdata.c -lm

$(APPNAME): $(OBJS) elite-sections.ld
	$(CC) $(CFLAGS) $(OBJS) elite-sections.ld -o $@

elite-sections.s elite-sections.ld: elite.def
	$(MULTIGEN) elite.def

dep:
	makedepend -Y $(CSRCS)

dist:	$(PRC)
	rm -f ../Elite.zip
	cd ..; zip -r Elite.zip Elite.prc rumble

zip:
	make clean
	rm -f ../../elite_src.zip
	cd ../..; zip -r elite_src.zip elite

backup:
	make zip
	if [ `hostname` == phobos ]; then \
	  scp ../../elite_src.zip harbaum@deimos:~/palm/projekte/elite; \
	fi;
	if [ `hostname` == deimos ]; then \
	  scp ../../elite_src.zip harbaum@phobos:~/palm/projekte/elite; \
	fi;

backup_itm:
	make zip
	scp ../../elite_src.zip harbaum@till:~/local/palm/projekte/elite

clean:
	rm -rf */*.bmp *.[oas] $(APPNAME) *.grc *.bin *.stamp *~ *.raw
	rm -rf *sections.ld mkshipdata 3d_limit.h rotsin.h *.map *.rgb
	rm -rf compress bitmapper *.bak sin sintab.h mktexts texts.h
	rm -rf mkgraytab font/font0000.bin limit.raw dategen
	cd font; make clean
	cd ../manual; make clean
	cd rumble; make clean

# DO NOT DELETE

palm_main.o: sections.h Rsc.h debug.h config.h gfx.h elite.h planet.h trade.h
palm_main.o: dialog.h texts.h main.h vector.h docked.h intro.h shipdata.h
palm_main.o: space.h sound.h threed.h swat.h random.h options.h stars.h
palm_main.o: missions.h pilot.h file.h keyboard.h splash.h sequences.h
palm_main.o: rumble.h
palm_debug.o: debug.h
palm_gfx.o: config.h gfx.h elite.h planet.h trade.h sections.h dialog.h
palm_gfx.o: texts.h shipdata.h docked.h debug.h Rsc.h splash.h decompress.h
palm_gfx.o: space.h vector.h sound.h options.h
palm_snd.o: elite.h planet.h trade.h sections.h dialog.h sound.h debug.h
palm_snd.o: options.h gfx.h config.h texts.h
palm_key.o: config.h keyboard.h elite.h planet.h trade.h sections.h dialog.h
palm_key.o: debug.h gfx.h texts.h options.h Rsc.h docked.h sound.h silkjoy.h
docked.o: sections.h config.h gfx.h elite.h planet.h trade.h dialog.h texts.h
docked.o: shipdata.h space.h vector.h debug.h docked.h random.h options.h
docked.o: sound.h
elite.o: config.h elite.h planet.h trade.h sections.h dialog.h vector.h
elite.o: shipdata.h debug.h file.h options.h
planet.o: config.h gfx.h elite.h planet.h trade.h sections.h dialog.h texts.h
planet.o: missions.h vector.h options.h
random.o: random.h
sequences.o: sections.h debug.h config.h gfx.h elite.h planet.h trade.h
sequences.o: dialog.h texts.h vector.h space.h shipdata.h sound.h random.h
sequences.o: intro.h stars.h threed.h main.h keyboard.h sequences.h colors.h
sequences.o: docked.h missions.h swat.h rumble.h pilot.h options.h
pilot.o: config.h gfx.h elite.h planet.h trade.h sections.h dialog.h texts.h
pilot.o: vector.h main.h space.h shipdata.h sound.h pilot.h debug.h
decompress.o: Rsc.h decompress.h sections.h debug.h
trade.o: config.h gfx.h elite.h planet.h trade.h sections.h dialog.h texts.h
trade.o: docked.h space.h vector.h shipdata.h sound.h random.h main.h swat.h
trade.o: debug.h
options.o: elite.h planet.h trade.h sections.h dialog.h config.h gfx.h
options.o: texts.h options.h main.h docked.h file.h debug.h space.h vector.h
options.o: shipdata.h sound.h
missions.o: config.h elite.h planet.h trade.h sections.h dialog.h gfx.h
missions.o: texts.h vector.h space.h shipdata.h main.h missions.h keyboard.h
missions.o: debug.h
palm_file.o: elite.h planet.h trade.h sections.h dialog.h config.h file.h
palm_file.o: options.h debug.h
space.o: sections.h vector.h config.h elite.h planet.h trade.h dialog.h gfx.h
space.o: texts.h docked.h intro.h shipdata.h space.h threed.h sound.h main.h
space.o: swat.h random.h stars.h pilot.h debug.h rumble.h options.h file.h
space.o: sintab.h
intro.o: config.h elite.h planet.h trade.h sections.h dialog.h gfx.h texts.h
intro.o: vector.h shipdata.h threed.h space.h stars.h intro.h debug.h
intro.o: options.h
stars.o: config.h elite.h planet.h trade.h sections.h dialog.h gfx.h texts.h
stars.o: vector.h stars.h random.h debug.h threed.h space.h shipdata.h
stars.o: options.h
swat.o: config.h gfx.h elite.h planet.h trade.h sections.h dialog.h texts.h
swat.o: vector.h swat.h space.h shipdata.h main.h sound.h random.h pilot.h
swat.o: debug.h colors.h threed.h docked.h
vector.o: config.h vector.h sections.h debug.h
threed.o: config.h elite.h planet.h trade.h sections.h dialog.h gfx.h texts.h
threed.o: vector.h shipdata.h threed.h space.h random.h debug.h sintab.h
threed.o: options.h 3d_limit.h
rumble.o: elite.h planet.h trade.h sections.h dialog.h rumble.h
rumble.o: rumble/rumble_lib.h debug.h options.h
dialog.o: config.h elite.h planet.h trade.h sections.h dialog.h debug.h gfx.h
dialog.o: texts.h space.h vector.h shipdata.h docked.h sequences.h file.h
dialog.o: options.h sound.h
