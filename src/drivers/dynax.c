/***************************************************************************

Some Dynax games using the second version of their blitter

driver by Luca Elia and Nicola Salmoria

CPU:    Z80
Sound:  various
VDP:    HD46505SP (6845) (CRT controller)
Custom: TC17G032AP-0246 (blitter)

----------------------------------------------------------------------------------------
Year + Game					Board			Sound						Palette
----------------------------------------------------------------------------------------
88 Hana no Mai				D1610088L1		AY8912 YM2203        M5205	PROM
88 Hana Kochou				D201901L		AY8912 YM2203        M5205	PROM
89 Hana Oriduru				D2304268L		AY8912        YM2413 M5205	RAM     (1)
89 Dragon Punch				D24?		 	       YM2203				PROM
89 Mahjong Friday			D2607198L1		              YM2413		PROM
89 Jantouki					D2711078L-B		**TODO** (3)
89 Sports Match				D31?		 	       YM2203				PROM
90 Mahjong Campus Hunting	D3312108L1-1	**TODO** (2)
90 7jigen no Youseitachi	D3707198L1		**TODO** (2)
91 Mahjong Dial Q2			D5212298L-1		              YM2413		PROM
94 Maya						same as sprtmtch?	     YM2203				PROM
9? The Return of Lady Frog	?					     YM2203				?
----------------------------------------------------------------------------------------

(1) partial support, major gfx problems
(2) uses D23 sub board
(3) quite different from the others: it has a slave Z80 and *two* blitters

Notes:
- In some games (drgpunch etc) there's a more complete service mode. To enter
  it, set the service mode dip switch and reset keeping start1 pressed.
  In hnkochou, keep F2 pressed and reset.

- sprtmtch and drgpunch are "clones", but the gfx are very different; sprtmtch
  is a trimmed down version, without all animations between levels.

- according to the readme, mjfriday should have a M5205. However there don't seem to be
  accesses to it, and looking at the ROMs I don't see ADPCM data. Note that apart from a
  minor difference in the memory map mjfriday and mjdialq2 are identical, and mjdialq2
  doesn't have a 5205 either. Therefore, I think it's either a mistake in the readme or
  the chip is on the board but unused.

TODO:
- Inputs are grossly mapped, especially for the card games.

- mjdialq2: the title screen is corrupted by the scrolling logo. This would be
  fixed by not wrapping around when drawing bìpast the bottom of the bitmap,
  but doing so would break the last picture of gal 6 (which is scrolled up so
  that the bottom of the bitmap is near the middle of the screen).

- In the interleaved games, "reverse write" test in service mode is wrong due to
  interleaving. Correct behaviour? None of these games has a "flip screen" dip
  switch, while mjfriday and mjdialq2, which aren't interleaved, have it.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/dynax.h"


/***************************************************************************


								Interrupts


***************************************************************************/

/***************************************************************************
								Sports Match
***************************************************************************/

UINT8 dynax_blitter_irq;
UINT8 dynax_sound_irq;
UINT8 dynax_vblank_irq;

/* It runs in IM 0, thus needs an opcode on the data bus */
void sprtmtch_update_irq(void)
{
	int irq	=	((dynax_sound_irq)   ? 0x08 : 0) |
				((dynax_vblank_irq)  ? 0x10 : 0) |
				((dynax_blitter_irq) ? 0x20 : 0) ;
	cpu_set_irq_line_and_vector(0, 0, irq ? ASSERT_LINE : CLEAR_LINE, 0xc7 | irq); /* rst $xx */
}

WRITE_HANDLER( dynax_vblank_ack_w )
{
	dynax_vblank_irq = 0;
	sprtmtch_update_irq();
}

WRITE_HANDLER( dynax_blitter_ack_w )
{
	dynax_blitter_irq = 0;
	sprtmtch_update_irq();
}

INTERRUPT_GEN( sprtmtch_vblank_interrupt )
{
	dynax_vblank_irq = 1;
	sprtmtch_update_irq();
}

void sprtmtch_sound_callback(int state)
{
	dynax_sound_irq = state;
	sprtmtch_update_irq();
}

/***************************************************************************


								Memory Maps


***************************************************************************/

/***************************************************************************
								Sports Match
***************************************************************************/

static WRITE_HANDLER( dynax_coincounter_0_w )
{
	coin_counter_w(0, data & 1);
	if (data & ~1)
		logerror("CPU#0 PC %06X: Warning, coin counter 0 <- %02X\n", activecpu_get_pc(), data);
}
static WRITE_HANDLER( dynax_coincounter_1_w )
{
	coin_counter_w(1, data & 1);
	if (data & ~1)
		logerror("CPU#0 PC %06X: Warning, coin counter 1 <- %02X\n", activecpu_get_pc(), data);
}

static READ_HANDLER( ret_ff )	{	return 0xff;	}


static int keyb;

static READ_HANDLER( hanami_keyboard_0_r )
{
	int res = 0x3f;

	/* the game reads all rows at once (keyb = 0) to check if a key is pressed */
	if (~keyb & 0x01) res &= readinputport(3);
	if (~keyb & 0x02) res &= readinputport(4);
	if (~keyb & 0x04) res &= readinputport(5);
	if (~keyb & 0x08) res &= readinputport(6);
	if (~keyb & 0x10) res &= readinputport(7);

	return res;
}

static READ_HANDLER( hanami_keyboard_1_r )
{
	return 0x3f;
}

static WRITE_HANDLER( hanami_keyboard_w )
{
	keyb = data;
}


static WRITE_HANDLER( dynax_rombank_w )
{
	data8_t *ROM = memory_region(REGION_CPU1);
	cpu_setbank(1,&ROM[0x08000+0x8000*(data & 0x0f)]);
}


static int hnoridur_bank;

static WRITE_HANDLER( hnoridur_rombank_w )
{
	data8_t *ROM = memory_region(REGION_CPU1);
//logerror("%04x: rom bank = %02x\n",activecpu_get_pc(),data);
	cpu_setbank(1,&ROM[0x8000*(data & 0x0f)]);
	hnoridur_bank = data;
}

static data8_t palette_ram[16*256*2];
static int palbank;

static WRITE_HANDLER( hnoridur_palbank_w )
{
	palbank = data & 0x0f;
}

static WRITE_HANDLER( hnoridur_palette_w )
{
	switch (hnoridur_bank)
	{
		case 0x10:
			palette_ram[256*palbank + offset + 16*256] = data;
			break;

		case 0x14:
			palette_ram[256*palbank + offset] = data;
			break;

		default:
			usrintf_showmessage("palette_w with bank = %02x",hnoridur_bank);
			break;
	}

	{
		int x =	(palette_ram[256*palbank + offset]<<8) + palette_ram[256*palbank + offset + 16*256];
		/* The bits are in reverse order! */
		int r = BITSWAP8((x >>  0) & 0x1f, 7,6,5, 0,1,2,3,4 );
		int g = BITSWAP8((x >>  5) & 0x1f, 7,6,5, 0,1,2,3,4 );
		int b = BITSWAP8((x >> 10) & 0x1f, 7,6,5, 0,1,2,3,4 );
		r =  (r << 3) | (r >> 2);
		g =  (g << 3) | (g >> 2);
		b =  (b << 3) | (b >> 2);
		palette_set_color(256*palbank + offset,r,g,b);
	}
}


static int msm5205next;
static int resetkludge;

static void adpcm_int(int data)
{
	static int toggle;

	MSM5205_data_w(0,msm5205next >> 4);
	msm5205next<<=4;

	toggle = 1 - toggle;
	if (toggle)
	{
		if (resetkludge)	// don't know what's wrong, but NMIs when the 5205 is reset make the game crash
		cpu_set_nmi_line(0,PULSE_LINE);
	}
}

static WRITE_HANDLER( adpcm_data_w )
{
	msm5205next = data;
}

static WRITE_HANDLER( adpcm_reset_w )
{
	resetkludge = data & 1;
	MSM5205_reset_w(0,~data & 1);
}

static MACHINE_INIT( adpcm )
{
	/* start with the MSM5205 reset */
	resetkludge = 0;
	MSM5205_reset_w(0,1);
}



static MEMORY_READ_START( sprtmtch_readmem )
	{ 0x0000, 0x6fff, MRA_ROM },
	{ 0x7000, 0x7fff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_BANK1 },
MEMORY_END

static MEMORY_WRITE_START( sprtmtch_writemem )
	{ 0x0000, 0x6fff, MWA_ROM },
	{ 0x7000, 0x7fff, MWA_RAM },
	{ 0x7000, 0x7fff, MWA_RAM, &generic_nvram, &generic_nvram_size },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( hnoridur_readmem )
	{ 0x0000, 0x6fff, MRA_ROM },
	{ 0x7000, 0x7fff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_BANK1 },
MEMORY_END

static MEMORY_WRITE_START( hnoridur_writemem )
	{ 0x0000, 0x6fff, MWA_ROM },
	{ 0x7000, 0x7fff, MWA_RAM, &generic_nvram, &generic_nvram_size },
	{ 0x8000, 0x80ff, hnoridur_palette_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( mjdialq2_readmem )
	{ 0x0000, 0x07ff, MRA_ROM },
	{ 0x0800, 0x1fff, MRA_RAM },
	{ 0x2000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xffff, MRA_BANK1 },
MEMORY_END

static MEMORY_WRITE_START( mjdialq2_writemem )
	{ 0x0000, 0x07ff, MWA_ROM },
	{ 0x0800, 0x0fff, MWA_RAM },
	{ 0x1000, 0x1fff, MWA_RAM, &generic_nvram, &generic_nvram_size },
	{ 0x2000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END



static PORT_READ_START( hanamai_readport )
	{ 0x78, 0x78, YM2203_status_port_0_r	},	// YM2203
	{ 0x79, 0x79, YM2203_read_port_0_r		},	// 2 x DSW
	{ 0x60, 0x60, hanami_keyboard_0_r		},	// P1
	{ 0x61, 0x61, hanami_keyboard_1_r		},	// P2
	{ 0x62, 0x62, input_port_2_r			},	// Coins
	{ 0x63, 0x63, ret_ff					},	// ?
PORT_END

static PORT_WRITE_START( hanamai_writeport )
	{ 0x41, 0x47, dynax_blitter_rev2_w		},	// Blitter
	{ 0x78, 0x78, YM2203_control_port_0_w	},	// YM2203
	{ 0x79, 0x79, YM2203_write_port_0_w		},	//
	{ 0x7a, 0x7a, AY8910_control_port_0_w	},	// AY8910
	{ 0x7b, 0x7b, AY8910_write_port_0_w		},	//
//	{ 0x7c, 0x7c, IOWP_NOP					},	// CRT Controller
//	{ 0x7d, 0x7d, IOWP_NOP					},	// CRT Controller
	{ 0x68, 0x68, dynax_layer_enable_w		},	// Layers Enable
	{ 0x65, 0x65, dynax_rombank_w		},	// BANK ROM Select  hanamai only
	{ 0x50, 0x50, dynax_rombank_w		},	// BANK ROM Select	hnkochou only
	{ 0x6a, 0x6a, dynax_blit_dest_w			},	// Destination Layer
	{ 0x6b, 0x6b, dynax_blit_pen_w			},	// Destination Pen
	{ 0x6c, 0x6c, dynax_blit_palette01_w	},	// Layers Palettes (Low Bits)
	{ 0x6d, 0x6d, dynax_blit_palette2_w		},	//
	{ 0x6e, 0x6e, dynax_blit_backpen_w		},	// Background Color
	{ 0x66, 0x66, dynax_vblank_ack_w		},	// VBlank IRQ Ack
	{ 0x70, 0x70, adpcm_reset_w				},	// MSM5205 reset
	{ 0x71, 0x71, dynax_flipscreen_w		},	// Flip Screen
	{ 0x72, 0x72, dynax_coincounter_0_w	},	// Coin Counters
	{ 0x73, 0x73, dynax_coincounter_1_w	},	//
	{ 0x74, 0x74, dynax_blitter_ack_w	},	// Blitter IRQ Ack
	{ 0x76, 0x76, dynax_blit_palbank_w		},	// Layers Palettes (High Bit)

	{ 0x00, 0x00, dynax_extra_scrollx_w	},	// screen scroll X
	{ 0x20, 0x20, dynax_extra_scrolly_w	},	// screen scroll Y
	{ 0x64, 0x64, hanami_keyboard_w			},	// keyboard row select
	{ 0x67, 0x67, adpcm_data_w				},	// MSM5205 data
	{ 0x77, 0x77, hanamai_layer_dest_w		},	// half of the interleaved layer to write to
	{ 0x69, 0x69, hanamai_priority_w		},	// layer priority
PORT_END



static PORT_READ_START( hnoridur_readport )
	{ 0x36, 0x36, AY8910_read_port_0_r		},	// AY8910, DSW1
	{ 0x24, 0x24, input_port_1_r			},	// DSW2
	{ 0x26, 0x26, input_port_8_r			},	// DSW3
	{ 0x25, 0x25, input_port_9_r			},	// DSW4
	{ 0x23, 0x23, hanami_keyboard_0_r		},	// P1
	{ 0x22, 0x22, hanami_keyboard_1_r		},	// P2
	{ 0x21, 0x21, input_port_2_r			},	// Coins
	{ 0x57, 0x57, ret_ff					},	// ?
PORT_END

static PORT_WRITE_START( hnoridur_writeport )
	{ 0x01, 0x07, dynax_blitter_rev2_w		},	// Blitter
	{ 0x34, 0x34, YM2413_register_port_0_w },
	{ 0x35, 0x35, YM2413_data_port_0_w },
	{ 0x3a, 0x3a, AY8910_control_port_0_w	},	// AY8910
	{ 0x38, 0x38, AY8910_write_port_0_w		},	//
//	{ 0x10, 0x10, IOWP_NOP					},	// CRT Controller
//	{ 0x11, 0x11, IOWP_NOP					},	// CRT Controller
	{ 0x44, 0x44, hanamai_priority_w		},	// layer priority and enable
	{ 0x54, 0x54, hnoridur_rombank_w		},	// BANK ROM Select
	{ 0x41, 0x41, dynax_blit_dest_w			},	// Destination Layer
	{ 0x40, 0x40, dynax_blit_pen_w			},	// Destination Pen
//	{ 0x6c, 0x6c, dynax_blit_palette01_w	},	// Layers Palettes (Low Bits)
//	{ 0x6d, 0x6d, dynax_blit_palette2_w		},	//
//	{ 0x6e, 0x6e, dynax_blit_backpen_w		},	// Background Color
	{ 0x56, 0x56, dynax_vblank_ack_w		},	// VBlank IRQ Ack
	{ 0x30, 0x30, adpcm_reset_w				},	// MSM5205 reset
	{ 0x60, 0x60, dynax_flipscreen_w		},	// Flip Screen
	{ 0x70, 0x70, dynax_coincounter_0_w	},	// Coin Counters
	{ 0x71, 0x71, dynax_coincounter_1_w	},	//
	{ 0x67, 0x67, dynax_blitter_ack_w	},	// Blitter IRQ Ack
//	{ 0x76, 0x76, dynax_blit_palbank_w		},	// Layers Palettes (High Bit)

	{ 0x50, 0x50, dynax_extra_scrollx_w	},	// screen scroll X
	{ 0x51, 0x51, dynax_extra_scrolly_w	},	// screen scroll Y
	{ 0x20, 0x20, hanami_keyboard_w			},	// keyboard row select
	{ 0x32, 0x32, adpcm_data_w				},	// MSM5205 data
	{ 0x61, 0x61, hanamai_layer_dest_w		},	// half of the interleaved layer to write to
	{ 0x47, 0x47, hnoridur_palbank_w		},
{ 0x55, 0x55, MWA_NOP },
PORT_END



static PORT_READ_START( sprtmtch_readport )
	{ 0x10, 0x10, YM2203_status_port_0_r	},	// YM2203
	{ 0x11, 0x11, YM2203_read_port_0_r		},	// 2 x DSW
	{ 0x20, 0x20, input_port_0_r			},	// P1
	{ 0x21, 0x21, input_port_1_r			},	// P2
	{ 0x22, 0x22, input_port_2_r			},	// Coins
	{ 0x23, 0x23, ret_ff					},	// ?
PORT_END

static PORT_WRITE_START( sprtmtch_writeport )
	{ 0x01, 0x07, dynax_blitter_rev2_w		},	// Blitter
	{ 0x10, 0x10, YM2203_control_port_0_w	},	// YM2203
	{ 0x11, 0x11, YM2203_write_port_0_w		},	//
//	{ 0x12, 0x12, IOWP_NOP					},	// CRT Controller
//	{ 0x13, 0x13, IOWP_NOP					},	// CRT Controller
	{ 0x30, 0x30, dynax_layer_enable_w		},	// Layers Enable
	{ 0x31, 0x31, dynax_rombank_w		},	// BANK ROM Select
	{ 0x32, 0x32, dynax_blit_dest_w			},	// Destination Layer
	{ 0x33, 0x33, dynax_blit_pen_w			},	// Destination Pen
	{ 0x34, 0x34, dynax_blit_palette01_w	},	// Layers Palettes (Low Bits)
	{ 0x35, 0x35, dynax_blit_palette2_w		},	//
	{ 0x36, 0x36, dynax_blit_backpen_w		},	// Background Color
	{ 0x37, 0x37, dynax_vblank_ack_w		},	// VBlank IRQ Ack
//	{ 0x40, 0x40, adpcm_reset_w				},	// MSM5205 reset
	{ 0x41, 0x41, dynax_flipscreen_w		},	// Flip Screen
	{ 0x42, 0x42, dynax_coincounter_0_w	},	// Coin Counters
	{ 0x43, 0x43, dynax_coincounter_1_w	},	//
	{ 0x44, 0x44, dynax_blitter_ack_w	},	// Blitter IRQ Ack
	{ 0x45, 0x45, dynax_blit_palbank_w		},	// Layers Palettes (High Bit)
PORT_END



static PORT_READ_START( mjfriday_readport )
	{ 0x63, 0x63, hanami_keyboard_0_r		},	// P1
	{ 0x62, 0x62, hanami_keyboard_1_r		},	// P2
	{ 0x61, 0x61, input_port_2_r			},	// Coins
	{ 0x64, 0x64, input_port_0_r			},	// DSW
	{ 0x67, 0x67, input_port_1_r			},	// DSW
PORT_END

static PORT_WRITE_START( mjfriday_writeport )
	{ 0x41, 0x47, dynax_blitter_rev2_w		},	// Blitter
//	{ 0x50, 0x50, IOWP_NOP					},	// CRT Controller
//	{ 0x51, 0x51, IOWP_NOP					},	// CRT Controller
	{ 0x60, 0x60, hanami_keyboard_w			},	// keyboard row select
	{ 0x70, 0x70, YM2413_register_port_0_w },
	{ 0x71, 0x71, YM2413_data_port_0_w },
	{ 0x00, 0x00, dynax_blit_pen_w			},	// Destination Pen
	{ 0x01, 0x01, dynax_blit_palette01_w	},	// Layers Palettes (Low Bits)
	{ 0x02, 0x02, dynax_rombank_w		},	// BANK ROM Select
	{ 0x03, 0x03, dynax_blit_backpen_w		},	// Background Color
	{ 0x10, 0x11, mjdialq2_blit_dest_w		},	// Destination Layer
	{ 0x12, 0x12, dynax_blit_palbank_w		},	// Layers Palettes (High Bit)
	{ 0x13, 0x13, dynax_flipscreen_w		},	// Flip Screen
	{ 0x14, 0x14, dynax_coincounter_0_w	},	// Coin Counters
	{ 0x15, 0x15, dynax_coincounter_1_w	},	//
	{ 0x16, 0x17, mjdialq2_layer_enable_w	},	// Layers Enable
//	{ 0x80, 0x80, IOWP_NOP					},	// IRQ ack?
PORT_END


/***************************************************************************

Lady Frog, or is it Dragon Punch, or is Lady Frog the name of a bootleg
Dragon Punch?

The program jumps straight away to an unmapped memory address. I don't know,
maybe there's a ROM missing.

ladyfrog.001 contains
VIDEO COMPUTER SYSTEM  (C)1989 DYNAX INC  NAGOYA JAPAN  DRAGON PUNCH  VER. 1.30

***************************************************************************/

static MEMORY_READ16_START( ladyfrog_readmem )
	{ 0x000000, 0x3fffff, MRA16_ROM },
	{ 0x881800, 0x881fff, MRA16_RAM },
	{ 0x840000, 0x840001, input_port_0_word_r },
	{ 0x840002, 0x840003, input_port_1_word_r },
	{ 0x840004, 0x840005, input_port_2_word_r },
	{ 0x840006, 0x840007, input_port_3_word_r },
	{ 0xffc000, 0xffffff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( ladyfrog_writemem )
	{ 0x000000, 0x3fffff, MWA16_ROM },
	{ 0x800000, 0x83ffff, MWA16_RAM },
	{ 0x881800, 0x881fff, MWA16_RAM },
	{ 0xffc000, 0xffffff, MWA16_RAM },
MEMORY_END


static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	8*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &charlayout,  0, 1 },
	{ -1 } /* end of array */
};

/***************************************************************************


								Input Ports


***************************************************************************/

INPUT_PORTS_START( hanamai )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE )	// Test
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )	// Analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE3 )	// Memory Reset
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1                              )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "Bet",   KEYCODE_RCONTROL, IP_JOY_NONE )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "C",   KEYCODE_C,     IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "G",   KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "K",   KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z,     IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Flip",   KEYCODE_X,        IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( hnkochou )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )		// Pay
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )	// Test (there isn't a dip switch)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )	// Analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE3 )	// Memory Reset
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )		// Note
	PORT_BIT_IMPULSE( 0x40, IP_ACTIVE_LOW, IPT_COIN1, 2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1                              )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "Bet",   KEYCODE_RCONTROL, IP_JOY_NONE )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "C",   KEYCODE_C,     IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "G",   KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "K",   KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z,     IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Flip",   KEYCODE_X,        IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( hnoridur )
	PORT_START	/* note that these are in reverse order wrt the others */
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE )	// Test
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )	// Analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE3 )	// Memory Reset
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1                              )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "Bet",   KEYCODE_RCONTROL, IP_JOY_NONE )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "C",   KEYCODE_C,     IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "G",   KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "K",   KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z,     IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Flip",   KEYCODE_X,        IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( sprtmtch )
	PORT_START	// IN0 - Player 1
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER1 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER1 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER1 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	// IN1 - Player 2
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_BUTTON3        | IPF_PLAYER2 )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	// IN2 - Coins
	PORT_BIT_IMPULSE(  0x01, IP_ACTIVE_LOW, IPT_COIN1, 10)
	PORT_BIT_IMPULSE(  0x02, IP_ACTIVE_LOW, IPT_COIN2, 10)
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START	// IN3 - DSW
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	// IN4 - DSW
	PORT_DIPNAME( 0x07, 0x04, DEF_STR( Difficulty ) )	// Time
	PORT_DIPSETTING(    0x00, "1 (Easy)" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x05, "6" )
	PORT_DIPSETTING(    0x06, "7" )
	PORT_DIPSETTING(    0x07, "8 (Hard)" )
	PORT_DIPNAME( 0x18, 0x10, "Vs Time" )
	PORT_DIPSETTING(    0x18, "8 s" )
	PORT_DIPSETTING(    0x10, "10 s" )
	PORT_DIPSETTING(    0x08, "12 s" )
	PORT_DIPSETTING(    0x00, "14 s" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 2-7" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2-8" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( mjfriday )
	PORT_START
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x04, "PINFU with TSUMO" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x18, 0x18, "Player Strength" )
	PORT_DIPSETTING(    0x18, "Weak" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x08, "Strong" )
	PORT_DIPSETTING(    0x00, "Strongest" )
	PORT_DIPNAME( 0x60, 0x60, "CPU Strength" )
	PORT_DIPSETTING(    0x60, "Weak" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Strong" )
	PORT_DIPSETTING(    0x00, "Strongest" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Auto TSUMO" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Gfx test" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	// "17B"
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	// "18B"
	PORT_SERVICE(0x04, IP_ACTIVE_LOW )	// Test (there isn't a dip switch)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )	// Analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE3 )	// Memory Reset
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	// "06B"
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	// "18A"

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1                              )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "Bet",   KEYCODE_RCONTROL, IP_JOY_NONE )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "C",   KEYCODE_C,     IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "G",   KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "K",   KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z,     IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Flip",   KEYCODE_X,        IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( mjdialq2 )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	// "17B"
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	// "18B"
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )	// Test (there isn't a dip switch)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 )	// Analyzer
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE3 )	// Memory Reset
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	// "06B"
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	// "18A"

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "A",   KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "E",   KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "I",   KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Kan", KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1                              )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "N",     KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX(0x20, IP_ACTIVE_LOW, 0, "Bet",   KEYCODE_RCONTROL, IP_JOY_NONE )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "C",   KEYCODE_C,     IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "G",   KEYCODE_G,     IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "K",   KEYCODE_K,     IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE, IP_JOY_NONE )
	PORT_BITX(0x10, IP_ACTIVE_LOW, 0, "Ron", KEYCODE_Z,     IP_JOY_NONE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "D",   KEYCODE_D,    IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "H",   KEYCODE_H,    IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "L",   KEYCODE_L,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT, IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Flip",   KEYCODE_X,        IP_JOY_NONE )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



/***************************************************************************


								Machine Drivers


***************************************************************************/

/***************************************************************************
								Hana no Mai
***************************************************************************/

static struct AY8910interface hanamai_ay8910_interface =
{
	1,			/* 1 chip */
	22000000 / 8,	/* 2.75MHz ??? */
	{ 20 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM2203interface hanamai_ym2203_interface =
{
	1,
	22000000 / 8,					/* 2.75MHz */
	{ YM2203_VOL(50,20) },
	{ input_port_1_r },				/* Port A Read: DSW */
	{ input_port_0_r },				/* Port B Read: DSW */
	{ 0 },							/* Port A Write */
	{ 0 },							/* Port B Write */
	{ sprtmtch_sound_callback },	/* IRQ handler */
};

struct MSM5205interface hanami_msm5205_interface =
{
	1,
	384000,
	{ adpcm_int },			/* IRQ handler */
	{ MSM5205_S48_4B },		/* 8 KHz, 4 Bits  */
	{ 100 }
};



static MACHINE_DRIVER_START( hanamai )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",Z80,22000000 / 4)	/* 5.5MHz */
	MDRV_CPU_MEMORY(sprtmtch_readmem,sprtmtch_writemem)
	MDRV_CPU_PORTS(hanamai_readport,hanamai_writeport)
	MDRV_CPU_VBLANK_INT(sprtmtch_vblank_interrupt,1)	/* IM 0 needs an opcode on the data bus */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(adpcm)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER|VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 512-1, 16, 256-1)
	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(sprtmtch)			// static palette
	MDRV_VIDEO_START(hanamai)
	MDRV_VIDEO_UPDATE(hanamai)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, hanamai_ay8910_interface)
	MDRV_SOUND_ADD(YM2203, hanamai_ym2203_interface)
	MDRV_SOUND_ADD(MSM5205, hanami_msm5205_interface)
MACHINE_DRIVER_END



/***************************************************************************
								Hana Oriduru
***************************************************************************/

static struct AY8910interface hnoridur_ay8910_interface =
{
	1,			/* 1 chip */
	22000000 / 8,	/* 2.75MHz ??? */
	{ 20 },
	{ input_port_0_r },		/* Port A Read: DSW */
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM2413interface hnoridur_ym2413_interface=
{
	1,	/* 1 chip */
	3580000,	/* 3.58 MHz */
	{ YM2413_VOL(100,MIXER_PAN_CENTER,100,MIXER_PAN_CENTER) }
};

static MACHINE_DRIVER_START( hnoridur )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",Z80,22000000 / 4)	/* 5.5MHz */
	MDRV_CPU_MEMORY(hnoridur_readmem,hnoridur_writemem)
	MDRV_CPU_PORTS(hnoridur_readport,hnoridur_writeport)
	MDRV_CPU_VBLANK_INT(sprtmtch_vblank_interrupt,1)	/* IM 0 needs an opcode on the data bus */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(adpcm)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER|VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 512-1, 16, 256-1)
	MDRV_PALETTE_LENGTH(16*256)

	MDRV_VIDEO_START(hnoridur)
	MDRV_VIDEO_UPDATE(hnoridur)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, hnoridur_ay8910_interface)
	MDRV_SOUND_ADD(YM2413, hnoridur_ym2413_interface)
	MDRV_SOUND_ADD(MSM5205, hanami_msm5205_interface)
MACHINE_DRIVER_END


/***************************************************************************
								Sports Match
***************************************************************************/

static struct YM2203interface sprtmtch_ym2203_interface =
{
	1,
	22000000 / 8,					/* 2.75MHz */
	{ YM2203_VOL(100,20) },
	{ input_port_3_r },				/* Port A Read: DSW */
	{ input_port_4_r },				/* Port B Read: DSW */
	{ 0 },							/* Port A Write */
	{ 0 },							/* Port B Write */
	{ sprtmtch_sound_callback },	/* IRQ handler */
};

static MACHINE_DRIVER_START( sprtmtch )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,22000000 / 4)	/* 5.5MHz */
	MDRV_CPU_MEMORY(sprtmtch_readmem,sprtmtch_writemem)
	MDRV_CPU_PORTS(sprtmtch_readport,sprtmtch_writeport)
	MDRV_CPU_VBLANK_INT(sprtmtch_vblank_interrupt,1)	/* IM 0 needs an opcode on the data bus */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER|VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 512-1, 16, 256-1)
	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(sprtmtch)			// static palette
	MDRV_VIDEO_START(sprtmtch)
	MDRV_VIDEO_UPDATE(sprtmtch)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, sprtmtch_ym2203_interface)
MACHINE_DRIVER_END


/***************************************************************************
								Lady Frog
***************************************************************************/

static MACHINE_DRIVER_START( ladyfrog )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,22000000 / 4)	/* 5.5MHz */
	MDRV_CPU_MEMORY(sprtmtch_readmem,sprtmtch_writemem)
	MDRV_CPU_PORTS(sprtmtch_readport,sprtmtch_writeport)
	MDRV_CPU_VBLANK_INT(sprtmtch_vblank_interrupt,1)	/* IM 0 needs an opcode on the data bus */

// Until the protection on the 68000 is figured out (or it will crash)
#if 0
	MDRV_CPU_ADD(M68000, 10000000)	/* 10 MHz??? */
	MDRV_CPU_MEMORY(ladyfrog_readmem,ladyfrog_writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)
#endif

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER|VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 512-1, 16, 256-1)
	MDRV_PALETTE_LENGTH(512)
	MDRV_GFXDECODE(gfxdecodeinfo)	// has gfx

	// no static palette
	MDRV_VIDEO_START(sprtmtch)
	MDRV_VIDEO_UPDATE(sprtmtch)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2203, sprtmtch_ym2203_interface)
MACHINE_DRIVER_END



/***************************************************************************
							Mahjong Friday
***************************************************************************/

static struct YM2413interface mjfriday_ym2413_interface=
{
	1,	/* 1 chip */
	24000000/6,	/* ???? */
	{ YM2413_VOL(100,MIXER_PAN_CENTER,100,MIXER_PAN_CENTER) }
};

static MACHINE_DRIVER_START( mjfriday )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",Z80,24000000/4)	/* 6 MHz? */
	MDRV_CPU_MEMORY(sprtmtch_readmem,sprtmtch_writemem)
	MDRV_CPU_PORTS(mjfriday_readport,mjfriday_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 16, 256-1)
	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(sprtmtch)			// static palette
	MDRV_VIDEO_START(mjdialq2)
	MDRV_VIDEO_UPDATE(mjdialq2)

	/* sound hardware */
	MDRV_SOUND_ADD(YM2413, mjfriday_ym2413_interface)
MACHINE_DRIVER_END


/***************************************************************************
							Mahjong Dial Q2
***************************************************************************/

static MACHINE_DRIVER_START( mjdialq2 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM( mjfriday )
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(mjdialq2_readmem,mjdialq2_writemem)
MACHINE_DRIVER_END


/***************************************************************************


								ROMs Loading


***************************************************************************/


/***************************************************************************

Hana no Mai
(c)1988 Dynax

D1610088L1

CPU:	Z80-A
Sound:	AY-3-8912A
	YM2203C
	M5205
OSC:	22.000MHz
Custom:	(TC17G032AP-0246)

***************************************************************************/

ROM_START( hanamai )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "1611.13a", 0x00000, 0x10000, CRC(5ca0b073) SHA1(56b64077e7967fdbb87a7685ca9662cc7881b5ec) )
	ROM_LOAD( "1610.14a", 0x48000, 0x10000, CRC(b20024aa) SHA1(bb6ce9821c1edbf7d4cfadc58a2b257755856937) )

	ROM_REGION( 0x140000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "1604.12e", 0x000000, 0x20000, CRC(3b8362f7) SHA1(166ce1fe5c48b02b728cf9b90f3dfae4461a3e2c) )
	ROM_LOAD( "1609.12c", 0x020000, 0x20000, CRC(91c5d211) SHA1(343ee4273e8075ba9c2f545c1ee2e403623e3185) )
	ROM_LOAD( "1603.13e", 0x040000, 0x20000, CRC(16a2a680) SHA1(7cd5de9a36fd05261d23f0a0e90d871b132368f0) )
	ROM_LOAD( "1608.13c", 0x060000, 0x20000, CRC(793dd4f8) SHA1(57c32fae553ba9d37c2ffdbfa96ede113f7bcccb) )
	ROM_LOAD( "1602.14e", 0x080000, 0x20000, CRC(3189a45f) SHA1(06e8cb1b6dd6d7e00a7270d4c76b894aba2e7734) )
	ROM_LOAD( "1607.14c", 0x0a0000, 0x20000, CRC(a58edfd0) SHA1(8112b0e0bf8c5bdd1d2e3338d23fe36cf3972a43) )
	ROM_LOAD( "1601.15e", 0x0c0000, 0x20000, CRC(84ff07af) SHA1(d7259056c4e09171aa8b9342ebaf3b8a3490613a) )
	ROM_LOAD( "1606.15c", 0x0e0000, 0x10000, CRC(ce7146c1) SHA1(dc2e202a67d1618538eb04248c1b2c7d7f62151e) )
	ROM_LOAD( "1605.10e", 0x100000, 0x10000, CRC(0f4fd9e4) SHA1(8bdc8b46bf4dafead25a5adaebb74d547386ce23) )
	ROM_LOAD( "1612.10c", 0x120000, 0x10000, CRC(8d9fb6e1) SHA1(2763f73069147d62fd46bb961b64cc9598687a28) )

	ROM_REGION( 0x400, REGION_PROMS, ROMREGION_DISPOSE )	/* Color PROMs */
	ROM_LOAD( "2.3j",  0x000, 0x200, CRC(7b0618a5) SHA1(df3aadcc7d54fab0c07f85d20c138a45798644e4) )	// FIXED BITS (0xxxxxxx)
	ROM_LOAD( "1.4j",  0x200, 0x200, CRC(9cfcdd2d) SHA1(a649e9381754c4a19ccecc6e558067cc3ff27f91) )
ROM_END


