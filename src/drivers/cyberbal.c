/***************************************************************************

	Atari Cyberball hardware

	driver by Aaron Giles

	Games supported:
		* Cyberball (1988) [2 sets]
		* Tournament Cyberball 2072 (1989)
		* Cyberball 2072, 2-players (1989)

	Known bugs:
		* none at this time

****************************************************************************

	Memory map (TBA)

***************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "sndhrdw/atarijsa.h"
#include "cyberbal.h"



/*************************************
 *
 *	Compiler options
 *
 *************************************/

/* better to leave this on; otherwise, you end up playing entire games out of the left speaker */
#define USE_MONO_SOUND			1



/*************************************
 *
 *	Initialization
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate1 = 0;
	int newstate2 = 0;
	int temp;

	if (atarigen_sound_int_state)
		newstate1 |= 1;
	if (atarigen_video_int_state)
		newstate2 |= 1;

	if (newstate1)
		cpu_set_irq_line(0, newstate1, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);

	if (newstate2)
		cpu_set_irq_line(2, newstate2, ASSERT_LINE);
	else
		cpu_set_irq_line(2, 7, CLEAR_LINE);

	/* check for screen swapping */
	temp = readinputport(2);
	if (temp & 1) cyberbal_set_screen(0);
	else if (temp & 2) cyberbal_set_screen(1);
}


static MACHINE_INIT( cyberbal )
{
	atarigen_eeprom_reset();
	atarigen_slapstic_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(cyberbal_scanline_update, 8);
	atarigen_sound_io_reset(1);

	cyberbal_sound_reset();

	/* CPU 2 doesn't run until reset */
	cpu_set_reset_line(2, ASSERT_LINE);

	/* make sure we're pointing to the right screen by default */
	cyberbal_set_screen(0);
}


static void cyberb2p_update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_video_int_state)
		newstate |= 1;
	if (atarigen_sound_int_state)
		newstate |= 3;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static MACHINE_INIT( cyberb2p )
{
	atarigen_eeprom_reset();
	atarigen_interrupt_reset(cyberb2p_update_interrupts);
	atarigen_scanline_timer_reset(cyberbal_scanline_update, 8);
	atarijsa_reset();

	/* make sure we're pointing to the only screen */
	cyberbal_set_screen(0);
}



/*************************************
 *
 *	I/O read dispatch.
 *
 *************************************/

static READ16_HANDLER( special_port0_r )
{
	int temp = readinputport(0);
	if (atarigen_cpu_to_sound_ready) temp ^= 0x0080;
	return temp;
}


static READ16_HANDLER( special_port2_r )
{
	int temp = readinputport(2);
	if (atarigen_cpu_to_sound_ready) temp ^= 0x2000;
	return temp;
}


static READ16_HANDLER( sound_state_r )
{
	int temp = 0xffff;
	if (atarigen_cpu_to_sound_ready) temp ^= 0xffff;
	return temp;
}



/*************************************
 *
 *	Extra I/O handlers.
 *
 *************************************/

static WRITE16_HANDLER( p2_reset_w )
{
	cpu_set_reset_line(2, CLEAR_LINE);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0xfc0000, 0xfc03ff, atarigen_eeprom_r },
	{ 0xfc8000, 0xfcffff, atarigen_sound_upper_r },
	{ 0xfe0000, 0xfe0fff, special_port0_r },
	{ 0xfe1000, 0xfe1fff, input_port_1_word_r },
	{ 0xfe8000, 0xfe8fff, cyberbal_paletteram_1_r },
	{ 0xfec000, 0xfecfff, cyberbal_paletteram_0_r },
	{ 0xff0000, 0xff37ff, MRA16_BANK1 },	/* shared */
	{ 0xff3800, 0xff3fff, MRA16_BANK2 },	/* shared */
	{ 0xff4000, 0xff77ff, MRA16_BANK3 },	/* shared */
	{ 0xff7800, 0xff9fff, MRA16_BANK4 },	/* shared */
	{ 0xffa000, 0xffbfff, MRA16_BANK5 },	/* shared */
	{ 0xffc000, 0xffffff, MRA16_BANK6 },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0xfc0000, 0xfc03ff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xfd0000, 0xfd1fff, atarigen_eeprom_enable_w },
	{ 0xfd2000, 0xfd3fff, atarigen_sound_reset_w },
	{ 0xfd4000, 0xfd5fff, watchdog_reset16_w },
	{ 0xfd6000, 0xfd7fff, p2_reset_w },
	{ 0xfd8000, 0xfd9fff, atarigen_sound_upper_w },
	{ 0xfe8000, 0xfe8fff, cyberbal_paletteram_1_w, &cyberbal_paletteram_1 },
	{ 0xfec000, 0xfecfff, cyberbal_paletteram_0_w, &cyberbal_paletteram_0 },
	{ 0xff0000, 0xff1fff, atarigen_playfield2_w, &atarigen_playfield2 },
	{ 0xff2000, 0xff2fff, atarigen_alpha2_w, &atarigen_alpha2 },
	{ 0xff3000, 0xff37ff, atarimo_1_spriteram_w, &atarimo_1_spriteram },
	{ 0xff3800, 0xff3fff, MWA16_BANK2 },
	{ 0xff4000, 0xff5fff, atarigen_playfield_w, &atarigen_playfield },
	{ 0xff6000, 0xff6fff, atarigen_alpha_w, &atarigen_alpha },
	{ 0xff7000, 0xff77ff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0xff7800, 0xff9fff, MWA16_BANK4 },
	{ 0xffa000, 0xffbfff, MWA16_NOP },
	{ 0xffc000, 0xffffff, MWA16_BANK6 },
MEMORY_END



