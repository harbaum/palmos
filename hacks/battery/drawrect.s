		# Battery Monitor Hack V1.0
		# (c) 1998 Till Harbaum
	
		# a218 WinDrawRectangle(rectPtr, cornerDiam)

	.include "header.s"

		LINK.W	%A6,#-4			|  4 Bytes (fuer TrapFtrGet)
		MOVEM.L	%D3-%D6/%A2/%A3,-(%A7)
		MOVEA.L	8(%A6),%A2
		PEA	-4(%A6)
		MOVE.W	#1000,-(%A7)
		MOVE.L	#THBM,-(%A7)
		TRAP	#15
		DC.W	0xa27b			| sysTrapFtrGet 
		MOVEA.L	-4(%A6),%A3		| alte Adresse merken
		ADDA.W	#10,%A7

		TRAP	#15
		DC.W	0xa175			| sysTrapFrmGetActiveFormID
		CMPI.W	#1000,%D0		| aktuelle FormID == 1000 ?
		BNE.B	ORIG_TRAP

		MOVE.L	(%A2),%D0		| X und Y-Koordinate
		CMPI.L	#(((ALTX0+1)*65536)+2),%D0 | Vergleich mit X==ALTX0+1 und Y==2
		BEQ.B	IS_BATT_MAIN		| das ist der Batteriekoerper

		CMPI.L	#((ALTX1*65536)+4),%D0	| Vergleich mit X==ALTX1 und Y==4
		BEQ.B	IS_BATT_CAP		| das ist der Batterieknopf
		
ORIG_TRAP:	MOVE.W	12(%A6),-(%A7)
		MOVE.L	%A2,-(%A7)
		JSR	(%A3)			| alten Trap anspringen
EXIT_TRAP:	MOVEM.L	-28(%A6),%D3-%D6/%A2/%A3
		UNLK	%A6
		RTS

IS_BATT_MAIN:	CMPI.W	#8,6(%A2)		| Hoehe == 8
		BNE.B	ORIG_TRAP		| Noe, kein Batteriekoerper
		MOVE.W	4(%A2),%D4
		MULS.W	#(NEUX1-NEUX0-1),%D4	| neue Breite
		DIVS.W	#(ALTX1-ALTX0-1),%D4
		MOVE.W	#(NEUX0+1-OFFX),%D5
		BRA.B	CONT0		
		
IS_BATT_CAP:	CMPI.W	#4,6(%A2)		| Hoehe == 4?
		BNE.B	ORIG_TRAP		| Noe, wohl doch kein Knopf
		MOVE.W	#(NEUX1-OFFX),%D5	| neue X-Koordinate
		MOVE.W	4(%A2),%D4
	.if (!OS33)
		SUBQ.W	#1,%D4			| W = W - 1
	.endif
	
CONT0:		MOVEP.L	1(%A2),%D6		| Koordinaten sichern	
		ADDI.W	#1,2(%A2)		| Batterie ein Pixel
		SUBI.W	#1,6(%A2)		| flacher
	
		MOVE.W	%D4,4(%A2)		| neue Breite
		MOVE.W	%D5,(%A2)		| neue X-Koordinate

		MOVE.W	12(%A6),-(%A7)		| DrawRect vorbereiten
		MOVE.L	%A2,-(%A7)

		CLR.L	-4(%A6)			| Default-Wert (schwarz/Prozent)
		PEA	-4(%A6)
		CLR.W	-(%A7)			| ID=0 
		MOVE.L	#THBM,-(%A7)		
		TRAP	#15
		DC.W	0xa27b			| sysTrapFtrGet	
		ADDA.W	#10,%A7

		MOVE.B	-1(%A6),%D3
		BTST.B	#0,%D3			| Preferences
		BNE.B	DRAW_GREY
	
		JSR	(%A3)			| alten Trap anspringen

DRAW_RET:	MOVEP.L	%D6,1(%A2)		| Koordinaten herstellen
	
		CMPI.W	#ALTX1,(%A2)		| nur Batterie-Spitze gemalt?
		BEQ	EXIT_TRAP
	
DRAW_TEXT:	LINK	%A6,#-6

		PEA	-6(%A6)			| fill level in percent
		CLR.L	-(%A7)			| plugged in
		CLR.L	-(%A7)			| battery kind
		CLR.L	-(%A7)			| max ticks
		CLR.L	-(%A7)			| critical threshold
		CLR.L	-(%A7)			| warning threshold
		CLR.B	-(%A7)			| set
		TRAP	#15
		DC.W	0xa324			| sysTrapSysBatteryInfo
		ADDA.W	#26,%A7

		MOVE.L	#((TEXTX-OFFX)*65536)+1,-(%A7)	| Textkoordinaten

		BTST.B	#1,%D3			| 0 = Prozente
		BNE.B	DISP_VOLTAGE
		CLR.L	%D0
		MOVE.B	-6(%A6),%D0		| Prozente nach D0
DISP_VOLTAGE:	EXT.L	%D0

		MOVE.L	%D0,-(%A7)
		PEA	-6(%A6)
		TRAP	#15
		DC.W	0xa0c9			| sysTrapStrIToA
		ADDQ.W	#8,%A7

		BTST.B	#1,%D3
		BEQ.B	DISP_PERCENT		| Prozente
		MOVE.B	-4(%A6),-3(%A6)		| Nachkommastellen	
		MOVE.B	-5(%A6),-4(%A6)		| verschieben
		MOVE.B	#'.',-5(%A6)		| Punkt
		MOVE.B	#'V',-2(%A6)		| und V dran

		MOVE.W	#5,-(%A7)
		BRA	DISPLAY

DRAW_GREY:	PEA	GREY_PATTERN(%PC)	| Muster auf Grauraster
		TRAP	#15			| schalten
		DC.W	0xa224			| sysTrapWinSetPattern
		ADDQ.L	#4,%A7
			
		TRAP	#15			| grau malen
		DC.W	0xa229			| sysTrapWinFillRectangle
		BRA.B	DRAW_RET

GREY_PATTERN:	DC.L    0x55aa55aa,0x55aa55aa,0x55aa55aa,0x55aa55aa
		
PERCENT_CHAR:	DC.B	'%',0			| das Prozentzeichen
PERC_CLEAR:	DC.W	(TEXTX-OFFX),1,23,9	| Breite von '100%'
	
DISP_PERCENT:	
		# Prozentausgabe kann in
		# der Laenge variieren
		CLR.W	-(%A7)
		PEA	PERC_CLEAR(%PC)		| Bereich loeschen
		TRAP	#15
		DC.W	0xa219			| SysTrapWinEraseRectangle

		PEA	PERCENT_CHAR(%PC)
		PEA	-6(%A6)
		TRAP	#15
		DC.W	0xa0c6			| sysTrapStrCat

		ADDA.W	#14,%A7
	
		PEA	-6(%A6)			| sonst Laenge
		TRAP	#15			| berechnen lassen
		DC.W	0xa0c7			| sysTrapStrLen
		ADDQ.W	#4,%A7
		MOVE.W	%D0,-(%A7)		| Stringlaenge
			
DISPLAY:	PEA	-6(%A6)			| und Daten
		TRAP	#15			| ausgeben
		DC.W	0xa220			| sysTrapWinDrawChars

		UNLK	%A6
		BRA	EXIT_TRAP

