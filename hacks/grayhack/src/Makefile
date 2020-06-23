HACK=gray
ID=GrHa

ICONS = col0.bmp col1.bmp col2.bmp col3.bmp

all: ../grayhack.prc

code0001.%.grc: %.c
	m68k-palmos-gcc -palmos3.1 -O3 -Wall -DCREATOR=\'GrHa\' -nostartfiles -o $* $^
	m68k-palmos-obj-res $*
	rm $*

CONVERT = -compress none -depth 8

%.bmp: %.gif
	convert $< $(CONVERT) $@

../grayhack.prc: code0001.ghWinScreenMode.grc code0001.ghcfg.grc grayhack.rcp $(ICONS)
	mv code0001.ghWinScreenMode.grc code03e8.bin
	mv code0001.ghcfg.grc code07d0.bin
	pilrc grayhack.rcp
	build-prc -t HACK ../grayhack.prc "GrayHack" $(ID) *.bin
	rm *.bin *.grc *.bmp

clean:
	rm -f *.bin *.grc *~ ../*~

dist:
	make clean
	rm -f ../../grayhack.zip ../../grayhack_src.zip
	cd ../..; zip -r grayhack_src.zip grayhack
	cd ..; zip ../grayhack.zip grayhack.prc readme.txt COPYING

disk:
	make dist
	mcopy -o ../../grayhack*.zip a:
