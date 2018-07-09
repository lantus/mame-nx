/***************************************************************************

						  -= Power Instinct =-
							(C) 1993 Atlus

				driver by	Luca Elia (l.elia@tin.it)

CPU:	MC68000
Sound:	OKIM6295

- Note:	To enter test mode press F2 (Test)
		Use 9 (Service Coin) to change page.

TODO:
- sprites flip y (not used by the game)


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Variables that vidhrdw has access to */

/* Variables defined in vidhrdw */
extern data16_t *powerins_vram_0, *powerins_vctrl_0;
extern data16_t *powerins_vram_1, *powerins_vctrl_1;

/* Functions defined in vidhrdw */

WRITE16_HANDLER( powerins_flipscreen_w );
WRITE16_HANDLER( powerins_tilebank_w );

WRITE16_HANDLER( powerins_paletteram16_w );

WRITE16_HANDLER( powerins_vram_0_w );
WRITE16_HANDLER( powerins_vram_1_w );

VIDEO_START( powerins );
VIDEO_UPDATE( powerins );


/***************************************************************************

								Memory Maps

***************************************************************************/

static int oki_bank;

WRITE16_HANDLER( powerins_okibank_w )
{
	if (ACCESSING_LSB)
	{
		unsigned char *RAM = memory_region(REGION_SOUND1);
		int new_bank = data & 0x7;

		if (new_bank != oki_bank)
		{
			oki_bank = new_bank;
			memcpy(&RAM[0x30000],&RAM[0x40000 + 0x10000*new_bank],0x10000);
		}
	}
}


static MEMORY_READ16_START( powerins_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM					},	// ROM
	{ 0x100000, 0x100001, input_port_0_word_r		},	// Coins + Start Buttons
	{ 0x100002, 0x100003, input_port_1_word_r		},	// P1 + P2
	{ 0x100008, 0x100009, input_port_2_word_r		},	// DSW 1
	{ 0x10000a, 0x10000b, input_port_3_word_r		},	// DSW 2
	{ 0x10003e, 0x10003f, OKIM6295_status_0_lsb_r	},	// OKI Status
	{ 0x120000, 0x120fff, MRA16_RAM					},	// Palette
/**/{ 0x130000, 0x130007, MRA16_RAM					},	// VRAM 0 Control
	{ 0x140000, 0x143fff, MRA16_RAM					},	// VRAM 0
	{ 0x170000, 0x170fff, MRA16_RAM					},	// VRAM 1
	{ 0x180000, 0x18ffff, MRA16_RAM					},	// RAM + Sprites
MEMORY_END

static MEMORY_WRITE16_START( powerins_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM								},	// ROM
	{ 0x100014, 0x100015, powerins_flipscreen_w					},	// Flip Screen
	{ 0x100016, 0x100017, MWA16_NOP								},	// ? always 1
	{ 0x100018, 0x100019, powerins_tilebank_w					},	// Tiles Banking (VRAM 0)
//	{ 0x10001e, 0x10001f, MWA16_NOP								},	//
	{ 0x100030, 0x100031, powerins_okibank_w					},	// Sound
	{ 0x10003e, 0x10003f, OKIM6295_data_0_lsb_w					},	//
	{ 0x120000, 0x120fff, powerins_paletteram16_w, &paletteram16	},	// Palette
	{ 0x130000, 0x130007, MWA16_RAM, &powerins_vctrl_0			},	// VRAM 0 Control
	{ 0x140000, 0x143fff, powerins_vram_0_w, &powerins_vram_0	},	// VRAM 0
	{ 0x170000, 0x170fff, powerins_vram_1_w, &powerins_vram_1	},	// VRAM 1
	{ 0x171000, 0x171fff, powerins_vram_1_w						},	// Mirror of VRAM 1?
	{ 0x180000, 0x18ffff, MWA16_RAM, &spriteram16				},	// RAM + Sprites
MEMORY_END

/* There is an hidden test mode screen (set 18ff08 to 4 during test mode)
   that calls the data writtent to $10001e "sound code".
   This is a bootleg, so the original may have a sound CPU */



