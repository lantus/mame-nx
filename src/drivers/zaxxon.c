/***************************************************************************

Notes:

- the duck season in razmataz is controlled by ff3c, maybe a timer. When it's
  0, it ends. We are currently returning 0xff so it never ends unitl you shoot
  stop.


Zaxxon memory map (preliminary)

0000-1fff ROM 3
2000-3fff ROM 2
4000-4fff ROM 1
6000-67ff RAM 1
6800-6fff RAM 2
8000-83ff Video RAM
a000-a0ff sprites

read:
c000	  IN0
c001	  IN1
c002	  DSW0
c003	  DSW1
c100	  IN2
see the input_ports definition below for details on the input bits

write:
c000	  coin A enable
c001	  coin B enable
c002	  coin aux enable
c003-c004 coin counters
c006	  flip screen
ff3c-ff3f sound (see below)
fff0	  interrupt enable
fff1	  character color bank (not used during the game, but used in test mode)
fff8-fff9 background playfield position (11 bits)
fffa	  background color bank (0 = standard  1 = reddish)
fffb	  background enable

interrupts:
VBlank triggers IRQ, handled with interrupt mode 1
NMI enters the test mode.

Changes:
25 Jan 98 LBO
	* Added crude support for samples based on Frank's info. As of yet, I don't have
	  a set that matches the names - I need a way to edit the .wav files I have.
	  Hopefully I'll be able to create a good set shortly. I also don't know which
	  sounds "loop".
26 Jan 98 LBO
	* Fixed the sound support. I lack explosion samples and the base missile sample so
	  these are untested. I'm also unsure about the background noise. It seems to have
	  a variable volume so I've tried to reproduce that via just 1 sample.

12 Mar 98 ATJ
		* For the moment replaced Brad's version of the samples with mine from the Mame/P
		  release. As yet, no variable volume, but I will be combining the features from
		  Brad's driver into mine ASAP.

****************************************************************************

Ixion Board Info

Ixion
Sega(prototype)  7/1/1983


Board set is a modified Super Zaxxon, similar to Razzmatazz

[G80 Sound board]



                        U51
                        U50

[834-0214 ZAXXON-SOUNDII]

                                   315-5013  U27 U28 U29


         U68 U69      U72
                                        U98

[         ZAXXON-VIDEOII]


U77 U78 U79                                U90 U91 U92 U93
                          U111 U112 U113

****************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/segacrpt.h"



READ_HANDLER( zaxxon_IN2_r );

extern unsigned char *zaxxon_char_color_bank;
extern unsigned char *zaxxon_background_position;
extern unsigned char *zaxxon_background_color_bank;
extern unsigned char *zaxxon_background_enable;
PALETTE_INIT( zaxxon );
VIDEO_START( zaxxon );
VIDEO_START( razmataz );
VIDEO_UPDATE( zaxxon );
VIDEO_UPDATE( razmataz );
VIDEO_UPDATE( ixion );
extern int zaxxon_vid_type;

WRITE_HANDLER( zaxxon_sound_w );



MACHINE_INIT( zaxxon )
{
	zaxxon_vid_type = 0;
}

MACHINE_INIT( futspy )
{
	zaxxon_vid_type = 2;
}

static WRITE_HANDLER( zaxxon_coin_counter_w )
{
	coin_counter_w(offset,data);
}

static WRITE_HANDLER( zaxxon_screen_flip_w )
{
	flip_screen_set(~data & 1);
}

static WRITE_HANDLER( razmataz_screen_flip_w )
{
	flip_screen_set(data & 1);
}

static READ_HANDLER( razmataz_unknown1_r )
{
	return rand() & 0xff;
}

static READ_HANDLER( razmataz_unknown2_r )
{
	return 0xff;
}

static int razmataz_dial_r(int num)
{
	static unsigned char pos[2];
	int delta,res;

	delta = readinputport(num);

	if (delta < 0x80)
	{
		/* right */
		pos[num] -= delta;
		res = (pos[num] << 1) | 1;
	}
	else
	{
		/* left */
		pos[num] += delta;
		res = (pos[num] << 1);
	}

	return res;
}

static READ_HANDLER( razmataz_dial_0_r )
{
	return razmataz_dial_r(0);
}

static READ_HANDLER( razmataz_dial_1_r )
{
	return razmataz_dial_r(1);
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x4fff, MRA_ROM },
	{ 0x6000, 0x6fff, MRA_RAM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0xa000, 0xa0ff, MRA_RAM },
	{ 0xc000, 0xc000, input_port_0_r }, /* IN0 */
	{ 0xc001, 0xc001, input_port_1_r }, /* IN1 */
	{ 0xc002, 0xc002, input_port_3_r }, /* DSW0 */
	{ 0xc003, 0xc003, input_port_4_r }, /* DSW1 */
	{ 0xc100, 0xc100, input_port_2_r }, /* IN2 */
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x4fff, MWA_ROM },
	{ 0x6000, 0x6fff, MWA_RAM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0xa000, 0xa0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc000, 0xc002, MWA_NOP },	/* coin enables */
	{ 0xc003, 0xc004, zaxxon_coin_counter_w },
	{ 0xc006, 0xc006, zaxxon_screen_flip_w },
	{ 0xff3c, 0xff3e, zaxxon_sound_w },
	{ 0xfff0, 0xfff0, interrupt_enable_w },
	{ 0xfff1, 0xfff1, MWA_RAM, &zaxxon_char_color_bank },
	{ 0xfff8, 0xfff9, MWA_RAM, &zaxxon_background_position },
	{ 0xfffa, 0xfffa, MWA_RAM, &zaxxon_background_color_bank },
	{ 0xfffb, 0xfffb, MWA_RAM, &zaxxon_background_enable },
MEMORY_END

static MEMORY_READ_START( ixion_readmem )
	{ 0x0000, 0x4fff, MRA_ROM },
	{ 0x6000, 0x6fff, MRA_RAM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0xa000, 0xa0ff, MRA_RAM },
	{ 0xc000, 0xc000, razmataz_dial_0_r }, /* IN0 */
	{ 0xc001, 0xc001, input_port_1_r }, /* IN1 */
	{ 0xc002, 0xc002, input_port_3_r }, /* DSW0 */
	{ 0xc003, 0xc003, input_port_4_r }, /* DSW1 */
	{ 0xc100, 0xc100, input_port_2_r }, /* IN2 */
	{ 0xff3c, 0xff3c, razmataz_unknown2_r },	/* no idea .. timer? in razmataz */
MEMORY_END

static MEMORY_WRITE_START( futspy_writemem )
	{ 0x0000, 0x4fff, MWA_ROM },
	{ 0x6000, 0x6fff, MWA_RAM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0xa000, 0xa0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc000, 0xc002, MWA_NOP },	/* coin enables */
	{ 0xe03c, 0xe03e, zaxxon_sound_w },
	{ 0xe0f0, 0xe0f0, interrupt_enable_w },
	{ 0xe0f1, 0xe0f1, MWA_RAM, &zaxxon_char_color_bank },
	{ 0xe0f8, 0xe0f9, MWA_RAM, &zaxxon_background_position },
	{ 0xe0fa, 0xe0fa, MWA_RAM, &zaxxon_background_color_bank },
	{ 0xe0fb, 0xe0fb, MWA_RAM, &zaxxon_background_enable },
MEMORY_END


static MEMORY_READ_START( razmataz_readmem )
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x6fff, MRA_RAM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0xa000, 0xa0ff, MRA_RAM },
	{ 0xc000, 0xc000, razmataz_dial_0_r },	/* dial pl 1 */
	{ 0xc002, 0xc002, input_port_3_r }, /* DSW0 */
	{ 0xc003, 0xc003, input_port_4_r }, /* DSW1 */
	{ 0xc004, 0xc004, input_port_6_r }, /* fire/start pl 1 */
	{ 0xc008, 0xc008, razmataz_dial_1_r },	/* dial pl 2 */
	{ 0xc00c, 0xc00c, input_port_7_r }, /* fire/start pl 2 */
	{ 0xc100, 0xc100, input_port_2_r }, /* coin */
	{ 0xc80a, 0xc80a, razmataz_unknown1_r },	/* needed, otherwise the game hangs */
	{ 0xff3c, 0xff3c, razmataz_unknown2_r },	/* timer? if 0, "duck season" ends */
MEMORY_END

static MEMORY_WRITE_START( razmataz_writemem )
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x6fff, MWA_RAM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0xa000, 0xa0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc000, 0xc002, MWA_NOP },	/* coin enables */
	{ 0xc003, 0xc004, zaxxon_coin_counter_w },
	{ 0xc006, 0xc006, razmataz_screen_flip_w },	/* ? */
	{ 0xe0f0, 0xe0f0, interrupt_enable_w },
	{ 0xe0f1, 0xe0f1, MWA_RAM, &zaxxon_char_color_bank },
	{ 0xe0f8, 0xe0f9, MWA_RAM, &zaxxon_background_position },
	{ 0xe0fa, 0xe0fa, MWA_RAM, &zaxxon_background_color_bank },
	{ 0xe0fb, 0xe0fb, MWA_RAM, &zaxxon_background_enable },
//	{ 0xff3c, 0xff3c, }, sound
MEMORY_END


/***************************************************************************

  Zaxxon uses NMI to trigger the self test. We use a fake input port to
  tie that event to a keypress.

***************************************************************************/

static INTERRUPT_GEN( zaxxon_interrupt )
{
	if (readinputport(5) & 1)	/* get status of the F2 key */
		nmi_line_pulse(); /* trigger self test */
	else
		irq0_line_hold();
}

