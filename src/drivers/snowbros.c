/***************************************************************************

  Snow Brothers (Toaplan) / SemiCom Hardware
  uses Kaneko's Pandora sprite chip (also used in DJ Boy, Air Buster ..)

  driver by Mike Coates

  Hyper Pacman addition by David Haywood
   + some bits by Nicola Salmoria


Stephh's notes (hyperpac):

  - According to the "Language" Dip Switch, this game is a Korean game.
     (although the Language Dipswitch doesn't affect language, but yes
      I believe SemiCom to be a Korean Company)
  - There is no "cocktail mode", nor way to flip the screen.

todo:

make the originals work.
they're probably all this hardware or a varation on it, they don't work
(most point the interrupt vectors directly at a small area of ram which I'd
guess is shared with the Philips 87c52 mcu, more more plus doesn't point the
vectors there but does have a jump there in the code). See hyperpac for an
example, the protection data for that game was extracted from the bootleg.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"


WRITE16_HANDLER( snowbros_flipscreen_w );
VIDEO_UPDATE( snowbros );
VIDEO_UPDATE( wintbob );
VIDEO_UPDATE( snowbro3 );

static data16_t *hyperpac_ram;

static INTERRUPT_GEN( snowbros_interrupt )
{
	cpu_set_irq_line(0, cpu_getiloops() + 2, HOLD_LINE);	/* IRQs 4, 3, and 2 */
}


/* Sound Routines */

READ16_HANDLER( snowbros_68000_sound_r )
{
	int ret;

	/* If the sound CPU is running, read the YM3812 status, otherwise
	   just make it pass the test */
	if (Machine->sample_rate != 0)
	{
		ret = soundlatch_r(offset);
	}
	else
	{
		ret = 3;
	}

	return ret;
}


static WRITE16_HANDLER( snowbros_68000_sound_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w(offset,data & 0xff);
		cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
	}
}

static WRITE16_HANDLER( semicom_soundcmd_w )
{
	if (ACCESSING_LSB) soundlatch_w(0,data & 0xff);
}


/* Snow Bros Memory Map */

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },
	{ 0x300000, 0x300001, snowbros_68000_sound_r },
	{ 0x500000, 0x500001, input_port_0_word_r },
	{ 0x500002, 0x500003, input_port_1_word_r },
	{ 0x500004, 0x500005, input_port_2_word_r },
	{ 0x600000, 0x6001ff, MRA16_RAM },
	{ 0x700000, 0x701fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM },
	{ 0x200000, 0x200001, watchdog_reset16_w },
	{ 0x300000, 0x300001, snowbros_68000_sound_w },
	{ 0x400000, 0x400001, snowbros_flipscreen_w },
	{ 0x600000, 0x6001ff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x700000, 0x701fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x800000, 0x800001, MWA16_NOP },	/* IRQ 4 acknowledge? */
	{ 0x900000, 0x900001, MWA16_NOP },	/* IRQ 3 acknowledge? */
	{ 0xa00000, 0xa00001, MWA16_NOP },	/* IRQ 2 acknowledge? */
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
MEMORY_END

static PORT_READ_START( sound_readport )
	{ 0x02, 0x02, YM3812_status_port_0_r },
	{ 0x04, 0x04, soundlatch_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ 0x02, 0x02, YM3812_control_port_0_w },
	{ 0x03, 0x03, YM3812_write_port_0_w },
	{ 0x04, 0x04, soundlatch_w },	/* goes back to the main CPU, checked during boot */
PORT_END

/* SemiCom Memory Map

the SemiCom games have slightly more ram and are protected
sound hardware is also different

*/

