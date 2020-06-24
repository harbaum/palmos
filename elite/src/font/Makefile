# /usr/share/kbd/consolefonts

all: font0000.raw

%.bit: %.gif
	convert $< -crop 768x6+0+0 -negate MONO:$@

%.raw: %.bit bitrev
	./bitrev <$< >$@

%.spc: %.bit mkfnttab
	./mkfnttab $< > $@

mkfnttab: mkfnttab.c

bitrev: bitrev.c

font0000.raw: font.spc font.raw
	cat font.spc font.raw > font0000.raw

clean:
	rm -f font_r.raw font.raw font.spc font0000.raw bitrev 
	rm -f mkfnttab *~ *.bit

