/***************************************************************************
Namco System 21

	There are at least three hardware variations, all of which are based on
	Namco System2:

	1. Winning Run was a mass-produced prototype
		(point ROMs and 2d sprites are different format)

	2. Cyber Sled uses 4xTMS320C25

	3. Starblade uses 5xTMS320C20


Known Issues:
  - The sprite layer's (tx,ty) registers are currently hacked for each game.

  - There's some low level rendering glitches (namcos3d.c)
	ROLT (roll type) specifies the order in which rotation transformations
	should be applied, i.e. xyz or xzy (matrix multiplication is not associative).
	The current meaning of ROLT seems to be consistant across System21 games, and even
	System22, but it's surely not 100% correct in the current implementation -
	There are some suspiciously oriented objects.

  - Starblade has some missing background graphics (i.e. the spaceport runway at
	beginning of stage#1, various large startship cruisers drifting in space, etc.
	The main CPU doesn't include information for these objects in its display list, nor
	does it seem to pass any temporal parameters to the DSP.

  	I think these things rely on custom DSP code/data written by the main CPU on startup,
	though I'd love to be proven wrong.  Consider that some of the other games (including
	Starblade itself) render comparably complex scenes all explicitly described by the
	the object display list.

  - Starblade's point ROMs frequently contain an apparently garbage value in the most
  	significant byte, but the ROMs have been confirmed to be good.  We currently mask it
  	out, depending on rendering context.

  - Starblade (and probably other games) writes a lot of data/code to the DSP processors on
	startup.  Once it is disassembled and understood, it may be possible to insert a
	trojan to fetch the DSP kernel ROM code, and faithfully emulate all aspects of
	the system.

  - Solvalou does some interesting communication with the DSP processors on startup, and
  	refuses to run if a problem occurs.  The routines testing the return code have been
  	patched out for now.

  - Possible bad sprite banking: see CyberSled title screen

  - Some incorrect polygon colors in CyberSled (note that we currently map only a small
  	slice of the palette).

  - The palette has many banks of similar color entries varying in intensity from
  	dark to bright.  Perhaps these are used for different shading schemes depending
	on view angle to a surface?

  - Controls not mapped in CyberSled

  - Lamps/vibration outputs not mapped

-----------------------------------------------------------------------
Board 1 : DSP Board - 1st PCB. (Uppermost)
DSP Type 1 : 4 x TMS320C25 connected x 4 x Namco Custom chip 67 (68 pin PLCC) (Cybersled)
DSP Type 2 : 5 x TMS320C20 (Starblade)
OSC: 40.000MHz
RAM: HM62832 x 2, M5M5189 x 4, ISSI IS61C68 x 16
ROMS: TMS27C040
Custom Chips:
4 x Namco Custom 327 (24 pin NDIP), each one located next to a chip 67.
4 x Namco Custom chip 342 (160 pin PQFP), there are 3 leds (red/green/yellow) connected to each 342 chip. (12 leds total)
2 x Namco Custom 197 (28 pin NDIP)
Namco Custom chip 317 IDC (180 pin PQFP)
Namco Custom chip 195 (160 pin PQFP)
-----------------------------------------------------------------------
Board 2 : Unknown Board - 2nd PCB (no roms)
OSC: 20.000MHz
RAM: HM62256 x 10, 84256 x 4, CY7C128 x 5, M5M5178 x 4
OTHER Chips:
MB8422-90LP
L7A0565 316 (111) x 1 (100 PIN PQFP)
150 (64 PIN PQFP)
167 (128 PIN PQFP)
L7A0564 x 2 (100 PIN PQFP)
157 x 16 (24 PIN NDIP)
-----------------------------------------------------------------------
Board 3 : CPU Board - 3rd PCB (looks very similar to Namco System 2 CPU PCB)
CPU: MC68000P12 x 2 @ 12 MHz (16-bit)
Sound CPU: MC68B09EP (3 MHz)
Sound Chips: C140 24-channel PCM (Sound Effects), YM2151 (Music), YM3012 (?)
XTAL: 3.579545 MHz
OSC: 49.152 MHz
RAM: MB8464 x 2, MCM2018 x 2, HM65256 x 4, HM62256 x 2

Other Chips:
Sharp PC900 - Opto-isolator
Sharp PC910 - Opto-isolator
HN58C65P (EEPROM)
MB3771
MB87077-SK x 2 (24 pin NDIP, located in sound section)
LB1760 (16 pin DIP, located next to SYS87B-2B)
CY7C132 (48 PIN DIP)

Namco Custom:
148 x 2 (64 pin PQFP)
C68 (64 pin PQFP)
139 (64 pin PQFP)
137 (28 pin NDIP)
149 (28 pin NDIP, near C68)
-----------------------------------------------------------------------
Board 4 : 4th PCB (bottom-most)
OSC: 38.76922 MHz
There is a 6 wire plug joining this PCB with the CPU PCB. It appears to be video cable (RGB, Sync etc..)
Jumpers:
JP7 INTERLACE = SHORTED (Other setting is NON-INTERLACE)
JP8 68000 = SHORTED (Other setting is 68020)
Namco Custom Chips:
C355 (160 pin PQFP)
187 (120 pin PQFP)
138 (64 pin PQFP)
165 (28 pin NDIP)
-----------------------------------------------------------------------

-------------------
Air Combat by NAMCO
-------------------
malcor


Location        Device     File ID      Checksum
-------------------------------------------------
CPU68  1J       27C4001    MPR-L.AC1      9859   [ main program ]  [ rev AC1 ]
CPU68  3J       27C4001    MPR-U.AC1      97F1   [ main program ]  [ rev AC1 ]
CPU68  1J       27C4001    MPR-L.AC2      C778   [ main program ]  [ rev AC2 ]
CPU68  3J       27C4001    MPR-U.AC2      6DD9   [ main program ]  [ rev AC2 ]
CPU68  1C      MB834000    EDATA1-L.AC1   7F77   [    data      ]
CPU68  3C      MB834000    EDATA1-U.AC1   FA2F   [    data      ]
CPU68  3A      MB834000    EDATA-U.AC1    20F2   [    data      ]
CPU68  1A      MB834000    EDATA-L.AC1    9E8A   [    data      ]
CPU68  8J        27C010    SND0.AC1       71A8   [  sound prog  ]
CPU68  12B     MB834000    VOI0.AC1       08CF   [   voice 0    ]
CPU68  12C     MB834000    VOI1.AC1       925D   [   voice 1    ]
CPU68  12D     MB834000    VOI2.AC1       C498   [   voice 2    ]
CPU68  12E     MB834000    VOI3.AC1       DE9F   [   voice 3    ]
CPU68  4C        27C010    SPR-L.AC1      473B   [ slave prog L ]  [ rev AC1 ]
CPU68  6C        27C010    SPR-U.AC1      CA33   [ slave prog U ]  [ rev AC1 ]
CPU68  4C        27C010    SPR-L.AC2      08CE   [ slave prog L ]  [ rev AC2 ]
CPU68  6C        27C010    SPR-U.AC2      A3F1   [ slave prog U ]  [ rev AC2 ]
OBJ(B) 5S       HN62344    OBJ0.AC1       CB72   [ object data  ]
OBJ(B) 5X       HN62344    OBJ1.AC1       85E2   [ object data  ]
OBJ(B) 3S       HN62344    OBJ2.AC1       89DC   [ object data  ]
OBJ(B) 3X       HN62344    OBJ3.AC1       58FF   [ object data  ]
OBJ(B) 4S       HN62344    OBJ4.AC1       46D6   [ object data  ]
OBJ(B) 4X       HN62344    OBJ5.AC1       7B91   [ object data  ]
OBJ(B) 2S       HN62344    OBJ6.AC1       5736   [ object data  ]
OBJ(B) 2X       HN62344    OBJ7.AC1       6D45   [ object data  ]
OBJ(B) 17N     PLHS18P8    3P0BJ3         4342
OBJ(B) 17N     PLHS18P8    3POBJ4         1143
DSP    2N       HN62344    AC1-POIL.L     8AAF   [   DSP data   ]
DSP    2K       HN62344    AC1-POIL.L     CF90   [   DSP data   ]
DSP    2E       HN62344    AC1-POIH       4D02   [   DSP data   ]
DSP    17D     GAL16V8A    3PDSP5         6C00


NOTE:  CPU68  - CPU board        2252961002  (2252971002)
       OBJ(B) - Object board     8623961803  (8623963803)
       DSP    - DSP board        8623961703  (8623963703)
       PGN(C) - PGN board        2252961300  (8623963600)

       Namco System 21 Hardware

       ROMs that have the same locations are different revisions
       of the same ROMs (AC1 or AC2).


Jumper settings:


Location    Position set    alt. setting
----------------------------------------

CPU68 PCB:

  JP2          /D-ST           /VBL
  JP3
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "namcos2.h"
#include "cpu/m6809/m6809.h"
#include "namcoic.h"

/* globals (shared by videohrdw/namcos21.c) */

