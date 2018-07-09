/*************************************************************************

	Atari Video Pinball hardware

*************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"
#include "videopin.h"

int ball_position;
int attract = 0;		/* Turns off sound in attract mode */
int volume = 0;			/* Volume of the noise signal */

static int NMI_mask = 1; // Active LOW

static int plunger_counter = -1;

INTERRUPT_GEN( videopin_interrupt )
{
	static int prev,counter;
	int curr;

	/* Plunger simulation
	 *	1) We look at pressure time on the plunger key to evaluate a strength
	 *     The chosen interval is from 0 to 3 seconds, the longer gives more strength
	 *	2) When the key is depressed we activate the NMI,
	 *     which is responsable for reading plunger hardware input.
	 *  3) Thereafter we feed input to the software corresponding to the strength
	 *     The NMI will make 1792 read before setting force to 0.
	 *     The quicker we drop PLUNGER2 LOW, the greater the strength
	 */
	if (cpu_getiloops() == 0)
	{
		curr = readinputport(3) & 1;
		if (curr != prev)
		{
			if (curr)	/* key pressed; initiate count */
			{
				counter = 2*60;
			}
			else	/* key released; cause NMI */
			{
				plunger_counter = counter*5 + 3;
				cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
			}
		}
		if (curr)
		{
			if (counter > 0) counter--;
		}
		prev = curr;
	}

	cpu_set_irq_line(0, 0, HOLD_LINE);
}

WRITE_HANDLER( videopin_out1_w )
{
	NMI_mask = data & 0x10;
	if (NMI_mask)
		{
			logerror("out1_w, NMI mask OFF\n");
		}
	else logerror("out1_w, NMI mask ON\n");

//	if (data & 0x10) logerror("out1_w, NMI mask\n");
//	if (data & 0x08) logerror("out1_w, lockout coil\n");
//	if (data & 0x07) logerror("out1_w, audio frequency : %02x\n", data & 0x07);
}

WRITE_HANDLER( videopin_out2_w )
{
//	if (data & 0x80) logerror("out2_w, audio disable during attract\n");
//	if (data & 0x40) logerror("out2_w, bell audio gen enable\n");
//	if (data & 0x20) logerror("out2_w, bong audio gen enable\n");
//	if (data & 0x10) logerror("out2_w, coin counter\n");
//	if (data & 0x07) logerror("out2_w, audio volume : %02x\n", data & 0x07);
}

WRITE_HANDLER( videopin_led_w )
{
	// No LEDs for now
//	logerror("led_w, LED write : %02x\n", data);
}


WRITE_HANDLER( videopin_watchdog_w )
{
//	logerror("watchdog_w, counter clear %02x:%02x\n", offset, data);
}

WRITE_HANDLER( videopin_ball_position_w )
{
	//logerror("ball_position_w, ball position : %02x\n", data);
	ball_position = data;
}

// No sound yet
// Load audio frequency
WRITE_HANDLER( videopin_note_dvslrd_w )
{
//	logerror("note_dvslrd_w, load audio frequency : %02x\n", data);
}


READ_HANDLER( videopin_in0_r )
{
//	logerror("in0_r\n");
	return input_port_0_r(offset);
}

READ_HANDLER( videopin_in1_r )
{
//	logerror("in1_r\n");
	return input_port_1_r(offset);
}

READ_HANDLER( videopin_in2_r )
{
	int res;

	res = input_port_2_r(0);

	if (plunger_counter >= 0) plunger_counter--;	/* will stop at -1 */
	if (plunger_counter)
		res |= 2;
	if (plunger_counter == -1)
		res |= 1;

	return res;
}