/*************************************
 *
 *	Extra CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( extra_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0xfe0000, 0xfe0fff, special_port0_r },
	{ 0xfe1000, 0xfe1fff, input_port_1_word_r },
	{ 0xfe8000, 0xfe8fff, cyberbal_paletteram_1_r },
	{ 0xfec000, 0xfecfff, cyberbal_paletteram_0_r },
	{ 0xff0000, 0xff37ff, MRA16_BANK1 },
	{ 0xff3800, 0xff3fff, MRA16_BANK2 },
	{ 0xff4000, 0xff77ff, MRA16_BANK3 },
	{ 0xff7800, 0xff9fff, MRA16_BANK4 },
	{ 0xffa000, 0xffbfff, MRA16_BANK5 },
	{ 0xffc000, 0xffffff, MRA16_BANK6 },
MEMORY_END


static MEMORY_WRITE16_START( extra_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0xfc0000, 0xfdffff, atarigen_video_int_ack_w },
	{ 0xfe8000, 0xfe8fff, cyberbal_paletteram_1_w },
	{ 0xfec000, 0xfecfff, cyberbal_paletteram_0_w },
	{ 0xff0000, 0xff1fff, atarigen_playfield2_w },
	{ 0xff2000, 0xff2fff, atarigen_alpha2_w },
	{ 0xff3000, 0xff37ff, atarimo_1_spriteram_w },
	{ 0xff3800, 0xff3fff, MWA16_BANK2 },
	{ 0xff4000, 0xff5fff, atarigen_playfield_w },
	{ 0xff6000, 0xff6fff, atarigen_alpha_w },
	{ 0xff7000, 0xff77ff, atarimo_0_spriteram_w },
	{ 0xff7800, 0xff9fff, MWA16_BANK4 },
	{ 0xffa000, 0xffbfff, MWA16_BANK5 },
	{ 0xffc000, 0xffffff, MWA16_NOP },
MEMORY_END



/*************************************
 *
 *	Sound CPU memory handlers
 *
 *************************************/

MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2001, YM2151_status_port_0_r },
	{ 0x2802, 0x2803, atarigen_6502_irq_ack_r },
	{ 0x2c00, 0x2c01, atarigen_6502_sound_r },
	{ 0x2c02, 0x2c03, cyberbal_special_port3_r },
	{ 0x2c04, 0x2c05, cyberbal_sound_68k_6502_r },
	{ 0x2c06, 0x2c07, cyberbal_sound_6502_stat_r },
	{ 0x3000, 0x3fff, MRA_BANK8 },
	{ 0x4000, 0xffff, MRA_ROM },
MEMORY_END


MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2000, YM2151_register_port_0_w },
	{ 0x2001, 0x2001, YM2151_data_port_0_w },
	{ 0x2800, 0x2801, cyberbal_sound_68k_6502_w },
	{ 0x2802, 0x2803, atarigen_6502_irq_ack_w },
	{ 0x2804, 0x2805, atarigen_6502_sound_w },
	{ 0x2806, 0x2807, cyberbal_sound_bank_select_w },
	{ 0x3000, 0xffff, MWA_ROM },
MEMORY_END



/*************************************
 *
 *	68000 Sound CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( sound_68k_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0xff8000, 0xff87ff, cyberbal_sound_68k_r },
	{ 0xfff000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( sound_68k_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0xff8800, 0xff8fff, cyberbal_sound_68k_w },
	{ 0xff9000, 0xff97ff, cyberbal_io_68k_irq_ack_w },
	{ 0xff9800, 0xff9fff, cyberbal_sound_68k_dac_w },
	{ 0xfff000, 0xffffff, MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	2-player main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( cyberb2p_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0xfc0000, 0xfc0003, input_port_0_word_r },
	{ 0xfc2000, 0xfc2003, input_port_1_word_r },
	{ 0xfc4000, 0xfc4003, special_port2_r },
	{ 0xfc6000, 0xfc6003, atarigen_sound_upper_r },
	{ 0xfc8000, 0xfc8fff, atarigen_eeprom_r },
	{ 0xfca000, 0xfcafff, MRA16_RAM },
	{ 0xfe0000, 0xfe0003, sound_state_r },
	{ 0xff0000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( cyberb2p_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0xfc8000, 0xfc8fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xfca000, 0xfcafff, atarigen_666_paletteram_w, &paletteram16 },
	{ 0xfd0000, 0xfd0003, atarigen_eeprom_enable_w },
	{ 0xfd2000, 0xfd2003, atarigen_sound_reset_w },
	{ 0xfd4000, 0xfd4003, watchdog_reset16_w },
	{ 0xfd6000, 0xfd6003, atarigen_video_int_ack_w },
	{ 0xfd8000, 0xfd8003, atarigen_sound_upper_w },
	{ 0xff0000, 0xff1fff, atarigen_playfield_w, &atarigen_playfield },
	{ 0xff2000, 0xff2fff, atarigen_alpha_w, &atarigen_alpha },
	{ 0xff3000, 0xff37ff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0xff3800, 0xffffff, MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( cyberbal )
	PORT_START      /* fe0000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER4 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER4 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER4 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x00c0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER3 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER3 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )

	PORT_START      /* fe1000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x00c0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START		/* fake port for screen switching */
	PORT_BITX(  0x0001, IP_ACTIVE_HIGH, IPT_BUTTON2, "Select Left Screen", KEYCODE_9, IP_JOY_NONE )
	PORT_BITX(  0x0002, IP_ACTIVE_HIGH, IPT_BUTTON2, "Select Right Screen", KEYCODE_0, IP_JOY_NONE )
	PORT_BIT( 0xfffc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* audio board port */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN4 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* output buffer full */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL )	/* input buffer full */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* self test */
INPUT_PORTS_END


INPUT_PORTS_START( cyberb2p )
	PORT_START      /* fc0000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0xffc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* fc2000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0xffc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* fc4000 */
	PORT_BIT( 0x1fff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )

	JSA_II_PORT		/* audio board port */
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout pfanlayout =
{
	16,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0,0, 4,4, 8,8, 12,12, 16,16, 20,20, 24,24, 28,28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8 },
	32*8
};

static struct GfxLayout pfanlayout_interleaved =
{
	16,8,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ RGN_FRAC(1,2)+0,RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4,RGN_FRAC(1,2)+4, 0,0, 4,4, RGN_FRAC(1,2)+8,RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+12,RGN_FRAC(1,2)+12, 8,8, 12,12 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8
};

static struct GfxLayout molayout =
{
	16,8,
	RGN_FRAC(1,4),
	4,
	{ 0, 1, 2, 3 },
	{ RGN_FRAC(3,4)+0, RGN_FRAC(3,4)+4, RGN_FRAC(2,4)+0, RGN_FRAC(2,4)+4, RGN_FRAC(1,4)+0, RGN_FRAC(1,4)+4, 0, 4,
	  RGN_FRAC(3,4)+8, RGN_FRAC(3,4)+12, RGN_FRAC(2,4)+8, RGN_FRAC(2,4)+12, RGN_FRAC(1,4)+8, RGN_FRAC(1,4)+12, 8, 12 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &pfanlayout,     0, 128 },
	{ REGION_GFX1, 0, &molayout,   0x600, 16 },
	{ REGION_GFX3, 0, &pfanlayout, 0x780, 8 },
	{ -1 }
};

static struct GfxDecodeInfo gfxdecodeinfo_interleaved[] =
{
	{ REGION_GFX2, 0, &pfanlayout_interleaved,     0, 128 },
	{ REGION_GFX1, 0, &molayout,               0x600, 16 },
	{ REGION_GFX3, 0, &pfanlayout_interleaved, 0x780, 8 },
	{ -1 }
};



/*************************************
 *
 *	Sound definitions
 *
 *************************************/

static struct YM2151interface ym2151_interface =
{
	1,
	ATARI_CLOCK_14MHz/4,
#if USE_MONO_SOUND
	{ YM3012_VOL(30,MIXER_PAN_CENTER,30,MIXER_PAN_CENTER) },
#else
	{ YM3012_VOL(60,MIXER_PAN_LEFT,60,MIXER_PAN_RIGHT) },
#endif
	{ atarigen_ym2151_irq_gen }
};


static struct DACinterface dac_interface =
{
	2,
#if USE_MONO_SOUND
	{ MIXER(50,MIXER_PAN_CENTER), MIXER(50,MIXER_PAN_CENTER) }
#else
	{ MIXER(100,MIXER_PAN_LEFT), MIXER(100,MIXER_PAN_RIGHT) }
#endif
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( cyberbal )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, ATARI_CLOCK_14MHz/2)
	MDRV_CPU_MEMORY(main_readmem,main_writemem)
	
	MDRV_CPU_ADD(M6502, ATARI_CLOCK_14MHz/8)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PERIODIC_INT(atarigen_6502_irq_gen,(UINT32)(1000000000.0/((double)ATARI_CLOCK_14MHz/4/4/16/16/14)))
	
	MDRV_CPU_ADD(M68000, ATARI_CLOCK_14MHz/2)
	MDRV_CPU_MEMORY(extra_readmem,extra_writemem)
	MDRV_CPU_VBLANK_INT(atarigen_video_int_gen,1)
	
	MDRV_CPU_ADD(M68000, ATARI_CLOCK_14MHz/2)
	MDRV_CPU_MEMORY(sound_68k_readmem,sound_68k_writemem)
	MDRV_CPU_PERIODIC_INT(cyberbal_sound_68k_irq_gen,10000)
	
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(10)
	
	MDRV_MACHINE_INIT(cyberbal)
	MDRV_NVRAM_HANDLER(atarigen)
	
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_PIXEL_ASPECT_RATIO_1_2 | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(42*16, 30*8)
	MDRV_VISIBLE_AREA(0*8, 42*16-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_interleaved)
	MDRV_PALETTE_LENGTH(4096)
	
	MDRV_VIDEO_START(cyberbal)
	MDRV_VIDEO_UPDATE(cyberbal)
	
	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( cyberb2p )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, ATARI_CLOCK_14MHz/2)
	MDRV_CPU_MEMORY(cyberb2p_readmem,cyberb2p_writemem)
	MDRV_CPU_VBLANK_INT(atarigen_video_int_gen,1)
	
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	
	MDRV_MACHINE_INIT(cyberb2p)
	MDRV_NVRAM_HANDLER(atarigen)
	
	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2 | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(42*16, 30*8)
	MDRV_VISIBLE_AREA(0*8, 42*16-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)
	
	MDRV_VIDEO_START(cyberb2p)
	MDRV_VIDEO_UPDATE(cyberbal)
	
	/* sound hardware */
	MDRV_IMPORT_FROM(jsa_ii_mono)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( cyberbal )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "4123.1m", 0x00000, 0x10000, CRC(fb872740) SHA1(15e6721d466fe56d7c97c6801e214b32803a0a0d) )
	ROM_LOAD16_BYTE( "4124.1k", 0x00001, 0x10000, CRC(87babad9) SHA1(acdc6b5976445e203de19eb03697e307fe6da77d) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "2131-snd.2f",  0x10000, 0x4000, CRC(bd7e3d84) SHA1(f87878042fc79fa3883136b31ac15ddc22c6023c) )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION( 0x40000, REGION_CPU3, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "2127.3c", 0x00000, 0x10000, CRC(3e5feb1f) SHA1(9f92f496adbdf74e394e0d710d6471b9666ba5e5) )
	ROM_LOAD16_BYTE( "2128.1b", 0x00001, 0x10000, CRC(4e642cc3) SHA1(f708b6dd4360380bb10059d66df66b07966210b4) )
	ROM_LOAD16_BYTE( "2129.1c", 0x20000, 0x10000, CRC(db11d2f0) SHA1(da9f49af533cbc732b17699040c7930070a90644) )
	ROM_LOAD16_BYTE( "2130.3b", 0x20001, 0x10000, CRC(fd86b8aa) SHA1(46310efed762632ed176a08aaec41e48aad41cc1) )

	ROM_REGION16_BE( 0x40000, REGION_CPU4, 0 )	/* 256k for 68000 sound code */
	ROM_LOAD16_BYTE( "1132-snd.5c",  0x00000, 0x10000, CRC(ca5ce8d8) SHA1(69dc83d43d8c9dc7ce3207e70f48fcfc5ddda0cc) )
	ROM_LOAD16_BYTE( "1133-snd.7c",  0x00001, 0x10000, CRC(ffeb8746) SHA1(0d8d28b2d997ff3cf01b4ef25b75fa5a69754af4) )
	ROM_LOAD16_BYTE( "1134-snd.5a",  0x20000, 0x10000, CRC(bcbd4c00) SHA1(f0bfcdf0b5491e15872b543e99b834ae384cbf18) )
	ROM_LOAD16_BYTE( "1135-snd.7a",  0x20001, 0x10000, CRC(d520f560) SHA1(fb0b8d021458379188c424a343622c46ad74edaa) )

	ROM_REGION( 0x140000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1150.15a",  0x000000, 0x10000, CRC(e770eb3e) SHA1(e9f9e9e05774005c8be3bbdc19985b59a0081ef4) ) /* MO */
	ROM_LOAD( "1154.16a",  0x010000, 0x10000, CRC(40db00da) SHA1(d92d856b06f6ba11621ba7aab3d40653b3c70159) ) /* MO */
	ROM_LOAD( "2158.17a",  0x020000, 0x10000, CRC(52bb08fb) SHA1(08caa156923daf444e0caafb2cdff0704c90ef1f) ) /* MO */
	ROM_LOAD( "1162.19a",  0x030000, 0x10000, CRC(0a11d877) SHA1(876cbeffd815c084d7cbd937067d65c04aeebce5) ) /* MO */

	ROM_LOAD( "1151.11a",  0x050000, 0x10000, CRC(6f53c7c1) SHA1(5856d714c3af338be58156b404fb1e5a89c24cf9) ) /* MO */
	ROM_LOAD( "1155.12a",  0x060000, 0x10000, CRC(5de609e5) SHA1(bbea36a4cbbfeab113925951ef097673eddf26a8) ) /* MO */
	ROM_LOAD( "2159.13a",  0x070000, 0x10000, CRC(e6f95010) SHA1(42b14cf0dadfab9ce1032385fd21339b46edcfc2) ) /* MO */
	ROM_LOAD( "1163.14a",  0x080000, 0x10000, CRC(47f56ced) SHA1(62e80191e1879ffb6c736aec004bbc30a366363f) ) /* MO */

	ROM_LOAD( "1152.15c",  0x0a0000, 0x10000, CRC(c8f1f7ff) SHA1(2e0374901871d66a243f87bc4b9cbdde5505f0ec) ) /* MO */
	ROM_LOAD( "1156.16c",  0x0b0000, 0x10000, CRC(6bf0bf98) SHA1(7d2b3b61da3749b352a6bf3f1ae1cb736b5b8386) ) /* MO */
	ROM_LOAD( "2160.17c",  0x0c0000, 0x10000, CRC(c3168603) SHA1(43e00fc739d1b3dd6d925bad63058fe74c1efc74) ) /* MO */
	ROM_LOAD( "1164.19c",  0x0d0000, 0x10000, CRC(7ff29d09) SHA1(81458b058f0b037778f255b5afe94a44aba74829) ) /* MO */

	ROM_LOAD( "1153.11c",  0x0f0000, 0x10000, CRC(99629412) SHA1(53a91b2a1ac62259ec9f78421b22c7b22f4233d6) ) /* MO */
	ROM_LOAD( "1157.12c",  0x100000, 0x10000, CRC(aa198cb7) SHA1(aad4a60210289d2e5a93aac336ba995ed6ac4886) ) /* MO */
	ROM_LOAD( "2161.13c",  0x110000, 0x10000, CRC(6cf79a67) SHA1(7f3271b575cf0b5033b5b19f0e71fae251040fc6) ) /* MO */
	ROM_LOAD( "1165.14c",  0x120000, 0x10000, CRC(40bdf767) SHA1(c57aaea838abeaea1a0060c45c2f33c38a51edb3) ) /* MO */

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1146.9l",   0x000000, 0x10000, CRC(a64b4da8) SHA1(f68778adb56a1eb964acdbc7e9d690a8a83f170b) ) /* playfield */
	ROM_LOAD( "1147.8l",   0x010000, 0x10000, CRC(ca91ec1b) SHA1(970c64e19893503cae796daee63b2d7d08eaf551) ) /* playfield */
	ROM_LOAD( "1148.11l",  0x020000, 0x10000, CRC(ee29d1d1) SHA1(2a7fea25728c66ce482de76299413ef5da01beef) ) /* playfield */
	ROM_LOAD( "1149.10l",  0x030000, 0x10000, CRC(882649f8) SHA1(fbaea597b6e318234e41df245023643f448a4938) ) /* playfield */

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "1166.14n",  0x000000, 0x10000, CRC(0ca1e3b3) SHA1(d934bc9a1def4404fb86175878404cbb18127a11) ) /* alphanumerics */
	ROM_LOAD( "1167.16n",  0x010000, 0x10000, CRC(882f4e1c) SHA1(f7517ff03502ff029fb375260a35e45414567433) ) /* alphanumerics */