data16_t *namcos21_dspram16;
data16_t *namcos21_spritepos;

/* private data */

static data16_t	*mpDataROM;
static data16_t	*mpSharedRAM1;
static data8_t	*mpDualPortRAM;
static data16_t	*mpSharedRAM2;

extern WRITE16_HANDLER( namcos21_polyattr0_w );
extern WRITE16_HANDLER( namcos21_polyattr1_w );
extern WRITE16_HANDLER( namcos21_objattr_w );

/* dual port ram memory handlers */

static READ16_HANDLER( namcos2_68k_dualportram_word_r )
{
	return mpDualPortRAM[offset];
}

static WRITE16_HANDLER( namcos2_68k_dualportram_word_w )
{
	if( ACCESSING_LSB )
	{
		mpDualPortRAM[offset] = data & 0xff;
	}
}

static READ_HANDLER( namcos2_dualportram_byte_r )
{
	return mpDualPortRAM[offset];
}

static WRITE_HANDLER( namcos2_dualportram_byte_w )
{
	mpDualPortRAM[offset] = data;
}

/* shared RAM memory handlers */

static READ16_HANDLER( shareram1_r )
{
	return mpSharedRAM1[offset];
}

static WRITE16_HANDLER( shareram1_w )
{
	COMBINE_DATA( &mpSharedRAM1[offset] );
}

static READ16_HANDLER( shareram2_r )
{
	return mpSharedRAM2[offset];
}

static WRITE16_HANDLER( shareram2_w )
{
	COMBINE_DATA(&mpSharedRAM2[offset]);
}

/* memory handlers for shared DSP RAM (used to pass 3d parameters) */

static READ16_HANDLER( dspram16_r )
{
	/* logerror( "polyram[%08x] == %04x; pc==0x%08x\n",
		offset*2, namcos21_dspram16[offset], activecpu_get_pc() ); */
	return namcos21_dspram16[offset];
}

static WRITE16_HANDLER( dspram16_w )
{
	COMBINE_DATA( &namcos21_dspram16[offset] );
	/* logerror( "polyram[%08x] := %04x\n", offset*2, namcos21_dspram16[offset] ); */
}

static READ16_HANDLER( dsp_status_r )
{
	return 1;
}

/* some games have read-only areas where more ROMs are mapped */

static READ16_HANDLER( data_r )
{
	return mpDataROM[offset];
}

static READ16_HANDLER( data2_r )
{
	return mpDataROM[0x100000/2+offset];
}

/* palette memory handlers */

static READ16_HANDLER( paletteram16_r )
{
	return paletteram16[offset];
}

static WRITE16_HANDLER( paletteram16_w )
{
	COMBINE_DATA(&paletteram16[offset]);
}


/*************************************************************/
/* MASTER 68000 CPU Memory declarations 					 */
/*************************************************************/

static MEMORY_READ16_START( readmem_master_default )
	{ 0x000000, 0x0fffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },
	{ 0x180000, 0x183fff, NAMCOS2_68K_EEPROM_R },
	{ 0x1c0000, 0x1fffff, namcos2_68k_master_C148_r },
	{ 0x200000, 0x20ffff, dspram16_r },
	{ 0x440000, 0x440001, dsp_status_r },
	{ 0x480000, 0x4807ff, MRA16_RAM },
	{ 0x700000, 0x7141ff, namco_obj16_r },
	{ 0x740000, 0x760001, MRA16_RAM }, /* palette */
	{ 0x800000, 0x8fffff, data_r },
	{ 0x900000, 0x90ffff, shareram1_r },
	{ 0xa00000, 0xa00fff, namcos2_68k_dualportram_word_r },
	{ 0xb00000, 0xb03fff, MRA16_RAM },
	{ 0xd00000, 0xdfffff, data2_r },
MEMORY_END

static MEMORY_WRITE16_START( writemem_master_default )
	{ 0x000000, 0x0fffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM },
	{ 0x180000, 0x183fff, NAMCOS2_68K_EEPROM_W },
	{ 0x1c0000, 0x1fffff, namcos2_68k_master_C148_w },
	{ 0x200000, 0x20ffff, dspram16_w },
	{ 0x400000, 0x400001, namcos21_polyattr0_w },
	{ 0x440000, 0x440001, namcos21_polyattr1_w },
	{ 0x480000, 0x4807ff, MWA16_RAM },
	{ 0x700000, 0x7141ff, namco_obj16_w },
	{ 0x720000, 0x72000f, namcos21_objattr_w },
	{ 0x740000, 0x760001, MWA16_RAM, &paletteram16 },
	{ 0x900000, 0x90ffff, shareram1_w, &mpSharedRAM1 },
	{ 0xa00000, 0xa00fff, namcos2_68k_dualportram_word_w },
	{ 0xb00000, 0xb03fff, MWA16_RAM, &mpSharedRAM2 },
MEMORY_END

/*************************************************************/
/* SLAVE 68000 CPU Memory declarations						 */
/*************************************************************/

static MEMORY_READ16_START( readmem_slave_default )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x13ffff, MRA16_RAM },
	{ 0x1c0000, 0x1fffff, namcos2_68k_slave_C148_r },
	{ 0x200000, 0x20ffff, MRA16_RAM }, /* DSP RAM */
	{ 0x700000, 0x7141ff, namco_obj16_r },
	{ 0x740000, 0x760001, paletteram16_r },
	{ 0x800000, 0x8fffff, data_r },
	{ 0x900000, 0x90ffff, shareram1_r },
	{ 0xa00000, 0xa00fff, namcos2_68k_dualportram_word_r },
	{ 0xb00000, 0xb03fff, shareram2_r },
	{ 0xd00000, 0xdfffff, data2_r },
MEMORY_END

static MEMORY_WRITE16_START( writemem_slave_default )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x13ffff, MWA16_RAM },
	{ 0x1c0000, 0x1fffff, namcos2_68k_slave_C148_w },
	{ 0x200000, 0x20ffff, MWA16_RAM, &namcos21_dspram16 },
	{ 0x700000, 0x7141ff, namco_obj16_w },
	{ 0x718000, 0x718001, MWA16_NOP },
	{ 0x740000, 0x760001, paletteram16_w },
	{ 0x900000, 0x90ffff, shareram1_w },
	{ 0xa00000, 0xa00fff, namcos2_68k_dualportram_word_w },
	{ 0xb00000, 0xb03fff, shareram2_w },
MEMORY_END

/*************************************************************/
/* SOUND 6809 CPU Memory declarations						 */
/*************************************************************/

static MEMORY_READ_START( readmem_sound )
	{ 0x0000, 0x3fff, BANKED_SOUND_ROM_R }, /* banked */
	{ 0x4000, 0x4001, YM2151_status_port_0_r },
	{ 0x5000, 0x6fff, C140_r },
	{ 0x7000, 0x77ff, namcos2_dualportram_byte_r },
	{ 0x7800, 0x7fff, namcos2_dualportram_byte_r },	/* mirror */
	{ 0x8000, 0x9fff, MRA_RAM },
	{ 0xd000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem_sound )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x4000, YM2151_register_port_0_w },
	{ 0x4001, 0x4001, YM2151_data_port_0_w },
	{ 0x5000, 0x6fff, C140_w },
	{ 0x7000, 0x77ff, namcos2_dualportram_byte_w, &mpDualPortRAM },
	{ 0x7800, 0x7fff, namcos2_dualportram_byte_w }, /* mirror */
	{ 0x8000, 0x9fff, MWA_RAM },
	{ 0xa000, 0xbfff, MWA_NOP }, /* amplifier enable on 1st write */
	{ 0xc000, 0xc001, namcos2_sound_bankselect_w },
	{ 0xd001, 0xd001, MWA_NOP }, /* watchdog */
	{ 0xc000, 0xffff, MWA_NOP }, /* avoid debug log noise; games write frequently to 0xe000 */
MEMORY_END

/*************************************************************/
/* I/O HD63705 MCU Memory declarations						 */
/*************************************************************/

static MEMORY_READ_START( readmem_mcu )
	{ 0x0000, 0x0000, MRA_NOP },
	{ 0x0001, 0x0001, input_port_0_r },			/* p1,p2 start */
	{ 0x0002, 0x0002, input_port_1_r },			/* coins */
	{ 0x0003, 0x0003, namcos2_mcu_port_d_r },
	{ 0x0007, 0x0007, input_port_10_r },		/* fire buttons */
	{ 0x0010, 0x0010, namcos2_mcu_analog_ctrl_r },
	{ 0x0011, 0x0011, namcos2_mcu_analog_port_r },
	{ 0x0008, 0x003f, MRA_RAM },
	{ 0x0040, 0x01bf, MRA_RAM },
	{ 0x01c0, 0x1fff, MRA_ROM },
	{ 0x2000, 0x2000, input_port_11_r }, /* dipswitches */
	{ 0x3000, 0x3000, input_port_12_r }, /* DIAL0 */
	{ 0x3001, 0x3001, input_port_13_r }, /* DIAL1 */
	{ 0x3002, 0x3002, input_port_14_r }, /* DIAL2 */
	{ 0x3003, 0x3003, input_port_15_r }, /* DIAL3 */
	{ 0x5000, 0x57ff, namcos2_dualportram_byte_r },
	{ 0x6000, 0x6fff, MRA_NOP }, /* watchdog */
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem_mcu )
	{ 0x0003, 0x0003, namcos2_mcu_port_d_w },
	{ 0x0010, 0x0010, namcos2_mcu_analog_ctrl_w },
	{ 0x0011, 0x0011, namcos2_mcu_analog_port_w },
	{ 0x0000, 0x003f, MWA_RAM },
	{ 0x0040, 0x01bf, MWA_RAM },
	{ 0x01c0, 0x1fff, MWA_ROM },
	{ 0x5000, 0x57ff, namcos2_dualportram_byte_w, &mpDualPortRAM },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static struct GfxLayout tile_layout =
{
	16,16,
	RGN_FRAC(1,4),	/* number of tiles */
	8,		/* bits per pixel */
	{		/* plane offsets */
		0,1,2,3,4,5,6,7
	},
	{ /* x offsets */
		0*8,RGN_FRAC(1,4)+0*8,RGN_FRAC(2,4)+0*8,RGN_FRAC(3,4)+0*8,
		1*8,RGN_FRAC(1,4)+1*8,RGN_FRAC(2,4)+1*8,RGN_FRAC(3,4)+1*8,
		2*8,RGN_FRAC(1,4)+2*8,RGN_FRAC(2,4)+2*8,RGN_FRAC(3,4)+2*8,
		3*8,RGN_FRAC(1,4)+3*8,RGN_FRAC(2,4)+3*8,RGN_FRAC(3,4)+3*8
	},
	{ /* y offsets */
		0*32,1*32,2*32,3*32,
		4*32,5*32,6*32,7*32,
		8*32,9*32,10*32,11*32,
		12*32,13*32,14*32,15*32
	},
	8*64 /* sprite offset */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &tile_layout,  0x1000, 0x10 },
	{ -1 }
};

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579580,	/* 3.58 MHz ? */
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },
	{ NULL }	/* YM2151 IRQ line is NOT connected on the PCB */
};

static struct C140interface C140_interface_typeA =
{
	C140_TYPE_SYSTEM21_A,
	8000000/374,
	REGION_SOUND1,
	50
};

static struct C140interface C140_interface_typeB =
{
	C140_TYPE_SYSTEM21_A,
	8000000/374,
	REGION_SOUND1,
	50
};

static MACHINE_DRIVER_START( s21base )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,12288000) /* Master */
	MDRV_CPU_MEMORY(readmem_master_default,writemem_master_default)
	MDRV_CPU_VBLANK_INT(namcos2_68k_master_vblank,1)

	MDRV_CPU_ADD(M68000,12288000) /* Slave */
	MDRV_CPU_MEMORY(readmem_slave_default,writemem_slave_default)
	MDRV_CPU_VBLANK_INT(namcos2_68k_slave_vblank,1)

	MDRV_CPU_ADD(M6809,3072000) /* Sound */
	MDRV_CPU_MEMORY(readmem_sound,writemem_sound)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,2)
	MDRV_CPU_PERIODIC_INT(irq1_line_hold,120)

	MDRV_CPU_ADD(HD63705,2048000) /* IO */
	MDRV_CPU_MEMORY(readmem_mcu,writemem_mcu)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100) /* 100 CPU slices per frame */

	MDRV_MACHINE_INIT(namcos2)
	MDRV_NVRAM_HANDLER(namcos2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(62*8, 60*8)
	MDRV_VISIBLE_AREA(0*8, 62*8-1, 0*8, 60*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(NAMCOS21_NUM_COLORS)
	MDRV_COLORTABLE_LENGTH(NAMCOS21_NUM_COLORS)

	MDRV_VIDEO_START(namcos21)
	MDRV_VIDEO_UPDATE(namcos21_default)

MACHINE_DRIVER_END

static MACHINE_DRIVER_START( poly_c140_typeA )
	MDRV_IMPORT_FROM(s21base)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(C140, C140_interface_typeA)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( poly_c140_typeB )
	MDRV_IMPORT_FROM(s21base)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(C140, C140_interface_typeB)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
MACHINE_DRIVER_END


ROM_START( aircombu )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* Master */
	ROM_LOAD16_BYTE( "mpr-u.ac2",  0x000000, 0x80000, CRC(a7133f85) )
	ROM_LOAD16_BYTE( "mpr-l.ac2",  0x000001, 0x80000, CRC(520a52e6) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* Slave */
	ROM_LOAD16_BYTE( "spr-u.ac2",  0x000000, 0x20000, CRC(42aca956) )
	ROM_LOAD16_BYTE( "spr-l.ac2",  0x000001, 0x20000, CRC(3e15fa19) )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound */
	ROM_LOAD( "snd0.ac1",	0x00c000, 0x004000, CRC(5c1fb84b) )
	ROM_CONTINUE(      		0x010000, 0x01c000 )
	ROM_RELOAD(				0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, CRC(a342a97e) SHA1(2c420d34dba21e409bf78ddca710fc7de65a6642) )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, CRC(a5b2a4ff) SHA1(068bdfcc71a5e83706e8b23330691973c1c214dc) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "obj0.ac2",  0x000000, 0x80000, CRC(8327ff22) )
	ROM_LOAD( "obj4.ac2",  0x080000, 0x80000, CRC(e433e344) )
	ROM_LOAD( "obj1.ac2",  0x100000, 0x80000, CRC(43af566d) )
	ROM_LOAD( "obj5.ac2",  0x180000, 0x80000, CRC(ecb19199) )
	ROM_LOAD( "obj2.ac2",  0x200000, 0x80000, CRC(dafbf489) )
	ROM_LOAD( "obj6.ac2",  0x280000, 0x80000, CRC(24cc3f36) )
	ROM_LOAD( "obj3.ac2",  0x300000, 0x80000, CRC(bd555a1d) )
	ROM_LOAD( "obj7.ac2",  0x380000, 0x80000, CRC(d561fbe3) )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 ) /* collision */
	ROM_LOAD16_BYTE( "edata-u.ac1",		0x000000, 0x80000, CRC(82320c71) )
	ROM_LOAD16_BYTE( "edata-l.ac1", 	0x000001, 0x80000, CRC(fd7947d3) )
	ROM_LOAD16_BYTE( "edata1-u.ac2",	0x100000, 0x80000, CRC(40c07095) )
	ROM_LOAD16_BYTE( "edata1-l.ac1",	0x100001, 0x80000, CRC(a87087dd) )

	ROM_REGION32_BE( 0x400000, REGION_USER2, ROMREGION_ERASE )		/* 24bit signed point data */
	ROM_LOAD32_BYTE( "poih.ac1",   0x000001, 0x80000, CRC(573bbc3b) )	/* most significant */
	ROM_LOAD32_BYTE( "poil-u.ac1", 0x000002, 0x80000, CRC(d99084b9) )
	ROM_LOAD32_BYTE( "poil-l.ac1", 0x000003, 0x80000, CRC(abb32307) )	/* least significant */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD("voi0.ac1",0x000000,0x80000,CRC(f427b119) )
	ROM_LOAD("voi1.ac1",0x080000,0x80000,CRC(c9490667) )
	ROM_LOAD("voi2.ac1",0x100000,0x80000,CRC(1fcb51ba) )
	ROM_LOAD("voi3.ac1",0x180000,0x80000,CRC(cd202e06) )
ROM_END

ROM_START( aircombj )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* Master */
	ROM_LOAD16_BYTE( "mpr-u.ac1",  0x000000, 0x80000, CRC(a4dec813) )
	ROM_LOAD16_BYTE( "mpr-l.ac1",  0x000001, 0x80000, CRC(8577b6a2) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 ) /* Slave */
	ROM_LOAD16_BYTE( "spr-u.ac1",  0x000000, 0x20000, CRC(5810e219) )
	ROM_LOAD16_BYTE( "spr-l.ac1",  0x000001, 0x20000, CRC(175a7d6c) )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound */
	ROM_LOAD( "snd0.ac1",	0x00c000, 0x004000, CRC(5c1fb84b) )
	ROM_CONTINUE(			0x010000, 0x01c000 )
	ROM_RELOAD(				0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, CRC(a342a97e) SHA1(2c420d34dba21e409bf78ddca710fc7de65a6642) )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, CRC(a5b2a4ff) SHA1(068bdfcc71a5e83706e8b23330691973c1c214dc) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "obj0.ac1",  0x000000, 0x80000, CRC(d2310c6a) )
	ROM_LOAD( "obj4.ac1",  0x080000, 0x80000, CRC(0c93b478) )
	ROM_LOAD( "obj1.ac1",  0x100000, 0x80000, CRC(f5783a77) )
	ROM_LOAD( "obj5.ac1",  0x180000, 0x80000, CRC(476aed15) )
	ROM_LOAD( "obj2.ac1",  0x200000, 0x80000, CRC(01343d5c) )
	ROM_LOAD( "obj6.ac1",  0x280000, 0x80000, CRC(c67607b1) )
	ROM_LOAD( "obj3.ac1",  0x300000, 0x80000, CRC(7717f52e) )
	ROM_LOAD( "obj7.ac1",  0x380000, 0x80000, CRC(cfa9fe5f) )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "edata-u.ac1",   0x000000, 0x80000, CRC(82320c71) )
	ROM_LOAD16_BYTE( "edata-l.ac1",   0x000001, 0x80000, CRC(fd7947d3) )
	ROM_LOAD16_BYTE( "edata1-u.ac1",  0x100000, 0x80000, CRC(a9547509) )
	ROM_LOAD16_BYTE( "edata1-l.ac1",  0x100001, 0x80000, CRC(a87087dd) )

	ROM_REGION32_BE( 0x400000, REGION_USER2, ROMREGION_ERASE )		/* 24bit signed point data */
	ROM_LOAD32_BYTE( "poih.ac1",   0x000001, 0x80000, CRC(573bbc3b) )	/* most significant */
	ROM_LOAD32_BYTE( "poil-u.ac1", 0x000002, 0x80000, CRC(d99084b9) )
	ROM_LOAD32_BYTE( "poil-l.ac1", 0x000003, 0x80000, CRC(abb32307) )	/* least significant */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD("voi0.ac1",0x000000,0x80000,CRC(f427b119) )
	ROM_LOAD("voi1.ac1",0x080000,0x80000,CRC(c9490667) )
	ROM_LOAD("voi2.ac1",0x100000,0x80000,CRC(1fcb51ba) )
	ROM_LOAD("voi3.ac1",0x180000,0x80000,CRC(cd202e06) )
