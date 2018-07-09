/***************************************************************************

							  -= Tetris Plus 2 =-

					driver by	Luca Elia (l.elia@tin.it)


Main  CPU    :  TMP68HC000P-12

Video Chips  :	SS91022-03 9428XX001
				GS91022-04 9721PD008
				SS91022-05 9347EX002
				GS91022-05 048 9726HX002

Sound Chips  :	Yamaha YMZ280B-F

Other        :  XILINX XC5210 PQ240C X68710M AKJ9544
				XC7336 PC44ACK9633 A63458A
				NVRAM


To Do:

-	There is a 3rd unimplemented layer capable of rotation (not used by
	the game, can be tested in service mode).
-	Priority RAM is not taken into account.

Notes:

-	The Japan set doesn't seem to have (or use) NVRAM. I can't enter
	a test mode or use the service coin either !?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


UINT16 tetrisp2_systemregs[0x10];
UINT16 rocknms_sub_systemregs[0x10];

UINT16 rockn_protectdata;
UINT16 rockn_adpcmbank;
UINT16 rockn_soundvolume;

static void *rockn_timer_l4;
static void *rockn_timer_sub_l4;

/* Variables defined in vidhrdw: */

extern data16_t *tetrisp2_vram_bg, *tetrisp2_scroll_bg;
extern data16_t *tetrisp2_vram_fg, *tetrisp2_scroll_fg;
extern data16_t *tetrisp2_vram_rot, *tetrisp2_rotregs;

extern data16_t *tetrisp2_priority;

extern data16_t *rocknms_sub_vram_bg, *rocknms_sub_scroll_bg;
extern data16_t *rocknms_sub_vram_fg, *rocknms_sub_scroll_fg;
extern data16_t *rocknms_sub_vram_rot, *rocknms_sub_rotregs;

extern data16_t *rocknms_sub_priority;

/* Functions defined in vidhrdw: */

WRITE16_HANDLER( tetrisp2_palette_w );
WRITE16_HANDLER( rocknms_sub_palette_w );
READ16_HANDLER( tetrisp2_priority_r );
WRITE16_HANDLER( tetrisp2_priority_w );
WRITE16_HANDLER( rockn_priority_w );
READ16_HANDLER( rocknms_sub_priority_r );
WRITE16_HANDLER( rocknms_sub_priority_w );

WRITE16_HANDLER( tetrisp2_vram_bg_w );
WRITE16_HANDLER( tetrisp2_vram_fg_w );
WRITE16_HANDLER( tetrisp2_vram_rot_w );

WRITE16_HANDLER( rocknms_sub_vram_bg_w );
WRITE16_HANDLER( rocknms_sub_vram_fg_w );
WRITE16_HANDLER( rocknms_sub_vram_rot_w );

VIDEO_START( tetrisp2 );
VIDEO_UPDATE( tetrisp2 );
VIDEO_START( rockntread );
VIDEO_UPDATE( rockntread );
VIDEO_START( rocknms );
VIDEO_UPDATE( rocknms );


/***************************************************************************


									Sound


***************************************************************************/

static WRITE16_HANDLER( tetrisp2_systemregs_w )
{
	if (ACCESSING_LSB)
	{
		tetrisp2_systemregs[offset] = data;
	}
}

#define ROCKN_TIMER_BASE 500000

static WRITE16_HANDLER( rockn_systemregs_w )
{
	if (ACCESSING_LSB)
	{
		tetrisp2_systemregs[offset] = data;
		if (offset == 0x0c)
		{
			double timer = TIME_IN_NSEC(ROCKN_TIMER_BASE) * (4096 - data);
			timer_adjust(rockn_timer_l4, timer, 0, timer);
		}
	}
}

static WRITE16_HANDLER( rocknms_sub_systemregs_w )
{
	if (ACCESSING_LSB)
	{
		rocknms_sub_systemregs[offset] = data;
		if (offset == 0x0c)
		{
			double timer = TIME_IN_NSEC(ROCKN_TIMER_BASE) * (4096 - data);
			timer_adjust(rockn_timer_sub_l4, timer, 0, timer);
		}
	}
}

/***************************************************************************


									Sound


***************************************************************************/

static READ16_HANDLER( tetrisp2_sound_r )
{
	return YMZ280B_status_0_r(offset);
}

static WRITE16_HANDLER( tetrisp2_sound_w )
{
	if (ACCESSING_LSB)
	{
		if (offset)	YMZ280B_data_0_w     (offset, data & 0xff);
		else		YMZ280B_register_0_w (offset, data & 0xff);
	}
}

static READ16_HANDLER( rockn_adpcmbank_r )
{
	return ((rockn_adpcmbank & 0xf0ff) | (rockn_protectdata << 8));
}

static WRITE16_HANDLER( rockn_adpcmbank_w )
{
	UINT8 *SNDROM = memory_region(REGION_SOUND1);
	int bank;

	rockn_adpcmbank = data;
	bank = ((data & 0x001f) >> 2);

	if (bank > 7)
	{
		usrintf_showmessage("!!!!! ADPCM BANK OVER:%01X (%04X) !!!!!", bank, data);
		bank = 0;
	}

	memcpy(&SNDROM[0x0400000], &SNDROM[0x1000000 + (0x0c00000 * bank)], 0x0c00000);
}


static WRITE16_HANDLER( rockn2_adpcmbank_w )
{
	UINT8 *SNDROM = memory_region(REGION_SOUND1);
	int bank;

	char banktable[9][3]=
	{
		{  0,  1,  2 },		// bank $00
		{  3,  4,  5 },		// bank $04
		{  6,  7,  8 },		// bank $08
		{  9, 10, 11 },		// bank $0c
		{ 12, 13, 14 },		// bank $10
		{ 15, 16, 17 },		// bank $14
		{ 18, 19, 20 },		// bank $18
		{  0,  0,  0 },		// bank $1c
		{  0,  5, 14 },		// bank $20
	};

	rockn_adpcmbank = data;
	bank = ((data & 0x003f) >> 2);

	if (bank > 8)
	{
		usrintf_showmessage("!!!!! ADPCM BANK OVER:%01X (%04X) !!!!!", bank, data);
		bank = 0;
	}

	memcpy(&SNDROM[0x0400000], &SNDROM[0x1000000 + (0x0400000 * banktable[bank][0] )], 0x0400000);
	memcpy(&SNDROM[0x0800000], &SNDROM[0x1000000 + (0x0400000 * banktable[bank][1] )], 0x0400000);
	memcpy(&SNDROM[0x0c00000], &SNDROM[0x1000000 + (0x0400000 * banktable[bank][2] )], 0x0400000);
}

static READ16_HANDLER( rockn_soundvolume_r )
{
	return 0xffff;
}

static WRITE16_HANDLER( rockn_soundvolume_w )
{
	rockn_soundvolume = data;
}

/***************************************************************************


								Protection


***************************************************************************/

static READ16_HANDLER( tetrisp2_ip_1_word_r )
{
	return	( readinputport(1) &  0xfcff ) |
			(           rand() & ~0xfcff ) |
			(      1 << (8 + (rand()&1)) );
}


/***************************************************************************


									NVRAM


***************************************************************************/

static data16_t *tetrisp2_nvram;
static size_t tetrisp2_nvram_size;

NVRAM_HANDLER( tetrisp2 )
{
	if (read_or_write)
		mame_fwrite(file,tetrisp2_nvram,tetrisp2_nvram_size);
	else
	{
		if (file)
			mame_fread(file,tetrisp2_nvram,tetrisp2_nvram_size);
		else
		{
			/* fill in the default values */
			memset(tetrisp2_nvram,0,tetrisp2_nvram_size);
		}
	}
}


/* The game only ever writes even bytes and reads odd bytes */
READ16_HANDLER( tetrisp2_nvram_r )
{
	return	( (tetrisp2_nvram[offset] >> 8) & 0x00ff ) |
			( (tetrisp2_nvram[offset] << 8) & 0xff00 ) ;
}

READ16_HANDLER( rockn_nvram_r )
{
	return	tetrisp2_nvram[offset];
}

WRITE16_HANDLER( tetrisp2_nvram_w )
{
	COMBINE_DATA(&tetrisp2_nvram[offset]);
}



/***************************************************************************





***************************************************************************/
static UINT16 rocknms_main2sub;
static UINT16 rocknms_sub2main;

READ16_HANDLER( rocknms_main2sub_r )
{
	return rocknms_main2sub;
}

WRITE16_HANDLER( rocknms_main2sub_w )
{
	if (ACCESSING_LSB)
		rocknms_main2sub = (data ^ 0xffff);
}

READ16_HANDLER( rocknms_port_0_r )
{
	return ((readinputport(0) & 0xfffc ) | (rocknms_sub2main & 0x0003));
}

WRITE16_HANDLER( rocknms_sub2main_w )
{
	if (ACCESSING_LSB)
		rocknms_sub2main = (data ^ 0xffff);
}


WRITE16_HANDLER( tetrisp2_coincounter_w )
{
	coin_counter_w( 0, (data & 0x0001));
}


/***************************************************************************


								Memory Map


***************************************************************************/

