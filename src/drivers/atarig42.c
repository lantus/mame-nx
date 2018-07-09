/***************************************************************************

	Atari G42 hardware

	driver by Aaron Giles

	Games supported:
		* Road Riot 4WD (1991)
		* Guardians of the 'Hood (1992)

	Known bugs:
		* ASIC65 behavior needs to be determined still
		* SLOOP banking for Guardians not yet worked out

****************************************************************************

	Memory map (TBA)

***************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "machine/asic65.h"
#include "sndhrdw/atarijsa.h"
#include "vidhrdw/atarirle.h"
#include "atarig42.h"


/*************************************
 *
 *	Statics
 *
 *************************************/

static UINT8 which_input;
static UINT8 pedal_value;



/*************************************
 *
 *	Initialization & interrupts
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_video_int_state)
		newstate = 4;
	if (atarigen_sound_int_state)
		newstate = 5;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static MACHINE_INIT( atarig42 )
{
	atarigen_eeprom_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(atarig42_scanline_update, 8);
	atarijsa_reset();
}



/*************************************
 *
 *	Interrupt handling
 *
 *************************************/

static INTERRUPT_GEN( vblank_int )
{
	/* update the pedals once per frame */
	if (readinputport(5) & 1)
	{
		if (pedal_value >= 4)
			pedal_value -= 4;
	}
	else
	{
		if (pedal_value < 0x40 - 4)
			pedal_value += 4;
	}
	atarigen_video_int_gen();
}



/*************************************
 *
 *	I/O read dispatch.
 *
 *************************************/

static READ16_HANDLER( special_port2_r )
{
	int temp = readinputport(2);
	if (atarigen_cpu_to_sound_ready) temp ^= 0x0020;
	if (atarigen_sound_to_cpu_ready) temp ^= 0x0010;
	temp ^= 0x0008;		/* A2D.EOC always high for now */
	return temp;
}


static WRITE16_HANDLER( a2d_select_w )
{
	which_input = offset;
}


static READ16_HANDLER( a2d_data_r )
{
	/* otherwise, assume it's hydra */
	switch (which_input)
	{
		case 0:
			return readinputport(4) << 8;

		case 1:
			return ~pedal_value << 8;
	}
	return 0;
}


static WRITE16_HANDLER( io_latch_w )
{
	logerror("io_latch = %04X (scan %d)\n", data, cpu_getscanline());

	/* upper byte */
	if (ACCESSING_MSB)
	{
		/* bit 14 controls the ASIC65 reset line */
		asic65_reset((~data >> 14) & 1);

		/* bits 13-11 are the MO control bits */
		atarirle_control_w(0, (data >> 11) & 7);
	}

	/* lower byte */
	if (ACCESSING_LSB)
	{
		/* bit 4 resets the sound CPU */
		cpu_set_reset_line(1, (data & 0x10) ? CLEAR_LINE : ASSERT_LINE);
		if (!(data & 0x10)) atarijsa_reset();

		/* bit 5 is /XRESET, probably related to the ASIC */

		/* bits 3 and 0 are coin counters */
	}
	logerror("sound control = %04X\n", data);
}



/*************************************
 *
 *	SLOOP banking
 *
 *************************************/

static int sloop_bank;
static int sloop_next_bank;
static int sloop_offset;
static int sloop_state;
static data16_t *sloop_base;