static MEMORY_READ16_START( hyperpac_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x100000, 0x10ffff, MRA16_RAM },

	{ 0x500000, 0x500001, input_port_0_word_r },
	{ 0x500002, 0x500003, input_port_1_word_r },
	{ 0x500004, 0x500005, input_port_2_word_r },

	{ 0x600000, 0x6001ff, MRA16_RAM },
	{ 0x700000, 0x701fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( hyperpac_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x100000, 0x10ffff, MWA16_RAM, &hyperpac_ram },
	{ 0x300000, 0x300001, semicom_soundcmd_w },
//	{ 0x400000, 0x400001,  }, ???
	{ 0x600000, 0x6001ff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x700000, 0x701fff, MWA16_RAM, &spriteram16, &spriteram_size },

	{ 0x800000, 0x800001, MWA16_NOP },	/* IRQ 4 acknowledge? */
	{ 0x900000, 0x900001, MWA16_NOP },	/* IRQ 3 acknowledge? */
	{ 0xa00000, 0xa00001, MWA16_NOP },	/* IRQ 2 acknowledge? */
MEMORY_END

static MEMORY_READ_START( hyperpac_sound_readmem )
	{ 0x0000, 0xcfff, MRA_ROM },
	{ 0xd000, 0xd7ff, MRA_RAM },
	{ 0xf001, 0xf001, YM2151_status_port_0_r },
	{ 0xf008, 0xf008, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( hyperpac_sound_writemem )
	{ 0x0000, 0xcfff, MWA_ROM },
	{ 0xd000, 0xd7ff, MWA_RAM },
	{ 0xf000, 0xf000, YM2151_register_port_0_w },
	{ 0xf001, 0xf001, YM2151_data_port_0_w },
	{ 0xf002, 0xf002, OKIM6295_data_0_w },
//	{ 0xf006, 0xf006,  }, ???
MEMORY_END

/* Snow Bros 3
close to SemiCom

*/

static MEMORY_READ16_START( readmem3 )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x100000, 0x103fff, MRA16_RAM },
//	{ 0x300000, 0x300001, OKIM6295_status_0_msb_r }, // ?
	{ 0x500000, 0x500001, input_port_0_word_r },
	{ 0x500002, 0x500003, input_port_1_word_r },
	{ 0x500004, 0x500005, input_port_2_word_r },
	{ 0x600000, 0x6003ff, MRA16_RAM },
	{ 0x700000, 0x7021ff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( writemem3 )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x100000, 0x103fff, MWA16_RAM },
	{ 0x200000, 0x200001, watchdog_reset16_w },
//	{ 0x300000, 0x300001, OKIM6295_data_0_msb_w }, // ?
	{ 0x400000, 0x400001, snowbros_flipscreen_w },
	{ 0x600000, 0x6003ff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x700000, 0x7021ff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x800000, 0x800001, MWA16_NOP },	/* IRQ 4 acknowledge? */
	{ 0x900000, 0x900001, MWA16_NOP },	/* IRQ 3 acknowledge? */
	{ 0xa00000, 0xa00001, MWA16_NOP },	/* IRQ 2 acknowledge? */
MEMORY_END

INPUT_PORTS_START( snowbros )
	PORT_START	/* 500001 */
	PORT_DIPNAME( 0x01, 0x00, "Country (Affects Coinage)" )
	PORT_DIPSETTING(    0x00, "Europe" )
	PORT_DIPSETTING(    0x01, "America (Romstar license)" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
/* Better to implement a coin mode 1-2 stuff later */
	PORT_DIPNAME( 0x30, 0x30, "Coin A Europe/America" )
	PORT_DIPSETTING(    0x00, "4C/1C 2C/3C" )
	PORT_DIPSETTING(    0x10, "3C/1C 2C/1C" )
	PORT_DIPSETTING(    0x20, "2C/1C 1C/2C" )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B Europe/America" )
	PORT_DIPSETTING(    0xc0, "1C/2C 1C/1C" )
	PORT_DIPSETTING(    0x80, "1C/3C 1C/2C" )
	PORT_DIPSETTING(    0x40, "1C/4C 2C/1C" )
	PORT_DIPSETTING(    0x00, "1C/6C 2C/3C" )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Must be low or game stops! */
													/* probably VBlank */

	PORT_START	/* 500003 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "100k and every 200k " )
	PORT_DIPSETTING(    0x0c, "100k Only" )
	PORT_DIPSETTING(    0x08, "200k Only" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* 500005 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( snowbroj )
	PORT_START	/* 500001 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Must be low or game stops! */
													/* probably VBlank */

	PORT_START	/* 500003 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "100k and every 200k " )
	PORT_DIPSETTING(    0x0c, "100k Only" )
	PORT_DIPSETTING(    0x08, "200k Only" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* 500005 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( hyperpac )
	PORT_START	/* 500000.w */
	PORT_DIPNAME( 0x0001, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Lives ) )	// "Language" in the "test mode"
	PORT_DIPSETTING(      0x0002, "3" )					// "Korean"
	PORT_DIPSETTING(      0x0000, "5" )					// "English"
	PORT_DIPNAME( 0x001c, 0x001c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x001c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0060, 0x0060, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, "Easy" )
	PORT_DIPSETTING(      0x0060, "Normal" )
	PORT_DIPSETTING(      0x0040, "Hard" )
	PORT_DIPSETTING(      0x0020, "Hardest" )			// "Very Hard"
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )	// jump
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )	// fire
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER1 )	// test mode only?
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* 500002.w */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )	// jump
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )	// fire
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )	// test mode only?
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* 500004.w */
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/* SnowBros */