ROM_END

ROM_START( cybsled )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* Master */
	ROM_LOAD16_BYTE( "mpru.3j",  0x000000, 0x80000, CRC(cc5a2e83) )
	ROM_LOAD16_BYTE( "mprl.1j",  0x000001, 0x80000, CRC(f7ee8b48) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 ) /* Slave */
	ROM_LOAD16_BYTE( "spru.6c",  0x000000, 0x80000, CRC(28dd707b) )
	ROM_LOAD16_BYTE( "sprl.4c",  0x000001, 0x80000, CRC(437029de) )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound */
	ROM_LOAD( "snd0.8j",	0x00c000, 0x004000, CRC(3dddf83b) )
	ROM_CONTINUE(			0x010000, 0x01c000 )
	ROM_RELOAD(				0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, CRC(a342a97e) SHA1(2c420d34dba21e409bf78ddca710fc7de65a6642) )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, CRC(a5b2a4ff) SHA1(068bdfcc71a5e83706e8b23330691973c1c214dc) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "obj0.5s",  0x000000, 0x80000, CRC(5ae542d5) )
	ROM_LOAD( "obj4.4s",  0x080000, 0x80000, CRC(57904076) )
	ROM_LOAD( "obj1.5x",  0x100000, 0x80000, CRC(4aae3eff) )
	ROM_LOAD( "obj5.4x",  0x180000, 0x80000, CRC(0e11ca47) )
	ROM_LOAD( "obj2.3s",  0x200000, 0x80000, CRC(d64ec4c3) )
	ROM_LOAD( "obj6.2s",  0x280000, 0x80000, CRC(7748b485) )
	ROM_LOAD( "obj3.3x",  0x300000, 0x80000, CRC(3d1f7168) )
	ROM_LOAD( "obj7.2x",  0x380000, 0x80000, CRC(b6eb6ad2) )

	ROM_REGION16_BE( 0x200000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "datau.3a",   0x000000, 0x80000, CRC(570da15d) )
	ROM_LOAD16_BYTE( "datal.1a",   0x000001, 0x80000, CRC(9cf96f9e) )
	ROM_LOAD16_BYTE( "edata0u.3b", 0x100000, 0x80000, CRC(77452533) )
	ROM_LOAD16_BYTE( "edata0l.1b", 0x100001, 0x80000, CRC(e812e290) )

	ROM_REGION32_BE( 0x400000, REGION_USER2, ROMREGION_ERASE )		/* 24bit signed point data */
	ROM_LOAD32_BYTE( "poih1.2f",  0x000001, 0x80000, CRC(eaf8bac3) )	/* most significant */
	ROM_LOAD32_BYTE( "poilu1.2k", 0x000002, 0x80000, CRC(c544a8dc) )
	ROM_LOAD32_BYTE( "poill1.2n", 0x000003, 0x80000, CRC(30acb99b) )	/* least significant */
	ROM_LOAD32_BYTE( "poih2.2j",  0x200001, 0x80000, CRC(4079f342) )
	ROM_LOAD32_BYTE( "poilu2.2l", 0x200002, 0x80000, CRC(61d816d4) )
	ROM_LOAD32_BYTE( "poill2.2p", 0x200003, 0x80000, CRC(faf09158) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD("voi0.12b",0x000000,0x80000,CRC(99d7ce46) )
	ROM_LOAD("voi1.12c",0x080000,0x80000,CRC(2b335f06) )
	ROM_LOAD("voi2.12d",0x100000,0x80000,CRC(10cd15f0) )
	ROM_LOAD("voi3.12e",0x180000,0x80000,CRC(c902b4a4) )
ROM_END

ROM_START( starblad )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* Master */
	ROM_LOAD16_BYTE( "st1_mpu.bin",  0x000000, 0x80000, CRC(483a311c) SHA1(dd9416b8d4b0f8b361630e312eac71c113064eae) )
	ROM_LOAD16_BYTE( "st1_mpl.bin",  0x000001, 0x80000, CRC(0a4dd661) SHA1(fc2b71a255a8613693c4d1c79ddd57a6d396165a) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* Slave */
	ROM_LOAD16_BYTE( "st1_spu.bin",  0x000000, 0x40000, CRC(9f9a55db) SHA1(72bf5d6908cc57cc490fa2292b4993d796b2974d) )
	ROM_LOAD16_BYTE( "st1_spl.bin",  0x000001, 0x40000, CRC(acbe39c7) SHA1(ca48b7ea619b1caaf590eed33001826ce7ef36d8) )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound */
	ROM_LOAD( "st1snd0.bin",		 0x00c000, 0x004000, CRC(c0e934a3) SHA1(678ed6705c6f494d7ecb801a4ef1b123b80979a5) )
	ROM_CONTINUE(					 0x010000, 0x01c000 )
	ROM_RELOAD( 					 0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, CRC(a342a97e) SHA1(2c420d34dba21e409bf78ddca710fc7de65a6642) )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, CRC(a5b2a4ff) SHA1(068bdfcc71a5e83706e8b23330691973c1c214dc) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "st1obj0.bin",  0x000000, 0x80000, CRC(5d42c71e) SHA1(f1aa2bb31bbbcdcac8e94334b1c78238cac1a0e7) )
	ROM_LOAD( "st1obj1.bin",  0x080000, 0x80000, CRC(c98011ad) SHA1(bc34c21428e0ef5887051c0eb0fdef5397823a82) )
	ROM_LOAD( "st1obj2.bin",  0x100000, 0x80000, CRC(6cf5b608) SHA1(c8537fbe97677c4c8a365b1cf86c4645db7a7d6b) )
	ROM_LOAD( "st1obj3.bin",  0x180000, 0x80000, CRC(cdc195bb) SHA1(91443917a6982c286b6f15381d441d061aefb138) )

	ROM_REGION16_BE( 0x100000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "st1_du.bin",  0x000000, 0x20000, CRC(2433e911) SHA1(95f5f00d3bacda4996e055a443311fb9f9a5fe2f) )
	ROM_LOAD16_BYTE( "st1_dl.bin",  0x000001, 0x20000, CRC(4a2cc252) SHA1(d9da9992bac878f8a1f5e84cc3c6d457b4705e8f) )

	ROM_REGION32_BE( 0x400000, REGION_USER2, ROMREGION_ERASE ) /* 24bit signed point data */
	ROM_LOAD32_BYTE( "st1pt0h.bin", 0x000001, 0x80000, CRC(84eb355f) SHA1(89a248b8be2e0afcee29ba4c4c9cca65d5fb246a) )
	ROM_LOAD32_BYTE( "st1pt0u.bin", 0x000002, 0x80000, CRC(1956cd0a) SHA1(7d21b3a59f742694de472c545a1f30c3d92e3390) )
	ROM_LOAD32_BYTE( "st1pt0l.bin", 0x000003, 0x80000, CRC(ff577049) SHA1(1e1595174094e88d5788753d05ce296c1f7eca75) )
	//
	ROM_LOAD32_BYTE( "st1pt1h.bin", 0x200001, 0x80000, CRC(96b1bd7d) SHA1(55da7896dda2aa4c35501a55c8605a065b02aa17) )
	ROM_LOAD32_BYTE( "st1pt1u.bin", 0x200002, 0x80000, CRC(ecf21047) SHA1(ddb13f5a2e7d192f0662fa420b49f89e1e991e66) )
	ROM_LOAD32_BYTE( "st1pt1l.bin", 0x200003, 0x80000, CRC(01cb0407) SHA1(4b58860bbc353de8b4b8e83d12b919d9386846e8) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD("st1voi0.bin",0x000000,0x80000,CRC(5b3d43a9) SHA1(cdc04f19dc91dca9fa88ba0c2fca72aa195a3694) )
	ROM_LOAD("st1voi1.bin",0x080000,0x80000,CRC(413e6181) SHA1(e827ec11f5755606affd2635718512aeac9354da) )
	ROM_LOAD("st1voi2.bin",0x100000,0x80000,CRC(067d0720) SHA1(a853b2d43027a46c5e707fc677afdaae00f450c7) )
	ROM_LOAD("st1voi3.bin",0x180000,0x80000,CRC(8b5aa45f) SHA1(e1214e639200758ad2045bde0368a2d500c1b84a) )
ROM_END

ROM_START( solvalou )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* Master */
	ROM_LOAD16_BYTE( "sv1mpu.bin",  0x000000, 0x20000, CRC(b6f92762) SHA1(d177328b3da2ab0580e101478142bc8c373d6140) )
	ROM_LOAD16_BYTE( "sv1mpl.bin",  0x000001, 0x20000, CRC(28c54c42) SHA1(32fcca2eb4bb8ba8c2587b03d3cf59f072f7fac5) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 ) /* Slave */
	ROM_LOAD16_BYTE( "sv1spu.bin",  0x000000, 0x20000, CRC(ebd4bf82) SHA1(67946360d680a675abcb3c131bac0502b2455573) )
	ROM_LOAD16_BYTE( "sv1spl.bin",  0x000001, 0x20000, CRC(7acab679) SHA1(764297c9601be99dbbffb75bbc6fe4a40ea38529) )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound */
	ROM_LOAD( "sv1snd0.bin",    0x00c000, 0x004000, CRC(5e007864) SHA1(94da2d51544c6127056beaa251353038646da15f) )
	ROM_CONTINUE(				0x010000, 0x01c000 )
	ROM_RELOAD(					0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, CRC(a342a97e) SHA1(2c420d34dba21e409bf78ddca710fc7de65a6642) )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, CRC(a5b2a4ff) SHA1(068bdfcc71a5e83706e8b23330691973c1c214dc) )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sv1obj0.bin",  0x000000, 0x80000, CRC(773798bb) SHA1(51ab76c95030bab834f1a74ae677b2f0afc18c52) )
	ROM_LOAD( "sv1obj4.bin",  0x080000, 0x80000, CRC(33a008a7) SHA1(4959a0ac24ad64f1367e2d8d63d39a0273c60f3e) )
	ROM_LOAD( "sv1obj1.bin",  0x100000, 0x80000, CRC(a36d9e79) SHA1(928d9995e97ee7509e23e6cc64f5e7bfb5c02d42) )
	ROM_LOAD( "sv1obj5.bin",  0x180000, 0x80000, CRC(31551245) SHA1(385452ea4830c466263ad5241313ac850dfef756) )
	ROM_LOAD( "sv1obj2.bin",  0x200000, 0x80000, CRC(c8672b8a) SHA1(8da037b27d2c2b178aab202781f162371458f788) )
	ROM_LOAD( "sv1obj6.bin",  0x280000, 0x80000, CRC(fe319530) SHA1(8f7e46c8f0b86c7515f6d763b795ce07d11c77bc) )
	ROM_LOAD( "sv1obj3.bin",  0x300000, 0x80000, CRC(293ef1c5) SHA1(f677883bfec16bbaeb0a01ac565d0e6cac679174) )
	ROM_LOAD( "sv1obj7.bin",  0x380000, 0x80000, CRC(95ed6dcb) SHA1(931706ce3fea630823ce0c79febec5eec0cc623d) )

	ROM_REGION16_BE( 0x100000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "sv1du.bin",  0x000000, 0x80000, CRC(2e561996) SHA1(982158481e5649f21d5c2816fdc80cb725ed1419) )
	ROM_LOAD16_BYTE( "sv1dl.bin",  0x000001, 0x80000, CRC(495fb8dd) SHA1(813d1da4109652008d72b3bdb03032efc5c0c2d5) )

	ROM_REGION32_BE( 0x400000, REGION_USER2, ROMREGION_ERASE )		/* 24bit signed point data */
	ROM_LOAD32_BYTE( "sv1pt0h.bin", 0x000001, 0x80000, CRC(3be21115) SHA1(c9f30353c1216f64199f87cd34e787efd728e739) ) /* most significant */
	ROM_LOAD32_BYTE( "sv1pt0u.bin", 0x000002, 0x80000, CRC(4aacfc42) SHA1(f0e179e057183b41744ca429764f44306f0ce9bf) )
	ROM_LOAD32_BYTE( "sv1pt0l.bin", 0x000003, 0x80000, CRC(6a4dddff) SHA1(9ed182d21d328c6a684ee6658a9dfcf3f3dd8646) ) /* least significant */

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD("sv1voi0.bin",0x000000,0x80000,CRC(7f61bbcf) SHA1(b3b7e66e24d9cb16ebd139237c1e51f5d60c1585) )
	ROM_LOAD("sv1voi1.bin",0x080000,0x80000,CRC(c732e66c) SHA1(14e75dd9bea4055f85eb2bcbf69cf6695a3f7ec4) )
	ROM_LOAD("sv1voi2.bin",0x100000,0x80000,CRC(51076298) SHA1(ec52c9ae3029118f3ea3732948d6de28f5fba561) )
	ROM_LOAD("sv1voi3.bin",0x180000,0x80000,CRC(33085ff3) SHA1(0a30b91618c250a5e7bd896a8ceeb3d16da178a9) )
ROM_END

ROM_START( winrun91 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* Master */
	ROM_LOAD16_BYTE( "mpu.3k",  0x000000, 0x20000, CRC(80a0e5be) )
	ROM_LOAD16_BYTE( "mpl.1k",  0x000001, 0x20000, CRC(942172d8) )

	ROM_REGION( 0x080000, REGION_CPU2, 0 ) /* Slave */
	ROM_LOAD16_BYTE( "spu.6b",  0x000000, 0x20000, CRC(0221d4b2) )
	ROM_LOAD16_BYTE( "spl.4b",  0x000001, 0x20000, CRC(288799e2) )

	ROM_REGION( 0x030000, REGION_CPU3, 0 ) /* Sound */
	ROM_LOAD( "snd0.7c",	0x00c000, 0x004000, CRC(6a321e1e) )
	ROM_CONTINUE(			0x010000, 0x01c000 )
	ROM_RELOAD(				0x010000, 0x020000 )

	ROM_REGION( 0x010000, REGION_CPU4, 0 ) /* I/O MCU */
	ROM_LOAD( "sys2mcpu.bin",  0x000000, 0x002000, CRC(a342a97e) SHA1(2c420d34dba21e409bf78ddca710fc7de65a6642) )
	ROM_LOAD( "sys2c65c.bin",  0x008000, 0x008000, CRC(a5b2a4ff) SHA1(068bdfcc71a5e83706e8b23330691973c1c214dc) )

	ROM_REGION16_BE( 0x100000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "d0u.3a",  0x000000, 0x20000, CRC(dcb27da5) )
	ROM_LOAD16_BYTE( "d0l.1a",  0x000001, 0x20000, CRC(f692a8f3) )
	ROM_LOAD16_BYTE( "d1u.3b",  0x000000, 0x20000, CRC(ac2afd1b) )
	ROM_LOAD16_BYTE( "d1l.1b",  0x000001, 0x20000, CRC(ebb51af1) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "gd0l.3p",  0x00000, 0x40000, CRC(9a29500e) )
	ROM_LOAD( "gd1u.1s",  0x40000, 0x40000, CRC(17e5a61c) )
	ROM_LOAD( "gd0u.1p",  0x80000, 0x40000, CRC(33f5a19b) )
	ROM_LOAD( "gd1l.3s",  0xc0000, 0x40000, CRC(64df59a2) )

	ROM_REGION32_BE( 0x400000, REGION_USER2, ROMREGION_ERASE ) /* 16 bit point data? */
	ROM_LOAD32_BYTE( "pt0u.8j", 0x00002, 0x20000, CRC(abf512a6) )
	ROM_LOAD32_BYTE( "pt0l.8d", 0x00003, 0x20000, CRC(ac8d468c) )
	ROM_LOAD32_BYTE( "pt1u.8l", 0x80002, 0x20000, CRC(7e5dab74) )
	ROM_LOAD32_BYTE( "pt1l.8e", 0x80003, 0x20000, CRC(38a54ec5) )

	ROM_REGION16_BE( 0x80000, REGION_USER3, 0 ) /* ? */
	ROM_LOAD( "gp0l.3j",  0x00000, 0x20000, CRC(5c18f596) )
	ROM_LOAD( "gp0u.1j",  0x00001, 0x20000, CRC(f5469a29) )
	ROM_LOAD( "gp1l.3l",  0x40000, 0x20000, CRC(96c2463c) )
	ROM_LOAD( "gp1u.1l",  0x40001, 0x20000, CRC(146ab6b8) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* sound samples */
	ROM_LOAD("avo1.11c",0x000000,0x80000,CRC(9fb33af3) )
	ROM_LOAD("avo3.11e",0x080000,0x80000,CRC(76e22f92) )
ROM_END

static void namcos21_init( int game_type )
{
	data32_t *pMem = (data32_t *)memory_region(REGION_USER2);
	int numWords = memory_region_length(REGION_USER2)/4;
	int i;

	/* sign-extend the 24 bit point data into a more-convenient 32 bit format */
	for( i=0; i<numWords; i++ )
	{
		if( pMem[i] &  0x00800000 )
		{
			pMem[i] |= 0xff000000;
		}
	}
	namcos2_gametype = game_type;
	mpDataROM = (data16_t *)memory_region( REGION_USER1 );
} /* namcos21_init */

static DRIVER_INIT( winrun )
{
	namcos21_init( NAMCOS21_WINRUN91 );
}

static DRIVER_INIT( aircombt )
{
#if 0
	/* replace first four tests of aircombj with special "hidden" tests */
	data16_t *pMem = (data16_t *)memory_region( REGION_CPU1 );
	pMem[0x2a32/2] = 0x90;
	pMem[0x2a34/2] = 0x94;
	pMem[0x2a36/2] = 0x88;
	pMem[0x2a38/2] = 0x8c;
#endif

	namcos21_init( NAMCOS21_AIRCOMBAT );
}

DRIVER_INIT( starblad )
{
	namcos21_init( NAMCOS21_STARBLADE );
}


DRIVER_INIT( cybsled )
{
	namcos21_init( NAMCOS21_CYBERSLED );
}

DRIVER_INIT( solvalou )
{
	data16_t *pMem = (data16_t *)memory_region( REGION_CPU1 );

	/* patch out DSP memtest/clear */
	pMem[0x1FD7C/2] = 0x4E71;
	pMem[0x1FD7E/2] = 0x4E71;

	/* patch out DSP test/populate */
	pMem[0x1FDA6/2] = 0x4E71;
	pMem[0x1FDA8/2] = 0x4E71;

	namcos21_init( NAMCOS21_SOLVALOU );
}

/*************************************************************/
/*															 */
/*	NAMCO SYSTEM 21 INPUT PORTS								 */
/*															 */
/*************************************************************/

INPUT_PORTS_START( default )
	PORT_START		/* 63B05Z0 - PORT B */
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* 63B05Z0 - PORT C & SCI */
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_DIPNAME( 0x40, 0x40, "Test Switch")
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START      /* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 1 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X,  15, 10, 0x60, 0x9f )
	PORT_START      /* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 2 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y,  20, 10, 0x60, 0x9f )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 5 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 6 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 7 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* 63B05Z0 - PORT H */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* 63B05Z0 - $2000 DIP SW */
	PORT_DIPNAME( 0x01, 0x01, "DSW1 (Test Mode)")
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DSW2")
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DSW3")
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DSW4")
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DSW5")
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DSW6")
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DSW7")
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "DSW8")
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* 63B05Z0 - $3000 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - $3001 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - $3002 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* 63B05Z0 - $3003 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( aircombt )
	PORT_START		/* IN#0: 63B05Z0 - PORT B */
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START		/* IN#1: 63B05Z0 - PORT C & SCI */
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_DIPNAME( 0x40, 0x40, "Test Switch")
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START		/* IN#2: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* IN#3: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 1 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X|IPF_CENTER, 100, 4, 0x00, 0xff )

	PORT_START		/* IN#4: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 2 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y|IPF_CENTER, 100, 4, 0x00, 0xff )

	PORT_START		/* IN#5: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 3 */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X|IPF_CENTER|IPF_PLAYER2, 100, 4, 0x00, 0xff )

	PORT_START		/* IN#6: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* IN#7: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 5 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* IN#8: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 6 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* IN#9: 63B05Z0 - 8 CHANNEL ANALOG - CHANNEL 7 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* IN#10: 63B05Z0 - PORT H */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 ) ///???
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) // prev color
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON5 ) // ???next color
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* IN#11: 63B05Z0 - $2000 DIP SW */
	PORT_DIPNAME( 0x01, 0x01, "DSW1 (Test Mode)")
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DSW2")
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DSW3")
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "DSW4")
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "DSW5")
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "DSW6")
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "DSW7")
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "DSW8")
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START		/* IN#12: 63B05Z0 - $3000 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* IN#13: 63B05Z0 - $3001 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* IN#14: 63B05Z0 - $3002 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START		/* IN#15: 63B05Z0 - $3003 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


/*    YEAR, NAME,     PARENT,   MACHINE,          INPUT,        INIT,     MONITOR,    COMPANY, FULLNAME,	    	FLAGS */
GAMEX( 1992, aircombj, 0,	    poly_c140_typeB,  aircombt, 	aircombt, ROT0,		  "Namco", "Air Combat (Japan)",	GAME_NOT_WORKING ) /* mostly working */
GAMEX( 1992, aircombu, aircombj,poly_c140_typeB,  aircombt, 	aircombt, ROT0, 	  "Namco", "Air Combat (US)",	GAME_NOT_WORKING ) /* mostly working */
GAMEX( 1993, cybsled,  0,       poly_c140_typeA,  default,      cybsled,  ROT0,       "Namco", "Cyber Sled",		GAME_NOT_WORKING ) /* mostly working */
/* 199?, Driver's Eyes */
/* 1992, ShimDrive */
GAMEX( 1991, solvalou, 0, 	    poly_c140_typeA,  default,	    solvalou, ROT0, 	  "Namco", "Solvalou (Japan)",	GAME_IMPERFECT_GRAPHICS ) /* working and playable */
GAMEX( 1991, starblad, 0, 	    poly_c140_typeA,  default,  	starblad, ROT0, 	  "Namco", "Starblade",			GAME_IMPERFECT_GRAPHICS ) /* working and playable */
/* 1988, Winning Run */
/* 1989, Winning Run Suzuka Grand Prix */
GAMEX( 1991, winrun91, 0, 	    poly_c140_typeB,  default,	    winrun,	  ROT0, 	  "Namco", "Winning Run 91", 	GAME_NOT_WORKING ) /* not working */
