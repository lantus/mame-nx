/**
 * driver\namcos22.c
 *
 * Hardware Notes:
 * - Serial controller is C139 SCI (same as System21).
 * - S22 has two TI320C25 DSP (printed as C71).
 * - Main DSP performs handling display list and matrix operation, and kicks hardware.
 * - Sub DSP may calculates shadings and sub operations.
 * - main CPU copies some DSP code to shared RAM on start
 * - C74 is sound MCU, Mitsubishi M37702 MCU with mask ROM
 * - some external subroutines for C74 is also embedded
 * - Super System22 has a different memory map, sound system, and an additional 2d sprite layer.
 *
 * Prop Cycle notes:
 * - motor output: fan, blows air towards player when flying fast
 * - lamp output: start button can light up
 * - steering yoke should be analog
 * - could use a new analog input port type for pedal speed
 * - needs nvmem save
 *
 *	The "dipswitch" settings are ignored - this isn't a bug.  The Prop Cycle software
 *  explicitly clears the chunk of work RAM used to cache the 8 bit dipswitch value
 *  immediately after populating it.  This is done to hide secret debug routines from
 *	release versions of the game.
 *
 *	When the red namco logos slides in from top right and bottom left during splash screen,
 *	it is supposed to be translucent with respect to the polygon layer
 *
 * TBA:
 *	- fix Alpine Racer (Super System22) - encrypted point ROMs
 *	- fix Ridge Racer1/2 (System22) - interrupts/protection need work
 *	- fix Rave Racer (System22) lots of glitches
 *	- fix Victory Lap (System22) - lots of glitches
 *
 * Special thanks to "team vivanonno" for System22 memory map, and insights on object list
 * parsing.
 */

/*
Alpine Racer (VER. D)
Namco, 1995

  Player stands on steps, steering left/right.
  Inner edge allows quick movement.

  The "viewpoint" button is used to select game mode
  and to change camera placement during gameplay.

1ST PCB
-------
PCB NO: SYSTEM SUPER22 CPU PCB 8646960102 (8646970102)
CPU   : 68EC020FG25
OSCs  : 40.000MHz, 49.1520MHz
RAMs  : IS61C256AH-20J (x2), CY7C182-25VC (x1)
DIPSW : 8 Position (x1), 4 Position (x1)
PALs  : PALCE 22V10H-15JC/4 (x3, labelled SS22C1, SS22C2, SS22C4)
CUSTOM: C352 (100 PIN PQFP)
        C405 (176 PIN PQFP)
        C391 (KEYCUS)
        137
        139  (64 PIN PQFP)
        C383 (100 PIN PQFP)
        M37710S4BFP (80 PIN PQFP)
OTHER : AT28C64 (EEPROM)
ROMs  : AR1DATAB.8K (near C405, 27C4096)
        AR1WAVEA.2L (near C352, 16M MASK)

        Tiny ROM PCB labelled AR2 VER. D
        SYSTEM SUPER22 MPM(F) PCB 8646961600 (8646971600)
        containing main program ....
                                     ROM1.BIN (INTEL FLASH, TYPE E28F008SA, TSOP40)
                                     ROM2.BIN         "
                                     ROM3.BIN         "
                                     ROM4.BIN         "



2ND PCB
-------
PCB NO: SYSTEM SUPER22 DSP PCB 8646960302 (8646970302)
OSC   : 40.0000MHz
PALs  : PALCE20V8H (Labelled SS22D1)
        PALCE16V8  (x4, Labelled SS22D2, SS22D3, SS22D4, SS22D5)
RAMs  : CY7C199-25VC (x9), M5M51008AFP-70L (x6)
CUSTOM: C396 (160 PIN PQFP)
        C71  (x2, 68 PIN PLCC)
        C199 (100 PIN PQFP)
        C353 (120 PIN PQFP)
        C402 (x2, 144 PIN PQFP)
        C403 (136 PIN PQFP)
        C300 (160 PIN PQFP)
        C342 (160 PIN PQFP)
        C405 (x2, 176 PIN PQFP)
        C379 (64 PIN PQFP)



3RD PCB
-------

PCB No: SYSTEM SUPER22 MROM PCB 8646960400 (8646970400)
PALs  : Labelled SS22M3, SS22M2, SS22M2 & SS22M1
RAMs  : TC551001BFL-70L (x3)
ROMs  : GFX....MANY, MANY, MANY! All ROMs are surface mounted

                                  Type
        ----------------------------------
        ar1ccrl.7b = ar1ccrl.3d   16M SOP44
        ar1ccrh.5b = ar1ccrh.1d   4M SOP32

        ar1cg1.13b = ar1cg1.10d   16M SOP44
        ar1cg2.14b = ar1cg2.12d      "
        ar1cg3.16b = ar1cg3.13d      "
        ar1cg4.18b = ar1cg4.14d      "
        ar1cg5.19b = ar1cg5.16d      "
        ar1cg6.18a = ar1cg6.18d      "
        ar1cg7.15a = ar1cg7.19d      "

        ar1scg0.12f= ar1scg0.12l  16M SOP44 (These have identical halves but)
        ar1scg1.10f= ar1scg1.10l     "      (     they *are* 16M ROMs       )

        ar1ptr*                   4M SOP32



4TH PCB
-------
PCB NO: SYSTEM SUPER22 VIDEO 8646960204 (8646970204)
OSC   : 51.2000MHz (near PLCC84)
RAMs  : KM424C257 (x22), IS61C256 (x7), LC321664 (x1)
CUSTOM: 304 (x4, 120 PIN PQFP)
        361 (120 PIN PQFP)
        374 (160 PIN PQFP)
        381 (x2, 144 PIN PQFP)
        395 (? - forgot to note leg count!)
        397 (160 PIN PQFP)
        399 (160 PIN PQFP)
        400 (x3, 100 PIN PQFP)
        401 (x5, 64 PIN PQFP)
        404 (208 PIN PQFP)
        406 (120 PIN PQFP)
        SONY CXD1178Q (48 PIN PQFP)
        ALTERA EPM7064LC84-15 (84 PIN PLCC)

	Notes on input ports:

	0xe00098: dsw
	0xe0009c: up/down

  ----------------------------
	e000a8.l	IO TR

	e000b6.l VR 0
	e000ba.l VR 1
	e000be.l VR 2
	e000c2.l VR 3

	e000a4: ?

	e000a6.w // IO SW
		bit 2: SERVICE SW
		bit 3: TEST SW
		bit 4: MID SW
		bit 5: LEFT SW
		bit 6: RIGHT SW

	e000b2.l EDGE VR
	e000ae.l SWING VR
*/

#include "namcos22.h"

enum namcos22_gametype namcos22_gametype;

static data32_t *namcos22_shareram;

static struct GfxLayout sprite_layout =
{
	32,32,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{
		0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8,
		8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8,
		16*8,17*8,18*8,19*8,20*8,21*8,22*8,23*8,
		24*8,25*8,26*8,27*8,28*8,29*8,30*8,31*8 },
	{
		0*32*8,1*32*8,2*32*8,3*32*8,4*32*8,5*32*8,6*32*8,7*32*8,
		8*32*8,9*32*8,10*32*8,11*32*8,12*32*8,13*32*8,14*32*8,15*32*8,
		16*32*8,17*32*8,18*32*8,19*32*8,20*32*8,21*32*8,22*32*8,23*32*8,
		24*32*8,25*32*8,26*32*8,27*32*8,28*32*8,29*32*8,30*32*8,31*32*8 },
	32*32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &sprite_layout,  0, 0x80 },
	{ -1 },
};

static READ32_HANDLER( alpiner_prot_r )
{
	if( namcos22_gametype == NAMCOS22_CYBER_CYCLES )
	{
		if( offset==0x1e/4 )
		{
			return 0x387;
		}
	}
	return 0x0187; /* Alpine Racer */
}


/**
 * Some port values are read serially one bit at a time via word reads at
 * 0x50000008 and 0x5000000a
 *
 * Writes to 0x50000008 and 0x5000000a reset the state of the input buffer.
 *
 * Note that only the values read at 0x50000008 seem to be used in-game.
 *
 * Some of these values are redundant with respects to the work-RAM supplied input port
 * values supplied by the IO CPU.  For example, the position of the stick shift is digital,
 * and may be read through this mechanism or through shared IO RAM at 0x60004030.
 *
 * Other values seem to be digital versions of analog ports, for example "the gas pedal is
 * pressed" as a boolean flag.  IO RAM supplies it as an analog value.
 */
static data32_t mSys22PortBits;

static READ32_HANDLER( namcos22_portbit_r )
{
	unsigned data;
	data = mSys22PortBits;
	data &= (1<<16)|1;
	mSys22PortBits>>=1;
	return data;
}
static WRITE32_HANDLER( namcos22_portbit_w )
{
	unsigned dat50000008;
	unsigned dat5000000a;
	unsigned inp;

	dat50000008 = 0;
	dat5000000a = 0;

	inp = readinputport(4);
	if( inp&0x1 ) dat50000008 |= 0x80; /* stick shift */
	if( inp&0x2 ) dat50000008 |= 0x40; /* stick shift */

	inp = readinputport(1);
	if( inp!=0x8000 ) dat50000008 |= 0x01; /* gas pedal pressed */
	inp = readinputport(2);
	if( inp!=0x8000 ) dat50000008 |= 0x02; /* brake -> "exit" */

	mSys22PortBits = (dat50000008<<16)|dat5000000a;
}

static READ32_HANDLER( namcos22_dipswitch_r )
{
	return readinputport(0)<<16;
}

static READ32_HANDLER( namcos22_mcuram_r )
{
	return namcos22_shareram[offset];
}

static WRITE32_HANDLER( polyram_w)
{
	COMBINE_DATA( &namcos22_polygonram[offset] );
}

static MEMORY_READ32_START( namcos22s_readmem )
	{ 0x000000, 0x3fffff, MRA32_ROM },
	{ 0x400000, 0x40001f, alpiner_prot_r },
	{ 0x410000, 0x413fff, MRA32_RAM }, /* unused by Prop Cycle? */
	{ 0x430000, 0x430003, MRA32_RAM },
	{ 0x440000, 0x440003, namcos22_dipswitch_r },
	{ 0x450008, 0x45000b, namcos22_portbit_r },

	{ 0x460000, 0x463fff, MRA32_RAM }, /* unknown */
		/* possible nvmem?
		 */

	{ 0x700000, 0x70001f, MRA32_RAM },
		/* 0x700014r: watchdog */

	{ 0x800000, 0x800003, MRA32_RAM }, /* 0x08380000 (?) */

	{ 0x810000, 0x81000f, MRA32_RAM }, /* always zero? */
		/* 00000000 00000000 44440000 00000000 */

	{ 0x810200, 0x8103ff, MRA32_RAM }, /* depth cueing; fog density, near to far */
		/* 0000 0015 002a 003f 0054 0069 ...14eb */

	{ 0x824000, 0x8243ff, namcos22_gamma_r }, /* GAMMA */
	{ 0x828000, 0x83ffff, namcos22_paletteram_r }, /* PALETTE */
	{ 0x860000, 0x860007, MRA32_RAM }, /* ? */

	{ 0x880000, 0x89dfff, namcos22_cgram_r },
	{ 0x89e000, 0x89ffff, namcos22_textram_r },
	{ 0x8a0000, 0x8a000f, MRA32_RAM },
		/*	+0x0000			BG Position X
		 *	+0x0002			BG Position Y
		 */

	{ 0x900000, 0x90ffff, MRA32_RAM },
	{ 0x940000, 0x94007f, MRA32_RAM },
	{ 0x980000, 0x9affff, MRA32_RAM }, /* spriteram */
	{ 0xa03ffc, 0xa03fff, MRA32_RAM },
	{ 0xa04000, 0xa0bfff, namcos22_mcuram_r },
	{ 0xc00000, 0xc1ffff, MRA32_RAM }, /* polygon RAM */
	{ 0xe00000, 0xefffff, MRA32_RAM }, /* workram */
MEMORY_END

static MEMORY_WRITE32_START( namcos22s_writemem )
	{ 0x000000, 0x3fffff, MWA32_ROM },
	{ 0x400000, 0x40001f, MWA32_NOP }, /* cuskey? */
	{ 0x410000, 0x413fff, MWA32_RAM },
	{ 0x430000, 0x430003, MWA32_RAM }, /* I/O related; bit select? */
	{ 0x450008, 0x45000b, namcos22_portbit_w },
	{ 0x460000, 0x463fff, MWA32_RAM },
	{ 0x700000, 0x70001f, MWA32_RAM }, /* CPU control registers */
	{ 0x800000, 0x800003, MWA32_RAM }, /* ? */
	{ 0x810000, 0x81000f, MWA32_RAM }, /* ? */
	{ 0x810200, 0x8103ff, MWA32_RAM }, /* depth cueing density */
	{ 0x824000, 0x8243ff, namcos22_gamma_w, &namcos22_gamma },
	{ 0x828000, 0x83ffff, namcos22_paletteram_w, &paletteram32 },
	{ 0x860000, 0x860007, MWA32_RAM }, /* ? */
	{ 0x880000, 0x89dfff, namcos22_cgram_w, &namcos22_cgram },
	{ 0x89e000, 0x89ffff, namcos22_textram_w, &namcos22_textram },
	{ 0x8a0000, 0x8a000f, MWA32_RAM }, /* ? */
		/* 035c0000 006e0000 01ff0000 0000000 */
	{ 0x900000, 0x90ffff, MWA32_RAM }, /* VICS(?) */
	{ 0x940000, 0x94007f, MWA32_RAM },
	{ 0x980000, 0x9affff, MWA32_RAM, &spriteram32 },
	{ 0xa03ffc, 0xa03fff, MWA32_RAM },
	{ 0xa04000, 0xa0bfff, MWA32_RAM, &namcos22_shareram }, /* MCU */
		/* 0xa0bd2f: 0x02 Prop Cycle: MOTOR ON */
		/* 0xa0bd2f: 0x04 Prop Cycle: LAMP ON */
	{ 0xc00000, 0xc1ffff, polyram_w, &namcos22_polygonram },
	{ 0xe00000, 0xefffff, MWA32_RAM }, /* WORK */
	/* note that at least some of this is battery backed ADS RAM */
MEMORY_END

static void
SimulateAlpineRacerMCU( void )
{
	/* ori.b   #$80, $a0bd00.l: signal from main CPU
	 * $a0bd01: bit 7 indicates mcu is busy
	 *
	 * $a0bd02 (byte)
	 * $a0bd04 (byte)
	 *
	 * $a0bd0a: swing
	 * $a0bd0c: edge
	 *
	 * move.w  #$0, $a0bd2e.l
	 *
	 * move.l  #$410000, $e05950.l
	 * move.l  #$414000, $e05954.l
	 *
	 * move.l  #$a04000, $e05950.l
	 * move.l  #$a0c000, $e05954.l
	 */
	if( namcos22_gametype == NAMCOS22_ALPINE_RACER )
	{
		namcos22_shareram[0x0300/4] = 0x7551<<16; /* protection? */
		namcos22_shareram[0x7d00/4] = readinputport(1)<<8;
		namcos22_shareram[(0xa0bd0a-0xa04000)/4] = (data16_t)(readinputport(2)-0x8000); /* swing */
		namcos22_shareram[(0xa0bd0c-0xa04000)/4] = ((data16_t)(readinputport(3)-0x8000))<<16; /* edge */
	}
}

static void
SimulatePropCycleMCU( void )
{
		int dx,dy;
		UINT16 data;
		static data16_t pedal;

		namcos22_shareram[(0xa0bd00-0xa04000)/4] = readinputport(1)<<8;

		data = 0;
		if( readinputport( 2 ) & 0x20 ) data |= 0x0100;
		namcos22_shareram[(0xa0bd04-0xa04000)/4] = data<<16; // start1

		dx = 0; dy = 0;
		if( readinputport( 2 ) & 0x04 ) dx++;
		if( readinputport( 2 ) & 0x08 ) dx--;
		if( readinputport( 2 ) & 0x01 ) dy--;
		if( readinputport( 2 ) & 0x02 ) dy++;

		if( readinputport( 2 ) & 0x10 ) pedal+=0x10;

		namcos22_shareram[(0xa0bd08-0xa04000)/4] = (UINT16)(dx*0x7fff);
		namcos22_shareram[(0xa0bd0c-0xa04000)/4] = (dy*0x7fff)<<16;
		namcos22_shareram[(0xa0bd1c-0xa04000)/4] = pedal<<16;
		/*	pc=00022ffa; offs=00a0be80
			pc=00022258; offs=00a0bd00
			pc=00022272; offs=00a0bd04
			pc=000222c6; offs=00a0bd08
			pc=000222c6; offs=00a0bd0c
			pc=000222c6; offs=00a0bd0c
			pc=000222c6; offs=00a0bd10
			pc=000222c6; offs=00a0bd10
			pc=000222c6; offs=00a0bd14
			pc=000222c6; offs=00a0bd14
			pc=000222c6; offs=00a0bd18
			pc=000223da; offs=00a0bd18
			pc=000223da; offs=00a0bd1c
		*/
}

static void
SimulateCyberCyclesMCU( void )
{
	data32_t data;
return;
	data = 0;
	if( keyboard_pressed(KEYCODE_SPACE) ) data |= 0x01000; // "view switch"
	namcos22_shareram[(0xa0bd00 - 0xa04000)/4] = data;

	data = 0; /* ? */
	namcos22_shareram[(0xa0bd2e - 0xa04000)/4] = data;
}

static INTERRUPT_GEN( namcos22s_interrupt )
{
	switch( namcos22_gametype )
	{
	case NAMCOS22_ALPINE_RACER:
		SimulateAlpineRacerMCU();
		break;

	case NAMCOS22_PROP_CYCLE:
		SimulatePropCycleMCU();
		break;

	case NAMCOS22_CYBER_CYCLES:
		SimulateCyberCyclesMCU();
		break;

	default:
		break;
	}

	switch( cpu_getiloops() )
	{
	case 0:
		cpu_set_irq_line(0, 4, HOLD_LINE); /* vblank */
		break;
	case 1:
		if( namcos22_gametype == NAMCOS22_CYBER_CYCLES )
		{
			cpu_set_irq_line(0, 2, HOLD_LINE); /* vblank */
		}
		else if( namcos22_gametype == NAMCOS22_AIR_COMBAT22 )
		{
			cpu_set_irq_line(0, 6, HOLD_LINE);
		}
		break;
	}
}

