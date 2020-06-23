all: battery.prc battry33.prc

battery.prc: code0001.drawrect.grc code0001.erasrect.grc code0001.getevent.grc code0001.drawline.grc 
	mv code0001.drawrect.grc code03e8.bin
	mv code0001.erasrect.grc code03e9.bin
	mv code0001.getevent.grc code03ea.bin
	mv code0001.drawline.grc code03eb.bin
	rm -f *.grc
	pilrc battery.rcp
	build-prc -t HACK battery.prc "Battery Meter Hack" THBM *.bin
	rm *.bin

battry33.prc: code0001.drawrect_33.grc code0001.erasrect_33.grc code0001.getevent_33.grc code0001.drawline_33.grc 
	mv code0001.drawrect_33.grc code03e8.bin
	mv code0001.erasrect_33.grc code03e9.bin
	mv code0001.getevent_33.grc code03ea.bin
	mv code0001.drawline_33.grc code03eb.bin
	rm -f *.grc
	pilrc battry33.rcp
	build-prc -t HACK battry33.prc "Battery Meter Hack" THBM *.bin
	rm *.bin

code0001.%.grc: %.s
	m68k-palmos-coff-as -mno-68881 -m68000 --defsym OS33=0 -l -o $* $^
	m68k-palmos-coff-obj-res $*
	rm $*

code0001.%_33.grc: %.s
	m68k-palmos-coff-as -mno-68881 -m68000 --defsym OS33=1 -l -o $*_33 $^
	m68k-palmos-coff-obj-res $*_33
	rm $*_33

dist: battery.prc battry33.prc
	make clean
	rm -f /usr/home/harbaum/WWW/pilot/battery*.zip
	zip /usr/home/harbaum/WWW/pilot/battery_prc.zip battery.prc battry33.prc readme.txt
	cd ..; zip /usr/home/harbaum/WWW/pilot/battery.zip -r battery

disk: battery.prc battry33.prc
	make clean
	rm -f ../battery.zip ../battery_prc.zip
	zip ../battery_prc.zip battery.prc battry33.prc readme.txt
	cd ..; zip battery.zip -r battery
	mcopy -o ../battery*.zip a:

clean:
	rm -f *.prc *.grc *~