static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ STEP8(0,4), STEP8(8*32,4) },
	{ STEP8(0,32), STEP8(16*32,32) },
	32*32
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout,  0, 16 },
	{ -1 } /* end of array */
};

/* Winter Bobble */

static struct GfxLayout tilelayout_wb =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ STEP4(3*4,-4), STEP4(7*4,-4), STEP4(11*4,-4), STEP4(15*4,-4) },
	{ STEP16(0,64) },
	16*64
};

static struct GfxDecodeInfo gfxdecodeinfo_wb[] =
{
	{ REGION_GFX1, 0, &tilelayout_wb,  0, 16 },
	{ -1 }
};

/* SemiCom */

static struct GfxLayout hyperpac_tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 8*32+4, 8*32+0, 20,16, 8*32+20, 8*32+16,
	  12, 8, 8*32+12, 8*32+8, 28, 24, 8*32+28, 8*32+24 },
	{ 0*32, 2*32, 1*32, 3*32, 16*32+0*32, 16*32+2*32, 16*32+1*32, 16*32+3*32,
	  4*32, 6*32, 5*32, 7*32, 16*32+4*32, 16*32+6*32, 16*32+5*32, 16*32+7*32 },
	32*32
};

static struct GfxDecodeInfo hyperpac_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &hyperpac_tilelayout,  0, 16 },
	{ -1 } /* end of array */
};

/* snow bros 3 */

static struct GfxLayout sb3_tilebglayout =
{
 	16,16,
 	RGN_FRAC(1,1),
 	8,
 	{8, 9,10, 11, 0, 1, 2, 3  },
 	{ 0, 4, 16, 20, 32, 36, 48, 52,
 	512+0,512+4,512+16,512+20,512+32,512+36,512+48,512+52},
 	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
 	1024+0*16,1024+1*64,1024+2*64,1024+3*64,1024+4*64,1024+5*64,1024+6*64,1024+7*64},
 	32*64
};


static struct GfxDecodeInfo sb3_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout,  0, 16 },
	{ REGION_GFX2, 0, &sb3_tilebglayout,  0, 2 },
	{ -1 } /* end of array */
};

/* handler called by the 3812/2151 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpu_set_irq_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

/* SnowBros Sound */

static struct YM3812interface ym3812_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz ? (hand tuned) */
	{ 100 },	/* volume */
	{ irqhandler },
};

/* SemiCom Sound */

static struct YM2151interface ym2151_interface =
{
	1,
	4000000,	/* 4 MHz??? */
	{ YM3012_VOL(10,MIXER_PAN_LEFT,10,MIXER_PAN_RIGHT) },
	{ irqhandler }
};

static struct OKIM6295interface okim6295_interface =
{
	1,			/* 1 chip */
	{ 7575 },		/* 7575Hz playback? */
	{ REGION_SOUND1 },
	{ 100 }
};