static MACHINE_DRIVER_START( namcos22s )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68EC020,25000000) /* 25 MHz? */
	MDRV_CPU_MEMORY(namcos22s_readmem,namcos22s_writemem)
	MDRV_CPU_VBLANK_INT(namcos22s_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(NAMCOS22_NUM_COLS*16,NAMCOS22_NUM_ROWS*16)
	MDRV_VISIBLE_AREA(0,NAMCOS22_NUM_COLS*16-1,0,NAMCOS22_NUM_ROWS*16-1)
	MDRV_PALETTE_LENGTH(NAMCOS22_PALETTE_SIZE)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(namcos22s)
	MDRV_VIDEO_UPDATE(namcos22s)
MACHINE_DRIVER_END

/*********************************************************************************/

/*
cpu #0 (PC=00027AC0): unmapped memory dword read from 20010020 & 0000FFFF
cpu #0 (PC=00027AC6): unmapped memory dword read from 20010024 & 00FF0000
cpu #0 (PC=00027E76): unmapped memory dword read from 20010040 & FFFF0000
cpu #0 (PC=00027E7C): unmapped memory dword read from 20010040 & 000000FF
cpu #0 (PC=00027EF8): unmapped memory dword write to 20020000 = 00000000 & 0000FFFF

000293ac: read(60004038)	move.b  ($403a,A5), D0
00001c16: read(60004030)	move.w  ($4030,A5), D0	(SWITCH)
00001c2c: read(60004030)	move.w  ($4032,A5), D0	(STEERING)
00001c40: read(60004034)	move.w  ($4034,A5), D0	(GAS)
00001c54: read(60004034)	move.w  ($4036,A5), D0	(BRAKE)
00001c68: read(60004038)	move.w  ($4038,A5), D0; move.w  D0, (-$7f3a,A6)
00001c70: read(6000403c)	move.w  ($403c,A5), D0; move.w  D0, (-$7f38,A6)
00001c78: read(6000403c)	move.w  ($403e,A5), D0; move.w  D0, (-$7f36,A6)
00001c80: read(60004040)	move.w  ($4040,A5), D0; move.w  D0, (-$7f34,A6)
00001c88: read(60004040)	move.w  ($4042,A5), D0; move.w  D0, (-$7f32,A6)
*/

static READ32_HANDLER( rr_prot_r )
{
	if( offset==0 )
	{
		return 0x0172<<16; /* rrs */
	}
	else
	{
		return 0x0188<<16;
	}
}
static READ32_HANDLER( rr_prot2_r )
{
	unsigned data;
	data = 0;
	data |= 0x04; /* victlap */
	return data<<16;
}

static READ32_HANDLER( sys22_shareram_r )
{
	return namcos22_shareram[offset];
}

static MEMORY_READ32_START( namcos22_readmem )
	{ 0x00000000, 0x001fffff, MRA32_ROM },
	{ 0x10000000, 0x1001ffff, MRA32_RAM },	/* work RAM */
	{ 0x20000000, 0x2000000f, rr_prot_r },	/* custom key? */
	{ 0x20010000, 0x20013fff, MRA32_RAM },  /* C139 SCI: serial controller (TX/RX buffer) - same as System21 */
	{ 0x20020000, 0x20020003, rr_prot2_r },	/* serial controller (registers) */
	{ 0x40000000, 0x4000001f, MRA32_RAM },	/* interrupt controller */
	/*	0x40000016		watchdog */
	{ 0x48000000, 0x4800003f, MRA32_RAM },	/* unknown */
	{ 0x50000000, 0x50000003, namcos22_dipswitch_r }, /* DSW2,3 */
	{ 0x50000008, 0x5000000b, namcos22_portbit_r },
	{ 0x58000000, 0x58001fff, MRA32_RAM },	/* EPROM */
	{ 0x60004000, 0x6000bfff, sys22_shareram_r },
	{ 0x70000000, 0x7001ffff, MRA32_RAM },
	{ 0x90010000, 0x90017fff, MRA32_RAM },	/* depth-cueing look-up table */
	{ 0x90020000, 0x90027fff, MRA32_RAM },	/* polygon control registers */
	{ 0x90028000, 0x9003ffff, MRA32_RAM },	/* Palette */
	{ 0x90040000, 0x9007ffff, MRA32_RAM },
	{ 0x90080000, 0x9009dfff, MRA32_RAM },	/* bg tiles */
	{ 0x9009e000, 0x9009ffff, MRA32_RAM },	/* bg tilemap */
	{ 0x900a0000, 0x900a000f, MRA32_RAM },	/* bg control register */
MEMORY_END

static MEMORY_WRITE32_START( namcos22_writemem )
	{ 0x00000000, 0x001fffff, MWA32_ROM },
	{ 0x10000000, 0x1001ffff, MWA32_RAM },
	{ 0x20010000, 0x20013fff, MWA32_RAM },  /* C139 SCI: serial controller  TX/RX buffer (same as System21) */
	{ 0x20020000, 0x20020003, MWA32_NOP },	/* serial controller (registers) */
	{ 0x40000000, 0x4000001f, MWA32_RAM },	/* interrupt controller */
	{ 0x48000000, 0x4800003f, MWA32_RAM },	/* unknown */
	{ 0x50000008, 0x5000000b, namcos22_portbit_w },
	{ 0x58000000, 0x58001fff, MWA32_RAM },	/* EPROM */
	{ 0x60004000, 0x6000bfff, MWA32_RAM, &namcos22_shareram },
	/*	0x60004022.w		Volume(R)
	 *	0x60004024.w		Volume(L)
	 *	0x60004026.w		Volume(R) (maybe rear channels, not put on real PCB)
	 *	0x60004028.w		Volume(L) (maybe rear channels, not put on real PCB)
	 *	0x60004030 b0     : system type 0
	 *	0x60004030 b1 = 0 : COIN2
	 *	0x60004030 b2 = 0 : TEST SW
	 *	0x60004030 b3 = 0 : SERVICE SW
	 *	0x60004030 b4 = 0 : COIN1
	 *	0x60004030 b5     : system type 1
	 *				(system type on RR2 (00:50inch, 01:two in one, 20: standard, 21: deluxe))
	 *	0x60004031 b0 = 0 : SWITCH1 (for manual transmission)
	 *	0x60004031 b1 = 0 : SWITCH2
	 *	0x60004031 b2 = 0 : SWITCH3
	 *	0x60004031 b3 = 0 : SWITCH4
	 *	0x60004031 b4 = 0 : CLUTCH
	 *	0x60004031 b6 = 0 : VIEW SW
	 *	0x60004032.w		Handle A/D (=steering wheel, default of center value is different in each game)
	 *	0x60004034.w		Gas A/D
	 *	0x60004036.w		Brake A/D
	 *	0x60004038.w		A/D3 (reserved)
	 *				(some GOUT (general outputs for lamps, etc) is also mapped this area)
	 *	0x60005000 - 0x503f	Song Request #00 to 31
	 *	0x60005100 - 0x52ff	Parameter RAM from Main MPU (for SEs)
	 *	0x60005300 - 0x53ff?	Song Title (put messages here from Sound CPU)
	 */
	{ 0x70000000, 0x7001ffff, polyram_w, &namcos22_polygonram },
	/*	0x70000040.l		Point RAM Write Enable (ff=enable)
	 *	0x70000044.l		Point RAM IDC (index count register)
	 *	0x70000c00 - 0x7000ffff	Point RAM Port
	 *				(Point RAM has 128K words, 512KB space (128K * 24bit))
	 *	0x70010000 - 0x700103ff Window Attribute Register #1 (0x80 * 8)
	 *	0x70010400 - 0x70017fff Display List Buffer #1
	 *	0x70018000 - 0x700183ff Window Attribute Register #2 (0x80 * 8)
	 *	0x70018400 - 0x7001ffff Display List Buffer #2
	 */
	{ 0x90000000, 0x90000003, MWA32_NOP },	/* LED on PCB(?) */
	{ 0x90010000, 0x90017fff, MWA32_RAM },	/* Depth-cueing Look-up Table
											 * (fog density between near to far) */
	{ 0x90020000, 0x90027fff, MWA32_RAM },	/* polygon control registers */
	/*	+0x0011.w		Display Fader (R) (0x0100 = 1.0)
	 *	+0x0013.w		Display Fader (G) (0x0100 = 1.0)
	 *	+0x0015.w		Display Fader (B) (0x0100 = 1.0)
	 *	+0x0100.b		Fog1 Color (R) (world fogging)
	 *	+0x0101.b		Fog2 Color (R) (used for heating of brake-disc on RV1)
	 *	+0x0180.b		Fog1 Color (G)
	 *	+0x0181.b		Fog2 Color (G)
	 *	+0x0200.b		Fog1 Color (B)
	 *	+0x0201.b		Fog2 Color (B)
	 */
	{ 0x90028000, 0x9003ffff, namcos22_paletteram_w, &paletteram32 },
	{ 0x90040000, 0x9007ffff, MWA32_RAM },	/* victory lap */
	{ 0x90080000, 0x9009dfff, namcos22_cgram_w, &namcos22_cgram },
	{ 0x9009e000, 0x9009ffff, namcos22_textram_w, &namcos22_textram },
	{ 0x900a0000, 0x900a000f, MWA32_RAM },	/* bg control register */
	/*	+0x0000			BG Position X
	 *	+0x0002			BG Position Y
	 */
MEMORY_END

static INTERRUPT_GEN( namcos22_interrupt )
{
	switch( namcos22_gametype )
	{
	case NAMCOS22_VICTORY_LAP:
		namcos22_shareram[0x00/4] = 0x10<<16; /* SUB CPU ready */
		namcos22_shareram[0x30/4] = (readinputport(4)<<16)|readinputport(3);
		namcos22_shareram[0x34/4] = (readinputport(1)<<16)|readinputport(2);
		break;
	default:
		break;
	}

	switch( cpu_getiloops() )
	{
	case 0: // hblank?
		cpu_set_irq_line(0, 1, HOLD_LINE);
		break;
	case 1: // vblank?
		cpu_set_irq_line(0, 5, HOLD_LINE);
		break;
	case 2:
		cpu_set_irq_line(0, 6, HOLD_LINE);
		break;
	}
}

static MACHINE_DRIVER_START( namcos22 )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68020,25000000) /* 25 MHz? */
	MDRV_CPU_MEMORY(namcos22_readmem,namcos22_writemem)
	MDRV_CPU_VBLANK_INT(namcos22_interrupt,3)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(NAMCOS22_NUM_COLS*16,NAMCOS22_NUM_ROWS*16)
	MDRV_VISIBLE_AREA(0,NAMCOS22_NUM_COLS*16-1,0,NAMCOS22_NUM_ROWS*16-1)
	MDRV_PALETTE_LENGTH(NAMCOS22_PALETTE_SIZE)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(namcos22s)
	MDRV_VIDEO_UPDATE(namcos22)
MACHINE_DRIVER_END

/*********************************************************************************/