INPUT_PORTS_START( zaxxon )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )	/* the self test calls this UP */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )	/* the self test calls this DOWN */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* button 2 - unused */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )	/* the self test calls this UP */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )	/* the self test calls this DOWN */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* button 2 - unused */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	/* the coin inputs must stay active for exactly one frame, otherwise */
	/* the game will keep inserting coins. */
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_HIGH, IPT_COIN2, 1 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE1, 1 )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x03, "10000" )
	PORT_DIPSETTING(    0x01, "20000" )
	PORT_DIPSETTING(    0x02, "30000" )
	PORT_DIPSETTING(    0x00, "40000" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unused ) )			// Only checked in "test mode"
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )			// (check code at 0x4e84)
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )			// Only checked in "test mode"
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )			// (check code at 0x4e84)
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x03, DEF_STR ( Coin_B ) )
	PORT_DIPSETTING(    0x0f, DEF_STR ( 4C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR ( 3C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR ( 2C_1C ) )
	PORT_DIPSETTING(    0x06, "2 Coins/1 Credit 5/3 6/4" )
	PORT_DIPSETTING(    0x0a, "2 Coins/1 Credit 3/2 4/3" )
	PORT_DIPSETTING(    0x03, DEF_STR ( 1C_1C ) )
	PORT_DIPSETTING(    0x02, "1 Coin/1 Credit 5/6" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit 4/5" )
	PORT_DIPSETTING(    0x04, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(    0x0d, DEF_STR ( 1C_2C ) )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits 5/11" )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits 4/9" )
	PORT_DIPSETTING(    0x05, DEF_STR ( 1C_3C ) )
	PORT_DIPSETTING(    0x09, DEF_STR ( 1C_4C ) )
	PORT_DIPSETTING(    0x01, DEF_STR ( 1C_5C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR ( 1C_6C ) )
	PORT_DIPNAME( 0xf0, 0x30, DEF_STR ( Coin_A ) )
	PORT_DIPSETTING(    0xf0, DEF_STR ( 4C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR ( 3C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR ( 2C_1C ) )
	PORT_DIPSETTING(    0x60, "2 Coins/1 Credit 5/3 6/4" )
	PORT_DIPSETTING(    0xa0, "2 Coins/1 Credit 3/2 4/3" )
	PORT_DIPSETTING(    0x30, DEF_STR ( 1C_1C ) )
	PORT_DIPSETTING(    0x20, "1 Coin/1 Credit 5/6" )
	PORT_DIPSETTING(    0xc0, "1 Coin/1 Credit 4/5" )
	PORT_DIPSETTING(    0x40, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(    0xd0, DEF_STR ( 1C_2C ) )
	PORT_DIPSETTING(    0x80, "1 Coin/2 Credits 5/11" )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits 4/9" )
	PORT_DIPSETTING(    0x50, DEF_STR ( 1C_3C ) )
	PORT_DIPSETTING(    0x90, DEF_STR ( 1C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR ( 1C_5C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR ( 1C_6C ) )

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
INPUT_PORTS_END

/* Same as 'zaxxon' but additional "Difficulty" Dip Switch */
INPUT_PORTS_START( szaxxon )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )	/* the self test calls this UP */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )	/* the self test calls this DOWN */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* button 2 - unused */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )	/* the self test calls this UP */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )	/* the self test calls this DOWN */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* button 2 - unused */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	/* the coin inputs must stay active for exactly one frame, otherwise */
	/* the game will keep inserting coins. */
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_HIGH, IPT_COIN2, 1 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE1, 1 )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x03, "10000" )
	PORT_DIPSETTING(    0x01, "20000" )
	PORT_DIPSETTING(    0x02, "30000" )
	PORT_DIPSETTING(    0x00, "40000" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unused ) )			// Only checked in "test mode"
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )			// (check code at 0x4e84)
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x03, DEF_STR ( Coin_B ) )
	PORT_DIPSETTING(    0x0f, DEF_STR ( 4C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR ( 3C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR ( 2C_1C ) )
	PORT_DIPSETTING(    0x06, "2 Coins/1 Credit 5/3 6/4" )
	PORT_DIPSETTING(    0x0a, "2 Coins/1 Credit 3/2 4/3" )
	PORT_DIPSETTING(    0x03, DEF_STR ( 1C_1C ) )
	PORT_DIPSETTING(    0x02, "1 Coin/1 Credit 5/6" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit 4/5" )
	PORT_DIPSETTING(    0x04, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(    0x0d, DEF_STR ( 1C_2C ) )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits 5/11" )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits 4/9" )
	PORT_DIPSETTING(    0x05, DEF_STR ( 1C_3C ) )
	PORT_DIPSETTING(    0x09, DEF_STR ( 1C_4C ) )
	PORT_DIPSETTING(    0x01, DEF_STR ( 1C_5C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR ( 1C_6C ) )
	PORT_DIPNAME( 0xf0, 0x30, DEF_STR ( Coin_A ) )
	PORT_DIPSETTING(    0xf0, DEF_STR ( 4C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR ( 3C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR ( 2C_1C ) )
	PORT_DIPSETTING(    0x60, "2 Coins/1 Credit 5/3 6/4" )
	PORT_DIPSETTING(    0xa0, "2 Coins/1 Credit 3/2 4/3" )
	PORT_DIPSETTING(    0x30, DEF_STR ( 1C_1C ) )
	PORT_DIPSETTING(    0x20, "1 Coin/1 Credit 5/6" )
	PORT_DIPSETTING(    0xc0, "1 Coin/1 Credit 4/5" )
	PORT_DIPSETTING(    0x40, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(    0xd0, DEF_STR ( 1C_2C ) )
	PORT_DIPSETTING(    0x80, "1 Coin/2 Credits 5/11" )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits 4/9" )
	PORT_DIPSETTING(    0x50, DEF_STR ( 1C_3C ) )
	PORT_DIPSETTING(    0x90, DEF_STR ( 1C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR ( 1C_5C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR ( 1C_6C ) )

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
INPUT_PORTS_END

INPUT_PORTS_START( futspy )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )	/* the self test calls this UP */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )	/* the self test calls this DOWN */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )	/* the self test calls this UP */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )	/* the self test calls this DOWN */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	/* the coin inputs must stay active for exactly one frame, otherwise */
	/* the game will keep inserting coins. */
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_HIGH, IPT_COIN2, 1 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE1, 1 )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x00, DEF_STR ( Coin_A ) )
	PORT_DIPSETTING(    0x08, DEF_STR ( 4C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR ( 3C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR ( 2C_1C ) )
	PORT_DIPSETTING(    0x0a, "2 Coins/1 Credit 5/3 6/4" )
	PORT_DIPSETTING(    0x0b, "2 Coins/1 Credit 4/3" )
	PORT_DIPSETTING(    0x00, DEF_STR ( 1C_1C ) )
	PORT_DIPSETTING(    0x0e, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(    0x0d, "1 Coin/1 Credit 4/5" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit 5/6" )
	PORT_DIPSETTING(    0x09, DEF_STR ( 2C_3C ) )
	PORT_DIPSETTING(    0x01, DEF_STR ( 1C_2C ) )
	PORT_DIPSETTING(    0x0f, "1 Coin/2 Credits 5/11" )
	PORT_DIPSETTING(    0x02, DEF_STR ( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR ( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR ( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR ( 1C_6C ) )
	PORT_DIPNAME( 0xf0, 0x00, DEF_STR ( Coin_B ) )
	PORT_DIPSETTING(    0x80, DEF_STR ( 4C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR ( 3C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR ( 2C_1C ) )
	PORT_DIPSETTING(    0xa0, "2 Coins/1 Credit 5/3 6/4" )
	PORT_DIPSETTING(    0xb0, "2 Coins/1 Credit 4/3" )
	PORT_DIPSETTING(    0x00, DEF_STR ( 1C_1C ) )
	PORT_DIPSETTING(    0xe0, "1 Coin/1 Credit 2/3" )
	PORT_DIPSETTING(    0xd0, "1 Coin/1 Credit 4/5" )
	PORT_DIPSETTING(    0xc0, "1 Coin/1 Credit 5/6" )
	PORT_DIPSETTING(    0x90, DEF_STR ( 2C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR ( 1C_2C ) )
	PORT_DIPSETTING(    0xf0, "1 Coin/2 Credits 5/11" )
	PORT_DIPSETTING(    0x20, DEF_STR ( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR ( 1C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR ( 1C_5C ) )
	PORT_DIPSETTING(    0x50, DEF_STR ( 1C_6C ) )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_BITX( 0,       0x0c, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20k 40k 60k" )
	PORT_DIPSETTING(    0x10, "30k 60k 90k" )
	PORT_DIPSETTING(    0x20, "40k 70k 100k" )
	PORT_DIPSETTING(    0x30, "40k 80k 120k" )
	PORT_DIPNAME( 0xc0, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x80, "Hard" )
	PORT_DIPSETTING(    0xc0, "Very Hard" )

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
INPUT_PORTS_END

INPUT_PORTS_START( razmataz )
	PORT_START	/* IN0 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_CENTER | IPF_PLAYER1, 30, 15, 0, 0 )

	PORT_START	/* IN1 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_CENTER | IPF_PLAYER2 | IPF_REVERSE, 30, 15, 0, 0 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	/* the coin inputs must stay active for exactly one frame, otherwise */
	/* the game will keep inserting coins. */
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_HIGH, IPT_COIN2, 1 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE1, 1 )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "Every 50k" )
	PORT_DIPSETTING(    0x01, "Every 100k" )
	PORT_DIPSETTING(    0x02, "Every 150k" )
	PORT_DIPSETTING(    0x03, "Every 200k" )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x0c, "Very Hard" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x30, "6" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )			// Only checked in "test mode"
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x07, 0x03, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR ( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR ( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR ( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR ( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR ( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR ( 1C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR ( 1C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR ( 1C_5C ) )
	PORT_DIPNAME( 0x38, 0x18, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR ( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR ( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR ( 2C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR ( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR ( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR ( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR ( 1C_4C ) )
	PORT_DIPSETTING(    0x38, DEF_STR ( 1C_5C ) )
	PORT_DIPNAME( 0x40, 0x40, "Test Screen Flipping" )		// Only checked in "test mode"
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )				// (check code at 0x0390)
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( ixion )
	PORT_START	/* IN0 */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_CENTER, 30, 15, 0, 0, KEYCODE_Z, KEYCODE_X, 0, 0 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	/* the coin inputs must stay active for exactly one frame, otherwise */
	/* the game will keep inserting coins. */
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_HIGH, IPT_COIN2, 1 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE1, 1 )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "Every 40k" )
	PORT_DIPSETTING(    0x01, "Every 60k" )
	PORT_DIPSETTING(    0x02, "Every 80k" )
	PORT_DIPSETTING(    0x03, "Every 100k" )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x70, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easiest" )
	PORT_DIPSETTING(    0x10, "Easier" )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x50, "Hard" )
	PORT_DIPSETTING(    0x60, "Harder" )
	PORT_DIPSETTING(    0x70, "Hardest" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x07, 0x03, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR ( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR ( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR ( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR ( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR ( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR ( 1C_3C ) )
	PORT_DIPSETTING(    0x06, DEF_STR ( 1C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR ( 1C_5C ) )
	PORT_DIPNAME( 0x38, 0x18, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR ( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR ( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR ( 2C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR ( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR ( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR ( 1C_3C ) )
	PORT_DIPSETTING(    0x30, DEF_STR ( 1C_4C ) )
	PORT_DIPSETTING(    0x38, DEF_STR ( 1C_5C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )			// Only checked in "test mode"
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
INPUT_PORTS_END



struct GfxLayout zaxxon_charlayout1 =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	3,	/* 3 bits per pixel (actually 2, the third plane is 0) */
	{ 2*256*8*8, 256*8*8, 0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};

struct GfxLayout zaxxon_charlayout2 =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	3,	/* 3 bits per pixel */
	{ 2*1024*8*8, 1024*8*8, 0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	32,32,	/* 32*32 sprites */
	64, /* 64 sprites */
	3,	/* 3 bits per pixel */
	{ 2*64*128*8, 64*128*8, 0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 24*8+4, 24*8+5, 24*8+6, 24*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8,
			64*8, 65*8, 66*8, 67*8, 68*8, 69*8, 70*8, 71*8,
			96*8, 97*8, 98*8, 99*8, 100*8, 101*8, 102*8, 103*8 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout futspy_spritelayout =
{
	32,32,	/* 32*32 sprites */
	128,	/* 128 sprites */
	3,	/* 3 bits per pixel */
	{ 2*128*128*8, 128*128*8, 0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 24*8+4, 24*8+5, 24*8+6, 24*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8,
			64*8, 65*8, 66*8, 67*8, 68*8, 69*8, 70*8, 71*8,
			96*8, 97*8, 98*8, 99*8, 100*8, 101*8, 102*8, 103*8 },
	128*8	/* every sprite takes 128 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &zaxxon_charlayout1,	 0, 32 },	/* characters */
	{ REGION_GFX2, 0, &zaxxon_charlayout2,	 0, 32 },	/* background tiles */
	{ REGION_GFX3, 0, &spritelayout,  0, 32 },			/* sprites */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo futspy_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &zaxxon_charlayout1,	 0, 32 },	/* characters */
	{ REGION_GFX2, 0, &zaxxon_charlayout2,	 0, 32 },	/* background tiles */
	{ REGION_GFX3, 0, &futspy_spritelayout,  0, 32 },	/* sprites */
	{ -1 } /* end of array */
};



static const char *zaxxon_sample_names[] =
{
	"*zaxxon",
	"03.wav",   /* Homing Missile */
	"02.wav",   /* Base Missile */
	"01.wav",   /* Laser (force field) */
	"00.wav",   /* Battleship (end of level boss) */
	"11.wav",   /* S-Exp (enemy explosion) */
	"10.wav",   /* M-Exp (ship explosion) */
	"08.wav",   /* Cannon (ship fire) */
	"23.wav",   /* Shot (enemy fire) */
	"21.wav",   /* Alarm 2 (target lock) */
	"20.wav",   /* Alarm 3 (low fuel) */
	"05.wav",   /* initial background noise */
	"04.wav",   /* looped asteroid noise */
	0
};

static struct Samplesinterface zaxxon_samples_interface =
{
	12, /* 12 channels */
	25, /* volume */
	zaxxon_sample_names
};


static MACHINE_DRIVER_START( zaxxon )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3072000)	/* 3.072 MHz ?? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(zaxxon_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(zaxxon)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(32*8)

	MDRV_PALETTE_INIT(zaxxon)
	MDRV_VIDEO_START(zaxxon)
	MDRV_VIDEO_UPDATE(zaxxon)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, zaxxon_samples_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( futspy )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3072000)	/* 3.072 MHz ?? */
	MDRV_CPU_MEMORY(readmem,futspy_writemem)
	MDRV_CPU_VBLANK_INT(zaxxon_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(futspy)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(futspy_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(32*8)

	MDRV_PALETTE_INIT(zaxxon)
	MDRV_VIDEO_START(zaxxon)
	MDRV_VIDEO_UPDATE(zaxxon)

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, zaxxon_samples_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( razmataz )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3072000)	/* 3.072 MHz ?? */
	MDRV_CPU_MEMORY(razmataz_readmem,razmataz_writemem)
	MDRV_CPU_VBLANK_INT(zaxxon_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(zaxxon)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(32*8)

	MDRV_PALETTE_INIT(zaxxon)
	MDRV_VIDEO_START(razmataz)
	MDRV_VIDEO_UPDATE(razmataz)

	/* sound hardware */
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ixion )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3072000)	/* 3.072 MHz ?? */
	MDRV_CPU_MEMORY(ixion_readmem,razmataz_writemem)
	MDRV_CPU_VBLANK_INT(zaxxon_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(zaxxon)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(32*8)

	MDRV_PALETTE_INIT(zaxxon)
	MDRV_VIDEO_START(razmataz)
	MDRV_VIDEO_UPDATE(ixion)

	/* sound hardware */
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( zaxxon )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "zaxxon.3",     0x0000, 0x2000, CRC(6e2b4a30) SHA1(80ac53c554c84226b119cbe3cf3470bcdbcd5762) )
	ROM_LOAD( "zaxxon.2",     0x2000, 0x2000, CRC(1c9ea398) SHA1(0cd259be3fa80f3d53dfa76d5ca06773cdfe5945) )
	ROM_LOAD( "zaxxon.1",     0x4000, 0x1000, CRC(1c123ef9) SHA1(2588be06ea7baca6112d58c78a1eeb98aad8a02e) )

	ROM_REGION( 0x1800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "zaxxon.14",    0x0000, 0x0800, CRC(07bf8c52) SHA1(425157a1625b1bd5169c3218b958010bf6af12bb) )  /* characters */
	ROM_LOAD( "zaxxon.15",    0x0800, 0x0800, CRC(c215edcb) SHA1(f1ded2173eb139f48d2ca86c5ef00acbe6c11cd3) )
	/* 1000-17ff empty space to convert the characters as 3bpp instead of 2 */

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "zaxxon.6",     0x0000, 0x2000, CRC(6e07bb68) SHA1(a002f3441b0f0044615ce71ecbd14edadba16270) )  /* background tiles */
	ROM_LOAD( "zaxxon.5",     0x2000, 0x2000, CRC(0a5bce6a) SHA1(a86543727389931244ba8a576b543d7ac05a2585) )
	ROM_LOAD( "zaxxon.4",     0x4000, 0x2000, CRC(a5bf1465) SHA1(a8cd27dfb4a606bae8bfddcf936e69e980fb1977) )

	ROM_REGION( 0x6000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "zaxxon.11",    0x0000, 0x2000, CRC(eaf0dd4b) SHA1(194e2ca0a806e0cb6bb7cc8341d1fc6f2ea911f6) )  /* sprites */
	ROM_LOAD( "zaxxon.12",    0x2000, 0x2000, CRC(1c5369c7) SHA1(af6a5984c3cedfa8c9efcd669f4f205b51a433b2) )
	ROM_LOAD( "zaxxon.13",    0x4000, 0x2000, CRC(ab4e8a9a) SHA1(4ac79cccc30e4adfa878b36101e97e20ac010438) )

	ROM_REGION( 0x8000, REGION_GFX4, ROMREGION_DISPOSE )	/* background tilemaps converted in vh_start */
	ROM_LOAD( "zaxxon.8",     0x0000, 0x2000, CRC(28d65063) SHA1(e1f90716236c61df61bdc6915a8e390cb4dcbf15) )
	ROM_LOAD( "zaxxon.7",     0x2000, 0x2000, CRC(6284c200) SHA1(d26a9049541479b8b19f5aa0690cf4aaa787c9b5) )
	ROM_LOAD( "zaxxon.10",    0x4000, 0x2000, CRC(a95e61fd) SHA1(a0f8c15ff75affa3532abf8f340811cf415421fd) )
	ROM_LOAD( "zaxxon.9",     0x6000, 0x2000, CRC(7e42691f) SHA1(2124363be8f590b74e2b15dd3f90d77dd9ca9528) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "zaxxon.u98",   0x0000, 0x0100, CRC(6cc6695b) SHA1(01ae8450ccc302e1a5ae74230d44f6f531a962e2) ) /* palette */
	ROM_LOAD( "zaxxon.u72",   0x0100, 0x0100, CRC(deaa21f7) SHA1(0cf08fb62f77d93ff7cb883c633e0db35906e11d) ) /* char lookup table */
ROM_END

ROM_START( zaxxon2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "3a",           0x0000, 0x2000, CRC(b18e428a) SHA1(d3ff077e37a3ed8a9cc32cba19e1694b79df6b30) )
	ROM_LOAD( "zaxxon.2",     0x2000, 0x2000, CRC(1c9ea398) SHA1(0cd259be3fa80f3d53dfa76d5ca06773cdfe5945) )
	ROM_LOAD( "1a",           0x4000, 0x1000, CRC(1977d933) SHA1(b0100a51a85928b8df3b07b27c9e7e4f929d7893) )

	ROM_REGION( 0x1800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "zaxxon.14",    0x0000, 0x0800, CRC(07bf8c52) SHA1(425157a1625b1bd5169c3218b958010bf6af12bb) )  /* characters */
	ROM_LOAD( "zaxxon.15",    0x0800, 0x0800, CRC(c215edcb) SHA1(f1ded2173eb139f48d2ca86c5ef00acbe6c11cd3) )
	/* 1000-17ff empty space to convert the characters as 3bpp instead of 2 */

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "zaxxon.6",     0x0000, 0x2000, CRC(6e07bb68) SHA1(a002f3441b0f0044615ce71ecbd14edadba16270) )  /* background tiles */
	ROM_LOAD( "zaxxon.5",     0x2000, 0x2000, CRC(0a5bce6a) SHA1(a86543727389931244ba8a576b543d7ac05a2585) )
	ROM_LOAD( "zaxxon.4",     0x4000, 0x2000, CRC(a5bf1465) SHA1(a8cd27dfb4a606bae8bfddcf936e69e980fb1977) )

	ROM_REGION( 0x6000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "zaxxon.11",    0x0000, 0x2000, CRC(eaf0dd4b) SHA1(194e2ca0a806e0cb6bb7cc8341d1fc6f2ea911f6) )  /* sprites */
	ROM_LOAD( "zaxxon.12",    0x2000, 0x2000, CRC(1c5369c7) SHA1(af6a5984c3cedfa8c9efcd669f4f205b51a433b2) )
	ROM_LOAD( "zaxxon.13",    0x4000, 0x2000, CRC(ab4e8a9a) SHA1(4ac79cccc30e4adfa878b36101e97e20ac010438) )

	ROM_REGION( 0x8000, REGION_GFX4, ROMREGION_DISPOSE )	/* background tilemaps converted in vh_start */
	ROM_LOAD( "zaxxon.8",     0x0000, 0x2000, CRC(28d65063) SHA1(e1f90716236c61df61bdc6915a8e390cb4dcbf15) )
	ROM_LOAD( "zaxxon.7",     0x2000, 0x2000, CRC(6284c200) SHA1(d26a9049541479b8b19f5aa0690cf4aaa787c9b5) )
	ROM_LOAD( "zaxxon.10",    0x4000, 0x2000, CRC(a95e61fd) SHA1(a0f8c15ff75affa3532abf8f340811cf415421fd) )
	ROM_LOAD( "zaxxon.9",     0x6000, 0x2000, CRC(7e42691f) SHA1(2124363be8f590b74e2b15dd3f90d77dd9ca9528) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "zaxxon.u98",   0x0000, 0x0100, CRC(6cc6695b) SHA1(01ae8450ccc302e1a5ae74230d44f6f531a962e2) ) /* palette */
	ROM_LOAD( "j214a2.72",    0x0100, 0x0100, CRC(a9e1fb43) SHA1(57dbcfe2438fd090c08594818549aeea6339eab2) ) /* char lookup table */
ROM_END

ROM_START( zaxxonb )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "zaxxonb.3",    0x0000, 0x2000, CRC(125bca1c) SHA1(f4160966d42e5282736cde8a276204ba8910ca61) )
	ROM_LOAD( "zaxxonb.2",    0x2000, 0x2000, CRC(c088df92) SHA1(c0c6cd8dcf6db65129980331fa9ecc3800b63436) )
	ROM_LOAD( "zaxxonb.1",    0x4000, 0x1000, CRC(e7bdc417) SHA1(209f0d259f60b984c84229bb31af1ef939adc73e) )

	ROM_REGION( 0x1800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "zaxxon.14",    0x0000, 0x0800, CRC(07bf8c52) SHA1(425157a1625b1bd5169c3218b958010bf6af12bb) )  /* characters */
	ROM_LOAD( "zaxxon.15",    0x0800, 0x0800, CRC(c215edcb) SHA1(f1ded2173eb139f48d2ca86c5ef00acbe6c11cd3) )
	/* 1000-17ff empty space to convert the characters as 3bpp instead of 2 */

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "zaxxon.6",     0x0000, 0x2000, CRC(6e07bb68) SHA1(a002f3441b0f0044615ce71ecbd14edadba16270) )  /* background tiles */
	ROM_LOAD( "zaxxon.5",     0x2000, 0x2000, CRC(0a5bce6a) SHA1(a86543727389931244ba8a576b543d7ac05a2585) )
	ROM_LOAD( "zaxxon.4",     0x4000, 0x2000, CRC(a5bf1465) SHA1(a8cd27dfb4a606bae8bfddcf936e69e980fb1977) )

	ROM_REGION( 0x6000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "zaxxon.11",    0x0000, 0x2000, CRC(eaf0dd4b) SHA1(194e2ca0a806e0cb6bb7cc8341d1fc6f2ea911f6) )  /* sprites */
	ROM_LOAD( "zaxxon.12",    0x2000, 0x2000, CRC(1c5369c7) SHA1(af6a5984c3cedfa8c9efcd669f4f205b51a433b2) )
	ROM_LOAD( "zaxxon.13",    0x4000, 0x2000, CRC(ab4e8a9a) SHA1(4ac79cccc30e4adfa878b36101e97e20ac010438) )

	ROM_REGION( 0x8000, REGION_GFX4, ROMREGION_DISPOSE )	/* background tilemaps converted in vh_start */
	ROM_LOAD( "zaxxon.8",     0x0000, 0x2000, CRC(28d65063) SHA1(e1f90716236c61df61bdc6915a8e390cb4dcbf15) )
	ROM_LOAD( "zaxxon.7",     0x2000, 0x2000, CRC(6284c200) SHA1(d26a9049541479b8b19f5aa0690cf4aaa787c9b5) )
	ROM_LOAD( "zaxxon.10",    0x4000, 0x2000, CRC(a95e61fd) SHA1(a0f8c15ff75affa3532abf8f340811cf415421fd) )
	ROM_LOAD( "zaxxon.9",     0x6000, 0x2000, CRC(7e42691f) SHA1(2124363be8f590b74e2b15dd3f90d77dd9ca9528) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "zaxxon.u98",   0x0000, 0x0100, CRC(6cc6695b) SHA1(01ae8450ccc302e1a5ae74230d44f6f531a962e2) ) /* palette */
	ROM_LOAD( "zaxxon.u72",   0x0100, 0x0100, CRC(deaa21f7) SHA1(0cf08fb62f77d93ff7cb883c633e0db35906e11d) ) /* char lookup table */
ROM_END

ROM_START( szaxxon )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "suzaxxon.3",   0x0000, 0x2000, CRC(af7221da) SHA1(b5d3beb296d52ed69b4ceacf329c20a72e3a1dce) )
	ROM_LOAD( "suzaxxon.2",   0x2000, 0x2000, CRC(1b90fb2a) SHA1(afb2bd2ffee3f5e589064f59b6ac21ed915094df) )
	ROM_LOAD( "suzaxxon.1",   0x4000, 0x1000, CRC(07258b4a) SHA1(91e3a0c0df6c9cf66980d1ffcc3830ffdbef8c2f) )

	ROM_REGION( 0x1800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "suzaxxon.14",  0x0000, 0x0800, CRC(bccf560c) SHA1(9f92bd15466048a5665bfc2ebc8c6504af9353eb) )  /* characters */
	ROM_LOAD( "suzaxxon.15",  0x0800, 0x0800, CRC(d28c628b) SHA1(42ab7dc0e4e0d09213054597373383cdb6a55699) )
	/* 1000-17ff empty space to convert the characters as 3bpp instead of 2 */

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "suzaxxon.6",   0x0000, 0x2000, CRC(f51af375) SHA1(8682217dc800f43b73cd5e8501dbf3b7cd136dc1) )  /* background tiles */
	ROM_LOAD( "suzaxxon.5",   0x2000, 0x2000, CRC(a7de021d) SHA1(a1bee07aa906366aa69866d1bdff38e2d90fafdd) )
	ROM_LOAD( "suzaxxon.4",   0x4000, 0x2000, CRC(5bfb3b04) SHA1(f898e42d6bc1fd3629c9caee3c2af27805969ac6) )

	ROM_REGION( 0x6000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "suzaxxon.11",  0x0000, 0x2000, CRC(1503ae41) SHA1(d4085f15fcbfb9547a7f9e2cb7ce9276c4d6c08d) )  /* sprites */
	ROM_LOAD( "suzaxxon.12",  0x2000, 0x2000, CRC(3b53d83f) SHA1(118e9d2b4f5daf96f5a38ccd92d0b046a470b0b2) )
	ROM_LOAD( "suzaxxon.13",  0x4000, 0x2000, CRC(581e8793) SHA1(2b3305dd55dc09d7394ed8ae691773972dba28b9) )

	ROM_REGION( 0x8000, REGION_GFX4, ROMREGION_DISPOSE )	/* background tilemaps converted in vh_start */
	ROM_LOAD( "suzaxxon.8",   0x0000, 0x2000, CRC(dd1b52df) SHA1(8170dd9f81c41104694951a2c74405d0c6d8b9b6) )
	ROM_LOAD( "suzaxxon.7",   0x2000, 0x2000, CRC(b5bc07f0) SHA1(1e4d460ce8cca66b081ee8ec1a9adb6ef98274ec) )
	ROM_LOAD( "suzaxxon.10",  0x4000, 0x2000, CRC(68e84174) SHA1(b78c44d92078552835a20bcb7125fc9ca8af5048) )
	ROM_LOAD( "suzaxxon.9",   0x6000, 0x2000, CRC(a509994b) SHA1(51541ec78ab3f8241a5ddf7f99a46f5e44292992) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "suzaxxon.u98", 0x0000, 0x0100, CRC(15727a9f) SHA1(42840e9ab303fb64102a1dbae03d66c9cf743a9f) ) /* palette */
	ROM_LOAD( "suzaxxon.u72", 0x0100, 0x0100, CRC(deaa21f7) SHA1(0cf08fb62f77d93ff7cb883c633e0db35906e11d) ) /* char lookup table */
ROM_END

ROM_START( futspy )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "fs_snd.u27",   0x0000, 0x2000, CRC(7578fe7f) SHA1(ab42bdf74b07c1ba5337c3d34647d3ee16f9db05) )
	ROM_LOAD( "fs_snd.u28",   0x2000, 0x2000, CRC(8ade203c) SHA1(f095f4019befff7af4203c886ef42357f79592a1) )
	ROM_LOAD( "fs_snd.u29",   0x4000, 0x1000, CRC(734299c3) SHA1(12acf71d9d00e0e0df29c4d8c397ad407266b364) )

	ROM_REGION( 0x1800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "fs_snd.u68",   0x0000, 0x0800, CRC(305fae2d) SHA1(fbe89feff0fb2d4515000d1b73b7c91aac4e0b67) )  /* characters */
	ROM_LOAD( "fs_snd.u69",   0x0800, 0x0800, CRC(3c5658c0) SHA1(70ac44b9334b086cdecd73f5f7820a0bf8ae2629) )
	/* 1000-17ff empty space to convert the characters as 3bpp instead of 2 */

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "fs_vid.113",   0x0000, 0x2000, CRC(36d2bdf6) SHA1(c27835055beedf61ba644070f8920b6008d99040) )  /* background tiles */
	ROM_LOAD( "fs_vid.112",   0x2000, 0x2000, CRC(3740946a) SHA1(e7579dd91628a811a60a8d8a5b407728b74aa17e) )
	ROM_LOAD( "fs_vid.111",   0x4000, 0x2000, CRC(4cd4df98) SHA1(3ae4b2d0a79069e0de81596805bcf1a9ae7912cf) )

	ROM_REGION( 0xc000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "fs_vid.u77",   0x0000, 0x4000, CRC(1b93c9ec) SHA1(4b1d3b7e35d65cc3b96eb4f2e98c59e779bcb1c1) )  /* sprites */
	ROM_LOAD( "fs_vid.u78",   0x4000, 0x4000, CRC(50e55262) SHA1(363acbde7b37a2358b3e53cfc08c9bd5dee73d55) )
	ROM_LOAD( "fs_vid.u79",   0x8000, 0x4000, CRC(bfb02e3e) SHA1(f53bcec46b8c7d26e9ab01c821a8d1578b85f786) )

	ROM_REGION( 0x8000, REGION_GFX4, ROMREGION_DISPOSE )	/* background tilemaps converted in vh_start */
	ROM_LOAD( "fs_vid.u91",   0x0000, 0x2000, CRC(86da01f4) SHA1(954e4be1b0e24c8bc88c2b328e3a0e32005bb7b2) )
	ROM_LOAD( "fs_vid.u90",   0x2000, 0x2000, CRC(2bd41d2d) SHA1(efb74b4bce31c7868ab6438e07b02b0539d35120) )
	ROM_LOAD( "fs_vid.u93",   0x4000, 0x2000, CRC(b82b4997) SHA1(263f74aab47fc4e516b2111eaa94beea61c5fbe5) )
	ROM_LOAD( "fs_vid.u92",   0x6000, 0x2000, CRC(af4015af) SHA1(6ed01a42d395ada6f2442b68f901fe61b04c8e44) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "futrprom.u98", 0x0000, 0x0100, CRC(9ba2acaa) SHA1(20e0257ca531ddc398b3aab861c7b5c41b659d40) ) /* palette */
	ROM_LOAD( "futrprom.u72", 0x0100, 0x0100, CRC(f9e26790) SHA1(339f27e0126312d35211b5ce533f293b58851c1d) ) /* char lookup table */
ROM_END

ROM_START( razmataz )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "u27",           0x0000, 0x2000, CRC(254f350f) SHA1(f8e84778b7ffc4da76e97992f01c742c212480cf) )
	ROM_LOAD( "u28",           0x2000, 0x2000, CRC(3a1eaa99) SHA1(d1f2a61a8548135c9754097aa468672616244710) )
	ROM_LOAD( "u29",           0x4000, 0x2000, CRC(0ee67e78) SHA1(c6c703000a4e0da8af65be53b2a6b2ef67860c30) )

	ROM_REGION( 0x1800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1921.u68",      0x0000, 0x0800, CRC(77f8ff5a) SHA1(d535109387559dd5b58dc6432a1eae6535442079) )  /* characters */
	ROM_LOAD( "1922.u69",      0x0800, 0x0800, CRC(cf63621e) SHA1(60452ad34f2b0e0afa0f09455d9aa84058c54fd5) )
	/* 1000-17ff empty space to convert the characters as 3bpp instead of 2 */

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1934.113",      0x0000, 0x2000, CRC(39bb679c) SHA1(0a384286dbfc8b35e4779119f62769b6cfc93a52) )  /* background tiles */
	ROM_LOAD( "1933.112",      0x2000, 0x2000, CRC(1022185e) SHA1(874d796baea8ade2c642f3640ec7875a9f509a68) )
	ROM_LOAD( "1932.111",      0x4000, 0x2000, CRC(c7a715eb) SHA1(8b04558c87c5a5f94a5bab9fbe198a0b8a84ebf4) )

	ROM_REGION( 0x6000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "1925.u77",      0x0000, 0x2000, CRC(a7965437) SHA1(24ab3cc9b6d70e8cab4f0a20f84fb98682b321f5) )  /* sprites */
	ROM_LOAD( "1926.u78",      0x2000, 0x2000, CRC(9a3af434) SHA1(0b5b1ac9cf8bee1c3830ef3baffcd7d3a05bf765) )
	ROM_LOAD( "1927.u79",      0x4000, 0x2000, CRC(0323de2b) SHA1(6f6ceafe6472d59bd0ffecb9dd2d401659157b50) )

	ROM_REGION( 0x8000, REGION_GFX4, ROMREGION_DISPOSE )	/* background tilemaps converted in vh_start */
	ROM_LOAD( "1929.u91",      0x0000, 0x2000, CRC(55c7c757) SHA1(ad8d548eb965f343e88bad4b4ad1b5b226f21d71) )
	ROM_LOAD( "1928.u90",      0x2000, 0x2000, CRC(e58b155b) SHA1(dd6abeae66de69734b7aa5e133dbfb8f8a35578e) )
	ROM_LOAD( "1931.u93",      0x4000, 0x2000, CRC(55fe0f82) SHA1(391434b41b6235199a4f19a8873a523cbb417f70) )
	ROM_LOAD( "1930.u92",      0x6000, 0x2000, CRC(f355f105) SHA1(93067b7390c05b71020e77abdd9577b39e486d9f) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "clr.u98",       0x0000, 0x0100, CRC(0fd671af) SHA1(7f26139398754dae7383c9375fca95b7970fcefb) ) /* palette */
	ROM_LOAD( "clr.u72",       0x0100, 0x0100, CRC(03233bc5) SHA1(30bd690da7eda4e13df90d7ee59dbf744b3541a4) ) /* char lookup table */

	ROM_REGION( 0x1000, REGION_SOUND1, 0 ) /* sound? */
	ROM_LOAD( "1923.u50",      0x0000, 0x0800, CRC(59994a51) SHA1(57ccee24a989efe39f8ffc08aab7d72a1cdef3d1) )
	ROM_LOAD( "1924.u51",      0x0800, 0x0800, CRC(a75e0011) SHA1(7d67ce2e8a2de471221b3b565a937ae1a35e1560) )
ROM_END

ROM_START( ixion )
	ROM_REGION( 2*0x10000, REGION_CPU1, 0 )	/* 64k for code + 64k for decrypted opcodes */
	ROM_LOAD( "1937d.u27",           0x0000, 0x2000, CRC(f447aac5) SHA1(f6ec02f20482649ba1765254e0e67a8593075092) )
	ROM_LOAD( "1938b.u28",           0x2000, 0x2000, CRC(17f48640) SHA1(d661e8ae0747c2c526360cb72e403deba7a98e71) )
	ROM_LOAD( "1955b.u29",           0x4000, 0x1000, CRC(78636ec6) SHA1(afca6418221e700749031cb3fa738907d77c1566) )

	ROM_REGION( 0x1800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1939a.u68",      0x0000, 0x0800, CRC(c717ddc7) SHA1(86fdef368f097a27aac6e05bf3208fcdaf7d9da7) )  /* characters */
	ROM_LOAD( "1940a.u69",      0x0800, 0x0800, CRC(ec4bb3ad) SHA1(8a38bc48cda59b5e76a5153d459bb2d01d6a56f3) )
	/* 1000-17ff empty space to convert the characters as 3bpp instead of 2 */

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1952a.113",      0x0000, 0x2000, CRC(ffb9b03d) SHA1(b7a900166a880ca4a71fec6ad02f5c0ecfc92df8) )  /* background tiles */
	ROM_LOAD( "1951a.112",      0x2000, 0x2000, CRC(db743f1b) SHA1(a5d13d597fe999757137d96fb4bf7c7efc7a3245) )
	ROM_LOAD( "1950a.111",      0x4000, 0x2000, CRC(c2de178a) SHA1(0347a751accb02576b9cd8b123b79018cc05268c) )

	ROM_REGION( 0x6000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "1945a.u77",      0x0000, 0x2000, CRC(3a3fbfe7) SHA1(f3a503f476524f9de5b55de49009972124e58601) )  /* sprites */
	ROM_LOAD( "1946a.u78",      0x2000, 0x2000, CRC(f2cb1b53) SHA1(8c2fb58ce7de7876c4d2f1a3d13c6a5efd06d354) )
	ROM_LOAD( "1947a.u79",      0x4000, 0x2000, CRC(d2421e92) SHA1(07000055ace2c6983c7add180904d6bc20e1bb3b) )

	ROM_REGION( 0x8000, REGION_GFX4, ROMREGION_DISPOSE )	/* background tilemaps converted in vh_start */
	ROM_LOAD( "1948a.u91",      0x0000, 0x2000, CRC(7a7fcbbe) SHA1(76dffffcadbe446091ee98958873aa76f7b17213) )
	ROM_LOAD( "1953a.u90",      0x2000, 0x2000, CRC(6b626ea7) SHA1(7e02da0b031a42b077c173d85f15f75242e61e98) )
	ROM_LOAD( "1949a.u93",      0x4000, 0x2000, CRC(e7722d09) SHA1(c9a0fb4fac798454facd3d5dd02d2c05cfe8e3a6) )
	ROM_LOAD( "1954a.u92",      0x6000, 0x2000, CRC(a970f5ff) SHA1(0f1f8f329ceefcbd0725f8eeff1b01348f5c9374) )

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "1942a.u98",       0x0000, 0x0100, CRC(3a8e6f74) SHA1(c2d480f8c8a111c1e23cbf819dea807f8128208d) ) /* palette */
	ROM_LOAD( "1941a.u72",       0x0100, 0x0100, CRC(a5d0d97e) SHA1(2677508b44f9b7a6c6ee56e49a7b88073e80debe) ) /* char lookup */

	ROM_REGION( 0x1000, REGION_SOUND1, 0 ) /* sound? */
	ROM_LOAD( "1943a.u50",      0x0000, 0x0800, CRC(77e5a1f0) SHA1(00152ffb59ebac718b300fdf24314b456748ffbe) )
	ROM_LOAD( "1944a.u51",      0x0800, 0x0800, CRC(88215098) SHA1(54bd1c71e7f10f20623e47f4e791f54ce698bc08) )
ROM_END

static DRIVER_INIT( zaxxonb )
{
/*
	the values vary, but the translation mask is always laid out like this:

	  0 1 2 3 4 5 6 7 8 9 a b c d e f
	0 A A B B A A B B C C D D C C D D
	1 A A B B A A B B C C D D C C D D
	2 E E F F E E F F G G H H G G H H
	3 E E F F E E F F G G H H G G H H
	4 A A B B A A B B C C D D C C D D
	5 A A B B A A B B C C D D C C D D
	6 E E F F E E F F G G H H G G H H
	7 E E F F E E F F G G H H G G H H
	8 H H G G H H G G F F E E F F E E
	9 H H G G H H G G F F E E F F E E
	a D D C C D D C C B B A A B B A A
	b D D C C D D C C B B A A B B A A
	c H H G G H H G G F F E E F F E E
	d H H G G H H G G F F E E F F E E
	e D D C C D D C C B B A A B B A A
	f D D C C D D C C B B A A B B A A

	(e.g. 0xc0 is XORed with H)
	therefore in the following tables we only keep track of A, B, C, D, E, F, G and H.
*/
	static const unsigned char data_xortable[2][8] =
	{
		{ 0x0a,0x0a,0x22,0x22,0xaa,0xaa,0x82,0x82 },	/* ...............0 */
		{ 0xa0,0xaa,0x28,0x22,0xa0,0xaa,0x28,0x22 },	/* ...............1 */
	};
	static const unsigned char opcode_xortable[8][8] =
	{
		{ 0x8a,0x8a,0x02,0x02,0x8a,0x8a,0x02,0x02 },	/* .......0...0...0 */
		{ 0x80,0x80,0x08,0x08,0xa8,0xa8,0x20,0x20 },	/* .......0...0...1 */
		{ 0x8a,0x8a,0x02,0x02,0x8a,0x8a,0x02,0x02 },	/* .......0...1...0 */
		{ 0x02,0x08,0x2a,0x20,0x20,0x2a,0x08,0x02 },	/* .......0...1...1 */
		{ 0x88,0x0a,0x88,0x0a,0xaa,0x28,0xaa,0x28 },	/* .......1...0...0 */
		{ 0x80,0x80,0x08,0x08,0xa8,0xa8,0x20,0x20 },	/* .......1...0...1 */
		{ 0x88,0x0a,0x88,0x0a,0xaa,0x28,0xaa,0x28 },	/* .......1...1...0 */
		{ 0x02,0x08,0x2a,0x20,0x20,0x2a,0x08,0x02 } 	/* .......1...1...1 */
	};
	int A;
	unsigned char *rom = memory_region(REGION_CPU1);
	int diff = memory_region_length(REGION_CPU1) / 2;


	memory_set_opcode_base(0,rom+diff);

	for (A = 0x0000;A < 0x8000;A++)
	{
		int i,j;
		unsigned char src;


		src = rom[A];

		/* pick the translation table from bit 0 of the address */
		i = A & 1;

		/* pick the offset in the table from bits 1, 3 and 5 of the source data */
		j = ((src >> 1) & 1) + (((src >> 3) & 1) << 1) + (((src >> 5) & 1) << 2);
		/* the bottom half of the translation table is the mirror image of the top */
		if (src & 0x80) j = 7 - j;

		/* decode the ROM data */
		rom[A] = src ^ data_xortable[i][j];

		/* now decode the opcodes */
		/* pick the translation table from bits 0, 4, and 8 of the address */
		i = ((A >> 0) & 1) + (((A >> 4) & 1) << 1) + (((A >> 8) & 1) << 2);
		rom[A + diff] = src ^ opcode_xortable[i][j];
	}
}

static DRIVER_INIT( szaxxon )
{
	szaxxon_decode();
}

static DRIVER_INIT( futspy )
{
	futspy_decode();
}

static DRIVER_INIT( razmataz )
{
	nprinces_decode();
}

static DRIVER_INIT( ixion )
{
	szaxxon_decode();
}


GAME( 1982, zaxxon,   0,         zaxxon,   zaxxon,   0,        ROT90,  "Sega",    "Zaxxon (set 1)" )
GAME( 1982, zaxxon2,  zaxxon,    zaxxon,   zaxxon,   0,        ROT90,  "Sega",    "Zaxxon (set 2)" )
GAME( 1982, zaxxonb,  zaxxon,    zaxxon,   zaxxon,   zaxxonb,  ROT90,  "bootleg", "Jackson" )
GAME( 1982, szaxxon,  0,         zaxxon,   szaxxon,  szaxxon,  ROT90,  "Sega",    "Super Zaxxon" )
GAME( 1984, futspy,   0,         futspy,   futspy,   futspy,   ROT270, "Sega",    "Future Spy" )
GAMEX(1983, razmataz, 0,         razmataz, razmataz, razmataz, ROT270, "Sega",    "Razzmatazz", GAME_NO_SOUND )
GAMEX(1983, ixion,    0,         ixion,    ixion,    ixion,    ROT270, "Sega",    "Ixion (prototype)", GAME_NO_SOUND )

