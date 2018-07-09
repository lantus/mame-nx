/***************************************************************************

	Emulation of various Midway ICs

***************************************************************************/

#include "driver.h"
#include "midwayic.h"
#include "machine/idectrl.h"
#include "sndhrdw/dcs.h"
#include <time.h>



/*************************************
 *
 *	Constants
 *
 *************************************/

#define PIC_NVRAM_SIZE		0x100



/*************************************
 *
 *	Type definitions
 *
 *************************************/

struct serial_state
{
	UINT8	data[16];
	UINT8	buffer;
	UINT8	index;
	UINT8	status;
	UINT8	bits;
	UINT8	ormask;
};

struct pic_state
{
	UINT16	latch;
	UINT8	state;
	UINT8	index;
	UINT8	total;
	UINT8	nvram_addr;
	UINT8	buffer[0x10];
	UINT8	nvram[PIC_NVRAM_SIZE];
};



/*************************************
 *
 *	Local variables
 *
 *************************************/

static struct serial_state serial;
static struct pic_state pic;

static data32_t io_asic[0x10];




/*************************************
 *
 *	Serial number encoding
 *
 *************************************/

static void generate_serial_data(int upper)
{
	int year = atoi(Machine->gamedrv->year), month = 12, day = 11;
	UINT32 serial_number, temp;
	UINT8 serial_digit[9];

	serial_number = 123456;
	serial_number += upper * 1000000;

	serial_digit[0] = (serial_number / 100000000) % 10;
	serial_digit[1] = (serial_number / 10000000) % 10;
	serial_digit[2] = (serial_number / 1000000) % 10;
	serial_digit[3] = (serial_number / 100000) % 10;
	serial_digit[4] = (serial_number / 10000) % 10;
	serial_digit[5] = (serial_number / 1000) % 10;
	serial_digit[6] = (serial_number / 100) % 10;
	serial_digit[7] = (serial_number / 10) % 10;
	serial_digit[8] = (serial_number / 1) % 10;

	serial.data[12] = rand() & 0xff;
	serial.data[13] = rand() & 0xff;

	serial.data[14] = 0; /* ??? */
	serial.data[15] = 0; /* ??? */

	temp = 0x174 * (year - 1980) + 0x1f * (month - 1) + day;
	serial.data[10] = (temp >> 8) & 0xff;
	serial.data[11] = temp & 0xff;

	temp = serial_digit[4] + serial_digit[7] * 10 + serial_digit[1] * 100;
	temp = (temp + 5 * serial.data[13]) * 0x1bcd + 0x1f3f0;
	serial.data[7] = temp & 0xff;
	serial.data[8] = (temp >> 8) & 0xff;
	serial.data[9] = (temp >> 16) & 0xff;

	temp = serial_digit[6] + serial_digit[8] * 10 + serial_digit[0] * 100 + serial_digit[2] * 10000;
	temp = (temp + 2 * serial.data[13] + serial.data[12]) * 0x107f + 0x71e259;
	serial.data[3] = temp & 0xff;
	serial.data[4] = (temp >> 8) & 0xff;
	serial.data[5] = (temp >> 16) & 0xff;
	serial.data[6] = (temp >> 24) & 0xff;

	temp = serial_digit[5] * 10 + serial_digit[3] * 100;
	temp = (temp + serial.data[12]) * 0x245 + 0x3d74;
	serial.data[0] = temp & 0xff;
	serial.data[1] = (temp >> 8) & 0xff;
	serial.data[2] = (temp >> 16) & 0xff;

	/* special hack for RevX */
	serial.ormask = 0x80;
	if (upper == 419)
		serial.ormask = 0x00;
}



/*************************************
 *
 *	Original serial number PIC
 *	interface
 *
 *************************************/

void midway_serial_pic_init(int upper)
{
	generate_serial_data(upper);
}


void midway_serial_pic_reset_w(int state)
{
	if (state)
	{
		serial.index = 0;
		serial.status = 0;
		serial.buffer = 0;
	}
}


UINT8 midway_serial_pic_status_r(void)
{
	return serial.status;
}


UINT8 midway_serial_pic_r(void)
{
	logerror("%08X:security R = %04X\n", activecpu_get_pc(), serial.buffer);
	serial.status = 1;
	return serial.buffer;
}


void midway_serial_pic_w(UINT8 data)
{
	logerror("%08X:security W = %04X\n", activecpu_get_pc(), data);

	/* status seems to reflect the clock bit */
	serial.status = (data >> 4) & 1;

	/* on the falling edge, clock the next data byte through */
	if (!serial.status)
	{
		/* the self-test writes 1F, 0F, and expects to read an F in the low 4 bits */
		/* Cruis'n World expects the high bit to be set as well */
		if (data & 0x0f)
			serial.buffer = serial.ormask | data;
		else
			serial.buffer = serial.data[serial.index++ % sizeof(serial.data)];
	}
}



/*************************************
 *
 *	Second generation serial number
 *	PIC interface; this version also
 *	contained some NVRAM and a real
 *	time clock
 *
 *************************************/

INLINE UINT8 make_bcd(UINT8 data)
{
	return ((data / 10) << 4) | (data % 10);
}


void midway_serial_pic2_init(int upper)
{
	generate_serial_data(upper);
}


UINT8 midway_serial_pic2_status_r(void)
{
	UINT8 result = 0;

	/* if we're still holding the data ready bit high, do it */
	if (pic.latch & 0xf00)
	{
		pic.latch -= 0x100;
		result = 1;
	}

	logerror("%06X:PIC status %d\n", activecpu_get_pc(), result);
	return result;
}


UINT8 midway_serial_pic2_r(void)
{
	UINT8 result = 0;

	/* PIC data register */
	logerror("%06X:PIC data read (index=%d total=%d latch=%03X) =", activecpu_get_pc(), pic.index, pic.total, pic.latch);

	/* return the current result */
	if (pic.latch & 0xf00)
		result = pic.latch & 0xff;

	/* otherwise, return 0xff if we have data ready */
	else if (pic.index < pic.total)
		result = 0xff;

	logerror("%02X\n", result);
	return result;
}


void midway_serial_pic2_w(UINT8 data)
{
	/* PIC command register */
	if (pic.state == 0)
		logerror("%06X:PIC command %02X\n", activecpu_get_pc(), data);
	else
		logerror("%06X:PIC data %02X\n", activecpu_get_pc(), data);

	/* store in the latch, along with a bit to indicate we have data */
	pic.latch = (data & 0x00f) | 0x480;
	if (data & 0x10)
	{
		int cmd = pic.state ? (pic.state & 0x0f) : (pic.latch & 0x0f);
		switch (cmd)
		{
			/* written to latch the next byte of data */
			case 0:
				if (pic.index < pic.total)
					pic.latch = 0x400 | pic.buffer[pic.index++];
				break;

			/* fetch the serial number */
			case 1:
				memcpy(pic.buffer, serial.data, 16);
				pic.total = 16;
				pic.index = 0;
				break;

			/* read the clock */
			case 3:
			{
				/* get the time */
				struct tm *exptime;
				time_t curtime;
				time(&curtime);
				exptime = localtime(&curtime);

				/* stuff it into the data bytes */
				pic.index = 0;
				pic.total = 0;
				pic.buffer[pic.total++] = make_bcd(exptime->tm_sec);
				pic.buffer[pic.total++] = make_bcd(exptime->tm_min);
				pic.buffer[pic.total++] = make_bcd(exptime->tm_hour);
				pic.buffer[pic.total++] = make_bcd(exptime->tm_wday + 1);
				pic.buffer[pic.total++] = make_bcd(exptime->tm_mday);
				pic.buffer[pic.total++] = make_bcd(exptime->tm_mon + 1);
				pic.buffer[pic.total++] = make_bcd(exptime->tm_year - 94);
				break;
			}

			/* write to NVRAM */
			case 5:

				/* if coming from state 0, go to state 1 (this is just the command byte) */
				if (pic.state == 0)
					pic.state = 0x15;

				/* coming from state 1, go to state 2 and latch the low 4 address bits */
				else if (pic.state == 0x15)
				{
					pic.nvram_addr = pic.latch & 0x0f;
					pic.state = 0x25;
				}

				/* coming from state 2, go to state 3 and latch the high 4 address bits */
				else if (pic.state == 0x25)
				{
					pic.state = 0x35;
					pic.nvram_addr |= pic.latch << 4;
				}

				/* coming from state 3, go to state 4 and write the low 4 bits */
				else if (pic.state == 0x35)
				{
					pic.state = 0x45;
					pic.nvram[pic.nvram_addr] = pic.latch & 0x0f;
				}

				/* coming from state 4, reset the states and write the upper 4 bits */
				else if (pic.state == 0x45)
				{
					pic.state = 0;
					pic.nvram[pic.nvram_addr] |= pic.latch << 4;
				}
				break;

			/* read from NVRAM */
			case 6:

				/* if coming from state 0, go to state 1 (this is just the command byte) */
				if (pic.state == 0)
					pic.state = 0x16;

				/* coming from state 1, go to state 2 and latch the low 4 address bits */
				else if (pic.state == 0x16)
				{
					pic.nvram_addr = pic.latch & 0x0f;
					pic.state = 0x26;
				}

				/* coming from state 2, reset the states and make the data available */
				else if (pic.state == 0x26)
				{
					pic.state = 0;
					pic.nvram_addr |= pic.latch << 4;

					pic.total = 0;
					pic.index = 0;
					pic.buffer[pic.total++] = pic.nvram[pic.nvram_addr];
				}
				break;
		}
	}
}




/*************************************
 *
 *	The I/O ASIC was first introduced
 *	in War Gods, then later used on
 *	the Seattle hardware
 *
 *************************************/

void midway_io_asic_init(int upper)
{
	generate_serial_data(upper);
}


READ32_HANDLER( midway_io_asic_r )
{
	data32_t result = io_asic[offset &= 15];

	switch (offset)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			result = readinputport(offset);
			break;

		case 10:
		{
			/* status from sound CPU */
			result = (dcs_control_r() >> 4) ^ 0x40;
			result |= 8;
			result |= dcs_data2_r() & 0x700;
{
static int last_result, last_pc;
int pc = activecpu_get_pc();
if (last_pc != pc || last_result != result)
			logerror("%06X:io_asic_r(%d) = %08X\n", activecpu_get_pc(), offset, result);
last_pc = pc;
last_result = result;
}
			break;
		}

		case 11:
			result = dcs_data_r();
//			logerror("%06X:dcs_data_r = %08X\n", activecpu_get_pc(), result);
			break;

		case 13:
			result = midway_serial_pic2_r() | (midway_serial_pic2_status_r() << 8);
			break;

		case 14:
			result |= 0x0001;
			break;

		default:
			logerror("%06X:io_asic_r(%d) = %08X\n", activecpu_get_pc(), offset, result);
			break;
	}

	return result;
}


WRITE32_HANDLER( midway_io_asic_w )
{
	offset &= 15;
	COMBINE_DATA(&io_asic[offset]);

	switch (offset)
	{
		case 8:
			/* sound reset? */
			dcs_reset_w(~io_asic[offset] & 1);
			if (io_asic[offset] & 2) printf("FIFO enabled!\n");
			logerror("%06X:io_asic_w(%d) = %08X\n", activecpu_get_pc(), offset, data);
			break;

		case 9:
			dcs_data_w(io_asic[offset]);
//			logerror("%06X:dcs_data_w = %08X\n", activecpu_get_pc(), data);
			break;

		case 11:
			/* acknowledge data read */
			break;

		case 12:
			midway_serial_pic2_w(io_asic[offset]);
			break;

		default:
			logerror("%06X:io_asic_w(%d) = %08X\n", activecpu_get_pc(), offset, data);
			break;
	}
}




/*************************************
 *
 *	The IDE ASIC was used on War Gods
 *	and Killer Instinct to map the IDE
 *	registers
 *
 *************************************/

READ32_HANDLER( midway_ide_asic_r )
{
	/* convert to standard IDE offsets */
	offs_t ideoffs = 0x1f0/4 + (offset >> 2);
	UINT8 shift = 8 * (offset & 3);
	data32_t result;

	/* offset 0 is a special case */
	if (offset == 0)
		result = ide_controller32_0_r(ideoffs, 0xffff0000);

	/* everything else is byte-sized */
	else
		result = ide_controller32_0_r(ideoffs, ~(0xff << shift)) >> shift;
	return result;
}


WRITE32_HANDLER( midway_ide_asic_w )
{
	/* convert to standard IDE offsets */
	offs_t ideoffs = 0x1f0/4 + (offset >> 2);
	UINT8 shift = 8 * (offset & 3);

	/* offset 0 is a special case */
	if (offset == 0)
		ide_controller32_0_w(ideoffs, data, 0xffff0000);

	/* everything else is byte-sized */
	else
		ide_controller32_0_w(ideoffs, data << shift, ~(0xff << shift));;
}
