# build the graphics

CONVERT = convert
COPTS = -compress none -depth 8

INT = -interlace plane -depth 8

IM = tif

BLOCK = block

# make double size block
DBLOCK = $(BLOCK) $(BLOCK)_d $(BLOCK)_sm $(BLOCK)_sm_d

DBLOCKS = $(DBLOCK:=_red) $(DBLOCK:=_blue) $(DBLOCK:=_green) \
	$(DBLOCK:=_cyan) $(DBLOCK:=_yellow) $(DBLOCK:=_magenta) \
	$(DBLOCK:=_grey)

BLOCKS_BMP = $(DBLOCKS:=.8.bmp) $(DBLOCKS:=.4.bmp) \
	$(BLOCK).1.bmp $(BLOCK)_sm.1.bmp	

BG_BMP = bg.1.bmp bg.4.bmp bg.8.bmp

all: $(BLOCKS_BMP) $(BG_BMP)

colorize: colorize.c
	gcc -o $@ $<

# make raw grey image
%.raw: %.gif
	$(CONVERT) $< GRAY:$@

# build a red image
%_red.rgb: %.raw colorize
	./colorize red $*.raw $@

# build a green image
%_green.rgb: %.raw colorize
	./colorize green $*.raw $@

# build a blue image
%_blue.rgb: %.raw colorize
	./colorize blue $*.raw $@

# build a cyan image
%_cyan.rgb: %.raw colorize
	./colorize cyan $*.raw $@

# build a yellow image
%_yellow.rgb: %.raw colorize
	./colorize yellow $*.raw $@

# build a magenta image
%_magenta.rgb: %.raw colorize
	./colorize magenta $*.raw $@

# build a grey image
%_grey.rgb: %.raw colorize
	./colorize grey $*.raw $@

# convert according to file size
%.$(IM): %.rgb
	LEN=`ls -l $< | awk '{ print $$5}'`;\
	SIZE=`echo "sqrt($${LEN}/3)" | bc`;\
	$(CONVERT) $(INT) -size $${SIZE}x$${SIZE} RGB:$< $@

# pilrc wants bmp
%.8.bmp: %.$(IM)
	$(CONVERT) $< $(COPTS) $@

%.8.bmp: %.gif
	$(CONVERT) $< $(COPTS) $@

%.4.bmp: %.8.bmp
	$(CONVERT) +dither -colorspace GRAY $< $(COPTS) $@

%.1.bmp: %.1.gif
	$(CONVERT) $< $(COPTS) $@

clean:
	rm -f *.rgb *.raw *~ *_red.* *_blue.* *_green.* *.bmp
	rm -f *_cyan.* *_yellow.* *_magenta.* *_grey.* *.rgb_?
	rm -f colorize
