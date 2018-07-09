/***************************************************************************

	Atari Vindicators hardware

	driver by Aaron Giles

	Games supported:
		* Vindicators (1988)

	Known bugs:
		* none at this time

****************************************************************************

	Memory map (TBA)

***************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "sndhrdw/atarijsa.h"
#include "vindictr.h"



/*************************************
 *
 *	Shared RAM handling
 *
 *************************************/

static data16_t *shared_ram;

static READ16_HANDLER( pfram_r ) { return atarigen_playfield[offset]; }
static READ16_HANDLER( moram_r ) { return atarimo_0_spriteram[offset]; }
static READ16_HANDLER( anram_r ) { return atarigen_alpha[offset]; }
static READ16_HANDLER( shared_ram_r ) { return shared_ram[offset]; }

static WRITE16_HANDLER( shared_ram_w ) { COMBINE_DATA(&shared_ram[offset]); }



/*************************************
 *
 *	Initialization
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_scanline_int_state)
		newstate |= 4;
	if (atarigen_sound_int_state)
		newstate |= 6;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static MACHINE_INIT( vindictr )
{
	atarigen_eeprom_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(vindictr_scanline_update, 8);
	atarijsa_reset();
}



/*************************************
 *
 *	I/O handling
 *
 *************************************/

static int fake_inputs(int real_port, int fake_port)
{
	int result = readinputport(real_port);
	int fake = readinputport(fake_port);

	if (fake & 0x01)			/* up */
	{
		if (fake & 0x04)		/* up and left */
			result &= ~0x2000;
		else if (fake & 0x08)	/* up and right */
			result &= ~0x1000;
		else					/* up only */
			result &= ~0x3000;
	}
	else if (fake & 0x02)		/* down */
	{
		if (fake & 0x04)		/* down and left */
			result &= ~0x8000;
		else if (fake & 0x08)	/* down and right */
			result &= ~0x4000;
		else					/* down only */
			result &= ~0xc000;
	}
	else if (fake & 0x04)		/* left only */
		result &= ~0x6000;
	else if (fake & 0x08)		/* right only */
		result &= ~0x9000;

	return result;
}


static READ16_HANDLER( port0_r )
{
	return fake_inputs(0, 3);
}


