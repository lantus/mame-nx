/***************************************************************************

Congo Bongo memory map (preliminary)

0000-1fff ROM 1
2000-3fff ROM 2
4000-5fff ROM 3
6000-7fff ROM 4

8000-87ff RAM 1
8800-8fff RAM 2
a000-a3ff Video RAM
a400-a7ff Color RAM

8400-8fff sprites

read:
c000	  IN0
c001	  IN1
c002	  DSW1
c003	  DSW2
c008	  IN2

write:
c000-c002 ?
c006	  ?
c018	  coinAen, used to enable input on coin slot A
c019	  coinBen, used to enable input on coin slot B
c01a	  serven,  used to enable input on service sw
c01b	  counterA,coin counter for slot A
c01c	  counterB,coin counter for slot B
c01d	  background enable
c01e	  flip screen
c01f	  interrupt enable

c028-c029 background playfield position
c031	  sprite enable ?
c032	  sprite start
c033	  sprite count


interrupts:
VBlank triggers IRQ, handled with interrupt mode 1
NMI causes a ROM/RAM test.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern struct GfxLayout zaxxon_charlayout1;
extern struct GfxLayout zaxxon_charlayout2;

extern int zaxxon_vid_type;
extern unsigned char *zaxxon_background_position;
extern unsigned char *zaxxon_background_enable;
PALETTE_INIT( zaxxon );
VIDEO_START( zaxxon );
VIDEO_UPDATE( zaxxon );



MACHINE_INIT( congo )
{
	zaxxon_vid_type = 1;
}

WRITE_HANDLER( congo_daio_w )
{
	if (offset == 1)
	{
		if (data & 2) sample_start(0,0,0);
	}
	else if (offset == 2)
	{
		data ^= 0xff;

		if (data & 0x80)
		{
			if (data & 8) sample_start(1,1,0);
			if (data & 4) sample_start(2,2,0);
			if (data & 2) sample_start(3,3,0);
			if (data & 1) sample_start(4,4,0);
		}
	}
}

static WRITE_HANDLER( congo_coin_counter_w )
{
	coin_counter_w(offset,data);
}

static MEMORY_READ_START( readmem )
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0xa000, 0xa7ff, MRA_RAM },
	{ 0xc000, 0xc000, input_port_0_r },
	{ 0xc001, 0xc001, input_port_1_r },
	{ 0xc008, 0xc008, input_port_2_r },
	{ 0xc002, 0xc002, input_port_3_r },
	{ 0xc003, 0xc003, input_port_4_r },
	{ 0x0000, 0x7fff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0xa000, 0xa3ff, videoram_w, &videoram, &videoram_size },
	{ 0xa400, 0xa7ff, colorram_w, &colorram },
	{ 0x8400, 0x8fff, MWA_RAM, &spriteram },
	{ 0xc01f, 0xc01f, interrupt_enable_w },
	{ 0xc028, 0xc029, MWA_RAM, &zaxxon_background_position },
	{ 0xc01d, 0xc01d, MWA_RAM, &zaxxon_background_enable },
	{ 0xc038, 0xc038, soundlatch_w },
	{ 0xc030, 0xc033, MWA_NOP }, /* ??? */
	{ 0xc01e, 0xc01e, MWA_NOP }, /* flip unused for now */
	{ 0xc018, 0xc018, MWA_NOP }, /* coinAen */
	{ 0xc019, 0xc019, MWA_NOP }, /* coinBen */
	{ 0xc01a, 0xc01a, MWA_NOP }, /* serven */
	{ 0xc01b, 0xc01c, congo_coin_counter_w }, /* counterA, counterB */
	{ 0x0000, 0x7fff, MWA_ROM },
MEMORY_END


static MEMORY_READ_START( sh_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x8000, 0x8003, soundlatch_r },
MEMORY_END
static MEMORY_WRITE_START( sh_writemem )
	{ 0x6000, 0x6003, SN76496_0_w },
	{ 0xa000, 0xa003, SN76496_1_w },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x8000, 0x8003, congo_daio_w },
	{ 0x0000, 0x2000, MWA_ROM },
MEMORY_END



/***************************************************************************

  Congo Bongo uses NMI to trigger the self test. We use a fake input port
  to tie that event to a keypress.

***************************************************************************/
static INTERRUPT_GEN( congo_interrupt )
{
	if (readinputport(5) & 1)	/* get status of the F2 key */
		nmi_line_pulse(); /* trigger self test */
	else
		irq0_line_hold();
}