ROM_END


ROM_START( cyberba2 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "2123.1m", 0x00000, 0x10000, CRC(502676e8) SHA1(c0f1f1ce50d3df21cb81244268faef6c303cdfab) )
	ROM_LOAD16_BYTE( "2124.1k", 0x00001, 0x10000, CRC(30f55915) SHA1(ab93ec46f282ab9a0cd47c989537a7e036975e3f) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "2131-snd.2f",  0x10000, 0x4000, CRC(bd7e3d84) SHA1(f87878042fc79fa3883136b31ac15ddc22c6023c) )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION( 0x40000, REGION_CPU3, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "2127.3c", 0x00000, 0x10000, CRC(3e5feb1f) SHA1(9f92f496adbdf74e394e0d710d6471b9666ba5e5) )
	ROM_LOAD16_BYTE( "2128.1b", 0x00001, 0x10000, CRC(4e642cc3) SHA1(f708b6dd4360380bb10059d66df66b07966210b4) )
	ROM_LOAD16_BYTE( "2129.1c", 0x20000, 0x10000, CRC(db11d2f0) SHA1(da9f49af533cbc732b17699040c7930070a90644) )
	ROM_LOAD16_BYTE( "2130.3b", 0x20001, 0x10000, CRC(fd86b8aa) SHA1(46310efed762632ed176a08aaec41e48aad41cc1) )

	ROM_REGION16_BE( 0x40000, REGION_CPU4, 0 )	/* 256k for 68000 sound code */
	ROM_LOAD16_BYTE( "1132-snd.5c",  0x00000, 0x10000, CRC(ca5ce8d8) SHA1(69dc83d43d8c9dc7ce3207e70f48fcfc5ddda0cc) )
	ROM_LOAD16_BYTE( "1133-snd.7c",  0x00001, 0x10000, CRC(ffeb8746) SHA1(0d8d28b2d997ff3cf01b4ef25b75fa5a69754af4) )
	ROM_LOAD16_BYTE( "1134-snd.5a",  0x20000, 0x10000, CRC(bcbd4c00) SHA1(f0bfcdf0b5491e15872b543e99b834ae384cbf18) )
	ROM_LOAD16_BYTE( "1135-snd.7a",  0x20001, 0x10000, CRC(d520f560) SHA1(fb0b8d021458379188c424a343622c46ad74edaa) )

	ROM_REGION( 0x140000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1150.15a",  0x000000, 0x10000, CRC(e770eb3e) SHA1(e9f9e9e05774005c8be3bbdc19985b59a0081ef4) ) /* MO */
	ROM_LOAD( "1154.16a",  0x010000, 0x10000, CRC(40db00da) SHA1(d92d856b06f6ba11621ba7aab3d40653b3c70159) ) /* MO */
	ROM_LOAD( "2158.17a",  0x020000, 0x10000, CRC(52bb08fb) SHA1(08caa156923daf444e0caafb2cdff0704c90ef1f) ) /* MO */
	ROM_LOAD( "1162.19a",  0x030000, 0x10000, CRC(0a11d877) SHA1(876cbeffd815c084d7cbd937067d65c04aeebce5) ) /* MO */

	ROM_LOAD( "1151.11a",  0x050000, 0x10000, CRC(6f53c7c1) SHA1(5856d714c3af338be58156b404fb1e5a89c24cf9) ) /* MO */
	ROM_LOAD( "1155.12a",  0x060000, 0x10000, CRC(5de609e5) SHA1(bbea36a4cbbfeab113925951ef097673eddf26a8) ) /* MO */
	ROM_LOAD( "2159.13a",  0x070000, 0x10000, CRC(e6f95010) SHA1(42b14cf0dadfab9ce1032385fd21339b46edcfc2) ) /* MO */
	ROM_LOAD( "1163.14a",  0x080000, 0x10000, CRC(47f56ced) SHA1(62e80191e1879ffb6c736aec004bbc30a366363f) ) /* MO */

	ROM_LOAD( "1152.15c",  0x0a0000, 0x10000, CRC(c8f1f7ff) SHA1(2e0374901871d66a243f87bc4b9cbdde5505f0ec) ) /* MO */
	ROM_LOAD( "1156.16c",  0x0b0000, 0x10000, CRC(6bf0bf98) SHA1(7d2b3b61da3749b352a6bf3f1ae1cb736b5b8386) ) /* MO */
	ROM_LOAD( "2160.17c",  0x0c0000, 0x10000, CRC(c3168603) SHA1(43e00fc739d1b3dd6d925bad63058fe74c1efc74) ) /* MO */
	ROM_LOAD( "1164.19c",  0x0d0000, 0x10000, CRC(7ff29d09) SHA1(81458b058f0b037778f255b5afe94a44aba74829) ) /* MO */

	ROM_LOAD( "1153.11c",  0x0f0000, 0x10000, CRC(99629412) SHA1(53a91b2a1ac62259ec9f78421b22c7b22f4233d6) ) /* MO */
	ROM_LOAD( "1157.12c",  0x100000, 0x10000, CRC(aa198cb7) SHA1(aad4a60210289d2e5a93aac336ba995ed6ac4886) ) /* MO */
	ROM_LOAD( "2161.13c",  0x110000, 0x10000, CRC(6cf79a67) SHA1(7f3271b575cf0b5033b5b19f0e71fae251040fc6) ) /* MO */
	ROM_LOAD( "1165.14c",  0x120000, 0x10000, CRC(40bdf767) SHA1(c57aaea838abeaea1a0060c45c2f33c38a51edb3) ) /* MO */

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1146.9l",   0x000000, 0x10000, CRC(a64b4da8) SHA1(f68778adb56a1eb964acdbc7e9d690a8a83f170b) ) /* playfield */
	ROM_LOAD( "1147.8l",   0x010000, 0x10000, CRC(ca91ec1b) SHA1(970c64e19893503cae796daee63b2d7d08eaf551) ) /* playfield */
	ROM_LOAD( "1148.11l",  0x020000, 0x10000, CRC(ee29d1d1) SHA1(2a7fea25728c66ce482de76299413ef5da01beef) ) /* playfield */
	ROM_LOAD( "1149.10l",  0x030000, 0x10000, CRC(882649f8) SHA1(fbaea597b6e318234e41df245023643f448a4938) ) /* playfield */

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "1166.14n",  0x000000, 0x10000, CRC(0ca1e3b3) SHA1(d934bc9a1def4404fb86175878404cbb18127a11) ) /* alphanumerics */
	ROM_LOAD( "1167.16n",  0x010000, 0x10000, CRC(882f4e1c) SHA1(f7517ff03502ff029fb375260a35e45414567433) ) /* alphanumerics */
