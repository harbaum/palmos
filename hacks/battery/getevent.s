		# Battery Monitor Hack V1.0
		# (c) 1998 Till Harbaum
	
		# A11D EvtGetEvent(&event, waitforever...)
		.include "header.s"

		LINK	%A6,#-16		| 16 Bytes 
		MOVEM.L	%D3/%D4/%A2,-(%A7)
		MOVEA.L	8(%A6),%A2
		MOVE.L	12(%A6),%D3
		PEA	-4(%A6)
		MOVE.W	#1002,-(%A7)
		MOVE.L	#THBM,-(%A7)
		TRAP	#15
		DC.W	0xa27b			| sysTrapFtrGet
		MOVEA.L	-4(%A6),%A0
		MOVE.L	%D3,-(%A7)
		MOVE.L	%A2,-(%A7)
		JSR	(%A0)			| originale Funktion
		ADDA.W	#18,%A7

		CMPI.W	#1,(%A2)		| penDownEvent
		BNE	EXIT_TRAP

		MOVE.W	4(%A2),%D0		| screenX
		ADDI.W	#-(NEUX0-OFFX),%D0
		CMPI.W	#((TEXTX+20)-NEUX0),%D0	| zwischen NEUX0 und Text-Ende 
		BHI	EXIT_TRAP

		MOVE.W	6(%A2),%D0		| screenY
		SUBQ.W	#1,%D0
		CMPI.W	#8,%D0			| zwischen 1 und 9
		BHI	EXIT_TRAP

		PEA	-10(%A6)		| Puffer fuer Database-ID
		PEA	-6(%A6)			| Puffer fuer Card-ID
		TRAP	#15
		DC.W	0xa0ac			| sysTrapSysCurAppDatabase

		PEA	-14(%A6)		| Puffer fuer Creator-ID
		MOVE.W	#9,%D0
CLR_LOOP:	CLR.L	-(%A7)
		DBF	%D0,CLR_LOOP
		MOVE.L	-10(%A6),-(%A7)		| Database-ID
		MOVE.W	-6(%A6),-(%A7)		| Card-ID
		TRAP	#15
		DC.W	0xa046			| sysTrapDmDatabaseInfo
		ADDA.W	#58,%A7
		CMPI.L	#0x6c6e6368,-14(%A6)	| 'lnch'
		BNE	EXIT_TRAP		| nicht im Launcher -> Ende

		TRAP	#15
		DC.W	0xa1ff			| sysTrapWinGetActiveWindow
		MOVE.L	%A0,%D3
		MOVE.W	#1000,-(%A7)
		TRAP	#15
		DC.W	0xa17e			| sysTrapFrmGetFormPtr
		ADDQ.W	#2,%A7
		CMPA.L	%D3,%A0			| Active Window==FormPtr(1000)?
		BNE.B	EXIT_TRAP		| Noe -> Ende

		MOVE.W	#1,-(%A7)
		MOVE.L	#THBM,-(%A7)		| Creator
		MOVE.L	#0x4841434b,-(%A7)	| DB-Id (HACK)
		TRAP	#15
		DC.W	0xa075			| sysTrapDmOpenDatabaseByTypeCreator
		MOVEA.L	%A0,%A2
		ADDA.W	#10,%A7

		CMPA.W	#0,%A2			| DB existiert nicht
		BEQ.B	EXIT_TRAP

		MOVE.W	#2000,-(%A7)		| Rsrc-ID
		TRAP	#15
		DC.W	0xa16f			| sysTrapFrmInitForm
		MOVE.L	%A0,%D3			| FormPtr

		PEA	EVENT_HANDLER(%PC)	| EvntHndlPtr
		MOVE.L	%D3,-(%A7)		| FormPtr
		TRAP	#15
		DC.W	0xa19f			| sysTrapFrmSetEventHandler

		MOVE.B	#7,-(%A7)		| sndClick
		TRAP	#15
		DC.W	0xa234			| sysTrapSndPlaySystemSound

		MOVE.L	%D3,-(%A7)		| FormPtr
		TRAP	#15
		DC.W	0xa193			| sysTrapFrmDoDialog
		MOVE.W	%D0,%D4			| ExitObject

		MOVE.L	%D3,-(%A7)		| FormPtr
		TRAP	#15
		DC.W	0xa170			| sysTrapFrmDeleteForm

		MOVE.L	%A2,-(%A7)
		TRAP	#15
		DC.W	0xa04a			| sysTrapDmCloseDatabase
		ADDA.W	#24,%A7

		CMPI.W	#3001,%D4		| Exit-Object == OK-Button?
		BNE.B	EXIT_TRAP		| Noe -> Ende

		CLR.L	-(%A7)			| restart Launcher
		CLR.W	-(%A7)
		MOVE.L	-10(%A6),-(%A7)		| Database-ID
		MOVE.W	-6(%A6),-(%A7)		| Card-ID
		TRAP	#15
		DC.W	0xa0a7			| sysTrapSysUIAppSwitch