static MEMORY_READ16_START( tetrisp2_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM				},	// ROM
	{ 0x100000, 0x103fff, MRA16_RAM				},	// Object RAM
	{ 0x104000, 0x107fff, MRA16_RAM				},	// Spare Object RAM
	{ 0x108000, 0x10ffff, MRA16_RAM				},	// Work RAM
	{ 0x200000, 0x23ffff, tetrisp2_priority_r	},	// Priority
	{ 0x300000, 0x31ffff, MRA16_RAM				},	// Palette
	{ 0x400000, 0x403fff, MRA16_RAM				},	// Foreground
	{ 0x404000, 0x407fff, MRA16_RAM				},	// Background
	{ 0x408000, 0x409fff, MRA16_RAM				},	// ???
	{ 0x500000, 0x50ffff, MRA16_RAM				},	// Line
	{ 0x600000, 0x60ffff, MRA16_RAM				},	// Rotation
	{ 0x650000, 0x651fff, MRA16_RAM				},	// Rotation (mirror)
	{ 0x800002, 0x800003, tetrisp2_sound_r		},	// Sound
	{ 0x900000, 0x903fff, tetrisp2_nvram_r	    },	// NVRAM
	{ 0x904000, 0x907fff, tetrisp2_nvram_r	    },	// NVRAM (mirror)
	{ 0xbe0000, 0xbe0001, MRA16_NOP				},	// INT-level1 dummy read
	{ 0xbe0002, 0xbe0003, input_port_0_word_r	},	// Inputs
	{ 0xbe0004, 0xbe0005, tetrisp2_ip_1_word_r	},	// Inputs & protection
	{ 0xbe0008, 0xbe0009, input_port_2_word_r	},	// Inputs
	{ 0xbe000a, 0xbe000b, watchdog_reset16_r	},	// Watchdog
MEMORY_END

static MEMORY_WRITE16_START( tetrisp2_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM									},	// ROM
	{ 0x100000, 0x103fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Object RAM
	{ 0x104000, 0x107fff, MWA16_RAM									},	// Spare Object RAM
	{ 0x108000, 0x10ffff, MWA16_RAM									},	// Work RAM
	{ 0x200000, 0x23ffff, tetrisp2_priority_w, &tetrisp2_priority	},	// Priority
	{ 0x300000, 0x31ffff, tetrisp2_palette_w, &paletteram16			},	// Palette
	{ 0x400000, 0x403fff, tetrisp2_vram_fg_w, &tetrisp2_vram_fg		},	// Foreground
	{ 0x404000, 0x407fff, tetrisp2_vram_bg_w, &tetrisp2_vram_bg		},	// Background
	{ 0x408000, 0x409fff, MWA16_RAM									},	// ???
	{ 0x500000, 0x50ffff, MWA16_RAM									},	// Line
	{ 0x600000, 0x60ffff, tetrisp2_vram_rot_w, &tetrisp2_vram_rot	},	// Rotation
	{ 0x650000, 0x651fff, tetrisp2_vram_rot_w						},	// Rotation (mirror)
	{ 0x800000, 0x800003, tetrisp2_sound_w							},	// Sound
	{ 0x900000, 0x903fff, tetrisp2_nvram_w, &tetrisp2_nvram, &tetrisp2_nvram_size	},	// NVRAM
	{ 0x904000, 0x907fff, tetrisp2_nvram_w							},	// NVRAM (mirror)
	{ 0xb00000, 0xb00001, tetrisp2_coincounter_w					},	// Coin Counter
	{ 0xb20000, 0xb20001, MWA16_NOP									},	// ???
	{ 0xb40000, 0xb4000b, MWA16_RAM, &tetrisp2_scroll_fg			},	// Foreground Scrolling
	{ 0xb40010, 0xb4001b, MWA16_RAM, &tetrisp2_scroll_bg			},	// Background Scrolling
	{ 0xb4003e, 0xb4003f, MWA16_NOP									},	// scr_size
	{ 0xb60000, 0xb6002f, MWA16_RAM, &tetrisp2_rotregs				},	// Rotation Registers
	{ 0xba0000, 0xba001f, tetrisp2_systemregs_w						},	// system param
	{ 0xba001a, 0xba001b, MWA16_NOP									},	// Lev 4 irq ack
	{ 0xba001e, 0xba001f, MWA16_NOP									},	// Lev 2 irq ack
MEMORY_END


static MEMORY_READ16_START( rockn1_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM				},	// ROM
	{ 0x100000, 0x103fff, MRA16_RAM				},	// Object RAM
	{ 0x104000, 0x107fff, MRA16_RAM				},	// Spare Object RAM
	{ 0x108000, 0x10ffff, MRA16_RAM				},	// Work RAM
	{ 0x200000, 0x23ffff, tetrisp2_priority_r	},	// Priority
	{ 0x300000, 0x31ffff, MRA16_RAM				},	// Palette
	{ 0x400000, 0x403fff, MRA16_RAM				},	// Foreground
	{ 0x404000, 0x407fff, MRA16_RAM				},	// Background
	{ 0x408000, 0x409fff, MRA16_RAM				},	// ???
	{ 0x500000, 0x50ffff, MRA16_RAM				},	// Line
	{ 0x600000, 0x60ffff, MRA16_RAM				},	// Rotation
	{ 0x900000, 0x903fff, rockn_nvram_r		    },	// NVRAM
	{ 0xa30000, 0xa30001, rockn_soundvolume_r	},	// Sound Volume
	{ 0xa40002, 0xa40003, tetrisp2_sound_r		},	// Sound
	{ 0xa44000, 0xa44001, rockn_adpcmbank_r		},	// Sound Bank
	{ 0xbe0000, 0xbe0001, MRA16_NOP				},	// INT-level1 dummy read
	{ 0xbe0002, 0xbe0003, input_port_0_word_r	},	// Inputs
	{ 0xbe0004, 0xbe0005, input_port_1_word_r	},	// Inputs
	{ 0xbe0008, 0xbe0009, input_port_2_word_r	},	// Inputs
	{ 0xbe000a, 0xbe000b, watchdog_reset16_r	},	// Watchdog
MEMORY_END

static MEMORY_WRITE16_START( rockn1_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM									},	// ROM
	{ 0x100000, 0x103fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Object RAM
	{ 0x104000, 0x107fff, MWA16_RAM									},	// Spare Object RAM
	{ 0x108000, 0x10ffff, MWA16_RAM									},	// Work RAM
	{ 0x200000, 0x23ffff, rockn_priority_w, &tetrisp2_priority	    },	// Priority
	{ 0x300000, 0x31ffff, tetrisp2_palette_w, &paletteram16			},	// Palette
	{ 0x400000, 0x403fff, tetrisp2_vram_fg_w, &tetrisp2_vram_fg		},	// Foreground
	{ 0x404000, 0x407fff, tetrisp2_vram_bg_w, &tetrisp2_vram_bg		},	// Background
	{ 0x408000, 0x409fff, MWA16_RAM									},	// ???
	{ 0x500000, 0x50ffff, MWA16_RAM									},	// Line
	{ 0x600000, 0x60ffff, tetrisp2_vram_rot_w, &tetrisp2_vram_rot	},	// Rotation
	{ 0x900000, 0x903fff, tetrisp2_nvram_w, &tetrisp2_nvram, &tetrisp2_nvram_size	},	// NVRAM
	{ 0xa30000, 0xa30001, rockn_soundvolume_w						},	// Sound Volume
	{ 0xa40000, 0xa40003, tetrisp2_sound_w							},	// Sound
	{ 0xa44000, 0xa44001, rockn_adpcmbank_w							},	// Sound Bank
	{ 0xa48000, 0xa48001, MWA16_NOP									},	// YMZ280 Reset
	{ 0xb00000, 0xb00001, tetrisp2_coincounter_w					},	// Coin Counter
	{ 0xb20000, 0xb20001, MWA16_NOP									},	// ???
	{ 0xb40000, 0xb4000b, MWA16_RAM, &tetrisp2_scroll_fg			},	// Foreground Scrolling
	{ 0xb40010, 0xb4001b, MWA16_RAM, &tetrisp2_scroll_bg			},	// Background Scrolling
	{ 0xb4003e, 0xb4003f, MWA16_NOP									},	// scr_size
	{ 0xb60000, 0xb6002f, MWA16_RAM, &tetrisp2_rotregs				},	// Rotation Registers
	{ 0xba0000, 0xba001f, rockn_systemregs_w						},	// system param
	{ 0xba001a, 0xba001b, MWA16_NOP									},	// Lev 4 irq ack
	{ 0xba001e, 0xba001f, MWA16_NOP									},	// Lev 2 irq ack
MEMORY_END


static MEMORY_READ16_START( rockn2_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM				},	// ROM
	{ 0x100000, 0x103fff, MRA16_RAM				},	// Object RAM
	{ 0x104000, 0x107fff, MRA16_RAM				},	// Spare Object RAM
	{ 0x108000, 0x10ffff, MRA16_RAM				},	// Work RAM
	{ 0x200000, 0x23ffff, tetrisp2_priority_r	},	// Priority
	{ 0x300000, 0x31ffff, MRA16_RAM				},	// Palette
	{ 0x500000, 0x50ffff, MRA16_RAM				},	// Line
	{ 0x600000, 0x60ffff, MRA16_RAM				},	// Rotation
	{ 0x800000, 0x803fff, MRA16_RAM				},	// Foreground
	{ 0x804000, 0x807fff, MRA16_RAM				},	// Background
	{ 0x808000, 0x809fff, MRA16_RAM				},	// ???
	{ 0x900000, 0x903fff, rockn_nvram_r		    },	// NVRAM
	{ 0xa30000, 0xa30001, rockn_soundvolume_r	},	// Sound Volume
	{ 0xa40002, 0xa40003, tetrisp2_sound_r		},	// Sound
	{ 0xa44000, 0xa44001, rockn_adpcmbank_r		},	// Sound Bank
	{ 0xbe0000, 0xbe0001, MRA16_NOP				},	// INT-level1 dummy read
	{ 0xbe0002, 0xbe0003, input_port_0_word_r	},	// Inputs
	{ 0xbe0004, 0xbe0005, input_port_1_word_r	},	// Inputs
	{ 0xbe0008, 0xbe0009, input_port_2_word_r	},	// Inputs
	{ 0xbe000a, 0xbe000b, watchdog_reset16_r	},	// Watchdog
MEMORY_END

static MEMORY_WRITE16_START( rockn2_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM									},	// ROM
	{ 0x100000, 0x103fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Object RAM
	{ 0x104000, 0x107fff, MWA16_RAM									},	// Spare Object RAM
	{ 0x108000, 0x10ffff, MWA16_RAM									},	// Work RAM
	{ 0x200000, 0x23ffff, rockn_priority_w, &tetrisp2_priority	    },	// Priority
	{ 0x300000, 0x31ffff, tetrisp2_palette_w, &paletteram16			},	// Palette
	{ 0x500000, 0x50ffff, MWA16_RAM									},	// Line
	{ 0x600000, 0x60ffff, tetrisp2_vram_rot_w, &tetrisp2_vram_rot	},	// Rotation
	{ 0x800000, 0x803fff, tetrisp2_vram_fg_w, &tetrisp2_vram_fg		},	// Foreground
	{ 0x804000, 0x807fff, tetrisp2_vram_bg_w, &tetrisp2_vram_bg		},	// Background
	{ 0x808000, 0x809fff, MWA16_RAM									},	// ???
	{ 0x900000, 0x903fff, tetrisp2_nvram_w, &tetrisp2_nvram, &tetrisp2_nvram_size	},	// NVRAM
	{ 0xa30000, 0xa30001, rockn_soundvolume_w						},	// Sound Volume
	{ 0xa40000, 0xa40003, tetrisp2_sound_w							},	// Sound
	{ 0xa44000, 0xa44001, rockn2_adpcmbank_w					    },	// Sound Bank
	{ 0xa48000, 0xa48001, MWA16_NOP									},	// YMZ280 Reset
	{ 0xb00000, 0xb00001, tetrisp2_coincounter_w					},	// Coin Counter
	{ 0xb20000, 0xb20001, MWA16_NOP									},	// ???
	{ 0xb40000, 0xb4000b, MWA16_RAM, &tetrisp2_scroll_fg			},	// Foreground Scrolling
	{ 0xb40010, 0xb4001b, MWA16_RAM, &tetrisp2_scroll_bg			},	// Background Scrolling
	{ 0xb4003e, 0xb4003f, MWA16_NOP									},	// scr_size
	{ 0xb60000, 0xb6002f, MWA16_RAM, &tetrisp2_rotregs				},	// Rotation Registers
	{ 0xba0000, 0xba001f, rockn_systemregs_w						},	// system param
	{ 0xba001a, 0xba001b, MWA16_NOP									},	// Lev 4 irq ack
	{ 0xba001e, 0xba001f, MWA16_NOP									},	// Lev 2 irq ack
MEMORY_END


static MEMORY_READ16_START( rocknms_main_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM				},	// ROM
	{ 0x100000, 0x103fff, MRA16_RAM				},	// Object RAM
	{ 0x104000, 0x107fff, MRA16_RAM				},	// Spare Object RAM
	{ 0x108000, 0x10ffff, MRA16_RAM				},	// Work RAM
	{ 0x200000, 0x23ffff, MRA16_RAM				},	// Priority
	{ 0x300000, 0x31ffff, MRA16_RAM				},	// Palette
//	{ 0x500000, 0x50ffff, MRA16_RAM				},	// Line
	{ 0x600000, 0x60ffff, MRA16_RAM				},	// Rotation
	{ 0x800000, 0x803fff, MRA16_RAM				},	// Foreground
	{ 0x804000, 0x807fff, MRA16_RAM				},	// Background
//	{ 0x808000, 0x809fff, MRA16_RAM				},	// ???
	{ 0x900000, 0x903fff, rockn_nvram_r	    	},	// NVRAM
	{ 0xa30000, 0xa30001, rockn_soundvolume_r	},	// Sound Volume
	{ 0xa40002, 0xa40003, tetrisp2_sound_r		},	// Sound
	{ 0xa44000, 0xa44001, rockn_adpcmbank_r		},	// Sound Bank
	{ 0xbe0000, 0xbe0001, MRA16_NOP				},	// INT-level1 dummy read
	{ 0xbe0002, 0xbe0003, rocknms_port_0_r	    },	// Inputs & MAIN <- SUB Communication
	{ 0xbe0004, 0xbe0005, input_port_1_word_r	},	// Inputs
	{ 0xbe0008, 0xbe0009, input_port_2_word_r	},	// Inputs
	{ 0xbe000a, 0xbe000b, watchdog_reset16_r	},	// Watchdog
MEMORY_END

static MEMORY_WRITE16_START( rocknms_main_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM									},	// ROM
	{ 0x100000, 0x103fff, MWA16_RAM, &spriteram16, &spriteram_size	},	// Object RAM
	{ 0x104000, 0x107fff, MWA16_RAM									},	// Spare Object RAM
	{ 0x108000, 0x10ffff, MWA16_RAM									},	// Work RAM
	{ 0x200000, 0x23ffff, rockn_priority_w, &tetrisp2_priority	    },	// Priority
	{ 0x300000, 0x31ffff, tetrisp2_palette_w, &paletteram16			},	// Palette
//	{ 0x500000, 0x50ffff, MWA16_RAM									},	// Line
	{ 0x600000, 0x60ffff, tetrisp2_vram_rot_w, &tetrisp2_vram_rot	},	// Rotation
	{ 0x800000, 0x803fff, tetrisp2_vram_fg_w, &tetrisp2_vram_fg		},	// Foreground
	{ 0x804000, 0x807fff, tetrisp2_vram_bg_w, &tetrisp2_vram_bg		},	// Background
//	{ 0x808000, 0x809fff, MWA16_RAM									},	// ???
	{ 0x900000, 0x903fff, tetrisp2_nvram_w, &tetrisp2_nvram, &tetrisp2_nvram_size	},	// NVRAM
	{ 0xa30000, 0xa30001, rockn_soundvolume_w						},	// Sound Volume
	{ 0xa40000, 0xa40003, tetrisp2_sound_w							},	// Sound
	{ 0xa44000, 0xa44001, rockn_adpcmbank_w							},	// Sound Bank
	{ 0xa48000, 0xa48001, MWA16_NOP									},	// YMZ280 Reset
	{ 0xa00000, 0xa00001, rocknms_main2sub_w	                    },	// MAIN -> SUB Communication
	{ 0xb00000, 0xb00001, tetrisp2_coincounter_w					},	// Coin Counter
	{ 0xb20000, 0xb20001, MWA16_NOP									},	// ???
	{ 0xb40000, 0xb4000b, MWA16_RAM, &tetrisp2_scroll_fg			},	// Foreground Scrolling
	{ 0xb40010, 0xb4001b, MWA16_RAM, &tetrisp2_scroll_bg			},	// Background Scrolling
	{ 0xb4003e, 0xb4003f, MWA16_NOP									},	// scr_size
	{ 0xb60000, 0xb6002f, MWA16_RAM, &tetrisp2_rotregs				},	// Rotation Registers
	{ 0xba0000, 0xba001f, rockn_systemregs_w						},	// system param
	{ 0xba001a, 0xba001b, MWA16_NOP									},	// Lev 4 irq ack
	{ 0xba001e, 0xba001f, MWA16_NOP									},	// Lev 2 irq ack
MEMORY_END


static MEMORY_READ16_START( rocknms_sub_readmem )
	{ 0x000000, 0x0fffff, MRA16_ROM					},	// ROM
	{ 0x100000, 0x103fff, MRA16_RAM					},	// Object RAM
	{ 0x104000, 0x107fff, MRA16_RAM					},	// Spare Object RAM
	{ 0x108000, 0x10ffff, MRA16_RAM					},	// Work RAM
	{ 0x200000, 0x23ffff, rocknms_sub_priority_r	},	// Priority
	{ 0x300000, 0x31ffff, MRA16_RAM					},	// Palette
//	{ 0x500000, 0x50ffff, MRA16_RAM					},	// Line
	{ 0x600000, 0x60ffff, MRA16_RAM					},	// Rotation
	{ 0x800000, 0x803fff, MRA16_RAM					},	// Foreground
	{ 0x804000, 0x807fff, MRA16_RAM					},	// Background
//	{ 0x808000, 0x809fff, MRA16_RAM					},	// ???
	{ 0x900000, 0x907fff, MRA16_RAM					},	// NVRAM
//	{ 0xbe0000, 0xbe0001, MRA16_NOP					},	// INT-level1 dummy read
	{ 0xbe0002, 0xbe0003, rocknms_main2sub_r		},	// MAIN -> SUB Communication
	{ 0xbe000a, 0xbe000b, watchdog_reset16_r		},	// Watchdog
MEMORY_END

static MEMORY_WRITE16_START( rocknms_sub_writemem )
	{ 0x000000, 0x0fffff, MWA16_ROM										},	// ROM
	{ 0x100000, 0x103fff, MWA16_RAM, &spriteram16_2, &spriteram_2_size	},	// Object RAM
	{ 0x104000, 0x107fff, MWA16_RAM										},	// Spare Object RAM
	{ 0x108000, 0x10ffff, MWA16_RAM										},	// Work RAM
	{ 0x200000, 0x23ffff, rocknms_sub_priority_w, &rocknms_sub_priority	},	// Priority
	{ 0x300000, 0x31ffff, rocknms_sub_palette_w, &paletteram16_2		},	// Palette
//	{ 0x500000, 0x50ffff, MWA16_RAM										},	// Line
	{ 0x600000, 0x60ffff, rocknms_sub_vram_rot_w, &rocknms_sub_vram_rot	},	// Rotation
	{ 0x800000, 0x803fff, rocknms_sub_vram_fg_w, &rocknms_sub_vram_fg	},	// Foreground
	{ 0x804000, 0x807fff, rocknms_sub_vram_bg_w, &rocknms_sub_vram_bg	},	// Background
//	{ 0x808000, 0x809fff, MWA16_RAM										},	// ???
	{ 0x900000, 0x907fff, MWA16_RAM										},	// NVRAM
	{ 0xb00000, 0xb00001, rocknms_sub2main_w							},	// MAIN <- SUB Communication
	{ 0xb20000, 0xb20001, MWA16_NOP										},	// ???
	{ 0xb40000, 0xb4000b, MWA16_RAM, &rocknms_sub_scroll_fg				},	// Foreground Scrolling
	{ 0xb40010, 0xb4001b, MWA16_RAM, &rocknms_sub_scroll_bg				},	// Background Scrolling
	{ 0xb4003e, 0xb4003f, MWA16_NOP										},	// scr_size
	{ 0xb60000, 0xb6002f, MWA16_RAM, &rocknms_sub_rotregs				},	// Rotation Registers
	{ 0xba0000, 0xba001f, rocknms_sub_systemregs_w						},	// system param
	{ 0xba001a, 0xba001b, MWA16_NOP										},	// Lev 4 irq ack
	{ 0xba001e, 0xba001f, MWA16_NOP										},	// Lev 2 irq ack
	{ 0xbe0002, 0xbe0003, rocknms_sub2main_w							},	// MAIN <- SUB Communication (mirror)
MEMORY_END


/***************************************************************************


								Input Ports


***************************************************************************/

/***************************************************************************
							Tetris Plus 2 (World)
***************************************************************************/

INPUT_PORTS_START( tetrisp2 )

	PORT_START	// IN0 - $be0002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	// IN1 - $be0004.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_COIN2    )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_SPECIAL  )	// ?
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_SPECIAL  )	// ?
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN2 - $be0008.w
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, "Easy" )
	PORT_DIPSETTING(      0x0300, "Normal" )
	PORT_DIPSETTING(      0x0100, "Hard" )
	PORT_DIPSETTING(      0x0200, "Hardest" )
	PORT_DIPNAME( 0x0400, 0x0400, "Vs Mode Rounds" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0400, "3" )
	PORT_DIPNAME( 0x0800, 0x0000, "Language" )
	PORT_DIPSETTING(      0x0800, "Japanese" )
	PORT_DIPSETTING(      0x0000, "English" )
	PORT_DIPNAME( 0x1000, 0x1000, "F.B.I Logo" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

INPUT_PORTS_END


/***************************************************************************
							Tetris Plus 2 (Japan)
***************************************************************************/


INPUT_PORTS_START( teplus2j )

	PORT_START	// IN0 - $be0002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )	// unused button

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )	// unused button

	PORT_START	// IN1 - $be0004.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_COIN2    )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_SPECIAL  )	// ?
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_SPECIAL  )	// ?
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN  )

