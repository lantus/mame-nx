/*
 * Signetics 2650 CPU Games
 *
 * Zaccaria - The Invaders
 * Zaccaria - Super Invader Attack
 * Cinematronics - Embargo
 *
 * Mame@btinternet.com
 */

#include "driver.h"
#include "8080bw.h"
#include "artwork.h"
#include "vidhrdw/generic.h"
#include "cpu/s2650/s2650.h"

extern UINT8 *s2636ram;

extern WRITE_HANDLER( tinvader_videoram_w );
extern WRITE_HANDLER( zac_s2636_w );
extern WRITE_HANDLER( tinvader_sound_w );
extern READ_HANDLER( zac_s2636_r );
extern READ_HANDLER( tinvader_port_0_r );

extern VIDEO_START( tinvader );
extern VIDEO_UPDATE( tinvader );

#define WHITE           MAKE_ARGB(0x04,0xff,0xff,0xff)
#define GREEN 			MAKE_ARGB(0x04,0x20,0xff,0x20)
#define PURPLE			MAKE_ARGB(0x04,0xff,0x20,0xff)

OVERLAY_START( tinv2650_overlay )
	OVERLAY_RECT(   0,   0, 240, 256, WHITE )
	OVERLAY_RECT(  16,   0,  72, 256, GREEN )
	OVERLAY_RECT(   0,  48,  16, 134, GREEN )
	OVERLAY_RECT( 192,   0, 209, 256, PURPLE )
OVERLAY_END



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x17ff, MRA_ROM },
    { 0x1800, 0x1bff, MRA_RAM },
    { 0x1E80, 0x1E80, tinvader_port_0_r },
    { 0x1E81, 0x1E81, input_port_1_r },
    { 0x1E82, 0x1E82, input_port_2_r },
    { 0x1D00, 0x1Dff, MRA_RAM },
    { 0x1F00, 0x1FFF, zac_s2636_r },			/* S2636 Chip */
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x17FF, MWA_ROM },
	{ 0x1800, 0x1bff, tinvader_videoram_w, &videoram },
    { 0x1D00, 0x1dff, MWA_RAM },
    { 0x1E80, 0x1E80, tinvader_sound_w },
    { 0x1F00, 0x1FFF, zac_s2636_w, &s2636ram },
MEMORY_END

static PORT_READ_START( readport )
    { S2650_SENSE_PORT, S2650_SENSE_PORT, input_port_3_r },
PORT_END

