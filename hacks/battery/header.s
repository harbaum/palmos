		# Battery Monitor Hack V1.0
		# (c) 1998 Till Harbaum
	
		.set THBM, 0x5448424d
		.set ALTX0, 52
	.if(OS33)
		.set ALTX1, 83
		.set ALTX2, 85	
	.else
		.set ALTX1, 82
		.set ALTX2, 84	
	.endif
		.set NEUX0, (ALTX0)
		.set NEUX1, 70
		.set NEUX2, (NEUX1+2)
		.set TEXTX, (NEUX2+3)
		.set OFFX,  12		| der ganze Kram nach links 