ROM_START( airco22b )
	ROM_REGION( 0x400000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_BYTE( "acs1verb.1", 0x00003, 0x100000, CRC(062c4f61) )
	ROM_LOAD32_BYTE( "acs1verb.2", 0x00002, 0x100000, CRC(8ae69711) )
	ROM_LOAD32_BYTE( "acs1verb.3", 0x00001, 0x100000, CRC(71738e67) )
	ROM_LOAD32_BYTE( "acs1verb.4", 0x00000, 0x100000, CRC(3b193add) )

	ROM_REGION( 0x200000*2, REGION_GFX1, ROMREGION_DISPOSE ) /* 32x32x8bpp sprite tiles */
	ROM_LOAD( "acs1scg0.12l", 0x200000*0, 0x200000,CRC(e5235404) )
	ROM_LOAD( "acs1scg1.10l", 0x200000*1, 0x200000,CRC(828e91e7) )

	ROM_REGION( 0x200000*8, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "acs1cg0.8d",  0x200000*0x0, 0x200000,CRC(1f31343e) )
	ROM_LOAD( "acs1cg1.10d", 0x200000*0x1, 0x200000,CRC(ccd5481d) )
	ROM_LOAD( "acs1cg2.12d", 0x200000*0x2, 0x200000,CRC(14e5d0d2) )
	ROM_LOAD( "acs1cg3.13d", 0x200000*0x3, 0x200000,CRC(1a7bcc16) )
	ROM_LOAD( "acs1cg4.14d", 0x200000*0x4, 0x200000,CRC(1920b7fb) )
	ROM_LOAD( "acs1cg5.16d", 0x200000*0x5, 0x200000,CRC(3dd109b7) )
	ROM_LOAD( "acs1cg6.18d", 0x200000*0x6, 0x200000,CRC(ec71c8a3) )
	ROM_LOAD( "acs1cg7.19d", 0x200000*0x7, 0x200000,CRC(82271757) )

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "acs1ccrl.3d",	 0x000000, 0x200000,CRC(07088ba1) )
	ROM_LOAD( "acs1ccrh.1d",	 0x200000, 0x080000,CRC(62936af6) )

	ROM_REGION( 0x80000*12, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "acs1ptl0.18k", 0x80000*0x0, 0x80000,CRC(bd5896c7) )
	ROM_LOAD( "acs1ptl1.16k", 0x80000*0x1, 0x80000,CRC(e583b975) )
	ROM_LOAD( "acs1ptl2.15k", 0x80000*0x2, 0x80000,CRC(802d737a) )
	ROM_LOAD( "acs1ptl3.14k", 0x80000*0x3, 0x80000,CRC(fe556ecb) )

	ROM_LOAD( "acs1ptm0.18j", 0x80000*0x4, 0x80000,CRC(949b6c58) )
	ROM_LOAD( "acs1ptm1.16j", 0x80000*0x5, 0x80000,CRC(8b2b99d9) )
	ROM_LOAD( "acs1ptm2.15j", 0x80000*0x6, 0x80000,CRC(f1515080) )
	ROM_LOAD( "acs1ptm3.14j", 0x80000*0x7, 0x80000,CRC(e364f4aa) )

	ROM_LOAD( "acs1ptu0.18f", 0x80000*0x8, 0x80000,CRC(746b3084) )
	ROM_LOAD( "acs1ptu1.16f", 0x80000*0x9, 0x80000,CRC(b44f1d3b) )
	ROM_LOAD( "acs1ptu2.15f", 0x80000*0xa, 0x80000,CRC(fdd2d778) )
	ROM_LOAD( "acs1ptu3.14f", 0x80000*0xb, 0x80000,CRC(38b425d4) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* S22-BIOS ver1.30 */
	ROM_LOAD( "acs1data.8k", 0, 0x080000, CRC(33824bc9) )
	ROM_REGION( 0x800000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "acs1wav0.1", 0x000000, 0x400000, CRC(52fb9762) )
	ROM_LOAD( "acs1wav1.2", 0x400000, 0x400000, CRC(b568dca2) )
ROM_END

ROM_START( alpinerc )
	ROM_REGION( 0x400000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_BYTE( "ar2ver-c.1", 0x00003, 0x100000, CRC(61323842) )
	ROM_LOAD32_BYTE( "ar2ver-c.2", 0x00002, 0x100000, CRC(43795b2d) )
	ROM_LOAD32_BYTE( "ar2ver-c.3", 0x00001, 0x100000, CRC(acb3003b) )
	ROM_LOAD32_BYTE( "ar2ver-c.4", 0x00000, 0x100000, CRC(a57b4e60) )

	ROM_REGION( 0x200000*2, REGION_GFX1, ROMREGION_DISPOSE ) /* 32x32x8bpp sprite tiles */
	ROM_LOAD( "ar1scg0.12f", 0x200000*0, 0x200000,CRC(e7be830a) ) /* identical to "ar1scg0.12l" */
	ROM_LOAD( "ar1scg1.10f", 0x200000*1, 0x200000,CRC(8f15a686) ) /* identical to "ar1scg1.10l" */

	ROM_REGION( 0x200000*8, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "ar1cg0.12b",  0x200000*0x0, 0x200000,CRC(93f3a9d9) ) /* identical to "ar1cg0.8d" */
	ROM_LOAD( "ar1cg1.10d",  0x200000*0x1, 0x200000,CRC(39828c8b) ) /* identical to "ar1cg1.13b" */
	ROM_LOAD( "ar1cg2.12d",  0x200000*0x2, 0x200000,CRC(f7b058d1) ) /* identical to "ar1cg2.14b" */
	ROM_LOAD( "ar1cg3.13d",  0x200000*0x3, 0x200000,CRC(c28a3d2a) ) /* identical to "ar1cg3.16b" */
	ROM_LOAD( "ar1cg4.14d",  0x200000*0x4, 0x200000,CRC(abdb161f) ) /* identical to "ar1cg4.18b" */
	ROM_LOAD( "ar1cg5.16d",  0x200000*0x5, 0x200000,CRC(2381cfea) ) /* identical to "ar1cg5.19b" */
	ROM_LOAD( "ar1cg6.18a",  0x200000*0x6, 0x200000,CRC(ca0b6d23) ) /* identical to "ar1cg6.18d" */
	ROM_LOAD( "ar1cg7.15a",	 0x200000*0x7, 0x200000,CRC(ffb9f9f9) ) /* identical to "ar1cg7.19d" */

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "ar1ccrl.3d",	 0x000000, 0x200000,CRC(17387b2c) ) /* identical to "ar1ccrl.7b" */
	ROM_LOAD( "ar1ccrh.1d",	 0x200000, 0x080000,CRC(ee7a4803) ) /* identical to "pr1ccrh.5b" */

	ROM_REGION( 0x80000*12, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "ar1ptrl0.18k", 0x80000*0x0, 0x80000,CRC(82405108) )
	ROM_LOAD( "ar1ptrl1.16k", 0x80000*0x1, 0x80000,CRC(8739b09c) )
	ROM_LOAD( "ar1ptrl2.15k", 0x80000*0x2, 0x80000,CRC(bda693a9) )
	ROM_LOAD( "ar1ptrl3.14k", 0x80000*0x3, 0x80000,CRC(82797405) )

	ROM_LOAD( "ar1ptrm0.18j", 0x80000*0x4, 0x80000,CRC(29d92097) )
	ROM_LOAD( "ar1ptrm1.16j", 0x80000*0x5, 0x80000,CRC(2232f0a5) )
	ROM_LOAD( "ar1ptrm2.15j", 0x80000*0x6, 0x80000,CRC(8ee14e6f) )
	ROM_LOAD( "ar1ptrm3.14j", 0x80000*0x7, 0x80000,CRC(1094a970) )

	ROM_LOAD( "ar1ptru0.18f", 0x80000*0x8, 0x80000,CRC(26d88467) )
	ROM_LOAD( "ar1ptru1.16f", 0x80000*0x9, 0x80000,CRC(c5e2c208) )
	ROM_LOAD( "ar1ptru2.15f", 0x80000*0xa, 0x80000,CRC(1321ec59) )
	ROM_LOAD( "ar1ptru3.14f", 0x80000*0xb, 0x80000,CRC(139d7dc1) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* S22-BIOS ver1.30 */
	ROM_LOAD( "ar1datab.8k", 0, 0x080000, CRC(c26306f8) )
	ROM_REGION( 0x200000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "ar1wavea.2l", 0, 0x200000, CRC(dbf64562) )
ROM_END

ROM_START( alpinerd )
	ROM_REGION( 0x400000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_BYTE( "ar2ver-d.1", 0x00003, 0x100000, CRC(fa3380b9) )
	ROM_LOAD32_BYTE( "ar2ver-d.2", 0x00002, 0x100000, CRC(76141352) )
	ROM_LOAD32_BYTE( "ar2ver-d.3", 0x00001, 0x100000, CRC(9beffe6a) )
	ROM_LOAD32_BYTE( "ar2ver-d.4", 0x00000, 0x100000, CRC(1f3f1134) )

	ROM_REGION( 0x200000*2, REGION_GFX1, ROMREGION_DISPOSE ) /* 32x32x8bpp sprite tiles */
	ROM_LOAD( "ar1scg0.12f", 0x200000*0, 0x200000,CRC(e7be830a) ) /* identical to "ar1scg0.12l" */
	ROM_LOAD( "ar1scg1.10f", 0x200000*1, 0x200000,CRC(8f15a686) ) /* identical to "ar1scg1.10l" */

	ROM_REGION( 0x200000*8, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "ar1cg0.12b",  0x200000*0x0, 0x200000,CRC(93f3a9d9) ) /* identical to "ar1cg0.8d" */
	ROM_LOAD( "ar1cg1.10d",  0x200000*0x1, 0x200000,CRC(39828c8b) ) /* identical to "ar1cg1.13b" */
	ROM_LOAD( "ar1cg2.12d",  0x200000*0x2, 0x200000,CRC(f7b058d1) ) /* identical to "ar1cg2.14b" */
	ROM_LOAD( "ar1cg3.13d",  0x200000*0x3, 0x200000,CRC(c28a3d2a) ) /* identical to "ar1cg3.16b" */
	ROM_LOAD( "ar1cg4.14d",  0x200000*0x4, 0x200000,CRC(abdb161f) ) /* identical to "ar1cg4.18b" */
	ROM_LOAD( "ar1cg5.16d",  0x200000*0x5, 0x200000,CRC(2381cfea) ) /* identical to "ar1cg5.19b" */
	ROM_LOAD( "ar1cg6.18a",  0x200000*0x6, 0x200000,CRC(ca0b6d23) ) /* identical to "ar1cg6.18d" */
	ROM_LOAD( "ar1cg7.15a",	 0x200000*0x7, 0x200000,CRC(ffb9f9f9) ) /* identical to "ar1cg7.19d" */

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "ar1ccrl.3d",	 0x000000, 0x200000,CRC(17387b2c) ) /* identical to "ar1ccrl.7b" */
	ROM_LOAD( "ar1ccrh.1d",	 0x200000, 0x080000,CRC(ee7a4803) ) /* identical to "pr1ccrh.5b" */

	ROM_REGION( 0x80000*12, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "ar1ptrl0.18k", 0x80000*0x0, 0x80000,CRC(82405108) )
	ROM_LOAD( "ar1ptrl1.16k", 0x80000*0x1, 0x80000,CRC(8739b09c) )
	ROM_LOAD( "ar1ptrl2.15k", 0x80000*0x2, 0x80000,CRC(bda693a9) )
	ROM_LOAD( "ar1ptrl3.14k", 0x80000*0x3, 0x80000,CRC(82797405) )

	ROM_LOAD( "ar1ptrm0.18j", 0x80000*0x4, 0x80000,CRC(29d92097) )
	ROM_LOAD( "ar1ptrm1.16j", 0x80000*0x5, 0x80000,CRC(2232f0a5) )
	ROM_LOAD( "ar1ptrm2.15j", 0x80000*0x6, 0x80000,CRC(8ee14e6f) )
	ROM_LOAD( "ar1ptrm3.14j", 0x80000*0x7, 0x80000,CRC(1094a970) )

	ROM_LOAD( "ar1ptru0.18f", 0x80000*0x8, 0x80000,CRC(26d88467) )
	ROM_LOAD( "ar1ptru1.16f", 0x80000*0x9, 0x80000,CRC(c5e2c208) )
	ROM_LOAD( "ar1ptru2.15f", 0x80000*0xa, 0x80000,CRC(1321ec59) )
	ROM_LOAD( "ar1ptru3.14f", 0x80000*0xb, 0x80000,CRC(139d7dc1) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* S22-BIOS ver1.30 */
	ROM_LOAD( "ar1datab.8k", 0, 0x080000, CRC(c26306f8) )
	ROM_REGION( 0x200000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "ar1wavea.2l", 0, 0x200000, CRC(dbf64562) )
ROM_END

ROM_START( cybrcomm )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_BYTE( "cy1prgll.4d", 0x00003, 0x80000, CRC(b3eab156) )
	ROM_LOAD32_BYTE( "cy1prglm.2d", 0x00002, 0x80000, CRC(884a5b0e) )
	ROM_LOAD32_BYTE( "cy1prgum.8d", 0x00001, 0x80000, CRC(c9c4a921) )
	ROM_LOAD32_BYTE( "cy1prguu.6d", 0x00000, 0x80000, CRC(5f22975b) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )

	ROM_REGION( 0x200000*8, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "cyc1cg0.1a", 0x200000*0x0, 0x200000,CRC(e839b9bd) ) /* cyc1cg0.6a */
	ROM_LOAD( "cyc1cg1.2a", 0x200000*0x2, 0x200000,CRC(7d13993f) ) /* cyc1cg1.7a */
	ROM_LOAD( "cyc1cg2.3a", 0x200000*0x4, 0x200000,CRC(7c464566) ) /* cyc1cg2.8a */
	ROM_LOAD( "cyc1cg3.5a", 0x200000*0x6, 0x200000,CRC(2222e16f) ) /* cyc1cg3.9a */

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "cyc1ccrl.1c",	 0x000000, 0x100000,CRC(1a0dc5f0) ) /* cyc1ccrl.8c */
	ROM_LOAD( "cyc1ccrh.2c",	 0x200000, 0x080000,CRC(8c4090b8) ) /* cyc1ccrh.7c */

	ROM_REGION( 0x80000*9, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "cyc1ptl0.5b", 0x80000*0x0, 0x80000,CRC(d91de03d) )
	ROM_LOAD( "cyc1ptl1.4b", 0x80000*0x1, 0x80000,CRC(e5b98021) )
	ROM_LOAD( "cyc1ptl2.3b", 0x80000*0x2, 0x80000,CRC(7ba786c6) )
	ROM_LOAD( "cyc1ptm0.5c", 0x80000*0x3, 0x80000,CRC(d454b5c6) )
	ROM_LOAD( "cyc1ptm1.4c", 0x80000*0x4, 0x80000,CRC(74fdf8cc) )
	ROM_LOAD( "cyc1ptm2.3c", 0x80000*0x5, 0x80000,CRC(b9c99a45) )
	ROM_LOAD( "cyc1ptu0.5d", 0x80000*0x6, 0x80000,CRC(4d40897f) )
	ROM_LOAD( "cyc1ptu1.4d", 0x80000*0x7, 0x80000,CRC(3bdaeeeb) )
	ROM_LOAD( "cyc1ptu2.3d", 0x80000*0x8, 0x80000,CRC(a0e73674) )

	ROM_REGION( 0x2100, REGION_USER1, 0 )
	ROM_LOAD( "cy1eeprm.9e", 0x0000, 0x2000, CRC(4e1d294b) ) /* EPROM */

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* MCU */
	ROM_LOAD( "cy1data.6r", 0, 0x020000, CRC(10d0005b) )
	ROM_REGION( 0x400000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "cy1wav0.10r", 0x000000, 0x100000, CRC(c6f366a2) )
	ROM_LOAD( "cy1wav1.10p", 0x100000, 0x100000, CRC(f30b5e37) )
	ROM_LOAD( "cy1wav2.10n", 0x200000, 0x100000, CRC(b98c1ca6) )
	ROM_LOAD( "cy1wav3.10l", 0x300000, 0x100000, CRC(43dbac19) )
ROM_END

ROM_START( cybrcycc )
	ROM_REGION( 0x400000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_BYTE( "cb2ver-c.1", 0x00003, 0x100000, CRC(a8e07a14) )
	ROM_LOAD32_BYTE( "cb2ver-c.2", 0x00002, 0x100000, CRC(054c504f) )
	ROM_LOAD32_BYTE( "cb2ver-c.3", 0x00001, 0x100000, CRC(47e6306c) )
	ROM_LOAD32_BYTE( "cb2ver-c.4", 0x00000, 0x100000, CRC(398426e4) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE ) /* 32x32x8bpp sprite tiles */
	ROM_LOAD( "cb1scg0.12f", 0x200000*0, 0x200000,CRC(7aaca90d) ) /* identical to "cb1scg0.12l" */

	ROM_REGION( 0x200000*7, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "cb1cg0.12b",  0x200000*0x0, 0x200000,CRC(762a47a0) ) /* identical to "cb1cg0.8d" */
	ROM_LOAD( "cb1cg1.10d",  0x200000*0x1, 0x200000,CRC(df92c3e6) ) /* identical to "cb1cg1.13b" */
	ROM_LOAD( "cb1cg2.12d",  0x200000*0x2, 0x200000,CRC(07bc508e) ) /* identical to "cb1cg2.14b" */
	ROM_LOAD( "cb1cg3.13d",  0x200000*0x3, 0x200000,CRC(50c86dea) ) /* identical to "cb1cg3.16b" */
	ROM_LOAD( "cb1cg4.14d",  0x200000*0x4, 0x200000,CRC(e93b8894) ) /* identical to "cb1cg4.18b" */
	ROM_LOAD( "cb1cg5.16d",  0x200000*0x5, 0x200000,CRC(9ee610a1) ) /* identical to "cb1cg5.19b" */
	ROM_LOAD( "cb1cg6.18a",  0x200000*0x6, 0x200000,CRC(ddc3b5cc) ) /* identical to "cb1cg6.18d" */

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "cb1ccrl.3d",	 0x000000, 0x200000,CRC(2f171c48) ) /* identical to "cb1ccrl.7b" */
	ROM_LOAD( "cb1ccrh.1d",	 0x200000, 0x080000,CRC(86124b93) ) /* identical to "cb1ccrh.5b" */

	ROM_REGION( 0x80000*12, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "cb1ptrl0.18k", 0x80000*0x0, 0x80000,CRC(f1393a03) )
	ROM_LOAD( "cb1ptrl1.16k", 0x80000*0x1, 0x80000,CRC(2ad51de7) )
	ROM_LOAD( "cb1ptrl2.15k", 0x80000*0x2, 0x80000,CRC(78f77c0d) )
	ROM_LOAD( "cb1ptrl3.14k", 0x80000*0x3, 0x80000,CRC(804bfb4a) )
	ROM_LOAD( "cb1ptrm0.18j", 0x80000*0x4, 0x80000,CRC(f4eece49) )
	ROM_LOAD( "cb1ptrm1.16j", 0x80000*0x5, 0x80000,CRC(5f3cbd7d) )
	ROM_LOAD( "cb1ptrm2.15j", 0x80000*0x6, 0x80000,CRC(02c7e4af) )
	ROM_LOAD( "cb1ptrm3.14j", 0x80000*0x7, 0x80000,CRC(ace3123b) )
	ROM_LOAD( "cb1ptru0.18f", 0x80000*0x8, 0x80000,CRC(58d35341) )
	ROM_LOAD( "cb1ptru1.16f", 0x80000*0x9, 0x80000,CRC(f4d005b0) )
	ROM_LOAD( "cb1ptru2.15f", 0x80000*0xa, 0x80000,CRC(68ffcd50) )
	ROM_LOAD( "cb1ptru3.14f", 0x80000*0xb, 0x80000,CRC(d89c1c2b) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* S22-BIOS ver1.30 */
	ROM_LOAD( "cb1datab.8k", 0, 0x080000, CRC(e2404221) )

	ROM_REGION( 0x600000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "cb1wavea.2l", 0x000000, 0x400000, CRC(b79a624d) )
	ROM_LOAD( "cb1waveb.1l", 0x400000, 0x200000, CRC(33bf08f6) )
ROM_END

ROM_START( propcycl )
	ROM_REGION( 0x400000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_BYTE( "pr2ver-a.1", 0x00003, 0x100000, CRC(3f58594c) SHA1(5fdd8c61b47b51088a201799ce0c2f08c32ef852) )
	ROM_LOAD32_BYTE( "pr2ver-a.2", 0x00002, 0x100000, CRC(c0da354a) SHA1(f27a71a62385b842404fcd8ed6513158e3639b8f) )
	ROM_LOAD32_BYTE( "pr2ver-a.3", 0x00001, 0x100000, CRC(74bf4b74) SHA1(02713aa07238cc9e30163ae24d12c034aa972ff3) )
	ROM_LOAD32_BYTE( "pr2ver-a.4", 0x00000, 0x100000, CRC(cf4d5638) SHA1(2ddd00d6ec3b85c234820507650d201e176c94a2) )

	ROM_REGION( 0x200000*2, REGION_GFX1, ROMREGION_DISPOSE ) /* 32x32x8bpp sprite tiles */
	ROM_LOAD( "pr1scg0.12f", 0x200000*0, 0x200000,CRC(2d09a869) SHA1(ce8beabaac255e1de29d944c9866022bad713519) ) /* identical to "pr1scg0.12l" */
	ROM_LOAD( "pr1scg1.10f", 0x200000*1, 0x200000,CRC(7433c5bd) SHA1(a8fd4e73de66e3d443c0cb5b5beef8f467014815) ) /* identical to "pr1scg1.10l" */

	ROM_REGION( 0x200000*8, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "pr1cg0.12b",  0x200000*0x0, 0x200000,CRC(0a041238) SHA1(da5688970432f7fe39337ee9fb46ca25a53fdb11) ) /* identical to "pr1cg0.8d" */
	ROM_LOAD( "pr1cg1.10d",  0x200000*0x1, 0x200000,CRC(7d09e6a7) SHA1(892317ee0bd796fa5c70d64912ef2e696792a2d4) ) /* identical to "pr1cg1.13b" */
	ROM_LOAD( "pr1cg2.12d",  0x200000*0x2, 0x200000,CRC(659f006e) SHA1(23362a922cb1100950181fac4858b953d8fc0794) ) /* identical to "pr1cg2.14b" */
	ROM_LOAD( "pr1cg3.13d",  0x200000*0x3, 0x200000,CRC(d30bffa3) SHA1(2f05227d91d257db9fa8cae114974de602d98729) ) /* identical to "pr1cg3.16b" */
	ROM_LOAD( "pr1cg4.14d",  0x200000*0x4, 0x200000,CRC(f4636cc9) SHA1(4e01a476e418e5790878572e83a8a11536ce30ae) ) /* identical to "pr1cg4.18b" */
	ROM_LOAD( "pr1cg5.16d",  0x200000*0x5, 0x200000,CRC(97d333de) SHA1(e8f8383f49aae834dd8b57231b25899703cef966) ) /* identical to "pr1cg5.19b" */
	ROM_LOAD( "pr1cg6.18a",  0x200000*0x6, 0x200000,CRC(3e081c03) SHA1(6ccb162952f6076359b2785b5d800b39a9a3c5ce) ) /* identical to "pr1cg6.18d" */
	ROM_LOAD( "pr1cg7.15a",	 0x200000*0x7, 0x200000,CRC(ec9fc5c8) SHA1(16de614b26f06bbddae3ab56cebba45efd6fe81b) ) /* identical to "pr1cg7.19d" */

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "pr1ccrl.3d",	 0x000000, 0x200000,CRC(e01321fd) SHA1(5938c6eff8e1b3642728c3be733f567a97cb5aad) ) /* identical to "pr1ccrl.7b" */
	ROM_LOAD( "pr1ccrh.1d",	 0x200000, 0x080000,CRC(1d68bc31) SHA1(d534d0daebe7018e83b57cc7919c294ab89bddc8) ) /* identical to "pr1ccrh.5b" */
	/* These two ROMs define a huge texture tilemap using the tiles from REGION_GFX2.
	 * The tilemap has 0x100 columns.
	 *
	 * pr1ccrl contains little endian 16 bit words.  Each word references a 16x16 tile.
	 *
	 * pr1ccrh.1d contains packed nibbles.  Each nibble encodes three tile attributes:
	 *	0x8 = swapxy
	 *	0x4 = flipx
	 *	0x2 = flipy
	 */

	ROM_REGION( 0x80000*9, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "pr1ptrl0.18k", 0x80000*0, 0x80000,CRC(fddb27a2) SHA1(6e837b45e3f9ed7ca3d1a457d0f0124de5618d1f) )
	ROM_LOAD( "pr1ptrl1.16k", 0x80000*1, 0x80000,CRC(6964dd06) SHA1(f38a550165504693d20892a7dcfaf01db19b04ef) )
	ROM_LOAD( "pr1ptrl2.15k", 0x80000*2, 0x80000,CRC(4d7ed1d4) SHA1(8f72864a06ff8962e640cb36d062bddf5d110308) )
	ROM_LOAD( "pr1ptrm0.18j", 0x80000*3, 0x80000,CRC(b6f204b7) SHA1(3b34f240b399b6406faaf338ae0ab536247e64a6) )
	ROM_LOAD( "pr1ptrm1.16j", 0x80000*4, 0x80000,CRC(949588b7) SHA1(fdaf50ff2496200b9c981efc18b035f3c0a96ace) )
	ROM_LOAD( "pr1ptrm2.15j", 0x80000*5, 0x80000,CRC(dc1cef0a) SHA1(8cbc02cf73fac3cc110b676d77602ae628385eae) )
	ROM_LOAD( "pr1ptru0.18f", 0x80000*6, 0x80000,CRC(5d66a7c4) SHA1(c9ed1c18724192d45c1f6b40096f15d02baf2401) )
	ROM_LOAD( "pr1ptru1.16f", 0x80000*7, 0x80000,CRC(e9a3f72b) SHA1(f967e1adf8eee4fffdf4288d36a93c5bb4f9a126) )
	ROM_LOAD( "pr1ptru2.15f", 0x80000*8, 0x80000,CRC(c346a842) SHA1(299bc0a30d0e74d8adfa3dc605aebf6439f5bc18) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* SS22-BIOS ver1.41 */
	ROM_LOAD( "pr1data.8k", 0, 0x080000, CRC(2e5767a4) )
	ROM_REGION( 0x800000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "pr1wavea.2l", 0x000000, 0x400000, CRC(320f3913) )
	ROM_LOAD( "pr1waveb.1l", 0x400000, 0x400000, CRC(d91acb26) )
ROM_END

ROM_START( victlapw )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_BYTE( "advprgll.4d", 0x00003, 0x80000, CRC(4dc1b0ab) )
	ROM_LOAD32_BYTE( "advprglm.2d", 0x00002, 0x80000, CRC(7b658bef) )
	ROM_LOAD32_BYTE( "advprgum.8d", 0x00001, 0x80000, CRC(af67f2fb) )
	ROM_LOAD32_BYTE( "advprguu.6d", 0x00000, 0x80000, CRC(b60e5d2b) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )

	ROM_REGION( 0x200000*8, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "adv1cg0.2a",  0x200000*0x0, 0x200000,CRC(13353848) )
	ROM_LOAD( "adv1cg1.1c",  0x200000*0x1, 0x200000,CRC(1542066c) )
	ROM_LOAD( "adv1cg2.2d",  0x200000*0x2, 0x200000,CRC(111f371c) )
	ROM_LOAD( "adv1cg3.1e",  0x200000*0x3, 0x200000,CRC(a077831f) )
	ROM_LOAD( "adv1cg4.2f",  0x200000*0x4, 0x200000,CRC(71abdacf) )
	ROM_LOAD( "adv1cg5.1j",  0x200000*0x5, 0x200000,CRC(cd6cd798) )
	ROM_LOAD( "adv1cg6.2k",  0x200000*0x6, 0x200000,CRC(94bdafba) )
	ROM_LOAD( "adv1cg7.1n",	 0x200000*0x7, 0x200000,CRC(18823475) )

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "adv1ccrl.5a",	 0x000000, 0x200000,CRC(dd2b96ae) ) /* ident to adv1ccrl.5l */
	ROM_LOAD( "adv1ccrh.5c",	 0x200000, 0x080000,CRC(5719844a) ) /* ident to adv1ccrh.5j */

	ROM_REGION( 0x80000*9, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "adv1pot.l0", 0x80000*0, 0x80000,CRC(3b85b2a4) )
	ROM_LOAD( "adv1pot.l1", 0x80000*1, 0x80000,CRC(601d6488) )
	ROM_LOAD( "adv1pot.l2", 0x80000*2, 0x80000,CRC(a0323a84) )
	ROM_LOAD( "adv1pot.m0", 0x80000*3, 0x80000,CRC(20951aa2) )
	ROM_LOAD( "adv1pot.m1", 0x80000*4, 0x80000,CRC(5aed6fbf) )
	ROM_LOAD( "adv1pot.m2", 0x80000*5, 0x80000,CRC(00cbff92) )
	ROM_LOAD( "adv1pot.u0", 0x80000*6, 0x80000,CRC(6b73dd2a) )
	ROM_LOAD( "adv1pot.u1", 0x80000*7, 0x80000,CRC(c8788f74) )
	ROM_LOAD( "adv1pot.u2", 0x80000*8, 0x80000,CRC(e67f29c5) )

	ROM_REGION( 8*1024, REGION_USER1, 0 ) /* EPROM */
	ROM_LOAD( "eeprom.9e", 0, 8*1024, CRC(35fd9f7a) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* MCU */
	ROM_LOAD( "adv1data.6r", 0, 0x080000, CRC(10eecdb4) )
	ROM_REGION( 0x400000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "adv1wav0.10r", 0x000000, 0x100000, CRC(f07b2d9d) )
	ROM_LOAD( "adv1wav1.10p", 0x100000, 0x100000, CRC(737f3c7a) )
	ROM_LOAD( "adv1wav2.10n", 0x200000, 0x100000, CRC(c1a5ca5e) )
	ROM_LOAD( "adv1wav3.10l", 0x300000, 0x100000, CRC(fc6b8004) )
ROM_END

ROM_START( raveracw )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_BYTE( "rv2prllb.4d", 0x00003, 0x80000, CRC(3017cd1e) )
	ROM_LOAD32_BYTE( "rv2prlmb.2d", 0x00002, 0x80000, CRC(894be0c3) )
	ROM_LOAD32_BYTE( "rv2prumb.8d", 0x00001, 0x80000, CRC(6414a800) )
	ROM_LOAD32_BYTE( "rv2pruub.6d", 0x00000, 0x80000, CRC(a9f18714) )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )

	ROM_REGION( 0x200000*8, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "rv1cg0.1a", 0x200000*0x0, 0x200000,CRC(c518f06b) ) /* rv1cg0.2a */
	ROM_LOAD( "rv1cg1.1c", 0x200000*0x1, 0x200000,CRC(6628f792) ) /* rv1cg1.2c */
	ROM_LOAD( "rv1cg2.1d", 0x200000*0x2, 0x200000,CRC(0b707cc5) ) /* rv1cg2.2d */
	ROM_LOAD( "rv1cg3.1e", 0x200000*0x3, 0x200000,CRC(39b62921) ) /* rv1cg3.2e */
	ROM_LOAD( "rv1cg4.1f", 0x200000*0x4, 0x200000,CRC(a9791ea2) ) /* rv1cg4.2f */
	ROM_LOAD( "rv1cg5.1j", 0x200000*0x5, 0x200000,CRC(b2c79ec1) ) /* rv1cg5.2j */
	ROM_LOAD( "rv1cg6.1k", 0x200000*0x6, 0x200000,CRC(8cddedc2) ) /* rv1cg6.2k */
	ROM_LOAD( "rv1cg7.1n", 0x200000*0x7, 0x200000,CRC(b39147ca) ) /* rv1cg7.2n */

	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "rv1ccrl.5a",	 0x000000, 0x200000,CRC(bc634f72) ) /* rv1ccrl.5l */
	ROM_LOAD( "rv1ccrh.5c",	 0x200000, 0x080000,CRC(a741b262) ) /* rv1ccrh.5j */

	ROM_REGION( 0x80000*12, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "rv1potl0.5b", 0x80000*0x0, 0x80000,CRC(de2ce519) )
	ROM_LOAD( "rv1potl1.4b", 0x80000*0x1, 0x80000,CRC(2215cb5a) )
	ROM_LOAD( "rv1potl2.3b", 0x80000*0x2, 0x80000,CRC(ddb15bf7) )
	ROM_LOAD( "rv1potl3.2b", 0x80000*0x3, 0x80000,CRC(fa9361ca) )
	ROM_LOAD( "rv1potm0.5c", 0x80000*0x4, 0x80000,CRC(3c024f3a) )
	ROM_LOAD( "rv1potm1.4c", 0x80000*0x5, 0x80000,CRC(b1a32a68) )
	ROM_LOAD( "rv1potm2.3c", 0x80000*0x6, 0x80000,CRC(a414fe15) )
	ROM_LOAD( "rv1potm3.2c", 0x80000*0x7, 0x80000,CRC(2953bbb4) )
	ROM_LOAD( "rv1potu0.5d", 0x80000*0x8, 0x80000,CRC(b9eaf3cc) )
	ROM_LOAD( "rv1potu1.4d", 0x80000*0x9, 0x80000,CRC(a5c55258) )
	ROM_LOAD( "rv1potu2.3d", 0x80000*0xa, 0x80000,CRC(c18fcb74) )
	ROM_LOAD( "rv1potu3.2d", 0x80000*0xb, 0x80000,CRC(79735aaa) )

	ROM_REGION( 0x2100, REGION_USER1, 0 )
	ROM_LOAD( "rv1eeprm.9e", 0x0000, 0x2000, CRC(801222e6) ) /* EPROM */
	ROM_LOAD( "rr1gam.2d",   0x2000, 0x0100, CRC(b2161bce) ) /* identical to rr1gam.3d,rr1gam.4d */

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* MCU */
	ROM_LOAD( "rv1data.6r", 0, 0x080000, CRC(d358ec20) )
	ROM_REGION( 0x400000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "rv1wav0.10r", 0x000000, 0x100000, CRC(5aef8143) )
	ROM_LOAD( "rv1wav1.10p", 0x100000, 0x100000, CRC(9ed9e6b3) )
	ROM_LOAD( "rv1wav2.10n", 0x200000, 0x100000, CRC(5af9dc83) )
	ROM_LOAD( "rv1wav3.10l", 0x300000, 0x100000, CRC(ffb9ad75) )
ROM_END

ROM_START( ridger2j )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_BYTE( "rrs1pr11.4d", 0x00003, 0x80000, CRC(fbf785a2) )
	ROM_LOAD32_BYTE( "rrs1prlm.2d", 0x00002, 0x80000, CRC(562f747a) )
	ROM_LOAD32_BYTE( "rrs1prum.8d", 0x00001, 0x80000, CRC(93259fb0) )
	ROM_LOAD32_BYTE( "rrs1pruu.6d", 0x00000, 0x80000, CRC(31cdefe8) )

	ROM_REGION( 0x400, REGION_GFX1, ROMREGION_DISPOSE )

	ROM_REGION( 0x200000*4, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "rrs1cg0.1a", 0x200000*0x0, 0x200000,CRC(714c0091) )
	ROM_LOAD( "rrs1cg1.2a", 0x200000*0x1, 0x200000,CRC(836545c1) )
	ROM_LOAD( "rrs1cg2.3a", 0x200000*0x2, 0x200000,CRC(00e9799d) )
	ROM_LOAD( "rrs1cg3.5a", 0x200000*0x3, 0x200000,CRC(3858983f) )

	ROM_REGION( 0x400000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "rrs1ccrl.5a", 0x000000, 0x200000,CRC(304a8b57) )
	ROM_LOAD( "rrs1ccrh.5c", 0x200000, 0x080000,CRC(bd3c86ab) )

	ROM_REGION( 0x80000*6, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "rrs1pol0.5b", 0x80000*0, 0x80000,CRC(9376c384) )
	ROM_LOAD( "rrs1pol1.4b", 0x80000*1, 0x80000,CRC(094fa832) )
	ROM_LOAD( "rrs1pom0.5c", 0x80000*2, 0x80000,CRC(b47a7f8b) )
	ROM_LOAD( "rrs1pom1.4c", 0x80000*3, 0x80000,CRC(27260361) )
	ROM_LOAD( "rrs1pou0.5d", 0x80000*4, 0x80000,CRC(74d6ec84) )
	ROM_LOAD( "rrs1pou1.4d", 0x80000*5, 0x80000,CRC(f527caaa) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* BIOS/music data? */
	ROM_LOAD( "rrs1data.6r", 0, 0x080000, CRC(b7063aa8) )
	/* 0x00000..0x001ff: data
	 * 0x10000..0x1fc7f: music data?
	 * 0x20000..0x2f57f: music data?
	 * 0x30000..0x334ff: code? contains english text
	 */
	ROM_REGION( 0x400000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "rrs1wav0.10r", 0x100000*0, 0x100000,CRC(99d11a2d) )
	ROM_LOAD( "rrs1wav1.10p", 0x100000*1, 0x100000,CRC(ad28444a) )
	ROM_LOAD( "rrs1wav2.10n", 0x100000*2, 0x100000,CRC(6f0d4619) )
	ROM_LOAD( "rrs1wav3.10l", 0x100000*3, 0x100000,CRC(106e761f) )
ROM_END

ROM_START( ridgeraj )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_BYTE( "rr1prll.4d", 0x00003, 0x80000, CRC(4bb7fc86) )
	ROM_LOAD32_BYTE( "rr1prlm.2d", 0x00002, 0x80000, CRC(68e13830) )
	ROM_LOAD32_BYTE( "rr1prum.8d", 0x00001, 0x80000, CRC(705ef78a) )
	ROM_LOAD32_BYTE( "rr1pruu.6d", 0x00000, 0x80000, CRC(c1371f96) )

	ROM_REGION( 0x400, REGION_GFX1, ROMREGION_DISPOSE )

	ROM_REGION( 0x200000*8, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_LOAD( "rr1cg0.1a", 0x200000*0x0, 0x200000,CRC(d1b0eec6) )
	ROM_LOAD( "rr1cg1.2a", 0x200000*0x1, 0x200000,CRC(bb695d89) )
	ROM_LOAD( "rr1cg2.3a", 0x200000*0x2, 0x200000,CRC(8f374c0a) )
	ROM_LOAD( "rr1cg3.5a", 0x200000*0x3, 0x200000,CRC(072a5c47) )

	ROM_REGION( 0x300000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_LOAD( "rr1ccrl.5a", 0x000000, 0x200000,CRC(c15cb257) )
	ROM_LOAD( "rr1ccrh.5c", 0x200000, 0x100000,CRC(346a1c95) )

	ROM_REGION( 0x80000*6, REGION_GFX4, 0 ) /* 3d model data */
	ROM_LOAD( "rr1potl0.5b", 0x80000*0, 0x80000,CRC(3ac193e3) )
	ROM_LOAD( "rr1potl1.4b", 0x80000*1, 0x80000,CRC(ac3ffba5) )
	ROM_LOAD( "rr1potm0.5c", 0x80000*2, 0x80000,CRC(42a3fa08) )
	ROM_LOAD( "rr1potm1.4c", 0x80000*3, 0x80000,CRC(1bc1ea7b) )
	ROM_LOAD( "rr1potu0.5d", 0x80000*4, 0x80000,CRC(5e367f72) )
	ROM_LOAD( "rr1potu1.4d", 0x80000*5, 0x80000,CRC(31d92475) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* BIOS */
	ROM_LOAD( "rr1data.6r", 0, 0x080000, CRC(18f5f748) )

	ROM_REGION( 0x400000, REGION_USER2, 0 ) /* sound samples */
	ROM_LOAD( "rr1wav0.10r", 0x100000*0, 0x100000,CRC(a8e85bde) )
	ROM_LOAD( "rr1wav1.10p", 0x100000*1, 0x100000,CRC(35f47c8e) )
	ROM_LOAD( "rr1wav2.10n", 0x100000*2, 0x100000,CRC(3244cb59) )
	ROM_LOAD( "rr1wav3.10l", 0x100000*3, 0x100000,CRC(c4cda1a7) )
ROM_END

ROM_START( timecrsa )
	ROM_REGION( 0x400000, REGION_CPU1, 0 ) /* main program */
	ROM_LOAD32_WORD_SWAP( "ts2ver-a.1", 0x00002, 0x200000, CRC(d57eb74b) )
	ROM_LOAD32_WORD_SWAP( "ts2ver-a.2", 0x00000, 0x200000, CRC(671588af) )

	ROM_REGION( 0x200000*2, REGION_GFX1, ROMREGION_DISPOSE ) /* 32x32x8bpp sprite tiles */
	ROM_REGION( 0x200000*8, REGION_GFX2, 0 ) /* 16x16x8bpp texture tiles */
	ROM_REGION( 0x280000, REGION_GFX3, 0 ) /* texture tilemap */
	ROM_REGION( 0x80000*9, REGION_GFX4, 0 ) /* 3d model data */
	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* SS22-BIOS ver1.41 */
	ROM_REGION( 0x800000, REGION_USER2, 0 ) /* sound samples */
ROM_END

INPUT_PORTS_START( alpiner )
	PORT_START /* DIP4 */
	PORT_DIPNAME( 0x01, 0x00, "DIP1" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "DIP2" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "DIP3" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "DIP4" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "DIP5" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "DIP6" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "DIP7" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "DIP8" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_SERVICE|IPF_TOGGLE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) /* DECISION */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) /* L SELECTION */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 ) /* R SELECTION */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START /* SWING */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_X|IPF_CENTER, 100, 4, 0x00, 0xffff )

	PORT_START /* EDGE */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_Y|IPF_CENTER, 100, 4, 0x00, 0xffff )
INPUT_PORTS_END

INPUT_PORTS_START( cybrcycc )
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "DIP2-1 (test mode)" )
	PORT_DIPSETTING(    0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "DIP2-2" )
	PORT_DIPSETTING(    0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "DIP2-3" )
	PORT_DIPSETTING(    0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "DIP2-4" )
	PORT_DIPSETTING(    0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "DIP2-5" )
	PORT_DIPSETTING(    0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "DIP2-6" )
	PORT_DIPSETTING(    0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "DIP2-7" )
	PORT_DIPSETTING(    0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "DIP2-8" )
	PORT_DIPSETTING(    0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, "DIP3-1" )
	PORT_DIPSETTING(    0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, "DIP3-2" )
	PORT_DIPSETTING(    0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, "DIP3-3" )
	PORT_DIPSETTING(    0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, "DIP3-4" )
	PORT_DIPSETTING(    0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, "DIP3-5" )
	PORT_DIPSETTING(    0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "DIP3-6" )
	PORT_DIPSETTING(    0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "DIP3-7" )
	PORT_DIPSETTING(    0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "DIP3-8 (test mode)" )
	PORT_DIPSETTING(    0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )

	PORT_START /* 1:gas */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_Y|IPF_CENTER|IPF_PLAYER1,	100, 4, 0x00, 0xffff )

	PORT_START /* 2:brake */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_Y|IPF_CENTER|IPF_PLAYER2,	100, 4, 0x00, 0xffff )

	PORT_START /* 3:steering */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_X|IPF_CENTER, 100, 4, 0x00, 0xffff )

	PORT_START /* 4 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* stick shift down */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* stick shift up */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* view */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )	/* (unused) */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_TILT )		/* motion stop (unused) */