/*
	The code for checking the "service mode" and "free play" DSWs
	is (deliberately?) bugged in this set
*/
	PORT_START	// IN2 - $be0008.w
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown 1-6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown 1-7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, "Easy" )
	PORT_DIPSETTING(      0x0300, "Normal" )
	PORT_DIPSETTING(      0x0100, "Hard" )
	PORT_DIPSETTING(      0x0200, "Hardest" )
	PORT_DIPNAME( 0x0400, 0x0400, "Vs Mode Rounds" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0400, "3" )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNUSED  ) // Language dip
	PORT_DIPNAME( 0x1000, 0x1000, "Unknown 2-4" )	// F.B.I. Logo (in the USA set?)
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Voice" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

INPUT_PORTS_END


/***************************************************************************
							Rock'n Tread (Japan)
***************************************************************************/


INPUT_PORTS_START( rockn1 )
	PORT_START	// IN0 - $be0002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1       | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2       | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - $be0004.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_COIN2    )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN2 - $be0008.w
	PORT_BITX(    0x0001, 0x0001, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-1", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0002, 0x0002, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-2", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0004, 0x0004, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-3", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0008, 0x0008, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-4", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0010, 0x0010, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-5", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0020, 0x0020, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-6", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0040, 0x0040, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-7", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0080, 0x0080, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-8", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_BITX(    0x0100, 0x0100, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-1", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0200, 0x0200, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-2", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0400, 0x0400, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-3", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0800, 0x0800, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-4", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x1000, 0x1000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-5", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x2000, 0x2000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-6", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x4000, 0x4000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-7", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x8000, 0x8000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-8", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( rocknms )
	PORT_START	// IN0 - $be0002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_SPECIAL )		// MAIN -> SUB Communication
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_SPECIAL )		// MAIN -> SUB Communication
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1       | IPF_PLAYER1 )
	PORT_BIT(  0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2       | IPF_PLAYER1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1       | IPF_PLAYER2 )
	PORT_BIT(  0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2       | IPF_PLAYER2 )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )

	PORT_START	// IN1 - $be0004.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BITX( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_COIN2    )

	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN2 - $be0008.w
	PORT_BITX(    0x0001, 0x0001, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-1", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0002, 0x0002, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-2", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0004, 0x0004, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-3", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0008, 0x0008, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-4", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0010, 0x0010, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-5", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0020, 0x0020, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-6", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0040, 0x0040, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-7", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0080, 0x0080, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 1-8", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_BITX(    0x0100, 0x0100, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-1", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0200, 0x0200, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-2", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0400, 0x0400, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-3", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x0800, 0x0800, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-4", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x1000, 0x1000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-5", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x2000, 0x2000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-6", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x4000, 0x4000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-7", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BITX(    0x8000, 0x8000, IPT_DIPSWITCH_NAME | IPF_CHEAT, "DIPSW 2-8", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END


/***************************************************************************


							Graphics Layouts


***************************************************************************/


/* 8x8x8 tiles */
static struct GfxLayout layout_8x8x8 =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	{ STEP8(0,8*8) },
	8*8*8
};

/* 16x16x8 tiles */
static struct GfxLayout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ STEP8(0,1) },
	{ STEP16(0,8) },
	{ STEP16(0,16*8) },
	16*16*8
};

static struct GfxDecodeInfo tetrisp2_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_8x8x8,   0x0000, 0x10 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x8, 0x1000, 0x10 }, // [1] Background
	{ REGION_GFX3, 0, &layout_16x16x8, 0x2000, 0x10 }, // [2] Rotation
	{ REGION_GFX4, 0, &layout_8x8x8,   0x6000, 0x10 }, // [3] Foreground
	{ -1 }
};

static struct GfxDecodeInfo rocknms_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_8x8x8,   0x0000, 0x10 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x8, 0x1000, 0x10 }, // [1] Background
	{ REGION_GFX3, 0, &layout_16x16x8, 0x2000, 0x10 }, // [2] Rotation
	{ REGION_GFX4, 0, &layout_8x8x8,   0x6000, 0x10 }, // [3] Foreground
	{ REGION_GFX5, 0, &layout_8x8x8,   0x8000, 0x10 }, // [0] Sprites
	{ REGION_GFX6, 0, &layout_16x16x8, 0x9000, 0x10 }, // [1] Background
	{ REGION_GFX7, 0, &layout_16x16x8, 0xa000, 0x10 }, // [2] Rotation
	{ REGION_GFX8, 0, &layout_8x8x8,   0xe000, 0x10 }, // [3] Foreground
	{ -1 }
};


/***************************************************************************


								Machine Drivers


***************************************************************************/

static struct YMZ280Binterface ymz280b_intf =
{
	1,
	{ 16934400 },
	{ REGION_SOUND1 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ 0 }	// irq
};

void rockn_timer_level4_callback(int param)
{
	cpu_set_irq_line(0, 4, HOLD_LINE);
}

void rockn_timer_sub_level4_callback(int param)
{
	cpu_set_irq_line(1, 4, HOLD_LINE);
}

void rockn_timer_level1_callback(int param)
{
	cpu_set_irq_line(0, 1, HOLD_LINE);
}

void rockn_timer_sub_level1_callback(int param)
{
	cpu_set_irq_line(1, 1, HOLD_LINE);
}

DRIVER_INIT( rockn_timer )
{
	timer_pulse(TIME_IN_MSEC(32), 0, rockn_timer_level1_callback);
	rockn_timer_l4 = timer_alloc(rockn_timer_level4_callback);
}

DRIVER_INIT( rockn1 )
{
	init_rockn_timer();
	rockn_protectdata = 1;
}

DRIVER_INIT( rockn2 )
{
	init_rockn_timer();
	rockn_protectdata = 2;
}

DRIVER_INIT( rocknms )
{
	init_rockn_timer();

	timer_pulse(TIME_IN_MSEC(32), 0, rockn_timer_sub_level1_callback);
	rockn_timer_sub_l4 = timer_alloc(rockn_timer_sub_level4_callback);

	rockn_protectdata = 3;
}

DRIVER_INIT( rockn3 )
{
	init_rockn_timer();
	rockn_protectdata = 4;
}

static MACHINE_DRIVER_START( tetrisp2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(tetrisp2_readmem,tetrisp2_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(tetrisp2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x140, 0xe0)
	MDRV_VISIBLE_AREA(0, 0x140-1, 0, 0xe0-1)
	MDRV_GFXDECODE(tetrisp2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_START(tetrisp2)
	MDRV_VIDEO_UPDATE(rockntread)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMZ280B, ymz280b_intf)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( rockn1 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(rockn1_readmem,rockn1_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(tetrisp2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x140, 0xe0)
	MDRV_VISIBLE_AREA(0, 0x140-1, 0, 0xe0-1)
	MDRV_GFXDECODE(tetrisp2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_START(rockntread)
	MDRV_VIDEO_UPDATE(rockntread)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMZ280B, ymz280b_intf)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( rockn2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(rockn2_readmem,rockn2_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(tetrisp2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(0x140, 0xe0)
	MDRV_VISIBLE_AREA(0, 0x140-1, 0, 0xe0-1)
	MDRV_GFXDECODE(tetrisp2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_START(rockntread)
	MDRV_VIDEO_UPDATE(rockntread)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMZ280B, ymz280b_intf)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( rocknms )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(rocknms_main_readmem,rocknms_main_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_MEMORY(rocknms_sub_readmem,rocknms_sub_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(tetrisp2)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_DUAL_MONITOR | VIDEO_RGB_DIRECT)
	MDRV_ASPECT_RATIO(4, 7)
	MDRV_SCREEN_SIZE(0x140, 0xe0+0x140)
	MDRV_VISIBLE_AREA(0, 0x140-1, 0, 0xe0+0x140-1)
	MDRV_GFXDECODE(rocknms_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x10000)

	MDRV_VIDEO_START(rocknms)
	MDRV_VIDEO_UPDATE(rocknms)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YMZ280B, ymz280b_intf)
MACHINE_DRIVER_END


/***************************************************************************


								ROMs Loading


***************************************************************************/


/***************************************************************************

								Tetris Plus 2

(C) Jaleco 1996

TP-97222
96019 EB-00-20117-0
MDK332V-0

BRIEF HARDWARE OVERVIEW

Toshiba TMP68HC000P-12
Yamaha YMZ280B-F
OSC: 12.000MHz, 48.000MHz, 16.9344MHz

Listing of custom chips. (Some on scan are hard to read).

IC38	JALECO SS91022-03 9428XX001
IC31	JALECO SS91022-05 9347EX002
IC32	JALECO GS91022-05    048  9726HX002
IC30	JALECO GS91022-04 9721PD008
IC39	XILINX XC5210 PQ240C X68710M AKJ9544
IC49	XILINX XC7336 PC44ACK9633 A63458A

***************************************************************************/

ROM_START( tetrisp2 )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "t2p_04.rom", 0x000000, 0x080000, CRC(e67f9c51) SHA1(d8b2937699d648267b163c7c3f591426877f3701) )
	ROM_LOAD16_BYTE( "t2p_01.rom", 0x000001, 0x080000, CRC(5020a4ed) SHA1(9c0f02fe3700761771ac026a2e375144e86e5eb7) )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD32_WORD( "96019-01.9", 0x000000, 0x400000, CRC(06f7dc64) SHA1(722c51b707b9854c0293afdff18b27ec7cae6719) )
	ROM_LOAD32_WORD( "96019-02.8", 0x000002, 0x400000, CRC(3e613bed) SHA1(038b5e43fa3d69654107c8093126eeb2e8fa4ddc) )
	/* If t2p_m01&2 from this board were correctly read, since they
	   hold the same data of the above but with swapped halves, it
	   means they had to invert the top bit of the "page select"
	   register in the sprite's hardware on this board! */

	ROM_REGION( 0x800000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD( "96019-06.13", 0x000000, 0x400000, CRC(16f7093c) SHA1(2be77c6a692c5d762f5553ae24e8c415ab194cc6) )
	ROM_LOAD( "96019-04.6",  0x400000, 0x100000, CRC(b849dec9) SHA1(fa7ac00fbe587a74c3fb8c74a0f91f7afeb8682f) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "96019-04.6",  0x000000, 0x100000, CRC(b849dec9) SHA1(fa7ac00fbe587a74c3fb8c74a0f91f7afeb8682f) )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "tetp2-10.bin", 0x000000, 0x080000, CRC(34dd1bad) SHA1(9bdf1dde11f82839676400de5dd7acb06ea8cdb2) )	// 11111xxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "96019-07.7", 0x000000, 0x400000, CRC(a8a61954) SHA1(86c3db10b348ba1f44ff696877b8b20845fa53de) )

ROM_END


/***************************************************************************

							Tetris Plus 2 (Japan)

(c)1997 Jaleco / The Tetris Company

TP-97222
96019 EB-00-20117-0

CPU:	68000-12
Sound:	YMZ280B-F
OSC:	12.000MHz
		48.0000MHz
		16.9344MHz

Custom:	SS91022-03
		GS91022-04
		GS91022-05
		SS91022-05

***************************************************************************/

ROM_START( teplus2j )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "tet2-4v2.2", 0x000000, 0x080000, CRC(5bfa32c8) SHA1(55fb2872695fcfbad13f5c0723302e72da69e44a) )	// v2.2
	ROM_LOAD16_BYTE( "tet2-1v2.2", 0x000001, 0x080000, CRC(919116d0) SHA1(3e1c0fd4c9175b2900a4717fbb9e8b591c5f534d) )

	ROM_REGION( 0x800000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD32_WORD( "96019-01.9", 0x000000, 0x400000, CRC(06f7dc64) SHA1(722c51b707b9854c0293afdff18b27ec7cae6719) )
	ROM_LOAD32_WORD( "96019-02.8", 0x000002, 0x400000, CRC(3e613bed) SHA1(038b5e43fa3d69654107c8093126eeb2e8fa4ddc) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD( "96019-06.13", 0x000000, 0x400000, CRC(16f7093c) SHA1(2be77c6a692c5d762f5553ae24e8c415ab194cc6) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "96019-04.6",  0x000000, 0x100000, CRC(b849dec9) SHA1(fa7ac00fbe587a74c3fb8c74a0f91f7afeb8682f) )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "tetp2-10.bin", 0x000000, 0x080000, CRC(34dd1bad) SHA1(9bdf1dde11f82839676400de5dd7acb06ea8cdb2) )	// 11111xxxxxxxxxxxxxx = 0xFF

	ROM_REGION( 0x400000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "96019-07.7", 0x000000, 0x400000, CRC(a8a61954) SHA1(86c3db10b348ba1f44ff696877b8b20845fa53de) )

ROM_END


/***************************************************************************

							Rock'n Tread 1 (Japan)
							Rock'n Tread 2 (Japan)
							Rock'n MegaSession (Japan)
							Rock'n 3 (Japan)
							Rock'n 4 (Japan)

(c)1997 Jaleco

CPU:	68000-12
Sound:	YMZ280B-F
OSC:	12.000MHz
		48.0000MHz
		16.9344MHz

Custom:	SS91022-03
		GS91022-04
		GS91022-05
		SS91022-05

***************************************************************************/

ROM_START( rockn1 )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD( "live.bin", 0x000000, 0x100000, BAD_DUMP CRC(ad90f2a3) SHA1(20004a68bd4580a46b8eb394be54a59eecf94158)  )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD16_WORD( "spr.img", 0x000000, 0x400000, BAD_DUMP CRC(eef37ad6) SHA1(1509d36053df98f4c68907d4b4357633d20f2d9a)  )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD16_WORD( "scr.bin", 0x000000, 0x200000, BAD_DUMP CRC(261b99a0) SHA1(7b3c768ae9d7429e2559fe32c1a4ff220d727e7e)  )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "rot.bin", 0x000000, 0x100000, BAD_DUMP CRC(5551717f) SHA1(64943a9a68ad4074f3f5128d7796e4f03baa14d5)  )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "asc.bin", 0x000000, 0x080000, BAD_DUMP CRC(918663a8) SHA1(aedacb741c986ef8159385cfef866cb7e3ef6cb6)  )

	ROM_REGION( 0x7000000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "ajrt00ma.shp", 0x0000000, 0x0400000, BAD_DUMP CRC(c354f753) SHA1(bf538c02e2162a93d8c6793a1211e21480156223)  ) // COMMON AREA
	ROM_FILL(                 0x0400000, 0x0c00000, 0xffffffff ) // BANK AREA
	ROM_LOAD( "ajrt01ma.shp", 0x1000000, 0x0400000, BAD_DUMP CRC(5b42999e) SHA1(376c773f292eae8b75db11bad3cb6ec5fe48392e)  ) // bank 0
	ROM_LOAD( "ajrt02ma.shp", 0x1400000, 0x0400000, BAD_DUMP CRC(8306f302) SHA1(8c0437d7ab8d74d4d15f4a641d30602e39cdd99d)  ) // bank 0
	ROM_LOAD( "ajrt03ma.shp", 0x1800000, 0x0400000, BAD_DUMP CRC(3fda842c) SHA1(2b9e7c548b689bab491237e36a2dcf4782a81d79)  ) // bank 0
	ROM_LOAD( "ajrt04ma.shp", 0x1c00000, 0x0400000, BAD_DUMP CRC(86d4f289) SHA1(908490ab0cf8d33cf3e127f71edee3bece70b86d)  ) // bank 1
	ROM_LOAD( "ajrt05ma.shp", 0x2000000, 0x0400000, BAD_DUMP CRC(f8dbf47d) SHA1(f19f7ae26e3b8af17a4e66e6722dd2f5c36d33f8)  ) // bank 1
	ROM_LOAD( "ajrt06ma.shp", 0x2400000, 0x0400000, BAD_DUMP CRC(525aff97) SHA1(b18e5bdf67d3a89f39c59f4f9bd3bb608dacc7f7)  ) // bank 1
	ROM_LOAD( "ajrt07ma.shp", 0x2800000, 0x0400000, BAD_DUMP CRC(5bd8bb95) SHA1(3b33c42778f7d50ca1513d37e7bc4a4efcc3cf82)  ) // bank 2
	ROM_LOAD( "ajrt08ma.shp", 0x2c00000, 0x0400000, BAD_DUMP CRC(304c1643) SHA1(0be090077e00d4b9abce2fac4821c630b6a40f22)  ) // bank 2
	ROM_LOAD( "ajrt09ma.shp", 0x3000000, 0x0400000, BAD_DUMP CRC(78c22c56) SHA1(eb48d188d25538a1d381ca760f8e98096ee12bfe)  ) // bank 2
	ROM_LOAD( "ajrt10ma.shp", 0x3400000, 0x0400000, BAD_DUMP CRC(d5e8d8a5) SHA1(df7db3c8b110ce1aa85e627537afb744c98877bd)  ) // bank 3
	ROM_LOAD( "ajrt13ma.shp", 0x3800000, 0x0400000, BAD_DUMP CRC(569ef4dd) SHA1(777f8a3aef741655555364d00a1eaa472ac4b922)  ) // bank 3
	ROM_LOAD( "ajrt14ma.shp", 0x3c00000, 0x0400000, BAD_DUMP CRC(aae8d59c) SHA1(ccca1f511ce0ea8d452f3b1d24350b5cee402ad2)  ) // bank 3
	ROM_LOAD( "ajrt15ma.shp", 0x4000000, 0x0400000, BAD_DUMP CRC(9ec1459b) SHA1(10e08a47636dec431cdb8e105cf61287fe9c6637)  ) // bank 4
	ROM_LOAD( "ajrt16ma.shp", 0x4400000, 0x0400000, BAD_DUMP CRC(b26f9a81) SHA1(0d1c8e382eb5877f9a748ff289be97cbdb73b0cc)  ) // bank 4

ROM_END


ROM_START( rockn2 )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD( "live.bin", 0x000000, 0x100000, BAD_DUMP CRC(369750fd) SHA1(d7958363d59e0e7d5fc1166ce3fb5c11b22e0aae)  )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD16_WORD( "spr.img", 0x000000, 0x3e0000, BAD_DUMP CRC(13de1054) SHA1(6efc94a0893c1748fe52bc273fd04be1628abea6)  )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD16_WORD( "scr.bin", 0x000000, 0x1a7400, BAD_DUMP CRC(9f35b2cf) SHA1(b56152101dfb7a1dea21a9179585c48e2acd1076)  )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "rot.bin", 0x000000, 0x130e00, BAD_DUMP CRC(4d9989ff) SHA1(50c5c319bdd4dc5077399f8187b79c4ed3218c14)  )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "asc.bin", 0x000000, 0x019c00, BAD_DUMP CRC(6e724e73) SHA1(0c7e8dde3abb4da72ddaa69141a4ce37bafa566f)  )

	ROM_REGION( 0x7000000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "snd25.bin", 0x0000000, 0x0400000, BAD_DUMP CRC(4e9611a3) SHA1(2a9b1d5afc0ea9a3285f9fc6b49a1c3abd8cd2a5)  ) // COMMON AREA
	ROM_FILL(              0x0400000, 0x0c00000, 0xffffffff ) 		  // BANK AREA
	ROM_LOAD( "snd01.bin", 0x1000000, 0x0400000, BAD_DUMP CRC(ec600f13) SHA1(151cb0a16782c8bba223d0f6881b80c1e43bc9bc)  ) // bank 0
	ROM_LOAD( "snd02.bin", 0x1400000, 0x0400000, BAD_DUMP CRC(8306f302) SHA1(8c0437d7ab8d74d4d15f4a641d30602e39cdd99d)  ) // bank 0
	ROM_LOAD( "snd03.bin", 0x1800000, 0x0400000, BAD_DUMP CRC(3fda842c) SHA1(2b9e7c548b689bab491237e36a2dcf4782a81d79)  ) // bank 0
	ROM_LOAD( "snd04.bin", 0x1c00000, 0x0400000, BAD_DUMP CRC(86d4f289) SHA1(908490ab0cf8d33cf3e127f71edee3bece70b86d)  ) // bank 1
	ROM_LOAD( "snd05.bin", 0x2000000, 0x0400000, BAD_DUMP CRC(f8dbf47d) SHA1(f19f7ae26e3b8af17a4e66e6722dd2f5c36d33f8)  ) // bank 1
	ROM_LOAD( "snd06.bin", 0x2400000, 0x0400000, BAD_DUMP CRC(06f7bd63) SHA1(d8b27212ebba99f5129483550aeac5b86ff2a1d2)  ) // bank 1
	ROM_LOAD( "snd07.bin", 0x2800000, 0x0400000, BAD_DUMP CRC(22f042f6) SHA1(649bdf43dd698150992e68b23fd758bca56c615b)  ) // bank 2
	ROM_LOAD( "snd08.bin", 0x2c00000, 0x0400000, BAD_DUMP CRC(dd294d8e) SHA1(49d889d341ab6167d9741340eb27902923b6cb42)  ) // bank 2
	ROM_LOAD( "snd09.bin", 0x3000000, 0x0400000, BAD_DUMP CRC(8fedee6e) SHA1(540c01d1c5f410abb1f86f33a5a532208946cb7c)  ) // bank 2
	ROM_LOAD( "snd10.bin", 0x3400000, 0x0400000, BAD_DUMP CRC(01292f11) SHA1(da88b14bf8df34e7574cf8c9f5dd385db13ab34c)  ) // bank 3
	ROM_LOAD( "snd11.bin", 0x3800000, 0x0400000, BAD_DUMP CRC(20dc76ba) SHA1(078397f2de54d4ca91035dce11419ac0d934fbfa)  ) // bank 3
	ROM_LOAD( "snd12.bin", 0x3c00000, 0x0400000, BAD_DUMP CRC(11fff0bc) SHA1(2767fcc3a5d3200750b011c97a83073719a9325f)  ) // bank 3
	ROM_LOAD( "snd13.bin", 0x4000000, 0x0400000, BAD_DUMP CRC(2367dd18) SHA1(b58f757ce4c832c5462637f4e08d7be511ca0c96)  ) // bank 4
	ROM_LOAD( "snd14.bin", 0x4400000, 0x0400000, BAD_DUMP CRC(75ced8c0) SHA1(fda17464767be073a36c117f5212411b66197dd9)  ) // bank 4
	ROM_LOAD( "snd15.bin", 0x4800000, 0x0400000, BAD_DUMP CRC(aeaca380) SHA1(1c389911aa766abec389b1c79a1542759ac58b9f)  ) // bank 4
	ROM_LOAD( "snd16.bin", 0x4c00000, 0x0400000, BAD_DUMP CRC(21d50e32) SHA1(24eaceb7c0b868b6e8fc16b403dae2427e422bf6)  ) // bank 5
	ROM_LOAD( "snd17.bin", 0x5000000, 0x0400000, BAD_DUMP CRC(de785a2a) SHA1(1f5ae46ac9476a31a431ce0f0cf124e1c8c930a6)  ) // bank 5
	ROM_LOAD( "snd18.bin", 0x5400000, 0x0400000, BAD_DUMP CRC(18cabb1e) SHA1(c769820e2e84eff0e4ce956236656ae757e3299c)  ) // bank 5
	ROM_LOAD( "snd19.bin", 0x5800000, 0x0400000, BAD_DUMP CRC(33c89e53) SHA1(7d216f5db6b30c9b05a9a77030498ff68ae6fbad)  ) // bank 6
	ROM_LOAD( "snd20.bin", 0x5c00000, 0x0400000, BAD_DUMP CRC(89c1b088) SHA1(9b4118815959a5fb65b2a293015f592a46f4126f)  ) // bank 6
	ROM_LOAD( "snd21.bin", 0x6000000, 0x0400000, BAD_DUMP CRC(13db74bd) SHA1(ab87438bbac97d46b1b8195b61dca1d72172a621)  ) // bank 6

ROM_END


ROM_START( rockn3 )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD( "live.bin", 0x000000, 0x100000, BAD_DUMP CRC(31895ef5) SHA1(064d9966dc74140bda0ab98ff73fcffc4df938a5)  )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD16_WORD( "spr.img", 0x000000, 0x400000, BAD_DUMP CRC(3fa0a3fa) SHA1(308df4201198cf1e54c6341886cc10c43b099077)  )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD16_WORD( "scr.bin", 0x000000, 0x200000, BAD_DUMP CRC(e01bf471) SHA1(4485c71770bdb8800ded4afb37814c2d287b78be)  )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "rot.bin", 0x000000, 0x200000, BAD_DUMP CRC(4e146de5) SHA1(5971cbb91da5fde652786d82d0143197518bad9b)  )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "asc.bin", 0x000000, 0x080000, BAD_DUMP CRC(8100039e) SHA1(e07b1e2f3cbcb1c086edd628d20423ecd4f74860)  )

	ROM_REGION( 0x7000000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "rt3rd001.bin", 0x0000000, 0x0400000, BAD_DUMP CRC(e2f69042) SHA1(deb361a53ed6a9033e21c2f805f327cc3e9b11c6)  ) // COMMON AREA
	ROM_FILL(                 0x0400000, 0x0c00000, 0xffffffff ) 		 // BANK AREA
	ROM_LOAD( "rt3rd002.bin", 0x1000000, 0x0400000, BAD_DUMP CRC(b328b18f) SHA1(22edebcabd6c8ed65d8c9e501621991d404c430d)  ) // bank 0
	ROM_LOAD( "rt3rd003.bin", 0x1400000, 0x0400000, BAD_DUMP CRC(f46438e3) SHA1(718f54fc0e3689f5ab29bef2ec13eb2aa9b117fc)  ) // bank 0
	ROM_LOAD( "rt3rd004.bin", 0x1800000, 0x0400000, BAD_DUMP CRC(b979e887) SHA1(10852ceb1b9e24fb87cf9339bc9fb4ae066a1221)  ) // bank 0
	ROM_LOAD( "rt3rd005.bin", 0x1c00000, 0x0400000, BAD_DUMP CRC(0bb2c212) SHA1(4f8ab3c96c3e1aa337a3fe871cffc04ec603f8c0)  ) // bank 1
	ROM_LOAD( "rt3rd006.bin", 0x2000000, 0x0400000, BAD_DUMP CRC(3116e437) SHA1(f1b06592a6f0eba92eb4511d3ca03a3bb51e8c9d)  ) // bank 1
	ROM_LOAD( "rt3rd007.bin", 0x2400000, 0x0400000, BAD_DUMP CRC(26b37ef6) SHA1(f7090f3ec81f0c651c53d460b476e63f52dd06dc)  ) // bank 1
	ROM_LOAD( "rt3rd008.bin", 0x2800000, 0x0400000, BAD_DUMP CRC(1dd3f4e3) SHA1(8474e00b962368164c717e5fe2e926852f3b4426)  ) // bank 2
	ROM_LOAD( "rt3rd009.bin", 0x2c00000, 0x0400000, BAD_DUMP CRC(a1b03d67) SHA1(95f89a37e97d62706e15fd5571ff2e70dd98fee2)  ) // bank 2
	ROM_LOAD( "rt3rd010.bin", 0x3000000, 0x0400000, BAD_DUMP CRC(35107aac) SHA1(d56a66e15c46c33cf6c9c28edf48b730b681d21a)  ) // bank 2
	ROM_LOAD( "rt3rd011.bin", 0x3400000, 0x0400000, BAD_DUMP CRC(059ec592) SHA1(205210af558eb7e8e1399b2a506ef0285c5feda3)  ) // bank 3
	ROM_LOAD( "rt3rd012.bin", 0x3800000, 0x0400000, BAD_DUMP CRC(84d4badb) SHA1(fc20f97a008f000a49e7cadd559789516643704a)  ) // bank 3
	ROM_LOAD( "rt3rd013.bin", 0x3c00000, 0x0400000, BAD_DUMP CRC(4527a9b7) SHA1(a73ebece5c84bf14f8d25bbd869b7b43b1fcd042)  ) // bank 3
	ROM_LOAD( "rt3rd014.bin", 0x4000000, 0x0400000, BAD_DUMP CRC(bfa4b7ce) SHA1(4100f2deabb8994e8e3ff897a1db13693ab64c11)  ) // bank 4
	ROM_LOAD( "rt3rd015.bin", 0x4400000, 0x0400000, BAD_DUMP CRC(a2ccd2ce) SHA1(fc6325219f7b8e68c22a129f5ec4e900e326fb9d)  ) // bank 4
	ROM_LOAD( "rt3rd016.bin", 0x4800000, 0x0400000, BAD_DUMP CRC(95baf678) SHA1(f7b39a3379f16df0560a22d4f42165ebbe05cebe)  ) // bank 4
	ROM_LOAD( "rt3rd017.bin", 0x4c00000, 0x0400000, BAD_DUMP CRC(5883c84b) SHA1(54aec4e1e2f5edc198aebc4788caf5062f9a5b6c)  ) // bank 5
	ROM_LOAD( "rt3rd018.bin", 0x5000000, 0x0400000, BAD_DUMP CRC(f92098ce) SHA1(9b13cd37ad5d7baf36b20218c4bced956084ec45)  ) // bank 5
	ROM_LOAD( "rt3rd019.bin", 0x5400000, 0x0400000, BAD_DUMP CRC(dbb2c228) SHA1(f7cd24026236e2c616376c695b9e986cc221f36d)  ) // bank 5
	ROM_LOAD( "rt3rd020.bin", 0x5800000, 0x0400000, BAD_DUMP CRC(9efdae1c) SHA1(6158a1804fbaa9ce27ae7e12cfda5f49084b4998)  ) // bank 6
	ROM_LOAD( "rt3rd021.bin", 0x5c00000, 0x0400000, BAD_DUMP CRC(5f301b83) SHA1(e24e85c43a62871360545aa42dfa439045334b79)  ) // bank 6