static MACHINE_DRIVER_START( snowbros )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 8000000)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(snowbros_interrupt,3)

	MDRV_CPU_ADD_TAG("sound", Z80, 3600000) /* 3.6 MHz ??? */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PORTS(sound_readport,sound_writeport)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_UPDATE(snowbros)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("3812", YM3812, ym3812_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( wintbob )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(snowbros)
	MDRV_CPU_REPLACE("main", M68000, 10000000) /* faster cpu on bootleg? otherwise the gfx break up */

	/* video hardware */
	MDRV_GFXDECODE(gfxdecodeinfo_wb)
	MDRV_VIDEO_UPDATE(wintbob)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( hyperpac )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(snowbros)
	MDRV_CPU_REPLACE("main", M68000, 16000000) /* 16mhz or 12mhz ? */
	MDRV_CPU_MEMORY(hyperpac_readmem,hyperpac_writemem)

	MDRV_CPU_REPLACE("sound", Z80, 4000000) /* 4.0 MHz ??? */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(hyperpac_sound_readmem,hyperpac_sound_writemem)

	MDRV_GFXDECODE(hyperpac_gfxdecodeinfo)

	/* sound hardware */
	MDRV_SOUND_REPLACE("3812",YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( snowbro3 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000) /* 16mhz or 12mhz ? */
	MDRV_CPU_MEMORY(readmem3,writemem3)
	MDRV_CPU_VBLANK_INT(snowbros_interrupt,3)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(sb3_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)

	MDRV_VIDEO_UPDATE(snowbro3)

	/* sound hardware */
	/* oki for sound */
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( snowbros )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "sn6.bin",  0x00000, 0x20000, CRC(4899ddcf) SHA1(47d750d3022a80e47ffabe47566bb2556cc8d477) )
	ROM_LOAD16_BYTE( "sn5.bin",  0x00001, 0x20000, CRC(ad310d3f) SHA1(f39295b38d99087dbb9c5b00bf9cb963337a50e2) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for z80 sound code */
	ROM_LOAD( "snowbros.4",   0x0000, 0x8000, CRC(e6eab4e4) SHA1(d08187d03b21192e188784cb840a37a7bdb5ad32) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ch0",          0x00000, 0x20000, CRC(36d84dfe) SHA1(5d45a750220930bc409de30f19282bb143fbf94f) )
	ROM_LOAD( "ch1",          0x20000, 0x20000, CRC(76347256) SHA1(48ec03965905adaba5e50eb3e42a2813f7883bb4) )
	ROM_LOAD( "ch2",          0x40000, 0x20000, CRC(fdaa634c) SHA1(1271c74df7da7596caf67caae3c51b4c163a49f4) )
	ROM_LOAD( "ch3",          0x60000, 0x20000, CRC(34024aef) SHA1(003a9b9ee3aaab3d787894d3d4126d372b19d2a8) )
ROM_END

ROM_START( snowbroa )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "snowbros.3a",  0x00000, 0x20000, CRC(10cb37e1) SHA1(786be4640f8df2c81a32decc189ea7657ace00c6) )
	ROM_LOAD16_BYTE( "snowbros.2a",  0x00001, 0x20000, CRC(ab91cc1e) SHA1(8cff61539dc7d35fcbf110d3e54fc1883e7b8509) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for z80 sound code */
	ROM_LOAD( "snowbros.4",   0x0000, 0x8000, CRC(e6eab4e4) SHA1(d08187d03b21192e188784cb840a37a7bdb5ad32) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ch0",          0x00000, 0x20000, CRC(36d84dfe) SHA1(5d45a750220930bc409de30f19282bb143fbf94f) )
	ROM_LOAD( "ch1",          0x20000, 0x20000, CRC(76347256) SHA1(48ec03965905adaba5e50eb3e42a2813f7883bb4) )
	ROM_LOAD( "ch2",          0x40000, 0x20000, CRC(fdaa634c) SHA1(1271c74df7da7596caf67caae3c51b4c163a49f4) )
	ROM_LOAD( "ch3",          0x60000, 0x20000, CRC(34024aef) SHA1(003a9b9ee3aaab3d787894d3d4126d372b19d2a8) )
ROM_END

ROM_START( snowbrob )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "sbros3-a",     0x00000, 0x20000, CRC(301627d6) SHA1(0d1dc70091c87e9c27916d4232ff31b7381a64e1) )
	ROM_LOAD16_BYTE( "sbros2-a",     0x00001, 0x20000, CRC(f6689f41) SHA1(e4fd27b930a31479c0d99e0ddd23d5db34044666) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for z80 sound code */
	ROM_LOAD( "snowbros.4",   0x0000, 0x8000, CRC(e6eab4e4) SHA1(d08187d03b21192e188784cb840a37a7bdb5ad32) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ch0",          0x00000, 0x20000, CRC(36d84dfe) SHA1(5d45a750220930bc409de30f19282bb143fbf94f) )
	ROM_LOAD( "ch1",          0x20000, 0x20000, CRC(76347256) SHA1(48ec03965905adaba5e50eb3e42a2813f7883bb4) )
	ROM_LOAD( "ch2",          0x40000, 0x20000, CRC(fdaa634c) SHA1(1271c74df7da7596caf67caae3c51b4c163a49f4) )
	ROM_LOAD( "ch3",          0x60000, 0x20000, CRC(34024aef) SHA1(003a9b9ee3aaab3d787894d3d4126d372b19d2a8) )
ROM_END

ROM_START( snowbroj )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "snowbros.3",   0x00000, 0x20000, CRC(3f504f9e) SHA1(700758b114c3fde6ea8f84222af0850dba13cd3b) )
	ROM_LOAD16_BYTE( "snowbros.2",   0x00001, 0x20000, CRC(854b02bc) SHA1(4ad1548eef94dcb95119cb4a7dcdefa037591b5b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for z80 sound code */
	ROM_LOAD( "snowbros.4",   0x0000, 0x8000, CRC(e6eab4e4) SHA1(d08187d03b21192e188784cb840a37a7bdb5ad32) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	/* The gfx ROM (snowbros.1) was bad, I'm using the ones from the other sets. */
	ROM_LOAD( "ch0",          0x00000, 0x20000, CRC(36d84dfe) SHA1(5d45a750220930bc409de30f19282bb143fbf94f) )
	ROM_LOAD( "ch1",          0x20000, 0x20000, CRC(76347256) SHA1(48ec03965905adaba5e50eb3e42a2813f7883bb4) )
	ROM_LOAD( "ch2",          0x40000, 0x20000, CRC(fdaa634c) SHA1(1271c74df7da7596caf67caae3c51b4c163a49f4) )
	ROM_LOAD( "ch3",          0x60000, 0x20000, CRC(34024aef) SHA1(003a9b9ee3aaab3d787894d3d4126d372b19d2a8) )
ROM_END

ROM_START( wintbob )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "wb03.bin", 0x00000, 0x10000, CRC(df56e168) SHA1(20dbabdd97e6f3d4bf6500bf9e8476942cb48ae3) )
	ROM_LOAD16_BYTE( "wb01.bin", 0x00001, 0x10000, CRC(05722f17) SHA1(9356e2488ea35e0a2978689f2ca6dfa0d57fd2ed) )
	ROM_LOAD16_BYTE( "wb04.bin", 0x20000, 0x10000, CRC(53be758d) SHA1(56cf85ba23fe699031d73e8f367a1b8ac837d5f8) )
	ROM_LOAD16_BYTE( "wb02.bin", 0x20001, 0x10000, CRC(fc8e292e) SHA1(857cfeb0be121e64e6117120514ae1f2ffeae4d6) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for z80 sound code */
	ROM_LOAD( "wb05.bin",     0x0000, 0x10000, CRC(53fe59df) SHA1(a99053e82b9fed76f744fa9f67078294641c6317) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	/* probably identical data to Snow Bros, in a different format */
	ROM_LOAD16_BYTE( "wb13.bin",     0x00000, 0x10000, CRC(426921de) SHA1(5107c58e7e08d71895baa67fe260b17ebd61389c) )
	ROM_LOAD16_BYTE( "wb06.bin",     0x00001, 0x10000, CRC(68204937) SHA1(fd2ef93df5fd8aa2d36072858dbcfce41157ef3e) )
	ROM_LOAD16_BYTE( "wb12.bin",     0x20000, 0x10000, CRC(ef4e04c7) SHA1(17158b61b3c158e0491db9abb2e1a8c20d981d37) )
	ROM_LOAD16_BYTE( "wb07.bin",     0x20001, 0x10000, CRC(53f40978) SHA1(058bbf3b7877f0cd320383e0386c5959e0d6589b) )
	ROM_LOAD16_BYTE( "wb11.bin",     0x40000, 0x10000, CRC(41cb4563) SHA1(94f1d12d299ac08fc8522139e1927f0cf739be75) )
	ROM_LOAD16_BYTE( "wb08.bin",     0x40001, 0x10000, CRC(9497b88c) SHA1(367c6106276f3816528341f11f3a97ae458d25cd) )
	ROM_LOAD16_BYTE( "wb10.bin",     0x60000, 0x10000, CRC(5fa22b1e) SHA1(1164003d873e9738a3ca133cce689c7120061e3c) )
	ROM_LOAD16_BYTE( "wb09.bin",     0x60001, 0x10000, CRC(9be718ca) SHA1(5c195e4f13efbdb229201d2408d018861bf389cc) )
ROM_END

/* SemiCom Games */

ROM_START( hyperpac )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "hyperpac.h12", 0x00001, 0x20000, CRC(2cf0531a) SHA1(c4321d728845035507352d0bcf4348d28b92e85e) )
	ROM_LOAD16_BYTE( "hyperpac.i12", 0x00000, 0x20000, CRC(9c7d85b8) SHA1(432d5fbe8bef875ce4a9aeb74a7b57dc79c709fd) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "hyperpac.u1", 0x00000, 0x10000 , CRC(03faf88e) SHA1(a8da883d4b765b809452bbffca37ff224edbe86d) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "hyperpac.j15", 0x00000, 0x40000, CRC(fb9f468d) SHA1(52857b1a04c64ac853340ebb8e92d98eabea8bc1) )

	ROM_REGION( 0x0c0000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "hyperpac.a4", 0x000000, 0x40000, CRC(bd8673da) SHA1(8466355894da4d2c9a54d03a833cc9b4ec0c67eb) )
	ROM_LOAD( "hyperpac.a5", 0x040000, 0x40000, CRC(5d90cd82) SHA1(56be68478a81bb4e1011990da83334929a0ac886) )
	ROM_LOAD( "hyperpac.a6", 0x080000, 0x40000, CRC(61d86e63) SHA1(974c634607993924fa098eff106b1b288bec1e26) )
ROM_END

ROM_START( hyperpcb )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "hpacuh12.bin", 0x00001, 0x20000, CRC(633ab2c6) SHA1(534435fa602adebf651e1d42f7c96b01eb6634ef) )
	ROM_LOAD16_BYTE( "hpacui12.bin", 0x00000, 0x20000, CRC(23dc00d1) SHA1(8d4d00f450b94912adcbb24073f9b3b01eab0450) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "hyperpac.u1", 0x00000, 0x10000 , CRC(03faf88e) SHA1(a8da883d4b765b809452bbffca37ff224edbe86d) ) // was missing from this set, using the one from the original

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "hyperpac.j15", 0x00000, 0x40000, CRC(fb9f468d) SHA1(52857b1a04c64ac853340ebb8e92d98eabea8bc1) )

	ROM_REGION( 0x0c0000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "hyperpac.a4", 0x000000, 0x40000, CRC(bd8673da) SHA1(8466355894da4d2c9a54d03a833cc9b4ec0c67eb) )
	ROM_LOAD( "hyperpac.a5", 0x040000, 0x40000, CRC(5d90cd82) SHA1(56be68478a81bb4e1011990da83334929a0ac886) )
	ROM_LOAD( "hyperpac.a6", 0x080000, 0x40000, CRC(61d86e63) SHA1(974c634607993924fa098eff106b1b288bec1e26) )
ROM_END

ROM_START( moremorp )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "mmp_u52.bin",  0x00001, 0x40000, CRC(66baf9b2) )
	ROM_LOAD16_BYTE( "mmp_u74.bin",  0x00000, 0x40000, CRC(7c6fede5) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "mmp_u35.bin", 0x00000, 0x10000 , CRC(4d098cad) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "mmp_u14.bin", 0x00000, 0x40000, CRC(211a2566) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "mmp_u75.bin", 0x000000, 0x80000, CRC(af9e824e) )
	ROM_LOAD( "mmp_u76.bin", 0x080000, 0x80000, CRC(c42af064) )
	ROM_LOAD( "mmp_u77.bin", 0x100000, 0x80000, CRC(1d7396e1) )
	ROM_LOAD( "mmp_u78.bin", 0x180000, 0x80000, CRC(5508d80b) )
ROM_END

ROM_START( 3in1semi )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "u52",  0x00001, 0x40000, CRC(b0e4a0f7) )
	ROM_LOAD16_BYTE( "u74",  0x00000, 0x40000, CRC(266862c4) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "u35", 0x00000, 0x10000 , CRC(e40481da) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "u14", 0x00000, 0x40000, CRC(c83c11be) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "u75", 0x000000, 0x80000, CRC(b66a0db6) )
	ROM_LOAD( "u76", 0x080000, 0x80000, CRC(5f4b48ea) )
	ROM_LOAD( "u77", 0x100000, 0x80000, CRC(d44211e3) )
	ROM_LOAD( "u78", 0x180000, 0x80000, CRC(af596afc) )
ROM_END

ROM_START( 4in1boot ) /* snow bros, tetris, hyperman 1, pacman 2 */
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "u52",  0x00001, 0x80000, CRC(71815878) SHA1(e3868f5687c1d8ec817671c50ade6c56ee83bfa1) )
	ROM_LOAD16_BYTE( "u74",  0x00000, 0x80000, CRC(e22d3fa2) SHA1(020ab92d8cbf37a9f8186a81934abb97088c16f9) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "u35", 0x00000, 0x10000 , CRC(c894ac80) SHA1(ee896675b5205ab2dbd0cbb13db16aa145391d06) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "u14", 0x00000, 0x40000, CRC(94b09b0e) SHA1(414de3e36eff85126038e8ff74145b35076e0a43) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* Sprites */
	/* this only seems to have snowbros gfx? */
	ROM_LOAD( "u78", 0x000000, 0x100000, BAD_DUMP CRC(0f925f01) SHA1(74f35185ffb19ca291673ce8a5330fd11fede276) )
ROM_END


ROM_START( cookbib2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "cookbib2.01",  0x00000, 0x40000, CRC(65aafde2) )
	ROM_LOAD16_BYTE( "cookbib2.02",  0x00001, 0x40000, CRC(b2909460) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "cookbib2.07", 0x00000, 0x10000 , CRC(f59f1c9a) )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "cookbib2.06", 0x00000, 0x20000, CRC(5e6f76b8) )

	ROM_REGION( 0x140000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "cookbib2.05", 0x000000, 0x80000, CRC(89fb38ce) )
	ROM_LOAD( "cookbib2.04", 0x080000, 0x80000, CRC(f240111f) )
	ROM_LOAD( "cookbib2.03", 0x100000, 0x40000, CRC(e1604821) )
ROM_END

/* this one could be more different hw, gfx don't decode the same */
ROM_START( htchctch )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "p03.b16",  0x00001, 0x20000, CRC(eff14c40) )
	ROM_LOAD16_BYTE( "p04.b17",  0x00000, 0x20000, CRC(6991483a) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "p02.b5", 0x00000, 0x10000 , CRC(c5a03186) )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "p01.c1", 0x00000, 0x20000, CRC(18c06829) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "p06srom5.bin", 0x00000, 0x40000, CRC(3d2cbb0d) )
	ROM_LOAD( "p07srom6.bin", 0x40000, 0x40000, CRC(0207949c) )
	ROM_LOAD( "p08uor1.bin",  0x80000, 0x20000, CRC(6811e7b6) )
	ROM_LOAD( "p09uor2.bin",  0xa0000, 0x20000, CRC(1c6549cf) )
	ROM_LOAD( "p10uor3.bin",  0xc0000, 0x20000, CRC(6462e6e0) )
	ROM_LOAD( "p11uor4.bin",  0xe0000, 0x20000, CRC(9c511d98) )
