/***************************************************************************

	Atari Xybots hardware

	driver by Aaron Giles

	Games supported:
		* Xybots (1987)

	Known bugs:
		* none at this time

****************************************************************************

	Memory map (TBA)

***************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "sndhrdw/atarijsa.h"
#include "xybots.h"



/*************************************
 *
 *	Initialization & interrupts
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_video_int_state)
		newstate = 1;
	if (atarigen_sound_int_state)
		newstate = 2;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static MACHINE_INIT( xybots )
{
	atarigen_eeprom_reset();
	atarigen_slapstic_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarijsa_reset();
}



/*************************************
 *
 *	I/O handlers
 *
 *************************************/

static READ16_HANDLER( special_port1_r )
{
	static int h256 = 0x0400;

	int result = readinputport(1);

	if (atarigen_cpu_to_sound_ready) result ^= 0x0200;
	result ^= h256 ^= 0x0400;
	return result;
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0xff8000, 0xffbfff, MRA16_RAM },
	{ 0xffc000, 0xffc7ff, MRA16_RAM },
	{ 0xffd000, 0xffdfff, atarigen_eeprom_r },
	{ 0xffe000, 0xffe0ff, atarigen_sound_r },
	{ 0xffe100, 0xffe1ff, input_port_0_word_r },
	{ 0xffe200, 0xffe2ff, special_port1_r },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0xff8000, 0xff8fff, atarigen_alpha_w, &atarigen_alpha },
	{ 0xff9000, 0xffadff, MWA16_RAM },
	{ 0xffae00, 0xffafff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0xffb000, 0xffbfff, atarigen_playfield_w, &atarigen_playfield },
	{ 0xffc000, 0xffc7ff, paletteram16_IIIIRRRRGGGGBBBB_word_w, &paletteram16 },
	{ 0xffd000, 0xffdfff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xffe800, 0xffe8ff, atarigen_eeprom_enable_w },
	{ 0xffe900, 0xffe9ff, atarigen_sound_w },
	{ 0xffea00, 0xffeaff, watchdog_reset16_w },
	{ 0xffeb00, 0xffebff, atarigen_video_int_ack_w },
	{ 0xffee00, 0xffeeff, atarigen_sound_reset_w },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( xybots )
	PORT_START	/* ffe100 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BITX(0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2, "P2 Twist Right", KEYCODE_W, IP_JOY_DEFAULT )
	PORT_BITX(0x0008, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2, "P2 Twist Left", KEYCODE_Q, IP_JOY_DEFAULT )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BITX(0x0400, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1, "P1 Twist Right", KEYCODE_X, IP_JOY_DEFAULT )
	PORT_BITX(0x0800, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1, "P1 Twist Left", KEYCODE_Z, IP_JOY_DEFAULT )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 | IPF_8WAY )

	PORT_START	/* ffe200 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x0100, IP_ACTIVE_LOW )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNUSED ) 	/* /AUDBUSY */
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_UNUSED )	/* 256H */
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_VBLANK )	/* VBLANK */
	PORT_BIT( 0xf000, IP_ACTIVE_LOW, IPT_UNUSED )

	JSA_I_PORT_SWAPPED	/* audio port */
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


static struct GfxLayout pfmolayout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8 },
	32*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pfmolayout,    512, 16 },		/* playfield */
	{ REGION_GFX2, 0, &pfmolayout,    256, 48 },		/* sprites */
	{ REGION_GFX3, 0, &anlayout,        0, 64 },		/* characters 8x8 */
	{ -1 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( xybots )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, ATARI_CLOCK_14MHz/2)
	MDRV_CPU_MEMORY(main_readmem,main_writemem)
	MDRV_CPU_VBLANK_INT(atarigen_video_int_gen,1)
	
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	
	MDRV_MACHINE_INIT(xybots)
	MDRV_NVRAM_HANDLER(atarigen)
	
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(42*8, 30*8)
	MDRV_VISIBLE_AREA(0*8, 42*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)
	
	MDRV_VIDEO_START(xybots)
	MDRV_VIDEO_UPDATE(xybots)
	
	/* sound hardware */
	MDRV_IMPORT_FROM(jsa_i_stereo_swapped)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( xybots )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "2112.c17",     0x00000, 0x10000, CRC(16d64748) SHA1(3c2ba8ec3185b69c4e1947ac842f2250ee35216e) )
	ROM_LOAD16_BYTE( "2113.c19",     0x00001, 0x10000, CRC(2677d44a) SHA1(23a3538df13a47f2fd78d4842b9f8b81e38c802e) )
	ROM_LOAD16_BYTE( "2114.b17",     0x20000, 0x08000, CRC(d31890cb) SHA1(b58722a4dcc79e97484c2f5e35b8dbf8c3520bd9) )
	ROM_LOAD16_BYTE( "2115.b19",     0x20001, 0x08000, CRC(750ab1b0) SHA1(0638de738bd804bde4b93cd23190ee0465887cf8) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "xybots.snd",   0x10000, 0x4000, CRC(3b9f155d) SHA1(7080681a7eab282023034379825ca88adc6b300f) )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION( 0x40000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "2102.l13",     0x00000, 0x08000, CRC(c1309674) SHA1(5a163c894142c8d662557c8322dc04fded637227) )
	ROM_RELOAD(               0x08000, 0x08000 )
	ROM_LOAD( "2103.l11",     0x10000, 0x10000, CRC(907c024d) SHA1(d41c7471136f4a0632cbae28644ab1650af1467f) )
	ROM_LOAD( "2117.l7",      0x30000, 0x10000, CRC(0cc9b42d) SHA1(a744d97d40afb469ee61c2fc8d4b04ff8cc72755) )

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1105.de1",     0x00000, 0x10000, CRC(315a4274) SHA1(9a6cfdd655560e5d0320f95c8b60e733991a0909) )
	ROM_LOAD( "1106.e1",      0x10000, 0x10000, CRC(3d8c1dd2) SHA1(dd61fc0b96c395e1e65bb7114a60b45d68d08140) )
	ROM_LOAD( "1107.f1",      0x20000, 0x10000, CRC(b7217da5) SHA1(b00ff4a3d0cffb94636f84cd923a78b5a02f9741) )
	ROM_LOAD( "1108.fj1",     0x30000, 0x10000, CRC(77ac65e1) SHA1(85a458adbc1a1c62dbd799f61e8f9f7f8811e06d) )
	ROM_LOAD( "1109.j1",      0x40000, 0x10000, CRC(1b482c53) SHA1(50f463f00b7fad91c61bfeeb56bf76e120d24129) )
	ROM_LOAD( "1110.k1",      0x50000, 0x10000, CRC(99665ff4) SHA1(e93a85a601ae364d1e773174d488fca74b8d5753) )
	ROM_LOAD( "1111.kl1",     0x60000, 0x10000, CRC(416107ee) SHA1(cdfe6c6bd8efaa08506cd5707887c552500c2108) )

	ROM_REGION( 0x02000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "1101.c4",      0x00000, 0x02000, CRC(59c028a2) SHA1(27dcde0da88f949a5e4a7632d4b403b937c8c6e0) )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( xybots )
{
	atarigen_eeprom_default = NULL;
	atarigen_slapstic_init(0, 0x008000, 107);
	atarijsa_init(1, 2, 1, 0x0100);
	atarigen_init_6502_speedup(1, 0x4157, 0x416f);
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1987, xybots, 0, xybots, xybots, xybots, ROT0, "Atari Games", "Xybots" )
