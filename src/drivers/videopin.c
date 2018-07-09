/***************************************************************************

	Atari Video Pinball hardware

	driver by Sebastien Monassa
	Questions, comments, etc to smonassa@mail.dotcom.fr

	Games supported:
		* Video Pinball

	Known issues:
		o Debug variable force plunger and false ball movement
		o Is nudge effect working ?
		o Sound (hum...)
		o LEDs (possible ?, depends on backdrop for exact coordinates)
		o High score saving/loading
		o Get a good and final Artwork backdrop to adjust display and leds finally

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "artwork.h"
#include "videopin.h"



/*************************************
 *
 *	Palette generation
 *
 *************************************/

static PALETTE_INIT( videopin )
{
	palette_set_color(0,0x00,0x00,0x00); /* BLACK (transparent) */
	palette_set_color(1,0xff,0xff,0xff);  /* WHITE */
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( videopin_readmem )
	{ 0x0000, 0x01ff, MRA_RAM },        /* working   RAM 512  bytes */
	{ 0x0200, 0x07ff, MRA_RAM },        /* playfield RAM 1,5 Kbytes */
	{ 0x0800, 0x0800, videopin_in2_r }, /* VBLANK, NUDGE, PLUNGER1, PLUNGER2 */
	{ 0x1000, 0x1000, videopin_in0_r }, /* IN0 Switches */
	{ 0x1800, 0x1800, videopin_in1_r }, /* IN1 DSW */
	{ 0x2000, 0x3fff, MRA_ROM },        /* PROM */
	{ 0xfff0, 0xffff, MRA_ROM },        /* PROM for 6502 vectors */
MEMORY_END


static MEMORY_WRITE_START( videopin_writemem )
	{ 0x0000, 0x01ff, MWA_RAM },                  /* working RAM */
	{ 0x0200, 0x07ff, videoram_w, &videoram, &videoram_size },
	                                              /* playfield RAM */
	{ 0x0800, 0x0800, videopin_note_dvslrd_w },   /* No sound yet, audio frequency load (NOTE DVSRLD) */
	{ 0x0801, 0x0801, videopin_led_w },           /* LED write (LED WR) */
	{ 0x0802, 0x0802, watchdog_reset_w },         /* watchdog counter clear (WATCHDOG) */
	{ 0x0804, 0x0804, videopin_ball_position_w }, /* video ball position (BALL POSITION) */
	{ 0x0805, 0x0805, videopin_out1_w },          /* NMI MASK, lockout coil, audio frequency (OUT 1) */
	{ 0x0806, 0x0806, videopin_out2_w },          /* audio disable during attract, bell audio gen enable,
                                                     bong audio gen enable, coin counter,
													 audio volume select (OUT 2) */
	{ 0x2000, 0x3fff, MWA_ROM }, /* PROM */
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( videopin )
	PORT_START		/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )   /* Right coin 1 */
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )   /* Left coin 2 */
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) /* Right flipper */
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) /* Left flipper */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_START1 )  /* No start2 button */
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* What the hell is Slam switch ??? */
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )

	PORT_START		/* IN1/DSW */
	PORT_DIPNAME( 0xC0, 0x80, "Coin Mode" )
	PORT_DIPSETTING(	0xC0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x80, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x30, 0x00, "Language" )
	PORT_DIPSETTING(	0x00, "English" )
	PORT_DIPSETTING(	0x10, "German" )
	PORT_DIPSETTING(	0x20, "French" )
	PORT_DIPSETTING(	0x30, "Spanish" )
	PORT_DIPNAME( 0x08, 0x08, "Balls" )
	PORT_DIPSETTING(	0x00, "5" )
	PORT_DIPSETTING(	0x08, "3" )
	PORT_DIPNAME( 0x04, 0x00, "Replay" )
	PORT_DIPSETTING(	0x00, "Special 1 Replay" )
	PORT_DIPSETTING(	0x04, "80000 Points" )
	PORT_DIPNAME( 0x01, 0x01, "Replay Points" )
	PORT_DIPSETTING(	0x00, "3b 180000/5b 300000" )
	PORT_DIPSETTING(	0x01, "3b 210000/5b 350000" )
	PORT_DIPNAME( 0x02, 0x00, "Extra Ball" )
	PORT_DIPSETTING(	0x00, "Extra Ball" )
	PORT_DIPSETTING(	0x02, "50000 Points" )

	PORT_START		/* IN2 VBLANK, NUDGE, PLUNGER1, PLUNGER2 */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* PLUNGER1 */
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL ) /* PLUNGER2 */
											  /* Ball shooter force = PLUNGER2 - PLUNGER1 */
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 ) /* NUDGE */
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK ) /* VBLANK */

	PORT_START		/* FAKE to read the plunger button */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON4 ) /* PLUNGER1 */
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout videopin_charlayout =
{
	8,8,
	64,
	1,
	{ 0 },
	{  4, 5, 6, 7, 0x200*8 + 4, 0x200*8 + 5, 0x200*8 + 6, 0x200*8 + 7},
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxLayout videopin_balllayout =
{
	8,8,
	4,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxDecodeInfo videopin_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &videopin_charlayout,  0x00, 0x01 }, /* offset into colors, # of colors */
	{ 1, 0x0400, &videopin_balllayout, 0x00, 0x01 }, /* offset into colors, # of colors */
	{ -1 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( videopin )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502,750000)	/* 12000000/16 = 12MHz/16 = 750 KHz not sure about the frequency ??? */
	MDRV_CPU_MEMORY(videopin_readmem,videopin_writemem)
	MDRV_CPU_VBLANK_INT(videopin_interrupt,8)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(45*8, 39*8)
	MDRV_VISIBLE_AREA(0*8, 37*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(videopin_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(videopin)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(videopin)

	/* sound hardware */
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( videopin )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64Kb code roms, the ROMs are nibble-wide */
    ROM_LOAD_NIB_LOW ( "34242-01.e0", 0x2000, 0x0400, CRC(c6a83795) SHA1(73a65cca7c1e337b336b7d515eafc2981e669be8) ) /*ROM0 */
	ROM_LOAD_NIB_HIGH( "34237-01.k0", 0x2000, 0x0400, CRC(9b5ef087) SHA1(4ecf441742e7c39237cd544b0f0d9339943e1a2c) ) /*ROM0 */
	ROM_LOAD_NIB_LOW ( "34243-01.d0", 0x2400, 0x0400, CRC(dc87d023) SHA1(1ecec121067a60b91b3912bd28737caaae463167) ) /*ROM4 */
	ROM_LOAD_NIB_HIGH( "34238-01.j0", 0x2400, 0x0400, CRC(280d9e67) SHA1(229cc0448bb95f86fc7acbcb9594bc313f316580) ) /*ROM4 */
	ROM_LOAD_NIB_LOW ( "34250-01.h1", 0x2800, 0x0400, CRC(26fdd5a3) SHA1(a5f1624b36f58fcdfc7c6c04784340bb08a89785) ) /*ROM1 */
	ROM_LOAD_NIB_HIGH( "34249-01.h1", 0x2800, 0x0400, CRC(923b3609) SHA1(1b9fc60b27ff80b0ec26d897ea1817f466269506) ) /*ROM1 */
	ROM_LOAD_NIB_LOW ( "34244-01.c0", 0x2c00, 0x0400, CRC(4c12a4b1) SHA1(4351887a8dada92cd24cfa5930456e7c5c251ceb) ) /*ROM5 */
	ROM_LOAD_NIB_HIGH( "34240-01.h0", 0x2c00, 0x0400, CRC(d487eff5) SHA1(45bbf7e693f5471ea25e6ec71ce34708335d2d0b) ) /*ROM5 */
	ROM_LOAD_NIB_LOW ( "34252-01.e1", 0x3000, 0x0400, CRC(4858d87a) SHA1(a50d7ef3d7a804defa25483768cf1a931dd799d5) ) /*ROM2 */
	ROM_LOAD_NIB_HIGH( "34247-01.k1", 0x3000, 0x0400, CRC(d3083368) SHA1(c2083edb0f424dbf02caeaf786ab572326ae48d0) ) /*ROM2 */
	ROM_LOAD_NIB_LOW ( "34246-01.a0", 0x3400, 0x0400, CRC(39ff2d49) SHA1(59221be088d783210516858a4272f7364e00e7b4) ) /*ROM6 */
	ROM_LOAD_NIB_HIGH( "34239-01.h0", 0x3400, 0x0400, CRC(692de455) SHA1(ccbed14cdbeaf23961c356dfac98c6c7fb022486) ) /*ROM6 */
	ROM_LOAD_NIB_LOW ( "34251-01.f1", 0x3800, 0x0400, CRC(5d416efc) SHA1(1debd835cc3e52f526fc0aab4955be7f3682b8c0) ) /*ROM3 */
	ROM_LOAD_NIB_HIGH( "34248-01.j1", 0x3800, 0x0400, CRC(9f120e95) SHA1(d7434f437137690873cba66b408ec8e92b6509c1) ) /*ROM3 */
	ROM_LOAD_NIB_LOW ( "34245-01.b0", 0x3c00, 0x0400, CRC(da02c194) SHA1(a4ec66c85f084286d13a9fc0b35ba5ad896bef44) ) /*ROM7 */
	ROM_RELOAD(                       0xfc00, 0x0400 ) /* for 6502 vectors */
	ROM_LOAD_NIB_HIGH( "34241-01.f0", 0x3c00, 0x0400, CRC(5bfb83da) SHA1(9f392b0d4a972b6ae15ec12913a7e66761f4175d) ) /*ROM7 */
	ROM_RELOAD(                       0xfc00, 0x0400 ) /* for 6502 vectors */

	ROM_REGION(0x520, REGION_GFX1, ROMREGION_DISPOSE )	  /* 1k for graphics: temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "34258-01.c5", 0x0000, 0x0200, CRC(91a5f117) SHA1(03ac6b0b3da0ed5faf1ba6695d16918d12ceeff5) )
	ROM_LOAD( "34259-01.d5", 0x0200, 0x0200, CRC(6cd98c06) SHA1(48bf077b7abbd2f529a19bdf85700b93014f39f9) )
	ROM_LOAD( "34257-01.m1", 0x0400, 0x0020, CRC(50245866) SHA1(b0692bc8d44f127f6e7182a1ce75a785e22ac5b9) ) /* 32bytes 16x16 space for 8x8 ball pix */
	ROM_LOAD( "9402-01.h4",  0x0420, 0x0100, CRC(b8094b4c) SHA1(82dc6799a19984f3b204ee3aeeb007e55afc8be3) ) /* sync (not used) */
ROM_END



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAMEX( 1979, videopin, 0, videopin, videopin, 0, ROT270, "Atari", "Video Pinball", GAME_NOT_WORKING | GAME_NO_SOUND )