/* almost the same as Zaxxon; UP and DOWN are inverted, and the joystick is 4 way. */
INPUT_PORTS_START( congo )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )				// "South"
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )				// "North"
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )				// "East"
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )				// "West"
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )							// "Jump"
	PORT_DIPNAME( 0x20, 0x00, "Test Back and Target" )						// Only checked in "test mode"
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )								// (check code at 0x7dc1)
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )	// "South"
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )	// "North"
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )	// "East"
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )	// "West"
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )				// "Jump"
	PORT_DIPNAME( 0x20, 0x00, "Test Input, Output and Dip SW" )					// Only checked in "test mode"
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )								// (check code at 0x7be9)
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	/* the coin inputs must stay active for exactly one frame, otherwise the game will keep inserting coins. */
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_HIGH, IPT_COIN2, 1 )
	/* Coin Aux doesn't need IMPULSE to pass the test, but it still needs it to avoid the freeze. */
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE1, 1 )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x03, "10000" )
	PORT_DIPSETTING(    0x01, "20000" )
	PORT_DIPSETTING(    0x02, "30000" )
	PORT_DIPSETTING(    0x00, "40000" )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0c, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
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
	PORT_DIPNAME( 0x0f, 0x03, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, "2C/1C 5C/3C 6C/4C" )
	PORT_DIPSETTING(    0x0a, "2C/1C 3C/2C 4C/3C" )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, "1C/1C 5C/6C" )
	PORT_DIPSETTING(    0x0c, "1C/1C 4C/5C" )
	PORT_DIPSETTING(    0x04, "1C/1C 2C/3C" )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, "1C/2C 5C/11C" )
	PORT_DIPSETTING(    0x00, "1C/2C 4C/9C" )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0xf0, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x60, "2C/1C 5C/3C 6C/4C" )
	PORT_DIPSETTING(    0xa0, "2C/1C 3C/2C 4C/3C" )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, "1C/1C 5C/6C" )
	PORT_DIPSETTING(    0xc0, "1C/1C 4C/5C" )
	PORT_DIPSETTING(    0x40, "1C/1C 2C/3C" )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, "1C/2C 5C/11C" )
	PORT_DIPSETTING(    0x00, "1C/2C 4C/9C" )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_6C ) )

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
INPUT_PORTS_END

/* Same as Congo Bongo, except the Demo Sounds dip, that here turns the sound off in the whole game. */
INPUT_PORTS_START( tiptop )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )				// "South"
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )				// "North"
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )				// "East"
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )				// "West"
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )							// "Jump"
	PORT_DIPNAME( 0x20, 0x00, "Test Back and Target" )						// Only checked in "test mode"
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )								// (check code at 0x7dc1)
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )	// "South"
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )	// "North"
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )	// "East"
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )	// "West"
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )				// "Jump"
	PORT_DIPNAME( 0x20, 0x00, "Test Input, Output and Dip SW" )					// Only checked in "test mode"
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )								// (check code at 0x7be9)
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	/* the coin inputs must stay active for exactly one frame, otherwise the game will keep inserting coins. */
	PORT_BIT_IMPULSE( 0x20, IP_ACTIVE_HIGH, IPT_COIN1, 1 )
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_HIGH, IPT_COIN2, 1 )
	/* Coin Aux doesn't need IMPULSE to pass the test, but it still needs it to avoid the freeze. */
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE1, 1 )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x03, "10000" )
	PORT_DIPSETTING(    0x01, "20000" )
	PORT_DIPSETTING(    0x02, "30000" )
	PORT_DIPSETTING(    0x00, "40000" )
	PORT_DIPNAME( 0x0c, 0x04, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0c, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Very Hard" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPNAME( 0x40, 0x40, "Sound" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x03, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, "2C/1C 5C/3C 6C/4C" )
	PORT_DIPSETTING(    0x0a, "2C/1C 3C/2C 4C/3C" )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, "1C/1C 5C/6C" )
	PORT_DIPSETTING(    0x0c, "1C/1C 4C/5C" )
	PORT_DIPSETTING(    0x04, "1C/1C 2C/3C" )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, "1C/2C 5C/11C" )
	PORT_DIPSETTING(    0x00, "1C/2C 4C/9C" )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0xf0, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x60, "2C/1C 5C/3C 6C/4C" )
	PORT_DIPSETTING(    0xa0, "2C/1C 3C/2C 4C/3C" )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, "1C/1C 5C/6C" )
	PORT_DIPSETTING(    0xc0, "1C/1C 4C/5C" )
	PORT_DIPSETTING(    0x40, "1C/1C 2C/3C" )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, "1C/2C 5C/11C" )
	PORT_DIPSETTING(    0x00, "1C/2C 4C/9C" )
	PORT_DIPSETTING(    0x50, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_6C ) )

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
INPUT_PORTS_END