/***************************************************************************

								Input Ports

***************************************************************************/

INPUT_PORTS_START( powerins )
	PORT_START	// IN0 - $100000 - Coins
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN1 - $100002 - Player 1 & 2
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	 | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4        | IPF_PLAYER1 )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4        | IPF_PLAYER2 )

	PORT_START	// IN2 - $100008 - DSW 1
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x000e, 0x000e, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 2C_1C ) )
//	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_1C ) ) /* 2C to start, 1C to continue */
	PORT_DIPSETTING(      0x000e, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x000a, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0070, 0x0070, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 2C_1C ) )
//	PORT_DIPSETTING(      0x0000, DEF_STR( 2C_1C ) ) /* 2C to start, 1C to continue */
	PORT_DIPSETTING(      0x0070, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0050, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On )  )

	PORT_START	// IN3 - $10000a - DSW 2
	PORT_DIPNAME( 0x0001, 0x0001, "Coin Chutes" )
	PORT_DIPSETTING(      0x0001, "1 Chute" )
	PORT_DIPSETTING(      0x0000, "2 Chutes" )
	PORT_DIPNAME( 0x0002, 0x0002, "Join In Mode" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
/*
	In "Join In" mode, a second player can join even if one player has aready
	begun to play.  Please refer to chart below:

	Join In Mode	Credit			Join In		Game Over
	-----------------------------------------------------------------------------------------------
	Join In ON	1C per Player		Anytime		Winner of VS Plays Computer
	Join In OFF	1C = VS Mode 2 players	Cannot		After win VS Game Over for both players

*/
	PORT_DIPNAME( 0x0004, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Blood Color" )
	PORT_DIPSETTING(      0x0010, "Red" )
	PORT_DIPSETTING(      0x0000, "Blue" )
	PORT_DIPNAME( 0x0020, 0x0020, "Game Time" )
	PORT_DIPSETTING(      0x0020, "Normal" )
	PORT_DIPSETTING(      0x0000, "Short" )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0040, "Easy" )
	PORT_DIPSETTING(      0x00c0, "Normal" )
	PORT_DIPSETTING(      0x0080, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
INPUT_PORTS_END



/***************************************************************************

								Graphics Layouts

***************************************************************************/

/* 8x8x4 tiles */
static struct GfxLayout layout_8x8x4 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{0,1,2,3},
	{0*4,1*4,2*4,3*4,4*4,5*4,6*4,7*4},
	{0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32},
	8*8*4
};


/* 16x16x4 tiles (made of four 8x8 tiles) */
static struct GfxLayout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{0,1,2,3},
	{0*4,1*4,2*4,3*4,4*4,5*4,6*4,7*4,
	 128*4,129*4,130*4,131*4,132*4,133*4,134*4,135*4},
	{0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32,
	 8*32,9*32,10*32,11*32,12*32,13*32,14*32,15*32},
	16*16*4
};


/* 16x16x4 tiles (made of four 8x8 tiles). The bytes are swapped */
static struct GfxLayout layout_16x16x4_swap =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{0,1,2,3},
	{2*4,3*4,0*4,1*4,6*4,7*4,4*4,5*4,
	 130*4,131*4,128*4,129*4,134*4,135*4,132*4,133*4},
	{0*32,1*32,2*32,3*32,4*32,5*32,6*32,7*32,
	 8*32,9*32,10*32,11*32,12*32,13*32,14*32,15*32},
	16*16*4
};


static struct GfxDecodeInfo powerins_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4,      0x000, 0x20 }, // [0] Tiles
	{ REGION_GFX2, 0, &layout_8x8x4,        0x200, 0x10 }, // [1] Tiles
	{ REGION_GFX3, 0, &layout_16x16x4_swap, 0x400, 0x40 }, // [2] Sprites
	{ -1 }
};






/***************************************************************************

								Machine Drivers

***************************************************************************/

MACHINE_INIT( powerins )
{
	oki_bank = -1;	// samples bank "unitialised"
}

static struct OKIM6295interface powerins_okim6295_interface =
{
	1,
	{ 6000 },		/* ? */
	{ REGION_SOUND1 },
	{ 100 }
};

