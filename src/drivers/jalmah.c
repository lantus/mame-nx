/* Jaleco Mahjong Games */
/* Board:	MJ-8956 */

/*

Board:	MJ-8956

CPU:	68000-8
	M50747 (not dumped)
Sound:	M6295
OSC:	12.000MHz
	4.000MHz


done:
rom loading
gfx decode

todo:

everything else
finish me :-)

similar to nmk16.c?

*/

/*

68k interrupts
lev 1 : 0x64 : 0000 049e -
lev 2 : 0x68 : 0000 049e -
lev 3 : 0x6c : 0000 049e -
lev 4 : 0x70 : 0000 091a -
lev 5 : 0x74 : 0000 0924 -
lev 6 : 0x78 : 0000 092e -
lev 7 : 0x7c : 0000 0938 -

*/

#include "driver.h"

static int respcount;

static READ16_HANDLER( daireika_mcu_r )
{
	static int resp[] = {	0x99, 0xd8, 0x00,
							0x2a, 0x6a, 0x00,
							0x9c, 0xd8, 0x00,
							0x2f, 0x6f, 0x00,
							0x22, 0x62, 0x00,
							0x25, 0x65, 0x00 };
	int res;

	res = resp[respcount++];
	if (respcount >= sizeof(resp)/sizeof(resp[0])) respcount = 0;

logerror("%04x: mcu_r %02x\n",activecpu_get_pc(),res);

	return res;
}

static MACHINE_INIT (daireika)
{
	respcount = 0;
}

VIDEO_START(jalmah)
{
	return 0;
}


VIDEO_UPDATE(jalmah)
{

}

INPUT_PORTS_START( jalmah )
INPUT_PORTS_END

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080004, 0x080005, daireika_mcu_r },
	{ 0x0f0000, 0x0fffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x0f0000, 0x0fffff, MWA16_RAM },
MEMORY_END

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			16*32+0*4, 16*32+1*4, 16*32+2*4, 16*32+3*4, 16*32+4*4, 16*32+5*4, 16*32+6*4, 16*32+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	32*32
};

static struct GfxDecodeInfo jalmah_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x000, 16 },
	{ REGION_GFX2, 0, &tilelayout, 0x000, 16 },
	{ REGION_GFX3, 0, &tilelayout, 0x000, 16 },
	{ REGION_GFX4, 0, &tilelayout, 0x000, 16 },
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( jalmah )
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_MEMORY(readmem,writemem)
//	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)


	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(jalmah_gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x300)
	MDRV_MACHINE_INIT ( daireika )

	MDRV_VIDEO_START(jalmah)
	MDRV_VIDEO_UPDATE(jalmah)

MACHINE_DRIVER_END




/*

Mahjong Daireikai (JPN Ver.)
(c)1989 Jaleco / NMK

*/

ROM_START( daireika )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "mj1.bin", 0x00001, 0x20000, CRC(3b4e8357) )
	ROM_LOAD16_BYTE( "mj2.bin", 0x00000, 0x20000, CRC(c54d2f9b) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "mj3.bin", 0x00000, 0x80000, CRC(65bb350c) )

	ROM_REGION( 0x10000, REGION_GFX1, 0 ) /* BG0 */
	ROM_LOAD( "mj14.bin", 0x00000, 0x10000, CRC(c84c5577) )

	ROM_REGION( 0x10000, REGION_GFX2, 0 ) /* BG1 */
	ROM_LOAD( "mj13.bin", 0x00000, 0x10000, CRC(c54bca14) )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* BG2 */
	ROM_LOAD( "mj12.bin", 0x00000, 0x20000, CRC(236f809f) )
	ROM_LOAD( "mj11.bin", 0x20000, 0x20000, CRC(14867c51) )

	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* BG3 */
	ROM_LOAD( "mj10.bin", 0x00000, 0x80000, CRC(1f5509a5) )

	ROM_REGION( 0x220, REGION_USER1, 0 ) /* Proms */
	ROM_LOAD( "mj15.bpr", 0x000, 0x100, CRC(ebac41f9) )
	ROM_LOAD( "mj16.bpr", 0x100, 0x100, CRC(8d5dc1f6) )
	ROM_LOAD( "mj17.bpr", 0x200, 0x020, CRC(a17c3e8a) )
ROM_END

/*

Mahjong Channel Zoom In (JPN Ver.)
(c)1990 Jaleco

*/

ROM_START( mjzoomin )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "zoomin-1.bin", 0x00001, 0x20000, CRC(b8b04d30) )
	ROM_LOAD16_BYTE( "zoomin-2.bin", 0x00000, 0x20000, CRC(c7eb982c) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "zoomin-3.bin", 0x00000, 0x80000, CRC(07d7b8cd) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) /* BG0 */
	ROM_LOAD( "zoomin14.bin", 0x00000, 0x20000, CRC(4e32aa45) )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* BG1 */
	ROM_LOAD( "zoomin13.bin", 0x00000, 0x20000, CRC(888d79fe) )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* BG2 */
	ROM_LOAD( "zoomin12.bin", 0x00000, 0x40000, CRC(b0b94554) )

	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* BG3 */
	ROM_LOAD( "zoomin10.bin", 0x00000, 0x80000, CRC(40aec575) )

	ROM_REGION( 0x220, REGION_USER1, 0 ) /* Proms */
	ROM_LOAD( "mj15.bpr", 0x000, 0x100, CRC(ebac41f9) )
	ROM_LOAD( "mj16.bpr", 0x100, 0x100, CRC(8d5dc1f6) )
	ROM_LOAD( "mj17.bpr", 0x200, 0x020, CRC(a17c3e8a) )
ROM_END

/*

Mahjong Kakumei (JPN Ver.)
(c)1990 Jaleco


*/

ROM_START( kakumei )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "mj-re-1.bin", 0x00001, 0x20000, CRC(b90215be) )
	ROM_LOAD16_BYTE( "mj-re-2.bin", 0x00000, 0x20000, CRC(37eff266) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "rom3.bin", 0x00000, 0x40000, CRC(c9b7a526) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) /* BG0 */
	ROM_LOAD( "rom14.bin", 0x00000, 0x20000, CRC(63e88dd6) )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* BG1 */
	ROM_LOAD( "rom13.bin", 0x00000, 0x20000, CRC(9bef4fc2) )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* BG2 */
	ROM_LOAD( "rom12.bin", 0x00000, 0x40000, CRC(31620a61) )

	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* BG3 */
	ROM_LOAD( "rom10.bin", 0x00000, 0x80000, CRC(88366377) )

	ROM_REGION( 0x220, REGION_USER1, 0 ) /* Proms */
	ROM_LOAD( "mj15.bpr", 0x000, 0x100, CRC(ebac41f9) )
	ROM_LOAD( "mj16.bpr", 0x100, 0x100, CRC(8d5dc1f6) )
	ROM_LOAD( "mj17.bpr", 0x200, 0x020, CRC(a17c3e8a) )
ROM_END

/*

Mahjong Kakumei2 Princess League (JPN Ver.)
(c)1992 Jaleco

*/

ROM_START( kakumei2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "mj-8956.1", 0x00001, 0x40000, CRC(db4ce32f) )
	ROM_LOAD16_BYTE( "mj-8956.2", 0x00000, 0x40000, CRC(0f942507) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "92000-01.3", 0x00000, 0x80000, CRC(4b0ed440) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 ) /* BG0 */
	ROM_LOAD( " mj-8956.14", 0x00000, 0x20000, CRC(2b2fe999) )

	ROM_REGION( 0x20000, REGION_GFX2, 0 ) /* BG1 */
	ROM_LOAD( "mj-8956.13", 0x00000, 0x20000, CRC(afe93cf4) )

	ROM_REGION( 0x20000, REGION_GFX3, 0 ) /* BG2 */
	ROM_LOAD( "mj-8956.12", 0x00000, 0x20000, CRC(43f7853d) )

	ROM_REGION( 0x80000, REGION_GFX4, 0 ) /* BG3 */
	ROM_LOAD( "92000-02.10", 0x00000, 0x80000, CRC(338fa9b2) )

	ROM_REGION( 0x220, REGION_USER1, 0 ) /* Proms */
	ROM_LOAD( "mj15.bpr", 0x000, 0x100, CRC(ebac41f9) )
	ROM_LOAD( "mj16.bpr", 0x100, 0x100, CRC(8d5dc1f6) )
	ROM_LOAD( "mj17.bpr", 0x200, 0x020, CRC(a17c3e8a) )
ROM_END



GAMEX( 1989, daireika, 0, jalmah, jalmah, 0, ROT0, "Jaleco / NMK", "Mahjong Daireikai", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 1990, mjzoomin, 0, jalmah, jalmah, 0, ROT0, "Jaleco",       "Mahjong Channel Zoom In", GAME_NO_SOUND | GAME_NOT_WORKING  )
GAMEX( 1990, kakumei,  0, jalmah, jalmah, 0, ROT0, "Jaleco",       "Mahjong Kakumei", GAME_NO_SOUND | GAME_NOT_WORKING  )
GAMEX( 1992, kakumei2, 0, jalmah, jalmah, 0, ROT0, "Jaleco",       "Mahjong Kakumei 2 - Princess League", GAME_NO_SOUND | GAME_NOT_WORKING  )