/***************************************************************************

Hana Kochou (Hana no Mai BET ver.)
(c)1988 Dynax

D201901L2
D201901L1-0

CPU:	Z80-A
Sound:	AY-3-8912A
	YM2203C
	M5205
OSC:	22.000MHz
VDP:	HD46505SP
Custom:	(TC17G032AP-0246)

***************************************************************************/

ROM_START( hnkochou )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "2009.s4a", 0x00000, 0x10000, CRC(b3657123) SHA1(3385edb2055abc7be3abb030509c6ac71907a5f3) )
	ROM_LOAD( "2008.s3a", 0x18000, 0x10000, CRC(1c009be0) SHA1(0f950d2685f8b67f37065e19deae0cf0cb9594f1) )

	ROM_REGION( 0x0e0000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "2004.12e", 0x000000, 0x20000, CRC(178aa996) SHA1(0bc8b7c093ed46a91c5781c03ae707d3dafaeef4) )
	ROM_LOAD( "2005.12c", 0x020000, 0x20000, CRC(ca57cac5) SHA1(7a55e1cb5cee5a38c67199f589e0c7ae5cd907a0) )
	ROM_LOAD( "2003.13e", 0x040000, 0x20000, CRC(092edf8d) SHA1(3d030462f96edbb0fa4efcc2a5302c17661dce54) )
	ROM_LOAD( "2006.13c", 0x060000, 0x20000, CRC(610c22ec) SHA1(af47bf94e01d1a83aa2a7195c906e13038057c35) )
	ROM_LOAD( "2002.14e", 0x080000, 0x20000, CRC(759b717d) SHA1(fd719fd792459789b559a1e99173144322605b8e) )
	ROM_LOAD( "2007.14c", 0x0a0000, 0x20000, CRC(d0f22355) SHA1(7b027930624ff1f883d620a8e78f962e821f4b23) )
	ROM_LOAD( "2001.15e", 0x0c0000, 0x20000, CRC(09ace2b5) SHA1(1756e3a52523557aa481c6bd6cdf168567af82ff) )

	ROM_REGION( 0x400, REGION_PROMS, ROMREGION_DISPOSE )	/* Color PROMs */
	ROM_LOAD( "2.3j",  0x000, 0x200, CRC(7b0618a5) SHA1(df3aadcc7d54fab0c07f85d20c138a45798644e4) )	// FIXED BITS (0xxxxxxx)
	ROM_LOAD( "1.4j",  0x200, 0x200, CRC(9cfcdd2d) SHA1(a649e9381754c4a19ccecc6e558067cc3ff27f91) )
ROM_END



/***************************************************************************

Hana Oriduru
(c)1989 Dynax
D2304268L

CPU  : Z80B
Sound: AY-3-8912A YM2413 M5205
OSC  : 22MHz (X1, near main CPU), 384KHz (X2, near M5205)
       3.58MHz (X3, Sound section)

CRT Controller: HD46505SP (6845)
Custom chip: DYNAX TC17G032AP-0246 JAPAN 8929EAI

***************************************************************************/

ROM_START( hnoridur )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "2309.12",  0x00000, 0x20000, CRC(5517dd68) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "2302.21",  0x000000, 0x20000, CRC(9dde2d59) )
	ROM_LOAD( "2303.22",  0x020000, 0x20000, CRC(1ac59443) )
	ROM_LOAD( "2301.20",  0x040000, 0x20000, CRC(24391ddc) )
	ROM_LOAD( "2304.1",   0x060000, 0x20000, BAD_DUMP CRC(9792d9fa)  )
	ROM_LOAD( "2305.2",   0x080000, 0x20000, CRC(249d360a) )
	ROM_LOAD( "2306.3",   0x0a0000, 0x20000, CRC(014a4945) )
	ROM_LOAD( "2307.4",   0x0c0000, 0x20000, CRC(8b6f8a2d) )
	ROM_LOAD( "2308.5",   0x0e0000, 0x20000, CRC(6f996e6e) )
ROM_END



/***************************************************************************

Sports Match
Dynax 1989

                     5563
                     3101
        SW2 SW1
                             3103
         YM2203              3102
                     16V8
                     Z80         DYNAX
         22MHz

           6845
                         53462
      17G                53462
      18G                53462
                         53462
                         53462
                         53462

***************************************************************************/

ROM_START( drgpunch )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "2401.3d", 0x00000, 0x10000, CRC(b310709c) SHA1(6ad6cfb54856f65a888ac44e694890f32f26e049) )
	ROM_LOAD( "2402.6d", 0x28000, 0x10000, CRC(d21ed237) SHA1(7e1c7b40c300578132ebd79cbad9f7976cc85947) )

	ROM_REGION( 0xc0000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "2403.6c", 0x00000, 0x20000, CRC(b936f202) SHA1(4920d29a814ebdd74ce6f780cf821c8cb8142d9f) )
	ROM_LOAD( "2404.5c", 0x20000, 0x20000, CRC(2ee0683a) SHA1(e29225e08be11f6971fa04ce2715be914d29976b) )
	ROM_LOAD( "2405.3c", 0x40000, 0x20000, CRC(aefbe192) SHA1(9ed0ec7d6357eedec80a90364f196e43a5bfee03) )
	ROM_LOAD( "2406.1c", 0x60000, 0x20000, CRC(e137f96e) SHA1(c652f3cb17a56127d30a516af75154e15d7ce6c0) )
	ROM_LOAD( "2407.6a", 0x80000, 0x20000, CRC(f3f1b065) SHA1(531317e4d1ab5db60595ca3327234a6bdea79ce9) )
	ROM_LOAD( "2408.5a", 0xa0000, 0x20000, CRC(3a91e2b9) SHA1(b762c38ff2ebbd4ed832ca772973a15dd4a4ad73) )

	ROM_REGION( 0x400, REGION_PROMS, ROMREGION_DISPOSE )	/* Color PROMs */
	ROM_LOAD( "2.18g", 0x000, 0x200, CRC(9adccc33) SHA1(acf4d5a28430378dbccc1b9fa0b6391cc8149fee) )	// FIXED BITS (0xxxxxxx)
	ROM_LOAD( "1.17g", 0x200, 0x200, CRC(324fa9cf) SHA1(a03e23d9a9687dec4c23a8e41254a3f4b70c7e25) )
ROM_END

ROM_START( sprtmtch )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "3101.3d", 0x00000, 0x08000, CRC(d8fa9638) SHA1(9851d38b6b3f56cf3cc101419c24f8d5f97950a9) )
	ROM_CONTINUE(        0x28000, 0x08000 )

	ROM_REGION( 0x40000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "3102.6c", 0x00000, 0x20000, CRC(46f90e59) SHA1(be4411c3cfa8c8ab26eba935289df0f0fd545b62) )
	ROM_LOAD( "3103.5c", 0x20000, 0x20000, CRC(ad29d7bd) SHA1(09ab84164e5cd14b595f33d129863735901aa922) )

	ROM_REGION( 0x400, REGION_PROMS, ROMREGION_DISPOSE )	/* Color PROMs */
	ROM_LOAD( "18g", 0x000, 0x200, CRC(dcc4e0dd) SHA1(4e0fb8fd7192bf32247966742df4b80585f32c37) )	// FIXED BITS (0xxxxxxx)
	ROM_LOAD( "17g", 0x200, 0x200, CRC(5443ebfb) SHA1(5b63220a3f6520e353db99b06e645640d1cfde2f) )
ROM_END


/***************************************************************************

The Return of Lady Frog
Microhard, 1993

PCB Layout
----------


YM2203                            68000
YM3014    6116           **       2   6
          6116          6116      3   7
6264                              4   8
1  Z80           MACH130          5   9
                 681000        6264  6264


DSW2              2148                10
DSW1              2148  6264  30MHz   11
                  2148  6264  24MHz   12
                  2148                13

Notes:
      68000 Clock = >10MHz (my meter can only read up to 10.000MHz)
        Z80 Clock = 3MHz
               ** = possibly PLD (surface is scratched, type PLCC44)
    Vertical Sync = 60Hz
      Horiz. Sync = 15.56kHz

Developers:
           More info reqd? Email me....
           theguru@emuunlim.com

***************************************************************************/

ROM_START( ladyfrog )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "ladyfrog.001", 0x00000, 0x20000, CRC(ba9eb1c6) )
	ROM_RELOAD(               0x20000, 0x20000 )

	ROM_REGION( 0x400000, REGION_CPU2, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "ladyfrog.002",	0x000000, 0x080000, CRC(724cf022) )
	ROM_LOAD16_BYTE( "ladyfrog.006",	0x000001, 0x080000, CRC(e52a7ae2) )
	ROM_LOAD16_BYTE( "ladyfrog.003",	0x100000, 0x080000, CRC(a1d49967) )
	ROM_LOAD16_BYTE( "ladyfrog.007",	0x100001, 0x080000, CRC(e5805c4e) )
	ROM_LOAD16_BYTE( "ladyfrog.004",	0x200000, 0x080000, CRC(709281f5) )
	ROM_LOAD16_BYTE( "ladyfrog.008",	0x200001, 0x080000, CRC(39adcba4) )
	ROM_LOAD16_BYTE( "ladyfrog.005",	0x300000, 0x080000, CRC(b683160c) )
	ROM_LOAD16_BYTE( "ladyfrog.009",	0x300001, 0x080000, CRC(e475fb76) )

	ROM_REGION( 0x10000, REGION_GFX1, 0 )	/* blitter data ?? */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ladyfrog.010",       0x00000, 0x20000, CRC(51fd0e1a) )
	ROM_LOAD( "ladyfrog.011",       0x20000, 0x20000, CRC(610bf6f3) )
	ROM_LOAD( "ladyfrog.012",       0x40000, 0x20000, CRC(466ede67) )
	ROM_LOAD( "ladyfrog.013",       0x60000, 0x20000, CRC(fad3e8be) )
ROM_END

ROM_START( ladyfrga )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "ladyfrog.001", 0x00000, 0x20000, CRC(ba9eb1c6) )
	ROM_RELOAD(               0x20000, 0x20000 )

	ROM_REGION( 0x400000, REGION_CPU2, 0 )	/* 68000 code */
	ROM_LOAD16_BYTE( "ladyfrog.002",	0x000000, 0x080000, CRC(724cf022) )
	ROM_LOAD16_BYTE( "ladyfrog.006",	0x000001, 0x080000, CRC(e52a7ae2) )
	ROM_LOAD16_BYTE( "ladyfrog.003",	0x100000, 0x080000, CRC(a1d49967) )
	ROM_LOAD16_BYTE( "ladyfrog.007",	0x100001, 0x080000, CRC(e5805c4e) )
	ROM_LOAD16_BYTE( "ladyfrog.004",	0x200000, 0x080000, CRC(709281f5) )
	ROM_LOAD16_BYTE( "ladyfrog.008",	0x200001, 0x080000, CRC(39adcba4) )
	ROM_LOAD16_BYTE( "ladyfrog.005",	0x300000, 0x080000, CRC(b683160c) )
	ROM_LOAD16_BYTE( "9",	            0x300001, 0x080000, CRC(fd515b58) )	// differs with ladyfrog.009 by 1 byte

	ROM_REGION( 0x10000, REGION_GFX1, 0 )	/* blitter data ?? */

	ROM_REGION( 0x80000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ladyfrog.010",       0x00000, 0x20000, CRC(51fd0e1a) )
	ROM_LOAD( "ladyfrog.011",       0x20000, 0x20000, CRC(610bf6f3) )
	ROM_LOAD( "ladyfrog.012",       0x40000, 0x20000, CRC(466ede67) )
	ROM_LOAD( "ladyfrog.013",       0x60000, 0x20000, CRC(fad3e8be) )
ROM_END


/***************************************************************************

Mahjong Friday
(c)1989 Dynax
D2607198L1

CPU  : Zilog Z0840006PSC (Z80)
Sound: YM2413 M5205
OSC  : 24MHz (X1)

CRT Controller: HD46505SP (6845)
Custom chip: DYNAX TC17G032AP-0246 JAPAN 8828EAI

***************************************************************************/

ROM_START( mjfriday )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "2606.2b",  0x00000, 0x10000, CRC(00e0e0d3) SHA1(89fa4d684ec36d5e974e39294efd65a9fd832517) )
	ROM_LOAD( "2605.2c",  0x28000, 0x10000, CRC(5459ebda) SHA1(86e51f0c120de87be8f51b498a562360e6b242b8) )

	ROM_REGION( 0x80000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "2601.2h",  0x000000, 0x20000, CRC(70a01fc7) SHA1(0ed2f7c258f3cd82229bea7514d262fca57bd925) )
	ROM_LOAD( "2602.2g",  0x020000, 0x20000, CRC(d9167c10) SHA1(0fa34a065b3ffd5d35d03275bdcdf753045d6491) )
	ROM_LOAD( "2603.2f",  0x040000, 0x20000, CRC(11892916) SHA1(0680ab77fc1a2cdb78637bf0c506f03ca514014b) )
	ROM_LOAD( "2604.2e",  0x060000, 0x20000, CRC(3cc1a65d) SHA1(221dc17042e46e58dc4634eef798568747aef3a2) )

	ROM_REGION( 0x400, REGION_PROMS, ROMREGION_DISPOSE )	/* Color PROMs */
	ROM_LOAD( "d26_2.9e", 0x000, 0x200, CRC(d6db5c60) SHA1(89ee10d092011c2c4eaab2c097aa88f5bb98bb97) )	// FIXED BITS (0xxxxxxx)
	ROM_LOAD( "d26_1.8e", 0x200, 0x200, CRC(af5edf32) SHA1(7202e0aa1ee3f22e3c5fb69a88db455a241929c5) )
ROM_END


/***************************************************************************

Maya
Promat, 1994

PCB Layout
----------

    6845      6264   3
 DSW1  DSW2    1     4
   YM2203      2     5
   3014B

              Z80
  22.1184MHz

 PROM1  TPC1020  D41264
 PROM2            (x6)


Notes:
      Z80 Clock: 5.522MHz
          HSync: 15.925 kHz
          VSync: 60Hz

Developers:
           More info reqd? Email me...
           theguru@emuunlim.com

***************************************************************************/