INPUT_PORTS_END

INPUT_PORTS_START( propcycl )
	PORT_START /* DIP4 */
	PORT_DIPNAME( 0x01, 0x01, "DIP1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DIP2" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DIP3" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DIP4" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DIP5" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DIP6" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DIP7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "DIP8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_SERVICE|IPF_TOGGLE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE ) /* good */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )

//	PORT_START
//	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_X|IPF_CENTER, 100, 4, 0x00, 0xffff )

//	PORT_START
//	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_Y|IPF_CENTER, 100, 4, 0x00, 0xffff )
INPUT_PORTS_END

INPUT_PORTS_START( victlap )
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "DIP2-1" )
	PORT_DIPSETTING(    0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "DIP2-2" )
	PORT_DIPSETTING(    0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "DIP2-3" )
	PORT_DIPSETTING(    0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "DIP2-4" )
	PORT_DIPSETTING(    0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "DIP2-5" )
	PORT_DIPSETTING(    0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "DIP2-6" )
	PORT_DIPSETTING(    0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "DIP2-7" )
	PORT_DIPSETTING(    0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "DIP2-8" )
	PORT_DIPSETTING(    0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, "DIP3-1 (dump memory)" )
	PORT_DIPSETTING(    0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, "DIP3-2" )
	PORT_DIPSETTING(    0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, "DIP3-3" )
	PORT_DIPSETTING(    0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, "DIP3-4" ) /* screen flip? */
	PORT_DIPSETTING(    0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, "DIP3-5" )
	PORT_DIPSETTING(    0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "DIP3-6" )
	PORT_DIPSETTING(    0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "DIP3-7" )
	PORT_DIPSETTING(    0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "DIP3-8 (test mode)" )
	PORT_DIPSETTING(    0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )

	PORT_START /* 1:gas */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_Y|IPF_CENTER|IPF_PLAYER1,	100, 4, 0x00, 0xffff )

	PORT_START /* 2:brake */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_Y|IPF_CENTER|IPF_PLAYER2,	100, 4, 0x00, 0xffff )

	PORT_START /* 3:steering */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_X|IPF_CENTER, 100, 4, 0x00, 0xffff )

	PORT_START /* 4 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* stick shift down */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* stick shift up */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* view */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )	/* (unused) */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_TILT )		/* motion stop (unused) */
