# axxPac JPEG plugin library

LIBNAME   = axxPacJPEG
CREATOR   = axP0

TOOLBASE = m68k-palmos-
CC = $(TOOLBASE)gcc -nostartfiles
BUILDPRC = build-prc
CSFLAGS = -palmos3.1 -O2 -S
CFLAGS = -palmos3.1 -O2 -DCREATOR=\'$(CREATOR)\' -DPALM
PILRC = pilrc

OBJECTS= jpeg_view.o config.o jcomapi.o jdapimin.o jdapistd.o \
        jdatadst.o jdatasrc.o jdcoefct.o jdcolor.o jddctmgr.o jdhuff.o \
        jdinput.o jdmainct.o jdmaster.o jdmerge.o jdphuff.o \
        jdsample.o jdtrans.o jerror.o jidctred.o jutils.o jmemmgr.o \
	jdpostct.o jdmarker.o djpeg.o palmview.o jmempalm.o

all: ../$(LIBNAME).prc

jpeg_view.o: jpeg_view.c
	$(CC) $(CFLAGS) -c $<

%.o: %.c
	$(CC) $(CFLAGS) -fomit-frame-pointer -c $<

../$(LIBNAME).prc: $(LIBNAME) res.stamp
	$(BUILDPRC) -o $@ jpeg.def $(LIBNAME) *.bin
	m68k-palmos-objdump --section-headers $(LIBNAME)

#../$(LIBNAME).prc: libr0000.$(LIBNAME).grc res.stamp
#	$(BUILDPRC) $@ "axxPac JPEG plugin" $(CREATOR)\
#			 libr0000.$(LIBNAME).grc *.bin
#	m68k-palmos-coff-objdump --section-headers $(LIBNAME)

#libr0000.$(LIBNAME).grc: $(LIBNAME)
#	$(OBJRES) $(LIBNAME)
#	rm -f {code,pref,data}0000.$(LIBNAME).grc
#	mv code0001.$(LIBNAME).grc libr0000.$(LIBNAME).grc

res.stamp: config.rcp config.h
	$(PILRC) config.rcp .
	touch res.stamp

$(LIBNAME): $(OBJECTS)
	$(CC) -o $(LIBNAME) $(OBJECTS) jpeg.ld

clean:
	rm -f *.grc *.o $(LIBNAME) *~ res.stamp

depend:
	makedepend *.c

# DO NOT DELETE


config.o: ../api/axxPac.h cderror.h cdjpeg.h config.h jconfig.h jerror.h
config.o: jinclude.h jmorecfg.h jpeglib.h
djpeg.o: ../api/axxPac.h cderror.h cdjpeg.h jconfig.h jerror.h jinclude.h
djpeg.o: jmorecfg.h jpeglib.h jversion.h
jcomapi.o: ../api/axxPac.h jconfig.h jerror.h jinclude.h jmorecfg.h jpegint.h
jcomapi.o: jpeglib.h
jdapimin.o: ../api/axxPac.h jconfig.h jerror.h jinclude.h jmorecfg.h jpegint.h
jdapimin.o: jpeglib.h
jdapistd.o: ../api/axxPac.h jconfig.h jerror.h jinclude.h jmorecfg.h jpegint.h
jdapistd.o: jpeglib.h
jdatadst.o: ../api/axxPac.h jconfig.h jerror.h jinclude.h jmorecfg.h jpeglib.h
jdatasrc.o: ../api/axxPac.h jconfig.h jerror.h jinclude.h jmorecfg.h jpeglib.h
jdcoefct.o: ../api/axxPac.h jconfig.h jerror.h jinclude.h jmorecfg.h jpegint.h
jdcoefct.o: jpeglib.h
jdcolor.o: ../api/axxPac.h jconfig.h jerror.h jinclude.h jmorecfg.h jpegint.h
jdcolor.o: jpeglib.h
jddctmgr.o: ../api/axxPac.h jconfig.h jdct.h jerror.h jinclude.h jmorecfg.h
jddctmgr.o: jpegint.h jpeglib.h
jdhuff.o: ../api/axxPac.h jconfig.h jdhuff.h jerror.h jinclude.h jmorecfg.h
jdhuff.o: jpegint.h jpeglib.h
jdinput.o: ../api/axxPac.h jconfig.h jerror.h jinclude.h jmorecfg.h jpegint.h
jdinput.o: jpeglib.h
jdmainct.o: ../api/axxPac.h jconfig.h jerror.h jinclude.h jmorecfg.h jpegint.h
jdmainct.o: jpeglib.h
jdmarker.o: ../api/axxPac.h jconfig.h jerror.h jinclude.h jmorecfg.h jpegint.h
jdmarker.o: jpeglib.h
jdmaster.o: ../api/axxPac.h jconfig.h jerror.h jinclude.h jmorecfg.h jpegint.h
jdmaster.o: jpeglib.h
jdmerge.o: ../api/axxPac.h jconfig.h jerror.h jinclude.h jmorecfg.h jpegint.h
jdmerge.o: jpeglib.h
jdphuff.o: ../api/axxPac.h jconfig.h jdhuff.h jerror.h jinclude.h jmorecfg.h
jdphuff.o: jpegint.h jpeglib.h
jdpostct.o: ../api/axxPac.h jconfig.h jerror.h jinclude.h jmorecfg.h jpegint.h
jdpostct.o: jpeglib.h
jdsample.o: ../api/axxPac.h jconfig.h jerror.h jinclude.h jmorecfg.h jpegint.h
jdsample.o: jpeglib.h
jdtrans.o: ../api/axxPac.h jconfig.h jerror.h jinclude.h jmorecfg.h jpegint.h
jdtrans.o: jpeglib.h
jerror.o: ../api/axxPac.h jconfig.h jerror.h jinclude.h jmorecfg.h jpeglib.h
jerror.o: jversion.h
jidctred.o: ../api/axxPac.h jconfig.h jdct.h jerror.h jinclude.h jmorecfg.h
jidctred.o: jpegint.h jpeglib.h
jmemmgr.o: ../api/axxPac.h jconfig.h jerror.h jinclude.h jmemsys.h jmorecfg.h
jmemmgr.o: jpegint.h jpeglib.h
jmempalm.o: ../api/axxPac.h jconfig.h jerror.h jinclude.h jmemsys.h jmorecfg.h
jmempalm.o: jpegint.h jpeglib.h
jpeg_view.o: ../api/axxPac.h jconfig.h jinclude.h
jutils.o: ../api/axxPac.h jconfig.h jerror.h jinclude.h jmorecfg.h jpegint.h
jutils.o: jpeglib.h
palmview.o: ../api/axxPac.h cderror.h cdjpeg.h jconfig.h jerror.h jinclude.h
palmview.o: jmorecfg.h jpeglib.h
viewpalm.o: ../api/axxPac.h cderror.h cdjpeg.h jconfig.h jerror.h jinclude.h
viewpalm.o: jmorecfg.h jpeglib.h