EXIT_TRAP:	MOVEM.L	-28(%A6),%D3/%D4/%A2
		UNLK	%A6
		RTS

EVENT_HANDLER:	LINK	%A6,#-12
		MOVEM.L	%D3-%D5/%A2,-(%A7)
		MOVEA.L	8(%A6),%A2

		MOVE.W	#2004,%D0		| Battery-Type-Liste
		BSR	GET_OBJ_ADDR
		MOVE.L	%A0,%D3

		MOVE.W	#2006,%D0		| Info-Liste
		BSR	GET_OBJ_ADDR
		MOVE.L	%A0,%D4

		MOVE.W	(%A2),%D0		| Event
	
		CMPI.W	#5,%D0			| winEnterEvent
		BEQ.B	EVT_WIN_ENTER

		CMPI.W	#9,%D0			| ctlSelectEvent
		BEQ	EVT_CTL_SELECT

		BRA	EVT_EXIT

EVT_WIN_ENTER:	MOVE.W	#1000,-(%A7)
		TRAP	#15
		DC.W	0xa17e			| sysTrapFrmGetFormPtr
		ADDQ.W	#2,%A7
		CMPA.L	12(%A2),%A0		| enter form 1000?
		BNE	EVT_EXIT		| nein -> exit

		# Batterie-Typ eintragen
		CLR.L	-(%A7)
		CLR.L	-(%A7)
		PEA	-1(%A6)			| Batterie-Typ
		CLR.L	-(%A7)
		CLR.L	-(%A7)
		CLR.L	-(%A7)
		CLR.B	-(%A7)
		TRAP	#15
		DC.W	0xa324			| sysTrapSysBatteryInfo
		CLR.W	%D0
	
		MOVE.B	-1(%A6),%D0
		MOVE.W	%D0,-(%A7)		| Batterie-Typ (Alkaline, NiCd/NiMH/Litium)
		MOVE.L	%D3,-(%A7)		| Typ-Listenadresse
		TRAP	#15
		DC.W	0xa1b7			| sysTrapLstSetSelection
		ADDA.W	#32,%A7
		CLR.W	%D0

		MOVE.B	-1(%A6),%D0
		MOVE.W	%D0,-(%A7)		| Batterie-Typ
		MOVE.L	%D3,-(%A7)		| Typ-Listenadresse
		TRAP	#15
		DC.W	0xa1b4			| sysTrapLstGetSelectionText

		MOVE.L	%A0,-(%A7)
		MOVE.W	#2005,%D0		| Typ-Popup
		BSR	GET_OBJ_ADDR
		MOVE.L	%A0,-(%A7)
		TRAP	#15
		DC.W	0xa114			| sysTrapCtlSetLabel

		# Batterie-Info eintragen
		CLR.L	-8(%A6)			| Default kein Shade/Prozent
		PEA	-8(%A6)
		CLR.W	-(%A7)			| ID=0 
		MOVE.L	#THBM,-(%A7)		
		TRAP	#15
		DC.W	0xa27b			| sysTrapFtrGet	
	
		MOVE.W	-6(%A6),%D5		| Batterie-Info (Proz/Volt) 
		ASR.W	#1,%D5			| Bit 1 = info type
		AND.W	#1,%D5
		
		MOVE.W	%D5,-(%A7)
		MOVE.L	%D4,-(%A7)		| Batterie-Info-Ptr
		TRAP	#15
		DC.W	0xa1b7			| sysTrapLstSetSelection
		ADDA.W	#30,%A7

		MOVE.W	%D5,-(%A7)		| Batterie-Info
		MOVE.L	%D4,-(%A7)		| Batterie-Info-Ptr
		TRAP	#15
		DC.W	0xa1b4			| sysTrapLstGetSelectionText

		MOVE.L	%A0,-(%A7)		| Info-String merken
	
		MOVE.W	#2007,%D0		| Batterie-Info-Popup
		BSR	GET_OBJ_ADDR

		MOVE.L	%A0,-(%A7)		| Object-Ptr des Popup
		TRAP	#15
		DC.W	0xa114			| sysTrapCtlSetLabel

		# die Shade-Checkbox

		MOVE.W	-6(%A6),%D0		| Batterie-Info (Proz/Volt) 
		ANDI.W	#1,%D0			| lower bit is shade indicator
		MOVE.W	%D0,-(%A7)

		MOVE.W	#2003,%D0		| ID der Checkbox
		BSR	GET_OBJ_ADDR

		MOVE.L	%A0,-(%A7)		| ein/ausschalten
		TRAP	#15
		DC.W	0xa112			| sysTrapCtlSetValue

		BRA	EVT_EXIT

