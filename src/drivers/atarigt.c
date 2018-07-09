/***************************************************************************

	Atari GT hardware

	driver by Aaron Giles

	Games supported:
		* T-Mek (1994) [2 sets]
		* Primal Rage (1994) [2 sets]

	Known bugs:
		* protection devices unknown
		* gamma map (?)
		* CAGE audio system missing

****************************************************************************

	Memory map (TBA)

***************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/atarirle.h"
#include "cpu/m68000/m68000.h"
#include "atarigt.h"


#define LOG_PROTECTION		1



/*************************************
 *
 *	Statics
 *
 *************************************/

static void (*protection_w)(offs_t offset, data16_t data);
static void (*protection_r)(offs_t offset, data16_t *data);



/*************************************
 *
 *	Initialization
 *
 *************************************/

static void update_interrupts(void)
{
	int newstate = 0;

	if (atarigen_video_int_state)
		newstate = 4;
	if (atarigen_sound_int_state)
		newstate = 5;
	if (atarigen_scanline_int_state)
		newstate = 6;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static MACHINE_INIT( atarigt )
{
	atarigen_eeprom_reset();
	atarigen_interrupt_reset(update_interrupts);
	atarigen_scanline_timer_reset(atarigt_scanline_update, 8);
}



/*************************************
 *
 *	Input ports
 *
 *************************************/

static READ32_HANDLER( inputs_01_r )
{
	return (readinputport(0) << 16) | readinputport(1);
}


static READ32_HANDLER( special_port2_r )
{
	int temp = readinputport(2);
	temp ^= 0x0001;		/* /A2DRDY always high for now */
	temp ^= 0x0008;		/* A2D.EOC always high for now */
	return (temp << 16) | temp;
}


static READ32_HANDLER( special_port3_r )
{
	int temp = readinputport(3);
	if (atarigen_video_int_state) temp ^= 0x0001;
	if (atarigen_scanline_int_state) temp ^= 0x0002;
	return (temp << 16) | temp;
}



/*************************************
 *
 *	Output ports
 *
 *************************************/

static WRITE32_HANDLER( latch_w )
{
	/*
		D13 = 68.DISA
		D12 = ERASE
		D11 = /MOGO
		D8  = VCR
		D5  = /XRESET
		D4  = /SNDRES
		D3  = CC.L
		D0  = CC.R
	*/

	/* upper byte */
	if (!(mem_mask & 0xff000000))
	{
		/* bits 13-11 are the MO control bits */
		atarirle_control_w(0, (data >> 27) & 7);
	}

	if (!(mem_mask & 0x00ff0000))
	{
		coin_counter_w(0, data & 0x00080000);
		coin_counter_w(1, data & 0x00010000);
	}

//logerror("latch=%08X\n", latch);

//	if (ACCESSING_MSW32 && !ACCESSING_MSB32)
//		cpu_set_reset_line(1, (data & 0x100000) ? CLEAR_LINE : ASSERT_LINE);
}


static WRITE32_HANDLER( led_w )
{
//	logerror("LED = %08X & %08X\n", data, ~mem_mask);
}



/*************************************
 *
 *	Sound I/O
 *
 *************************************/

static data32_t sound_data;
static READ32_HANDLER( sound_data_r )
{
	return sound_data;
}


static WRITE32_HANDLER( sound_data_w )
{
	COMBINE_DATA(&sound_data);
}



/*************************************
 *
 *	T-Mek protection
 *
 *************************************/

#define ADDRSEQ_COUNT	4

static offs_t protaddr[ADDRSEQ_COUNT];
static UINT8 protmode;
static UINT16 protresult;
static UINT8 protdata[0x800];

static void tmek_update_mode(offs_t offset)
{
	int i;

	/* pop us into the readseq */
	for (i = 0; i < ADDRSEQ_COUNT - 1; i++)
		protaddr[i] = protaddr[i + 1];
	protaddr[ADDRSEQ_COUNT - 1] = offset;

	/* check for particular sequences */
	if (!protmode)
	{
		/* this is from the code at $20f90 */
		if (protaddr[1] == 0xdcc7c4 && protaddr[2] == 0xdcc7c4 && protaddr[3] == 0xdc4010)
		{
//			logerror("prot:Entering mode 1\n");
			protmode = 1;
		}

		/* this is from the code at $27592 */
		if (protaddr[0] == 0xdcc7ca && protaddr[1] == 0xdcc7ca && protaddr[2] == 0xdcc7c6 && protaddr[3] == 0xdc4022)
		{
//			logerror("prot:Entering mode 2\n");
			protmode = 2;
		}

		/* this is from the code at $3d8dc */
		if (protaddr[0] == 0xdcc7c0 && protaddr[1] == 0xdcc7c0 && protaddr[2] == 0xdc80f2 && protaddr[3] == 0xdc7af2)
		{
//			logerror("prot:Entering mode 3\n");
			protmode = 3;
		}
	}
}


static void tmek_protection_w(offs_t offset, UINT16 data)
{
#if LOG_PROTECTION
	logerror("%06X:Protection W@%06X = %04X\n", activecpu_get_previouspc(), offset, data);
#endif

/* mask = 0x78fff */

	/* track accesses */
	tmek_update_mode(offset);

	/* check for certain read sequences */
	if (protmode == 1 && offset >= 0xdc7800 && offset < 0xdc7800 + sizeof(protdata) * 2)
		protdata[(offset - 0xdc7800) / 2] = data;

	if (protmode == 2)
	{
		int temp = (offset - 0xdc7800) / 2;
		logerror("prot:mode 2 param = %04X\n", temp);
		protresult = temp * 0x6915 + 0x6915;
	}

	if (protmode == 3)
	{
		if (offset == 0xdc4700)
		{
			logerror("prot:Clearing mode 3\n");
			protmode = 0;
		}
	}
}

static void tmek_protection_r(offs_t offset, data16_t *data)
{
	/* track accesses */
	tmek_update_mode(offset);

#if LOG_PROTECTION
	logerror("%06X:Protection R@%06X\n", activecpu_get_previouspc(), offset);
#endif

	/* handle specific reads */
	switch (offset)
	{
		/* status register; the code spins on this waiting for the high bit to be set */
		case 0xdb8700:
//			if (protmode != 0)
			{
				*data = 0x8000;
			}
			break;

		/* some kind of result register */
		case 0xdcc7c2:
			if (protmode == 2)
			{
				*data = protresult;
				protmode = 0;
				logerror("prot:Clearing mode 2\n");
			}
			break;

		case 0xdcc7c4:
			if (protmode == 1)
			{
				protmode = 0;
				logerror("prot:Clearing mode 1\n");
			}
			break;
	}
}



/*************************************
 *
 *	Primal Rage protection
 *
 *************************************/

static void primage_update_mode(offs_t offset)
{
	int i;

	/* pop us into the readseq */
	for (i = 0; i < ADDRSEQ_COUNT - 1; i++)
		protaddr[i] = protaddr[i + 1];
	protaddr[ADDRSEQ_COUNT - 1] = offset;

	/* check for particular sequences */
	if (!protmode)
	{
		/* this is from the code at $20f90 */
		if (protaddr[1] == 0xdcc7c4 && protaddr[2] == 0xdcc7c4 && protaddr[3] == 0xdc4010)
		{
//			logerror("prot:Entering mode 1\n");
			protmode = 1;
		}

		/* this is from the code at $27592 */
		if (protaddr[0] == 0xdcc7ca && protaddr[1] == 0xdcc7ca && protaddr[2] == 0xdcc7c6 && protaddr[3] == 0xdc4022)
		{
//			logerror("prot:Entering mode 2\n");
			protmode = 2;
		}

		/* this is from the code at $3d8dc */
		if (protaddr[0] == 0xdcc7c0 && protaddr[1] == 0xdcc7c0 && protaddr[2] == 0xdc80f2 && protaddr[3] == 0xdc7af2)
		{
//			logerror("prot:Entering mode 3\n");
			protmode = 3;
		}
	}
}



static void primrage_protection_w(offs_t offset, data16_t data)
{
#if LOG_PROTECTION
{
	UINT32 pc = activecpu_get_previouspc();
	switch (pc)
	{
		/* protection code from 20f90 - 21000 */
		case 0x20fba:
			if (offset % 16 == 0) logerror("\n   ");
			logerror("W@%06X(%04X) ", offset, data);
			break;

		/* protection code from 27592 - 27664 */
		case 0x275f6:
			logerror("W@%06X(%04X) ", offset, data);
			break;

		/* protection code from 3d8dc - 3d95a */
		case 0x3d908:
		case 0x3d932:
		case 0x3d938:
		case 0x3d93e:
			logerror("W@%06X(%04X) ", offset, data);
			break;
		case 0x3d944:
			logerror("W@%06X(%04X) - done\n", offset, data);
			break;

		/* protection code from 437fa - 43860 */
		case 0x43830:
		case 0x43838:
			logerror("W@%06X(%04X) ", offset, data);
			break;

		/* catch anything else */
		default:
			logerror("%06X:Unknown protection W@%06X = %04X\n", activecpu_get_previouspc(), offset, data);
			break;
	}
}
#endif

/* mask = 0x78fff */

	/* track accesses */
	primage_update_mode(offset);

	/* check for certain read sequences */
	if (protmode == 1 && offset >= 0xdc7800 && offset < 0xdc7800 + sizeof(protdata) * 2)
		protdata[(offset - 0xdc7800) / 2] = data;

	if (protmode == 2)
	{
		int temp = (offset - 0xdc7800) / 2;
//		logerror("prot:mode 2 param = %04X\n", temp);
		protresult = temp * 0x6915 + 0x6915;
	}

	if (protmode == 3)
	{
		if (offset == 0xdc4700)
		{
//			logerror("prot:Clearing mode 3\n");
			protmode = 0;
		}
	}
}



static void primrage_protection_r(offs_t offset, data16_t *data)
{
	/* track accesses */
	primage_update_mode(offset);

#if LOG_PROTECTION
{
	UINT32 pc = activecpu_get_previouspc();
	UINT32 p1, p2, a6;
	switch (pc)
	{
		/* protection code from 20f90 - 21000 */
		case 0x20f90:
			logerror("Known Protection @ 20F90: R@%06X ", offset);
			break;
		case 0x20f98:
		case 0x20fa0:
			logerror("R@%06X ", offset);
			break;
		case 0x20fcc:
			logerror("R@%06X - done\n", offset);
			break;

		/* protection code from 27592 - 27664 */
		case 0x275bc:
			break;
		case 0x275cc:
			a6 = activecpu_get_reg(M68K_A6);
			p1 = (cpu_readmem24bedw_word(a6+8) << 16) | cpu_readmem24bedw_word(a6+10);
			p2 = (cpu_readmem24bedw_word(a6+12) << 16) | cpu_readmem24bedw_word(a6+14);
			logerror("Known Protection @ 275BC(%08X, %08X): R@%06X ", p1, p2, offset);
			break;
		case 0x275d2:
		case 0x275d8:
		case 0x275de:
		case 0x2761e:
		case 0x2762e:
			logerror("R@%06X ", offset);
			break;
		case 0x2763e:
			logerror("R@%06X - done\n", offset);
			break;

		/* protection code from 3d8dc - 3d95a */
		case 0x3d8f4:
			a6 = activecpu_get_reg(M68K_A6);
			p1 = (cpu_readmem24bedw_word(a6+12) << 16) | cpu_readmem24bedw_word(a6+14);
			logerror("Known Protection @ 3D8F4(%08X): R@%06X ", p1, offset);
			break;
		case 0x3d8fa:
		case 0x3d90e:
			logerror("R@%06X ", offset);
			break;

		/* protection code from 437fa - 43860 */
		case 0x43814:
			a6 = activecpu_get_reg(M68K_A6);
			p1 = cpu_readmem24bedw(a6+15);
			logerror("Known Protection @ 43814(%08X): R@%06X ", p1, offset);
			break;
		case 0x4381c:
		case 0x43840:
			logerror("R@%06X ", offset);
			break;
		case 0x43848:
			logerror("R@%06X - done\n", offset);
			break;

		/* catch anything else */
		default:
			logerror("%06X:Unknown protection R@%06X\n", activecpu_get_previouspc(), offset);
			break;
	}
}
#endif

	/* handle specific reads */
	switch (offset)
	{
		/* status register; the code spins on this waiting for the high bit to be set */
		case 0xdc4700:
//			if (protmode != 0)
			{
				*data = 0x8000;
			}
			break;

		/* some kind of result register */
		case 0xdcc7c2:
			if (protmode == 2)
			{
				*data = protresult;
				protmode = 0;
//				logerror("prot:Clearing mode 2\n");
			}
			break;

		case 0xdcc7c4:
			if (protmode == 1)
			{
				protmode = 0;
//				logerror("prot:Clearing mode 1\n");
			}
			break;
	}
}



/*************************************
 *
 *	Protection/color RAM
 *
 *************************************/

static READ32_HANDLER( colorram_protection_r )
{
	offs_t address = 0xd80000 + offset * 4;
	data32_t result32 = 0;
	data16_t result;

	if ((mem_mask & 0xffff0000) != 0xffff0000)
	{
		result = atarigt_colorram_r(address);
		(*protection_r)(address, &result);
		result32 |= result << 16;
	}
	if ((mem_mask & 0x0000ffff) != 0x0000ffff)
	{
		result = atarigt_colorram_r(address + 2);
		(*protection_r)(address + 2, &result);
		result32 |= result;
	}

	return result32;
}


static WRITE32_HANDLER( colorram_protection_w )
{
	offs_t address = 0xd80000 + offset * 4;

	if ((mem_mask & 0xffff0000) != 0xffff0000)
	{
		atarigt_colorram_w(address, data >> 16, mem_mask >> 16);
		(*protection_w)(address, data >> 16);
	}
	if ((mem_mask & 0x0000ffff) != 0x0000ffff)
	{
		atarigt_colorram_w(address + 2, data, mem_mask);
		(*protection_w)(address + 2, data);
	}
}



/*************************************
 *
 *	Primal Rage speedups
 *
 *************************************/

static data32_t *speedup;

static READ32_HANDLER( primrage_speedup_r )
{
	if (activecpu_get_previouspc() == 0x10b0e && ((*speedup >> 8) & 0xff) == 0)
		cpu_spinuntil_int();
	return *speedup;
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ32_START( readmem )
	{ 0x000000, 0x1fffff, MRA32_ROM },
	{ 0xc00000, 0xc00003, sound_data_r },
	{ 0xd20000, 0xd20fff, atarigen_eeprom_upper32_r },
	{ 0xd70000, 0xd7ffff, MRA32_RAM },
	{ 0xd80000, 0xdfffff, colorram_protection_r },
	{ 0xe80000, 0xe80003, inputs_01_r },
	{ 0xe82000, 0xe82003, special_port2_r },
	{ 0xe82004, 0xe82007, special_port3_r },
	{ 0xf80000, 0xffffff, MRA32_RAM },
MEMORY_END


static MEMORY_WRITE32_START( writemem )
	{ 0x000000, 0x1fffff, MWA32_ROM },
	{ 0xc00000, 0xc00003, sound_data_w },
	{ 0xd20000, 0xd20fff, atarigen_eeprom32_w, (data32_t **)&atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xd40000, 0xd4ffff, atarigen_eeprom_enable32_w },
	{ 0xd70000, 0xd71fff, MWA32_RAM },
	{ 0xd72000, 0xd75fff, atarigen_playfield32_w, &atarigen_playfield32 },
	{ 0xd76000, 0xd76fff, atarigen_alpha32_w, &atarigen_alpha32 },
	{ 0xd77000, 0xd77fff, MWA32_RAM },
	{ 0xd78000, 0xd78fff, atarirle_0_spriteram32_w, &atarirle_0_spriteram32 },
	{ 0xd79000, 0xd7ffff, MWA32_RAM },
	{ 0xd80000, 0xdfffff, colorram_protection_w, (data32_t **)&atarigt_colorram },
	{ 0xe04000, 0xe04003, led_w },
	{ 0xe08000, 0xe08003, latch_w },
	{ 0xe0a000, 0xe0a003, atarigen_scanline_int_ack32_w },
	{ 0xe0c000, 0xe0c003, atarigen_video_int_ack32_w },
	{ 0xe0e000, 0xe0e003, MWA32_NOP },//watchdog_reset_w },
	{ 0xf80000, 0xffffff, MWA32_RAM },
MEMORY_END


#if 0
static struct MemoryReadAddress readmem_cage[] =
{
	{ 0x000000, 0x680000, MRA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress writemem_cage[] =
{
	{ 0x000000, 0x680000, MWA_ROM },
	{ -1 }  /* end of table */
};
#endif



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( tmek )
	PORT_START		/* 68.SW (A1=0) */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )

	PORT_START		/* 68.SW (A1=1) */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )

	PORT_START      /* 68.STATUS (A2=0) */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )	/* /A2DRDY */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_TILT )		/* TILT */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )	/* /XIRQ23 */
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_UNUSED )	/* A2D.EOC */
	PORT_BIT( 0x0030, IP_ACTIVE_LOW, IPT_UNUSED )	/* NC */
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )			/* SELFTEST */
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_VBLANK )	/* VBLANK */
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* 68.STATUS (A2=1) */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )	/* /VBIRQ */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )	/* /4MSIRQ */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )	/* /XIRQ0 */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )	/* /XIRQ1 */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )	/* /SERVICER */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )	/* /SER.L */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN2 )	/* COINR */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )	/* COINL */
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( primrage )
	PORT_START		/* 68.SW (A1=0) */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 )

	PORT_START		/* 68.SW (A1=1) */
