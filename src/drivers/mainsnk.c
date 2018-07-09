/* Main Event - SNK */

/*

is there some kind of video ram banking? see fg layer in attract with ring,
it seems to draw one thing then overwrite it with something else.

*/

#include "driver.h"

static struct tilemap *me_fg_tilemap;
data8_t *me_fgram;

static struct tilemap *me_bg_tilemap;
data8_t *me_bgram;

static void get_me_fg_tile_info(int tile_index)
{
	int code = (me_fgram[tile_index]);

	SET_TILE_INFO(
			0,
			code & 0xff,
			0,
			0)
}


WRITE_HANDLER( me_fgram_w )
{
	me_fgram[offset] = data;
	tilemap_mark_tile_dirty(me_fg_tilemap,offset);
}


static void get_me_bg_tile_info(int tile_index)
{
	int code = (me_bgram[tile_index]);

	SET_TILE_INFO(
			0,
			(code & 0xff) + 0x100,
			0,
			0)
}


WRITE_HANDLER( me_bgram_w )
{
	me_bgram[offset] = data;
	tilemap_mark_tile_dirty(me_bg_tilemap,offset);
}


VIDEO_START(mainsnk)
{

me_fg_tilemap = tilemap_create(get_me_fg_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,32, 32);
	tilemap_set_transparent_pen(me_fg_tilemap,15);

me_bg_tilemap = tilemap_create(get_me_bg_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE,8,8,32, 32);


return 0;
}

VIDEO_UPDATE(mainsnk)
{
tilemap_draw(bitmap,cliprect,me_bg_tilemap,0,0);

tilemap_draw(bitmap,cliprect,me_fg_tilemap,0,0);

}

static WRITE_HANDLER(c600_write)
{
usrintf_showmessage("%02x", data);
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0xbfff, MRA_ROM },

	{ 0xC000, 0xC000, input_port_0_r },
	{ 0xC100, 0xC100, input_port_1_r },
	{ 0xC200, 0xC200, input_port_2_r },
	{ 0xC300, 0xC300, input_port_3_r },

	{ 0xC500, 0xC500, input_port_4_r },

	{ 0xd800, 0xdbff, MRA_RAM },
	{ 0xdc00, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe3ff, MRA_RAM },
	{ 0xe400, 0xe7ff, MRA_RAM },
	{ 0xe800, 0xebff, MRA_RAM },
	{ 0xec00, 0xefff, MRA_RAM },
	{ 0xf000, 0xf3ff, MRA_RAM },
	{ 0xf400, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xfbff, MRA_RAM },
	{ 0xfc00, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },

	{ 0xC600, 0xC600, c600_write },
	{ 0xC700, 0xC700, MWA_NOP },  // sounds?

	{ 0xd800, 0xdbff, me_bgram_w, &me_bgram },
	{ 0xdc00, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe3ff, MWA_RAM },
	{ 0xe400, 0xe7ff, MWA_RAM },
	{ 0xe800, 0xebff, MWA_RAM },
	{ 0xec00, 0xefff, MWA_RAM },
	{ 0xf000, 0xf3ff, me_fgram_w, &me_fgram },
	{ 0xf400, 0xf7ff, MWA_RAM }, // some energy bars + rubbish ;-) ..
MEMORY_END

INPUT_PORTS_START( mainsnk )
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "0" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "0" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "0" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "0" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "0" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

static struct GfxLayout tile_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	256
};


static struct GfxLayout sprite_layout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3),RGN_FRAC(1,3),RGN_FRAC(2,3) },
	{ 7,6,5,4,3,2,1,0, 15,14,13,12,11,10,9,8 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	256
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0, &tile_layout,	0,  8 },
	{ REGION_GFX2, 0x0, &sprite_layout,	0, 16 },
	{ -1 }
};


static MACHINE_DRIVER_START( mainsnk)
	MDRV_CPU_ADD(Z80, 8000000)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

//	MDRV_CPU_ADD(Z80,8000000/2)
//	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 4 MHz ??? */
//	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)


	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START(mainsnk)
	MDRV_VIDEO_UPDATE(mainsnk)

MACHINE_DRIVER_END


ROM_START( mainsnk)
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "snk.p01",	0x0000, 0x2000, CRC(00db1ca2) )
	ROM_LOAD( "snk.p02",	0x2000, 0x2000, CRC(df5c86b5) )
	ROM_LOAD( "snk.p03",	0x4000, 0x2000, CRC(5c2b7bca) )
	ROM_LOAD( "snk.p04",	0x6000, 0x2000, CRC(68b4b2a1) )
	ROM_LOAD( "snk.p05",	0x8000, 0x2000, CRC(5a6265e8) )
	ROM_LOAD( "snk.p06",	0xa000, 0x2000, CRC(5f8a60a2) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "snk.p07",	0x0000, 0x2000, CRC(2c92a8d4) ) // rom is twice the size in the other set, is this one ok?

	ROM_REGION( 0x08000, REGION_GFX1, 0 )
	ROM_LOAD( "snk.p09",	0x06000, 0x2000, CRC(0ebcf837) )
	ROM_LOAD( "snk.p10",	0x04000, 0x2000, CRC(b5147a96) )
	ROM_LOAD( "snk.p11",	0x02000, 0x2000, CRC(3f6bc5ba) )
	ROM_LOAD( "snk.p12",	0x00000, 0x2000, CRC(ecf87eb7) )

	ROM_REGION( 0x12000, REGION_GFX2, 0 )
	ROM_LOAD( "snk.p13",	0x00000, 0x2000, CRC(2eb624a4) )
	ROM_LOAD( "snk.p16",	0x02000, 0x2000, CRC(dc502869) )
	ROM_LOAD( "snk.p19",	0x04000, 0x2000, CRC(58d566a1) )
	ROM_LOAD( "snk.p14",	0x06000, 0x2000, CRC(bb927d82) )
	ROM_LOAD( "snk.p17",	0x08000, 0x2000, CRC(66f60c32) )
	ROM_LOAD( "snk.p20",	0x0a000, 0x2000, CRC(d12c6333) )
	ROM_LOAD( "snk.p15",	0x0c000, 0x2000, CRC(d242486d) )
	ROM_LOAD( "snk.p18",	0x0e000, 0x2000, CRC(838b12a3) )
	ROM_LOAD( "snk.p21",	0x10000, 0x2000, CRC(8961a51e) )

	ROM_REGION( 0x1800, REGION_PROMS, 0 )
	ROM_LOAD( "main1.bin",  0x0000, 0x800, CRC(deb895c4) )
	ROM_LOAD( "main2.bin",  0x0800, 0x800, CRC(7c314c93) )
	ROM_LOAD( "main3.bin",  0x1000, 0x800, CRC(78b29dde) )
ROM_END

GAMEX( 1984, mainsnk,      0,          mainsnk, mainsnk, 0,          ROT0, "SNK", "Main Event (1984)", GAME_NO_SOUND | GAME_NOT_WORKING )

