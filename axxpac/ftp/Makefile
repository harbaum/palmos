# axxPac FTP program

PROGNAME  = axxPacFTP
CREATOR   = axPF

OBJECTS   = axxPacFTP.o ftp.o filio.o fsel.o

TOOLBASE = m68k-palmos-
CC = $(TOOLBASE)gcc
LD = m68k-palmos-gcc
XTRALIBS =
BUILDPRC = build-prc
PILRC = pilrc
CONVERT = convert
COPTS = -compress none

ICONP = icons
ICON  =  $(ICONP)/ftp.8.bmp $(ICONP)/ftp.4.bmp \
	 $(ICONP)/ftp.2.bmp $(ICONP)/ftp.1.bmp 
ICONS =  $(ICONP)/ftp_sm.8.bmp $(ICONP)/ftp_sm.4.bmp \
	 $(ICONP)/ftp_sm.2.bmp $(ICONP)/ftp_sm.1.bmp 
FOLDER = $(ICONP)/folder.8.bmp $(ICONP)/folder.4.bmp \
	 $(ICONP)/folder.2.bmp $(ICONP)/folder.1.bmp 
BUSY   = $(ICONP)/busy.8.bmp $(ICONP)/busy.4.bmp \
	 $(ICONP)/busy.2.bmp $(ICONP)/busy.1.bmp 
BACK   = $(ICONP)/back.8.bmp $(ICONP)/back.4.bmp \
	 $(ICONP)/back.2.bmp $(ICONP)/back.1.bmp 
GRAPHICS = $(ICON) $(ICONS) $(FOLDER) $(BUSY) $(BACK)


ICONTEXT = "FTP"

CSFLAGS = -O2 -S
#DEBUG = -DDEBUG
CFLAGS = -O2 -palmos4 -DCREATOR=\'$(CREATOR)\' $(DEBUG)

all: ../$(PROGNAME).prc

%.bmp: %.gif
	$(CONVERT) $< $(COPTS) $@

%.4.bmp: %.8.bmp
	convert +dither -colorspace GRAY $< $(COPTS) $@

%.2.bmp: %.4.bmp $(ICONP)/palm_2.gif
	convert +dither $< -map $(ICONP)/palm_2.gif $(COPTS) $@

../$(PROGNAME).prc: res.stamp $(PROGNAME)
	$(BUILDPRC) ../$(PROGNAME).prc $(ICONTEXT) $(CREATOR) $(PROGNAME) *.bin

res.stamp: ftp.rcp ftpRsc.h fsel.rcp fselRsc.h $(GRAPHICS)
	$(PILRC) fsel.rcp .
	$(PILRC) ftp.rcp .
	touch res.stamp

$(PROGNAME): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(PROGNAME)

ftp: ftp.c
	gcc -o ftp ftp.c

clean:
	rm -f *~ */*~ core $(PROGNAME) *.bin *.grc *.o *.stamp */*.bmp

depend:
	makedepend *.c

www:	$(PROGNAME)
	rm -f ~/WWW/pilot/ftp/axxpac_ftp.zip
	cd ..; zip ~/WWW/pilot/ftp/axxpac_ftp.zip $(PROGNAME).prc $(PROGNAME).txt

dist:	
	make www

# DO NOT DELETE


axxPacFTP.o: ../api/axxPac.h axxPacFTP.h ftp.h ftpRsc.h
ftp.o: ../api/axxPac.h axxPacFTP.h ftp.h ftpRsc.h