ROM_END


ROM_START( rockn4 )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD( "live.bin", 0x000000, 0x100000, BAD_DUMP CRC(f2c9ff1c) SHA1(0d385951bf9b04f69744938c5d6296942c2bf85e)  )

	ROM_REGION( 0x400000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD16_WORD( "spr.img", 0x000000, 0x400000, BAD_DUMP CRC(5ca2228e) SHA1(bd1463c2a8156e7f808ec57c3c01397161b6dda8)  )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD16_WORD( "scr.bin", 0x000000, 0x200000, BAD_DUMP CRC(ead41e79) SHA1(9c24b1e52b6ed43d5b5a1caf48f2974b8fa61f4a)  )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "rot.bin", 0x000000, 0x200000, BAD_DUMP CRC(eb16fc67) SHA1(5be40f2c9a5693785268eafcfcf348f147533463)  )

	ROM_REGION( 0x100000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "asc.bin", 0x000000, 0x100000, BAD_DUMP CRC(37d50259) SHA1(fd02f98a981470c47889f0b2f813ce59373a4b42)  )

	ROM_REGION( 0x7000000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "snd25.bin", 0x0000000, 0x0400000, BAD_DUMP CRC(918ea8eb) SHA1(0cd82859634635b6ce49db36fb91ed3365a101eb)  ) // COMMON AREA
	ROM_FILL(              0x0400000, 0x0c00000, 0xffffffff ) 		  // BANK AREA
	ROM_LOAD( "snd01.bin", 0x1000000, 0x0400000, BAD_DUMP CRC(c548e51e) SHA1(4fe1e35c9ed4366dce98b4f4c00f94e202ef15dc)  ) // bank 0
	ROM_LOAD( "snd02.bin", 0x1400000, 0x0400000, BAD_DUMP CRC(ffda0253) SHA1(9b8ae98accc2f72a1cd881086f89e647e4904ad9)  ) // bank 0
	ROM_LOAD( "snd03.bin", 0x1800000, 0x0400000, BAD_DUMP CRC(1f813af5) SHA1(a72d842e39b9fc955a2fc6721673b34b1b591e4a)  ) // bank 0
	ROM_LOAD( "snd04.bin", 0x1c00000, 0x0400000, BAD_DUMP CRC(035c4ff3) SHA1(9290c49244dc45ad5d6543775c5f2cc507e54e77)  ) // bank 1
	ROM_LOAD( "snd05.bin", 0x2000000, 0x0400000, BAD_DUMP CRC(0f01f7b0) SHA1(e0c6daa1606dd5aaac59a7ae75d76e937e9c0151)  ) // bank 1
	ROM_LOAD( "snd06.bin", 0x2400000, 0x0400000, BAD_DUMP CRC(31574b1c) SHA1(a08b50b4c4f2be32892b7534f2192101f8af6762)  ) // bank 1
	ROM_LOAD( "snd07.bin", 0x2800000, 0x0400000, BAD_DUMP CRC(388e2c91) SHA1(493ef760858a82cbc38de59c4db3f273c0ddfdfb)  ) // bank 2
	ROM_LOAD( "snd08.bin", 0x2c00000, 0x0400000, BAD_DUMP CRC(6e7e3f23) SHA1(4b9b959f79254d0633f1c4324b7ee6a17e222308)  ) // bank 2
	ROM_LOAD( "snd09.bin", 0x3000000, 0x0400000, BAD_DUMP CRC(39fa512f) SHA1(d07426bc74492496756b67b8ded1b507726720c7)  ) // bank 2