INPUT_PORTS_START( tinvader )

	PORT_START /* 1E80 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Missile-Background Collision */

    PORT_START /* 1E81 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
    PORT_DIPNAME( 0x02, 0x00, "Lightning Speed" )	/* Velocita Laser Inv */
	PORT_DIPSETTING(    0x00, "Slow" )
	PORT_DIPSETTING(    0x02, "Fast" )
	PORT_DIPNAME( 0x1C, 0x04, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x0C, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x14, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(	0x18, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(	0x1C, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "1000" )
	PORT_DIPSETTING(    0x20, "1500" )
    PORT_DIPNAME( 0x40, 0x00, "Extended Play" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )

	PORT_START /* 1E82 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* SENSE */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

INPUT_PORTS_END

/* Almost identical, no number of bases selection */

INPUT_PORTS_START( sinvader )

	PORT_START /* 1E80 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED  )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Missile-Background Collision */

    PORT_START /* 1E81 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED  )
    PORT_DIPNAME( 0x02, 0x00, "Lightning Speed" )	/* Velocita Laser Inv */
	PORT_DIPSETTING(    0x00, "Slow" )
	PORT_DIPSETTING(    0x02, "Fast" )
	PORT_DIPNAME( 0x1C, 0x04, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x04, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(	0x0C, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(	0x10, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(	0x14, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(	0x18, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(	0x1C, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "1000" )
	PORT_DIPSETTING(    0x20, "1500" )
    PORT_DIPNAME( 0x40, 0x00, "Extended Play" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Yes ) )

	PORT_START /* 1E82 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* SENSE */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

INPUT_PORTS_END

INPUT_PORTS_START( embargo )
/* TODO */
INPUT_PORTS_END

static PALETTE_INIT( zac2650 )
{
	palette_set_color(0,0x00,0x00,0x00); /* BLACK */
	palette_set_color(1,0xff,0xff,0xff); /* WHITE */
	colortable[0] = 0;
	colortable[1] = 1;
	colortable[2] = 0;
	colortable[3] = 0;
}

static struct GfxLayout tinvader_character =
{
	8,8,
	128,
	1,
	{ 0 },
	{ 0,1,2,3,4,5,6,7 },
   	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


/* These are really 6x8, but overlay an 8x8 screen  */
/* so we stretch them slightly to occupy same space */

static struct GfxLayout s2636_character8 =
{
	8,8,
	16,
	1,
	{ 0 },
	{ 0,1,1,2,3,4,5,5 },
   	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout s2636_character16 =
{
	16,16,
	16,
	1,
	{ 0 },
	{ 0,0,1,1,1,2,2,2,3,3,3,4,4,4,5,5 },
   	{ 0*8,0*8,1*8,1*8,2*8,2*8,3*8,3*8,4*8,4*8,5*8,5*8,6*8,6*8,7*8,7*8 },
	8*8
};

static struct GfxDecodeInfo tinvader_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tinvader_character,  0, 2 },
  	{ REGION_CPU1, 0x1F00, &s2636_character8, 0, 2 },	/* dynamic */
  	{ REGION_CPU1, 0x1F00, &s2636_character16, 0, 2 },	/* dynamic */
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( tinvader )

	/* basic machine hardware */
	MDRV_CPU_ADD(S2650, 3800000/4/3)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,0)

	MDRV_FRAMES_PER_SECOND(55)
	MDRV_VBLANK_DURATION(1041)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(30*8, 32*8)
	MDRV_VISIBLE_AREA(0, 239, 0, 255)
	MDRV_GFXDECODE(tinvader_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2)
	MDRV_COLORTABLE_LENGTH(4)

	MDRV_PALETTE_INIT(zac2650)
	MDRV_VIDEO_START(tinvader)
	MDRV_VIDEO_UPDATE(tinvader)

	/* sound hardware */
MACHINE_DRIVER_END

WRITE_HANDLER( tinvader_sound_w )
{
    /* sounds are NOT the same as space invaders */

	logerror("Register %x = Data %d\n",data & 0xfe,data & 0x01);

    /* 08 = hit invader */
    /* 20 = bonus (extra base) */
    /* 40 = saucer */
	/* 84 = fire */
    /* 90 = die */
    /* c4 = hit saucer */
}

ROM_START( sia2650 )
	ROM_REGION( 0x2000, REGION_CPU1, 0 )
	ROM_LOAD( "42_1.bin",   0x0000, 0x0800, CRC(a85550a9) SHA1(3f1e6b8e61894ff997e31b9c5ff819aa4678394e) )
	ROM_LOAD( "44_2.bin",   0x0800, 0x0800, CRC(48d5a3ed) SHA1(7f6421ba8225d49c1038595517f31b076d566586) )
	ROM_LOAD( "46_3.bin",   0x1000, 0x0800, CRC(d766e784) SHA1(88c113855c4cde8cefbe862d3e5abf80bd17aaa0) )

	ROM_REGION( 0x400, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "06_inv.bin", 0x0000, 0x0400, CRC(7bfed23e) SHA1(f754f0a4d6c8f9812bf333c30fa433b63d49a750) )
ROM_END

ROM_START( tinv2650 )
	ROM_REGION( 0x2000, REGION_CPU1, 0 )
	ROM_LOAD( "42_1.bin",   0x0000, 0x0800, CRC(a85550a9) SHA1(3f1e6b8e61894ff997e31b9c5ff819aa4678394e) )
	ROM_LOAD( "44_2t.bin",  0x0800, 0x0800, CRC(083c8621) SHA1(d9b33d532903b0e6dee2357b9e3b329856505a73) )
	ROM_LOAD( "46_3t.bin",  0x1000, 0x0800, CRC(12c0934f) SHA1(9fd67d425c533b0e09b201301020639eb9e452f7) )

	ROM_REGION( 0x400, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "06_inv.bin", 0x0000, 0x0400, CRC(7bfed23e) SHA1(f754f0a4d6c8f9812bf333c30fa433b63d49a750) )
ROM_END

/*
 * Embargo
 *
 */

static MEMORY_READ_START( emb_readmem )
	{ 0x0000, 0x0fff, MRA_ROM },
    { 0x1e00, 0x3dff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( emb_writemem )
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x1e00, 0x1fff, MWA_RAM },
	{ 0x2000, 0x3dff, c8080bw_videoram_w, &videoram, &videoram_size },
MEMORY_END

static PORT_READ_START( emb_readport )
PORT_END

static MACHINE_DRIVER_START( embargo )

	/* basic machine hardware */
	MDRV_CPU_ADD(S2650, 625000)
	MDRV_CPU_MEMORY(emb_readmem,emb_writemem)
	MDRV_CPU_PORTS(emb_readport,0)

	MDRV_FRAMES_PER_SECOND(55)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-2, 0*8, 32*8-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_COLORTABLE_LENGTH(4)

	MDRV_PALETTE_INIT(zac2650)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(8080bw)

	/* sound hardware */
MACHINE_DRIVER_END

ROM_START( embargo )
	ROM_REGION( 0x8000, REGION_CPU1, 0 )
	ROM_LOAD( "emb1", 0x0000, 0x0200, CRC(00dcbc24) )
	ROM_LOAD( "emb2", 0x0200, 0x0200, CRC(e7069b11) )
	ROM_LOAD( "emb3", 0x0400, 0x0200, CRC(1af7a966) )
	ROM_LOAD( "emb4", 0x0600, 0x0200, CRC(d9c75da0) )
	ROM_LOAD( "emb5", 0x0800, 0x0200, CRC(15960b58) )
	ROM_LOAD( "emb6", 0x0a00, 0x0200, CRC(7ba23058) )
	ROM_LOAD( "emb7", 0x0c00, 0x0200, CRC(6d46a593) )
	ROM_LOAD( "emb8", 0x0e00, 0x0200, CRC(f0b00634) )
ROM_END



static DRIVER_INIT( tinvader )
{
	artwork_set_overlay(tinv2650_overlay);
}


GAMEX( 1978, sia2650,  0,       tinvader, sinvader, 0,        ROT270, "Zaccaria/Zelco", "Super Invader Attack", GAME_NO_SOUND )
GAMEX( 1978, tinv2650, sia2650, tinvader, tinvader, tinvader, ROT270, "Zaccaria/Zelco", "The Invaders", GAME_NO_SOUND )
GAMEX( 1977, embargo,  0,       embargo,  embargo,  8080bw,   ROT0,   "Cinematronics",  "Embargo", GAME_NO_SOUND )
