HACK=af
ID=AfHa

code0001.%.grc: %.c
	m68k-palmos-coff-gcc -O3 -Wall -DCREATOR=\'AfHa\' -nostartfiles -o $* $^
	m68k-palmos-coff-obj-res $*
	rm $*

../$(HACK)hack.prc: code0001.$(HACK)KeyCurrentState.grc code0001.$(HACK)cfg.grc $(HACK)hack.rcp
	mv code0001.$(HACK)KeyCurrentState.grc code03e8.bin
	mv code0001.$(HACK)cfg.grc code07d0.bin
	pilrc $(HACK)hack.rcp
	build-prc -t HACK ../$(HACK)hack.prc "AutoFireHack" $(ID) *.bin
	rm *.bin *.grc

clean:
	rm -f *.bin *.grc *~ ../*~

dist:
	make clean
	rm -f ../../$(HACK)hack.zip ../../$(HACK)hack_src.zip
	cd ../..; zip -r $(HACK)hack_src.zip $(HACK)hack
	cd ..; zip ../$(HACK)hack.zip $(HACK)hack.prc readme.txt	

disk:
	make dist
	mcopy -o ../../$(HACK)hack*.zip a:
