/* Hanaroku */

/*
TODO:
- colour decoding might not be perfect
- Background color should be green, but current handling might be wrong.
- some unknown sprite attributes, sprite flipping in flip screen needed
- don't know what to do when the jackpot is displayed (missing controls ?)
- according to the board pic, there should be one more 4-switches dip
  switch bank, and probably some NVRAM because there's a battery.
*/

#include "driver.h"


/* vidhrdw */


data8_t* hanaroku_vidram1;
data8_t* hanaroku_vidram2;
data8_t* hanaroku_vidram3;


PALETTE_INIT( hanaroku )
{
	int i;
	int r,g,b;

	for (i = 0; i < 0x200; i++)
	{
		b = (color_prom[i*2+1] & 0x1f);
		g = ((color_prom[i*2+1] & 0xe0) | ( (color_prom[i*2+0]& 0x03) <<8)  ) >> 5;
		r = (color_prom[i*2+0]&0x7c) >> 2;

		palette_set_color(i,r<<3,g<<3,b<<3);
	}
}


VIDEO_START(hanaroku)
{
	return 0;
}

VIDEO_UPDATE(hanaroku)
{
	int spr;
	const struct GfxElement *gfx = Machine->gfx[0];

	fillbitmap(bitmap, Machine->pens[0x1f0], cliprect);	/* ??? */

	for (spr = 0x1ff; spr >=0x0  ; spr--)
	{
		int tile  = hanaroku_vidram1[0x0000+spr];
		int xpos  = hanaroku_vidram1[0x0200+spr];
		int ypos  = hanaroku_vidram3[0x0000+spr];
		int tile2 = hanaroku_vidram2[0x0000+spr];
		int xpos2 = hanaroku_vidram2[0x0200+spr] & 0x07;
//		int ypos2 = hanaroku_vidram2[0x0600+spr];
		int colr  = (hanaroku_vidram2[0x0200+spr] & 0xf8) >>3;

		tile = tile | (tile2 << 8);
		xpos = xpos | (xpos2 << 8);
//		ypos = ypos | (ypos2 << 8);

		drawgfx(bitmap,gfx,tile,colr,0,0,xpos,242-ypos,cliprect,TRANSPARENCY_PEN,0);
	}
}


/* main cpu */

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9000, 0x97ff, MRA_RAM },
	{ 0xa000, 0xa1ff, MRA_RAM },
	{ 0xc000, 0xc3ff, MRA_RAM },
	{ 0xc400, 0xc4ff, MRA_RAM },
	{ 0xd000, 0xd000, AY8910_read_port_0_r },
	{ 0xe000, 0xe000, input_port_0_r },
	{ 0xe001, 0xe001, input_port_1_r },
	{ 0xe002, 0xe002, input_port_2_r },
	{ 0xe004, 0xe004, input_port_5_r },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM, &hanaroku_vidram1 },
	{ 0x9000, 0x97ff, MWA_RAM, &hanaroku_vidram2 },
	{ 0xa000, 0xa1ff, MWA_RAM, &hanaroku_vidram3 },
//	{ 0xa200, 0xa2ff, MWA_NOP },			// written once during P.O.S.T. - unknown purpose
	{ 0xc000, 0xc3ff, MWA_RAM },			// main ram
	{ 0xc400, 0xc4ff, MWA_RAM },			// ?
	{ 0xb000, 0xb000, MWA_NOP },			// written with 0x40 (but not read back)
	{ 0xd000, 0xd000, AY8910_control_port_0_w },
	{ 0xd001, 0xd001, AY8910_write_port_0_w },
//	{ 0xe000, 0xe000, MWA_NOP },			// unneeded IMO
//	{ 0xe002, 0xe002, MWA_NOP },			// unneeded IMO
//	{ 0xe004, 0xe004, MWA_NOP },
MEMORY_END


/* Desceiption of the buttons don't match exactly what is in the "test mode"  */
/* Buttons 9 and 10 only have an effect when "Game Mode" is set to "1" or "2" */

INPUT_PORTS_START( hanaroku )
	PORT_START	/* IN0	(0xe000) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )	// adds n credits depending on "Coinage" Dip Switch
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )	// adds 5 credits
//	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )	// adds 10 credits (not in this game)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE1, "1/2 D-Up",     KEYCODE_H,        IP_JOY_DEFAULT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	// [0xc06a] = 0x03
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_SERVICE2, "Analyser",     IP_KEY_DEFAULT,   IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_SERVICE3, "Data Clear",   IP_KEY_DEFAULT,   IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_START2,   "Bet",          IP_KEY_DEFAULT,   IP_JOY_DEFAULT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN1	(0xe001) */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON1,  "Card 1",       IP_KEY_DEFAULT,   IP_JOY_DEFAULT )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON2,  "Card 2",       IP_KEY_DEFAULT,   IP_JOY_DEFAULT )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON3,  "Card 3",       IP_KEY_DEFAULT,   IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON4,  "Card 4",       IP_KEY_DEFAULT,   IP_JOY_DEFAULT )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON5,  "Card 5",       IP_KEY_DEFAULT,   IP_JOY_DEFAULT )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON6,  "Card 6",       IP_KEY_DEFAULT,   IP_JOY_DEFAULT )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_BUTTON7,  DEF_STR( Yes ), KEYCODE_Y,        IP_JOY_DEFAULT )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_BUTTON8,  DEF_STR( No  ), KEYCODE_N,        IP_JOY_DEFAULT )

	PORT_START	/* IN2	(0xe002) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	// "Reset" (jump to 0x0000)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON9,  "Sub 1 Credit", KEYCODE_RSHIFT,   IP_JOY_DEFAULT )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON10, "Collect",      KEYCODE_RCONTROL, IP_JOY_DEFAULT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW1	(0xd000 - Port A) */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW2	(0xd000 - Port B) */
#if 0		// code at 0x0283 NEVER executed !
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x03, "0" )
	PORT_DIPSETTING(    0x02, "25" )
	PORT_DIPSETTING(    0x01, "50" )
	PORT_DIPSETTING(    0x00, "100" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x0c, "0" )
	PORT_DIPSETTING(    0x08, "25" )
	PORT_DIPSETTING(    0x04, "50" )
	PORT_DIPSETTING(    0x00, "100" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x30, "0" )
	PORT_DIPSETTING(    0x20, "10" )
	PORT_DIPSETTING(    0x10, "20" )
	PORT_DIPSETTING(    0x00, "40" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0xc0, "0" )
	PORT_DIPSETTING(    0x80, "10" )
	PORT_DIPSETTING(    0x40, "20" )
	PORT_DIPSETTING(    0x00, "40" )
#else
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
#endif

	PORT_START	/* DSW3	(0xe004) */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )		// Stored at 0xc028
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x00, "1 Coin/10 Credits" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Flip_Screen ) )	// Stored at 0xc03a
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )		// Stored at 0xc078
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x20, "Game Mode" )			// Stored at 0xc02e
	PORT_DIPSETTING(    0x30, "Mode 0" )			// Collect OFF
	PORT_DIPSETTING(    0x20, "Mode 1" )			// Collect ON (code at 0x36ea)
	PORT_DIPSETTING(    0x10, "Mode 2" )			// Collect ON (code at 0x3728)
	PORT_DIPSETTING(    0x00, "Mode 3" )			// No credit counter
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END


static struct GfxLayout hanaroku_charlayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4),RGN_FRAC(2,4),RGN_FRAC(1,4),RGN_FRAC(0,4) },
	{ 0,1,2,3,4,5,6,7,
		64,65,66,67,68,69,70,71},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		128+0*8,128+1*8,128+2*8,128+3*8,128+4*8,128+5*8,128+6*8,128+7*8 },
	16*16
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &hanaroku_charlayout,   0, 32  },

	{ -1 } /* end of array */
};


static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1500000,	/* 1.5 MHz ???? */
	{ 50 },
	{ input_port_3_r },
	{ input_port_4_r },
	{ 0 },
	{ 0 }
};


static MACHINE_DRIVER_START( hanaroku )
	MDRV_CPU_ADD(Z80,6000000)		 /* ? MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0, 48*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x200)

	MDRV_PALETTE_INIT(hanaroku)
	MDRV_VIDEO_START(hanaroku)
	MDRV_VIDEO_UPDATE(hanaroku)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END


ROM_START( hanaroku )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* z80 code */
	ROM_LOAD( "zc5_1a.u02",  0x00000, 0x08000, CRC(9e3b62ce) SHA1(81aee570b67950c21ab3c8f9235dd383529b34d5) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) /* tiles */
	ROM_LOAD( "zc0_002.u14",  0x00000, 0x08000, CRC(76adab7f) SHA1(6efbe52ae4a1d15fe93bd05058546bf146a64154) )
	ROM_LOAD( "zc0_003.u15",  0x08000, 0x08000, CRC(c208e64b) SHA1(0bc226c39331bb2e1d4d8f756199ceec85c28f28) )
	ROM_LOAD( "zc0_004.u16",  0x10000, 0x08000, CRC(e8a46ee4) SHA1(09cac230c1c49cb282f540b1608ad33b1cc1a943) )
	ROM_LOAD( "zc0_005.u17",  0x18000, 0x08000, CRC(7ad160a5) SHA1(c897fbe4a7c2a2f352333131dfd1a76e176f0ed8) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 ) /* colour */
	ROM_LOAD16_BYTE( "zc0_006.u21",  0x0000, 0x0200, CRC(8e8fbc30) SHA1(7075521bbd790c46c58d9e408b0d7d6a42ed00bc) )
	ROM_LOAD16_BYTE( "zc0_007.u22",  0x0001, 0x0200, CRC(67225de1) SHA1(98322e71d93d247a67fb4e52edad6c6c32a603d8) )
ROM_END


GAMEX( 1988, hanaroku, 0,        hanaroku, hanaroku, 0, ROT0, "Alba", "Hanaroku", GAME_NO_COCKTAIL | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_COLORS )
