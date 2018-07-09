/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


unsigned char *slapfight_dpram;
size_t slapfight_dpram_size;

int slapfight_status;
int getstar_sequence_index;
int getstar_sh_intenabled;

static int slapfight_status_state;
extern unsigned char *getstar_e803;

static unsigned char mcu_val;

/* Perform basic machine initialisation */
MACHINE_INIT( slapfight )
{
	/* MAIN CPU */

	slapfight_status_state=0;
	slapfight_status = 0xc7;

	getstar_sequence_index = 0;
	getstar_sh_intenabled = 0;	/* disable sound cpu interrupts */

	/* SOUND CPU */
	cpu_set_reset_line(1,ASSERT_LINE);

	/* MCU */
	mcu_val = 0;
}

/* Interrupt handlers cpu & sound */

WRITE_HANDLER( slapfight_dpram_w )
{
    slapfight_dpram[offset]=data;
}

READ_HANDLER( slapfight_dpram_r )
{
    return slapfight_dpram[offset];
}



/* Slapfight CPU input/output ports

  These ports seem to control memory access

*/

/* Reset and hold sound CPU */
WRITE_HANDLER( slapfight_port_00_w )
{
	cpu_set_reset_line(1,ASSERT_LINE);
	getstar_sh_intenabled = 0;
}

/* Release reset on sound CPU */
WRITE_HANDLER( slapfight_port_01_w )
{
	cpu_set_reset_line(1,CLEAR_LINE);
}

/* Disable and clear hardware interrupt */
WRITE_HANDLER( slapfight_port_06_w )
{
	interrupt_enable_w(0,0);
}

/* Enable hardware interrupt */
WRITE_HANDLER( slapfight_port_07_w )
{
	interrupt_enable_w(0,1);
}

WRITE_HANDLER( slapfight_port_08_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	cpu_setbank(1,&RAM[0x10000]);
}

WRITE_HANDLER( slapfight_port_09_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	cpu_setbank(1,&RAM[0x14000]);
}


/* Status register */
READ_HANDLER( slapfight_port_00_r )
{
	int states[3]={ 0xc7, 0x55, 0x00 };

	slapfight_status = states[slapfight_status_state];

	slapfight_status_state++;
	if (slapfight_status_state > 2) slapfight_status_state = 0;

	return slapfight_status;
}



/*
 Reads at e803 expect a sequence of values such that:
 - first value is different from successive
 - third value is (first+5)^0x56
 I don't know what writes to this address do (connected to port 0 reads?).
*/
READ_HANDLER( getstar_e803_r )
{
unsigned char seq[] = { 0, 1, ((0+5)^0x56) };
unsigned char val;

	val = seq[getstar_sequence_index];
	getstar_sequence_index = (getstar_sequence_index+1)%3;
	return val;
}

/*
Tiger Heli MCU simulation.
The MCU protection in Tiger Heli is very simple.
It compares for a value to return a specific number,otherwise
it will give the BAD HW message(stored at locations $10AB-$10B5).The program itself says
what kind of value is needed (usually,but not always 0x83).This is simulated by reading
what value the main program asks,then adjusting it to the value really needed
(as it was managed by a real MCU).
The bootlegs patches this with different ways:
\-The first one patches the final 'ret z' opcode check with a 'ret' at 10AAh.
\-The second one patches the e803 checks with a 'ret' at location 109Dh.

-AS 1 may 2k3
*/

READ_HANDLER( tigerh_e803_r )
{
	switch(mcu_val)
	{
		/*This controls the background scroll(tigerh & tigerhj only)*/
		case 0x40:
		case 0x41:
		case 0x42: return 0xf0;
		/*Protection check at start-up*/
		case 0x73: return (mcu_val+0x10);
		case 0xf3: return (mcu_val-0x80);
		default:   return (mcu_val);
	}
}

WRITE_HANDLER( tigerh_e803_w )
{
	//usrintf_showmessage("PC %04x %02x written",activecpu_get_pc(),data);
	mcu_val = data;
}

/* Enable hardware interrupt of sound cpu */
WRITE_HANDLER( getstar_sh_intenable_w )
{
	getstar_sh_intenabled = 1;
	logerror("cpu #1 PC=%d: %d written to a0e0\n",activecpu_get_pc(),data);
}



/* Generate interrups only if they have been enabled */
INTERRUPT_GEN( getstar_interrupt )
{
	if (getstar_sh_intenabled)
		cpu_set_irq_line(1, IRQ_LINE_NMI, PULSE_LINE);
}

WRITE_HANDLER( getstar_port_04_w )
{
//	cpu_halt(0,0);
}
