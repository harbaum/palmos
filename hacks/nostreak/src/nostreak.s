	# no streak hack
	# (c) 1999 Till Harbaum
	# GPL'd freeware, see COPYING for details
	
	# to be linked to trap A11D (EvtGetEvent)
	
	link	%a6,#-8			| get space for two variables
	pea	-8(%a6)			| return address goes here
	move.w	#1000,-(%a7)
	move.l	#0x4e537472,-(%a7)	| 'NStr' - NoStreak
	trap	#15
	dc.w	0xa27b			| sysTrapFtrGet
	lea	10(%a7),%a7
	
	pea	-4(%a6)			| refresh value goes here
	clr	-(%a7)
	move.l	#0x4e537472,-(%a7)	| 'NStr' - NoStreak
	trap	#15
	dc.w	0xa27b			| sysTrapFtrGet
	lea	10(%a7),%a7
	tst	%d0			| no such feature?
	bne.s	quit                    | -> exit

	move.l 	#0xfffffa20,%a0		| LCD panel interface configuration 
	move.b	(%a0),%d0
		
	and.b	#3,%d0			| GS0/GS1 (greyscale?)
	beq.s	ok			| this is b&w mode, run nostreak always

	btst.b	#4,-1(%a6)		| is nostreak activated for greyscale?
	bne.s	ok

	clr.b	5(%a0)			| use pixel clock directly (for greyscale)
	bra.s	quit

ok:	
	move.l 	#0xfffffa20,%a0		| LCD panel interface configuration 
	and.b	#0x0f, -1(%a6)		| mask greyscale bit
	move.b	-1(%a6),5(%a0)		| load LCD pixel clock devider

quit:	
	move.l 	-8(%a6),%a0		| original address
	unlk	%a6			| free local variable
	jmp	(%a0)			| jump to original trap
