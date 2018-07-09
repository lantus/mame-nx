/***************************************************************************

	Atari Toobin' hardware

	driver by Aaron Giles

	Games supported:
		* Toobin' (1988) [3 sets]

	Known bugs:
		* none at this time

****************************************************************************

	Memory map (TBA)

***************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "sndhrdw/atarijsa.h"
#include "toobin.h"



/*************************************
 *
 *	Statics
 *
 *************************************/

static data16_t *interrupt_scan;



/*************************************
 *
 *	Initialization & interrupts
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_scanline_int_state)
		newstate |= 1;
	if (atarigen_sound_int_state)
		newstate |= 2;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static MACHINE_INIT( toobin )
{
	atarigen_eeprom_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarijsa_reset();
}



/*************************************
 *
 *	Interrupt handlers
 *
 *************************************/

static WRITE16_HANDLER( interrupt_scan_w )
{
	int oldword = interrupt_scan[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);

	/* if something changed, update the word in memory */
	if (oldword != newword)
	{
		interrupt_scan[offset] = newword;
		atarigen_scanline_int_set(newword & 0x1ff);
	}
}



/*************************************
 *
 *	I/O read dispatch
 *
 *************************************/

static READ16_HANDLER( special_port1_r )
{
	int result = readinputport(1);
	if (atarigen_get_hblank()) result ^= 0x8000;
	if (atarigen_cpu_to_sound_ready) result ^= 0x2000;
	return result;
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0xc00000, 0xc09fff, MRA16_RAM },
	{ 0xc10000, 0xc107ff, MRA16_RAM },
	{ 0xff6000, 0xff6001, MRA16_NOP },		/* who knows? read at controls time */
	{ 0xff8800, 0xff8801, input_port_0_word_r },
	{ 0xff9000, 0xff9001, special_port1_r },
	{ 0xff9800, 0xff9801, atarigen_sound_r },
	{ 0xffa000, 0xffafff, atarigen_eeprom_r },
	{ 0xffc000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0xc00000, 0xc07fff, atarigen_playfield_large_w, &atarigen_playfield },
	{ 0xc08000, 0xc097ff, atarigen_alpha_w, &atarigen_alpha },
	{ 0xc09800, 0xc09fff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0xc10000, 0xc107ff, toobin_paletteram_w, &paletteram16 },
	{ 0xff8000, 0xff8001, watchdog_reset16_w },
	{ 0xff8100, 0xff8101, atarigen_sound_w },
	{ 0xff8300, 0xff8301, toobin_intensity_w },
	{ 0xff8340, 0xff8341, interrupt_scan_w, &interrupt_scan },
	{ 0xff8380, 0xff8381, toobin_slip_w, &atarimo_0_slipram },
	{ 0xff83c0, 0xff83c1, atarigen_scanline_int_ack_w },
	{ 0xff8400, 0xff8401, atarigen_sound_reset_w },
	{ 0xff8500, 0xff8501, atarigen_eeprom_enable_w },
	{ 0xff8600, 0xff8601, toobin_xscroll_w, &atarigen_xscroll },
	{ 0xff8700, 0xff8701, toobin_yscroll_w, &atarigen_yscroll },
	{ 0xffa000, 0xffafff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xffc000, 0xffffff, MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( toobin )
	PORT_START	/* ff8800 */
	PORT_BITX(0x0001, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2, "P2 R Paddle Forward", KEYCODE_L, IP_JOY_DEFAULT )
	PORT_BITX(0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2, "P2 L Paddle Forward", KEYCODE_J, IP_JOY_DEFAULT )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2, "P2 L Paddle Backward", KEYCODE_U, IP_JOY_DEFAULT )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2, "P2 R Paddle Backward", KEYCODE_O, IP_JOY_DEFAULT )
	PORT_BITX(0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1, "P1 R Paddle Forward", KEYCODE_D, IP_JOY_DEFAULT )
	PORT_BITX(0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1, "P1 L Paddle Forward", KEYCODE_A, IP_JOY_DEFAULT )
	PORT_BITX(0x0040, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1, "P1 L Paddle Backward", KEYCODE_Q, IP_JOY_DEFAULT )
	PORT_BITX(0x0080, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1, "P1 R Paddle Backward", KEYCODE_E, IP_JOY_DEFAULT )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1, "P1 Throw", KEYCODE_LCONTROL, IP_JOY_DEFAULT )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2, "P2 Throw", KEYCODE_RCONTROL, IP_JOY_DEFAULT )
	PORT_BIT( 0xfc00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* ff9000 */
	PORT_BIT( 0x03ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x1000, IP_ACTIVE_LOW )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	JSA_I_PORT	/* audio board port */
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout anlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};