EVT_CTL_SELECT:	CMPI.W	#3001,8(%A2)		| OK-Button
		BNE.B	EVT_EXIT
	
		MOVE.L	%D3,-(%A7)		| Typ-Liste
		TRAP	#15
		DC.W	0xa1b3			| sysTrapLstGetSelection

		MOVE.B	%D0,-1(%A6)
		CLR.L	-(%A7)
		CLR.L	-(%A7)
		PEA	-1(%A6)			| Batterie-Typ
		CLR.L	-(%A7)
		CLR.L	-(%A7)
		CLR.L	-(%A7)
		MOVE.B	#1,-(%A7)		| setzen!
		TRAP	#15
		DC.W	0xa324			| sysTrapSysBatteryInfo

		MOVE.W	#2003,%D0		| Checkbox
		BSR	GET_OBJ_ADDR

		MOVE.L	%A0,-(%A7)
		TRAP	#15
		DC.W	0xa111			| sysTrapCtlGetValue
		MOVE.B	%D0,-5(%A6)
		ADDA.W	#34,%A7

		MOVE.L	%D4,-(%A7)		| Batterie-Info-Liste
		TRAP	#15
		DC.W	0xa1b3			| sysTrapLstGetSelection

		ASL.B	#1,%D0
		OR.B	%D0,-5(%A6)

		MOVE.L	-8(%A6),-(%A7)
		CLR.W	-(%A7)			| ID=0 
		MOVE.L	#THBM,-(%A7)		
		TRAP	#15
		DC.W	0xa27c			| sysTrapFtrSet

EVT_EXIT:	CLR.W	%D0			| Event handled
		MOVEM.L	-28(%A6),%D3-%D5/%A2
		UNLK	%A6
		RTS

GET_OBJ_ADDR:	MOVE.W	%D0,-(%A7)
		TRAP	#15
		DC.W	0xa173			| sysTrapFrmGetActiveForm
		MOVE.L	%A0,-(%A7)
		TRAP	#15
		DC.W	0xa180			| sysTrapFrmGetObjectIndex
		MOVE.W	%D0,-(%A7)
		TRAP	#15
		DC.W	0xa173			| sysTrapFrmGetActiveForm
		MOVE.L	%A0,-(%A7)
		TRAP	#15
		DC.W	0xa183			| sysTrapFrmGetObjectPtr
		ADDA.W	#12,%A7
		RTS


