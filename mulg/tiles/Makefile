# create two bit/pixel tiles
gray2/tile0000.bin: gray2n
	for i in gray2/*.gif ; do convert $$i gray:$$i.gray ; done
	gray2n 2 gray2/tile0000.bin gray2/*.gray
	rm gray2/*.gray

# create four bit/pixel tiles from color tiles
gray4/tile0000.bin: gray2n
	for i in color/*.gif ; do convert $$i gray:$$i.gray ; done
	gray2n 4 gray4/tile0000.bin color/*.gray
	rm color/*.gray

# create four bit/pixel tiles
color/tile0000.raw: mkcolor
	for i in color/*.gif ; do convert $$i RGB:$$i.rgb ; done
	mkcolor color/tile0000.raw color/*.rgb
	rm color/*.rgb

gray2n: gray2n.c
	gcc -o gray2n gray2n.c

mkcolor: mkcolor.c coltab.h
	gcc -o mkcolor mkcolor.c

view:	mkcolor
	mkcolor rgb.raw
	convert -size 16x16 RGB:rgb.raw rgb.GIF
	xv rgb.GIF

clean:
	rm -rf *~ gray2n mkcolor rgb.GIF */tile0000.bin */tile0000.raw
	rm -rf */.xvpics */*/.xvpics