static READ16_HANDLER( port1_r )
{
	int result = fake_inputs(1, 4);
	if (atarigen_sound_to_cpu_ready) result ^= 0x0004;
	if (atarigen_cpu_to_sound_ready) result ^= 0x0008;
	result ^= 0x0010;
	return result;
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x05ffff, MRA16_ROM },
	{ 0x0e0000, 0x0e0fff, atarigen_eeprom_r },
	{ 0x260000, 0x26000f, port0_r },
	{ 0x260010, 0x26001f, port1_r },
	{ 0x260020, 0x26002f, input_port_2_word_r },
	{ 0x260030, 0x260031, atarigen_sound_r },
	{ 0x3e0000, 0x3e0fff, MRA16_RAM },
	{ 0x3f0000, 0x3f1fff, pfram_r },
	{ 0x3f2000, 0x3f3fff, moram_r },
	{ 0x3f4000, 0x3f4fff, anram_r },
	{ 0x3f5000, 0x3f7fff, shared_ram_r },
	{ 0xff8000, 0xff9fff, pfram_r },
	{ 0xffa000, 0xffbfff, moram_r },
	{ 0xffc000, 0xffcfff, anram_r },
	{ 0xffd000, 0xffffff, shared_ram_r },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x05ffff, MWA16_ROM },
	{ 0x0e0000, 0x0e0fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0x1f0000, 0x1fffff, atarigen_eeprom_enable_w },
	{ 0x2e0000, 0x2e0001, watchdog_reset16_w },
	{ 0x360000, 0x360001, atarigen_scanline_int_ack_w },
	{ 0x360010, 0x360011, MWA16_NOP },
	{ 0x360020, 0x360021, atarigen_sound_reset_w },
	{ 0x360030, 0x360031, atarigen_sound_w },
	{ 0x3e0000, 0x3e0fff, vindictr_paletteram_w, &paletteram16 },
	{ 0x3f0000, 0x3f1fff, atarigen_playfield_w, &atarigen_playfield },
	{ 0x3f2000, 0x3f3fff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0x3f4000, 0x3f4f7f, atarigen_alpha_w, &atarigen_alpha },
	{ 0x3f4f80, 0x3f4fff, atarimo_0_slipram_w, &atarimo_0_slipram },
	{ 0x3f5000, 0x3f7fff, shared_ram_w, &shared_ram },
	{ 0xff8000, 0xff9fff, atarigen_playfield_w },
	{ 0xffa000, 0xffbfff, atarimo_0_spriteram_w },
	{ 0xffc000, 0xffcf7f, atarigen_alpha_w },
	{ 0xffcf80, 0xffcfff, atarimo_0_slipram_w },
	{ 0xffd000, 0xffffff, shared_ram_w },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( vindictr )
	PORT_START		/* 26000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP   | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_PLAYER1 | IPF_2WAY )

	PORT_START		/* 26010 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_SERVICE( 0x0002, IP_ACTIVE_LOW )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x00e0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_PLAYER2 | IPF_2WAY )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP   | IPF_PLAYER2 | IPF_2WAY )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_PLAYER2 | IPF_2WAY )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_PLAYER2 | IPF_2WAY )

	PORT_START		/* 26020 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xfc00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* single joystick */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )

	PORT_START		/* single joystick */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )

	JSA_I_PORT		/* audio port */
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
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &pfmolayout,  256, 32 },		/* sprites & playfield */
	{ REGION_GFX2, 0, &anlayout,      0, 64 },		/* characters 8x8 */
	{ -1 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( vindictr )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68010, ATARI_CLOCK_14MHz/2)
	MDRV_CPU_MEMORY(main_readmem,main_writemem)
	
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	
	MDRV_MACHINE_INIT(vindictr)
	MDRV_NVRAM_HANDLER(atarigen)
	
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(42*8, 30*8)
	MDRV_VISIBLE_AREA(0*8, 42*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048*8)
	
	MDRV_VIDEO_START(vindictr)
	MDRV_VIDEO_UPDATE(vindictr)
	
	/* sound hardware */
	MDRV_IMPORT_FROM(jsa_i_stereo_pokey)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( vindictr )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "vin.d1", 0x00000, 0x10000, CRC(2e5135e4) SHA1(804b3ba201088ac2c35cfcbd530acbd73548ea8c) )
	ROM_LOAD16_BYTE( "vin.d3", 0x00001, 0x10000, CRC(e357fa79) SHA1(220a10287f4bf9d981fd412c8dd0a9c106eaf342) )
	ROM_LOAD16_BYTE( "romalot.j1", 0x20000, 0x10000, CRC(0deb7330) SHA1(e9fb311e96bcf57f2136fff87a973a5a3b5208b3) )
	ROM_LOAD16_BYTE( "romalot.j3", 0x20001, 0x10000, CRC(a6ae4753) SHA1(e69067ba0f1e5a4e446356e2fee3763dd4bcdd5a) )
	ROM_LOAD16_BYTE( "romalot.k1", 0x40000, 0x10000, CRC(96b150c5) SHA1(405c848f7990c981fefd355ca635bfb0ac24eb26) )
	ROM_LOAD16_BYTE( "romalot.k3", 0x40001, 0x10000, CRC(6415d312) SHA1(0115e32c1c42421cb3d978cc8642f7f88d492043) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k + 16k for 6502 code */
	ROM_LOAD( "vin.snd",     0x10000, 0x4000, CRC(d2212c0a) SHA1(df11fe76d74abc0cea23f18264cef4b0f33b1ffd) )
	ROM_CONTINUE(            0x04000, 0xc000 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "vin.p13",     0x00000, 0x20000, CRC(062f8e52) SHA1(0968b8c822d8fee1cf7ddcf9c3b1bf059e446417) )
	ROM_LOAD( "vin.p14",     0x20000, 0x10000, CRC(0e4366fa) SHA1(1891f6b818f7b0e447e8a83ad0c12aade0b776ee) )
	ROM_RELOAD(              0x30000, 0x10000 )
	ROM_LOAD( "vin.p8",      0x40000, 0x20000, CRC(09123b57) SHA1(ddd5a4033b5f5ed45f639909364fe5ccd7a0cb53) )
	ROM_LOAD( "vin.p6",      0x60000, 0x10000, CRC(6b757bca) SHA1(2d615b1b42f554bbfebc34928c106c3dd93dc7b2) )
	ROM_RELOAD(              0x70000, 0x10000 )
	ROM_LOAD( "vin.r13",     0x80000, 0x20000, CRC(a5268c4f) SHA1(99f1f1f2e88f8b2f235070e525aaed9aff6e91c6) )
	ROM_LOAD( "vin.r14",     0xa0000, 0x10000, CRC(609f619e) SHA1(64b5c2b0f5da07a9dd148aa19bb87e2b2cb1c395) )
	ROM_RELOAD(              0xb0000, 0x10000 )
	ROM_LOAD( "vin.r8",      0xc0000, 0x20000, CRC(2d07fdaa) SHA1(b6772fd764ddc1d2fa1c44c931b269aab9ad5e2b) )
	ROM_LOAD( "vin.r6",      0xe0000, 0x10000, CRC(0a2aba63) SHA1(e4780c790278034f0332697d5f06e6ed6b57d273) )
	ROM_RELOAD(              0xf0000, 0x10000 )

	ROM_REGION( 0x04000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "vin.n17",     0x00000, 0x04000, CRC(f99b631a) SHA1(7a2430b6810c77b0f717d6e9d71823eadbcf6013) )        /* alpha font */
ROM_END


ROM_START( vindicta )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "vin.d1", 0x00000, 0x10000, CRC(2e5135e4) SHA1(804b3ba201088ac2c35cfcbd530acbd73548ea8c) )
	ROM_LOAD16_BYTE( "vin.d3", 0x00001, 0x10000, CRC(e357fa79) SHA1(220a10287f4bf9d981fd412c8dd0a9c106eaf342) )
	ROM_LOAD16_BYTE( "vin.j1", 0x20000, 0x10000, CRC(44c77ee0) SHA1(f47307126a4960d59d19d1783497971f76ee00a5) )
	ROM_LOAD16_BYTE( "vin.j3", 0x20001, 0x10000, CRC(4deaa77f) SHA1(1c582186d07f39dadf81e90a65928ff1520a60cc) )
	ROM_LOAD16_BYTE( "vin.k1", 0x40000, 0x10000, CRC(9a0444ee) SHA1(211be931a8b6ca42dd140baf3e165ce23f75431f) )
	ROM_LOAD16_BYTE( "vin.k3", 0x40001, 0x10000, CRC(d5022d78) SHA1(eeb6876ee6994f5736114a786c5c4ba97f26ef01) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k + 16k for 6502 code */
	ROM_LOAD( "vin.snd",     0x10000, 0x4000, CRC(d2212c0a) SHA1(df11fe76d74abc0cea23f18264cef4b0f33b1ffd) )
	ROM_CONTINUE(            0x04000, 0xc000 )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "vin.p13",     0x00000, 0x20000, CRC(062f8e52) SHA1(0968b8c822d8fee1cf7ddcf9c3b1bf059e446417) )
	ROM_LOAD( "vin.p14",     0x20000, 0x10000, CRC(0e4366fa) SHA1(1891f6b818f7b0e447e8a83ad0c12aade0b776ee) )
	ROM_RELOAD(              0x30000, 0x10000 )
	ROM_LOAD( "vin.p8",      0x40000, 0x20000, CRC(09123b57) SHA1(ddd5a4033b5f5ed45f639909364fe5ccd7a0cb53) )
	ROM_LOAD( "vin.p6",      0x60000, 0x10000, CRC(6b757bca) SHA1(2d615b1b42f554bbfebc34928c106c3dd93dc7b2) )
	ROM_RELOAD(              0x70000, 0x10000 )
	ROM_LOAD( "vin.r13",     0x80000, 0x20000, CRC(a5268c4f) SHA1(99f1f1f2e88f8b2f235070e525aaed9aff6e91c6) )
	ROM_LOAD( "vin.r14",     0xa0000, 0x10000, CRC(609f619e) SHA1(64b5c2b0f5da07a9dd148aa19bb87e2b2cb1c395) )
	ROM_RELOAD(              0xb0000, 0x10000 )
	ROM_LOAD( "vin.r8",      0xc0000, 0x20000, CRC(2d07fdaa) SHA1(b6772fd764ddc1d2fa1c44c931b269aab9ad5e2b) )
	ROM_LOAD( "vin.r6",      0xe0000, 0x10000, CRC(0a2aba63) SHA1(e4780c790278034f0332697d5f06e6ed6b57d273) )
	ROM_RELOAD(              0xf0000, 0x10000 )

	ROM_REGION( 0x04000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "vin.n17",     0x00000, 0x04000, CRC(f99b631a) SHA1(7a2430b6810c77b0f717d6e9d71823eadbcf6013) )        /* alpha font */
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( vindictr )
{
	atarigen_eeprom_default = NULL;
	atarijsa_init(1, 5, 1, 0x0002);
	atarigen_init_6502_speedup(1, 0x4150, 0x4168);
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1988, vindictr, 0,        vindictr, vindictr, vindictr, ROT0, "Atari Games", "Vindicators (4/26/88)" )
GAME( 1988, vindicta, vindictr, vindictr, vindictr, vindictr, ROT0, "Atari Games", "Vindicators (4/20/88)" )
