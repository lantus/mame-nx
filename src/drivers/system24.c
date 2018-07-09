/*
 * Preliminary Sega System 24
 *  Olivier Galibert
 * Kudos to Charles MacDonald (http://cgfm2.emuviews.com) for his
 * very useful research
 *
 * Hotrod
 * Bonanza Bros
 * Quiz Ghost Hunter
 * Tokoro San no MahMahjan
 * Tokoro San no MahMahjan 2
 */

/*

there are still a lot of issues with this but by including it now
hopefully it can develop and also aid others looking at this system
saving them time etc.

*/


/* system24temp_ functions / variables are from shared rewrite files,
   once the rest of the rewrite is complete they can be removed, I
   just made a copy & renamed them for now to avoid any conflicts
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68k.h"
#include "system24.h"
#include "system16.h"
#include "vidhrdw/segaic24.h"
#include "sound/ym2151.h"

VIDEO_START(system24);
VIDEO_UPDATE(system24);

static int fdc_status, fdc_track, fdc_sector, fdc_data;
static int fdc_phys_track, fdc_irq, fdc_drq, fdc_span, fdc_index_count;
static unsigned char *fdc_pt;
static int track_size;

static void fdc_init(void)
{
	fdc_status = 0;
	fdc_track = 0;
	fdc_sector = 0;
	fdc_data = 0;
	fdc_phys_track = 0;
	fdc_irq = 0;
	fdc_drq = 0;
	fdc_index_count = 0;
}

static READ16_HANDLER( fdc_r )
{
	switch(offset) {
	case 0:
		fdc_irq = 0;
		return fdc_status;
	case 1:
		return fdc_track;
	case 2:
		return fdc_sector;
	case 3:
	default: {
		int res = fdc_data;
		if(fdc_drq) {
			fdc_span--;
			//			logerror("Read %02x (%d)\n", res, fdc_span);
			if(fdc_span) {
				fdc_pt++;
				fdc_data = *fdc_pt;
			} else {
				logerror("FDC: transfert complete\n");
				fdc_drq = 0;
				fdc_status = 0;
				fdc_irq = 1;
			}
		} else
			logerror("FDC: data read with drq down\n");
		return res;
	}
	}
}

static WRITE16_HANDLER( fdc_w )
{
	if(ACCESSING_LSB) {
		data &= 0xff;
		switch(offset) {
		case 0:
			fdc_irq = 0;
			switch(data >> 4) {
			case 0x0:
				logerror("FDC: Restore\n");
				fdc_phys_track = fdc_track = 0;
				fdc_irq = 1;
				fdc_status = 4;
				break;
			case 0x1:
				logerror("FDC: Seek %d\n", fdc_data);
				fdc_phys_track = fdc_track = fdc_data;
				fdc_irq = 1;
				fdc_status = fdc_track ? 0 : 4;
				break;
			case 0x9:
				logerror("Read multiple [%02x] %d..%d side %d track %d\n", data, fdc_sector, fdc_sector+fdc_data-1, data & 8 ? 1 : 0, fdc_phys_track);
				fdc_pt = memory_region(REGION_USER2) + track_size*(2*fdc_phys_track+(data & 8 ? 1 : 0));
				fdc_span = track_size;
				fdc_status = 3;
				fdc_drq = 1;
				fdc_data = *fdc_pt;
				break;
			case 0xb:
				logerror("Write multiple [%02x] %d..%d side %d track %d\n", data, fdc_sector, fdc_sector+fdc_data-1, data & 8 ? 1 : 0, fdc_phys_track);
				fdc_pt = memory_region(REGION_USER2) + track_size*(2*fdc_phys_track+(data & 8 ? 1 : 0));
				fdc_span = track_size;
				fdc_status = 3;
				fdc_drq = 1;
				break;
			case 0xd:
				logerror("FDC: Forced interrupt\n");
				fdc_span = 0;
				fdc_drq = 0;
				fdc_irq = data & 1;
				fdc_status = 0;
				break;
			case 0xf:
				if(data == 0xfe)
					logerror("FDC: Assign mode %02x\n", fdc_data);
				else if(data == 0xfd)
					logerror("FDC: Assign parameter %02x\n", fdc_data);
				else
					logerror("FDC: Unknown command %02x\n", data);
				break;
			default:
				logerror("FDC: Unknown command %02x\n", data);
				break;
			}
			break;
		case 1:
			logerror("FDC: Track register %02x\n", data);
			fdc_track = data;
			break;
		case 2:
			logerror("FDC: Sector register %02x\n", data);
			fdc_sector = data;
			break;
		case 3:
			if(fdc_drq) {
				//				logerror("Write %02x (%d)\n", data, fdc_span);
				*fdc_pt++ = data;
				fdc_span--;
				if(!fdc_span) {
					logerror("FDC: transfert complete\n");
					fdc_drq = 0;
					fdc_status = 0;
					fdc_irq = 1;
				}
			} else
				logerror("FDC: Data register %02x\n", data);
			fdc_data = data;
			break;
		}
	}
}

static READ16_HANDLER( fdc_status_r )
{
	return 0x90 | (fdc_irq ? 2 : 0) | (fdc_drq ? 1 : 0) | (fdc_phys_track ? 0x40 : 0) | (fdc_index_count ? 0x20 : 0);
}

static WRITE16_HANDLER( fdc_ctrl_w )
{
	if(ACCESSING_LSB)
		logerror("FDC control %02x\n", data & 0xff);
}

static UINT8 hotrod_io_r(int port)
{
	switch(port) {
	case 0:
		return readinputport(0);
	case 1:
		return readinputport(1);
	case 2:
		return 0xff;
	case 3:
		return 0xff;
	case 4:
		return readinputport(2);
	case 5: // Dip switches
		return readinputport(3);
	case 6:
		return readinputport(4);
	case 7: // DAC
		return 0xff;
	}
	return 0x00;
}

static UINT8 qgh_io_r(int port)
{
	switch(port) {
	case 0:
		return readinputport(0);
	case 1:
		return readinputport(1);
	case 2:
		return 0xff;
	case 3:
		return 0xff;
	case 4:
		return readinputport(2);
	case 5: // Dip switches
		return readinputport(3);
	case 6:
		return readinputport(4);
	case 7: // DAC
		return 0xff;
	}
	return 0x00;
}

static int cur_input_line;

static UINT8 mahmajn_io_r(int port)
{
	switch(port) {
	case 0:
		return ~(1 << cur_input_line);
	case 1:
		return 0xff;
	case 2:
		return readinputport(cur_input_line);
	case 3:
		return 0xff;
	case 4:
		return readinputport(8);
	case 5: // Dip switches
		return readinputport(9);
	case 6:
		return readinputport(10);
	case 7: // DAC
		return 0xff;
	}
	return 0x00;
}

static void mahmajn_io_w(int port, UINT8 data)
{
	switch(port) {
	case 3:
		if(data & 4)
			cur_input_line = (cur_input_line + 1) & 7;
		break;
	case 7: // DAC
		DAC_0_signed_data_w(0, data);
		break;
	default:
		fprintf(stderr, "Port %d : %02x\n", port, data & 0xff);
	}
}

static void hotrod_io_w(int port, UINT8 data)
{
	switch(port) {
	case 3: // Lamps
		break;
	case 7: // DAC
		DAC_0_signed_data_w(0, data);
		break;
	default:
		fprintf(stderr, "Port %d : %02x\n", port, data & 0xff);
	}
}

static READ16_HANDLER( iod_r )
{
	logerror("IO daughterboard read %02x (%x)\n", offset, activecpu_get_pc());
	return 0xffff;
}

static WRITE16_HANDLER( iod_w )
{
	logerror("IO daughterboard write %02x, %04x & %04x (%x)\n", offset, data, mem_mask, activecpu_get_pc());
}

static unsigned char resetcontrol, prev_resetcontrol;

static void reset_reset(void)
{
	int changed = resetcontrol ^ prev_resetcontrol;
	if(changed & 2) {
		if(resetcontrol & 2) {
			cpu_set_halt_line(1, CLEAR_LINE);
			cpu_set_reset_line(1, PULSE_LINE);
		} else
			cpu_set_halt_line(1, ASSERT_LINE);
	}
	if(changed & 4)
		YM2151ResetChip(0);
	prev_resetcontrol = resetcontrol;
}

static void resetcontrol_w(UINT8 data)
{
	resetcontrol = data;
	logerror("Reset control %02x (%x:%x)\n", resetcontrol, cpu_getactivecpu(), activecpu_get_pc());
	reset_reset();
}

static unsigned char curbank;

static void reset_bank(void)
{
	cpu_setbank(1, memory_region(REGION_USER1) + curbank * 0x40000);
}

static READ16_HANDLER( curbank_r )
{
	return curbank;
}

static WRITE16_HANDLER( curbank_w )
{
	if(ACCESSING_LSB) {
		curbank = data & 0xff;
		reset_bank();
	}
}

static READ16_HANDLER( ym_status_r )
{
	return YM2151_status_port_0_r(0);
}

static WRITE16_HANDLER( ym_register_w )
{
	if(ACCESSING_LSB)
		YM2151_register_port_0_w(0, data);
}

static WRITE16_HANDLER( ym_data_w )
{
	if(ACCESSING_LSB)
		YM2151_data_port_0_w(0, data);
}

// Protection magic latch

static UINT8  mahmajn_mlt[8] = { 5, 1, 6, 2, 3, 7, 4, 0 };
static UINT8 mahmajn2_mlt[8] = { 6, 0, 5, 3, 1, 4, 2, 7 };
static UINT8      gqh_mlt[8] = { 3, 7, 4, 0, 2, 6, 5, 1 };
static UINT8 bnzabros_mlt[8] = { 2, 4, 0, 5, 7, 3, 1, 6 };
static UINT8       qu_mlt[8] = { 1, 6, 4, 7, 0, 5, 3, 2 };
static UINT8 quizmeku_mlt[8] = { 0, 3, 2, 4, 6, 1, 7, 5 };

static UINT8 mlatch;
static const unsigned char *mlatch_table;

static READ16_HANDLER( mlatch_r )
{
	return mlatch;
}

static WRITE16_HANDLER( mlatch_w )
{
	if(ACCESSING_LSB) {
		int i;
		unsigned char mxor = 0;
		if(!mlatch_table) {
			logerror("Protection: magic latch accessed but no table loaded (%x)\n", activecpu_get_pc());
			return;
		}

		data &= 0xff;

		if(data != 0xff) {
			for(i=0; i<8; i++)
				if(mlatch & (1<<i))
					mxor |= 1 << mlatch_table[i];
			mlatch = data ^ mxor;
			logerror("Magic latching %02x ^ %02x as %02x (%x)\n", data & 0xff, mxor, mlatch, activecpu_get_pc());
		} else {
			logerror("Magic latch reset (%x)\n", activecpu_get_pc());
			mlatch = 0x00;
		}
	}
}

enum {
	IRQ_YM2151 = 1,
	IRQ_TIMER  = 2,
	IRQ_VBLANK = 3,
	IRQ_SPRITE = 4
};

static UINT16 irq_timera;
static UINT8  irq_timerb;
static UINT8  irq_allow0, irq_allow1;
static int    irq_timer_pend0, irq_timer_pend1, irq_yms;
static void  *irq_timer;

static void irq_timer_cb(int param)
{
	irq_timer_pend0 = irq_timer_pend1 = 1;
	if(irq_allow0 & (1 << IRQ_TIMER))
		cpu_set_irq_line(0, IRQ_TIMER+1, ASSERT_LINE);
	if(irq_allow1 & (1 << IRQ_TIMER))
		cpu_set_irq_line(1, IRQ_TIMER+1, ASSERT_LINE);
}

static void irq_init(void)
{
	irq_timera = 0;
	irq_timerb = 0;
	irq_allow0 = 0;
	irq_allow1 = 0;
	irq_timer_pend0 = 0;
	irq_timer_pend1 = 0;
	irq_timer = timer_alloc(irq_timer_cb);
}

static void irq_timer_reset(void)
{
	int freq = (irq_timerb << 12) | irq_timera;
	freq &= 0x1fff;

	timer_adjust(irq_timer, TIME_IN_HZ(freq), 0, TIME_IN_HZ(freq));
	logerror("New timer frequency: %0d [%02x %04x]\n", freq, irq_timerb, irq_timera);
}

static WRITE16_HANDLER(irq_w)
{
	switch(offset) {
	case 0: {
		UINT16 old_ta = irq_timera;
		COMBINE_DATA(&irq_timera);
		if(old_ta != irq_timera)
			irq_timer_reset();
		break;
	}
	case 1:
		if(ACCESSING_LSB) {
			UINT8 old_tb = irq_timerb;
			irq_timerb = data;
			if(old_tb != irq_timerb)
				irq_timer_reset();
		}
		break;
	case 2:
		irq_allow0 = data;
		cpu_set_irq_line(0, IRQ_TIMER+1, irq_timer_pend0 && (irq_allow0 & (1 << IRQ_TIMER)) ? ASSERT_LINE : CLEAR_LINE);
		cpu_set_irq_line(0, IRQ_YM2151+1, irq_yms && (irq_allow0 & (1 << IRQ_YM2151)) ? ASSERT_LINE : CLEAR_LINE);
		break;
	case 3:
		irq_allow1 = data;
		cpu_set_irq_line(1, IRQ_TIMER+1, irq_timer_pend1 && (irq_allow1 & (1 << IRQ_TIMER)) ? ASSERT_LINE : CLEAR_LINE);
		cpu_set_irq_line(1, IRQ_YM2151+1, irq_yms && (irq_allow1 & (1 << IRQ_YM2151)) ? ASSERT_LINE : CLEAR_LINE);
		break;
	}
}

static READ16_HANDLER(irq_r)
{
	switch(offset) {
	case 2:
		irq_timer_pend0 = 0;
		cpu_set_irq_line(0, IRQ_TIMER+1, CLEAR_LINE);
		break;
	case 3:
		irq_timer_pend1 = 0;
		cpu_set_irq_line(1, IRQ_TIMER+1, CLEAR_LINE);
		break;
	}
	return 0xffff;
}

static INTERRUPT_GEN(irq_vbl)
{
	int irq = cpu_getiloops() ? IRQ_SPRITE : IRQ_VBLANK;
	int mask = 1 << irq;

	if(irq_allow0 & mask)
		cpu_set_irq_line(0, 1+irq, HOLD_LINE);

	if(irq_allow1 & mask)
		cpu_set_irq_line(1, 1+irq, HOLD_LINE);

	if(!cpu_getiloops()) {
		// Ensure one index pulse every 20 frames
		// The is some code in bnzabros at 0x852 that makes it crash
		// if the pulse train is too fast
		fdc_index_count++;
		if(fdc_index_count >= 20)
			fdc_index_count = 0;
	}
}

static void irq_ym(int irq)
{
	irq_yms = irq;
	cpu_set_irq_line(0, IRQ_YM2151+1, irq_yms && (irq_allow0 & (1 << IRQ_YM2151)) ? ASSERT_LINE : CLEAR_LINE);
	cpu_set_irq_line(1, IRQ_YM2151+1, irq_yms && (irq_allow1 & (1 << IRQ_YM2151)) ? ASSERT_LINE : CLEAR_LINE);
}

static READ16_HANDLER(ctrl_r)
{
	return 0xff;
}

static MEMORY_READ16_START( system24_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x080000, 0x0fffff, MRA16_BANK2 },
{ 0x1003a0, 0x1003a3, MRA16_NOP },
	{ 0x200000, 0x20ffff, sys24_tile_r },
	{ 0x280000, 0x29ffff, sys24_char_r },
	{ 0x400000, 0x403fff, MRA16_RAM },
	{ 0x404000, 0x40401f, sys24_mixer_r },
	{ 0x600000, 0x63ffff, sys24_sprite_r },
	{ 0x800000, 0x80007f, system24temp_sys16_io_r },
	{ 0x800102, 0x800103, ym_status_r },
	{ 0xa00000, 0xa00007, irq_r },
	{ 0xb00000, 0xb00007, fdc_r },
	{ 0xb00008, 0xb0000f, fdc_status_r },
{ 0xc00000, 0xc00011, ctrl_r },
	{ 0xc80000, 0xcbffff, MRA16_BANK1 },
	{ 0xcc0000, 0xcc0001, curbank_r },
	{ 0xcc0006, 0xcc0007, mlatch_r },
	{ 0xf00000, 0xf3ffff, system24temp_sys16_shared_ram_r },
	{ 0xfc0000, 0xffffff, MRA16_BANK3 },
MEMORY_END

static MEMORY_WRITE16_START( system24_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },
	{ 0x080000, 0x0fffff, MWA16_BANK2 },
	{ 0x200000, 0x20ffff, sys24_tile_w },
	{ 0x220000, 0x220001, MWA16_NOP }, // Unknown, always 0
	{ 0x240000, 0x240001, MWA16_NOP }, // Horizontal synchronization register
	{ 0x260000, 0x260001, MWA16_NOP }, // Vertical synchronization register
	{ 0x270000, 0x270001, MWA16_NOP }, // Video synchronization switch
	{ 0x280000, 0x29ffff, sys24_char_w },
	{ 0x400000, 0x403fff, system24temp_sys16_paletteram1_w, &paletteram16 },
	{ 0x404000, 0x40401f, sys24_mixer_w },
	{ 0x600000, 0x63ffff, sys24_sprite_w },
	{ 0x800000, 0x80007f, system24temp_sys16_io_w },
	{ 0x800100, 0x800101, ym_register_w },
	{ 0x800102, 0x800103, ym_data_w },
	{ 0xa00000, 0xa00007, irq_w },
	{ 0xb00000, 0xb00007, fdc_w },
	{ 0xb00008, 0xb0000f, fdc_ctrl_w },
{ 0xc00000, 0xc00011, MWA16_NOP },
	{ 0xcc0000, 0xcc0001, curbank_w },
	{ 0xcc0006, 0xcc0007, mlatch_w },
{ 0xd00300, 0xd00301, MWA16_NOP },
	{ 0xf00000, 0xf3ffff, system24temp_sys16_shared_ram_w },
	{ 0xfc0000, 0xffffff, MWA16_BANK3 },
MEMORY_END

static MEMORY_READ16_START( system24_readmem2 )
	{ 0x000000, 0x03ffff, MRA16_RAM },
	{ 0x080000, 0x0fffff, MRA16_BANK2 },
{ 0x1003a0, 0x1003a3, MRA16_NOP },
	{ 0x200000, 0x20ffff, sys24_tile_r },
	{ 0x280000, 0x29ffff, sys24_char_r },
	{ 0x400000, 0x403fff, paletteram16_word_r },
	{ 0x404000, 0x40401f, sys24_mixer_r },
	{ 0x600000, 0x63ffff, sys24_sprite_r },
	{ 0x800000, 0x80007f, system24temp_sys16_io_r },
	{ 0x800102, 0x800103, ym_status_r },
	{ 0xa00000, 0xa00007, irq_r },
{ 0xc00000, 0xc00011, ctrl_r },
	{ 0xc80000, 0xcbffff, MRA16_BANK1 },
	{ 0xcc0000, 0xcc0001, curbank_r },
	{ 0xcc0006, 0xcc0007, mlatch_r },
	{ 0xf00000, 0xf3ffff, system24temp_sys16_shared_ram_r },
	{ 0xfc0000, 0xffffff, MRA16_BANK3 },
MEMORY_END

static MEMORY_WRITE16_START( system24_writemem2 )
	{ 0x000000, 0x03ffff, MWA16_RAM, &system24temp_sys16_shared_ram },
	{ 0x080000, 0x0fffff, MWA16_BANK2 },
	{ 0x200000, 0x20ffff, sys24_tile_w },
	{ 0x220000, 0x220001, MWA16_NOP }, // Unknown, always 0
	{ 0x240000, 0x240001, MWA16_NOP }, // Horizontal synchronization register
	{ 0x260000, 0x260001, MWA16_NOP }, // Vertical synchronization register
	{ 0x270000, 0x270001, MWA16_NOP }, // Video synchronization switch
	{ 0x280000, 0x29ffff, sys24_char_w },
	{ 0x400000, 0x403fff, system24temp_sys16_paletteram1_w },
	{ 0x404000, 0x40401f, sys24_mixer_w },
	{ 0x600000, 0x63ffff, sys24_sprite_w },
	{ 0x800000, 0x80007f, system24temp_sys16_io_w },
	{ 0x800100, 0x800101, ym_register_w },
	{ 0x800102, 0x800103, ym_data_w },
	{ 0xa00000, 0xa00007, irq_w },
{ 0xc00000, 0xc00011, MWA16_NOP },
	{ 0xcc0000, 0xcc0001, curbank_w },
	{ 0xcc0006, 0xcc0007, mlatch_w },
{ 0xd00300, 0xd00301, MWA16_NOP },
	{ 0xf00000, 0xf3ffff, system24temp_sys16_shared_ram_w },
	{ 0xfc0000, 0xffffff, MWA16_BANK3 },
MEMORY_END

static DRIVER_INIT(qgh)
{
	system24temp_sys16_io_set_callbacks(qgh_io_r, hotrod_io_w, resetcontrol_w, iod_r, iod_w);
	mlatch_table = gqh_mlt;
	track_size = 0;
}

static DRIVER_INIT(qu)
{
	system24temp_sys16_io_set_callbacks(qgh_io_r, hotrod_io_w, resetcontrol_w, iod_r, iod_w);
	mlatch_table = qu_mlt;
	track_size = 0;
}

static DRIVER_INIT(quizmeku)
{
	system24temp_sys16_io_set_callbacks(qgh_io_r, hotrod_io_w, resetcontrol_w, iod_r, iod_w);
	mlatch_table = quizmeku_mlt;
	track_size = 0;
}

static DRIVER_INIT(mahmajn)
{

	system24temp_sys16_io_set_callbacks(mahmajn_io_r, mahmajn_io_w, resetcontrol_w, iod_r, iod_w);
	mlatch_table = mahmajn_mlt;
	track_size = 0;
	cur_input_line = 0;
}

static DRIVER_INIT(mahmajn2)
{

	system24temp_sys16_io_set_callbacks(mahmajn_io_r, mahmajn_io_w, resetcontrol_w, iod_r, iod_w);
	mlatch_table = mahmajn2_mlt;
	track_size = 0;
	cur_input_line = 0;
}

static DRIVER_INIT(hotrod)
{
	system24temp_sys16_io_set_callbacks(hotrod_io_r, hotrod_io_w, resetcontrol_w, iod_r, iod_w);
	mlatch_table = 0;

	// Sector  Size
	// 1       8192
	// 2       1024
	// 3       1024
	// 4       1024
	// 5        512
	// 6        256

	track_size = 0x2f00;
}

static DRIVER_INIT(bnzabros)
{
	system24temp_sys16_io_set_callbacks(hotrod_io_r, hotrod_io_w, resetcontrol_w, iod_r, iod_w);
	mlatch_table = bnzabros_mlt;

	// Sector  Size
	// 1       2048
	// 2       2048
	// 3       2048
	// 4       2048
	// 5       2048
	// 6       1024
	// 7        256

	track_size = 0x2d00;
}

static MACHINE_INIT(system24)
{
	cpu_set_halt_line(1, ASSERT_LINE);
	prev_resetcontrol = resetcontrol = 0x06;
	fdc_init();
	curbank = 0;
	reset_bank();
	irq_init();
	mlatch = 0x00;
}

INPUT_PORTS_START( hotrod )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )


	SYS16_COINAGE

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "Monitor flip" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x30, 0x30, "Initial lives" )
	PORT_DIPSETTING(    0x30, "5" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( bnzabros )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( qgh )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	SYS16_COINAGE

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "Monitor flip" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x30, 0x30, "Initial lives" )
	PORT_DIPSETTING(    0x30, "5" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( mahmajn )
	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "A", KEYCODE_A, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "B", KEYCODE_B, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "C", KEYCODE_C, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "D", KEYCODE_D, IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "E", KEYCODE_E, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "F", KEYCODE_F, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "G", KEYCODE_G, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "H", KEYCODE_H, IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "I", KEYCODE_I, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "J", KEYCODE_J, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "K", KEYCODE_K, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "L", KEYCODE_L, IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "M", KEYCODE_M, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "N", KEYCODE_N, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT, IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z, IP_JOY_NONE )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	SYS16_COINAGE

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Start score" )
	PORT_DIPSETTING(    0x02, "3000" )
	PORT_DIPSETTING(    0x00, "2000" )
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty (computer)" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, "Difficulty (player)" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

ROM_START( hotrod )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr11339.bin", 0x000000, 0x20000, CRC(75130e73) )
	ROM_LOAD16_BYTE( "epr11338.bin", 0x000001, 0x20000, CRC(7d4a7ff3) )

	ROM_REGION( 0x1d6000, REGION_USER2, 0)
	/* not 100% sure of the integrity of this, the game has some strange issues */
	ROM_LOAD( "hotrod3t.dsk", 0x000000, 0x1d6000, CRC(cad6cb5d) ) // label: ds3-5000-01d_3p_turbo
//	ROM_LOAD( "ds3-5000-01d_3p_turbo.bin", 0x000000, 0x1d6000, CRC(cad6cb5d) ) // but mame complains ;-)
ROM_END

ROM_START( qgh )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "16900b", 0x000000, 0x20000, CRC(20d7b7d1) SHA1(345b228c27e5f2fef9a2b8b5f619c59450a070f8) )
	ROM_LOAD16_BYTE( "16899b", 0x000001, 0x20000, CRC(397b3ba9) SHA1(1773212cd87dcff840f3953ec368be7e2394faf0) )

	ROM_REGION16_BE( 0x400000, REGION_USER1, 0)
	ROM_LOAD16_BYTE( "16902a", 0x000000, 0x80000, CRC(d35b7706) SHA1(341bca0af6b6d3f326328a88cdc69c7897b83a0d) )
	ROM_LOAD16_BYTE( "16901a", 0x000001, 0x80000, CRC(ab4bcb33) SHA1(8acd73096eb485c6dc83da6adfcc47d5d0f5b7f3) )
	ROM_LOAD16_BYTE( "16904",  0x100000, 0x80000, CRC(10987c88) SHA1(66f893690565aed613427421958ebe225a20ad0f) )
	ROM_LOAD16_BYTE( "16903",  0x100001, 0x80000, CRC(c19f9e46) SHA1(f1275674a8b44957428d79402f240ca21a34f48d) )
	ROM_LOAD16_BYTE( "16906",  0x200000, 0x80000, CRC(99c6773e) SHA1(568570b607d2cbbedb39ceae5bbc479478fae4ca) )
	ROM_LOAD16_BYTE( "16905",  0x200001, 0x80000, CRC(3922bbe3) SHA1(4378ca900f96138b5e33265ddac56af7b45afbc8) )
	ROM_LOAD16_BYTE( "16908",  0x300000, 0x80000, CRC(407ec20f) SHA1(c8a909d8e9ba024a37a5af6b7920fe7023f80d49) )
	ROM_LOAD16_BYTE( "16907",  0x300001, 0x80000, CRC(734b0a82) SHA1(d3fb31c55e79b99040beb7c49faaf2e17b95aa87) )
ROM_END

ROM_START( qrouka )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "14485", 0x000000, 0x20000, CRC(fc0085f9) )
	ROM_LOAD16_BYTE( "14484", 0x000001, 0x20000, CRC(f51c641c) )

	ROM_REGION16_BE( 0x100000, REGION_USER1, 0)
	ROM_LOAD16_BYTE( "14482", 0x000000, 0x80000, CRC(7a13dd97) )
	ROM_LOAD16_BYTE( "14483", 0x000001, 0x80000, CRC(f3eb51a0) )
ROM_END

ROM_START( mahmajn )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr14813.bin", 0x000000, 0x20000, CRC(ea38cf4b) SHA1(118ab2e0ae20a4db5e619945dfbb3f200de3979c) )
	ROM_LOAD16_BYTE( "epr14812.bin", 0x000001, 0x20000, CRC(5a3cb4a7) SHA1(c0f21282140e8e6e927664f5f2b90525ae0207e9) )

	ROM_REGION16_BE( 0x400000, REGION_USER1, 0)
	ROM_LOAD16_BYTE( "mpr14820.bin", 0x000000, 0x80000, CRC(8d2a03d3) SHA1(b3339bcd101bcfe042e2a1cfdc8baef0a86624df) )
	ROM_LOAD16_BYTE( "mpr14819.bin", 0x000001, 0x80000, CRC(e84c4827) SHA1(54741295e1bdca7d0c78eb795a68b92212d43b2e) )
	ROM_LOAD16_BYTE( "mpr14822.bin", 0x100000, 0x80000, CRC(7c3dcc51) SHA1(a199c2c71cda44a2c8755074c1007d83c8d45d2d) )
	ROM_LOAD16_BYTE( "mpr14821.bin", 0x100001, 0x80000, CRC(bd8dc543) SHA1(fd50b14fa73307a62dc0b522cfedb8b3332c407e) )
	ROM_LOAD16_BYTE( "mpr14824.bin", 0x200000, 0x80000, CRC(38311933) SHA1(237d9a8ffe14ba9ec371bb571d7c9e74a93fe1f3) )
	ROM_LOAD16_BYTE( "mpr14823.bin", 0x200001, 0x80000, CRC(4c8d4550) SHA1(be8717d4080ce932fc8272ebe54e2b0a60b20edd) )
	ROM_LOAD16_BYTE( "mpr14826.bin", 0x300000, 0x80000, CRC(c31b8805) SHA1(b446388c83af2e14300b0c4248470d3a8c504f2c) )
	ROM_LOAD16_BYTE( "mpr14825.bin", 0x300001, 0x80000, CRC(191080a1) SHA1(407c1c5fa4c76732e8a444860094542e90a1e8e8) )
