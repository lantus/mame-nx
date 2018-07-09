/***************************************************************************

  snes.c

  File to handle the sound emulation of the Nintendo Super NES.

  Anthony Kruize
  Based on the original MESS driver by Lee Hammerton (aka Savoury Snax)

  Just a shell for the moment.

***************************************************************************/
#include "driver.h"
#include "includes/snes.h"

static int channel;
UINT8 fakeapu_port[4] = { 0xaa, 0xbb, 0x00, 0x00 };	/* This is just temporary */

int snes_sh_start( const struct MachineSound *driver )
{
	const char *names[2] = { "SNES left", "SNES right" };
	const int volume[2] = { MIXER( 50, MIXER_PAN_LEFT ), MIXER( 50, MIXER_PAN_RIGHT ) };

	channel = stream_init_multi( 2, names, volume, Machine->sample_rate, 0, snes_sh_update );

	return 0;
}

/* --- DSP specific --- */

struct DSP_VOICE
{
	INT8   vol_left;
	INT8   vol_right;
	UINT16 pitch;
	UINT8  source_no;
	UINT16 address;
	UINT8  gain;
	UINT8  envelope;
	UINT8  outx;
};

struct DSP_CONTROL
{
	INT8  vol_left;
	INT8  vol_right;
	INT8  evol_left;
	INT8  evol_right;
	UINT8 key_on;
	UINT8 key_off;
	UINT8 flags;
	UINT8 end_block;
	UINT8 pitch_mod;
	UINT8 noise_on;
	UINT8 dir_offset;
	UINT8 echo_on;
	UINT8 echo_fback;
	UINT8 echo_offset;
	UINT8 echo_delay;
	UINT8 echo_coeff[8];
};

/*static struct DSP_VOICE   snd_voice[8];*/
static struct DSP_CONTROL snd_control;

void snes_sh_update( int param, INT16 **buffer, int length )
{
	INT16 left, right;

	while( length-- > 0 )
	{
		/* no sound for now */
		left = 0;
		right = 0;

		/* Adjust for master volume */
		left *= snd_control.vol_left;
		right *= snd_control.vol_right;

		/* Update the buffers */
		*(buffer[0]++) = left;
		*(buffer[1]++) = right;
	}
}

static UINT16 snes_dsp_r_io( UINT16 offset )
{
	switch( offset )
	{
	}
	return 0xff;
}

static void snes_dsp_w_io( UINT16 offset, UINT8 data )
{
	switch( offset )
	{
		case DSP_MVOLL:
			snd_control.vol_left = data;
			break;
		case DSP_MVOLR:
			snd_control.vol_right = data;
			break;
		case DSP_EVOLL:
			snd_control.evol_left = data;
			break;
		case DSP_EVOLR:
			snd_control.evol_right = data;
			break;
	}
}

/* --- SPC700 specific --- */

READ_HANDLER( spc_r_io )
{
	/* offset is from 0x00f0 */
	switch( offset )
	{
		case 0x2:		/* Register address */
			break;
		case 0x3:		/* Register data */
			return snes_dsp_r_io( spc_ram[0x00f2] );
		case 0x4:		/* Port 0 */
		case 0x5:		/* Port 1 */
		case 0x6:		/* Port 2 */
		case 0x7:		/* Port 3 */
#ifdef MAME_DEBUG
/*			printf( "SPC port %d read: %X\n", offset - 4, spc_port_in[offset - 4] ); */
#endif
			return spc_port_in[offset - 4];
	}
	return 0;
}

WRITE_HANDLER( spc_w_io )
{
	/* offset is from 0x00f0 */
	switch( offset )
	{
		case 0x1:		/* Control */
			if( data & 0x10 )
			{
				spc_port_in[0] = spc_port_out[0] = 0;
				spc_port_in[1] = spc_port_out[1] = 0;
			}
			if( data & 0x20 )
			{
				spc_port_in[2] = spc_port_out[2] = 0;
				spc_port_in[3] = spc_port_out[3] = 0;
			}
			spc_ram[0x00f1] = data;
			break;
		case 0x2:		/* Register address */
			spc_ram[0x00f2] = data;
			break;
		case 0x3:		/* Register data */
			snes_dsp_w_io( spc_ram[0x00f2], data );
			break;
		case 0x4:		/* Port 0 */
		case 0x5:		/* Port 1 */
		case 0x6:		/* Port 2 */
		case 0x7:		/* Port 3 */
#ifdef MAME_DEBUG
/*			printf( "SPC port %d write: %X\n", offset - 4, data ); */
#endif
			spc_port_out[offset - 4] = data;
			break;
		case 0xA:		/* Timer 0 */
		case 0xB:		/* Timer 1 */
		case 0xC:		/* Timer 2 */
		case 0xD:		/* Counter 0 */
		case 0xE:		/* Counter 1 */
		case 0xF:		/* Counter 2 */
			spc_ram[0xf0 + offset] = data;
			break;
	}
}

/* --- Fake APU stuff --- */
/* This is here until I can get the 65816 and SPC700 to stay in sync with each
 * other. */

void snes_fakeapu_w_port( UINT8 port, UINT8 data )
{
	if( port == 0 )
	{
		fakeapu_port[2]++;
		fakeapu_port[3]++;
	}

	fakeapu_port[port] = data;
}

UINT8 snes_fakeapu_r_port( UINT8 port )
{
/*  G65816_PC=1, G65816_S, G65816_P, G65816_A, G65816_X, G65816_Y,
 *  G65816_PB, G65816_DB, G65816_D, G65816_E,
 *  G65816_NMI_STATE, G65816_IRQ_STATE
 */

	static UINT8 portcount[2] = {0,0};
	UINT8 retVal = 0;

	switch( port )
	{
		case 0:
		case 1:
		{
			switch( portcount[0] )
			{
				case 0:
					retVal = fakeapu_port[port];
					break;
				case 1:
					retVal = activecpu_get_reg(4) & 0xFF;
					break;
				case 2:
					retVal = (activecpu_get_reg(4) >> 8) & 0xFF;
					break;
				case 3:
					retVal = activecpu_get_reg(5) & 0xFF;
					break;
				case 4:
					retVal = (activecpu_get_reg(5) >> 8) & 0xFF;
					break;
				case 5:
					retVal = activecpu_get_reg(6) & 0xFF;
					break;
				case 6:
					retVal = (activecpu_get_reg(6) >> 8) & 0xFF;
					break;
				case 7:
					retVal = 0xAA;
					break;
				case 8:
					retVal = 0xBB;
					break;
				case 9:
				case 10:
					retVal = rand() & 0xFF;
					break;
			}
			portcount[0]++;
			if( portcount[0] > 10 )
				portcount[0] = 0;
			return retVal;
		} break;
		case 2:
		case 3:
		{
			switch( portcount[1] )
			{
				case 0:
					retVal = fakeapu_port[port];
					break;
				case 1:
					retVal = activecpu_get_reg(4) & 0xFF;
					break;
				case 2:
					retVal = (activecpu_get_reg(4) >> 8) & 0xFF;
					break;
				case 3:
				case 4:
					retVal = rand() & 0xFF;
					break;
			}
			portcount[1]++;
			if( portcount[1] > 4 )
				portcount[1] = 0;
			return retVal;
		} break;
	}

	return fakeapu_port[port];
}