ROM_END

/* cool mini, only 3 roms? */
/* bc story, encrypted?, dif. hardware? */

ROM_START( snowbro3 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "ur4",  0x00000, 0x20000, CRC(19c13ffd) SHA1(4f9db70354bd410b7bcafa96be4591de8dc33d90) )
	ROM_LOAD16_BYTE( "ur3",  0x00001, 0x20000, CRC(3f32fa15) SHA1(1402c173c1df142ff9dd7b859689c075813a50e5) )

	/* is sound cpu code missing or is it driven by the main cpu? */

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ua5",		0x000000, 0x80000, CRC(0604e385) SHA1(96acbc65a8db89a7be100f852dc07ba9a0313167) )	/* 16x16 tiles */

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* 16x16 BG Tiles */
	ROM_LOAD( "un7",		0x000000, 0x200000, CRC(4a79da4c) SHA1(59207d116d39b9ee25c51affe520f5fdff34e536) )
	ROM_LOAD( "un8",		0x200000, 0x200000, CRC(7a4561a4) SHA1(1dd823369c09368d1f0ec8e1cb85d700f10ff448) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "us5",     0x00000, 0x80000, CRC(7c6368ef) SHA1(53393c570c605f7582b61c630980041e2ed32e2d) )
ROM_END

static DRIVER_INIT( hyperpac )
{
	/* simulate RAM initialization done by the protection MCU */
	/* not verified on real hardware */
	hyperpac_ram[0xe000/2] = 0x4ef9;
	hyperpac_ram[0xe002/2] = 0x0000;
	hyperpac_ram[0xe004/2] = 0x062c;

	hyperpac_ram[0xe080/2] = 0xfedc;
	hyperpac_ram[0xe082/2] = 0xba98;
	hyperpac_ram[0xe084/2] = 0x7654;
	hyperpac_ram[0xe086/2] = 0x3210;
}