static void roadriot_sloop_tweak(int offset)
{
/*
	sequence 1:

		touch $68000
		touch $68eee and $124/$678/$abc/$1024(bank) in the same instruction
		touch $69158/$6a690/$6e708/$71166

	sequence 2:

		touch $5edb4 to add 2 to the bank
		touch $5db0a to add 1 to the bank
		touch $5f042
		touch $69158/$6a690/$6e708/$71166
		touch $68000
		touch $5d532/$5d534
*/

	switch (offset)
	{
		// standard 68000 -> 68eee -> (bank) addressing
		case 0x68000/2:
			sloop_state = 1;
			break;
		case 0x68eee/2:
			if (sloop_state == 1)
				sloop_state = 2;
			break;
		case 0x00124/2:
			if (sloop_state == 2)
			{
				sloop_next_bank = 0;
				sloop_state = 3;
			}
			break;
		case 0x00678/2:
			if (sloop_state == 2)
			{
				sloop_next_bank = 1;
				sloop_state = 3;
			}
			break;
		case 0x00abc/2:
			if (sloop_state == 2)
			{
				sloop_next_bank = 2;
				sloop_state = 3;
			}
			break;
		case 0x01024/2:
			if (sloop_state == 2)
			{
				sloop_next_bank = 3;
				sloop_state = 3;
			}
			break;

		// lock in the change?
		case 0x69158/2:
			// written if $ff8007 == 0
		case 0x6a690/2:
			// written if $ff8007 == 1
		case 0x6e708/2:
			// written if $ff8007 == 2
		case 0x71166/2:
			// written if $ff8007 == 3
			if (sloop_state == 3)
				sloop_bank = sloop_next_bank;
			sloop_state = 0;
			break;

		// bank offsets
		case 0x5edb4/2:
//fprintf(stderr, "@ %06X, state=%d\n", offset*2, sloop_state);
			if (sloop_state == 0)
			{
				sloop_state = 10;
				sloop_offset = 0;
			}
			sloop_offset += 2;
			break;
		case 0x5db0a/2:
//fprintf(stderr, "@ %06X, state=%d\n", offset*2, sloop_state);
			if (sloop_state == 0)
			{
				sloop_state = 10;
				sloop_offset = 0;
			}
			sloop_offset += 1;
			break;

		// apply the offset
		case 0x5f042/2:
//fprintf(stderr, "@ %06X, state=%d\n", offset*2, sloop_state);
			if (sloop_state == 10)
			{
				sloop_bank = (sloop_bank + sloop_offset) & 3;
//fprintf(stderr, "bank = %d\n", sloop_bank);
				sloop_offset = 0;
				sloop_state = 0;
			}
			break;

		// unknown
		case 0x5d532/2:
			break;
		case 0x5d534/2:
			break;

		default:
			break;
	}
}


static READ16_HANDLER( roadriot_sloop_data_r )
{
	roadriot_sloop_tweak(offset);
	if (offset < 0x78000/2)
		return sloop_base[offset];
	else
		return sloop_base[0x78000/2 + sloop_bank * 0x1000 + (offset & 0xfff)];
}


static WRITE16_HANDLER( roadriot_sloop_data_w )
{
	roadriot_sloop_tweak(offset);
}


/*************************************
 *
 *	Main CPU memory handlers
 *
 *

 	FExxxx = 68.RAM
 	FCxxxx = 68.CRAM
 	FAxxxx = 68.EEROM
 	F8xxxx = 68.TDSPWR
 	F6xxxx = 68.TDSPRD
 	F4xxxx = 68.RDSTAT
 	  8000 = TFULL (clocked when 68.TDSPWR, clear when T.RD68)
 	  4000 = 68FULL (clocked when T.WR68, clear when 68.TDSPRD)
 	  2000 = XFLG (= TD0 when T.WRSTAT)
 	  1000 = 1

 	E038xx = 68.WDOG
 	E030xx = 68.IRQACK
 	E008xx = 68.MTRSOL
 	E000xx = 68.CIO

 	E0006x = 68.UNLOCK
 	E0005x = 68.LATCH
 	  4000 = /TRESET
 	  2000 = FRAME
 	  1000 = ERASE
 	  0800 = /MOGO
 	  0100 = VCR
 	  0020 = /XRESET
 	  0010 = /SNDRES
 	  0008 = CC.L
 	  0001 = CC.R
 	E0004x = 68.AUDWR
 	E0003x = 68.AUDRD
 	E0002x = 68.A2D (upper 8 bits)
 	E0001x = 68.STATUS
 	E0000x = 68.SW

	E00012 = 68.STATUS1
	  0080 = 0
	  0040 = 0
	  0020 = /SER.L
	  0010 = /SER.R
	  0008 = /XFULL
	  0004 = /X.IRQ
	  0002 = n/c
	  0001 = n/c
	E00010 = 68.STATUS0
	  0080 = VBLANK
	  0040 = S-TEST
	  0020 = /AUDFULL
	  0010 = /AUDIRQ
	  0008 = A2D.EOC
	  0004 = 0
	  0002 = 0
	  0001 = 0

 	E00002 = 68.SW1
 	  8000 = UP2
 	  4000 = DN2
 	  2000 = LF2
 	  1000 = RT2
 	  0800 = ACTC2
 	  0400 = ACTB2
 	  0200 = ACTA2
 	  0100 = STRT2
 	  0080 = SERVICE3
 	  0040 = COIN3
 	  0020 = ACTTL3
 	  0010 = ACTTR3
 	  0008 = ACTTL2
 	  0004 = ACTTR2
 	  0002 = ACTTL1
 	  0001 = ACTTR1
 	E00000 = 68.SW0
 	  8000 = UP1
 	  4000 = DN1
 	  2000 = LF1
 	  1000 = RT1
 	  0800 = ACTC1
 	  0400 = ACTB1
 	  0200 = ACTA1
 	  0100 = STRT1
 	  0080 = UP3
 	  0040 = DN3
 	  0020 = LF3
 	  0010 = RT3
 	  0008 = ACTC3
 	  0004 = ACTB3
 	  0002 = ACTA3
 	  0001 = STRT3

 	68.RDSTAT


 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x080001, MRA16_ROM },
	{ 0xe00000, 0xe00001, input_port_0_word_r },
	{ 0xe00002, 0xe00003, input_port_1_word_r },
	{ 0xe00010, 0xe00011, special_port2_r },
	{ 0xe00012, 0xe00013, input_port_3_word_r },
	{ 0xe00020, 0xe00027, a2d_data_r },
	{ 0xe00030, 0xe00031, atarigen_sound_r },
	{ 0xe80000, 0xe80fff, MRA16_RAM },
	{ 0xf40000, 0xf40001, asic65_io_r },
	{ 0xf60000, 0xf60001, asic65_r },
	{ 0xfa0000, 0xfa0fff, atarigen_eeprom_r },
	{ 0xfc0000, 0xfc0fff, MRA16_RAM },
	{ 0xff0000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x080001, MWA16_ROM },
	{ 0xe00020, 0xe00027, a2d_select_w },
	{ 0xe00040, 0xe00041, atarigen_sound_w },
	{ 0xe00050, 0xe00051, io_latch_w },
	{ 0xe00060, 0xe00061, atarigen_eeprom_enable_w },
	{ 0xe03000, 0xe03001, atarigen_video_int_ack_w },
	{ 0xe03800, 0xe03801, watchdog_reset16_w },
	{ 0xe80000, 0xe80fff, MWA16_RAM },
	{ 0xf80000, 0xf80003, asic65_data_w },
	{ 0xfa0000, 0xfa0fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xfc0000, 0xfc0fff, atarigen_666_paletteram_w, &paletteram16 },
	{ 0xff0000, 0xff0fff, atarirle_0_spriteram_w, &atarirle_0_spriteram },
	{ 0xff1000, 0xff1fff, MWA16_RAM },
	{ 0xff2000, 0xff5fff, atarigen_playfield_w, &atarigen_playfield },
	{ 0xff6000, 0xff6fff, atarigen_alpha_w, &atarigen_alpha },
	{ 0xff7000, 0xffffff, MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( roadriot )
	PORT_START		/* e00000 */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0xf800, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* e00002 */
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* e00010 */
	PORT_BIT( 0x003f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	JSA_III_PORT	/* audio board port */

	PORT_START		/* e00012 */
	PORT_ANALOG ( 0x00ff, 0x0080, IPT_AD_STICK_X, 50, 10, 0, 255 )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* fake for pedals */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0xfffe, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( guardian )
	PORT_START		/* e00000 */
	PORT_BIT( 0x01ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )

	PORT_START      /* e00002 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x01f0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )

	PORT_START		/* e00010 */
	PORT_BIT( 0x003f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	JSA_III_PORT	/* audio board port */

	PORT_START      /* e00012 */
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* not used */
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout pflayout =
{
	8,8,
	RGN_FRAC(1,3),
	5,
	{ 0, 0, 1, 2, 3 },
	{ RGN_FRAC(1,3)+0, RGN_FRAC(1,3)+4, 0, 4, RGN_FRAC(1,3)+8, RGN_FRAC(1,3)+12, 8, 12 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8
};

static struct GfxLayout pftoplayout =
{
	8,8,
	RGN_FRAC(1,3),
	6,
	{ RGN_FRAC(2,3)+0, RGN_FRAC(2,3)+4, 0, 0, 0, 0 },
	{ 3, 2, 1, 0, 11, 10, 9, 8 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8
};

static struct GfxLayout anlayout =
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
	{ REGION_GFX1, 0, &pflayout, 0x000, 64 },
	{ REGION_GFX2, 0, &anlayout, 0x000, 16 },
	{ REGION_GFX1, 0, &pftoplayout, 0x000, 64 },
	{ -1 } /* end of array */
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( atarig42 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, ATARI_CLOCK_14MHz)
	MDRV_CPU_MEMORY(main_readmem,main_writemem)
	MDRV_CPU_VBLANK_INT(vblank_int,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(atarig42)
	MDRV_NVRAM_HANDLER(atarigen)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(42*8, 30*8)
	MDRV_VISIBLE_AREA(0*8, 42*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(atarig42)
	MDRV_VIDEO_EOF(atarirle)
	MDRV_VIDEO_UPDATE(atarig42)

	/* sound hardware */
	MDRV_IMPORT_FROM(jsa_iii_mono)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( roadriot )
	ROM_REGION( 0x80004, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "rriot.8d", 0x00000, 0x20000, CRC(bf8aaafc) )
	ROM_LOAD16_BYTE( "rriot.8c", 0x00001, 0x20000, CRC(5dd2dd70) )
	ROM_LOAD16_BYTE( "rriot.9d", 0x40000, 0x20000, CRC(6191653c) )
	ROM_LOAD16_BYTE( "rriot.9c", 0x40001, 0x20000, CRC(0d34419a) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "rriots.12c", 0x10000, 0x4000, CRC(849dd26c) )
	ROM_CONTINUE(           0x04000, 0xc000 )

	ROM_REGION( 0xc0000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rriot.22d",    0x000000, 0x20000, CRC(b7451f92) ) /* playfield, planes 0-1 */
	ROM_LOAD( "rriot.22c",    0x020000, 0x20000, CRC(90f3c6ee) )
	ROM_LOAD( "rriot20.21d",  0x040000, 0x20000, CRC(d40de62b) ) /* playfield, planes 2-3 */
	ROM_LOAD( "rriot20.21c",  0x060000, 0x20000, CRC(a7e836b1) )
	ROM_LOAD( "rriot.20d",    0x080000, 0x20000, CRC(a81ae93f) ) /* playfield, planes 4-5 */
	ROM_LOAD( "rriot.20c",    0x0a0000, 0x20000, CRC(b8a6d15a) )

	ROM_REGION( 0x020000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "rriot.22j",    0x000000, 0x20000, CRC(0005bab0) ) /* alphanumerics */

	ROM_REGION16_BE( 0x200000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "rriot.2s", 0x000000, 0x20000, CRC(19590a94) )
	ROM_LOAD16_BYTE( "rriot.2p", 0x000001, 0x20000, CRC(c2bf3f69) )
	ROM_LOAD16_BYTE( "rriot.3s", 0x040000, 0x20000, CRC(bab110e4) )
	ROM_LOAD16_BYTE( "rriot.3p", 0x040001, 0x20000, CRC(791ad2c5) )
	ROM_LOAD16_BYTE( "rriot.4s", 0x080000, 0x20000, CRC(79cba484) )
	ROM_LOAD16_BYTE( "rriot.4p", 0x080001, 0x20000, CRC(86a2e257) )
	ROM_LOAD16_BYTE( "rriot.5s", 0x0c0000, 0x20000, CRC(67d28478) )
	ROM_LOAD16_BYTE( "rriot.5p", 0x0c0001, 0x20000, CRC(74638838) )
	ROM_LOAD16_BYTE( "rriot.6s", 0x100000, 0x20000, CRC(24933c37) )
	ROM_LOAD16_BYTE( "rriot.6p", 0x100001, 0x20000, CRC(054a163b) )
	ROM_LOAD16_BYTE( "rriot.7s", 0x140000, 0x20000, CRC(31ff090a) )
	ROM_LOAD16_BYTE( "rriot.7p", 0x140001, 0x20000, CRC(bbe5b69b) )
	ROM_LOAD16_BYTE( "rriot.8s", 0x180000, 0x20000, CRC(6c89d2c5) )
	ROM_LOAD16_BYTE( "rriot.8p", 0x180001, 0x20000, CRC(40d9bde5) )
	ROM_LOAD16_BYTE( "rriot.9s", 0x1c0000, 0x20000, CRC(eca3c595) )
	ROM_LOAD16_BYTE( "rriot.9p", 0x1c0001, 0x20000, CRC(88acdb53) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 1MB for ADPCM samples */
	ROM_LOAD( "rriots.19e",  0x80000, 0x20000, CRC(2db638a7) )
	ROM_LOAD( "rriots.17e",  0xa0000, 0x20000, CRC(e1dd7f9e) )
	ROM_LOAD( "rriots.15e",  0xc0000, 0x20000, CRC(64d410bb) )
	ROM_LOAD( "rriots.12e",  0xe0000, 0x20000, CRC(bffd01c8) )

	ROM_REGION( 0x0600, REGION_PROMS, ROMREGION_DISPOSE )	/* microcode for growth renderer */
	ROM_LOAD( "089-1001.bin",  0x0000, 0x0200, CRC(5836cb5a) )
	ROM_LOAD( "089-1002.bin",  0x0200, 0x0200, CRC(44288753) )
	ROM_LOAD( "089-1003.bin",  0x0400, 0x0200, CRC(1f571706) )
ROM_END


ROM_START( guardian )
	ROM_REGION( 0x80004, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "2021.8e",  0x00000, 0x20000, CRC(efea1e02) )
	ROM_LOAD16_BYTE( "2020.8cd", 0x00001, 0x20000, CRC(a8f655ba) )
	ROM_LOAD16_BYTE( "2023.9e",  0x40000, 0x20000, CRC(cfa29316) )
	ROM_LOAD16_BYTE( "2022.9cd", 0x40001, 0x20000, CRC(ed2abc91) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "0080-snd.12c", 0x10000, 0x4000, CRC(0388f805) )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION( 0x180000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "0037a.23e",  0x000000, 0x80000, CRC(ca10b63e) ) /* playfield, planes 0-1 */
	ROM_LOAD( "0038a.22e",  0x080000, 0x80000, CRC(cb1431a1) ) /* playfield, planes 2-3 */
	ROM_LOAD( "0039a.20e",  0x100000, 0x80000, CRC(2eee7188) ) /* playfield, planes 4-5 */

	ROM_REGION( 0x020000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "0030.23k",   0x000000, 0x20000, CRC(0fd7baa1) ) /* alphanumerics */

	ROM_REGION16_BE( 0x600000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "0041.2s",  0x000000, 0x80000, CRC(a2a5ae08) )
	ROM_LOAD16_BYTE( "0040.2p",  0x000001, 0x80000, CRC(ef95132e) )
	ROM_LOAD16_BYTE( "0043.3s",  0x100000, 0x80000, CRC(6438b8e4) )
	ROM_LOAD16_BYTE( "0042.3p",  0x100001, 0x80000, CRC(46bf7c0d) )
	ROM_LOAD16_BYTE( "0045.4s",  0x200000, 0x80000, CRC(4f4f2bee) )
	ROM_LOAD16_BYTE( "0044.4p",  0x200001, 0x80000, CRC(20a4250b) )
	ROM_LOAD16_BYTE( "0063a.6s", 0x300000, 0x80000, CRC(93933bcf) )
	ROM_LOAD16_BYTE( "0062a.6p", 0x300001, 0x80000, CRC(613e6f1d) )
	ROM_LOAD16_BYTE( "0065a.7s", 0x400000, 0x80000, CRC(6bcd1205) )
	ROM_LOAD16_BYTE( "0064a.7p", 0x400001, 0x80000, CRC(7b4dce05) )
	ROM_LOAD16_BYTE( "0067a.9s", 0x500000, 0x80000, CRC(15845fba) )
	ROM_LOAD16_BYTE( "0066a.9p", 0x500001, 0x80000, CRC(7130c575) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )	/* 1MB for ADPCM samples */
	ROM_LOAD( "0010-snd",  0x80000, 0x80000, CRC(bca27f40) )

	ROM_REGION( 0x0600, REGION_PROMS, ROMREGION_DISPOSE )	/* microcode for growth renderer */
	ROM_LOAD( "092-1001.bin",  0x0000, 0x0200, CRC(b3251eeb) )
	ROM_LOAD( "092-1002.bin",  0x0200, 0x0200, CRC(0c5314da) )
	ROM_LOAD( "092-1003.bin",  0x0400, 0x0200, CRC(344b406a) )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( roadriot )
{
	static const UINT16 default_eeprom[] =
	{
		0x0001,0x01B7,0x01AF,0x01E4,0x0100,0x0130,0x0300,0x01CC,
		0x0700,0x01FE,0x0500,0x0102,0x0200,0x0108,0x011B,0x01C8,
		0x0100,0x0107,0x0120,0x0100,0x0125,0x0500,0x0177,0x0162,
		0x013A,0x010A,0x01B7,0x01AF,0x01E4,0x0100,0x0130,0x0300,
		0x01CC,0x0700,0x01FE,0x0500,0x0102,0x0200,0x0108,0x011B,
		0x01C8,0x0100,0x0107,0x0120,0x0100,0x0125,0x0500,0x0177,
		0x0162,0x013A,0x010A,0xE700,0x0164,0x0106,0x0100,0x0104,
		0x01B0,0x0146,0x012E,0x1A00,0x01C8,0x01D0,0x0118,0x0D00,
		0x0118,0x0100,0x01C8,0x01D0,0x0000
	};
	atarigen_eeprom_default = default_eeprom;
	atarijsa_init(1, 3, 2, 0x0040);
	atarijsa3_init_adpcm(REGION_SOUND1);
	atarigen_init_6502_speedup(1, 0x4168, 0x4180);

	atarig42_playfield_base = 0x400;
	atarig42_motion_object_base = 0x200;
	atarig42_motion_object_mask = 0x1ff;

	sloop_base = install_mem_read16_handler(0, 0x000000, 0x07ffff, roadriot_sloop_data_r);
	sloop_base = install_mem_write16_handler(0, 0x000000, 0x07ffff, roadriot_sloop_data_w);

	asic65_config(ASIC65_STANDARD);
}


static DRIVER_INIT( guardian )
{
	static const UINT16 default_eeprom[] =
	{
		0x0001,0x01FD,0x01FF,0x01EF,0x0100,0x01CD,0x0300,0x0104,
		0x0700,0x0117,0x0F00,0x0133,0x1F00,0x0133,0x2400,0x0120,
		0x0600,0x0104,0x0300,0x010C,0x01A0,0x0100,0x0152,0x0179,
		0x012D,0x01BD,0x01FD,0x01FF,0x01EF,0x0100,0x01CD,0x0300,
		0x0104,0x0700,0x0117,0x0F00,0x0133,0x1F00,0x0133,0x2400,
		0x0120,0x0600,0x0104,0x0300,0x010C,0x01A0,0x0100,0x0152,
		0x0179,0x012D,0x01BD,0x8C00,0x0118,0x01AB,0x015A,0x0100,
		0x01D0,0x010B,0x01B8,0x01C7,0x01E2,0x0134,0x0100,0x010A,
		0x01BE,0x016D,0x0142,0x0100,0x0120,0x0109,0x0110,0x0141,
		0x0109,0x0100,0x0108,0x0134,0x0105,0x0148,0x1400,0x0000
	};
	atarigen_eeprom_default = default_eeprom;
	atarijsa_init(1, 3, 2, 0x0040);
	atarijsa3_init_adpcm(REGION_SOUND1);
	atarigen_init_6502_speedup(1, 0x3168, 0x3180);

	atarig42_playfield_base = 0x000;
	atarig42_motion_object_base = 0x400;
	atarig42_motion_object_mask = 0x3ff;

	/* it looks like they jsr to $80000 as some kind of protection */
	/* put an RTS there so we don't die */
	*(data16_t *)&memory_region(REGION_CPU1)[0x80000] = 0x4E75;

	asic65_config(ASIC65_STANDARD);
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAMEX( 1991, roadriot, 0,        atarig42, roadriot, roadriot, ROT0, "Atari Games", "Road Riot 4WD", GAME_UNEMULATED_PROTECTION )
GAMEX( 1992, guardian, 0,        atarig42, guardian, guardian, ROT0, "Atari Games", "Guardians of the Hood", GAME_UNEMULATED_PROTECTION )
