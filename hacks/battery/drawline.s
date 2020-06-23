		# Battery Monitor Hack V1.0
		# (c) 1998 Till Harbaum
	
		# A213 WinDrawLine(short x1,y1,x2,y2)
		.include "header.s"
	
		LINK	%A6,#-4			| 4 Bytes (fuer FtrGet)
		MOVEM.L	%D3-%D4/%A2,-(%A7)

		PEA	-4(%A6)
		MOVE.W	#1003,-(%A7)
		MOVE.L	#THBM,-(%A7)
		TRAP	#15
		DC.W	0xa27b			| sysTrapFtrGet
		MOVEA.L	-4(%A6),%A2		| originaler Trap
		ADDA.W	#10,%A7

                CMP.W	#10,10(%A6)		| Y1
		BHI.B	ORIG_TRAP
		
		MOVEP.L	9(%A6),%D3		| Trick! Alle Werte in ein Register!!

		LEA	CONV_TABLE(%PC),%A0	| Zeiger auf Umwandlungstabelle
		MOVEQ.L	#7,%D4			| 8 Linien zu versetzen
COMP_LOOP:	CMP.L	(%A0)+,%D3		| eine davon?
		BEQ.B	MODIFY
		DBF	%D4,COMP_LOOP		| weitersuchen
	
ORIG_TRAP:	MOVE.L	12(%A6),-(%A7)
		MOVE.L	8(%A6),-(%A7)
		JSR	(%A2)
EXIT_TRAP:	MOVEM.L	-16(%A6),%D3-%D4/%A2
		UNLK	%A6
		RTS

MODIFY:		MOVE.L	28(%A0),%D4		| Lese neuen Wert
	
		TRAP	#15
		DC.W	0xa175			| sysTrapFrmGetActiveFormID
		CMPI.W	#1000,%D0		| aktive Form=1000?
		BNE.B	ORIG_TRAP

		MOVEP.L	%D4,9(%A6)		| Schreibe neuen Wert	
		BRA.B	ORIG_TRAP

	
CONV_TABLE:	DC.B	     ALTX0,  1,      ALTX0, 10	| links senkrecht
		DC.B	     ALTX0,  1,      ALTX1,  1	| oben lang
		DC.B	     ALTX0, 10,      ALTX1, 10	| unten lang
		DC.B	     ALTX1,  1,      ALTX1,  3	| Knopf 1
		DC.B	     ALTX1,  3,      ALTX2,  3	| Knopf 2
		DC.B	     ALTX2,  3,      ALTX2,  8	| Knopf 3
		DC.B	     ALTX1,  8,      ALTX2,  8	| Knopf 4
		DC.B	     ALTX1, 10,      ALTX1,  8	| Knopf 5

		DC.B	NEUX0-OFFX,  2, NEUX0-OFFX, 10	| links senkrecht
		DC.B	NEUX0-OFFX,  2, NEUX1-OFFX,  2	| oben lang
		DC.B	NEUX0-OFFX, 10, NEUX1-OFFX, 10	| unten lang
		DC.B	NEUX1-OFFX,  2, NEUX1-OFFX,  4	| Knopf 1
		DC.B	NEUX1-OFFX,  4, NEUX2-OFFX,  4	| Knopf 2
		DC.B	NEUX2-OFFX,  4, NEUX2-OFFX,  8	| Knopf 3
		DC.B	NEUX1-OFFX,  8, NEUX2-OFFX,  8	| Knopf 4
		DC.B	NEUX1-OFFX, 10, NEUX1-OFFX,  8	| Knopf 5