static DRIVER_INIT(snowbro3)
{
	unsigned char *buffer;
	data8_t *src = memory_region(REGION_CPU1);
	int len = memory_region_length(REGION_CPU1);

	/* strange order */
	if ((buffer = malloc(len)))
	{
		int i;
		for (i = 0;i < len; i++)
			buffer[i] = src[BITSWAP24(i,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,3,4,1,2,0)];
		memcpy(src,buffer,len);
		free(buffer);
	}
}

static DRIVER_INIT(4in1boot)
{
	unsigned char *buffer;
	data8_t *src = memory_region(REGION_CPU1);
	int len = memory_region_length(REGION_CPU1);

	/* strange order */
	if ((buffer = malloc(len)))
	{
		int i;
		for (i = 0;i < len; i++)
			if (i&1) buffer[i] = BITSWAP8(src[i],6,7,5,4,3,2,1,0);
			else buffer[i] = src[i];

		memcpy(src,buffer,len);
		free(buffer);
	}

	src = memory_region(REGION_CPU2);
	len = memory_region_length(REGION_CPU2);

	/* strange order */
	if ((buffer = malloc(len)))
	{
		int i;
		for (i = 0;i < len; i++)
			buffer[i] = src[i^0x4000];
		memcpy(src,buffer,len);
		free(buffer);
	}

	/* is this swapped too? i'm not sure */
	src = memory_region(REGION_GFX1);
	len = memory_region_length(REGION_GFX1);

	if ((buffer = malloc(len)))
	{
		int i;
		for (i = 0;i < len; i++)
			buffer[i] = src[BITSWAP24(i,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)];
		memcpy(src,buffer,len);
		free(buffer);
	}

}

