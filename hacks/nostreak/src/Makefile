../nostreak.prc: code0001.nostreak.grc code0001.nostreak_cfg.grc
	mv code0001.nostreak.grc code03e8.bin
	mv code0001.nostreak_cfg.grc code07d0.bin
	pilrc nostreak.rcp
	build-prc -t HACK ../nostreak.prc "No Streak Hack" NStr *.bin
	rm *.bin *.grc

code0001.%.grc: %.s
	m68k-palmos-coff-as -mno-68881 -m68000 -l -o $* $^
	m68k-palmos-coff-obj-res $*
	rm $*

code0001.%.grc: %.c
	m68k-palmos-coff-gcc -O3 -Wall -nostartfiles -o $* $^
	m68k-palmos-coff-obj-res $*
	rm $*

dist: ../nostreak.prc
	rm -f *~ ../*~ /usr/home/harbaum/WWW/pilot/nostreak.zip
	cd ..; zip /usr/home/harbaum/WWW/pilot/nostreak.zip -r .

