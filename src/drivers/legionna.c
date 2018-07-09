/***************************************************************************

Legionnaire (c) Tad 1992
-----------

David Graves

Made from MAME D-con and Toki drivers (by Bryan McPhail, Jarek Parchanski)


Heated Barrel looks like a minor revision of the Legionnaire
hardware. It has a graphics banking facility, which doubles the 0xfff
different tiles available for use in the foreground layer.


Legionnaire BK3 charsets
------------------------

The GFX roms contain two odd sections of 256 16x16 tiles marked as BK3.
These need to be brought together and decoded as a single section of
0x200 tiles.

The 0x104000 area appears to be extra paletteram?


TODO
----

Unemulated protection messes up both games.

Seibu sound system mentioned in log.


Legionnaire
-----------

Foreground tiles screwy (screen after character selection screen).

Need 16 px off top of vis area?


Heated Barrel
-------------

Big problems with layers not being cleared, especially the text
layer. There may be a write to the COP controlling layer clearance?

Ends with Access violation when you die on round 1 boss. A lot of
non-existent area reads in log - maybe because of bad reads from
the COP.


Preliminary COP MCU memory map
------------------------------

0x400-0x5ff   Protection related
0x600-0x6ff   Includes standard screen control words
0x700-0x7ff   Includes standard Seibu sound system


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "sndhrdw/seibu.h"

WRITE16_HANDLER( legionna_background_w );
WRITE16_HANDLER( legionna_foreground_w );
WRITE16_HANDLER( legionna_midground_w );
WRITE16_HANDLER( legionna_text_w );
WRITE16_HANDLER( legionna_control_w );

VIDEO_START( legionna );
VIDEO_UPDATE( legionna );
void heatbrl_setgfxbank(UINT16 data);

extern data16_t *legionna_back_data,*legionna_fore_data,*legionna_mid_data,*legionna_scrollram16,*legionna_textram;
static data16_t *mcu_ram;

static WRITE16_HANDLER( legionna_paletteram16_w )	/* xBBBBxRRRRxGGGGx */
{
	int a,r,g,b;
	COMBINE_DATA(&paletteram16[offset]);

	a = paletteram16[offset];

	r = (a >> 1) & 0x0f;
	g = (a >> 6) & 0x0f;
	b = (a >> 11) & 0x0f;

	r = (r << 4) | r;
	g = (g << 4) | g;
	b = (b << 4) | b;

	palette_set_color(offset,r,g,b);
}

/* Mcu reads in attract in Legionnaire game demo

Guess the 0x400-0x5ff area of the COP is protection related.

CPU0 PC 0032a2 unknown MCU write offset: 0260 data: 9c6c
CPU0 PC 0032a8 unknown MCU write offset: 0250 data: 0010
CPU0 PC 0032c8 unknown MCU write offset: 0261 data: 987c
CPU0 PC 0032ce unknown MCU write offset: 0251 data: 0010
CPU0 PC 003546 unknown MCU write offset: 0262 data: 02c4
CPU0 PC 00354a unknown MCU write offset: 0252 data: 0004
CPU0 PC 00355c unknown MCU write offset: 0263 data: 0000
CPU0 PC 003560 unknown MCU write offset: 0253 data: 0004
CPU0 PC 003568 unknown MCU write offset: 0280 data: a180
CPU0 PC 00356e unknown MCU write offset: 0280 data: a980
CPU0 PC 003574 unknown MCU write offset: 0280 data: b100
CPU0 PC 00357a unknown MCU write offset: 0280 data: b900
CPU0 PC 003580 unknown MCU read offset: 02c4
CPU0 PC 003588 unknown MCU read offset: 02c2
CPU0 PC 003594 unknown MCU read offset: 02c1
CPU0 PC 0035a0 unknown MCU read offset: 02c3
CPU0 PC 0032a2 unknown MCU write offset: 0260 data: 9c6c
CPU0 PC 0032a8 unknown MCU write offset: 0250 data: 0010
CPU0 PC 0032c8 unknown MCU write offset: 0261 data: 987c
CPU0 PC 0032ce unknown MCU write offset: 0251 data: 0010
CPU0 PC 003422 unknown MCU write offset: 0280 data: 138e
CPU0 PC 003428 unknown MCU read offset: 02da
CPU0 PC 00342e unknown MCU read offset: 02d8
CPU0 PC 00346c unknown MCU write offset: 0280 data: 3bb0
CPU0 PC 0032a2 unknown MCU write offset: 0260 data: 987c
CPU0 PC 0032a8 unknown MCU write offset: 0250 data: 0010
CPU0 PC 003306 unknown MCU write offset: 0280 data: 8100
CPU0 PC 00330c unknown MCU write offset: 0280 data: 8900


Mcu reads in attract in Heated Barrel game demo (note
partial similarity)

(i) This sequence repeats a number of times early on:

CPU0 PC 0085b4 unknown MCU write offset: 0210 data: 0064
CPU0 PC 0085ba unknown MCU write offset: 0211 data: 0000
CPU0 PC 0085be unknown MCU read offset: 02ca
CPU0 PC 0085ee unknown MCU read offset: 02c9
CPU0 PC 008622 unknown MCU read offset: 02c8

(ii) This happens a few times:

CPU0 PC 0017ac unknown MCU write offset: 0260 data: b6cc
CPU0 PC 0017b2 unknown MCU write offset: 0250 data: 0010
CPU0 PC 0017d2 unknown MCU write offset: 0261 data: babc
CPU0 PC 0017d8 unknown MCU write offset: 0251 data: 0010
CPU0 PC 00192c unknown MCU write offset: 0280 data: 138e
CPU0 PC 001932 unknown MCU read offset: 02da
CPU0 PC 001938 unknown MCU read offset: 02d8
CPU0 PC 001976 unknown MCU write offset: 0280 data: 3bb0
CPU0 PC 0017ac unknown MCU write offset: 0260 data: b6cc
CPU0 PC 0017b2 unknown MCU write offset: 0250 data: 0010
CPU0 PC 0017d2 unknown MCU write offset: 0261 data: bb9c
CPU0 PC 0017d8 unknown MCU write offset: 0251 data: 0010
CPU0 PC 00192c unknown MCU write offset: 0280 data: 138e
CPU0 PC 001932 unknown MCU read offset: 02da
CPU0 PC 001938 unknown MCU read offset: 02d8
CPU0 PC 001976 unknown MCU write offset: 0280 data: 3bb0

(iii) Later on this happens a lot:

CPU0 PC 0017ac unknown MCU write offset: 0260 data: c61c
CPU0 PC 0017b2 unknown MCU write offset: 0250 data: 0010
CPU0 PC 0017d2 unknown MCU write offset: 0261 data: bb9c
CPU0 PC 0017d8 unknown MCU write offset: 0251 data: 0010
CPU0 PC 001a5c unknown MCU write offset: 0262 data: aa48
CPU0 PC 001a62 unknown MCU write offset: 0252 data: 0003
CPU0 PC 001a7c unknown MCU write offset: 0263 data: a0c8
CPU0 PC 001a82 unknown MCU write offset: 0253 data: 0003
CPU0 PC 001a86 unknown MCU write offset: 0280 data: a100
CPU0 PC 001a8c unknown MCU write offset: 0280 data: b080
CPU0 PC 001a92 unknown MCU write offset: 0280 data: a900
CPU0 PC 001a98 unknown MCU write offset: 0280 data: b880
CPU0 PC 001a9e unknown MCU read offset: 02c0
CPU0 PC 001aa6 unknown MCU read offset: 02c2
CPU0 PC 001ab2 unknown MCU read offset: 02c1

*/

static READ16_HANDLER( mcu_r )
{
	switch (offset)
	{
		/* Protection is not understood */

		case (0x470/2):	/* read PC $110a, could be some sort of control word:
				sometimes a bit is changed then it's poked back in... */
			return (rand() &0xffff);

		case (0x582/2):	/* read PC $3594 */
			return (rand() &0xffff);

		case (0x584/2):	/* read PC $3588 */
			return (rand() &0xffff);

		case (0x586/2):	/* read PC $35a0 */
			return (rand() &0xffff);

		case (0x588/2):	/* read PC $3580 */
			return (rand() &0xffff);

		case (0x5b0/2):	/* bit 15 is branched on a few times in the $3300 area */
			return (rand() &0xffff);

		case (0x5b4/2):	/* read and stored in ram before +0x5b0 bit 15 tested */
			return (rand() &0xffff);

		/* Non-protection reads */

		case (0x708/2):	/* seibu sound: these three around $b10 on */
			return seibu_main_word_r(2,0);

		case (0x70c/2):
			return seibu_main_word_r(3,0);

		case (0x714/2):
			return seibu_main_word_r(5,0);

		/* Inputs */

		case (0x740/2):	/* code at $b00 sticks waiting for bit 6 hi */
			return input_port_1_word_r(0,0);

		case (0x744/2):
			return input_port_2_word_r(0,0);

		case (0x748/2):	/* code at $f4a reads this 4 times in _weird_ fashion */
			return input_port_0_word_r(0,0);

		case (0x74c/2):
			return input_port_3_word_r(0,0);

	}
logerror("CPU0 PC %06x unknown MCU read offset: %04x\n",activecpu_get_previouspc(),offset);

	return mcu_ram[offset];
}

