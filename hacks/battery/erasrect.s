		# Battery Monitor Hack V1.0
		# (c) 1998 Till Harbaum
	
		# A219 WinEraseRectangle(rectPtr, cornerDiam)
		.include "header.s"
		
		LINK	%A6,#-4			|  4 Bytes (fuer FtrGet)
		MOVEM.L	%A2/%A3,-(%A7)
		MOVEA.L	8(%A6),%A2 
		PEA	-4(%A6)
		MOVE.W	#1001,-(%A7)
		MOVE.L	#THBM,-(%A7)
		TRAP	#15
		DC.W	0xa27b			| sysTrapFtrGet
		MOVEA.L	-4(%A6),%A3		| originale Adresse
		ADDA.W	#10,%A7
		
		TRAP	#15
		DC.W    0xa175			| sysTrapFrmGetActiveFormID
		CMPI.W	#1000,%D0		| Form == 1000
		BNE.B	ORIG_TRAP
	
		MOVE.L	(%A2),%D0		| X und Y-Koordinate
		CMPI.L	#((ALTX1*65536)+4),%D0	| Batterieknopf?
		BEQ.B	IS_BATT_CAP
		
		ADD.L	4(%A2),%D0		| Breite und Hoehe 
		CMPI.L	#((ALTX1*65536)+10),%D0	| Batteriekoerper?
		BEQ.B	IS_BATT_MAIN

ORIG_TRAP:	MOVE.W	12(%A6),-(%A7)
		MOVE.L	%A2,-(%A7)
		JSR	(%A3)
EXIT_TRAP:	MOVEM.L	-12(%A6),%A2/%A3	| gesicherte A3 und A2
		UNLK	%A6
		RTS

IS_BATT_CAP:	MOVE.W	#NEUX1-OFFX,(%A2)	| dann neuer Knopfanfang
		BRA.B	DRAW
	
IS_BATT_MAIN:	MOVE.W	(%A2),%D0
		SUB.W	#ALTX0, %D0
		MULS.W	#(NEUX1-NEUX0-1),%D0	| neue Breite
		DIVS.W	#(ALTX1-ALTX0-1),%D0	| alte Breite
		ADD.W	#NEUX0-OFFX, %D0
		MOVE.W	%D0,(%A2)

		MOVEQ	#NEUX1-OFFX,%D1
		SUB.W	%D0,%D1
		MOVE.W	%D1,4(%A2)		| Breite = NEUX1 - X
	
DRAW:		ADDI.W	#1,2(%A2)		| Batterie ein Pixel
		SUBI.W	#1,6(%A2)		| flacher

		BRA.B	ORIG_TRAP





