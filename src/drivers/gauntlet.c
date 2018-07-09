/***************************************************************************

	Atari Gauntlet hardware

	driver by Aaron Giles

	Games supported:
		* Gauntlet (1985) [3 sets]
		* Gauntlet 2-player Version (1985)
		* Gauntlet II (1986)
		* Vindicators Part II (1988)

	Known bugs:
		* none at this time

****************************************************************************

	Memory map

****************************************************************************

	========================================================================
	MAIN CPU
	========================================================================
	000000-037FFF   R     xxxxxxxx xxxxxxxx   Program ROM
	038000-03FFFF   R     xxxxxxxx xxxxxxxx   Slapstic-protected ROM
	040000-07FFFF   R     xxxxxxxx xxxxxxxx   Program ROM
	800000-801FFF   R/W   xxxxxxxx xxxxxxxx   Program RAM
	802000-802FFF   R/W   -------- xxxxxxxx   EEPROM
	803000          R     -------- xxxxxxxx   Input port 1
	803002          R     -------- xxxxxxxx   Input port 2
	803004          R     -------- xxxxxxxx   Input port 3
	803006          R     -------- xxxxxxxx   Input port 4
	803008          R     -------- -xxxx---   Status port
	                R     -------- -x------      (VBLANK)
	                R     -------- --x-----      (Sound command buffer full)
	                R     -------- ---x----      (Sound response buffer full)
	                R     -------- ----x---      (Self test)
	80300E          R     -------- xxxxxxxx   Sound response read
	803100            W   -------- --------   Watchdog reset
	80312E            W   -------- -------x   Sound CPU reset
	803140            W   -------- --------   VBLANK IRQ acknowledge
	803150            W   -------- --------   EEPROM enable
	803170            W   -------- xxxxxxxx   Sound command write
	900000-901FFF   R/W   xxxxxxxx xxxxxxxx   Playfield RAM (64x64 tiles)
	                R/W   x------- --------      (Horizontal flip)
	                R/W   -xxx---- --------      (Palette select)
	                R/W   ----xxxx xxxxxxxx      (Tile index)
	902000-903FFF   R/W   xxxxxxxx xxxxxxxx   Motion object RAM (1024 entries x 4 words)
	                R/W   -xxxxxxx xxxxxxxx      (0: Tile index)
	                R/W   xxxxxxxx x-------      (1024: X position)
	                R/W   -------- ----xxxx      (1024: Palette select)
	                R/W   xxxxxxxx x-------      (2048: Y position)
	                R/W   -------- -x------      (2048: Horizontal flip)
	                R/W   -------- --xxx---      (2048: Number of X tiles - 1)
	                R/W   -------- -----xxx      (2048: Number of Y tiles - 1)
	                R/W   ------xx xxxxxxxx      (3072: Link to next object)
	904000-904FFF   R/W   xxxxxxxx xxxxxxxx   Spare video RAM
	905000-905FFF   R/W   xxxxxxxx xxxxxxxx   Alphanumerics RAM (64x32 tiles)
	                R/W   x------- --------      (Opaque/transparent)
	                R/W   -xxxxx-- --------      (Palette select)
	                R/W   ------xx xxxxxxxx      (Tile index)
	905F6E          R/W   xxxxxxxx x-----xx   Playfield Y scroll/tile bank select
	                R/W   xxxxxxxx x-------      (Playfield Y scroll)
	                R/W   -------- ------xx      (Playfield tile bank select)
	910000-9101FF   R/W   xxxxxxxx xxxxxxxx   Alphanumercs palette RAM (256 entries)
	                R/W   xxxx---- --------      (Intensity)
	                R/W   ----xxxx --------      (Red)
	                R/W   -------- xxxx----      (Green)
	                R/W   -------- ----xxxx      (Blue)
	910200-9103FF   R/W   xxxxxxxx xxxxxxxx   Motion object palette RAM (256 entries)
	910400-9105FF   R/W   xxxxxxxx xxxxxxxx   Playfield palette RAM (256 entries)
	910600-9107FF   R/W   xxxxxxxx xxxxxxxx   Extra palette RAM (256 entries)
	930000            W   xxxxxxxx x-------   Playfield X scroll
	========================================================================
	Interrupts:
		IRQ4 = VBLANK
		IRQ6 = sound CPU communications
	========================================================================


	========================================================================
	SOUND CPU
	========================================================================
	0000-0FFF   R/W   xxxxxxxx   Program RAM
	1000          W   xxxxxxxx   Sound response write
	1010        R     xxxxxxxx   Sound command read
	1020        R     ----xxxx   Coin inputs
	            R     ----x---      (Coin 1)
	            R     -----x--      (Coin 2)
	            R     ------x-      (Coin 3)
	            R     -------x      (Coin 4)
	1020          W   xxxxxxxx   Mixer control
	              W   xxx-----      (TMS5220 volume)
	              W   ---xx---      (POKEY volume)
	              W   -----xxx      (YM2151 volume)
	1030        R     xxxx----   Sound status read
	            R     x-------      (Sound command buffer full)
	            R     -x------      (Sound response buffer full)
	            R     --x-----      (TMS5220 ready)
	            R     ---x----      (Self test)
	1030          W   x-------   YM2151 reset
	1031          W   x-------   TMS5220 data strobe
	1032          W   x-------   TMS5220 reset
	1033          W   x-------   TMS5220 frequency
	1800-180F   R/W   xxxxxxxx   POKEY communications
	1810-1811   R/W   xxxxxxxx   YM2151 communications
	1820          W   xxxxxxxx   TMS5220 data latch
	1830        R/W   --------   IRQ acknowledge
	4000-FFFF   R     xxxxxxxx   Program ROM
	========================================================================
	Interrupts:
		IRQ = timed interrupt
		NMI = latch on sound command
	========================================================================

****************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "gauntlet.h"



/*************************************
 *
 *	Statics
 *
 *************************************/

static data16_t *speed_check;
static UINT32 last_speed_check;

static UINT8 speech_val;
static UINT8 last_speech_write;
static UINT8 speech_squeak;

static data16_t sound_reset_val;



/*************************************
 *
 *	Initialization & interrupts
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_video_int_state)
		newstate |= 4;
	if (atarigen_sound_int_state)
		newstate |= 6;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static void scanline_update(int scanline)
{
	/* sound IRQ is on 32V */
	if (scanline & 32)
		atarigen_6502_irq_gen();
	else
		atarigen_6502_irq_ack_r(0);
}


static MACHINE_INIT( gauntlet )
{
	last_speed_check = 0;
	last_speech_write = 0x80;
	sound_reset_val = 1;
	speech_squeak = 0;

	atarigen_eeprom_reset();
	atarigen_slapstic_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(scanline_update, 32);
	atarigen_sound_io_reset(1);
}



/*************************************
 *
 *	Controller reads
 *
 *************************************/

static int fake_inputs(int real_port, int fake_port)
{
	int result = readinputport(real_port);
	int fake = readinputport(fake_port);

	if (fake & 0x01)			/* up */
	{
		if (fake & 0x04)		/* up and left */
			result &= ~0x20;
		else if (fake & 0x08)	/* up and right */
			result &= ~0x10;
		else					/* up only */
			result &= ~0x30;
	}
	else if (fake & 0x02)		/* down */
	{
		if (fake & 0x04)		/* down and left */
			result &= ~0x80;
		else if (fake & 0x08)	/* down and right */
			result &= ~0x40;
		else					/* down only */
			result &= ~0xc0;
	}
	else if (fake & 0x04)		/* left only */
		result &= ~0x60;
	else if (fake & 0x08)		/* right only */
		result &= ~0x90;

	return result;
}


static READ16_HANDLER( port0_r )
{
	return vindctr2_screen_refresh ?
				fake_inputs(0, 6) :
				readinputport(readinputport(6));
}


static READ16_HANDLER( port1_r )
{
	return vindctr2_screen_refresh ?
				fake_inputs(1, 7) :
				readinputport((readinputport(6) != 1) ? 1 : 0);
}


static READ16_HANDLER( port2_r )
{
	return vindctr2_screen_refresh ?
				readinputport(2) :
				readinputport((readinputport(6) != 2) ? 2 : 0);
}


static READ16_HANDLER( port3_r )
{
	return vindctr2_screen_refresh ?
				readinputport(3) :
				readinputport((readinputport(6) != 3) ? 3 : 0);
}


static READ16_HANDLER( port4_r )
{
	int temp = readinputport(4);
	if (atarigen_cpu_to_sound_ready) temp ^= 0x0020;
	if (atarigen_sound_to_cpu_ready) temp ^= 0x0010;
	return temp;
}



/*************************************
 *
 *	Sound reset
 *
 *************************************/

static WRITE16_HANDLER( sound_reset_w )
{
	if (ACCESSING_LSB)
	{
		int oldword = sound_reset_val;
		COMBINE_DATA(&sound_reset_val);

		if ((oldword ^ sound_reset_val) & 1)
		{
			cpu_set_reset_line(1, (sound_reset_val & 1) ? CLEAR_LINE : ASSERT_LINE);
			atarigen_sound_reset();
		}
	}
}



/*************************************
 *
 *	Speed cheats
 *
 *************************************/

static READ16_HANDLER( speedup_68010_r )
{
	int result = speed_check[offset];
	int time = activecpu_gettotalcycles();
	int delta = time - last_speed_check;

	last_speed_check = time;
	if (delta <= 100 && result == 0 && delta >= 0)
		cpu_spin();

	return result;
}


static WRITE16_HANDLER( speedup_68010_w )
{
	last_speed_check -= 1000;
	COMBINE_DATA(&speed_check[offset]);
}



/*************************************
 *
 *	Sound I/O inputs
 *
 *************************************/

static READ_HANDLER( switch_6502_r )
{
	int temp = 0x30;

	if (atarigen_cpu_to_sound_ready) temp ^= 0x80;
	if (atarigen_sound_to_cpu_ready) temp ^= 0x40;
	if (tms5220_ready_r()) temp ^= 0x20;
	if (!(readinputport(4) & 0x0008)) temp ^= 0x10;

	return temp;
}



/*************************************
 *
 *	Sound TMS5220 write
 *
 *************************************/

static WRITE_HANDLER( tms5220_w )
{
	speech_val = data;
}



/*************************************
 *
 *	Sound control write
 *
 *************************************/