static struct GfxLayout spritelayout =
{
	32,32,	/* 32*32 sprites */
	128,	/* 128 sprites */
	3,	/* 3 bits per pixel */
	{ 2*128*128*8, 128*128*8, 0 },	  /* the bitplanes are separated */
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
	{ REGION_GFX1, 0, &zaxxon_charlayout1,	0, 32 },	/* characters */
	{ REGION_GFX2, 0, &zaxxon_charlayout2,	0, 32 },	/* background tiles */
	{ REGION_GFX3, 0, &spritelayout,		0, 32 },	/* sprites */
	{ -1 } /* end of array */
};



static struct SN76496interface sn76496_interface =
{
	2,	/* 2 chips */
	{ 4000000, 4000000 },	/* 4 MHz??? */
	{ 100, 100 }
};

/* Samples for Congo Bongo, these are needed for now	*/
/* as I haven't gotten around to calculate a formula for the */
/* simple oscillators producing the drums VL*/
/* As for now, thanks to Tim L. for providing samples */

static const char *congo_sample_names[] =
{
	"*congo",
	"gorilla.wav",
	"bass.wav",
	"congaa.wav",
	"congab.wav",
	"rim.wav",
	0
};

static struct Samplesinterface samples_interface =
{
	5,	/* 5 channels */
	25, /* volume */
	congo_sample_names
};



static MACHINE_DRIVER_START( congo )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 3072000)	/* 3.072 MHz ?? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(congo_interrupt,1)

	MDRV_CPU_ADD(Z80, 2000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sh_readmem,sh_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(congo)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1,2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(32*8)

	MDRV_PALETTE_INIT(zaxxon)
	MDRV_VIDEO_START(zaxxon)
	MDRV_VIDEO_UPDATE(zaxxon)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
	MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( congo )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "congo1.bin",   0x0000, 0x2000, CRC(09355b5b) SHA1(0085ac7eb0035a88cb54cdd3dd6b2643141d39db) )
	ROM_LOAD( "congo2.bin",   0x2000, 0x2000, CRC(1c5e30ae) SHA1(7cc5420e0e7a2793a671b938c121ae4079f5b1b8) )
	ROM_LOAD( "congo3.bin",   0x4000, 0x2000, CRC(5ee1132c) SHA1(26294cd69ee43dfd29fc3642e8c04552dcdbaa49) )
	ROM_LOAD( "congo4.bin",   0x6000, 0x2000, CRC(5332b9bf) SHA1(8440cc6f92918b3b467a5a0b86c9defeb0a7db0e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /*64K for the sound cpu*/
	ROM_LOAD( "congo17.bin",  0x0000, 0x2000, CRC(5024e673) SHA1(6f846146a4e29bcdfd5bd1bc5f1211d344cd5afa) )

	ROM_REGION( 0x1800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "congo5.bin",   0x00000, 0x1000, CRC(7bf6ba2b) SHA1(3a2bd21b0e0e55cbd737c7b075492b5e8f944150) ) /* characters */
	/* 1000-1800 empty space to convert the characters as 3bpp instead of 2 */

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "congo8.bin",   0x00000, 0x2000, CRC(db99a619) SHA1(499029197d26f9aea3ac15d66b5738ce7dea1f6c) ) /* background tiles */
	ROM_LOAD( "congo9.bin",   0x02000, 0x2000, CRC(93e2309e) SHA1(bd8a74332cac0cf85f319c1f35d04a4781c9d655) )
	ROM_LOAD( "congo10.bin",  0x04000, 0x2000, CRC(f27a9407) SHA1(d41c90c89ae28c92bf0c57927357d9b68ed7e0ef) )

	ROM_REGION( 0xc000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "congo12.bin",  0x00000, 0x2000, CRC(15e3377a) SHA1(04a7fbfd58924359fae0ba76ed152f325f07beae) ) /* sprites */
	ROM_LOAD( "congo13.bin",  0x02000, 0x2000, CRC(1d1321c8) SHA1(d12e156a24db105c5f941b7ef79f32181b616710) )
	ROM_LOAD( "congo11.bin",  0x04000, 0x2000, CRC(73e2709f) SHA1(14919facf08f6983c3a9baad031239a1b57c8202) )
	ROM_LOAD( "congo14.bin",  0x06000, 0x2000, CRC(bf9169fe) SHA1(303d68e38e9a47464f14dc5be6bff1be01b88bb6) )
	ROM_LOAD( "congo16.bin",  0x08000, 0x2000, CRC(cb6d5775) SHA1(b1f8ead6e6f8ad995baaeb7f8554d41ed2296fff) )
	ROM_LOAD( "congo15.bin",  0x0a000, 0x2000, CRC(7b15a7a4) SHA1(b1c05e60a1442e4dd56d197be8b768bcbf45e2d9) )

	ROM_REGION( 0x8000, REGION_GFX4, ROMREGION_DISPOSE )	/* background tilemaps converted in vh_start */
	ROM_LOAD( "congo6.bin",   0x0000, 0x2000, CRC(d637f02b) SHA1(29127149924c5bfdeb9456d7df2a5a5d14098794) )
	/* 2000-3fff empty space to match Zaxxon */
	ROM_LOAD( "congo7.bin",   0x4000, 0x2000, CRC(80927943) SHA1(4683520c241d209c6cabeaead9b363f046c30f70) )
	/* 6000-7fff empty space to match Zaxxon */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "congo.u68",    0x0000, 0x100, CRC(b788d8ae) SHA1(9765180f3087140c75e5953409df841787558160) ) /* palette */
ROM_END

ROM_START( tiptop )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "tiptop1.bin",  0x0000, 0x2000, CRC(e19dc77b) SHA1(d3782dd55701e0f5cd426ad2771c1bd0264c366a) )
	ROM_LOAD( "tiptop2.bin",  0x2000, 0x2000, CRC(3fcd3b6e) SHA1(2898807ee36fca7fbc06616c9a070604beb782b9) )
	ROM_LOAD( "tiptop3.bin",  0x4000, 0x2000, CRC(1c94250b) SHA1(cb70a91d07b0a9c61a093f1b5d37f2e69d1345c1) )
	ROM_LOAD( "tiptop4.bin",  0x6000, 0x2000, CRC(577b501b) SHA1(5cad98a60a5241ba9467aa03fcd94c7490e6dbbb) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /*64K for the sound cpu*/
	ROM_LOAD( "congo17.bin",  0x0000, 0x2000, CRC(5024e673) SHA1(6f846146a4e29bcdfd5bd1bc5f1211d344cd5afa) )

	ROM_REGION( 0x1800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "congo5.bin",   0x00000, 0x1000, CRC(7bf6ba2b) SHA1(3a2bd21b0e0e55cbd737c7b075492b5e8f944150) ) /* characters */
	/* 1000-1800 empty space to convert the characters as 3bpp instead of 2 */

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "congo8.bin",   0x00000, 0x2000, CRC(db99a619) SHA1(499029197d26f9aea3ac15d66b5738ce7dea1f6c) ) /* background tiles */
	ROM_LOAD( "congo9.bin",   0x02000, 0x2000, CRC(93e2309e) SHA1(bd8a74332cac0cf85f319c1f35d04a4781c9d655) )
	ROM_LOAD( "congo10.bin",  0x04000, 0x2000, CRC(f27a9407) SHA1(d41c90c89ae28c92bf0c57927357d9b68ed7e0ef) )

	ROM_REGION( 0xc000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "congo12.bin",  0x00000, 0x2000, CRC(15e3377a) SHA1(04a7fbfd58924359fae0ba76ed152f325f07beae) ) /* sprites */
	ROM_LOAD( "congo13.bin",  0x02000, 0x2000, CRC(1d1321c8) SHA1(d12e156a24db105c5f941b7ef79f32181b616710) )
	ROM_LOAD( "congo11.bin",  0x04000, 0x2000, CRC(73e2709f) SHA1(14919facf08f6983c3a9baad031239a1b57c8202) )
	ROM_LOAD( "congo14.bin",  0x06000, 0x2000, CRC(bf9169fe) SHA1(303d68e38e9a47464f14dc5be6bff1be01b88bb6) )
	ROM_LOAD( "congo16.bin",  0x08000, 0x2000, CRC(cb6d5775) SHA1(b1f8ead6e6f8ad995baaeb7f8554d41ed2296fff) )
	ROM_LOAD( "congo15.bin",  0x0a000, 0x2000, CRC(7b15a7a4) SHA1(b1c05e60a1442e4dd56d197be8b768bcbf45e2d9) )

	ROM_REGION( 0x8000, REGION_GFX4, ROMREGION_DISPOSE )	/* background tilemaps converted in vh_start */
	ROM_LOAD( "congo6.bin",   0x0000, 0x2000, CRC(d637f02b) SHA1(29127149924c5bfdeb9456d7df2a5a5d14098794) )
	/* 2000-3fff empty space to match Zaxxon */
	ROM_LOAD( "congo7.bin",   0x4000, 0x2000, CRC(80927943) SHA1(4683520c241d209c6cabeaead9b363f046c30f70) )
	/* 6000-7fff empty space to match Zaxxon */

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "congo.u68",    0x0000, 0x100, CRC(b788d8ae) SHA1(9765180f3087140c75e5953409df841787558160) ) /* palette */
ROM_END



GAME (1983, congo,  0,     congo, congo,  0, ROT90, "Sega", "Congo Bongo" )
GAME (1983, tiptop, congo, congo, tiptop, 0, ROT90, "Sega", "Tip Top" )

