#
# Makefile for axxPac
#
# 01'2000 - Till Harbaum
#

ARCHIVE      = axxpac
ARCHIVE_SRC  = axxpac_src
ARCHIVE_JPEG = axxpac_jpeg
ARCHIVE_FTP = axxpac_ftp
DEBUGTTY     = /dev/ttyS0

all::
	cd driver; make
	cd explorer; make
	cd jpeg_view; make
	cd ftp; make

clean::
	rm -f *~ api/*~ core SmartMedia.16m
	cd driver; make clean
	cd explorer; make clean
	cd jpeg_view; make clean
	cd ftp; make clean

depend::
	cd driver; make depend
	cd explorer; make depend
	cd jpeg_view; make depend
	cd ftp; make depend

dist::
	make clean
	rm -f ../$(ARCHIVE_SRC).zip ../$(ARCHIVE).zip ../$(ARCHIVE_JPEG).zip ../$(ARCHIVE_FTP).zip
	cd ..; zip -r $(ARCHIVE_SRC).zip axxpac
	zip -r ../$(ARCHIVE).zip axxPacShell.prc axxPacDrv.prc readme.txt
	zip -r ../$(ARCHIVE_JPEG).zip axxPacJPEG.prc axxPacJPEG.txt
	zip -r ../$(ARCHIVE_FTP).zip COPYING axxPacFTP.prc axxPacFTP.txt ftp

www::
	make dist
	cp ../$(ARCHIVE_FTP).zip /usr/home/harbaum/WWW/pilot/ftp

disk::
	make dist
	mcopy -o ../$(ARCHIVE_SRC).zip a:

debug:: 
	cd driver; make
	pilot-xfer $(DEBUGTTY) -i axxPacDrv.prc 
	stty cs8 19200 <$(DEBUGTTY)
	cat $(DEBUGTTY) > trace.txt

install::
	make
	pilot-xfer $(PALM) -i axxPacShell.prc axxPacDrv.prc axxPacJPEG.prc