static WRITE_HANDLER( sound_ctl_w )
{
	switch (offset & 7)
	{
		case 0:	/* music reset, bit D7, low reset */
			break;

		case 1:	/* speech write, bit D7, active low */
			if (((data ^ last_speech_write) & 0x80) && (data & 0x80))
				tms5220_data_w(0, speech_val);
			last_speech_write = data;
			break;

		case 2:	/* speech reset, bit D7, active low */
			if (((data ^ last_speech_write) & 0x80) && (data & 0x80))
				tms5220_reset();
			break;

		case 3:	/* speech squeak, bit D7 */
			data = 5 | ((data >> 6) & 2);
			tms5220_set_frequency(ATARI_CLOCK_14MHz/2 / (16 - data));
			break;
	}
}



/*************************************
 *
 *	Sound mixer write
 *
 *************************************/

static WRITE_HANDLER( mixer_w )
{
	atarigen_set_ym2151_vol((data & 7) * 100 / 7);
	atarigen_set_pokey_vol(((data >> 3) & 3) * 100 / 3);
	atarigen_set_tms5220_vol(((data >> 5) & 7) * 100 / 7);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( main_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x800000, 0x801fff, MRA16_RAM },
	{ 0x802000, 0x802fff, atarigen_eeprom_r },
	{ 0x803000, 0x803001, port0_r },
	{ 0x803002, 0x803003, port1_r },
	{ 0x803004, 0x803005, port2_r },
	{ 0x803006, 0x803007, port3_r },
	{ 0x803008, 0x803009, port4_r },
	{ 0x80300e, 0x80300f, atarigen_sound_r },
	{ 0x900000, 0x905fff, MRA16_RAM },
	{ 0x910000, 0x9107ff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( main_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x800000, 0x801fff, MWA16_RAM },
	{ 0x802000, 0x802fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0x803100, 0x803101, watchdog_reset16_w },
	{ 0x80312e, 0x80312f, sound_reset_w },
	{ 0x803140, 0x803141, atarigen_video_int_ack_w },
	{ 0x803150, 0x803151, atarigen_eeprom_enable_w },
	{ 0x803170, 0x803171, atarigen_sound_w },
	{ 0x900000, 0x901fff, atarigen_playfield_w, &atarigen_playfield },
	{ 0x902000, 0x903fff, atarimo_0_spriteram_w, &atarimo_0_spriteram },
	{ 0x904000, 0x904fff, MWA16_RAM },
	{ 0x905f6e, 0x905f6f, gauntlet_yscroll_w, &atarigen_yscroll },
	{ 0x905000, 0x905f7f, atarigen_alpha_w, &atarigen_alpha },
	{ 0x905f80, 0x905fff, atarimo_0_slipram_w, &atarimo_0_slipram },
	{ 0x910000, 0x9107ff, paletteram16_IIIIRRRRGGGGBBBB_word_w, &paletteram16 },
	{ 0x930000, 0x930001, gauntlet_xscroll_w, &atarigen_xscroll },
MEMORY_END



/*************************************
 *
 *	Sound CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1010, 0x101f, atarigen_6502_sound_r },
	{ 0x1020, 0x102f, input_port_5_r },
	{ 0x1030, 0x103f, switch_6502_r },
	{ 0x1800, 0x180f, pokey1_r },
	{ 0x1811, 0x1811, YM2151_status_port_0_r },
	{ 0x1830, 0x183f, atarigen_6502_irq_ack_r },
	{ 0x4000, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x100f, atarigen_6502_sound_w },
	{ 0x1020, 0x102f, mixer_w },
	{ 0x1030, 0x103f, sound_ctl_w },
	{ 0x1800, 0x180f, pokey1_w },
	{ 0x1810, 0x1810, YM2151_register_port_0_w },
	{ 0x1811, 0x1811, YM2151_data_port_0_w },
	{ 0x1820, 0x182f, tms5220_w },
	{ 0x1830, 0x183f, atarigen_6502_irq_ack_w },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( gauntlet )
	PORT_START	/* 803000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x000c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803002 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x000c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803004 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x000c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803006 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x000c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803008 */
	PORT_BIT( 0x0007, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_SERVICE( 0x0008, IP_ACTIVE_LOW )
	PORT_BIT( 0x0030, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_VBLANK )
	PORT_BIT( 0xff80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* 1020 (sound) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* Fake! */
	PORT_DIPNAME( 0x0003, 0x0000, "Player 1 Plays" )
	PORT_DIPSETTING(      0x0000, "Red/Warrior" )
	PORT_DIPSETTING(      0x0001, "Blue/Valkyrie" )
	PORT_DIPSETTING(      0x0002, "Yellow/Wizard" )
	PORT_DIPSETTING(      0x0003, "Green/Elf" )
INPUT_PORTS_END


INPUT_PORTS_START( vindctr2 )
	PORT_START	/* 803000 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP   | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803002 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_PLAYER2 | IPF_2WAY )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP   | IPF_PLAYER2 | IPF_2WAY )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_PLAYER2 | IPF_2WAY )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_PLAYER2 | IPF_2WAY )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803004 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xfffc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803006 */
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 803008 */
	PORT_BIT( 0x0007, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_SERVICE( 0x0008, IP_ACTIVE_LOW )
	PORT_BIT( 0x0030, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW,  IPT_VBLANK )
	PORT_BIT( 0xff80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* 1020 (sound) */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* single joystick */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )

	PORT_START	/* single joystick */
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
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
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &pfmolayout,  256, 32 },
	{ REGION_GFX1, 0, &anlayout,      0, 64 },
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
	{ YM3012_VOL(48,MIXER_PAN_LEFT,48,MIXER_PAN_RIGHT) },
	{ 0 }
};


static struct POKEYinterface pokey_interface =
{
	1,
	ATARI_CLOCK_14MHz/8,
	{ 32 },
};


static struct TMS5220interface tms5220_interface =
{
	ATARI_CLOCK_14MHz/2/11,	/* potentially ATARI_CLOCK_14MHz/2/9 as well */
	80,
	0
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( gauntlet )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68010, ATARI_CLOCK_14MHz/2)
	MDRV_CPU_MEMORY(main_readmem,main_writemem)
	MDRV_CPU_VBLANK_INT(atarigen_video_int_gen,1)

	MDRV_CPU_ADD(M6502, ATARI_CLOCK_14MHz/8)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(gauntlet)
	MDRV_NVRAM_HANDLER(atarigen)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(42*8, 30*8)
	MDRV_VISIBLE_AREA(0*8, 42*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(gauntlet)
	MDRV_VIDEO_UPDATE(gauntlet)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(POKEY, pokey_interface)
	MDRV_SOUND_ADD(TMS5220, tms5220_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( gauntlet )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "gauntlt1.9a",  0x00000, 0x08000, CRC(46fe8743) SHA1(d5fa19e028a2f43658330c67c10e0c811d332780) )
	ROM_LOAD16_BYTE( "gauntlt1.9b",  0x00001, 0x08000, CRC(276e15c4) SHA1(7467b2ec21b1b4fcc18ff9387ce891495f4b064c) )
	ROM_LOAD16_BYTE( "gauntlt1.10a", 0x38000, 0x04000, CRC(6d99ed51) SHA1(a7bc18f32908451859ba5cdf1a5c97ecc5fe325f) )
	ROM_LOAD16_BYTE( "gauntlt1.10b", 0x38001, 0x04000, CRC(545ead91) SHA1(7fad5a63c6443249bb6dad5b2a1fd08ca5f11e10) )
	ROM_LOAD16_BYTE( "gauntlt1.7a",  0x40000, 0x08000, CRC(6fb8419c) SHA1(299fee0368f6027bacbb57fb469e817e64e0e41d) )
	ROM_LOAD16_BYTE( "gauntlt1.7b",  0x40001, 0x08000, CRC(931bd2a0) SHA1(d69b45758d1c252a93dbc2263efa9de1f972f62e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "gauntlt1.16r", 0x4000, 0x4000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "gauntlt1.16s", 0x8000, 0x8000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gauntlt1.6p",  0x00000, 0x04000, CRC(6c276a1d) SHA1(ec383a8fdcb28efb86b7f6ba4a3306fea5a09d72) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "gauntlt1.1a",  0x00000, 0x08000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "gauntlt1.1b",  0x08000, 0x08000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )

	ROM_LOAD( "gauntlt1.1l",  0x10000, 0x08000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "gauntlt1.1mn", 0x18000, 0x08000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )

	ROM_LOAD( "gauntlt1.2a",  0x20000, 0x08000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "gauntlt1.2b",  0x28000, 0x08000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )

	ROM_LOAD( "gauntlt1.2l",  0x30000, 0x08000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "gauntlt1.2mn", 0x38000, 0x08000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gauntir1 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "gaun1ir1.9a",  0x00000, 0x08000, CRC(fd871f81) SHA1(111615cb3990fe2121ed5b3dd0c28054c98ef665) )
	ROM_LOAD16_BYTE( "gaun1ir1.9b",  0x00001, 0x08000, CRC(bcb2fb1d) SHA1(62f2acf81d8094617e4fcaa427e47c5940d85ad2) )
	ROM_LOAD16_BYTE( "gaun1ir1.10a", 0x38000, 0x04000, CRC(4642cd95) SHA1(96ff5a28a8ccd80d1a09bd1c5ce038ce5b400ac7) )
	ROM_LOAD16_BYTE( "gaun1ir1.10b", 0x38001, 0x04000, CRC(c8df945e) SHA1(71d675aaed7e128bd5fd9b137ddd1b1751ecf681) )
	ROM_LOAD16_BYTE( "gaun1ir1.7a",  0x40000, 0x08000, CRC(c57377b3) SHA1(4e7bf488240ec85ed4efd76a69d77f0308459ee5) )
	ROM_LOAD16_BYTE( "gaun1ir1.7b",  0x40001, 0x08000, CRC(1cac2071) SHA1(e8038c00e17dea6df6bd251505e525e3ef1a4c80) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "gauntlt1.16r", 0x4000, 0x4000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "gauntlt1.16s", 0x8000, 0x8000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gauntlt1.6p",  0x00000, 0x04000, CRC(6c276a1d) SHA1(ec383a8fdcb28efb86b7f6ba4a3306fea5a09d72) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "gauntlt1.1a",  0x00000, 0x08000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "gauntlt1.1b",  0x08000, 0x08000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )

	ROM_LOAD( "gauntlt1.1l",  0x10000, 0x08000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "gauntlt1.1mn", 0x18000, 0x08000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )

	ROM_LOAD( "gauntlt1.2a",  0x20000, 0x08000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "gauntlt1.2b",  0x28000, 0x08000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )

	ROM_LOAD( "gauntlt1.2l",  0x30000, 0x08000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "gauntlt1.2mn", 0x38000, 0x08000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gauntir2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "gaun1ir1.9a",  0x00000, 0x08000, CRC(fd871f81) SHA1(111615cb3990fe2121ed5b3dd0c28054c98ef665) )
	ROM_LOAD16_BYTE( "gaun1ir1.9b",  0x00001, 0x08000, CRC(bcb2fb1d) SHA1(62f2acf81d8094617e4fcaa427e47c5940d85ad2) )
	ROM_LOAD16_BYTE( "gaun1ir1.10a", 0x38000, 0x04000, CRC(4642cd95) SHA1(96ff5a28a8ccd80d1a09bd1c5ce038ce5b400ac7) )
	ROM_LOAD16_BYTE( "gaun1ir1.10b", 0x38001, 0x04000, CRC(c8df945e) SHA1(71d675aaed7e128bd5fd9b137ddd1b1751ecf681) )
	ROM_LOAD16_BYTE( "gaun1ir2.7a",  0x40000, 0x08000, CRC(73e1ad79) SHA1(11c17f764cbbe87acca05c9e6179010b09c5a856) )
	ROM_LOAD16_BYTE( "gaun1ir2.7b",  0x40001, 0x08000, CRC(fd248cea) SHA1(85db2c3b31fa8d9c8a048f553c3b195b2ff43586) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "gauntlt1.16r", 0x4000, 0x4000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "gauntlt1.16s", 0x8000, 0x8000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gauntlt1.6p",  0x00000, 0x04000, CRC(6c276a1d) SHA1(ec383a8fdcb28efb86b7f6ba4a3306fea5a09d72) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "gauntlt1.1a",  0x00000, 0x08000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "gauntlt1.1b",  0x08000, 0x08000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )

	ROM_LOAD( "gauntlt1.1l",  0x10000, 0x08000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "gauntlt1.1mn", 0x18000, 0x08000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )

	ROM_LOAD( "gauntlt1.2a",  0x20000, 0x08000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "gauntlt1.2b",  0x28000, 0x08000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )

	ROM_LOAD( "gauntlt1.2l",  0x30000, 0x08000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "gauntlt1.2mn", 0x38000, 0x08000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gaunt2p )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "gaunt2p.9a",   0x00000, 0x08000, CRC(8784133f) SHA1(98017427d84209405bb15d95a47bda5e1bd69f45) )
	ROM_LOAD16_BYTE( "gaunt2p.9b",   0x00001, 0x08000, CRC(2843bde3) SHA1(15e480c5245fd407f0fd5f0a3f3189ff18de88b3) )
	ROM_LOAD16_BYTE( "gauntlt1.10a", 0x38000, 0x04000, CRC(6d99ed51) SHA1(a7bc18f32908451859ba5cdf1a5c97ecc5fe325f) )
	ROM_LOAD16_BYTE( "gauntlt1.10b", 0x38001, 0x04000, CRC(545ead91) SHA1(7fad5a63c6443249bb6dad5b2a1fd08ca5f11e10) )
	ROM_LOAD16_BYTE( "gaunt2p.7a",   0x40000, 0x08000, CRC(5b4ee415) SHA1(dd9faba778710a86780b51d13deef1c9ebce0d44) )
	ROM_LOAD16_BYTE( "gaunt2p.7b",   0x40001, 0x08000, CRC(41f5c9e2) SHA1(791609520686ad48aaa76db1b3192ececf0d4e91) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "gauntlt1.16r", 0x4000, 0x4000, CRC(6ee7f3cc) SHA1(b86676340b06f07c164690862c1f6f75f30c080b) )
	ROM_LOAD( "gauntlt1.16s", 0x8000, 0x8000, CRC(fa19861f) SHA1(7568b4ab526bd5849f7ef70dfa6d1ef1f30c0abc) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gauntlt1.6p",  0x00000, 0x04000, CRC(6c276a1d) SHA1(ec383a8fdcb28efb86b7f6ba4a3306fea5a09d72) )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "gauntlt1.1a",  0x00000, 0x08000, CRC(91700f33) SHA1(fac1ce700c4cd46b643307998df781d637f193aa) )
	ROM_LOAD( "gauntlt1.1b",  0x08000, 0x08000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )

	ROM_LOAD( "gauntlt1.1l",  0x10000, 0x08000, CRC(d497d0a8) SHA1(bb715bcec7f783dd04151e2e3b221a72133bf17d) )
	ROM_LOAD( "gauntlt1.1mn", 0x18000, 0x08000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )

	ROM_LOAD( "gauntlt1.2a",  0x20000, 0x08000, CRC(9510b898) SHA1(e6c8c7af1898d548f0f01e4ff37c2c7b22c0b5c2) )
	ROM_LOAD( "gauntlt1.2b",  0x28000, 0x08000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )

	ROM_LOAD( "gauntlt1.2l",  0x30000, 0x08000, CRC(29a5db41) SHA1(94f4f5dd39e724570a0f54af176ad018497697fd) )
	ROM_LOAD( "gauntlt1.2mn", 0x38000, 0x08000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
ROM_END


ROM_START( gaunt2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "gauntlt2.9a",  0x00000, 0x08000, CRC(46fe8743) SHA1(d5fa19e028a2f43658330c67c10e0c811d332780) )
	ROM_LOAD16_BYTE( "gauntlt2.9b",  0x00001, 0x08000, CRC(276e15c4) SHA1(7467b2ec21b1b4fcc18ff9387ce891495f4b064c) )
	ROM_LOAD16_BYTE( "gauntlt2.10a", 0x38000, 0x04000, CRC(45dfda47) SHA1(a9a03150f5a0ad6ce62c5cfdffb4a9f54340590c) )
	ROM_LOAD16_BYTE( "gauntlt2.10b", 0x38001, 0x04000, CRC(343c029c) SHA1(d2df4e5b036500dcc537a1e0025abb2a8c730bdd) )
	ROM_LOAD16_BYTE( "gauntlt2.7a",  0x40000, 0x08000, CRC(58a0a9a3) SHA1(7f51184840e3c96574836b8a00bfb4a7a5f508d0) )
	ROM_LOAD16_BYTE( "gauntlt2.7b",  0x40001, 0x08000, CRC(658f0da8) SHA1(dfce027ea50188659907be698aeb26f9d8bfab23) )
	ROM_LOAD16_BYTE( "gauntlt2.6a",  0x50000, 0x08000, CRC(ae301bba) SHA1(3d93236aaffe6ef692e5073b1828633e8abf0ce4) )
	ROM_LOAD16_BYTE( "gauntlt2.6b",  0x50001, 0x08000, CRC(e94aaa8a) SHA1(378c582c360440b808820bcd3be78ec6e8800c34) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "gauntlt2.16r", 0x4000, 0x4000, CRC(5c731006) SHA1(045ad571db34ef870b1bf003e77eea403204f55b) )
	ROM_LOAD( "gauntlt2.16s", 0x8000, 0x8000, CRC(dc3591e7) SHA1(6d0d8493609974bd5a63be858b045fe4db35d8df) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gauntlt2.6p",  0x00000, 0x04000, CRC(d101905d) SHA1(6f50a84f4d263a7a459b642fa49a619e877535b6) )

	ROM_REGION( 0x60000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "gauntlt2.1a",  0x00000, 0x08000, CRC(09df6e23) SHA1(726984275c6a338c12ec0c4cc449f92f4a7a138c) )
	ROM_LOAD( "gauntlt2.1b",  0x08000, 0x08000, CRC(869330be) SHA1(5dfaaf54ee2b3c0eaf35e8c17558313db9791616) )
	ROM_LOAD( "gauntlt2.1c",  0x10000, 0x04000, CRC(e4c98f01) SHA1(a24bece3196d13c38e4acdbf62783860253ba67d) )
	ROM_RELOAD(               0x14000, 0x04000 )

	ROM_LOAD( "gauntlt2.1l",  0x18000, 0x08000, CRC(33cb476e) SHA1(e0757ee0120de2d38be44f8dc8702972c35b87b3) )
	ROM_LOAD( "gauntlt2.1mn", 0x20000, 0x08000, CRC(29ef9882) SHA1(91e1465af6505b35cd97434c13d2b4d40a085946) )
	ROM_LOAD( "gauntlt2.1p",  0x28000, 0x04000, CRC(c4857879) SHA1(3b4ce96da0d178b4bc2d05b5b51b42c7ec461113) )
	ROM_RELOAD(               0x2c000, 0x04000 )

	ROM_LOAD( "gauntlt2.2a",  0x30000, 0x08000, CRC(f71e2503) SHA1(244e108668eaef6b64c6ff733b08b9ee6b7a2d2b) )
	ROM_LOAD( "gauntlt2.2b",  0x38000, 0x08000, CRC(11e0ac5b) SHA1(729b7561d59d94ef33874a134b97bcd37573dfa6) )
	ROM_LOAD( "gauntlt2.2c",  0x40000, 0x04000, CRC(d9c2c2d1) SHA1(185e38c75c06b6ca131a17ee3a46098279bfe17e) )
	ROM_RELOAD(               0x44000, 0x04000 )

	ROM_LOAD( "gauntlt2.2l",  0x48000, 0x08000, CRC(9e30b2e9) SHA1(e9b513089eaf3bec269058b437fefe7075a3fd6f) )
	ROM_LOAD( "gauntlt2.2mn", 0x50000, 0x08000, CRC(8bf3b263) SHA1(683d900ab7591ee661218be2406fb375a12e435c) )
	ROM_LOAD( "gauntlt2.2p",  0x58000, 0x04000, CRC(a32c732a) SHA1(abe801dff7bb3f2712e2189c2b91f172d941fccd) )
	ROM_RELOAD(               0x5c000, 0x04000 )
ROM_END


ROM_START( vindctr2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD16_BYTE( "1186", 0x00000, 0x08000, CRC(af138263) SHA1(acb1b7f497b83c9950d51776e620adee347b48a7) )
	ROM_LOAD16_BYTE( "1187", 0x00001, 0x08000, CRC(44baff64) SHA1(3cb3af1e93208ac139e90482d329e2368fde66d5) )
	ROM_LOAD16_BYTE( "1196", 0x38000, 0x04000, CRC(c92bf6dd) SHA1(bdd179d6fae9565823917baefae17ace71be8191) )
	ROM_LOAD16_BYTE( "1197", 0x38001, 0x04000, CRC(d7ace347) SHA1(9842cec069b11bd77908801be4c454571a8f04c2) )
	ROM_LOAD16_BYTE( "3188", 0x40000, 0x08000, CRC(10f558d2) SHA1(b9ea79a7f3cbd0122d861180631a601ff77fae00) )
	ROM_LOAD16_BYTE( "3189", 0x40001, 0x08000, CRC(302e24b6) SHA1(b138138ae397a0e911b0502d6622fff1f1419716) )
	ROM_LOAD16_BYTE( "2190", 0x50000, 0x08000, CRC(e7dc2b74) SHA1(55da5d0293d3ff41bdeaaa9b52d153bfb88bfcad) )
	ROM_LOAD16_BYTE( "2191", 0x50001, 0x08000, CRC(ed8ed86e) SHA1(8fedb1c25d3f4069df68118266faf0a74561a6d7) )
	ROM_LOAD16_BYTE( "2192", 0x60000, 0x08000, CRC(eec2c93d) SHA1(d35e871ccbbccb35e35813b2cf9bf8821c000440) )
	ROM_LOAD16_BYTE( "2193", 0x60001, 0x08000, CRC(3fbee9aa) SHA1(5802291e4a71cece4175ef1d2cecdaabfc096c3d) )
	ROM_LOAD16_BYTE( "1194", 0x70000, 0x08000, CRC(e6bcf458) SHA1(0492ebca7baa5ee456b739628200c094cdf4879e) )
	ROM_LOAD16_BYTE( "1195", 0x70001, 0x08000, CRC(b9bf245d) SHA1(ba190518fd7f630976d97b00af7e28a113a33ce1) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for 6502 code */
	ROM_LOAD( "1160", 0x4000, 0x4000, CRC(eef0a003) SHA1(4b1c0810e8c60e364051ed867fed0dc3a0b3a872) )
	ROM_LOAD( "1161", 0x8000, 0x8000, CRC(68c74337) SHA1(13a9333e0b58ce771774632ecdfa8ca9c9664e57) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "1198",  0x00000, 0x04000, CRC(f99b631a) SHA1(7a2430b6810c77b0f717d6e9d71823eadbcf6013) )

	ROM_REGION( 0xc0000, REGION_GFX2, ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "1101", 0x00000, 0x08000, CRC(dd3833ad) SHA1(e78a44b5f2033b618b5879a8a39bfdf428b5e4c7) )
	ROM_LOAD( "1166", 0x08000, 0x08000, CRC(e2db50a0) SHA1(953e621f7312340dcbda9e4a727ebeba69ba7d4e) )
	ROM_LOAD( "1170", 0x10000, 0x08000, CRC(f050ab43) SHA1(72fbba20b6c8a1838842084c07157cdc2fd923c1) )
	ROM_LOAD( "1174", 0x18000, 0x08000, CRC(b6704bd1) SHA1(0876e51e54a0f876f637f934d0ed2808d67a3515) )
	ROM_LOAD( "1178", 0x20000, 0x08000, CRC(d3006f05) SHA1(00e08b9b11eca017fd6ee0dea6f1818fcfddd830) )
	ROM_LOAD( "1182", 0x28000, 0x08000, CRC(9046e985) SHA1(0cc0cd67faa467dcdf6b90c106a3662ff9e5fe41) )

	ROM_LOAD( "1102", 0x30000, 0x08000, CRC(d505b04a) SHA1(cabf61f74146fbe84c7db368f014e17237126056) )
	ROM_LOAD( "1167", 0x38000, 0x08000, CRC(1869c76d) SHA1(c2ed2b94726a0a97925d0c05ad65fe8c05bac01b) )
	ROM_LOAD( "1171", 0x40000, 0x08000, CRC(1b229c2b) SHA1(b8bf5e17d8b73bdf04bbb9ca553ce8e69c8f71db) )
	ROM_LOAD( "1175", 0x48000, 0x08000, CRC(73c41aca) SHA1(c401f5d1664c9a86231feda0ba110f586632a1a2) )
	ROM_LOAD( "1179", 0x50000, 0x08000, CRC(9b7cb0ef) SHA1(7febc479ddf52a5b72eba2abc9e12d3e48e804ff) )
	ROM_LOAD( "1183", 0x58000, 0x08000, CRC(393bba42) SHA1(1c7eb448d7a4862d16bef7aa1419e8db99fb6815) )

	ROM_LOAD( "1103", 0x60000, 0x08000, CRC(50e76162) SHA1(7aaf55c4d0ba44609c29d222babe2fb4990d0004) )
	ROM_LOAD( "1168", 0x68000, 0x08000, CRC(35c78469) SHA1(1b3ab6e826ec2a8c8bef1d35a8ed2c46651336a6) )
	ROM_LOAD( "1172", 0x70000, 0x08000, CRC(314ac268) SHA1(2a3b2be3b548d60489265bf78a4ab135c2bff692) )
	ROM_LOAD( "1176", 0x78000, 0x08000, CRC(061d79db) SHA1(adf94aa01547df578039567126ca9ea53be33c37) )
	ROM_LOAD( "1180", 0x80000, 0x08000, CRC(89c1fe16) SHA1(e58fbe710f11529151814892e380ba0fa3296995) )
	ROM_LOAD( "1184", 0x88000, 0x08000, CRC(541209d3) SHA1(d862f1759c1e56d61e60e0760f7743b10f65e765) )

	ROM_LOAD( "1104", 0x90000, 0x08000, CRC(9484ba65) SHA1(ad5e3589c4bcc7be814e2dc274de0fe9d321e37c) )
	ROM_LOAD( "1169", 0x98000, 0x08000, CRC(132d3337) SHA1(4e50f35773ab19a0319a6fbe81e87ef69d7d0ee8) )
	ROM_LOAD( "1173", 0xa0000, 0x08000, CRC(98de2426) SHA1(2f3df9abef8a5ae3c09346d70ce96e65b728ffaf) )
	ROM_LOAD( "1177", 0xa8000, 0x08000, CRC(9d0824f8) SHA1(db921fea0ffd6c07af3affe7e3cf9282d48e6eee) )
	ROM_LOAD( "1181", 0xb0000, 0x08000, CRC(9e62b27c) SHA1(2df265abe412613beb6bee0b6179232b4c45d5fc) )
	ROM_LOAD( "1185", 0xb8000, 0x08000, CRC(9d62f6b7) SHA1(0d0f94dd81958c41674096d326ad1662284209e6) )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( gauntlet )
{
	atarigen_eeprom_default = NULL;
	atarigen_slapstic_init(0, 0x038000, 104);
	atarigen_init_6502_speedup(1, 0x410f, 0x4127);

	/* swap the top and bottom halves of the main CPU ROM images */
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x000000, memory_region(REGION_CPU1) + 0x008000, 0x8000);
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x040000, memory_region(REGION_CPU1) + 0x048000, 0x8000);

	/* indicate that we're not vindicators 2 */
	vindctr2_screen_refresh = 0;

	/* speed up the 68010 */
	speed_check = install_mem_write16_handler(0, 0x904002, 0x904003, speedup_68010_w);
	install_mem_read16_handler(0, 0x904002, 0x904003, speedup_68010_r);
}


static DRIVER_INIT( gaunt2p )
{
	atarigen_eeprom_default = NULL;
	atarigen_slapstic_init(0, 0x038000, 107);
	atarigen_init_6502_speedup(1, 0x410f, 0x4127);

	/* swap the top and bottom halves of the main CPU ROM images */
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x000000, memory_region(REGION_CPU1) + 0x008000, 0x8000);
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x040000, memory_region(REGION_CPU1) + 0x048000, 0x8000);

	/* indicate that we're not vindicators 2 */
	vindctr2_screen_refresh = 0;

	/* speed up the 68010 */
	speed_check = install_mem_write16_handler(0, 0x904002, 0x904003, speedup_68010_w);
	install_mem_read16_handler(0, 0x904002, 0x904003, speedup_68010_r);
}


static DRIVER_INIT( gauntlet2 )
{
	atarigen_eeprom_default = NULL;
	atarigen_slapstic_init(0, 0x038000, 106);
	atarigen_init_6502_speedup(1, 0x410f, 0x4127);

	/* swap the top and bottom halves of the main CPU ROM images */
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x000000, memory_region(REGION_CPU1) + 0x008000, 0x8000);
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x040000, memory_region(REGION_CPU1) + 0x048000, 0x8000);
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x050000, memory_region(REGION_CPU1) + 0x058000, 0x8000);

	/* indicate that we're not vindicators 2 */
	vindctr2_screen_refresh = 0;

	/* speed up the 68010 */
	speed_check = install_mem_write16_handler(0, 0x904002, 0x904003, speedup_68010_w);
	install_mem_read16_handler(0, 0x904002, 0x904003, speedup_68010_r);
}


static DRIVER_INIT( vindctr2 )
{
	UINT8 *gfx2_base = memory_region(REGION_GFX2);
	UINT8 *data;
	int i;

	atarigen_eeprom_default = NULL;
	atarigen_slapstic_init(0, 0x038000, 118);
	atarigen_init_6502_speedup(1, 0x40ff, 0x4117);

	/* swap the top and bottom halves of the main CPU ROM images */
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x000000, memory_region(REGION_CPU1) + 0x008000, 0x8000);
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x040000, memory_region(REGION_CPU1) + 0x048000, 0x8000);
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x050000, memory_region(REGION_CPU1) + 0x058000, 0x8000);
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x060000, memory_region(REGION_CPU1) + 0x068000, 0x8000);
	atarigen_swap_mem(memory_region(REGION_CPU1) + 0x070000, memory_region(REGION_CPU1) + 0x078000, 0x8000);

	/* highly strange -- the address bits on the chip at 2J (and only that
	   chip) are scrambled -- this is verified on the schematics! */
	data = malloc(0x8000);
	if (data)
	{
		memcpy(data, &gfx2_base[0x88000], 0x8000);
		for (i = 0; i < 0x8000; i++)
		{
			int srcoffs = (i & 0x4000) | ((i << 11) & 0x3800) | ((i >> 3) & 0x07ff);
			gfx2_base[0x88000 + i] = data[srcoffs];
		}
		free(data);
	}

	/* indicate that we are vindicators 2 */
	vindctr2_screen_refresh = 1;
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAME( 1985, gauntlet, 0,        gauntlet, gauntlet, gauntlet,  ROT0, "Atari Games", "Gauntlet" )
GAME( 1985, gauntir1, gauntlet, gauntlet, gauntlet, gauntlet,  ROT0, "Atari Games", "Gauntlet (Intermediate Release 1)" )
GAME( 1985, gauntir2, gauntlet, gauntlet, gauntlet, gauntlet,  ROT0, "Atari Games", "Gauntlet (Intermediate Release 2)" )
GAME( 1985, gaunt2p,  gauntlet, gauntlet, gauntlet, gaunt2p,   ROT0, "Atari Games", "Gauntlet (2 Players)" )
GAME( 1986, gaunt2,   0,        gauntlet, gauntlet, gauntlet2, ROT0, "Atari Games", "Gauntlet II" )
GAME( 1988, vindctr2, 0,        gauntlet, vindctr2, vindctr2,  ROT0, "Atari Games", "Vindicators Part II" )