static WRITE16_HANDLER( mcu_w )
{
	COMBINE_DATA(&mcu_ram[offset]);

	switch (offset)
	{
		// 61a bit 0 is flipscreen
		// 61c probably layer disables, like Dcon
		// 620 - 62a scroll control;  is there a layer priority switch...?

		case (0x620/2):
		{
			legionna_scrollram16[0] = mcu_ram[offset];
			break;
		}
		case (0x622/2):
		{
			legionna_scrollram16[1] = mcu_ram[offset];
			break;
		}
		case (0x624/2):
		{
			legionna_scrollram16[2] = mcu_ram[offset];
			break;
		}
		case (0x626/2):
		{
			legionna_scrollram16[3] = mcu_ram[offset];
			break;
		}
		case (0x628/2):
		{
			legionna_scrollram16[4] = mcu_ram[offset];
			break;
		}
		case (0x62a/2):
		{
			legionna_scrollram16[5] = mcu_ram[offset];
			break;
		}
		case (0x700/2):	/* seibu(0) */
		{
			seibu_main_word_w(0,mcu_ram[offset],0xff00);
			break;
		}
		case (0x704/2):	/* seibu(1) */
		{
			seibu_main_word_w(1,mcu_ram[offset],0xff00);
			break;
		}
		case (0x710/2):	/* seibu(4) */
		{
			seibu_main_word_w(4,mcu_ram[offset],0xff00);
			break;
		}
		case (0x718/2):	/* seibu(6) */
		{
			seibu_main_word_w(6,mcu_ram[offset],0xff00);
			break;
		}
		default:
logerror("CPU0 PC %06x unknown MCU write offset: %04x data: %04x\n",activecpu_get_previouspc(),offset,data);
	}
}

static READ16_HANDLER( cop2_mcu_r )
{
	switch (offset)
	{
		/* Protection is not understood */

		case (0x580/2):	/* read PC $1a9e */
			return (rand() &0xffff);

		case (0x582/2):	/* read PC $1ab2 */
			return (rand() &0xffff);

		case (0x584/2):	/* read PC $1aa6 */
			return (rand() &0xffff);

		case (0x590/2):	/* read PC $8622 */
			return (rand() &0xffff);

		case (0x592/2):	/* read PC $85ee */
			return (rand() &0xffff);

		case (0x594/2):	/* read PC $85be */
			return (rand() &0xffff);

		case (0x5b0/2):	/* bit 15 is branched on a few times in the $1938 area */
			return (rand() &0xffff);

		case (0x5b4/2):	/* read at $1932 and stored in ram before +0x5b0 bit 15 tested */
			return (rand() &0xffff);

		/* Non-protection reads */

		case (0x7c8/2):	/* seibu sound */
			return seibu_main_word_r(2,0);

		case (0x7cc/2):
			return seibu_main_word_r(3,0);

		case (0x7d4/2):
			return seibu_main_word_r(5,0);

		/* Inputs */

		case (0x740/2):
			return input_port_1_word_r(0,0);

		case (0x744/2):
			return input_port_2_word_r(0,0);

		case (0x748/2):
			return input_port_4_word_r(0,0);

		case (0x74c/2):
			return input_port_3_word_r(0,0);

	}
logerror("CPU0 PC %06x unknown MCU read offset: %04x\n",activecpu_get_previouspc(),offset);

	return mcu_ram[offset];
}

static WRITE16_HANDLER( cop2_mcu_w )
{
	COMBINE_DATA(&mcu_ram[offset]);

	switch (offset)
	{
		case (0x470/2):
		{
			heatbrl_setgfxbank( mcu_ram[offset] );
			break;
		}

		// 65a bit 0 is flipscreen
		// 65c probably layer disables, like Dcon? Used on screen when you press P1-4 start (values 13, 11, 0 seen)
		// 660 - 66a scroll control;  is there a layer priority switch...?

		case (0x660/2):
		{
			legionna_scrollram16[0] = mcu_ram[offset];
			break;
		}
		case (0x662/2):
		{
			legionna_scrollram16[1] = mcu_ram[offset];
			break;
		}
		case (0x664/2):
		{
			legionna_scrollram16[2] = mcu_ram[offset];
			break;
		}
		case (0x666/2):
		{
			legionna_scrollram16[3] = mcu_ram[offset];
			break;
		}
		case (0x668/2):
		{
			legionna_scrollram16[4] = mcu_ram[offset];
			break;
		}
		case (0x66a/2):
		{
			legionna_scrollram16[5] = mcu_ram[offset];
			break;
		}
		case (0x7c0/2):	/* seibu(0) */
		{
			seibu_main_word_w(0,mcu_ram[offset],0xff00);
			break;
		}
		case (0x7c4/2):	/* seibu(1) */
		{
			seibu_main_word_w(1,mcu_ram[offset],0xff00);
			break;
		}
		case (0x7d0/2):	/* seibu(4) */
		{
			seibu_main_word_w(4,mcu_ram[offset],0xff00);
			break;
		}
		case (0x7d8/2):	/* seibu(6) */
		{
			seibu_main_word_w(6,mcu_ram[offset],0xff00);
			break;
		}
		default:
logerror("CPU0 PC %06x unknown MCU write offset: %04x data: %04x\n",activecpu_get_previouspc(),offset,data);
	}
}

/*****************************************************************************/

static MEMORY_READ16_START( legionna_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x1007ff, mcu_r },	/* COP mcu */
	{ 0x101000, 0x1017ff, MRA16_RAM },	/* 32x16 bg layer, 16x16 tiles */
	{ 0x101800, 0x101fff, MRA16_RAM },	/* 32x16 bg layer, 16x16 tiles */
	{ 0x102000, 0x1027ff, MRA16_RAM },	/* 32x16 bg layer, 16x16 tiles */
	{ 0x102800, 0x1037ff, MRA16_RAM },	/* 64x32 text/front layer, 8x8 tiles */

	/* The 4000-4fff area contains PALETTE words and may be extra paletteram? */
	{ 0x104000, 0x104fff, MRA16_RAM },	/* palette mirror ? */
//	{ 0x104000, 0x10401f, MRA16_RAM },	/* debugging... */
//	{ 0x104200, 0x1043ff, MRA16_RAM },	/* ??? */
//	{ 0x104600, 0x1047ff, MRA16_RAM },	/* ??? */
//	{ 0x104800, 0x10481f, MRA16_RAM },	/* ??? */

	{ 0x105000, 0x105fff, MRA16_RAM },	/* spriteram */
	{ 0x106000, 0x106fff, MRA16_RAM },
	{ 0x107000, 0x107fff, MRA16_RAM },	/* palette */
	{ 0x108000, 0x113fff, MRA16_RAM },	/* main ram */
MEMORY_END

static MEMORY_WRITE16_START( legionna_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x1007ff, mcu_w, &mcu_ram },	/* COP mcu */
	{ 0x101000, 0x1017ff, legionna_background_w, &legionna_back_data },
	{ 0x101800, 0x101fff, legionna_foreground_w, &legionna_fore_data },
	{ 0x102000, 0x1027ff, legionna_midground_w,  &legionna_mid_data },
	{ 0x102800, 0x1037ff, legionna_text_w, &legionna_textram },

	/* The 4000-4fff area contains PALETTE words and may be extra paletteram? */
	{ 0x104000, 0x104fff, MWA16_RAM },
//	{ 0x104000, 0x104fff, legionna_paletteram16_w },
//	{ 0x104000, 0x10401f, MWA16_RAM },
//	{ 0x104200, 0x1043ff, MWA16_RAM },
//	{ 0x104600, 0x1047ff, MWA16_RAM },
//	{ 0x104800, 0x10481f, MWA16_RAM },

	{ 0x105000, 0x105fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x106000, 0x106fff, MWA16_RAM },	/* is this used outside inits ?? */
	{ 0x107000, 0x107fff, legionna_paletteram16_w, &paletteram16 },	/* palette xRRRRxGGGGxBBBBx ? */
	{ 0x108000, 0x113fff, MWA16_RAM },
MEMORY_END

static MEMORY_READ16_START( heatbrl_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x1007ff, cop2_mcu_r },	/* COP mcu */
	{ 0x100800, 0x100fff, MRA16_RAM },	/* 32x16 bg layer, 16x16 tiles */
	{ 0x101000, 0x1017ff, MRA16_RAM },	/* 32x16 bg layer, 16x16 tiles */
	{ 0x101800, 0x101fff, MRA16_RAM },	/* 32x16 bg layer, 16x16 tiles */
	{ 0x102000, 0x102fff, MRA16_RAM },	/* 64x32 text/front layer, 8x8 tiles */
	{ 0x103000, 0x103fff, MRA16_RAM },	/* spriteram */
	{ 0x104000, 0x104fff, MRA16_RAM },	/* palette */
	{ 0x108000, 0x11ffff, MRA16_RAM },	/* main ram */
MEMORY_END

static MEMORY_WRITE16_START( heatbrl_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x1007ff, cop2_mcu_w, &mcu_ram },	/* COP mcu */
	{ 0x100800, 0x100fff, legionna_background_w, &legionna_back_data },
	{ 0x101000, 0x1017ff, legionna_foreground_w, &legionna_fore_data },
	{ 0x101800, 0x101fff, legionna_midground_w,  &legionna_mid_data },
	{ 0x102000, 0x102fff, legionna_text_w, &legionna_textram },
	{ 0x103000, 0x103fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x104000, 0x104fff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x108000, 0x11ffff, MWA16_RAM },
MEMORY_END


/*****************************************************************************/

INPUT_PORTS_START( legionna )
	SEIBU_COIN_INPUTS	/* Must be port 0: coin inputs read through sound cpu */

	PORT_START
	PORT_DIPNAME( 0x001f, 0x001f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0015, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x0017, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0019, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x001b, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0x001d, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x001f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0013, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0011, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x001e, "A 1/1 B 1/2" )
	PORT_DIPSETTING(      0x0014, "A 2/1 B 1/3" )
	PORT_DIPSETTING(      0x000a, "A 3/1 B 1/5" )
	PORT_DIPSETTING(      0x0000, "A 5/1 B 1/6" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Freeze" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0200, "1" )
	PORT_DIPSETTING(      0x0300, "2" )
	PORT_DIPSETTING(      0x0100, "3" )
	PORT_BITX( 0,         0x0000, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0400, 0x0400, "Extend" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, "Easy" )
	PORT_DIPSETTING(      0x3000, "Medium" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x4000, 0x4000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
//	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( heatbrl )
	SEIBU_COIN_INPUTS	/* Must be port 0: coin inputs read through sound cpu */

	PORT_START
	PORT_DIPNAME( 0x001f, 0x001f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0015, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(      0x0017, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0019, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x001b, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 8C_3C ) )
	PORT_DIPSETTING(      0x001d, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 5C_3C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0x001f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0009, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0013, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0011, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x000f, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x000d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x000b, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(      0x001e, "A 1/1 B 1/2" )
	PORT_DIPSETTING(      0x0014, "A 2/1 B 1/3" )
	PORT_DIPSETTING(      0x000a, "A 3/1 B 1/5" )
	PORT_DIPSETTING(      0x0000, "A 5/1 B 1/6" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Freeze" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0200, "1" )
	PORT_DIPSETTING(      0x0300, "2" )
	PORT_DIPSETTING(      0x0100, "3" )
	PORT_BITX( 0,         0x0000, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x0400, 0x0400, "Extend" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x2000, "Easy" )
	PORT_DIPSETTING(      0x3000, "Medium" )
	PORT_DIPSETTING(      0x1000, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" )
	PORT_DIPNAME( 0x4000, 0x4000, "Allow Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN4 )	// haven't found coin4, maybe it doesn't exist
//	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER3  )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER3  )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW,  IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW,  IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW,  IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW,  IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW,  IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW,  IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW,  IPT_UNKNOWN )
INPUT_PORTS_END


/*****************************************************************************/


static struct GfxLayout legionna_charlayout =
{
	8,8,
	RGN_FRAC(1,4),	/* other half is BK3, decoded in char2layout */
	4,
	{ 0, 4, 4096*16*8+0, 4096*16*8+4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout heatbrl_charlayout =
{
	8,8,
	RGN_FRAC(1,2),	/* second half is junk, like legionna we may need a different decode */
	4,
	{ 0, 4, 4096*16*8+0, 4096*16*8+4 },
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};


static struct GfxLayout legionna_char2layout =
{
	16,16,
	256,	/* Can't use RGN_FRAC as (1,16) not supported */
	4,
	{ 0, 4, 4096*16*8+0, 4096*16*8+4 },
	{ 3, 2, 1, 0, 11, 10, 9, 8,
	  1024*16*8 +3,  1024*16*8 +2,  1024*16*8 +1, 1024*16*8 +0,
	  1024*16*8 +11, 1024*16*8 +10, 1024*16*8 +9, 1024*16*8 +8 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  512*16*8 +0*16, 512*16*8 +1*16, 512*16*8 +2*16, 512*16*8 +3*16,
	  512*16*8 +4*16, 512*16*8 +5*16, 512*16*8 +6*16, 512*16*8 +7*16 },
	16*8
};

static struct GfxLayout legionna_tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 2*4, 3*4, 0*4, 1*4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
	  64*8+3, 64*8+2, 64*8+1, 64*8+0, 64*8+16+3, 64*8+16+2, 64*8+16+1, 64*8+16+0 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};

static struct GfxLayout legionna_spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 2*4, 3*4, 0*4, 1*4 },
	{ 3, 2, 1, 0, 16+3, 16+2, 16+1, 16+0,
	  64*8+3, 64*8+2, 64*8+1, 64*8+0, 64*8+16+3, 64*8+16+2, 64*8+16+1, 64*8+16+0 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	128*8
};

static struct GfxDecodeInfo legionna_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &legionna_charlayout,   48*16, 16 },
	{ REGION_GFX3, 0, &legionna_tilelayout,    0*16, 16 },
	{ REGION_GFX4, 0, &legionna_char2layout,  32*16, 16 },	/* example BK3 decode */
	{ REGION_GFX2, 0, &legionna_spritelayout,  0*16, 8*16 },
	{ REGION_GFX5, 0, &legionna_tilelayout,   32*16, 16 },	/* this should be the BK3 decode */
	{ REGION_GFX6, 0, &legionna_tilelayout,   16*16, 16 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo heatbrl_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &heatbrl_charlayout,    48*16, 16 },
	{ REGION_GFX3, 0, &legionna_tilelayout,    0*16, 16 },
	{ REGION_GFX4, 0, &legionna_char2layout,  32*16, 16 },	/* unused */
	{ REGION_GFX2, 0, &legionna_spritelayout,  0*16, 8*16 },
	{ REGION_GFX5, 0, &legionna_tilelayout,   32*16, 16 },
	{ REGION_GFX6, 0, &legionna_tilelayout,   16*16, 16 },
	{ -1 } /* end of array */
};


/*****************************************************************************/

/* Parameters: YM3812 frequency, Oki frequency, Oki memory region */
SEIBU_SOUND_SYSTEM_YM3812_HARDWARE(14318180/4,8000,REGION_SOUND1);


/*****************************************************************************/

static MACHINE_DRIVER_START( legionna )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,20000000/2) 	/* ??? */
	MDRV_CPU_MEMORY(legionna_readmem,legionna_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)/* VBL */

	SEIBU_SOUND_SYSTEM_CPU(14318180/4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(seibu_sound_1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(legionna_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(128*16)

	MDRV_VIDEO_START(legionna)
	MDRV_VIDEO_UPDATE(legionna)

	/* sound hardware */
	SEIBU_SOUND_SYSTEM_YM3812_INTERFACE
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( heatbrl )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,20000000/2) 	/* ??? */
	MDRV_CPU_MEMORY(heatbrl_readmem,heatbrl_writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)/* VBL */

	SEIBU_SOUND_SYSTEM_CPU(14318180/4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(seibu_sound_1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(heatbrl_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(128*16)

	MDRV_VIDEO_START(legionna)
	MDRV_VIDEO_UPDATE(legionna)

	/* sound hardware */
	SEIBU_SOUND_SYSTEM_YM3812_INTERFACE
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( legionna )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD32_BYTE( "1",           0x00000, 0x20000, CRC(9e2d3ec8) )
	ROM_LOAD32_BYTE( "2",           0x00001, 0x20000, CRC(35c8a28f) )
	ROM_LOAD32_BYTE( "3",           0x00002, 0x20000, CRC(553fc7c0) )
	ROM_LOAD32_BYTE( "legion4.bin", 0x00003, 0x20000, CRC(2cc36c98) )

	ROM_REGION( 0x20000*2, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "6",   0x00000, 0x08000, CRC(fe7b8d06) )
	ROM_CONTINUE(    0x10000, 0x08000 )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "7",   0x000000, 0x10000, CRC(88e26809) )	/* chars, some BK3 tiles too */
	ROM_LOAD( "8",   0x010000, 0x10000, CRC(06e35407) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "obj1",     0x000000, 0x100000, CRC(d35602f5) )	/* sprites */
	ROM_LOAD( "obj2",     0x100000, 0x100000, CRC(351d3917) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "back",     0x000000, 0x100000, CRC(58280989) )	/* 3 sets of tiles ('MBK','LBK','BK3') */

	ROM_REGION( 0x020000, REGION_GFX4, ROMREGION_DISPOSE )	/* example BK3 decode */
	ROM_COPY( REGION_GFX1, 0x00000, 0x00000, 0x20000 )

	ROM_REGION( 0x020000, REGION_GFX5, ROMREGION_DISPOSE )	/* we _should_ decode all BK3 tiles here */

	ROM_REGION( 0x080000, REGION_GFX6, ROMREGION_DISPOSE )	/* LBK tiles (plus BK3 at end) */
	ROM_COPY( REGION_GFX3, 0x80000, 0x00000, 0x80000 )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "5",   0x00000, 0x20000, CRC(21d09bde) )
ROM_END

ROM_START( legionnu )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD32_BYTE( "1",   0x00000, 0x20000, CRC(9e2d3ec8) )
	ROM_LOAD32_BYTE( "2",   0x00001, 0x20000, CRC(35c8a28f) )
	ROM_LOAD32_BYTE( "3",   0x00002, 0x20000, CRC(553fc7c0) )
	ROM_LOAD32_BYTE( "4",   0x00003, 0x20000, CRC(91fd4648) )

	ROM_REGION( 0x20000*2, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "6",   0x00000, 0x08000, CRC(fe7b8d06) )
	ROM_CONTINUE(    0x10000, 0x08000 )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "7",   0x000000, 0x10000, CRC(88e26809) )	/* chars, some BK3 tiles too */
	ROM_LOAD( "8",   0x010000, 0x10000, CRC(06e35407) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "obj1",     0x000000, 0x100000, CRC(d35602f5) )	/* sprites */
	ROM_LOAD( "obj2",     0x100000, 0x100000, CRC(351d3917) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "back",     0x000000, 0x100000, CRC(58280989) )	/* 3 sets of tiles ('MBK','LBK','BK3') */

	ROM_REGION( 0x020000, REGION_GFX4, ROMREGION_DISPOSE )	/* example BK3 decode */
	ROM_COPY( REGION_GFX1, 0x00000, 0x00000, 0x20000 )

	ROM_REGION( 0x020000, REGION_GFX5, ROMREGION_DISPOSE )	/* we _should_ decode all BK3 tiles here */

	ROM_REGION( 0x080000, REGION_GFX6, ROMREGION_DISPOSE )	/* LBK tiles (plus BK3 at end) */
	ROM_COPY( REGION_GFX3, 0x80000, 0x00000, 0x80000 )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "5",   0x00000, 0x20000, CRC(21d09bde) )
ROM_END

ROM_START( heatbrl )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD32_BYTE( "1e_ver2.9k",   0x00000, 0x20000, CRC(b30bd632) )
	ROM_LOAD32_BYTE( "2e_ver2.9m",   0x00001, 0x20000, CRC(f3a23056) )
	ROM_LOAD32_BYTE( "3e_ver2.9f",   0x00002, 0x20000, CRC(a2c41715) )
	ROM_LOAD32_BYTE( "4e_ver2.9h",   0x00003, 0x20000, CRC(a50f4f08) )

	ROM_REGION( 0x20000*2, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "barrel.7",   0x00000, 0x08000, CRC(0784dbd8) )
	ROM_CONTINUE(    0x10000, 0x08000 )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "barrel.6",   0x000000, 0x10000, CRC(bea3c581) )	/* chars */
	ROM_LOAD( "barrel.5",   0x010000, 0x10000, CRC(5604d155) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "obj1",     0x000000, 0x100000, CRC(f7a7c31c) )	/* sprites */
	ROM_LOAD( "obj2",     0x100000, 0x100000, CRC(24236116) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* MBK tiles */
	ROM_LOAD( "bg-1",     0x000000, 0x100000, CRC(2f5d8baa) )

	ROM_REGION( 0x020000, REGION_GFX4, ROMREGION_DISPOSE )	/* not used */

	ROM_REGION( 0x080000, REGION_GFX5, ROMREGION_DISPOSE )	/* BK3 tiles */
	ROM_LOAD( "bg-3",     0x000000, 0x080000, CRC(83850e2d) )

	ROM_REGION( 0x080000, REGION_GFX6, ROMREGION_DISPOSE )	/* LBK tiles */
	ROM_LOAD( "bg-2",     0x000000, 0x080000, CRC(77ee4c6f) )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "barrel.8",  0x00000, 0x20000, CRC(489e5b1d) )
ROM_END

ROM_START( heatbrlo )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD32_BYTE( "barrel.1h",   0x00000, 0x20000, CRC(d5a85c36) )
	ROM_LOAD32_BYTE( "barrel.2h",   0x00001, 0x20000, CRC(5104d463) )
	ROM_LOAD32_BYTE( "barrel.3h",   0x00002, 0x20000, CRC(823373a0) )
	ROM_LOAD32_BYTE( "barrel.4h",   0x00003, 0x20000, CRC(19a8606b) )

	ROM_REGION( 0x20000*2, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "barrel.7",   0x00000, 0x08000, CRC(0784dbd8) )
	ROM_CONTINUE(    0x10000, 0x08000 )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "barrel.6",   0x000000, 0x10000, CRC(bea3c581) )	/* chars */
	ROM_LOAD( "barrel.5",   0x010000, 0x10000, CRC(5604d155) )

/* Sprite + tilemap gfx roms not dumped, for now we use ones from heatbrlu
Readme mentions as undumped:
barrel1,2,3,4.OBJ
barrel1,2,3,4.BG */

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "obj1",     0x000000, 0x100000, CRC(f7a7c31c) )	/* sprites */
	ROM_LOAD( "obj2",     0x100000, 0x100000, CRC(24236116) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* MBK tiles */
	ROM_LOAD( "bg-1",     0x000000, 0x100000, CRC(2f5d8baa) )

	ROM_REGION( 0x020000, REGION_GFX4, ROMREGION_DISPOSE )	/* not used */

	ROM_REGION( 0x080000, REGION_GFX5, ROMREGION_DISPOSE )	/* BK3 tiles */
	ROM_LOAD( "bg-3",     0x000000, 0x080000, CRC(83850e2d) )

	ROM_REGION( 0x080000, REGION_GFX6, ROMREGION_DISPOSE )	/* LBK tiles */
	ROM_LOAD( "bg-2",     0x000000, 0x080000, CRC(77ee4c6f) )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "barrel.8",  0x00000, 0x20000, CRC(489e5b1d) )
ROM_END

ROM_START( heatbrlu )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD32_BYTE( "1e_ver2.9k",   0x00000, 0x20000, CRC(b30bd632) )
	ROM_LOAD32_BYTE( "2u",           0x00001, 0x20000, CRC(289dd629) )
	ROM_LOAD32_BYTE( "3e_ver2.9f",   0x00002, 0x20000, CRC(a2c41715) )
	ROM_LOAD32_BYTE( "4e_ver2.9h",   0x00003, 0x20000, CRC(a50f4f08) )

	ROM_REGION( 0x20000*2, REGION_CPU2, 0 )	/* Z80 code, banked data */
	ROM_LOAD( "barrel.7",   0x00000, 0x08000, CRC(0784dbd8) )
	ROM_CONTINUE(    0x10000, 0x08000 )	/* banked stuff */

	ROM_REGION( 0x020000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "barrel.6",   0x000000, 0x10000, CRC(bea3c581) )	/* chars */
	ROM_LOAD( "barrel.5",   0x010000, 0x10000, CRC(5604d155) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "obj1",     0x000000, 0x100000, CRC(f7a7c31c) )	/* sprites */
	ROM_LOAD( "obj2",     0x100000, 0x100000, CRC(24236116) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* MBK tiles */
	ROM_LOAD( "bg-1",     0x000000, 0x100000, CRC(2f5d8baa) )

	ROM_REGION( 0x020000, REGION_GFX4, ROMREGION_DISPOSE )	/* not used */

	ROM_REGION( 0x080000, REGION_GFX5, ROMREGION_DISPOSE )	/* BK3 tiles */
	ROM_LOAD( "bg-3",     0x000000, 0x080000, CRC(83850e2d) )

	ROM_REGION( 0x080000, REGION_GFX6, ROMREGION_DISPOSE )	/* LBK tiles */
	ROM_LOAD( "bg-2",     0x000000, 0x080000, CRC(77ee4c6f) )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "barrel.8",  0x00000, 0x20000, CRC(489e5b1d) )
ROM_END


static DRIVER_INIT( legionna )
{
	/* Unscramble gfx: quarters 1&2 swapped, quarters 3&4 swapped */

	data8_t *gfx = memory_region(REGION_GFX1);
	int len = memory_region_length(REGION_GFX1)/2;
	int a,i;

	for (i = 0; i < len/2; i++)
	{
		a = gfx[i];
		gfx[i] = gfx[i + len/2];
		gfx[i+len/2] = a;

		a = gfx[i+len];
		gfx[i+len] = gfx[i + len/2 + len];
		gfx[i + len/2 +len] = a;
	}
}



GAMEX( 1992, legionna, 0,        legionna, legionna, legionna, ROT0, "Tad", "Legionnaire (World)", GAME_UNEMULATED_PROTECTION )
GAMEX( 1992, legionnu, legionna, legionna, legionna, legionna, ROT0, "Tad (Fabtek license)", "Legionnaire (US)", GAME_UNEMULATED_PROTECTION )
GAMEX( 1992, heatbrl,  0,        heatbrl,  heatbrl,  0,        ROT0, "Tad", "Heated Barrel (World)", GAME_UNEMULATED_PROTECTION )
GAMEX( 1992, heatbrlo, heatbrl,  heatbrl,  heatbrl,  0,        ROT0, "Tad", "Heated Barrel (World old version)", GAME_UNEMULATED_PROTECTION )
GAMEX( 1992, heatbrlu, heatbrl,  heatbrl,  heatbrl,  0,        ROT0, "Tad", "Heated Barrel (US)", GAME_UNEMULATED_PROTECTION )
