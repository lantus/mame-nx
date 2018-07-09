/***************************************************************************

	Midway DCS Audio Board

****************************************************************************/

#include "driver.h"
#include "cpu/adsp2100/adsp2100.h"
#include "dcs.h"

#include <math.h>


/***************************************************************************
	COMPILER SWITCHES
****************************************************************************/

#define DISABLE_DCS_SPEEDUP			0
#define DEBUG_DCS					0



/***************************************************************************
	CONSTANTS
****************************************************************************/

#define DCS_BUFFER_SIZE				4096
#define DCS_BUFFER_MASK				(DCS_BUFFER_SIZE - 1)

#define LCTRL_OUTPUT_EMPTY			0x400
#define LCTRL_INPUT_EMPTY			0x800

#define IS_OUTPUT_EMPTY()			(dcs.latch_control & LCTRL_OUTPUT_EMPTY)
#define IS_OUTPUT_FULL()			(!(dcs.latch_control & LCTRL_OUTPUT_EMPTY))
#define SET_OUTPUT_EMPTY()			(dcs.latch_control |= LCTRL_OUTPUT_EMPTY)
#define SET_OUTPUT_FULL()			(dcs.latch_control &= ~LCTRL_OUTPUT_EMPTY)

#define IS_INPUT_EMPTY()			(dcs.latch_control & LCTRL_INPUT_EMPTY)
#define IS_INPUT_FULL()				(!(dcs.latch_control & LCTRL_INPUT_EMPTY))
#define SET_INPUT_EMPTY()			(dcs.latch_control |= LCTRL_INPUT_EMPTY)
#define SET_INPUT_FULL()			(dcs.latch_control &= ~LCTRL_INPUT_EMPTY)



/***************************************************************************
	STRUCTURES
****************************************************************************/

struct dcs_state
{
	int		stream;

	UINT8 * mem;
	UINT16	size;
	UINT16	incs;
	void  * reg_timer;
	void  * sport_timer;
	int		ireg;
	UINT16	ireg_base;
	UINT16	control_regs[32];

	UINT16	rombank;
	UINT16	rombank_count;
	UINT16	srambank;
	UINT16	drambank;
	UINT16	drambank_count;
	UINT8	enabled;

	INT16 *	buffer;
	UINT32	buffer_in;
	UINT32	sample_step;
	UINT32	sample_position;
	INT16	current_sample;

	UINT16	latch_control;
	UINT16	input_data;
	UINT16	output_data;
	UINT16	output_control;

	void	(*notify)(int);
};



/***************************************************************************
	STATIC GLOBALS
****************************************************************************/

static INT8 dcs_cpunum;

static struct dcs_state dcs;

static data16_t *dcs_speedup1;
static data16_t *dcs_speedup2;
static data16_t *dcs_speedup3;
static data16_t *dcs_speedup4;

static data16_t *dcs_sram_bank0;
static data16_t *dcs_sram_bank1;

#if DEBUG_DCS
static int transfer_state;
static int transfer_start;
static int transfer_stop;
static int transfer_writes_left;
static UINT16 transfer_sum;
#endif



/***************************************************************************
	PROTOTYPES
****************************************************************************/

static int dcs_custom_start(const struct MachineSound *msound);
static void dcs_dac_update(int num, INT16 *buffer, int length);

static READ16_HANDLER( dcs_sdrc_asic_ver_r );

static WRITE16_HANDLER( dcs_rombank_select_w );
static READ16_HANDLER( dcs_rombank_data_r );
static WRITE16_HANDLER( dcs_sram_bank_w );
static READ16_HANDLER( dcs_sram_bank_r );
static WRITE16_HANDLER( dcs_dram_bank_w );
static READ16_HANDLER( dcs_dram_bank_r );

static WRITE16_HANDLER( dcs_control_w );

static READ16_HANDLER( latch_status_r );
static READ16_HANDLER( input_latch_r );
static WRITE16_HANDLER( output_latch_w );
static READ16_HANDLER( output_control_r );
static WRITE16_HANDLER( output_control_w );

static void dcs_irq(int state);
static void sport0_irq(int state);
static void sound_tx_callback(int port, INT32 data);

static WRITE16_HANDLER(dcs_speedup1_w);
static WRITE16_HANDLER(dcs_speedup2_w);
static WRITE16_HANDLER(dcs_speedup3_w);
static WRITE16_HANDLER(dcs_speedup4_w);
static void dcs_speedup_common(void);



/***************************************************************************
	PROCESSOR STRUCTURES
****************************************************************************/

/* DCS readmem/writemem structures */
MEMORY_READ16_START( dcs_readmem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MRA16_RAM },			/* ??? */
	{ ADSP_DATA_ADDR_RANGE(0x2000, 0x2fff), dcs_rombank_data_r },	/* banked roms read */
	{ ADSP_DATA_ADDR_RANGE(0x3400, 0x3403), input_latch_r },		/* soundlatch read */
	{ ADSP_DATA_ADDR_RANGE(0x3800, 0x39ff), MRA16_RAM },			/* internal data ram */
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x1fff), MRA16_RAM },			/* internal/external program ram */
MEMORY_END