ROM_END

ROM_START( mahmajn2 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr16799.bin", 0x000000, 0x20000, CRC(3a34cf75) SHA1(d22bf6334668af29167cf4244d18f9cd2e7ff7d6) )
	ROM_LOAD16_BYTE( "epr16798.bin", 0x000001, 0x20000, CRC(662923fa) SHA1(dcd3964d899d3f34dab22ffcd1a5af895804fae1) )

	ROM_REGION16_BE( 0x400000, REGION_USER1, 0)
	ROM_LOAD16_BYTE( "mpr16801.bin", 0x000000, 0x80000, CRC(74855a17) SHA1(d2d8e7da7b261e7cb64605284d2c78fbd1465b69) )
	ROM_LOAD16_BYTE( "mpr16800.bin", 0x000001, 0x80000, CRC(6dbc1e02) SHA1(cce5734243ff171759cecb5c05c12dc743a25c1d) )
	ROM_LOAD16_BYTE( "mpr16803.bin", 0x100000, 0x80000, CRC(9b658dd6) SHA1(eaaae289a3555aa6a92f57eea964dbbf48c5c2a4) )
	ROM_LOAD16_BYTE( "mpr16802.bin", 0x100001, 0x80000, CRC(b4723225) SHA1(acb8923c7d9908b1112f8d1f2512f18236915e5d) )
	ROM_LOAD16_BYTE( "mpr16805.bin", 0x200000, 0x80000, CRC(d15528df) SHA1(bda1dd5c98867c2e7666380bca0bc7eef8022fbc) )
	ROM_LOAD16_BYTE( "mpr16804.bin", 0x200001, 0x80000, CRC(a0de08e2) SHA1(2c36b66e74b88fb076e2eaa250c6d06ee0b4ac88) )
	ROM_LOAD16_BYTE( "mpr16807.bin", 0x300000, 0x80000, CRC(816188bb) SHA1(76b2690a6156766a1af94f01f6de1209b7756b2c) )
	ROM_LOAD16_BYTE( "mpr16806.bin", 0x300001, 0x80000, CRC(54b353d3) SHA1(40632e4571b44ee215b5a1f7aab9d89c460a5c9e) )
ROM_END

ROM_START( bnzabros )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr12187.ic2", 0x000000, 0x20000, CRC(e83783f3) )
	ROM_LOAD16_BYTE( "epr12186.ic1", 0x000001, 0x20000, CRC(ce76319d) )

	ROM_REGION16_BE( 0x180000, REGION_USER1, 0)
	ROM_LOAD16_BYTE( "mpr13188.h2",  0x000000, 0x80000, CRC(d3802294) )
	ROM_LOAD16_BYTE( "mpr13187.h1",  0x000001, 0x80000, CRC(e3d8c5f7) )
	ROM_LOAD16_BYTE( "mpr13190.4",   0x080000, 0x40000, CRC(0b4df388) )
	ROM_LOAD16_BYTE( "mpr13189.3",   0x080001, 0x40000, CRC(5ea5a2f3) )

	ROM_REGION( 0x1c2000, REGION_USER2, 0)
	ROM_LOAD( "bb-disk.bin",        0x000000, 0x1c2000, CRC(ea7a3302) )
ROM_END

ROM_START( quizmeku ) // Quiz Mekiromeki Story (not 100% sure of name)
	 ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 code */
	 ROM_LOAD16_BYTE( "epr15343.ic2", 0x000000, 0x20000, CRC(c72399a7) SHA1(bfbf0079ea63f89bca4ce9081aed5d5c1d9d169a) )
	 ROM_LOAD16_BYTE( "epr15342.ic1", 0x000001, 0x20000, CRC(0968ac84) SHA1(4e1170ac123adaec32819754b5075531ff1925fe) )

	 ROM_REGION16_BE( 0x400000, REGION_USER1, 0)
	 ROM_LOAD16_BYTE( "epr15345.ic5", 0x000000, 0x80000, CRC(88030b5d) SHA1(d2feeedb9a64c3dc8dd25716209f945d12fa9b53) )
	 ROM_LOAD16_BYTE( "epr15344.ic4", 0x000001, 0x80000, CRC(dd11b382) SHA1(2b0f49fb307a9aba0f295de64782ee095c557170) )
	 ROM_LOAD16_BYTE( "mpr15347.ic7", 0x100000, 0x80000, CRC(0472677b) SHA1(93ae57a2817b6b54c99814fca28ef51f7ff5e559) )
	 ROM_LOAD16_BYTE( "mpr15346.ic6", 0x100001, 0x80000, CRC(746d4d0e) SHA1(7863abe36126684772a4459d5b6f3b24670ec02b) )
	 ROM_LOAD16_BYTE( "mpr15349.ic9", 0x200000, 0x80000, CRC(770eecf1) SHA1(86cc5b4a325198dc1da1446ecd8e718415b7998a) )
	 ROM_LOAD16_BYTE( "mpr15348.ic8", 0x200001, 0x80000, CRC(7666e960) SHA1(f3f88d5c8318301a8c73141de60292f8349ac0ce) )
ROM_END

static struct YM2151interface ym2151_interface =
{
	1,
	4000000,
	{ YM3012_VOL(50, MIXER_PAN_LEFT, 50, MIXER_PAN_RIGHT) },
	{ irq_ym }
};

static struct DACinterface dac_interface =
{
	1,
	{ 50 }
};

static MACHINE_DRIVER_START( system24 )
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(system24_readmem, system24_writemem)
	MDRV_CPU_VBLANK_INT(irq_vbl, 2)

	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_MEMORY(system24_readmem2, system24_writemem2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(100)
	MDRV_INTERLEAVE(4)

	MDRV_MACHINE_INIT(system24)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(62*8, 48*8)
	MDRV_VISIBLE_AREA(0*8, 62*8-1, 0*8, 48*8-1)
	MDRV_PALETTE_LENGTH(8192*2)

	MDRV_VIDEO_START(system24)
	MDRV_VIDEO_UPDATE(system24)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(DAC,    dac_interface)
MACHINE_DRIVER_END

/* PLAYABLE */

/* generally playable apart from the odd sprite getting in the way of answers, some priority & zooming problems */
GAMEX( 1994, qgh,      0, system24, qgh,      qgh,      ROT0, "Sega", "Quiz Ghost Hunter", GAME_IMPERFECT_GRAPHICS )

/* mostly ok, playable, some graphical problems */
GAMEX( 1992, mahmajn,  0, system24, mahmajn,  mahmajn,  ROT0, "Sega", "Tokoro San no MahMahjan", GAME_IMPERFECT_GRAPHICS )

/* mostly ok, playable, some graphical problems */
GAMEX( 1994, mahmajn2, 0, system24, mahmajn,  mahmajn2, ROT0, "Sega", "Tokoro San no MahMahjan 2", GAME_IMPERFECT_GRAPHICS )

/* reasonable, playable, some graphical problems, appears to take a while to start up */
GAMEX( 1994, quizmeku, 0, system24, qgh,      quizmeku, ROT0, "Sega", "Quiz Mekurumeku Story", GAME_IMPERFECT_GRAPHICS )

/* NOT REALLY WORKING */

/* not working, game boots up but you can't coin up, attract mode doesn't work, graphical problems */
GAMEX( 1988, hotrod,   0, system24, hotrod,   hotrod,   ROT0, "Sega", "Hot Rod", GAME_IMPERFECT_GRAPHICS | GAME_NOT_WORKING )

/* coins up, controls work but not really playable due to serious priority problems */
GAMEX( 1990, bnzabros, 0, system24, bnzabros, bnzabros, ROT0, "Sega", "Bonanza Bros", GAME_IMPERFECT_GRAPHICS )

/* not working, doesn't boot */
GAMEX( 1994, qrouka,   0, system24, qgh,      qu,       ROT0, "Sega", "Quiz Rouka Ni Tattenasai", GAME_NOT_WORKING )

/* Other S24 Games, mostly not dumped / encrypted / only bad disk images exist

Crackdown - Encrypted, Disk Based
Dynamic Country Club - Encrypted, Disk Based
Gain Ground - Encrypted, Disk Based
Jumbo Ozaki Super Masters - Encrypted, Disk Based?
Scramble Spirits - Disk Based, Encrypted and Non-Encrypted versions Exist
Quiz Shukudai Wo Wasuremashita - Encrypted, Disk Based (roms dumped, disk isn't)
+ a bunch of other Japanese Quiz Games

*/