ROM_END


ROM_START( cyberbt )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 4*64k for 68000 code */
	ROM_LOAD16_BYTE( "cyb1007.bin", 0x00000, 0x10000, CRC(d434b2d7) SHA1(af6d51399ad4fca01ffbc7afa2bf73d7ee2f89b6) )
	ROM_LOAD16_BYTE( "cyb1008.bin", 0x00001, 0x10000, CRC(7d6c4163) SHA1(f1fe9d758f30bd0ebc990d8604ba32cc0d780683) )
	ROM_LOAD16_BYTE( "cyb1009.bin", 0x20000, 0x10000, CRC(3933e089) SHA1(4bd453bddabeafd07d193a1bc8ac0792e7aa99c3) )
	ROM_LOAD16_BYTE( "cyb1010.bin", 0x20001, 0x10000, CRC(e7a7cae8) SHA1(91e0c6a1b0c138a0e6a599011518fe10df44e76e) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "cyb1029.bin",  0x10000, 0x4000, CRC(afee87e1) SHA1(da5e91167c68eecd2cb4436ac64cda14e5f6eae7) )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION( 0x40000, REGION_CPU3, 0 )
	ROM_LOAD16_BYTE( "cyb1011.bin", 0x00000, 0x10000, CRC(22d3e09c) SHA1(18298951659badef39f839341c4d66958fcc86aa) )
	ROM_LOAD16_BYTE( "cyb1012.bin", 0x00001, 0x10000, CRC(a8eeed8c) SHA1(965765e5762ec09573243983db491a9fc85b37ef) )
	ROM_LOAD16_BYTE( "cyb1013.bin", 0x20000, 0x10000, CRC(11d287c9) SHA1(a25095ab29a7103f2bf02d656414d9dab0b79215) )
	ROM_LOAD16_BYTE( "cyb1014.bin", 0x20001, 0x10000, CRC(be15db42) SHA1(f3b1a676106e9956f62d3f36fbb1f849695ff771) )

	ROM_REGION16_BE( 0x40000, REGION_CPU4, 0 )	/* 256k for 68000 sound code */
	ROM_LOAD16_BYTE( "1132-snd.5c",  0x00000, 0x10000, CRC(ca5ce8d8) SHA1(69dc83d43d8c9dc7ce3207e70f48fcfc5ddda0cc) )
	ROM_LOAD16_BYTE( "1133-snd.7c",  0x00001, 0x10000, CRC(ffeb8746) SHA1(0d8d28b2d997ff3cf01b4ef25b75fa5a69754af4) )
	ROM_LOAD16_BYTE( "1134-snd.5a",  0x20000, 0x10000, CRC(bcbd4c00) SHA1(f0bfcdf0b5491e15872b543e99b834ae384cbf18) )
	ROM_LOAD16_BYTE( "1135-snd.7a",  0x20001, 0x10000, CRC(d520f560) SHA1(fb0b8d021458379188c424a343622c46ad74edaa) )

	ROM_REGION( 0x140000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1001.bin",  0x000000, 0x20000, CRC(586ba107) SHA1(f15d4489f5834ea5fe695f43cb9d1c2401179870) ) /* MO */
	ROM_LOAD( "1005.bin",  0x020000, 0x20000, CRC(a53e6248) SHA1(4f2466c6af74a5498468801b1de7adfc34873d5d) ) /* MO */
	ROM_LOAD( "1032.bin",  0x040000, 0x10000, CRC(131f52a0) SHA1(fa50ea82d26c36dd6a135e22dee509676d1dfe86) ) /* MO */

	ROM_LOAD( "1002.bin",  0x050000, 0x20000, CRC(0f71f86c) SHA1(783f33ba5cc1b2f0c42b8515b1cf8b6a2270acb9) ) /* MO */
	ROM_LOAD( "1006.bin",  0x070000, 0x20000, CRC(df0ab373) SHA1(3d511236eb55a773c66643158c6ef2c4dce53b68) ) /* MO */
	ROM_LOAD( "1033.bin",  0x090000, 0x10000, CRC(b6270943) SHA1(356e58dfd30c6db15264eceacef0eacda99aabae) ) /* MO */

	ROM_LOAD( "1003.bin",  0x0a0000, 0x20000, CRC(1cf373a2) SHA1(c8538855bb82fc03e26c64fc9008fbd0c66fac09) ) /* MO */
	ROM_LOAD( "1007.bin",  0x0c0000, 0x20000, CRC(f2ffab24) SHA1(6c5c90a9d9b342414a0d6258dd27b0b84bf0af0b) ) /* MO */
	ROM_LOAD( "1034.bin",  0x0e0000, 0x10000, CRC(6514f0bd) SHA1(e887dfb9e0334a7d94e7124cea6101d9ac7f0ab6) ) /* MO */

	ROM_LOAD( "1004.bin",  0x0f0000, 0x20000, CRC(537f6de3) SHA1(d5d385c3ff07aaef7bd3bd4f6c8066948a45ce9c) ) /* MO */
	ROM_LOAD( "1008.bin",  0x110000, 0x20000, CRC(78525bbb) SHA1(98ece6c0672cb60f818b8005c76cc4ae1d24b104) ) /* MO */
	ROM_LOAD( "1035.bin",  0x130000, 0x10000, CRC(1be3e5c8) SHA1(1a4d0e0d53b902c28977c8598e363c7b61c9c1c8) ) /* MO */

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "cyb1015.bin",  0x000000, 0x10000, CRC(dbbad153) SHA1(1004292e320037fc1d5e5e8e7b6a068b1305e872) ) /* playfield */
	ROM_LOAD( "cyb1016.bin",  0x010000, 0x10000, CRC(76e0d008) SHA1(2af4e48a229d23d85272d3c3203d977d81143a7f) ) /* playfield */
	ROM_LOAD( "cyb1017.bin",  0x020000, 0x10000, CRC(ddca9ca2) SHA1(19cb170fe6aeed6c67b68376b5bde07f7f115fb0) ) /* playfield */
	ROM_LOAD( "cyb1018.bin",  0x030000, 0x10000, CRC(aa495b6f) SHA1(c7d8e16d3084143928f25f66ee4d037ff7c43bcb) ) /* playfield */

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "cyb1019.bin",  0x000000, 0x10000, CRC(833b4768) SHA1(754f00089d439fb0aa1f650c1fef73cf7e5f33a1) ) /* alphanumerics */
	ROM_LOAD( "cyb1020.bin",  0x010000, 0x10000, CRC(4976cffd) SHA1(4cac8d9bd30743da6e6e4f013e6101ebc27060b6) ) /* alphanumerics */
ROM_END


ROM_START( cyberb2p )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "3019.bin", 0x00000, 0x10000, CRC(029f8cb6) SHA1(528ab6357592313b41964102c14b90862c05f248) )
	ROM_LOAD16_BYTE( "3020.bin", 0x00001, 0x10000, CRC(1871b344) SHA1(2b6f2f4760af0f0e1e0b6cb34017dcdca7635e60) )
	ROM_LOAD16_BYTE( "3021.bin", 0x20000, 0x10000, CRC(fd7ebead) SHA1(4118c865897c7a9f73de200ea9766fe190547606) )
	ROM_LOAD16_BYTE( "3022.bin", 0x20001, 0x10000, CRC(173ccad4) SHA1(4a8d2751b338dbb6dc556dfab13799561fee4836) )
	ROM_LOAD16_BYTE( "2023.bin", 0x40000, 0x10000, CRC(e541b08f) SHA1(345843209b02fb50cb08f55f80036046873b834f) )
	ROM_LOAD16_BYTE( "2024.bin", 0x40001, 0x10000, CRC(5a77ee95) SHA1(441a45a358eb78cc140c96dc4c42b30e1929ad07) )
	ROM_LOAD16_BYTE( "1025.bin", 0x60000, 0x10000, CRC(95ff68c6) SHA1(43f716a4c44fe1a38fcc6e2600bac948bb603504) )
	ROM_LOAD16_BYTE( "1026.bin", 0x60001, 0x10000, CRC(f61c4898) SHA1(9e4a14eac6d197f63c3392af3d804e81c034cb09) )

	ROM_REGION( 0x14000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "1042.bin",  0x10000, 0x4000, CRC(e63cf125) SHA1(449880f561660ba67ac2d7f8ce6333768e0ae0be) )
	ROM_CONTINUE(          0x04000, 0xc000 )

	ROM_REGION( 0x140000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1001.bin",  0x000000, 0x20000, CRC(586ba107) SHA1(f15d4489f5834ea5fe695f43cb9d1c2401179870) ) /* MO */
	ROM_LOAD( "1005.bin",  0x020000, 0x20000, CRC(a53e6248) SHA1(4f2466c6af74a5498468801b1de7adfc34873d5d) ) /* MO */
	ROM_LOAD( "1032.bin",  0x040000, 0x10000, CRC(131f52a0) SHA1(fa50ea82d26c36dd6a135e22dee509676d1dfe86) ) /* MO */

	ROM_LOAD( "1002.bin",  0x050000, 0x20000, CRC(0f71f86c) SHA1(783f33ba5cc1b2f0c42b8515b1cf8b6a2270acb9) ) /* MO */
	ROM_LOAD( "1006.bin",  0x070000, 0x20000, CRC(df0ab373) SHA1(3d511236eb55a773c66643158c6ef2c4dce53b68) ) /* MO */
	ROM_LOAD( "1033.bin",  0x090000, 0x10000, CRC(b6270943) SHA1(356e58dfd30c6db15264eceacef0eacda99aabae) ) /* MO */

	ROM_LOAD( "1003.bin",  0x0a0000, 0x20000, CRC(1cf373a2) SHA1(c8538855bb82fc03e26c64fc9008fbd0c66fac09) ) /* MO */
	ROM_LOAD( "1007.bin",  0x0c0000, 0x20000, CRC(f2ffab24) SHA1(6c5c90a9d9b342414a0d6258dd27b0b84bf0af0b) ) /* MO */
	ROM_LOAD( "1034.bin",  0x0e0000, 0x10000, CRC(6514f0bd) SHA1(e887dfb9e0334a7d94e7124cea6101d9ac7f0ab6) ) /* MO */

	ROM_LOAD( "1004.bin",  0x0f0000, 0x20000, CRC(537f6de3) SHA1(d5d385c3ff07aaef7bd3bd4f6c8066948a45ce9c) ) /* MO */
	ROM_LOAD( "1008.bin",  0x110000, 0x20000, CRC(78525bbb) SHA1(98ece6c0672cb60f818b8005c76cc4ae1d24b104) ) /* MO */
	ROM_LOAD( "1035.bin",  0x130000, 0x10000, CRC(1be3e5c8) SHA1(1a4d0e0d53b902c28977c8598e363c7b61c9c1c8) ) /* MO */

	ROM_REGION( 0x040000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1036.bin",  0x000000, 0x10000, CRC(cdf6e3d6) SHA1(476644065b82e2ea553dfb51844c0bbf3313c481) ) /* playfield */
	ROM_LOAD( "1037.bin",  0x010000, 0x10000, CRC(ec2fef3e) SHA1(07ed472fa1723ebf82d608667df70511a50dca3e) ) /* playfield */
	ROM_LOAD( "1038.bin",  0x020000, 0x10000, CRC(e866848f) SHA1(35b438dcc1947151a7aafe919b9adf33d7a8fb77) ) /* playfield */
	ROM_LOAD( "1039.bin",  0x030000, 0x10000, CRC(9b9a393c) SHA1(db4ceb33756bab0ac7bea30e6c7345d06a0f4aa2) ) /* playfield */

	ROM_REGION( 0x020000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "1040.bin",  0x000000, 0x10000, CRC(a4c116f9) SHA1(fc7becef35306ef99ffbd0cd9202759352eb6cbe) ) /* alphanumerics */
	ROM_LOAD( "1041.bin",  0x010000, 0x10000, CRC(e25d7847) SHA1(3821c62f9bdc04eb774c2210a84e26b36f2e163d) ) /* alphanumerics */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* 256k for ADPCM samples */
	ROM_LOAD( "1049.bin",  0x00000, 0x10000, CRC(94f24575) SHA1(b93b326e15cd328362ce409b7c0cc42b8a28c701) )
	ROM_LOAD( "1050.bin",  0x10000, 0x10000, CRC(87208e1e) SHA1(3647867ddc36df7633ed740c0b9365a979ef5621) )
	ROM_LOAD( "1051.bin",  0x20000, 0x10000, CRC(f82558b9) SHA1(afbecccc6203db9bdcf60638e0f4e95040d7aaf2) )
	ROM_LOAD( "1052.bin",  0x30000, 0x10000, CRC(d96437ad) SHA1(b0b5cd75de4048e54b9d7d09a75381eb73c22ee1) )
ROM_END



/*************************************
 *
 *	Machine initialization
 *
 *************************************/

static const data16_t default_eeprom[] =
{
	0x0001,0x01FF,0x0F00,0x011A,0x014A,0x0100,0x01A1,0x0200,
	0x010E,0x01AF,0x0300,0x01FF,0x0114,0x0144,0x01FF,0x0F00,
	0x011A,0x014A,0x0100,0x01A1,0x0200,0x010E,0x01AF,0x0300,
	0x01FF,0x0114,0x0144,0x01FF,0x0E00,0x01FF,0x0E00,0x01FF,
	0x0E00,0x01FF,0x0E00,0x01FF,0x0E00,0x01FF,0x0E00,0x01FF,
	0x0E00,0x01A8,0x0131,0x010B,0x0100,0x014C,0x0A00,0x01FF,
	0x0E00,0x01FF,0x0E00,0x01FF,0x0E00,0xB5FF,0x0E00,0x01FF,
	0x0E00,0x01FF,0x0E00,0x01FF,0x0E00,0x01FF,0x0E00,0x01FF,
	0x0E00,0x01FF,0x0E00,0x0000
};


static DRIVER_INIT( cyberbal )
{
	atarigen_eeprom_default = default_eeprom;
	atarigen_slapstic_init(0, 0x018000, 0);
	atarigen_init_6502_speedup(1, 0x4191, 0x41A9);

	/* make sure the banks are pointing to the correct location */
	cpu_setbank(1, atarigen_playfield2);
	cpu_setbank(3, atarigen_playfield);
}


static DRIVER_INIT( cyberbt )
{
	atarigen_eeprom_default = default_eeprom;
	atarigen_slapstic_init(0, 0x018000, 116);
	atarigen_init_6502_speedup(1, 0x4191, 0x41A9);

	/* make sure the banks are pointing to the correct location */
	cpu_setbank(1, atarigen_playfield2);
	cpu_setbank(3, atarigen_playfield);
}


static DRIVER_INIT( cyberb2p )
{
	atarigen_eeprom_default = default_eeprom;
	atarijsa_init(1, 3, 2, 0x8000);
	atarigen_init_6502_speedup(1, 0x4159, 0x4171);
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1988, cyberbal, 0,        cyberbal, cyberbal, cyberbal, ROT0, "Atari Games", "Cyberball (Version 4)" )
GAME( 1988, cyberba2, cyberbal, cyberbal, cyberbal, cyberbal, ROT0, "Atari Games", "Cyberball (Version 2)" )
GAME( 1989, cyberbt,  cyberbal, cyberbal, cyberbal, cyberbt,  ROT0, "Atari Games", "Tournament Cyberball 2072" )
GAME( 1989, cyberb2p, cyberbal, cyberb2p, cyberb2p, cyberb2p, ROT0, "Atari Games", "Cyberball 2072 (2 player)" )