MEMORY_WRITE16_START( dcs_writemem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MWA16_RAM },			/* ??? */
	{ ADSP_DATA_ADDR_RANGE(0x3000, 0x3000), dcs_rombank_select_w },	/* bank selector */
	{ ADSP_DATA_ADDR_RANGE(0x3400, 0x3403), output_latch_w },		/* soundlatch write */
	{ ADSP_DATA_ADDR_RANGE(0x3800, 0x39ff), MWA16_RAM },			/* internal data ram */
	{ ADSP_DATA_ADDR_RANGE(0x3fe0, 0x3fff), dcs_control_w },		/* adsp control regs */
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x1fff), MWA16_RAM },			/* internal/external program ram */
MEMORY_END



/* DCS with UART readmem/writemem structures */
MEMORY_READ16_START( dcs_uart_readmem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MRA16_RAM },			/* ??? */
	{ ADSP_DATA_ADDR_RANGE(0x2000, 0x2fff), dcs_rombank_data_r },	/* banked roms read */
	{ ADSP_DATA_ADDR_RANGE(0x3400, 0x3402), MRA16_NOP },			/* UART (ignored) */
	{ ADSP_DATA_ADDR_RANGE(0x3403, 0x3403), input_latch_r },		/* soundlatch read */
	{ ADSP_DATA_ADDR_RANGE(0x3404, 0x3405), MRA16_NOP },			/* UART (ignored) */
	{ ADSP_DATA_ADDR_RANGE(0x3800, 0x39ff), MRA16_RAM },			/* internal data ram */
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x1fff), MRA16_RAM },			/* internal/external program ram */
MEMORY_END


MEMORY_WRITE16_START( dcs_uart_writemem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MWA16_RAM },			/* ??? */
	{ ADSP_DATA_ADDR_RANGE(0x3000, 0x3000), dcs_rombank_select_w },	/* bank selector */
	{ ADSP_DATA_ADDR_RANGE(0x3400, 0x3402), MWA16_NOP },			/* UART (ignored) */
	{ ADSP_DATA_ADDR_RANGE(0x3403, 0x3403), output_latch_w },		/* soundlatch write */
	{ ADSP_DATA_ADDR_RANGE(0x3404, 0x3405), MWA16_NOP },			/* UART (ignored) */
	{ ADSP_DATA_ADDR_RANGE(0x3800, 0x39ff), MWA16_RAM },			/* internal data ram */
	{ ADSP_DATA_ADDR_RANGE(0x3fe0, 0x3fff), dcs_control_w },		/* adsp control regs */
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x1fff), MWA16_RAM },			/* internal/external program ram */
MEMORY_END



/* DCS RAM-based readmem/writemem structures */
MEMORY_READ16_START( dcs_ram_readmem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x03ff), MRA16_BANK20 },			/* D/RAM */
	{ ADSP_DATA_ADDR_RANGE(0x0400, 0x0400), input_latch_r },		/* input latch read */
	{ ADSP_DATA_ADDR_RANGE(0x0402, 0x0402), output_control_r },		/* secondary soundlatch read */
	{ ADSP_DATA_ADDR_RANGE(0x0403, 0x0403), latch_status_r },		/* latch status read */
	{ ADSP_DATA_ADDR_RANGE(0x0480, 0x0480), dcs_sram_bank_r },		/* S/RAM bank */
	{ ADSP_DATA_ADDR_RANGE(0x0481, 0x0481), MRA16_NOP },			/* LED in bit $2000 */
	{ ADSP_DATA_ADDR_RANGE(0x0482, 0x0482), dcs_dram_bank_r },		/* D/RAM bank */
	{ ADSP_DATA_ADDR_RANGE(0x0483, 0x0483), dcs_sdrc_asic_ver_r },	/* SDRC version number */
	{ ADSP_DATA_ADDR_RANGE(0x0800, 0x17ff), MRA16_RAM },			/* S/RAM */
	{ ADSP_DATA_ADDR_RANGE(0x1800, 0x27ff), MRA16_BANK21 },			/* banked S/RAM */
	{ ADSP_DATA_ADDR_RANGE(0x2800, 0x37ff), MRA16_RAM },			/* S/RAM */
	{ ADSP_DATA_ADDR_RANGE(0x3800, 0x39ff), MRA16_RAM },			/* internal data ram */
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x3fff), MRA16_RAM },			/* internal/external program ram */
MEMORY_END


MEMORY_WRITE16_START( dcs_ram_writemem )
	{ ADSP_DATA_ADDR_RANGE(0x0000, 0x03ff), MWA16_BANK20 },			/* D/RAM */
	{ ADSP_DATA_ADDR_RANGE(0x0400, 0x0400), MWA16_NOP },			/* input latch ack */
	{ ADSP_DATA_ADDR_RANGE(0x0401, 0x0401), output_latch_w },		/* soundlatch write */
	{ ADSP_DATA_ADDR_RANGE(0x0402, 0x0402), output_control_w },		/* secondary soundlatch write */
	{ ADSP_DATA_ADDR_RANGE(0x0480, 0x0480), dcs_sram_bank_w },		/* S/RAM bank */
	{ ADSP_DATA_ADDR_RANGE(0x0481, 0x0481), MWA16_NOP },			/* LED in bit $2000 */
	{ ADSP_DATA_ADDR_RANGE(0x0482, 0x0482), dcs_dram_bank_w },		/* D/RAM bank */
	{ ADSP_DATA_ADDR_RANGE(0x0800, 0x17ff), MWA16_RAM },			/* S/RAM */
	{ ADSP_DATA_ADDR_RANGE(0x1800, 0x27ff), MWA16_BANK21, &dcs_sram_bank0 },/* banked S/RAM */
	{ ADSP_DATA_ADDR_RANGE(0x2800, 0x37ff), MWA16_RAM },			/* S/RAM */
	{ ADSP_DATA_ADDR_RANGE(0x3800, 0x39ff), MWA16_RAM },			/* internal data ram */
	{ ADSP_DATA_ADDR_RANGE(0x3fe0, 0x3fff), dcs_control_w },		/* adsp control regs */
	{ ADSP_PGM_ADDR_RANGE (0x0000, 0x3fff), MWA16_RAM },			/* internal/external program ram */