INPUT_PORTS_END

INPUT_PORTS_START( timecris )
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "DIP2-1" )
	PORT_DIPSETTING(    0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "DIP2-2" )
	PORT_DIPSETTING(    0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "DIP2-3" )
	PORT_DIPSETTING(    0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "DIP2-4" )
	PORT_DIPSETTING(    0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "DIP2-5" )
	PORT_DIPSETTING(    0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "DIP2-6" )
	PORT_DIPSETTING(    0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "DIP2-7" )
	PORT_DIPSETTING(    0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "DIP2-8" )
	PORT_DIPSETTING(    0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, "DIP3-1 (dump memory)" )
	PORT_DIPSETTING(    0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, "DIP3-2" )
	PORT_DIPSETTING(    0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, "DIP3-3" )
	PORT_DIPSETTING(    0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, "DIP3-4" ) /* screen flip? */
	PORT_DIPSETTING(    0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, "DIP3-5" )
	PORT_DIPSETTING(    0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "DIP3-6" )
	PORT_DIPSETTING(    0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "DIP3-7" )
	PORT_DIPSETTING(    0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "DIP3-8 (test mode)" )
	PORT_DIPSETTING(    0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x0000, DEF_STR( On ) )

	PORT_START /* 1:gas */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_Y|IPF_CENTER|IPF_PLAYER1,	100, 4, 0x00, 0xffff )

	PORT_START /* 2:brake */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_Y|IPF_CENTER|IPF_PLAYER2,	100, 4, 0x00, 0xffff )

	PORT_START /* 3:steering */
	PORT_ANALOG( 0xffff, 0x8000, IPT_AD_STICK_X|IPF_CENTER, 100, 4, 0x00, 0xffff )

	PORT_START /* 4 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* stick shift down */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* stick shift up */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* view */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )	/* (unused) */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_TILT )		/* motion stop (unused) */