static MACHINE_DRIVER_START( powerins )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)	/* ? (it affects the game's speed!) */
	MDRV_CPU_MEMORY(powerins_readmem,powerins_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(powerins)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 320-1, 0+16, 256-16-1)
	MDRV_GFXDECODE(powerins_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(powerins)
	MDRV_VIDEO_UPDATE(powerins)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, powerins_okim6295_interface)
MACHINE_DRIVER_END






/***************************************************************************

								ROMs Loading

***************************************************************************/



/***************************************************************************

								Power Instinct

Location     Device       File ID     Checksum
----------------------------------------------
             27C240        ROM1         4EA1    [ MAIN PROGRAM ]
             27C240        ROM2         FE60    [ PROGRAM DATA ]
             27C010        ROM3         B9F7    [  CHARACTER   ]
             27C040        ROM4         2780    [  BACKGROUND  ]
             27C040        ROM5         98E0    [   PCM DATA   ]
            23C1600        ROM6         D9E9    [  BACKGROUND  ]
            23C1600        ROM7         8B04    [  MOTION OBJ  ]
            23C1600        ROM8         54B2    [  MOTION OBJ  ]
            23C1600        ROM9         C7C8    [  MOTION OBJ  ]
            23C1600        ROM10        852A    [  MOTION OBJ  ]

Notes:  This archive is of a bootleg version
        The main program is encrypted, using PLDs

Brief hardware overview
-----------------------

Main processor  -  68000
                -  TPC1020AFN-084C (CPLD)

Sound processor -  Main processor
                -  K-665-9249      (M6295)

***************************************************************************/

ROM_START( powerins )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD_SWAP( "rom1", 0x000000, 0x080000, CRC(b86c84d6) SHA1(2ec0933130925dfae859ea6abe62a8c92385aee8) )
	ROM_LOAD16_WORD_SWAP( "rom2", 0x080000, 0x080000, CRC(d3d7a782) SHA1(7846de0ebb09bd9b2534cd451ff9aa5175e60647) )

	ROM_REGION( 0x280000, REGION_GFX1, ROMREGION_DISPOSE )	/* Layer 0 */
	ROM_LOAD( "rom6",  0x000000, 0x200000, CRC(b6c10f80) SHA1(feece0aeaa01a455d0c4885a3699f8bda14fe00f) )
	ROM_LOAD( "rom4",  0x200000, 0x080000, CRC(2dd76149) SHA1(975e4d371fdfbbd9a568da4d4c91ffd3f0ae636e) )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 1 */
	ROM_LOAD( "rom3",  0x000000, 0x020000, CRC(6a579ee0) SHA1(438e87b930e068e0cf7352e614a14049ebde6b8a) )

	ROM_REGION( 0x800000, REGION_GFX3, ROMREGION_DISPOSE )	/* Sprites */
	ROM_LOAD( "rom10", 0x000000, 0x200000, CRC(efad50e8) SHA1(89e8c307b927e987a32d22ab4ab7f3be037cca03) )
	ROM_LOAD( "rom9",  0x200000, 0x200000, CRC(08229592) SHA1(759679e89832b475adfdc783630d9ee2c105b0f3) )
	ROM_LOAD( "rom8",  0x400000, 0x200000, CRC(b02fdd6d) SHA1(1e2c52b4e9999f0b564fcf13ff41b097ad7d0c39) )
	ROM_LOAD( "rom7",  0x600000, 0x200000, CRC(92ab9996) SHA1(915ec8f383cc3652c3816a9b56ee54e22e104a5c) )

	ROM_REGION( 0x090000, REGION_SOUND1, 0 )	/* 8 bit adpcm (banked) */
	ROM_LOAD( "rom5", 0x000000, 0x030000, CRC(88579c8f) SHA1(13083934ab294c9b08d3e36f55c00a6a2e5a0507) )
	ROM_CONTINUE(     0x040000, 0x050000 )
ROM_END


GAME( 1993, powerins, 0, powerins, powerins, 0, ROT0, "Atlus", "Power Instinct (USA bootleg)" )