static struct GfxLayout pflayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0, 4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};


static struct GfxLayout molayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0, 4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11, 16, 17, 18, 19, 24, 25, 26, 27 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32, 8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	8*64
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pflayout,     0, 16 },
	{ REGION_GFX2, 0, &molayout,   256, 16 },
	{ REGION_GFX3, 0, &anlayout,   512, 64 },
	{ -1 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( toobin )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68010, ATARI_CLOCK_32MHz/4)
	MDRV_CPU_MEMORY(main_readmem,main_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(toobin)
	MDRV_NVRAM_HANDLER(atarigen)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(64*8, 48*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 48*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(toobin)
	MDRV_VIDEO_UPDATE(toobin)

	/* sound hardware */
	MDRV_IMPORT_FROM(jsa_i_stereo_pokey)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( toobin )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "061-3133.bin", 0x00000, 0x10000, CRC(79a92d02) SHA1(72eebb96a3963f94558bb204e0afe08f2b4c1864) )
	ROM_LOAD16_BYTE( "061-3137.bin", 0x00001, 0x10000, CRC(e389ef60) SHA1(24861fe5eb49de852987993a905fefe4dd43b204) )
	ROM_LOAD16_BYTE( "061-3134.bin", 0x20000, 0x10000, CRC(3dbe9a48) SHA1(37fe2534fed5708a63995e53ea0cb1d2d23fc1b9) )
	ROM_LOAD16_BYTE( "061-3138.bin", 0x20001, 0x10000, CRC(a17fb16c) SHA1(ae0a2c675a88dfaafffe47971c46c83dc7552148) )
	ROM_LOAD16_BYTE( "061-3135.bin", 0x40000, 0x10000, CRC(dc90b45c) SHA1(78c648be8e0aec6d1be45f909f2e468f3b572957) )
	ROM_LOAD16_BYTE( "061-3139.bin", 0x40001, 0x10000, CRC(6f8a719a) SHA1(bca7280155a4c44f55b402aed59927343c651acc) )
	ROM_LOAD16_BYTE( "061-1136.bin", 0x60000, 0x10000, CRC(5ae3eeac) SHA1(583b6c3f61e8ad4d98449205fedecf3e21ee993c) )
	ROM_LOAD16_BYTE( "061-1140.bin", 0x60001, 0x10000, CRC(dacbbd94) SHA1(0e3a93f439ff9f3dd57ee13604be02e9c74c8eec) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "061-1114.bin", 0x10000, 0x4000, CRC(c0dcce1a) SHA1(285c13f08020cf5827eca2afcc2fa8a3a0a073e0) )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "061-1101.bin", 0x000000, 0x10000, CRC(02696f15) SHA1(51856c331c45d287e574e2e4013b62a6472ad720) )  /* bank 0 (4 bpp)*/
	ROM_LOAD( "061-1102.bin", 0x010000, 0x10000, CRC(4bed4262) SHA1(eda16ece14cb60012edbe006b2839986d082822e) )
	ROM_LOAD( "061-1103.bin", 0x020000, 0x10000, CRC(e62b037f) SHA1(9a2341b822265269c07b65c4bc0fbc760c1bd456) )
	ROM_LOAD( "061-1104.bin", 0x030000, 0x10000, CRC(fa05aee6) SHA1(db0dbf94ba1f2c1bb3ad55df2f38a71b4ecb38e4) )
	ROM_LOAD( "061-1105.bin", 0x040000, 0x10000, CRC(ab1c5578) SHA1(e80a1c7d2f279a523dcc9d943bd5a1ce75045d2e) )
	ROM_LOAD( "061-1106.bin", 0x050000, 0x10000, CRC(4020468e) SHA1(fa83e3d903d254c598fcbf120492ac77777ae31f) )
	ROM_LOAD( "061-1107.bin", 0x060000, 0x10000, CRC(fe6f6aed) SHA1(11bd17be3c9fe409db8268cb17515040bfd92ee2) )
	ROM_LOAD( "061-1108.bin", 0x070000, 0x10000, CRC(26fe71e1) SHA1(cac22f969c943e184a58d7bb62072f93273638de) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "061-1143.bin", 0x000000, 0x20000, CRC(211c1049) SHA1(fcf4d9321b2871723a10b7607069da83d3402723) )  /* bank 0 (4 bpp)*/
	ROM_LOAD( "061-1144.bin", 0x020000, 0x20000, CRC(ef62ed2c) SHA1(c2c21023b2f559b8a63e6ae9c59c33a3055cc465) )
	ROM_LOAD( "061-1145.bin", 0x040000, 0x20000, CRC(067ecb8a) SHA1(a42e4602e1de1cc83a30a901a9adb5519f426cff) )
	ROM_LOAD( "061-1146.bin", 0x060000, 0x20000, CRC(fea6bc92) SHA1(c502ab022fdafdef71a720237094fe95c3137d69) )
	ROM_LOAD( "061-1125.bin", 0x080000, 0x10000, CRC(c37f24ac) SHA1(341fab8244d8063a516a4a25d7ee778f708cd386) )
	ROM_RELOAD(               0x0c0000, 0x10000 )
	ROM_LOAD( "061-1126.bin", 0x090000, 0x10000, CRC(015257f0) SHA1(c5ae6a43b95ecb06a04ea877f435b1f925cff136) )
	ROM_RELOAD(               0x0d0000, 0x10000 )
	ROM_LOAD( "061-1127.bin", 0x0a0000, 0x10000, CRC(d05417cb) SHA1(5cbd54050364e82fe443ca2150c34fca84c42419) )
	ROM_RELOAD(               0x0e0000, 0x10000 )
	ROM_LOAD( "061-1128.bin", 0x0b0000, 0x10000, CRC(fba3e203) SHA1(5951571e6e64061e5448cae27af0cedd5bda2b1e) )
	ROM_RELOAD(               0x0f0000, 0x10000 )
	ROM_LOAD( "061-1147.bin", 0x100000, 0x20000, CRC(ca4308cf) SHA1(966970524915a0a5a77e3525e446b50ecde5b119) )
	ROM_LOAD( "061-1148.bin", 0x120000, 0x20000, CRC(23ddd45c) SHA1(8ee19e8982a928b56d6010f283fb2f720dc71cd6) )
	ROM_LOAD( "061-1149.bin", 0x140000, 0x20000, CRC(d77cd1d0) SHA1(148fa17c9b7a2453adf059325cb608073d1ef195) )
	ROM_LOAD( "061-1150.bin", 0x160000, 0x20000, CRC(a37157b8) SHA1(347cea600f28709fc3d942feb5cadce7def72dbb) )
	ROM_LOAD( "061-1129.bin", 0x180000, 0x10000, CRC(294aaa02) SHA1(69b42dfc444b2c9f2bb0fdcb96e2becb0df6226a) )
	ROM_RELOAD(               0x1c0000, 0x10000 )
	ROM_LOAD( "061-1130.bin", 0x190000, 0x10000, CRC(dd610817) SHA1(25f542ae4e7e77399d6df328edbc4cceb390db04) )
	ROM_RELOAD(               0x1d0000, 0x10000 )
	ROM_LOAD( "061-1131.bin", 0x1a0000, 0x10000, CRC(e8e2f919) SHA1(292dacb60db867cb2ae69942c7502af6526ab550) )
	ROM_RELOAD(               0x1e0000, 0x10000 )
	ROM_LOAD( "061-1132.bin", 0x1b0000, 0x10000, CRC(c79f8ffc) SHA1(6e90044755097387011e7cc04548bedb399b7cc3) )
	ROM_RELOAD(               0x1f0000, 0x10000 )

	ROM_REGION( 0x004000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "061-1142.bin", 0x000000, 0x04000, CRC(a6ab551f) SHA1(6a11e16f3965416c81737efcb81e751484ba5ace) )  /* alpha font */
ROM_END


ROM_START( toobin2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "061-2133.1j",  0x00000, 0x10000, CRC(2c3382e4) SHA1(39919e9b5b586b630e0581adabfe25d83b2bfaef) )
	ROM_LOAD16_BYTE( "061-2137.1f",  0x00001, 0x10000, CRC(891c74b1) SHA1(2f39d0e4934ccf48bb5fc0737f34fc5a65cfd903) )
	ROM_LOAD16_BYTE( "061-2134.2j",  0x20000, 0x10000, CRC(2b8164c8) SHA1(aeeaff9df9fda23b295b59efadf52160f084d256) )
	ROM_LOAD16_BYTE( "061-2138.2f",  0x20001, 0x10000, CRC(c09cadbd) SHA1(93598a512d17664c111e3d88397fde37a492b4a6) )
	ROM_LOAD16_BYTE( "061-2135.4j",  0x40000, 0x10000, CRC(90477c4a) SHA1(69b4bcf5c329d8710d0985ce3e45bd40a7102a91) )
	ROM_LOAD16_BYTE( "061-2139.4f",  0x40001, 0x10000, CRC(47936958) SHA1(ac7c99272f3b21d15e5673d2e8f206d60c32f4f9) )
	ROM_LOAD16_BYTE( "061-1136.bin", 0x60000, 0x10000, CRC(5ae3eeac) SHA1(583b6c3f61e8ad4d98449205fedecf3e21ee993c) )
	ROM_LOAD16_BYTE( "061-1140.bin", 0x60001, 0x10000, CRC(dacbbd94) SHA1(0e3a93f439ff9f3dd57ee13604be02e9c74c8eec) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "061-1114.bin", 0x10000, 0x4000, CRC(c0dcce1a) SHA1(285c13f08020cf5827eca2afcc2fa8a3a0a073e0) )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "061-1101.bin", 0x000000, 0x10000, CRC(02696f15) SHA1(51856c331c45d287e574e2e4013b62a6472ad720) )  /* bank 0 (4 bpp)*/
	ROM_LOAD( "061-1102.bin", 0x010000, 0x10000, CRC(4bed4262) SHA1(eda16ece14cb60012edbe006b2839986d082822e) )
	ROM_LOAD( "061-1103.bin", 0x020000, 0x10000, CRC(e62b037f) SHA1(9a2341b822265269c07b65c4bc0fbc760c1bd456) )
	ROM_LOAD( "061-1104.bin", 0x030000, 0x10000, CRC(fa05aee6) SHA1(db0dbf94ba1f2c1bb3ad55df2f38a71b4ecb38e4) )
	ROM_LOAD( "061-1105.bin", 0x040000, 0x10000, CRC(ab1c5578) SHA1(e80a1c7d2f279a523dcc9d943bd5a1ce75045d2e) )
	ROM_LOAD( "061-1106.bin", 0x050000, 0x10000, CRC(4020468e) SHA1(fa83e3d903d254c598fcbf120492ac77777ae31f) )
	ROM_LOAD( "061-1107.bin", 0x060000, 0x10000, CRC(fe6f6aed) SHA1(11bd17be3c9fe409db8268cb17515040bfd92ee2) )
	ROM_LOAD( "061-1108.bin", 0x070000, 0x10000, CRC(26fe71e1) SHA1(cac22f969c943e184a58d7bb62072f93273638de) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "061-1143.bin", 0x000000, 0x20000, CRC(211c1049) SHA1(fcf4d9321b2871723a10b7607069da83d3402723) )  /* bank 0 (4 bpp)*/
	ROM_LOAD( "061-1144.bin", 0x020000, 0x20000, CRC(ef62ed2c) SHA1(c2c21023b2f559b8a63e6ae9c59c33a3055cc465) )
	ROM_LOAD( "061-1145.bin", 0x040000, 0x20000, CRC(067ecb8a) SHA1(a42e4602e1de1cc83a30a901a9adb5519f426cff) )
	ROM_LOAD( "061-1146.bin", 0x060000, 0x20000, CRC(fea6bc92) SHA1(c502ab022fdafdef71a720237094fe95c3137d69) )
	ROM_LOAD( "061-1125.bin", 0x080000, 0x10000, CRC(c37f24ac) SHA1(341fab8244d8063a516a4a25d7ee778f708cd386) )
	ROM_RELOAD(               0x0c0000, 0x10000 )
	ROM_LOAD( "061-1126.bin", 0x090000, 0x10000, CRC(015257f0) SHA1(c5ae6a43b95ecb06a04ea877f435b1f925cff136) )
	ROM_RELOAD(               0x0d0000, 0x10000 )
	ROM_LOAD( "061-1127.bin", 0x0a0000, 0x10000, CRC(d05417cb) SHA1(5cbd54050364e82fe443ca2150c34fca84c42419) )
	ROM_RELOAD(               0x0e0000, 0x10000 )
	ROM_LOAD( "061-1128.bin", 0x0b0000, 0x10000, CRC(fba3e203) SHA1(5951571e6e64061e5448cae27af0cedd5bda2b1e) )
	ROM_RELOAD(               0x0f0000, 0x10000 )
	ROM_LOAD( "061-1147.bin", 0x100000, 0x20000, CRC(ca4308cf) SHA1(966970524915a0a5a77e3525e446b50ecde5b119) )
	ROM_LOAD( "061-1148.bin", 0x120000, 0x20000, CRC(23ddd45c) SHA1(8ee19e8982a928b56d6010f283fb2f720dc71cd6) )
	ROM_LOAD( "061-1149.bin", 0x140000, 0x20000, CRC(d77cd1d0) SHA1(148fa17c9b7a2453adf059325cb608073d1ef195) )
	ROM_LOAD( "061-1150.bin", 0x160000, 0x20000, CRC(a37157b8) SHA1(347cea600f28709fc3d942feb5cadce7def72dbb) )
	ROM_LOAD( "061-1129.bin", 0x180000, 0x10000, CRC(294aaa02) SHA1(69b42dfc444b2c9f2bb0fdcb96e2becb0df6226a) )
	ROM_RELOAD(               0x1c0000, 0x10000 )
	ROM_LOAD( "061-1130.bin", 0x190000, 0x10000, CRC(dd610817) SHA1(25f542ae4e7e77399d6df328edbc4cceb390db04) )
	ROM_RELOAD(               0x1d0000, 0x10000 )
	ROM_LOAD( "061-1131.bin", 0x1a0000, 0x10000, CRC(e8e2f919) SHA1(292dacb60db867cb2ae69942c7502af6526ab550) )
	ROM_RELOAD(               0x1e0000, 0x10000 )
	ROM_LOAD( "061-1132.bin", 0x1b0000, 0x10000, CRC(c79f8ffc) SHA1(6e90044755097387011e7cc04548bedb399b7cc3) )
	ROM_RELOAD(               0x1f0000, 0x10000 )

	ROM_REGION( 0x004000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "061-1142.bin", 0x000000, 0x04000, CRC(a6ab551f) SHA1(6a11e16f3965416c81737efcb81e751484ba5ace) )  /* alpha font */
ROM_END


ROM_START( toobinp )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "pg-0-up.1j",   0x00000, 0x10000, CRC(caeb5d1b) SHA1(8036871a04b5206fd383ac0fd9a9d3218128088b) )
	ROM_LOAD16_BYTE( "pg-0-lo.1f",   0x00001, 0x10000, CRC(9713d9d3) SHA1(55791150312de201bdd330bfd4cbb132cb3959e4) )
	ROM_LOAD16_BYTE( "pg-20-up.2j",  0x20000, 0x10000, CRC(119f5d7b) SHA1(edd0b1ab29bb9c15c3b80037635c3b6d5fb434dc) )
	ROM_LOAD16_BYTE( "pg-20-lo.2f",  0x20001, 0x10000, CRC(89664841) SHA1(4ace8e4fd0026d0d73726d339a71d841652fdc87) )
	ROM_LOAD16_BYTE( "061-2135.4j",  0x40000, 0x10000, CRC(90477c4a) SHA1(69b4bcf5c329d8710d0985ce3e45bd40a7102a91) )
	ROM_LOAD16_BYTE( "pg-40-lo.4f",  0x40001, 0x10000, CRC(a9f082a9) SHA1(b1d45e528d466efa3f7562c80d2ee0c8913a33a6) )
	ROM_LOAD16_BYTE( "061-1136.bin", 0x60000, 0x10000, CRC(5ae3eeac) SHA1(583b6c3f61e8ad4d98449205fedecf3e21ee993c) )
	ROM_LOAD16_BYTE( "061-1140.bin", 0x60001, 0x10000, CRC(dacbbd94) SHA1(0e3a93f439ff9f3dd57ee13604be02e9c74c8eec) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "061-1114.bin", 0x10000, 0x4000, CRC(c0dcce1a) SHA1(285c13f08020cf5827eca2afcc2fa8a3a0a073e0) )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "061-1101.bin", 0x000000, 0x10000, CRC(02696f15) SHA1(51856c331c45d287e574e2e4013b62a6472ad720) )  /* bank 0 (4 bpp)*/
	ROM_LOAD( "061-1102.bin", 0x010000, 0x10000, CRC(4bed4262) SHA1(eda16ece14cb60012edbe006b2839986d082822e) )
	ROM_LOAD( "061-1103.bin", 0x020000, 0x10000, CRC(e62b037f) SHA1(9a2341b822265269c07b65c4bc0fbc760c1bd456) )
	ROM_LOAD( "061-1104.bin", 0x030000, 0x10000, CRC(fa05aee6) SHA1(db0dbf94ba1f2c1bb3ad55df2f38a71b4ecb38e4) )
	ROM_LOAD( "061-1105.bin", 0x040000, 0x10000, CRC(ab1c5578) SHA1(e80a1c7d2f279a523dcc9d943bd5a1ce75045d2e) )
	ROM_LOAD( "061-1106.bin", 0x050000, 0x10000, CRC(4020468e) SHA1(fa83e3d903d254c598fcbf120492ac77777ae31f) )
	ROM_LOAD( "061-1107.bin", 0x060000, 0x10000, CRC(fe6f6aed) SHA1(11bd17be3c9fe409db8268cb17515040bfd92ee2) )
	ROM_LOAD( "061-1108.bin", 0x070000, 0x10000, CRC(26fe71e1) SHA1(cac22f969c943e184a58d7bb62072f93273638de) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "061-1143.bin", 0x000000, 0x20000, CRC(211c1049) SHA1(fcf4d9321b2871723a10b7607069da83d3402723) )  /* bank 0 (4 bpp)*/
	ROM_LOAD( "061-1144.bin", 0x020000, 0x20000, CRC(ef62ed2c) SHA1(c2c21023b2f559b8a63e6ae9c59c33a3055cc465) )
	ROM_LOAD( "061-1145.bin", 0x040000, 0x20000, CRC(067ecb8a) SHA1(a42e4602e1de1cc83a30a901a9adb5519f426cff) )
	ROM_LOAD( "061-1146.bin", 0x060000, 0x20000, CRC(fea6bc92) SHA1(c502ab022fdafdef71a720237094fe95c3137d69) )
	ROM_LOAD( "061-1125.bin", 0x080000, 0x10000, CRC(c37f24ac) SHA1(341fab8244d8063a516a4a25d7ee778f708cd386) )
	ROM_RELOAD(               0x0c0000, 0x10000 )
	ROM_LOAD( "061-1126.bin", 0x090000, 0x10000, CRC(015257f0) SHA1(c5ae6a43b95ecb06a04ea877f435b1f925cff136) )
	ROM_RELOAD(               0x0d0000, 0x10000 )
	ROM_LOAD( "061-1127.bin", 0x0a0000, 0x10000, CRC(d05417cb) SHA1(5cbd54050364e82fe443ca2150c34fca84c42419) )
	ROM_RELOAD(               0x0e0000, 0x10000 )
	ROM_LOAD( "061-1128.bin", 0x0b0000, 0x10000, CRC(fba3e203) SHA1(5951571e6e64061e5448cae27af0cedd5bda2b1e) )
	ROM_RELOAD(               0x0f0000, 0x10000 )
	ROM_LOAD( "061-1147.bin", 0x100000, 0x20000, CRC(ca4308cf) SHA1(966970524915a0a5a77e3525e446b50ecde5b119) )
	ROM_LOAD( "061-1148.bin", 0x120000, 0x20000, CRC(23ddd45c) SHA1(8ee19e8982a928b56d6010f283fb2f720dc71cd6) )
	ROM_LOAD( "061-1149.bin", 0x140000, 0x20000, CRC(d77cd1d0) SHA1(148fa17c9b7a2453adf059325cb608073d1ef195) )
	ROM_LOAD( "061-1150.bin", 0x160000, 0x20000, CRC(a37157b8) SHA1(347cea600f28709fc3d942feb5cadce7def72dbb) )
	ROM_LOAD( "061-1129.bin", 0x180000, 0x10000, CRC(294aaa02) SHA1(69b42dfc444b2c9f2bb0fdcb96e2becb0df6226a) )
	ROM_RELOAD(               0x1c0000, 0x10000 )
	ROM_LOAD( "061-1130.bin", 0x190000, 0x10000, CRC(dd610817) SHA1(25f542ae4e7e77399d6df328edbc4cceb390db04) )
	ROM_RELOAD(               0x1d0000, 0x10000 )
	ROM_LOAD( "061-1131.bin", 0x1a0000, 0x10000, CRC(e8e2f919) SHA1(292dacb60db867cb2ae69942c7502af6526ab550) )
	ROM_RELOAD(               0x1e0000, 0x10000 )
	ROM_LOAD( "061-1132.bin", 0x1b0000, 0x10000, CRC(c79f8ffc) SHA1(6e90044755097387011e7cc04548bedb399b7cc3) )
	ROM_RELOAD(               0x1f0000, 0x10000 )

	ROM_REGION( 0x004000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "061-1142.bin", 0x000000, 0x04000, CRC(a6ab551f) SHA1(6a11e16f3965416c81737efcb81e751484ba5ace) )  /* alpha font */
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( toobin )
{
	atarigen_eeprom_default = NULL;
	atarijsa_init(1, 2, 1, 0x1000);
	atarigen_init_6502_speedup(1, 0x414e, 0x4166);
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1988, toobin,  0,      toobin, toobin, toobin, ROT270, "Atari Games", "Toobin' (version 3)" )
GAME( 1988, toobin2, toobin, toobin, toobin, toobin, ROT270, "Atari Games", "Toobin' (version 2)" )
GAME( 1988, toobinp, toobin, toobin, toobin, toobin, ROT270, "Atari Games", "Toobin' (prototype)" )