INPUT_PORTS_END

DRIVER_INIT( airco22 )
{
	namcos22_gametype = NAMCOS22_AIR_COMBAT22;
	/* int0 writes 0x700005 */
	/* int1 writes 0x700007 */
	/* int2 writes 0x700006 */
	/* int3 700004, 700014, proc */
	/* int4 rte */
	/* int5 700004, 700014, proc */
	/* int6 rte */
}

DRIVER_INIT( alpiner )
{
	/* hack for old method of reading ports */
	//	data32_t *pROM = (data32_t *)memory_region( REGION_CPU1 );
	//	pROM[0xabee/4] &= 0xffff0000; pROM[0xabee/4] |= 0x4E71; pROM[0xabf0/4] |= 0x33C1<<16;
	//	pROM[0xac9e/4] &= 0xffff0000; pROM[0xac9e/4] |= 0x4E71; pROM[0xaca0/4] |= 0x33C1<<16;
	namcos22_gametype = NAMCOS22_ALPINE_RACER;
}

DRIVER_INIT( propcycl )
{
	data32_t *pROM = (data32_t *)memory_region(REGION_CPU1);

	/* patch out protection */
	pROM[0x1992C/4] = 0x4E754E75;

	/**
	 * The dipswitch reading routine in Prop Cycle polls the
	 * dipswitch value, but promptly overwrites with zero, discarding
	 * it.
	 *
	 * Patch out this behavior, exposing an additional "secret" test.
	 *
	 * DIP5: real time display of "INST_CUNT, MODE_NUM, MODE_CUNT"
	 */
	pROM[0x22296/4] &= 0xffff0000;
	pROM[0x22296/4] |= 0x00004e75;

	namcos22_gametype = NAMCOS22_PROP_CYCLE;
}