// bit 0x0008 does something
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )

	PORT_START      /* 68.STATUS (A2=0) */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )	/* /A2DRDY */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_TILT )		/* TILT */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )	/* /XIRQ23 */
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_UNUSED )	/* A2D.EOC */
	PORT_BIT( 0x0030, IP_ACTIVE_LOW, IPT_UNUSED )	/* NC */
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )			/* SELFTEST */
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_VBLANK )	/* VBLANK */
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* 68.STATUS (A2=1) */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )	/* /VBIRQ */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )	/* /4MSIRQ */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )	/* /XIRQ0 */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )	/* /XIRQ1 */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )	/* /SERVICER */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )	/* /SER.L */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_COIN2 )	/* COINR */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN1 )	/* COINL */
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
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

static MACHINE_DRIVER_START( atarigt )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68EC020, ATARI_CLOCK_50MHz/2)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(atarigen_video_int_gen,1)
	MDRV_CPU_PERIODIC_INT(atarigen_scanline_int_gen,250)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(atarigt)
	MDRV_NVRAM_HANDLER(atarigen)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(42*8, 30*8)
	MDRV_VISIBLE_AREA(0*8, 42*8-1, 0*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(atarigt)
	MDRV_VIDEO_EOF(atarirle)
	MDRV_VIDEO_UPDATE(atarigt)

	/* sound hardware */
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definition(s)
 *
 *************************************/

ROM_START( tmek )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD32_BYTE( "0044d", 0x00000, 0x20000, CRC(1cd62725) )
	ROM_LOAD32_BYTE( "0043d", 0x00001, 0x20000, CRC(82185051) )
	ROM_LOAD32_BYTE( "0042d", 0x00002, 0x20000, CRC(ef9feda4) )
	ROM_LOAD32_BYTE( "0041d", 0x00003, 0x20000, CRC(179da056) )

	ROM_REGION( 0x680000, REGION_CPU2, 0 )	/* 6.5MB for TMS320C31 code */
	ROM_LOAD( "0078c", 0x000000, 0x080000, CRC(ff5b979a) )
	ROM_LOAD( "0074",  0x080000, 0x200000, CRC(8dfc6ce0) )
	ROM_LOAD( "0076",  0x280000, 0x200000, CRC(74dffe2d) )
	ROM_LOAD( "0077",  0x480000, 0x200000, CRC(8f650f8b) )

	ROM_REGION( 0x300000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "0250",  0x000000, 0x80000, CRC(56bd9f25) ) /* playfield, planes 0-1 */
	ROM_LOAD( "0253a", 0x080000, 0x80000, CRC(23e2f83d) )
	ROM_LOAD( "0251",  0x100000, 0x80000, CRC(0d3b08f7) ) /* playfield, planes 2-3 */
	ROM_LOAD( "0254a", 0x180000, 0x80000, CRC(448aea87) )
	ROM_LOAD( "0252",  0x200000, 0x80000, CRC(95a1c23b) ) /* playfield, planes 4-5 */
	ROM_LOAD( "0255a", 0x280000, 0x80000, CRC(f0fbb700) )

	ROM_REGION( 0x020000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "0045a", 0x000000, 0x20000, CRC(057a5304) ) /* alphanumerics */

	ROM_REGION16_BE( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "0300", 0x000001, 0x100000, CRC(8367ddac) )
	ROM_LOAD16_BYTE( "0301", 0x000000, 0x100000, CRC(94524b5b) )
	ROM_LOAD16_BYTE( "0302", 0x200001, 0x100000, CRC(c03f1aa7) )
	ROM_LOAD16_BYTE( "0303", 0x200000, 0x100000, CRC(3ac5b24f) )
	ROM_LOAD16_BYTE( "0304", 0x400001, 0x100000, CRC(b053ef78) )
	ROM_LOAD16_BYTE( "0305", 0x400000, 0x100000, CRC(b012b8e9) )
	ROM_LOAD16_BYTE( "0306", 0x600001, 0x100000, CRC(d086f149) )
	ROM_LOAD16_BYTE( "0307", 0x600000, 0x100000, CRC(49c1a541) )
	ROM_LOAD16_BYTE( "0308", 0x800001, 0x100000, CRC(97033c8a) )
	ROM_LOAD16_BYTE( "0309", 0x800000, 0x100000, CRC(e095ecb3) )
	ROM_LOAD16_BYTE( "0310", 0xa00001, 0x100000, CRC(e056a0c3) )
	ROM_LOAD16_BYTE( "0311", 0xa00000, 0x100000, CRC(05afb2dc) )
	ROM_LOAD16_BYTE( "0312", 0xc00001, 0x100000, CRC(cc224dae) )
	ROM_LOAD16_BYTE( "0313", 0xc00000, 0x100000, CRC(a8cf049d) )
	ROM_LOAD16_BYTE( "0314", 0xe00001, 0x100000, CRC(4f01db8d) )
	ROM_LOAD16_BYTE( "0315", 0xe00000, 0x100000, CRC(28e97d06) )

	ROM_REGION( 0x0600, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "0001a",  0x0000, 0x0200, CRC(a70ade3f) SHA1(f4a558b17767eed2683c768d1b441e75edcff967) )	/* microcode for growth renderer */
	ROM_LOAD( "0001b",  0x0200, 0x0200, CRC(f4768b4d) SHA1(a506fa5386ab0ea2851ff1f8474d4bfc66deaa70) )
	ROM_LOAD( "0001c",  0x0400, 0x0200, CRC(22a76ad4) SHA1(ce840c283bbd3a5f19dc8d91b19d1571eff51ff4) )
ROM_END


ROM_START( tmekprot )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD32_BYTE( "pgm0", 0x00000, 0x20000, CRC(f5f7f7be) )
	ROM_LOAD32_BYTE( "pgm1", 0x00001, 0x20000, CRC(284f7971) )
	ROM_LOAD32_BYTE( "pgm2", 0x00002, 0x20000, CRC(ce9a77d4) )
	ROM_LOAD32_BYTE( "pgm3", 0x00003, 0x20000, CRC(28b0e210) )

	ROM_REGION( 0x680000, REGION_CPU2, 0 )	/* 6.5MB for TMS320C31 code */
	ROM_LOAD( "0078c", 0x000000, 0x080000, CRC(ff5b979a) )
	ROM_LOAD( "0074",  0x080000, 0x200000, CRC(8dfc6ce0) )
	ROM_LOAD( "0076",  0x280000, 0x200000, CRC(74dffe2d) )
	ROM_LOAD( "0077",  0x480000, 0x200000, CRC(8f650f8b) )

	ROM_REGION( 0x300000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "0250",  0x000000, 0x80000, CRC(56bd9f25) ) /* playfield, planes 0-1 */
	ROM_LOAD( "0253a", 0x080000, 0x80000, CRC(23e2f83d) )
	ROM_LOAD( "0251",  0x100000, 0x80000, CRC(0d3b08f7) ) /* playfield, planes 2-3 */
	ROM_LOAD( "0254a", 0x180000, 0x80000, CRC(448aea87) )
	ROM_LOAD( "0252",  0x200000, 0x80000, CRC(95a1c23b) ) /* playfield, planes 4-5 */
	ROM_LOAD( "0255a", 0x280000, 0x80000, CRC(f0fbb700) )

	ROM_REGION( 0x020000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "alpha", 0x000000, 0x20000, CRC(8f57a604) ) /* alphanumerics */

	ROM_REGION16_BE( 0x1000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "0300", 0x000001, 0x100000, CRC(8367ddac) )
	ROM_LOAD16_BYTE( "0301", 0x000000, 0x100000, CRC(94524b5b) )
	ROM_LOAD16_BYTE( "0302", 0x200001, 0x100000, CRC(c03f1aa7) )
	ROM_LOAD16_BYTE( "0303", 0x200000, 0x100000, CRC(3ac5b24f) )
	ROM_LOAD16_BYTE( "0304", 0x400001, 0x100000, CRC(b053ef78) )
	ROM_LOAD16_BYTE( "0305", 0x400000, 0x100000, CRC(b012b8e9) )
	ROM_LOAD16_BYTE( "0306", 0x600001, 0x100000, CRC(d086f149) )
	ROM_LOAD16_BYTE( "0307", 0x600000, 0x100000, CRC(49c1a541) )
	ROM_LOAD16_BYTE( "0308", 0x800001, 0x100000, CRC(97033c8a) )
	ROM_LOAD16_BYTE( "0309", 0x800000, 0x100000, CRC(e095ecb3) )
	ROM_LOAD16_BYTE( "0310", 0xa00001, 0x100000, CRC(e056a0c3) )
	ROM_LOAD16_BYTE( "0311", 0xa00000, 0x100000, CRC(05afb2dc) )
	ROM_LOAD16_BYTE( "0312", 0xc00001, 0x100000, CRC(cc224dae) )
	ROM_LOAD16_BYTE( "0313", 0xc00000, 0x100000, CRC(a8cf049d) )
	ROM_LOAD16_BYTE( "0314", 0xe00001, 0x100000, CRC(4f01db8d) )
	ROM_LOAD16_BYTE( "0315", 0xe00000, 0x100000, CRC(28e97d06) )

	ROM_REGION( 0x0600, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "0001a",  0x0000, 0x0200, CRC(a70ade3f) SHA1(f4a558b17767eed2683c768d1b441e75edcff967) )	/* microcode for growth renderer */
	ROM_LOAD( "0001b",  0x0200, 0x0200, CRC(f4768b4d) SHA1(a506fa5386ab0ea2851ff1f8474d4bfc66deaa70) )
	ROM_LOAD( "0001c",  0x0400, 0x0200, CRC(22a76ad4) SHA1(ce840c283bbd3a5f19dc8d91b19d1571eff51ff4) )
ROM_END


ROM_START( primrage )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD32_BYTE( "1044b", 0x000000, 0x80000, CRC(35c9c34b) SHA1(4bd1d35cc7c68574819afd648405eedb8db25b4c) )
	ROM_LOAD32_BYTE( "1043b", 0x000001, 0x80000, CRC(86322829) SHA1(e0e72888def0931d078921f099bae6788738a291) )
	ROM_LOAD32_BYTE( "1042b", 0x000002, 0x80000, CRC(750e8095) SHA1(4660637136b1a25169d8c43646c8b87081763987) )
	ROM_LOAD32_BYTE( "1041b", 0x000003, 0x80000, CRC(6a90d283) SHA1(7c18c97cb5e5cdd26a52cd6bc099fbce87055311) )

	ROM_REGION( 0x680000, REGION_CPU2, 0 )	/* 6.5MB for TMS320C31 code */
	ROM_LOAD( "1078a", 0x000000, 0x080000, CRC(0656435f) SHA1(f8e498171e754eb8703dad6b2351509bbb27e06b) )
	ROM_LOAD( "0075",  0x080000, 0x200000, CRC(b685a88e) SHA1(998b8fe54971f6cd96e4c22b19e3831f29d8172d) )
	ROM_LOAD( "0077",  0x480000, 0x200000, CRC(3283cea8) SHA1(fb7333ca951053a56c501f2ce0eb197c8fcafaf7) )

	ROM_REGION( 0x300000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "0050a",  0x000000, 0x80000, CRC(66896e8f) SHA1(7675b24c15ca0608f11f2a7b8d70717adb10924c) ) /* playfield, planes 0-1 */
	ROM_LOAD( "0051a",  0x100000, 0x80000, CRC(fb5b3e7b) SHA1(f43fe4b5c4bbea10da46b60c644f586fb391355d) ) /* playfield, planes 2-3 */
	ROM_LOAD( "0052a",  0x200000, 0x80000, CRC(cbe38670) SHA1(0780e599007851f6d37cdd8c701d01cb1ae48b9d) ) /* playfield, planes 4-5 */

	ROM_REGION( 0x020000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "1045b", 0x000000, 0x20000, CRC(1d3260bf) SHA1(85d9db8499cbe180c8d52710f3cfe64453a530ff) ) /* alphanumerics */

	ROM_REGION16_BE( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "1100a", 0x0000001, 0x080000, CRC(6e9c80b5) SHA1(ec724011527dd8707c733211b1a6c51b22f580c7) )
	ROM_LOAD16_BYTE( "1101a", 0x0000000, 0x080000, CRC(bb7ee624) SHA1(0de6385aee7d25b41fd5bf232e44e5da536504ac) )
	ROM_LOAD16_BYTE( "0332",  0x0800001, 0x100000, CRC(610cfcb4) SHA1(bed1bd0d11c0a7cc48d020fc0acec34daf48c5ac) )
	ROM_LOAD16_BYTE( "0333",  0x0800000, 0x100000, CRC(3320448e) SHA1(aef42328bf72fca5c04bfed1ea41100bb5aafeaa) )
	ROM_LOAD16_BYTE( "0334",  0x0a00001, 0x100000, CRC(be3acb6f) SHA1(664cb4cd4d325577ab0cbe0cf48870a9f4706573) )
	ROM_LOAD16_BYTE( "0335",  0x0a00000, 0x100000, CRC(e4f6e87a) SHA1(2a3f8ff46b289c25cd4ca2a1369b14613f48e964) )
	ROM_LOAD16_BYTE( "0336",  0x0c00001, 0x100000, CRC(a78a8525) SHA1(69c3da4d45b0f09f5bdabcedd238b82efab48a70) )
	ROM_LOAD16_BYTE( "0337",  0x0c00000, 0x100000, CRC(73fdd050) SHA1(63c67187953d2dab93a260e548ef5965e7cba4e8) )
	ROM_LOAD16_BYTE( "0338",  0x0e00001, 0x100000, CRC(fa19cae6) SHA1(7d0560516971f32835329a17450c7561631a27d1) )
	ROM_LOAD16_BYTE( "0339",  0x0e00000, 0x100000, CRC(e0cd1393) SHA1(0de59d04165d64320512936c194db19cca6455fd) )
	ROM_LOAD16_BYTE( "0316",  0x1000001, 0x100000, CRC(9301c672) SHA1(a8971049c857ae283a95b257dd0d6aaff6d787cd) )
	ROM_LOAD16_BYTE( "0317",  0x1000000, 0x100000, CRC(9e3b831a) SHA1(b799e57bea9522cb83f9aa7ea38a17b1d8273b8d) )
	ROM_LOAD16_BYTE( "0318",  0x1200001, 0x100000, CRC(8523db5d) SHA1(f2476aa26b1a7cbe7510994d92eb209fda65593d) )
	ROM_LOAD16_BYTE( "0319",  0x1200000, 0x100000, CRC(42f22e4b) SHA1(2a1a6f0a7aca7b7b64bce0bd54eb4cb23a2336b1) )
	ROM_LOAD16_BYTE( "0320",  0x1400001, 0x100000, CRC(21369d13) SHA1(28e03595c098fd9bec6f7316180d17905a51a51b) )
	ROM_LOAD16_BYTE( "0321",  0x1400000, 0x100000, CRC(3b7d498a) SHA1(804e9e1567bf97e8dae3b9444428254ced8b60da) )
	ROM_LOAD16_BYTE( "0322",  0x1600001, 0x100000, CRC(05e9f407) SHA1(fa25a893d4cb805df02d7d12df4dbabefb3114a2) )
	ROM_LOAD16_BYTE( "0323",  0x1600000, 0x100000, CRC(603f3bb6) SHA1(d7c22dc900d9edc36d8f211df67a206d14637fab) )
	ROM_LOAD16_BYTE( "0324",  0x1800001, 0x100000, CRC(3c37769f) SHA1(ca0306a439949d2a0305cc0cf05808a58e84084c) )
	ROM_LOAD16_BYTE( "0325",  0x1800000, 0x100000, CRC(f43321e3) SHA1(8bb4dd4a5d5400b17052d50dca9078211dc6b861) )
	ROM_LOAD16_BYTE( "0326",  0x1a00001, 0x100000, CRC(63d4ccea) SHA1(340fced6998a8ae6fd285d8fe666f5f1e4b6bfaf) )
	ROM_LOAD16_BYTE( "0327",  0x1a00000, 0x100000, CRC(9f4806d5) SHA1(76e9f1a47e7fa45e834fa8739528f1e3c54b14dc) )
	ROM_LOAD16_BYTE( "0328",  0x1c00001, 0x100000, CRC(a08d73e1) SHA1(25a58777f15e9550111447b47a98762fd6bb498d) )
	ROM_LOAD16_BYTE( "0329",  0x1c00000, 0x100000, CRC(eff3d2cd) SHA1(8532568b5fd91d2b738947e9cd575a4eb2a03be2) )
	ROM_LOAD16_BYTE( "0330",  0x1e00001, 0x100000, CRC(7bf6bb8f) SHA1(f34bd8a9c7f95436a1c816badc59673cd2a6969a) )
	ROM_LOAD16_BYTE( "0331",  0x1e00000, 0x100000, CRC(c6a64dad) SHA1(ee54514463ab61cbaef70da064cf5de591e5861f) )

	ROM_REGION( 0x0600, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "0001a",  0x0000, 0x0200, CRC(a70ade3f) SHA1(f4a558b17767eed2683c768d1b441e75edcff967) )	/* microcode for growth renderer */
	ROM_LOAD( "0001b",  0x0200, 0x0200, CRC(f4768b4d) SHA1(a506fa5386ab0ea2851ff1f8474d4bfc66deaa70) )
	ROM_LOAD( "0001c",  0x0400, 0x0200, CRC(22a76ad4) SHA1(ce840c283bbd3a5f19dc8d91b19d1571eff51ff4) )
ROM_END


ROM_START( primrag2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )	/* 8*64k for 68000 code */
	ROM_LOAD32_BYTE( "0044b", 0x000000, 0x80000, CRC(26139575) SHA1(22e59ab621d58e56969b64701fc59aec085193dd) )
	ROM_LOAD32_BYTE( "0043b", 0x000001, 0x80000, CRC(928d2447) SHA1(9bbbdbf056a7b986d985d79be889b9876a710631) )
	ROM_LOAD32_BYTE( "0042b", 0x000002, 0x80000, CRC(cd6062b9) SHA1(2973fb561ab68cd48ec132b6720c04d10bedfd19) )
	ROM_LOAD32_BYTE( "0041b", 0x000003, 0x80000, CRC(3008f6f0) SHA1(45aac457b4584ee3bd3561e3b2e34e49aa61fbc5) )

	ROM_REGION( 0x680000, REGION_CPU2, 0 )	/* 6.5MB for TMS320C31 code */
	ROM_LOAD( "0078a", 0x000000, 0x080000, CRC(91df8d8f) SHA1(6d361f88de604b8f11dd9bfe85ff18bcd322862d) )
	ROM_LOAD( "0075",  0x080000, 0x200000, CRC(b685a88e) SHA1(998b8fe54971f6cd96e4c22b19e3831f29d8172d) )
	ROM_LOAD( "0077",  0x480000, 0x200000, CRC(3283cea8) SHA1(fb7333ca951053a56c501f2ce0eb197c8fcafaf7) )

	ROM_REGION( 0x300000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "0050a",  0x000000, 0x80000, CRC(66896e8f) SHA1(7675b24c15ca0608f11f2a7b8d70717adb10924c) ) /* playfield, planes 0-1 */
	ROM_LOAD( "0051a",  0x100000, 0x80000, CRC(fb5b3e7b) SHA1(f43fe4b5c4bbea10da46b60c644f586fb391355d) ) /* playfield, planes 2-3 */
	ROM_LOAD( "0052a",  0x200000, 0x80000, CRC(cbe38670) SHA1(0780e599007851f6d37cdd8c701d01cb1ae48b9d) ) /* playfield, planes 4-5 */

	ROM_REGION( 0x020000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "0045a", 0x000000, 0x20000, CRC(c8b39b1c) SHA1(836c0ccf96b2beccacf6d8ac23981fc2d1f09803) ) /* alphanumerics */

	ROM_REGION16_BE( 0x2000000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "0100a", 0x0000001, 0x080000, CRC(5299fb2a) SHA1(791378215ab6ffff3ab2ae7192ce9f88dae4090d) )
	ROM_LOAD16_BYTE( "0101a", 0x0000000, 0x080000, CRC(3e234711) SHA1(6a9f19db2b4c8c34d3d7b4984206e3d5c4398d7f) )
	ROM_LOAD16_BYTE( "0332",  0x0800001, 0x100000, CRC(610cfcb4) SHA1(bed1bd0d11c0a7cc48d020fc0acec34daf48c5ac) )
	ROM_LOAD16_BYTE( "0333",  0x0800000, 0x100000, CRC(3320448e) SHA1(aef42328bf72fca5c04bfed1ea41100bb5aafeaa) )
	ROM_LOAD16_BYTE( "0334",  0x0a00001, 0x100000, CRC(be3acb6f) SHA1(664cb4cd4d325577ab0cbe0cf48870a9f4706573) )
	ROM_LOAD16_BYTE( "0335",  0x0a00000, 0x100000, CRC(e4f6e87a) SHA1(2a3f8ff46b289c25cd4ca2a1369b14613f48e964) )
	ROM_LOAD16_BYTE( "0336",  0x0c00001, 0x100000, CRC(a78a8525) SHA1(69c3da4d45b0f09f5bdabcedd238b82efab48a70) )
	ROM_LOAD16_BYTE( "0337",  0x0c00000, 0x100000, CRC(73fdd050) SHA1(63c67187953d2dab93a260e548ef5965e7cba4e8) )
	ROM_LOAD16_BYTE( "0338",  0x0e00001, 0x100000, CRC(fa19cae6) SHA1(7d0560516971f32835329a17450c7561631a27d1) )
	ROM_LOAD16_BYTE( "0339",  0x0e00000, 0x100000, CRC(e0cd1393) SHA1(0de59d04165d64320512936c194db19cca6455fd) )
	ROM_LOAD16_BYTE( "0316",  0x1000001, 0x100000, CRC(9301c672) SHA1(a8971049c857ae283a95b257dd0d6aaff6d787cd) )
	ROM_LOAD16_BYTE( "0317",  0x1000000, 0x100000, CRC(9e3b831a) SHA1(b799e57bea9522cb83f9aa7ea38a17b1d8273b8d) )
	ROM_LOAD16_BYTE( "0318",  0x1200001, 0x100000, CRC(8523db5d) SHA1(f2476aa26b1a7cbe7510994d92eb209fda65593d) )
	ROM_LOAD16_BYTE( "0319",  0x1200000, 0x100000, CRC(42f22e4b) SHA1(2a1a6f0a7aca7b7b64bce0bd54eb4cb23a2336b1) )
	ROM_LOAD16_BYTE( "0320",  0x1400001, 0x100000, CRC(21369d13) SHA1(28e03595c098fd9bec6f7316180d17905a51a51b) )
	ROM_LOAD16_BYTE( "0321",  0x1400000, 0x100000, CRC(3b7d498a) SHA1(804e9e1567bf97e8dae3b9444428254ced8b60da) )
	ROM_LOAD16_BYTE( "0322",  0x1600001, 0x100000, CRC(05e9f407) SHA1(fa25a893d4cb805df02d7d12df4dbabefb3114a2) )
	ROM_LOAD16_BYTE( "0323",  0x1600000, 0x100000, CRC(603f3bb6) SHA1(d7c22dc900d9edc36d8f211df67a206d14637fab) )
	ROM_LOAD16_BYTE( "0324",  0x1800001, 0x100000, CRC(3c37769f) SHA1(ca0306a439949d2a0305cc0cf05808a58e84084c) )
	ROM_LOAD16_BYTE( "0325",  0x1800000, 0x100000, CRC(f43321e3) SHA1(8bb4dd4a5d5400b17052d50dca9078211dc6b861) )
	ROM_LOAD16_BYTE( "0326",  0x1a00001, 0x100000, CRC(63d4ccea) SHA1(340fced6998a8ae6fd285d8fe666f5f1e4b6bfaf) )
	ROM_LOAD16_BYTE( "0327",  0x1a00000, 0x100000, CRC(9f4806d5) SHA1(76e9f1a47e7fa45e834fa8739528f1e3c54b14dc) )
	ROM_LOAD16_BYTE( "0328",  0x1c00001, 0x100000, CRC(a08d73e1) SHA1(25a58777f15e9550111447b47a98762fd6bb498d) )
	ROM_LOAD16_BYTE( "0329",  0x1c00000, 0x100000, CRC(eff3d2cd) SHA1(8532568b5fd91d2b738947e9cd575a4eb2a03be2) )
	ROM_LOAD16_BYTE( "0330",  0x1e00001, 0x100000, CRC(7bf6bb8f) SHA1(f34bd8a9c7f95436a1c816badc59673cd2a6969a) )
	ROM_LOAD16_BYTE( "0331",  0x1e00000, 0x100000, CRC(c6a64dad) SHA1(ee54514463ab61cbaef70da064cf5de591e5861f) )

	ROM_REGION( 0x0600, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "0001a",  0x0000, 0x0200, CRC(a70ade3f) SHA1(f4a558b17767eed2683c768d1b441e75edcff967) )	/* microcode for growth renderer */
	ROM_LOAD( "0001b",  0x0200, 0x0200, CRC(f4768b4d) SHA1(a506fa5386ab0ea2851ff1f8474d4bfc66deaa70) )
	ROM_LOAD( "0001c",  0x0400, 0x0200, CRC(22a76ad4) SHA1(ce840c283bbd3a5f19dc8d91b19d1571eff51ff4) )
ROM_END



/*************************************
 *
 *	Driver initialization
 *
 *************************************/

static DRIVER_INIT( tmek )
{
	atarigen_eeprom_default = NULL;
	atarigt_motion_object_mask = 0xff0;

	/* setup protection */
	protection_r = tmek_protection_r;
	protection_w = tmek_protection_w;
}


static DRIVER_INIT( primrage )
{
	atarigen_eeprom_default = NULL;
	atarigt_motion_object_mask = 0x7f0;

	/* install protection */
	protection_r = primrage_protection_r;
	protection_w = primrage_protection_w;

	/* intall speedups */
	speedup = install_mem_read32_handler(0, 0xffcde8, 0xffcdeb, primrage_speedup_r);
}



/*************************************
 *
 *	Game driver(s)
 *
 *************************************/

GAMEX( 1994, tmek,     0,        atarigt,  tmek,     tmek,     ROT0, "Atari Games", "T-MEK", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND )
GAMEX( 1994, tmekprot, tmek,     atarigt,  tmek,     tmek,     ROT0, "Atari Games", "T-MEK (prototype)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND )
GAMEX( 1994, primrage, 0,        atarigt,  primrage, primrage, ROT0, "Atari Games", "Primal Rage (version 2.3)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND )
GAMEX( 1994, primrag2, primrage, atarigt,  primrage, primrage, ROT0, "Atari Games", "Primal Rage (version 1.7)", GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND )
