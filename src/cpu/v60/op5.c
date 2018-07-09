
/*
 *  HALT: must add log
 */

UINT32 opBRK(void)
{
/*
	UPDATEPSW();

	SP -=4;
	MemWrite32(SP, 0x0d00);
	SP -=4;
	MemWrite32(SP, PSW);
	SP -=4;
	MemWrite32(SP, PC + 1);
	PC = GETINTVECT(13);
	ChangePC(PC);
*/
//	MessageBox(NULL,"BRK opcode: skipping","Warning",MB_OK);
	messagebox("Skipping BRK opcode!");

	return 1;
}

UINT32 opBRKV(void)
{
	UPDATEPSW();

	SP -=4;
	MemWrite32(SP, PC);
	SP -=4;
	MemWrite32(SP, 0x0d00);
	SP -=4;
	MemWrite32(SP, PSW);
	SP -=4;
	MemWrite32(SP, PC + 1);
	PC = GETINTVECT(21);
	ChangePC(PC);

	return 0;
}

UINT32 opCLRTLBA(void)
{
	// @@@ TLB not yet supported
	messagebox("Skipping CLRTLBA opcode!\n");
	return 1;
}

UINT32 opDISPOSE(void)
{
	SP = FP;
	FP = MemRead32(SP);
	SP +=4;

	return 1;
}

UINT32 opHALT(void)
{
	// @@@ It should wait for an interrupt to occur
	//MessageBox(NULL,"HALT found: skipping","Warning",MB_OK);
	return 1;
}

UINT32 opNOP(void) /* TRUSTED */
{
	return 1;
}

UINT32 opRSR(void)
{
	PC = MemRead32(SP);
	SP +=4;
	ChangePC(PC);

	return 0;
}

UINT32 opTRAPFL(void)
{
	UPDATEPSW();

	if ((TKCW & 0x1F0) & ((PSW & 0x1F00) >> 4))
	{
		// @@@ FPU exception
		messagebox("Hit TRAPFL!\n");
	}

	return 1;
}