DRIVER_INIT( ridgera )
{
	namcos22_gametype = NAMCOS22_RIDGE_RACER;
}

DRIVER_INIT( victlap )
{
	namcos22_gametype = NAMCOS22_VICTORY_LAP;
}

DRIVER_INIT( raveracw )
{
	namcos22_gametype = NAMCOS22_RAVE_RACER;
}

DRIVER_INIT( cybrcomm )
{
	namcos22_gametype = NAMCOS22_CYBER_COMMANDO;
}

DRIVER_INIT( cybrcyc )
{
	namcos22_gametype = NAMCOS22_CYBER_CYCLES;
}

DRIVER_INIT( timecris )
{
	namcos22_gametype = NAMCOS22_TIME_CRISIS;
}

/*     YEAR, NAME,    PARENT,    MACHINE,   INPUT,    INIT,     MNTR,  COMPANY, FULLNAME,                                    FLAGS */
/* System22 games */
GAMEX( 1995, cybrcomm, 0,        namcos22,  victlap,  cybrcomm, ROT0, "Namco", "Cyber Commando (Rev. CY1, Japan)"          , GAME_NO_SOUND|GAME_NOT_WORKING )
GAMEX( 1995, raveracw, 0,        namcos22,  victlap,  raveracw, ROT0, "Namco", "Rave Racer (Rev. RV2, World)"              , GAME_NO_SOUND|GAME_NOT_WORKING )
GAMEX( 1993, ridgeraj, 0,        namcos22,  victlap,  ridgera,  ROT0, "Namco", "Ridge Racer (Rev. RR1, Japan)"             , GAME_NO_SOUND|GAME_NOT_WORKING )
GAMEX( 1994, ridger2j, 0,        namcos22,  victlap,  ridgera,  ROT0, "Namco", "Ridge Racer 2 (Rev. RRS1, Japan)"          , GAME_NO_SOUND|GAME_NOT_WORKING )
GAMEX( 1996, victlapw, 0,        namcos22,  victlap,  victlap,  ROT0, "Namco", "Ace Driver: Victory Lap (Rev. ADV2, World)", GAME_NO_SOUND|GAME_NOT_WORKING )

/* Super System22 games */
GAMEX( 1995, airco22b, 0,        namcos22s, victlap,  airco22,  ROT0, "Namco", "Air Combat 22 (Rev. ACS1 Ver.B)"           , GAME_NO_SOUND|GAME_NOT_WORKING )
GAMEX( 1995, alpinerd, 0,        namcos22s, alpiner,  alpiner,  ROT0, "Namco", "Alpine Racer (Rev. AR2 Ver.D)"             , GAME_NO_SOUND|GAME_NOT_WORKING )
GAMEX( 1995, alpinerc, alpinerd, namcos22s, alpiner,  alpiner,  ROT0, "Namco", "Alpine Racer (Rev. AR2 Ver.C)"             , GAME_NO_SOUND|GAME_NOT_WORKING )
GAMEX( 1995, cybrcycc, 0,        namcos22s, cybrcycc, cybrcyc,  ROT0, "Namco", "Cyber Cycles (Rev. CB2 Ver.C)"             , GAME_NO_SOUND|GAME_NOT_WORKING )
GAMEX( 1996, propcycl, 0,        namcos22s, propcycl, propcycl, ROT0, "Namco", "Prop Cycle (Rev. PR2 Ver.A)"                , GAME_NO_SOUND|GAME_IMPERFECT_GRAPHICS )
GAMEX( 1995, timecrsa, 0,        namcos22s, timecris, timecris, ROT0, "Namco", "Time Crisis (Rev. TS2 Ver.A)"              , GAME_NO_SOUND|GAME_NOT_WORKING )

/* Not dumped yet */

/* System22 games */
//GAMEX( 1994, acedrvrx, "Ace Driver")

/* Super System22 games */
//GAMEX( 1995, dirtdshx, "Dirt Dash")
//GAMEX( 1996, tokyowrx, "Tokyo Wars")
//GAMEX( 1996, alpinr2x, "Alpine Racer 2")
//GAMEX( 1996, alpinesx, "Alpine Surfer")
//GAMEX( 1996, aquajetx, "Aqua Jet")
//GAMEX( 1997, armdilox, "Armidillo Racing")
//GAMEX( 199?, downhbkx, "Downhill Bikers")
//GAMEX( 1995, timecris, 0,      namcos22s, timecris, timecris, ROT0, "Namco", "Time Crisis (Rev. TS2 Ver.B)"              , GAME_NO_SOUND|GAME_NOT_WORKING )