MEMORY_END



/***************************************************************************
	AUDIO STRUCTURES
****************************************************************************/

/* Custom structure (DCS variant) */
static struct CustomSound_interface dcs_custom_interface =
{
	dcs_custom_start,0,0
};



/***************************************************************************
	MACHINE DRIVERS
****************************************************************************/

MACHINE_DRIVER_START( dcs_audio )
	MDRV_CPU_ADD_TAG("dcs", ADSP2105, 10240000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(dcs_readmem,dcs_writemem)

	MDRV_SOUND_ADD(CUSTOM, dcs_custom_interface)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( dcs_audio_uart )
	MDRV_IMPORT_FROM(dcs_audio)

	MDRV_CPU_MODIFY("dcs")
	MDRV_CPU_MEMORY(dcs_uart_readmem,dcs_uart_writemem)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( dcs_audio_ram )
	MDRV_CPU_ADD_TAG("dcsram", ADSP2115, 16000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(dcs_ram_readmem,dcs_ram_writemem)

	MDRV_SOUND_ADD(CUSTOM, dcs_custom_interface)
MACHINE_DRIVER_END



/***************************************************************************
	INITIALIZATION
****************************************************************************/

static void dcs_reset(void)
{
	int i;

	/* initialize our state structure and install the transmit callback */
	dcs.mem = 0;
	dcs.size = 0;
	dcs.incs = 0;
	dcs.ireg = 0;

	/* initialize the ADSP control regs */
	for (i = 0; i < sizeof(dcs.control_regs) / sizeof(dcs.control_regs[0]); i++)
		dcs.control_regs[i] = 0;

	/* initialize banking */
	dcs.rombank = 0;
	dcs.srambank = 0;
	dcs.drambank = 0;
	if (dcs_sram_bank0)
	{
		cpu_setbank(20, memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_SIZE + 0x8000);
		cpu_setbank(21, dcs_sram_bank0);
	}

	/* start with no sound output */
	dcs.enabled = 0;

	/* reset DAC generation */
	dcs.buffer_in = 0;
	dcs.sample_step = 0x10000;
	dcs.sample_position = 0;
	dcs.current_sample = 0;

	/* initialize the ADSP Tx callback */
	adsp2105_set_tx_callback(sound_tx_callback);

	/* clear all interrupts */
	cpu_set_irq_line(dcs_cpunum, ADSP2105_IRQ0, CLEAR_LINE);
	cpu_set_irq_line(dcs_cpunum, ADSP2105_IRQ1, CLEAR_LINE);
	cpu_set_irq_line(dcs_cpunum, ADSP2105_IRQ2, CLEAR_LINE);

	/* initialize the comm bits */
	SET_INPUT_EMPTY();
	SET_OUTPUT_EMPTY();

	/* disable notification by default */
	dcs.notify = NULL;

	/* boot */
	{
		data8_t *src = (data8_t *)(memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_SIZE);
		data32_t *dst = (data32_t *)(memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_PGM_OFFSET);
		adsp2105_load_boot_data(src, dst);
	}

	/* start the SPORT0 timer */
	if (dcs.sport_timer)
		timer_adjust(dcs.sport_timer, TIME_IN_HZ(1000), 0, TIME_IN_HZ(1000));
}


void dcs_init(void)
{
	/* find the DCS CPU */
	dcs_cpunum = mame_find_cpu_index("dcs");

	/* reset RAM-based variables */
	dcs_sram_bank0 = dcs_sram_bank1 = NULL;

	/* install the speedup handler */
#if (!DISABLE_DCS_SPEEDUP)
	dcs_speedup1 = install_mem_write16_handler(dcs_cpunum, ADSP_DATA_ADDR_RANGE(0x04f8, 0x04f8), dcs_speedup1_w);
	dcs_speedup2 = install_mem_write16_handler(dcs_cpunum, ADSP_DATA_ADDR_RANGE(0x063d, 0x063d), dcs_speedup2_w);
	dcs_speedup3 = install_mem_write16_handler(dcs_cpunum, ADSP_DATA_ADDR_RANGE(0x063a, 0x063a), dcs_speedup3_w);
	dcs_speedup4 = install_mem_write16_handler(dcs_cpunum, ADSP_DATA_ADDR_RANGE(0x0641, 0x0641), dcs_speedup4_w);
#endif

	/* create the timer */
	dcs.reg_timer = timer_alloc(dcs_irq);
	dcs.sport_timer = NULL;

	/* reset the system */
	dcs_reset();
}


void dcs_ram_init(void)
{
	/* find the DCS CPU */
	dcs_cpunum = mame_find_cpu_index("dcsram");

	/* borrow memory for the extra 8k */
	dcs_sram_bank1 = (UINT16 *)(memory_region(REGION_CPU1 + dcs_cpunum) + 0x4000*2);

	/* install the speedup handler */
#if (!DISABLE_DCS_SPEEDUP)
//	dcs_speedup1 = install_mem_write16_handler(dcs_cpunum, ADSP_DATA_ADDR_RANGE(0x04f8, 0x04f8), dcs_speedup1_w);
//	dcs_speedup2 = install_mem_write16_handler(dcs_cpunum, ADSP_DATA_ADDR_RANGE(0x063d, 0x063d), dcs_speedup2_w);
#endif

	/* create the timer */
	dcs.reg_timer = timer_alloc(dcs_irq);
	dcs.sport_timer = timer_alloc(sport0_irq);

	/* reset the system */
	dcs_reset();
}



/***************************************************************************
	CUSTOM SOUND INTERFACES
****************************************************************************/

static int dcs_custom_start(const struct MachineSound *msound)
{
	/* allocate a DAC stream */
	dcs.stream = stream_init("DCS DAC", 100, Machine->sample_rate, 0, dcs_dac_update);

	/* allocate memory for our buffer */
	dcs.buffer = auto_malloc(DCS_BUFFER_SIZE * sizeof(INT16));
	if (!dcs.buffer)
		return 1;

	return 0;
}



/***************************************************************************
	DCS BANK SELECT
****************************************************************************/

static READ16_HANDLER( dcs_sdrc_asic_ver_r )
{
	return 0x5a03;
}



/***************************************************************************
	DCS BANK SELECT
****************************************************************************/

static WRITE16_HANDLER( dcs_rombank_select_w )
{
	dcs.rombank = data & 0x7ff;

	/* bit 11 = sound board led */
#if 0
	set_led_status(2, data & 0x800);
#endif

#if (!DISABLE_DCS_SPEEDUP)
	/* they write 0x800 here just before entering the stall loop */
	if (data == 0x800)
	{
		/* calculate the next buffer address */
		int source = activecpu_get_reg(ADSP2100_I0 + dcs.ireg);
		int ar = source + dcs.size / 2;

		/* check for wrapping */
		if (ar >= (dcs.ireg_base + dcs.size))
			ar = dcs.ireg_base;

		/* set it */
		activecpu_set_reg(ADSP2100_AR, ar);

		/* go around the buffer syncing code, we sync manually */
		activecpu_set_reg(ADSP2100_PC, activecpu_get_pc() + 8);
		cpu_spinuntil_int();
	}
#endif
}


static READ16_HANDLER( dcs_rombank_data_r )
{
	UINT8	*banks = memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_SIZE;

	offset += (dcs.rombank & 0x7ff) << 12;
	return banks[BYTE_XOR_LE(offset)];
}


static WRITE16_HANDLER( dcs_sram_bank_w )
{
	COMBINE_DATA(&dcs.srambank);
	cpu_setbank(21, (dcs.srambank & 0x1000) ? dcs_sram_bank1 : dcs_sram_bank0);
}


static READ16_HANDLER( dcs_sram_bank_r )
{
	return dcs.srambank;
}


static WRITE16_HANDLER( dcs_dram_bank_w )
{
	dcs.drambank = data & 0x7ff;
logerror("dcs_dram_bank_w(%03X)\n", dcs.drambank);
	cpu_setbank(20, memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_SIZE + 0x8000 + dcs.drambank * 0x400*2);
}


static READ16_HANDLER( dcs_dram_bank_r )
{
	return dcs.drambank;
}



/***************************************************************************
	DCS COMMUNICATIONS
****************************************************************************/

void dcs_set_notify(void (*callback)(int))
{
	dcs.notify = callback;
}


int dcs_control_r(void)
{
	/* this is read by the TMS before issuing a command to check */
	/* if the ADSP has read the last one yet. We give 50 usec to */
	/* the ADSP to read the latch and thus preventing any sound  */
	/* loss */
	if (IS_INPUT_FULL())
		cpu_spinuntil_time(TIME_IN_USEC(50));

	return dcs.latch_control;
}


void dcs_reset_w(int state)
{
	logerror("%08x: DCS reset = %d\n", activecpu_get_pc(), state);

	/* going high halts the CPU */
	if (state)
	{
		/* just run through the init code again */
		dcs_reset();
		cpu_set_reset_line(dcs_cpunum, ASSERT_LINE);
	}

	/* going low resets and reactivates the CPU */
	else
		cpu_set_reset_line(dcs_cpunum, CLEAR_LINE);
}


static READ16_HANDLER( latch_status_r )
{
	int result = 0;
	if (IS_INPUT_FULL())
		result |= 0x80;
	if (IS_OUTPUT_EMPTY())
		result |= 0x40;
	return result;
}



/***************************************************************************
	INPUT LATCH (data from host to DCS)
****************************************************************************/

static void delayed_dcs_w(int data)
{
	SET_INPUT_FULL();
	cpu_set_irq_line(dcs_cpunum, ADSP2105_IRQ2, ASSERT_LINE);
	dcs.input_data = data;

	timer_set(TIME_IN_USEC(1), 0, NULL);

#if DEBUG_DCS
switch (transfer_state)
{
	case 0:
		if (data == 0x55d0)
		{
			printf("Transfer command\n");
			transfer_state++;
		}
		else if (data == 0x55d1)
		{
			printf("Transfer command alternate\n");
			transfer_state++;
		}
		else
			printf("Command: %04X\n", data);
		break;
	case 1:
		transfer_start = data << 16;
		transfer_state++;
		break;
	case 2:
		transfer_start |= data;
		transfer_state++;
		printf("Start address = %08X\n", transfer_start);
		break;
	case 3:
		transfer_stop = data << 16;
		transfer_state++;
		break;
	case 4:
		transfer_stop |= data;
		transfer_state++;
		printf("Stop address = %08X\n", transfer_stop);
		transfer_writes_left = transfer_stop - transfer_start + 1;
		transfer_sum = 0;
		break;
	case 5:
		transfer_sum += data;
		if (--transfer_writes_left == 0)
		{
			printf("Transfer done, sum = %04X\n", transfer_sum);
			transfer_state = 0;
		}
		break;
}
#endif
}


void dcs_data_w(int data)
{
	timer_set(TIME_NOW, data, delayed_dcs_w);
}


static READ16_HANDLER( input_latch_r )
{
	SET_INPUT_EMPTY();
	cpu_set_irq_line(dcs_cpunum, ADSP2105_IRQ2, CLEAR_LINE);
	return dcs.input_data;
}



/***************************************************************************
	OUTPUT LATCH (data from DCS to host)
****************************************************************************/

static void latch_delayed_w(int data)
{
logerror("output_data = %04X\n", data);
	if (IS_OUTPUT_EMPTY() && dcs.notify)
		(*dcs.notify)(1);
	SET_OUTPUT_FULL();
	dcs.output_data = data;
}


static WRITE16_HANDLER( output_latch_w )
{
#if DEBUG_DCS
	printf("%04X:Output data = %04X\n", activecpu_get_pc(), data);
#endif
	timer_set(TIME_NOW, data, latch_delayed_w);
}


int dcs_data_r(void)
{
	/* data is actually only 8 bit (read from d8-d15) */
	if (IS_OUTPUT_FULL() && dcs.notify)
		(*dcs.notify)(0);
	SET_OUTPUT_EMPTY();

	return dcs.output_data;
}



/***************************************************************************
	OUTPUT CONTROL BITS (has 3 additional lines to the host)
****************************************************************************/

static void output_control_delayed_w(int data)
{
logerror("output_control = %04X\n", data);
	dcs.output_control = data;
}


static WRITE16_HANDLER( output_control_w )
{
	timer_set(TIME_NOW, data, output_control_delayed_w);
}


static READ16_HANDLER( output_control_r )
{
	return dcs.output_control;
}


int dcs_data2_r(void)
{
	return dcs.output_control;
}



/***************************************************************************
	SOUND GENERATION
****************************************************************************/

static void dcs_dac_update(int num, INT16 *buffer, int length)
{
	UINT32 current, step, indx;
	INT16 *source;
	int i;

	/* DAC generation */
	if (dcs.enabled)
	{
		source = dcs.buffer;
		current = dcs.sample_position;
		step = dcs.sample_step;

		/* fill in with samples until we hit the end or run out */
		for (i = 0; i < length; i++)
		{
			indx = current >> 16;
			if (indx >= dcs.buffer_in)
				break;
			current += step;
			*buffer++ = source[indx & DCS_BUFFER_MASK];
		}

if (i < length)
	logerror("DCS ran out of input data\n");

		/* fill the rest with the last sample */
		for ( ; i < length; i++)
			*buffer++ = source[(dcs.buffer_in - 1) & DCS_BUFFER_MASK];

		/* mask off extra bits */
		while (current >= (DCS_BUFFER_SIZE << 16))
		{
			current -= DCS_BUFFER_SIZE << 16;
			dcs.buffer_in -= DCS_BUFFER_SIZE;
		}

logerror("DCS dac update: bytes in buffer = %d\n", dcs.buffer_in - (current >> 16));

		/* update the final values */
		dcs.sample_position = current;
	}
	else
		memset(buffer, 0, length * sizeof(INT16));
}



/***************************************************************************
	ADSP CONTROL & TRANSMIT CALLBACK
****************************************************************************/

/*
	The ADSP2105 memory map when in boot rom mode is as follows:

	Program Memory:
	0x0000-0x03ff = Internal Program Ram (contents of boot rom gets copied here)
	0x0400-0x07ff = Reserved
	0x0800-0x3fff = External Program Ram

	Data Memory:
	0x0000-0x03ff = External Data - 0 Waitstates
	0x0400-0x07ff = External Data - 1 Waitstates
	0x0800-0x2fff = External Data - 2 Waitstates
	0x3000-0x33ff = External Data - 3 Waitstates
	0x3400-0x37ff = External Data - 4 Waitstates
	0x3800-0x39ff = Internal Data Ram
	0x3a00-0x3bff = Reserved (extra internal ram space on ADSP2101, etc)
	0x3c00-0x3fff = Memory Mapped control registers & reserved.
*/

/* These are the some of the control register, we dont use them all */
enum
{
	S1_AUTOBUF_REG = 15,
	S1_RFSDIV_REG,
	S1_SCLKDIV_REG,
	S1_CONTROL_REG,
	S0_AUTOBUF_REG,
	S0_RFSDIV_REG,
	S0_SCLKDIV_REG,
	S0_CONTROL_REG,
	S0_MCTXLO_REG,
	S0_MCTXHI_REG,
	S0_MCRXLO_REG,
	S0_MCRXHI_REG,
	TIMER_SCALE_REG,
	TIMER_COUNT_REG,
	TIMER_PERIOD_REG,
	WAITSTATES_REG,
	SYSCONTROL_REG
};


static WRITE16_HANDLER( dcs_control_w )
{
	dcs.control_regs[offset] = data;
logerror("dcs_control_w(%X) = %04X\n", offset, data);
	switch (offset)
	{
		case SYSCONTROL_REG:
			if (data & 0x0200)
			{
				/* boot force */
				cpu_set_reset_line(dcs_cpunum, PULSE_LINE);
				{
					data8_t *src = (data8_t *)(memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_SIZE + ((dcs.rombank & 0x7ff) << 12));
					data32_t *dst = (data32_t *)(memory_region(REGION_CPU1 + dcs_cpunum) + ADSP2100_PGM_OFFSET);
					adsp2105_load_boot_data(src, dst);
				}
				dcs.control_regs[SYSCONTROL_REG] &= ~0x0200;
			}

			/* see if SPORT1 got disabled */
			stream_update(dcs.stream, 0);
			if ((data & 0x0800) == 0)
			{
				dcs.enabled = 0;
				timer_adjust(dcs.reg_timer, TIME_NEVER, 0, 0);
			}
			break;

		case S1_AUTOBUF_REG:
			/* autobuffer off: nuke the timer, and disable the DAC */
			stream_update(dcs.stream, 0);
			if ((data & 0x0002) == 0)
			{
				dcs.enabled = 0;
				timer_adjust(dcs.reg_timer, TIME_NEVER, 0, 0);
			}
			break;

		case S1_CONTROL_REG:
			if (((data >> 4) & 3) == 2)
				logerror("Oh no!, the data is compresed with u-law encoding\n");
			if (((data >> 4) & 3) == 3)
				logerror("Oh no!, the data is compresed with A-law encoding\n");
			break;
	}
}



/***************************************************************************
	DCS IRQ GENERATION CALLBACKS
****************************************************************************/

static void dcs_irq(int state)
{
	/* get the index register */
	int reg = cpunum_get_reg(dcs_cpunum, ADSP2100_I0 + dcs.ireg);

	/* translate into data memory bus address */
	int source = ADSP2100_DATA_OFFSET + (reg << 1);
	int i;

	/* copy the current data into the buffer */
	for (i = 0; i < dcs.size / 2; i += dcs.incs)
		dcs.buffer[dcs.buffer_in++ & DCS_BUFFER_MASK] = ((UINT16 *)&dcs.mem[source])[i];

	/* increment it */
	reg += dcs.size / 2;

	/* check for wrapping */
	if (reg >= dcs.ireg_base + dcs.size)
	{
		/* reset the base pointer */
		reg = dcs.ireg_base;

		/* generate the (internal, thats why the pulse) irq */
		cpu_set_irq_line(dcs_cpunum, ADSP2105_IRQ1, PULSE_LINE);
	}

	/* store it */
	cpunum_set_reg(dcs_cpunum, ADSP2100_I0 + dcs.ireg, reg);

#if (!DISABLE_DCS_SPEEDUP)
	/* this is the same trigger as an interrupt */
	cpu_triggerint(dcs_cpunum);
#endif
}


static void sport0_irq(int state)
{
	/* this latches internally, so we just pulse */
	cpu_set_irq_line(dcs_cpunum, ADSP2115_SPORT0_RX, PULSE_LINE);
}


static void sound_tx_callback(int port, INT32 data)
{
	/* check if it's for SPORT1 */
	if (port != 1)
		return;

	/* check if SPORT1 is enabled */
	if (dcs.control_regs[SYSCONTROL_REG] & 0x0800) /* bit 11 */
	{
		/* we only support autobuffer here (wich is what this thing uses), bail if not enabled */
		if (dcs.control_regs[S1_AUTOBUF_REG] & 0x0002) /* bit 1 */
		{
			/* get the autobuffer registers */
			int		mreg, lreg;
			UINT16	source;
			int		sample_rate;

			stream_update(dcs.stream, 0);

			dcs.ireg = (dcs.control_regs[S1_AUTOBUF_REG] >> 9) & 7;
			mreg = (dcs.control_regs[S1_AUTOBUF_REG] >> 7) & 3;
			mreg |= dcs.ireg & 0x04; /* msb comes from ireg */
			lreg = dcs.ireg;

			/* now get the register contents in a more legible format */
			/* we depend on register indexes to be continuous (wich is the case in our core) */
			source = cpunum_get_reg(dcs_cpunum, ADSP2100_I0 + dcs.ireg);
			dcs.incs = cpunum_get_reg(dcs_cpunum, ADSP2100_M0 + mreg);
			dcs.size = cpunum_get_reg(dcs_cpunum, ADSP2100_L0 + lreg);
#if DEBUG_DCS
	printf("source = %04X(I%d), incs = %04X(M%d), size = %04X(L%d)\n", source, dcs.ireg, dcs.incs, mreg, dcs.size, lreg);
#endif

			/* get the base value, since we need to keep it around for wrapping */
			source -= dcs.incs;

			/* make it go back one so we dont lose the first sample */
			cpunum_set_reg(dcs_cpunum, ADSP2100_I0 + dcs.ireg, source);

			/* save it as it is now */
			dcs.ireg_base = source;

			/* get the memory chunk to read the data from */
			dcs.mem = memory_region(REGION_CPU1 + dcs_cpunum);

			/* enable the dac playing */
			dcs.enabled = 1;

			/* calculate how long until we generate an interrupt */

			/* frequency in Hz per each bit sent */
			sample_rate = Machine->drv->cpu[dcs_cpunum].cpu_clock / (2 * (dcs.control_regs[S1_SCLKDIV_REG] + 1));

			/* now put it down to samples, so we know what the channel frequency has to be */
			sample_rate /= 16;

			/* fire off a timer wich will hit every half-buffer */
			timer_adjust(dcs.reg_timer, TIME_IN_HZ(sample_rate) * (dcs.size / (2 * dcs.incs)), 0, TIME_IN_HZ(sample_rate) * (dcs.size / (2 * dcs.incs)));

			/* configure the DAC generator */
			dcs.sample_step = (int)(sample_rate * 65536.0 / (double)Machine->sample_rate);
			dcs.sample_position = 0;
			dcs.buffer_in = 0;

			return;
		}
		else
			logerror( "ADSP SPORT1: trying to transmit and autobuffer not enabled!\n" );
	}

	/* if we get there, something went wrong. Disable playing */
	stream_update(dcs.stream, 0);
	dcs.enabled = 0;

	/* remove timer */
	timer_adjust(dcs.reg_timer, TIME_NEVER, 0, 0);
}



/***************************************************************************
	DCS SPEEDUPS
****************************************************************************/

static WRITE16_HANDLER( dcs_speedup1_w )
{
/*
	MK3:     trigger = $04F8 = 2, PC = $00FD, SKIPTO = $0128
	UMK3:    trigger = $04F8 = 2, PC = $00FD, SKIPTO = $0128
	OPENICE: trigger = $04F8 = 2, PC = $00FD, SKIPTO = $0128
	WWFMANIA:trigger = $04F8 = 2, PC = $00FD, SKIPTO = $0128
	NBAHANGT:trigger = $04F8 = 2, PC = $00FD, SKIPTO = $0128
	NBAMAXHT:trigger = $04F8 = 2, PC = $00FD, SKIPTO = $0128
	RMPGWT:  trigger = $04F8 = 2, PC = $00FD, SKIPTO = $0128
*/
	COMBINE_DATA(&dcs_speedup1[offset]);
	if (data == 2 && activecpu_get_pc() == 0xfd)
		dcs_speedup_common();
}


static WRITE16_HANDLER( dcs_speedup2_w )
{
/*
	MK2:     trigger = $063D = 2, PC = $00F6, SKIPTO = $0121
	REVX:    trigger = $063D = 2, PC = $00F6, SKIPTO = $0121
	KINST:   trigger = $063D = 2, PC = $00F6, SKIPTO = $0121
*/
	COMBINE_DATA(&dcs_speedup2[offset]);
	if (data == 2 && activecpu_get_pc() == 0xf6)
		dcs_speedup_common();
}


static WRITE16_HANDLER( dcs_speedup3_w )
{
/*
	CRUSNUSA: trigger = $063A = 2, PC = $00E1, SKIPTO = $010C
*/
	COMBINE_DATA(&dcs_speedup3[offset]);
	if (data == 2 && activecpu_get_pc() == 0xe1)
		dcs_speedup_common();
}


static WRITE16_HANDLER( dcs_speedup4_w )
{
/*
	CRUSNWLD: trigger = $0641 = 2, PC = $00DA, SKIPTO = $0105
	OFFROADC: trigger = $0641 = 2, PC = $00DA, SKIPTO = $0105
*/
	COMBINE_DATA(&dcs_speedup4[offset]);
	if (data == 2 && activecpu_get_pc() == 0xda)
		dcs_speedup_common();
}


static void dcs_speedup_common(void)
{
/*
	00F4: AR = $0002
	00F5: DM($063D) = AR
	00F6: SI = $0040
	00F7: DM($063E) = SI
	00F8: SR = LSHIFT SI BY -1 (LO)
	00F9: DM($063F) = SR0
	00FA: M0 = $3FFF
	00FB: CNTR = $0006
	00FC: DO $0120 UNTIL CE
		00FD: I4 = $0780
		00FE: I5 = $0700
		00FF: I0 = $3800
		0100: I1 = $3800
		0101: AY0 = DM($063E)
		0102: M2 = AY0
		0103: MODIFY (I1,M2)
		0104: I2 = I1
		0105: AR = AY0 - 1
		0106: M3 = AR
		0107: CNTR = DM($063D)
		0108: DO $0119 UNTIL CE
			0109: CNTR = DM($063F)
			010A: MY0 = DM(I4,M5)
			010B: MY1 = DM(I5,M5)
			010C: MX0 = DM(I1,M1)
			010D: DO $0116 UNTIL CE
				010E: MR = MX0 * MY0 (SS), MX1 = DM(I1,M1)
				010F: MR = MR - MX1 * MY1 (RND), AY0 = DM(I0,M1)
				0110: MR = MX1 * MY0 (SS), AX0 = MR1
				0111: MR = MR + MX0 * MY1 (RND), AY1 = DM(I0,M0)
				0112: AR = AY0 - AX0, MX0 = DM(I1,M1)
				0113: AR = AX0 + AY0, DM(I0,M1) = AR
				0114: AR = AY1 - MR1, DM(I2,M1) = AR
				0115: AR = MR1 + AY1, DM(I0,M1) = AR
				0116: DM(I2,M1) = AR
			0117: MODIFY (I2,M2)
			0118: MODIFY (I1,M3)
			0119: MODIFY (I0,M2)
		011A: SI = DM($063D)
		011B: SR = LSHIFT SI BY 1 (LO)
		011C: DM($063D) = SR0
		011D: SI = DM($063F)
		011E: DM($063E) = SI
		011F: SR = LSHIFT SI BY -1 (LO)
		0120: DM($063F) = SR0
*/

	INT16 *source = (INT16 *)memory_region(REGION_CPU1 + dcs_cpunum);
	int mem63d = 2;
	int mem63e = 0x40;
	int mem63f = mem63e >> 1;
	int i, j, k;

	for (i = 0; i < 6; i++)
	{
		INT16 *i4 = &source[0x780];
		INT16 *i5 = &source[0x700];
		INT16 *i0 = &source[0x3800];
		INT16 *i1 = &source[0x3800 + mem63e];
		INT16 *i2 = i1;

		for (j = 0; j < mem63d; j++)
		{
			INT32 mx0, mx1, my0, my1, ax0, ay0, ay1, mr1, temp;

			my0 = *i4++;
			my1 = *i5++;

			for (k = 0; k < mem63f; k++)
			{
				mx0 = *i1++;
				mx1 = *i1++;
				ax0 = (mx0 * my0 - mx1 * my1) >> 15;
				mr1 = (mx1 * my0 + mx0 * my1) >> 15;
				ay0 = i0[0];
				ay1 = i0[1];

				temp = ay0 - ax0;
				if (temp < -32768) temp = -32768;
				else if (temp > 32767) temp = 32767;
				*i0++ = temp;

				temp = ax0 + ay0;
				if (temp < -32768) temp = -32768;
				else if (temp > 32767) temp = 32767;
				*i2++ = temp;

				temp = ay1 - mr1;
				if (temp < -32768) temp = -32768;
				else if (temp > 32767) temp = 32767;
				*i0++ = temp;

				temp = ay1 + mr1;
				if (temp < -32768) temp = -32768;
				else if (temp > 32767) temp = 32767;
				*i2++ = temp;
			}
			i2 += mem63e;
			i1 += mem63e;
			i0 += mem63e;
		}
		mem63d <<= 1;
		mem63e = mem63f;
		mem63f >>= 1;
	}
	activecpu_set_reg(ADSP2100_PC, activecpu_get_pc() + 0x121 - 0xf6);
}


/* War Gods:
2BA4: 0000      AR = $0002
2BA5: 0000      DM($0A09) = AR
2BA6: 0000      SI = $0040
2BA7: 0000      DM($0A0B) = SI
2BA8: 0000      SR = LSHIFT SI BY -1 (LO)
2BA9: 0000      DM($0A0D) = SR0
2BAA: 0000      M0 = $3FFF
2BAB: 0000      CNTR = $0006
2BAC: 0000      DO $2BD0 UNTIL CE
2BAD: 0000          I4 = $1080
2BAE: 0000          I5 = $1000
2BAF: 0000          I0 = $2000
2BB0: 0000          I1 = $2000
2BB1: 0000          AY0 = DM($0A0B)
2BB2: 0000          M2 = AY0
2BB3: 0000          MODIFY (I1,M2)
2BB4: 0000          I2 = I1
2BB5: 0000          AR = AY0 - 1
2BB6: 0000          M3 = AR
2BB7: 0000          CNTR = DM($0A09)
2BB8: 0000          DO $2BC9 UNTIL CE
2BB9: 0000              CNTR = DM($0A0D)
2BBA: 0000              MY0 = DM(I4,M5)
2BBB: 0000              MY1 = DM(I5,M5)
2BBC: 0000              MX0 = DM(I1,M1)
2BBD: 0000              DO $2BC6 UNTIL CE
2BBE: 0000                  MR = MX0 * MY0 (SS), MX1 = DM(I1,M1)
2BBF: 0000                  MR = MR - MX1 * MY1 (RND), AY0 = DM(I0,M1)
2BC0: 0000                  MR = MX1 * MY0 (SS), AX0 = MR1
2BC1: 0000                  MR = MR + MX0 * MY1 (RND), AY1 = DM(I0,M0)
2BC2: 0000                  AR = AY0 - AX0, MX0 = DM(I1,M1)
2BC3: 0000                  AR = AX0 + AY0, DM(I0,M1) = AR
2BC4: 0000                  AR = AY1 - MR1, DM(I2,M1) = AR
2BC5: 0000                  AR = MR1 + AY1, DM(I0,M1) = AR
2BC6: 0000                  DM(I2,M1) = AR
2BC7: 0000              MODIFY (I2,M2)
2BC8: 0000              MODIFY (I1,M3)
2BC9: 0000              MODIFY (I0,M2)
2BCA: 0000          SI = DM($0A09)
2BCB: 0000          SR = LSHIFT SI BY 1 (LO)
2BCC: 0000          DM($0A09) = SR0
2BCD: 0000          SI = DM($0A0D)
2BCE: 0000          DM($0A0B) = SI
2BCF: 0000          SR = LSHIFT SI BY -1 (LO)
2BD0: 0000          DM($0A0D) = SR0
*/