ROM_END


ROM_START( rocknms )

	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD( "live_m.bin", 0x000000, 0x100000, BAD_DUMP CRC(857483da) SHA1(6151e57dbe61fbcc273b9aed74e6a9253ab068a0)  )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )		/* 68000 Code */
	ROM_LOAD16_WORD( "live_s.bin", 0x000000, 0x100000, BAD_DUMP CRC(630dcb2e) SHA1(c43e0c3ffb0661d1f9d9b48904ff352ae642a742)  )

	ROM_REGION( 0x0800000, REGION_GFX1, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD32_WORD( "spr_m.im1", 0x000000, 0x400000, BAD_DUMP CRC(1caad02a) SHA1(00c3fc849d1f633874fee30f7d0caf0c62735c50)  )
	ROM_LOAD32_WORD( "spr_m.im2", 0x000002, 0x400000, BAD_DUMP CRC(520152dc) SHA1(619a55352c0dab914f6188d66272a24495b5d1d4)  )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD16_WORD( "scr_m.bin", 0x000000, 0x200000, BAD_DUMP CRC(1ca30e3f) SHA1(763c9dd287c186b6ca8ecb88c3ce29d68fea9179)  )

	ROM_REGION( 0x200000, REGION_GFX3, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "rot_m.bin", 0x000000, 0x200000, BAD_DUMP CRC(1f29b622) SHA1(aab6aafb98fa732266675daa63dc4c0d2084bcbd)  )

	ROM_REGION( 0x080000, REGION_GFX4, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "asc_m.bin", 0x000000, 0x080000, BAD_DUMP CRC(a4717579) SHA1(cf28c0f19713ebf9f8fd5d55d654c1cd2e8cd73d)  )

	ROM_REGION( 0x0800000, REGION_GFX5, ROMREGION_DISPOSE )	/* 8x8x8 (Sprites) */
	ROM_LOAD32_WORD( "spr_s.im1", 0x000000, 0x340000, BAD_DUMP CRC(eea4699c) SHA1(9cceb9dd6ae1db326fd6754a7ff20013dd163b1c)  )
	ROM_LOAD32_WORD( "spr_s.im2", 0x000002, 0x340000, BAD_DUMP CRC(2c02a1ec) SHA1(77daaa644c54353ef8f58d47bde49af0424c9c88)  )

	ROM_REGION( 0x200000, REGION_GFX6, ROMREGION_DISPOSE )	/* 16x16x8 (Background) */
	ROM_LOAD16_WORD( "scr_s.bin", 0x000000, 0x0ac900, BAD_DUMP CRC(a00dfdaa) SHA1(37543516f00c9aea88a6a7ebd3bb777c69796b9c)  )

	ROM_REGION( 0x200000, REGION_GFX7, ROMREGION_DISPOSE )	/* 16x16x8 (Rotation) */
	ROM_LOAD( "rot_s.bin", 0x000000,  0x148300, BAD_DUMP CRC(c76eff18) SHA1(0b38b5ad6a45fd237aefa4a070815e6698deb31b)  )

	ROM_REGION( 0x080000, REGION_GFX8, ROMREGION_DISPOSE )	/* 8x8x8 (Foreground) */
	ROM_LOAD( "asc_s.bin", 0x000000, 0x02c700, BAD_DUMP CRC(26a8bcaa) SHA1(7de2815c772204937a3e634f93a27933bfce12cd)  )

	ROM_REGION( 0x7000000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "rtdx001.bin", 0x0000000, 0x0400000, BAD_DUMP CRC(8bafae71) SHA1(db74accd4bc1bfeb4a3341a0fd572b81287f1278)  ) // COMMON AREA
	ROM_FILL(                0x0400000, 0x0c00000, 0xffffffff ) 		// BANK AREA
	ROM_LOAD( "rtdx002.bin", 0x1000000, 0x0400000, BAD_DUMP CRC(eec0589b) SHA1(f54c1c7e7741100a1398ebd45aef4755171d9965)  ) // bank 0
	ROM_LOAD( "rtdx003.bin", 0x1400000, 0x0400000, BAD_DUMP CRC(564aa972) SHA1(b19e960fd79647e5bcca509982c9887decb92bc6)  ) // bank 0
	ROM_LOAD( "rtdx004.bin", 0x1800000, 0x0400000, BAD_DUMP CRC(940302d0) SHA1(b28c2bb1a9b8cea0b6963ffa5d3ac26d90b0bffc)  ) // bank 0
	ROM_LOAD( "rtdx005.bin", 0x1c00000, 0x0400000, BAD_DUMP CRC(766db7f8) SHA1(41cfcac2e8d4307f75c56d57431b841e6d64b23c)  ) // bank 1
	ROM_LOAD( "rtdx006.bin", 0x2000000, 0x0400000, BAD_DUMP CRC(3a3002f9) SHA1(27b24b8a34a0b919e051e81a10e87aa300b11d8f)  ) // bank 1
	ROM_LOAD( "rtdx007.bin", 0x2400000, 0x0400000, BAD_DUMP CRC(06b04df9) SHA1(4bfc7c05843b4533f238f5360230cb71d7a66d56)  ) // bank 1
	ROM_LOAD( "rtdx008.bin", 0x2800000, 0x0400000, BAD_DUMP CRC(da74305e) SHA1(9dfb744f36ac8b3661006921dc482e941711f389)  ) // bank 2
	ROM_LOAD( "rtdx009.bin", 0x2c00000, 0x0400000, BAD_DUMP CRC(b5a0aa48) SHA1(2deb2c1c97c259f5e79e9dc3cd8859548549a189)  ) // bank 2
	ROM_LOAD( "rtdx010.bin", 0x3000000, 0x0400000, BAD_DUMP CRC(0fd4a088) SHA1(5c1ea8a14dee7ee885ce0c86fb463741599db44d)  ) // bank 2
	ROM_LOAD( "rtdx011.bin", 0x3400000, 0x0400000, BAD_DUMP CRC(33c89e53) SHA1(7d216f5db6b30c9b05a9a77030498ff68ae6fbad)  ) // bank 3
	ROM_LOAD( "rtdx012.bin", 0x3800000, 0x0400000, BAD_DUMP CRC(f9256a3f) SHA1(a3ec0845497d349c97222a1f986c252c8ca781e7)  ) // bank 3
	ROM_LOAD( "rtdx013.bin", 0x3c00000, 0x0400000, BAD_DUMP CRC(b0a09f3e) SHA1(d2e37eb935d7ef7e887ff79a49bc11da11c31f3c)  ) // bank 3
	ROM_LOAD( "rtdx014.bin", 0x4000000, 0x0400000, BAD_DUMP CRC(d5cee673) SHA1(85194c73c43b69bccbcc895f147d5251bb039c2a)  ) // bank 4
	ROM_LOAD( "rtdx015.bin", 0x4400000, 0x0400000, BAD_DUMP CRC(b394aa8a) SHA1(68541d5d98e2d59d6a3096f0c10b74b6f5803722)  ) // bank 4
	ROM_LOAD( "rtdx016.bin", 0x4800000, 0x0400000, BAD_DUMP CRC(6c791501) SHA1(8c67f070651493d6f7a2ef7b8a5f9e12c0181f67)  ) // bank 4
	ROM_LOAD( "rtdx017.bin", 0x4c00000, 0x0400000, BAD_DUMP CRC(fe80159e) SHA1(b6a980d4f62dfeaa6f51a99518aa4d483fe338e5)  ) // bank 5
	ROM_LOAD( "rtdx018.bin", 0x5000000, 0x0400000, BAD_DUMP CRC(142c1159) SHA1(dfabbe69119c84040d6368561e93514ce7bb91db)  ) // bank 5
	ROM_LOAD( "rtdx019.bin", 0x5400000, 0x0400000, BAD_DUMP CRC(cc595d85) SHA1(5f725771d79e71d62b64bb18e2a51b839a6e4c7f)  ) // bank 5
	ROM_LOAD( "rtdx020.bin", 0x5800000, 0x0400000, BAD_DUMP CRC(82b085a3) SHA1(5a5f2ed90d659bbad710c23b9df2a7dbb3c9acfe)  ) // bank 6
	ROM_LOAD( "rtdx021.bin", 0x5c00000, 0x0400000, BAD_DUMP CRC(dd5e9680) SHA1(5a2826641ad75757ce4a583e0ea901d54d20ffca)  ) // bank 6

ROM_END


/***************************************************************************/

#define ROCKNMS_MONITOR 0

#if (ROCKNMS_MONITOR == 1)
static MACHINE_DRIVER_START( rocknmt1 )
	MDRV_IMPORT_FROM(rocknms)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_ASPECT_RATIO(4, 3)
	MDRV_SCREEN_SIZE(0x140, 0xe0)
	MDRV_VISIBLE_AREA(0, 0x140-1, 0, 0xe0-1)
	MDRV_GFXDECODE(tetrisp2_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x8000)
	MDRV_VIDEO_UPDATE(rockntread)
MACHINE_DRIVER_END
#endif

#if (ROCKNMS_MONITOR == 2)
VIDEO_UPDATE( rocknmt2 );
static MACHINE_DRIVER_START( rocknmt2 )
	MDRV_IMPORT_FROM(rocknms)
	MDRV_ASPECT_RATIO(7, 4)
	MDRV_SCREEN_SIZE(0x140+0xe0, 0x140)
	MDRV_VISIBLE_AREA(0, 0x140+0xe0-1, 0, 0x140-1)
	MDRV_VIDEO_UPDATE(rocknmt2)
MACHINE_DRIVER_END
#endif


/***************************************************************************


								Game Drivers


***************************************************************************/

GAME( 1997, tetrisp2, 0,        tetrisp2, tetrisp2, 0,       ROT0,   "Jaleco / The Tetris Company", "Tetris Plus 2 (World?)" )
GAME( 1997, teplus2j, tetrisp2, tetrisp2, teplus2j, 0,       ROT0,   "Jaleco / The Tetris Company", "Tetris Plus 2 (Japan)" )

GAME( 1999, rockn1,   0,        rockn1,   rockn1,   rockn1,  ROT270, "Jaleco", "Rock'n Tread 1 (Japan)" )
GAME( 1999, rockn2,   0,        rockn2,   rockn1,   rockn2,  ROT270, "Jaleco", "Rock'n Tread 2 (Japan)" )

#if (ROCKNMS_MONITOR == 0)
GAMEX(1999, rocknms,  0,        rocknms,  rocknms,  rocknms, ROT0,   "Jaleco", "Rock'n MegaSession (Japan)", GAME_IMPERFECT_GRAPHICS )
#endif
#if (ROCKNMS_MONITOR == 1)
GAMEX(1999, rocknms,  0,        rocknmt1, rocknms,  rocknms, ROT270, "Jaleco", "Rock'n MegaSession (Japan)", GAME_IMPERFECT_GRAPHICS )
#endif
#if (ROCKNMS_MONITOR == 2)
GAMEX(1999, rocknms,  0,        rocknmt2, rocknms,  rocknms, ROT0,   "Jaleco", "Rock'n MegaSession (Japan)", GAME_IMPERFECT_GRAPHICS )
#endif

GAME( 1999, rockn3,   0,        rockn2,   rockn1,   rockn3,  ROT270, "Jaleco", "Rock'n 3 (Japan)" )
GAME( 2000, rockn4,   0,        rockn2,   rockn1,   rockn3,  ROT270, "Jaleco (PCCWJ)", "Rock'n 4 (Japan prototype version)" )