GAME( 1990, snowbros, 0,        snowbros, snowbros, 0, ROT0, "Toaplan", "Snow Bros. - Nick & Tom (set 1)" )
GAME( 1990, snowbroa, snowbros, snowbros, snowbros, 0, ROT0, "Toaplan", "Snow Bros. - Nick & Tom (set 2)" )
GAME( 1990, snowbrob, snowbros, snowbros, snowbros, 0, ROT0, "Toaplan", "Snow Bros. - Nick & Tom (set 3)" )
GAME( 1990, snowbroj, snowbros, snowbros, snowbroj, 0, ROT0, "Toaplan", "Snow Bros. - Nick & Tom (Japan)" )
GAME( 1990, wintbob,  snowbros, wintbob,  snowbros, 0, ROT0, "bootleg", "The Winter Bobble" )
/* SemiCom Games */
GAME( 1995, hyperpac, 0,        hyperpac, hyperpac, hyperpac, ROT0, "SemiCom", "Hyper Pacman" )
GAME( 1995, hyperpcb, hyperpac, hyperpac, hyperpac, 0,        ROT0, "bootleg", "Hyper Pacman (bootleg)" )
GAMEX(2002, snowbro3, 0,        snowbro3, snowbroj, snowbro3, ROT0, "Syrmex / hack?", "Snow Brothers 3 - Magical Adventure", GAME_NO_SOUND ) // its basically snowbros code?...
/* the following don't work, they either point the interrupts at an area of ram probably shared by
   some kind of mcu which puts 68k code there, or jump to the area in the interrupts */
GAMEX(199?, moremorp, 0,        hyperpac, hyperpac, 0,        ROT0, "SemiCom", "More More +", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX(1997, 3in1semi, 0,        hyperpac, hyperpac, 0,        ROT0, "SemiCom", "3-in-1 (SemiCom)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX(1996, cookbib2, 0,        hyperpac, hyperpac, 0,        ROT0, "SemiCom", "Cookie and Bibi 2", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX(1995, htchctch, 0,        hyperpac, hyperpac, 0,        ROT0, "SemiCom", "Hatch Catch", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
/* bad dump? */
GAMEX(199?, 4in1boot, 0,        hyperpac, hyperpac, 4in1boot, ROT0, "bootleg", "4-in-1 bootleg", GAME_NOT_WORKING )