ROM_START( maya )
	ROM_REGION( 0x90000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "1.17e", 0x00000, 0x10000, CRC(5aaa015e) SHA1(b84d02b1b6c07636176f226fef09a034d00445f0) )
	ROM_LOAD( "2.15e", 0x28000, 0x10000, CRC(7ea5b49a) SHA1(aaae848669d9f88c0660f46cc801e4eb0f5e3b89) )

	ROM_REGION( 0xc0000, REGION_USER1, ROMREGION_DISPOSE )	/* blitter data */
	ROM_LOAD( "3.18g", 0x00000, 0x40000, CRC(8534af04) SHA1(b9bc94541776b5c0c6bf0ecc63ffef914756376e) )
	ROM_LOAD( "4.17g", 0x40000, 0x40000, CRC(ab85ce5e) SHA1(845b846e0fb8c9fcd1540960cda006fdac364fea) )
	ROM_LOAD( "5.15g", 0x80000, 0x40000, CRC(c4316dec) SHA1(2e727a491a71eb1f4d9f338cc6ec76e03f7b46fd) )

	ROM_REGION( 0xc0000, REGION_GFX1, 0 )
	/* blitter data will be decrypted here*/

	ROM_REGION( 0x400, REGION_PROMS, ROMREGION_DISPOSE )	/* Color PROMs */
	ROM_LOAD( "prom2.5b",  0x000, 0x200, CRC(d276bf61) SHA1(987058b37182a54a360a80a2f073b000606a11c9) )	// FIXED BITS (0xxxxxxx)
	ROM_LOAD( "prom1.6b",  0x200, 0x200, CRC(e38eb360) SHA1(739960dd57ec3305edd57aa63816a81ddfbebf3e) )
ROM_END

static DRIVER_INIT( maya )
{
	/* Address lines scrambling on 1 z80 rom */
	int i;
	data8_t	*gfx = (data8_t *)memory_region(REGION_GFX1);
	data8_t	*rom = memory_region(REGION_CPU1) + 0x28000,
			*end = rom + 0x10000;
	for (;rom < end; rom+=8)
	{
		data8_t temp[8];
		temp[0] = rom[0];	temp[1] = rom[1];	temp[2] = rom[2];	temp[3] = rom[3];
		temp[4] = rom[4];	temp[5] = rom[5];	temp[6] = rom[6];	temp[7] = rom[7];

		rom[0] = temp[0];	rom[1] = temp[4];	rom[2] = temp[1];	rom[3] = temp[5];
		rom[4] = temp[2];	rom[5] = temp[6];	rom[6] = temp[3];	rom[7] = temp[7];
	}

	/* Address lines scrambling on the blitter data roms */
	rom = memory_region(REGION_USER1);

	for (i = 0; i < 0xc0000; i++)
		gfx[i] = rom[BITSWAP24(i,23,22,21,20,19,18,14,15, 16,17,13,12,11,10,9,8, 7,6,5,4,3,2,1,0)];
}


/***************************************************************************

Mahjong Dial Q2
(c)1991 Dynax
D5212298L-1

CPU  : Z80
Sound: YM2413
OSC  : (240-100 624R001 KSSOB)?
Other: TC17G032AP-0246
CRT Controller: HD46505SP (6845)

***************************************************************************/

ROM_START( mjdialq2 )
	ROM_REGION( 0x78000, REGION_CPU1, 0 )	/* Z80 Code */
	ROM_LOAD( "5201.2b", 0x00000, 0x10000, CRC(5186c2df) SHA1(f05ae3fd5e6c39f3bf2263eaba645d89c454bd70) )
	ROM_RELOAD(          0x10000, 0x08000 )				// 1
	ROM_CONTINUE(        0x20000, 0x08000 )				// 3
	ROM_LOAD( "5202.2c", 0x30000, 0x08000, CRC(8e8b0038) SHA1(44130bb29b569610826e1fc7e4b2822f0e1034b1) )	// 5
	ROM_CONTINUE(        0x70000, 0x08000 )				// d

	ROM_REGION( 0xa0000, REGION_GFX1, 0 )	/* blitter data */
	ROM_LOAD( "5207.2h", 0x00000, 0x20000, CRC(7794cd62) SHA1(7fa2fd50d7c975c381dda36f505df0e152196bb5) )
	ROM_LOAD( "5206.2g", 0x20000, 0x20000, CRC(9e810021) SHA1(cf1052c96b9da3abb263be1ce8481aeded2c5d00) )
	ROM_LOAD( "5205.2f", 0x40000, 0x20000, CRC(8c05572f) SHA1(544a5eb8b989fb1195986ed856da04350941ef59) )
	ROM_LOAD( "5204.2e", 0x60000, 0x20000, CRC(958ef9ab) SHA1(ec768c587dc9e6b691564b6b35abbece252bcd28) )
	ROM_LOAD( "5203.2d", 0x80000, 0x20000, CRC(706072d7) SHA1(d4692296d234b824961a94390e6d646ed9a7d5fd) )

	ROM_REGION( 0x400, REGION_PROMS, ROMREGION_DISPOSE )	/* Color PROMs */
	ROM_LOAD( "d52-2.9e", 0x000, 0x200, CRC(18585ce3) SHA1(7f2e20bb09c1d810910094a6b19e5151666d74ac) )	// FIXED BITS (0xxxxxxx)
	ROM_LOAD( "d52-1.8e", 0x200, 0x200, CRC(8868247a) SHA1(97652025c411b379dfab576dc7f2d8d0d61d0828) )
ROM_END



/***************************************************************************


								Game Drivers


***************************************************************************/

GAME( 1988, hanamai,  0,        hanamai,  hanamai,  0,    ROT180, "Dynax",     "Hana no Mai (Japan)" )
GAME( 1989, hnkochou, hanamai,  hanamai,  hnkochou, 0,    ROT180, "Dynax",     "Hana Kochou [BET] (Japan)" )
GAMEX(1989, hnoridur, 0,        hnoridur, hnoridur, 0,    ROT180, "Dynax",     "Hana Oriduru (Japan)", GAME_NOT_WORKING )
GAME( 1989, drgpunch, 0,        sprtmtch, sprtmtch, 0,    ROT0,   "Dynax",     "Dragon Punch (Japan)" )
GAME( 1989, sprtmtch, drgpunch, sprtmtch, sprtmtch, 0,    ROT0,   "Dynax (Fabtek license)", "Sports Match" )
GAME( 1989, mjfriday, 0,        mjfriday, mjfriday, 0,    ROT180, "Dynax",     "Mahjong Friday (Japan)" )
GAMEX(1991, mjdialq2, 0,        mjdialq2, mjdialq2, 0,    ROT180, "Dynax",     "Mahjong Dial Q2 (Japan)", GAME_IMPERFECT_GRAPHICS )
GAMEX(1993, ladyfrog, 0,        ladyfrog, sprtmtch, 0,    ROT0,   "Microhard", "The Return of Lady Frog", GAME_NOT_WORKING )
GAMEX(1993, ladyfrga, ladyfrog, ladyfrog, sprtmtch, 0,    ROT0,   "Microhard", "The Return of Lady Frog (set 2)", GAME_NOT_WORKING )
GAME( 1994, maya,     0,        sprtmtch, sprtmtch, maya, ROT0,   "Promat",    "Maya" )

