/******************************************************************

	Double Dragon 3 					Technos Japan Corp 1990
	The Combatribes 					Technos Japan Corp 1990


	Notes:

	Both games have original and bootleg versions supported.
	Double Dragon 3 bootleg has some misplaced graphics, but I
	think this is how the real thing would look.
	Double Dragon 3 original cut scenes seem to fade a bit fast?
	Combatribes has sprite lag but it seems to be caused by poor
	programming and I think the original does the same.

******************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/m68000/m68000.h"
#include "vidhrdw/generic.h"

VIDEO_UPDATE( ddragon3 );
VIDEO_UPDATE( ctribe );
WRITE16_HANDLER( ddragon3_scroll16_w );

extern VIDEO_START( ddragon3 );

extern data16_t *ddragon3_bg_videoram16;
WRITE16_HANDLER( ddragon3_bg_videoram16_w );
READ16_HANDLER( ddragon3_bg_videoram16_r );

extern data16_t *ddragon3_fg_videoram16;
WRITE16_HANDLER( ddragon3_fg_videoram16_w );
READ16_HANDLER( ddragon3_fg_videoram16_r );

/***************************************************************************/

static WRITE_HANDLER( oki_bankswitch_w )
{
	OKIM6295_set_bank_base(0, (data & 1) * 0x40000);
}

static READ16_HANDLER( ddrago3b_io16_r )
{
	switch (offset)
	{
	case 0: return readinputport(0) + 256*((readinputport(3)&0x0f)|((readinputport(4)&0xc0)<<2));
	case 1: return readinputport(1) + 256*(readinputport(4)&0x3f);
	case 2: return readinputport(2) + 256*(readinputport(5)&0x3f);
	case 3: return (readinputport(5)&0xc0)<<2;
	}
	return ~0;
}

static READ16_HANDLER( ctribe_io16_r )
{
	switch (offset)
	{
	case 0: return readinputport(0) + 256*((readinputport(3)&0x0f)|((readinputport(4)&0xc0)<<2));
	case 1: return readinputport(1) + 256*(readinputport(4)&0x3f);
	case 2: return readinputport(2) + 256*(readinputport(5)&0x3f);
	case 3: return 256*(readinputport(5)&0xc0);
	}
	return ~0;
}

static READ16_HANDLER( ddragon3_io16_r )
{
	switch (offset)
	{
	case 0: return readinputport(0);
	case 1: return readinputport(1);
	case 2: return readinputport(2);
	case 3: return readinputport(3);

	default: logerror("INPUT 1800[%02x] \n", offset);
	}
	return ~0;
}

extern UINT16 ddragon3_vreg;

static INTERRUPT_GEN( ddragon3_cpu_interrupt ) { /* 6:0x177e - 5:0x176a */
	if( cpu_getiloops() == 0 ){
		cpu_set_irq_line(0, 6, HOLD_LINE);  /* VBlank */
	}
	else {
		cpu_set_irq_line(0, 5, HOLD_LINE); /* Input Ports */
	}
}

static data16_t reg[8];

static WRITE16_HANDLER( ddragon3_io16_w )
{
	COMBINE_DATA(&reg[offset]);

	switch (offset)
	{
	case 0:
	ddragon3_vreg = reg[0];
	break;

	case 1: /* soundlatch_w */
	soundlatch_w(1,reg[1]&0xff);
	cpu_set_irq_line( 1, IRQ_LINE_NMI, PULSE_LINE );
	break;

	case 2:
	/*	this gets written to on startup and at the end of IRQ6
	**	possibly trigger IRQ on sound CPU
	*/
	break;

	case 3:
	/*	this gets written to on startup,
	**	and at the end of IRQ5 (input port read) */
	break;

	case 4:
	/* this gets written to at the end of IRQ6 only */
	break;

	default:
	logerror("OUTPUT 1400[%02x] %08x, pc=%06x \n", offset,(unsigned)data, activecpu_get_pc() );
	break;
	}
}

/**************************************************************************/

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x080000, 0x080fff, MRA16_RAM },	/* Foreground (32x32 Tiles - 4 by per tile) */
	{ 0x082000, 0x0827ff, MRA16_RAM },	/* Background (32x32 Tiles - 2 by per tile) */
	{ 0x100000, 0x100007, ddragon3_io16_r },
	{ 0x140000, 0x1405ff, MRA16_RAM },	/* Palette RAM */
	{ 0x180000, 0x180fff, MRA16_RAM },
	{ 0x1c0000, 0x1c3fff, MRA16_RAM },	/* working RAM */
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x080000, 0x080fff, ddragon3_fg_videoram16_w, &ddragon3_fg_videoram16 },
	{ 0x082000, 0x0827ff, ddragon3_bg_videoram16_w, &ddragon3_bg_videoram16 },
	{ 0x0c0000, 0x0c000f, ddragon3_scroll16_w },
	{ 0x100000, 0x10000f, ddragon3_io16_w },
	{ 0x140000, 0x1405ff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x180000, 0x180fff, MWA16_RAM, &spriteram16 }, /* Sprites (16 bytes per sprite) */
	{ 0x1c0000, 0x1c3fff, MWA16_RAM },
MEMORY_END

static MEMORY_READ16_START( dd3b_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x080000, 0x080fff, MRA16_RAM },	/* Foreground (32x32 Tiles - 4 by per tile) */
	{ 0x081000, 0x081fff, MRA16_RAM },
	{ 0x082000, 0x0827ff, MRA16_RAM },	/* Background (32x32 Tiles - 2 by per tile) */
	{ 0x100000, 0x1005ff, MRA16_RAM },	/* Palette RAM */
	{ 0x180000, 0x180007, ddrago3b_io16_r },
	{ 0x1c0000, 0x1c3fff, MRA16_RAM },	/* working RAM */
MEMORY_END

static MEMORY_WRITE16_START( dd3b_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x080000, 0x080fff, ddragon3_fg_videoram16_w, &ddragon3_fg_videoram16 },
	{ 0x081000, 0x081fff, MWA16_RAM, &spriteram16 }, /* Sprites (16 bytes per sprite) */
	{ 0x082000, 0x0827ff, ddragon3_bg_videoram16_w, &ddragon3_bg_videoram16 },
	{ 0x0c0000, 0x0c000f, ddragon3_scroll16_w },
	{ 0x100000, 0x1005ff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x140000, 0x14000f, ddragon3_io16_w },
	{ 0x1c0000, 0x1c3fff, MWA16_RAM },
MEMORY_END

static MEMORY_READ16_START( ctribe_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },
	{ 0x080000, 0x080fff, MRA16_RAM },	/* Foreground (32x32 Tiles - 4 by per tile) */
	{ 0x081000, 0x081fff, MRA16_RAM },
	{ 0x082000, 0x0827ff, MRA16_RAM },	/* Background (32x32 Tiles - 2 by per tile) */
	{ 0x100000, 0x1005ff, MRA16_RAM },	/* Palette RAM */
	{ 0x180000, 0x180007, ctribe_io16_r },
	{ 0x1c0000, 0x1c3fff, MRA16_RAM },	/* working RAM */
MEMORY_END

static MEMORY_WRITE16_START( ctribe_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x080000, 0x080fff, ddragon3_fg_videoram16_w, &ddragon3_fg_videoram16 },
	{ 0x081000, 0x081fff, MWA16_RAM, &spriteram16 }, /* Sprites (16 bytes per sprite) */
	{ 0x082000, 0x0827ff, ddragon3_bg_videoram16_w, &ddragon3_bg_videoram16 },
	{ 0x0c0000, 0x0c000f, ddragon3_scroll16_w },
	{ 0x100000, 0x1005ff, paletteram16_xxxxBBBBGGGGRRRR_word_w, &paletteram16 },
	{ 0x140000, 0x14000f, ddragon3_io16_w },
	{ 0x1c0000, 0x1c3fff, MWA16_RAM },
MEMORY_END

/**************************************************************************/

static MEMORY_READ_START( readmem_sound )
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc801, 0xc801, YM2151_status_port_0_r },
	{ 0xd800, 0xd800, OKIM6295_status_0_r },
	{ 0xe000, 0xe000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( writemem_sound )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xc800, YM2151_register_port_0_w },
	{ 0xc801, 0xc801, YM2151_data_port_0_w },
	{ 0xd800, 0xd800, OKIM6295_data_0_w },
	{ 0xe800, 0xe800, oki_bankswitch_w },
MEMORY_END

static MEMORY_READ_START( ctribe_readmem_sound )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8801, 0x8801, YM2151_status_port_0_r },
	{ 0x9800, 0x9800, OKIM6295_status_0_r },
	{ 0xa000, 0xa000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( ctribe_writemem_sound )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, YM2151_register_port_0_w },
	{ 0x8801, 0x8801, YM2151_data_port_0_w },
	{ 0x9800, 0x9800, OKIM6295_data_0_w },
MEMORY_END

/***************************************************************************/

INPUT_PORTS_START( ddrago3b )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPF_PLAYER3 | IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPF_PLAYER3 | IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPF_PLAYER3 | IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPF_PLAYER3 | IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPF_PLAYER3 | IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPF_PLAYER3 | IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPF_PLAYER3 | IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )

	PORT_START /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Continue Discount" )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START /* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR(Difficulty) )
	PORT_DIPSETTING(	0x02, "Easy" )
	PORT_DIPSETTING(	0x03, "Normal" )
	PORT_DIPSETTING(	0x01, "Hard" )
	PORT_DIPSETTING(	0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, "P1 hurt P2" )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) ) /* timer speed? */
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Test Mode" )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Stage Clear Power" )
	PORT_DIPSETTING(	0x20, "0" )
	PORT_DIPSETTING(	0x00, "50" )
	PORT_DIPNAME( 0x40, 0x40, "Starting Power" )
	PORT_DIPSETTING(	0x00, "200" )
	PORT_DIPSETTING(	0x40, "230" )
	PORT_DIPNAME( 0x80, 0x80, "Simultaneous Players" )
	PORT_DIPSETTING(	0x80, "2" )
	PORT_DIPSETTING(	0x00, "3" )
INPUT_PORTS_END

INPUT_PORTS_START( ddragon3 )
	PORT_START /* 180000 (P1 Controls) */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )

	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_BUTTON1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_BUTTON2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_BUTTON3 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START /* 180002 */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* 180004 (P3 Controls) */
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPF_PLAYER3 | IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPF_PLAYER3 | IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPF_PLAYER3 | IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPF_PLAYER3 | IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPF_PLAYER3 | IPT_BUTTON1 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPF_PLAYER3 | IPT_BUTTON2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPF_PLAYER3 | IPT_BUTTON3 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START3 )

	/* DSWA */
	/*PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	  0x0000, "A" )
	PORT_DIPSETTING(	  0x0100, "B" )
	PORT_DIPSETTING(	  0x0200, "C" )
	PORT_DIPSETTING(	  0x0300, "D" )*/
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, "Coin Statistics" )
	PORT_DIPSETTING(	  0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Starting Power" )
	PORT_DIPSETTING( 0x0000, "200" )
	PORT_DIPSETTING( 0x4000, "230" )
	PORT_DIPNAME( 0x8000, 0x8000, "Simultaneous Players" )
	PORT_DIPSETTING(	  0x8000, "2" )
	PORT_DIPSETTING(	  0x0000, "3" )

	PORT_START /* 180006 DSW */
	PORT_BIT( 0x00ff, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* fixes skipping through cut scenes without a button being pressed */
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x0100, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, "Stage Clear Power" )
	PORT_DIPSETTING(	  0x0400, "50" )
	PORT_DIPSETTING(	  0x0000, "0" )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(	  0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(	  0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0xc000, 0xc000, DEF_STR(Difficulty) )
	PORT_DIPSETTING(	  0x4000, "Easy" )
	PORT_DIPSETTING(	  0xc000, "Normal" )
	PORT_DIPSETTING(	  0x8000, "Hard" )
	PORT_DIPSETTING(	  0x0000, "Hardest" )
INPUT_PORTS_END

INPUT_PORTS_START( ctribe )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPF_PLAYER2 | IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPF_PLAYER3 | IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPF_PLAYER3 | IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPF_PLAYER3 | IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPF_PLAYER3 | IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPF_PLAYER3 | IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPF_PLAYER3 | IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPF_PLAYER3 | IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coinage ) )
	PORT_DIPSETTING(	0x00, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(	0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(	0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(	0x02, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Continue Discount" )
	PORT_DIPSETTING(	0x10, DEF_STR( No ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START /* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR(Difficulty) )
	PORT_DIPSETTING(	0x02, "Easy" )
	PORT_DIPSETTING(	0x03, "Normal" )
	PORT_DIPSETTING(	0x01, "Hard" )
	PORT_DIPSETTING(	0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, "Timer Speed" )
	PORT_DIPSETTING(	0x04, "Normal" )
	PORT_DIPSETTING(	0x00, "Fast" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Test Mode" )
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, "Stage Clear Power" )
	PORT_DIPSETTING(	0x60, "0" )
	PORT_DIPSETTING(	0x40, "50" )
	PORT_DIPSETTING(	0x20, "100" )
	PORT_DIPSETTING(	0x00, "150" )
	PORT_DIPNAME( 0x80, 0x80, "Simultaneous Players" )
	PORT_DIPSETTING(	0x80, "2" )
	PORT_DIPSETTING(	0x00, "3" )
INPUT_PORTS_END

/***************************************************************************/

static struct GfxLayout tile_layout =
{
	16,16,	/* 16*16 tiles */
	8192,	/* 8192 tiles */
	4,	/* 4 bits per pixel */
	{ 0, 0x40000*8, 2*0x40000*8 , 3*0x40000*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every tile takes 32 consecutive bytes */
};

static struct GfxLayout sprite_layout = {
	16,16,	/* 16*16 tiles */
	0x90000/32, /* 4096 tiles */
	4,	/* 4 bits per pixel */
	{ 0, 0x100000*8, 2*0x100000*8 , 3*0x100000*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every tile takes 32 consecutive bytes */
};

static struct GfxDecodeInfo ddragon3_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_layout,   256, 32 },
	{ REGION_GFX2, 0, &sprite_layout,		0, 16 },
	{ -1 }
};

/***************************************************************************/

static void dd3_ymirq_handler(int irq)
{
	cpu_set_irq_line( 1, 0 , irq ? ASSERT_LINE : CLEAR_LINE );
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* Guess */
	{ YM3012_VOL(45,MIXER_PAN_LEFT,45,MIXER_PAN_RIGHT) },
	{ dd3_ymirq_handler }
};

static struct OKIM6295interface okim6295_interface =
{
	1,				/* 1 chip */
	{ 8500 },		/* frequency (Hz) */
	{ REGION_SOUND1 },	/* memory region */
	{ 47 }
};

/**************************************************************************/

static MACHINE_DRIVER_START( ddragon3 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000) /* Guess */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(ddragon3_cpu_interrupt,2)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* Guess */
	MDRV_CPU_MEMORY(readmem_sound,writemem_sound)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 319, 8, 239)
	MDRV_GFXDECODE(ddragon3_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)

	MDRV_VIDEO_START(ddragon3)
	MDRV_VIDEO_UPDATE(ddragon3)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ddrago3b )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000) /* Guess */
	MDRV_CPU_MEMORY(dd3b_readmem,dd3b_writemem)
	MDRV_CPU_VBLANK_INT(ddragon3_cpu_interrupt,2)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* Guess */
	MDRV_CPU_MEMORY(readmem_sound,writemem_sound)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 319, 8, 239)
	MDRV_GFXDECODE(ddragon3_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)

	MDRV_VIDEO_START(ddragon3)
	MDRV_VIDEO_UPDATE(ddragon3)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ctribe )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000) /* Guess */
	MDRV_CPU_MEMORY(ctribe_readmem,ctribe_writemem)
	MDRV_CPU_VBLANK_INT(ddragon3_cpu_interrupt,2)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* Guess */
	MDRV_CPU_MEMORY(ctribe_readmem_sound,ctribe_writemem_sound)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 319, 8, 239)
	MDRV_GFXDECODE(ddragon3_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)

	MDRV_VIDEO_START(ddragon3)
	MDRV_VIDEO_UPDATE(ctribe)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

/**************************************************************************/

ROM_START( ddragon3 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 64k for cpu code */
	ROM_LOAD16_BYTE( "30a14",   0x00001, 0x40000, CRC(f42fe016) SHA1(11511aa43caa12b36a795bfaefee824821282523) )
	ROM_LOAD16_BYTE( "30a15",   0x00000, 0x20000, CRC(ad50e92c) SHA1(facac5bbe11716d076a40eacbb67f7caab7a4a27) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu code */
	ROM_LOAD( "dd3.06",    0x00000, 0x10000, CRC(1e974d9b) SHA1(8e54ff747efe587a2e971c15e729445c4e232f0f) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "dd3.f",   0x000000, 0x40000, CRC(89d58d32) SHA1(54cfc154024e014f537c7ae0c2275ece50413bc5) ) /* Background */
	ROM_LOAD( "dd3.e",   0x040000, 0x40000, CRC(9bf1538e) SHA1(c7cb96c6b1ac73ec52f46b2a6687bfcfd375ab44) )
	ROM_LOAD( "dd3.b",   0x080000, 0x40000, CRC(8f671a62) SHA1(b5dba61ad6ed39440bb98f7b2dc1111779d6c4a1) )
	ROM_LOAD( "dd3.a",   0x0c0000, 0x40000, CRC(0f74ea1c) SHA1(6bd8dd89bd22b29038cf502a898336e95e50a9cc) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	/* sprites	*/
	ROM_LOAD( "dd3.3e",   0x000000, 0x20000, CRC(726c49b7) SHA1(dbafad47bb6b717c409fdc5d81c413f1282f2bbb) ) //4a
	ROM_LOAD( "dd3.3d",   0x020000, 0x20000, CRC(37a1c335) SHA1(de70ba51788b601591c3aff71cb94aae349b272d) ) //3a
	ROM_LOAD( "dd3.3c",   0x040000, 0x20000, CRC(2bcfe63c) SHA1(678ef0e7cc38e4df1e1d1e3f5cba6601aa520ec6) ) //2a
	ROM_LOAD( "dd3.3b",   0x060000, 0x20000, CRC(b864cf17) SHA1(39a5155f40ba500bf201acca6f7d230cb0ea8309) ) //1a
	ROM_LOAD( "dd3.3a",   0x080000, 0x10000, CRC(20d64bea) SHA1(c2bd86bc5310f13f158ca2f93cfc57e5dbf01f7e) ) //5a

	ROM_LOAD( "dd3.2e",   0x100000, 0x20000, CRC(8c71eb06) SHA1(e47acf9e2d5eeec0cff9654210a43c690a45d447) ) //4b
	ROM_LOAD( "dd3.2d",   0x120000, 0x20000, CRC(3e134be9) SHA1(0a75b56353bed2743f7ce8f3f74379fc9f0d3cb9) ) //3b
	ROM_LOAD( "dd3.2c",   0x140000, 0x20000, CRC(b4115ef0) SHA1(d90943f75051c7590a0effcc30fa813890c9ad11) ) //2b
	ROM_LOAD( "dd3.2b",   0x160000, 0x20000, CRC(4639333d) SHA1(8e3c982d6fa38cbec42e8de780f165547b5b0271) ) //1b
	ROM_LOAD( "dd3.2a",   0x180000, 0x10000, CRC(785d71b0) SHA1(e3f63f6984589d4d6ec6200ae33ce12610d27774) ) //5b

	ROM_LOAD( "dd3.1e",   0x200000, 0x20000, CRC(04420cc8) SHA1(ed148c52374bbd0d29c12070ea1499333fc04449) ) //4c
	ROM_LOAD( "dd3.1d",   0x220000, 0x20000, CRC(33f97b2f) SHA1(40dc5357caa17ed6673588422332966ee97752b7) ) //3c
	ROM_LOAD( "dd3.1c",   0x240000, 0x20000, CRC(0f9a8f2a) SHA1(d7e46d32067d3f8b3bacbf96ea313645a9a48410) ) //2c
	ROM_LOAD( "dd3.1b",   0x260000, 0x20000, CRC(15c91772) SHA1(8578b6c501e3af64863bd6b28ef59c6884dfe028) ) //1c
	ROM_LOAD( "dd3.1a",   0x280000, 0x10000, CRC(15e43d12) SHA1(b51cbd0c4c38b802e60616e11795b1ac43bfcb01) ) //5c

	ROM_LOAD( "dd3.0e",   0x300000, 0x20000, CRC(894734b3) SHA1(46fa174a303e85f439254976252835626c4b2ddc) ) //4d
	ROM_LOAD( "dd3.0d",   0x320000, 0x20000, CRC(cd504584) SHA1(674481b524853dbfcb7d173d58250b1be8464313) ) //3d
	ROM_LOAD( "dd3.0c",   0x340000, 0x20000, CRC(38e8a9ad) SHA1(1c66acde8f72fa7c6415a7aadc2dbf4300446c88) ) //2d
	ROM_LOAD( "dd3.0b",   0x360000, 0x20000, CRC(80c1cb74) SHA1(5558fa36b238cff1bee9df921e77d7de2062bf15) ) //1d
	ROM_LOAD( "dd3.0a",   0x380000, 0x10000, CRC(5a47e7a4) SHA1(74b9dff6e3d5fe22ea505dc439121ff64889769c) ) //5d

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* ADPCM Samples */
	ROM_LOAD( "dd3.j7",   0x000000, 0x40000, CRC(3af21dbe) SHA1(295d0b7f33c55ef37a71382a22edd8fc97fa5353) )
	ROM_LOAD( "dd3.j8",   0x040000, 0x40000, CRC(c28b53cd) SHA1(93d29669ec899fd5852f61b1d91d0a90cc30e192) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "mb7114h.38", 0x0000, 0x0100, CRC(113c7443) SHA1(7b0b13e9f0c219f6d436aeec06494734d1f4a599) )
ROM_END

ROM_START( ddrago3b )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 64k for cpu code */
	ROM_LOAD16_BYTE( "dd3.01",   0x00001, 0x20000, CRC(68321d8b) SHA1(bd34d361e8ef18ef2b7e8bfe438b1b098c3151b5) )
	ROM_LOAD16_BYTE( "dd3.03",   0x00000, 0x20000, CRC(bc05763b) SHA1(49f661fdc98bd43a6622945e9aa8d8e7a7dc1ce6) )
	ROM_LOAD16_BYTE( "dd3.02",   0x40001, 0x20000, CRC(38d9ae75) SHA1(d42e1d9c704c66bad94e14d14f5e0b7209cc938e) )
	/* No EVEN rom! */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu code */
	ROM_LOAD( "dd3.06",    0x00000, 0x10000, CRC(1e974d9b) SHA1(8e54ff747efe587a2e971c15e729445c4e232f0f) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	/* Background */
	ROM_LOAD( "dd3.f",   0x000000, 0x40000, CRC(89d58d32) SHA1(54cfc154024e014f537c7ae0c2275ece50413bc5) )
	ROM_LOAD( "dd3.e",   0x040000, 0x40000, CRC(9bf1538e) SHA1(c7cb96c6b1ac73ec52f46b2a6687bfcfd375ab44) )
	ROM_LOAD( "dd3.b",   0x080000, 0x40000, CRC(8f671a62) SHA1(b5dba61ad6ed39440bb98f7b2dc1111779d6c4a1) )
	ROM_LOAD( "dd3.a",   0x0c0000, 0x40000, CRC(0f74ea1c) SHA1(6bd8dd89bd22b29038cf502a898336e95e50a9cc) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	/* sprites	*/
	ROM_LOAD( "dd3.3e",   0x000000, 0x20000, CRC(726c49b7) SHA1(dbafad47bb6b717c409fdc5d81c413f1282f2bbb) ) //4a
	ROM_LOAD( "dd3.3d",   0x020000, 0x20000, CRC(37a1c335) SHA1(de70ba51788b601591c3aff71cb94aae349b272d) ) //3a
	ROM_LOAD( "dd3.3c",   0x040000, 0x20000, CRC(2bcfe63c) SHA1(678ef0e7cc38e4df1e1d1e3f5cba6601aa520ec6) ) //2a
	ROM_LOAD( "dd3.3b",   0x060000, 0x20000, CRC(b864cf17) SHA1(39a5155f40ba500bf201acca6f7d230cb0ea8309) ) //1a
	ROM_LOAD( "dd3.3a",   0x080000, 0x10000, CRC(20d64bea) SHA1(c2bd86bc5310f13f158ca2f93cfc57e5dbf01f7e) ) //5a

	ROM_LOAD( "dd3.2e",   0x100000, 0x20000, CRC(8c71eb06) SHA1(e47acf9e2d5eeec0cff9654210a43c690a45d447) ) //4b
	ROM_LOAD( "dd3.2d",   0x120000, 0x20000, CRC(3e134be9) SHA1(0a75b56353bed2743f7ce8f3f74379fc9f0d3cb9) ) //3b
	ROM_LOAD( "dd3.2c",   0x140000, 0x20000, CRC(b4115ef0) SHA1(d90943f75051c7590a0effcc30fa813890c9ad11) ) //2b
	ROM_LOAD( "dd3.2b",   0x160000, 0x20000, CRC(4639333d) SHA1(8e3c982d6fa38cbec42e8de780f165547b5b0271) ) //1b
	ROM_LOAD( "dd3.2a",   0x180000, 0x10000, CRC(785d71b0) SHA1(e3f63f6984589d4d6ec6200ae33ce12610d27774) ) //5b

	ROM_LOAD( "dd3.1e",   0x200000, 0x20000, CRC(04420cc8) SHA1(ed148c52374bbd0d29c12070ea1499333fc04449) ) //4c
	ROM_LOAD( "dd3.1d",   0x220000, 0x20000, CRC(33f97b2f) SHA1(40dc5357caa17ed6673588422332966ee97752b7) ) //3c
	ROM_LOAD( "dd3.1c",   0x240000, 0x20000, CRC(0f9a8f2a) SHA1(d7e46d32067d3f8b3bacbf96ea313645a9a48410) ) //2c
	ROM_LOAD( "dd3.1b",   0x260000, 0x20000, CRC(15c91772) SHA1(8578b6c501e3af64863bd6b28ef59c6884dfe028) ) //1c
	ROM_LOAD( "dd3.1a",   0x280000, 0x10000, CRC(15e43d12) SHA1(b51cbd0c4c38b802e60616e11795b1ac43bfcb01) ) //5c

	ROM_LOAD( "dd3.0e",   0x300000, 0x20000, CRC(894734b3) SHA1(46fa174a303e85f439254976252835626c4b2ddc) ) //4d
	ROM_LOAD( "dd3.0d",   0x320000, 0x20000, CRC(cd504584) SHA1(674481b524853dbfcb7d173d58250b1be8464313) ) //3d
	ROM_LOAD( "dd3.0c",   0x340000, 0x20000, CRC(38e8a9ad) SHA1(1c66acde8f72fa7c6415a7aadc2dbf4300446c88) ) //2d
	ROM_LOAD( "dd3.0b",   0x360000, 0x20000, CRC(80c1cb74) SHA1(5558fa36b238cff1bee9df921e77d7de2062bf15) ) //1d
	ROM_LOAD( "dd3.0a",   0x380000, 0x10000, CRC(5a47e7a4) SHA1(74b9dff6e3d5fe22ea505dc439121ff64889769c) ) //5d

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* ADPCM Samples */
	ROM_LOAD( "dd3.j7",   0x000000, 0x40000, CRC(3af21dbe) SHA1(295d0b7f33c55ef37a71382a22edd8fc97fa5353) )
	ROM_LOAD( "dd3.j8",   0x040000, 0x40000, CRC(c28b53cd) SHA1(93d29669ec899fd5852f61b1d91d0a90cc30e192) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "mb7114h.38", 0x0000, 0x0100, CRC(113c7443) SHA1(7b0b13e9f0c219f6d436aeec06494734d1f4a599) )
ROM_END

ROM_START( ctribe )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 64k for cpu code */
	ROM_LOAD16_BYTE( "ic-26",      0x00001, 0x20000, CRC(c46b2e63) SHA1(86ace715dca48c78a46da1d102de47e5f948a86c) )
	ROM_LOAD16_BYTE( "ic-25",      0x00000, 0x20000, CRC(3221c755) SHA1(0f6fe5cd6947f6547585eedb7fc5e6af8544b1f7) )
	ROM_LOAD16_BYTE( "ct_ep2.rom", 0x40001, 0x10000, CRC(8c2c6dbd) SHA1(b99b9be6e0bdc8340fedd258819c4df587926a84) )
	/* No EVEN rom! */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu code */
	ROM_LOAD( "ct_ep4.rom",   0x00000, 0x8000, CRC(4346de13) SHA1(67c6de90ba31a325f03e64d28c9391a315ee359c) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ct_mr7.rom",  0x000000, 0x40000, CRC(a8b773f1) SHA1(999e41dfeb3fb937da769c4a33bb29bf4076dc63) )    /* Background */
	ROM_LOAD( "ct_mr6.rom",  0x040000, 0x40000, CRC(617530fc) SHA1(b9155ed0ae1437bf4d0b7a95e769bc05a820ecec) )
	ROM_LOAD( "ct_mr5.rom",  0x080000, 0x40000, CRC(cef0a821) SHA1(c7a35048d5ebf3f09abf9d27f91d12adc03befeb) )
	ROM_LOAD( "ct_mr4.rom",  0x0c0000, 0x40000, CRC(b84fda09) SHA1(3ae0c0ec6c398dea17e248b017ea3e2f6c3571e1) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ct_mr3.rom",  0x000000, 0x80000, CRC(1ac2a461) SHA1(17436f5dcf29041ca5f470dfae538e4fc12153cc) )    /* Sprites */
	ROM_LOAD( "ct_ep5.rom",  0x080000, 0x10000, CRC(972faddb) SHA1(f2b211e8f8301667e6c9a3ce9612e39b16e66a67) )
	ROM_LOAD( "ct_mr2.rom",  0x100000, 0x80000, CRC(8c796707) SHA1(7417ad0413083876ed65a8612845ccb0d2717530) )
	ROM_LOAD( "ct_ep6.rom",  0x180000, 0x10000, CRC(eb3ab374) SHA1(db66cb7976c111fa76a3a211e96ad1d7b78ce0ad) )
	ROM_LOAD( "ct_mr1.rom",  0x200000, 0x80000, CRC(1c9badbd) SHA1(d28f6d684d88448eaa3feae0bba2f5a836d89bd7) )
	ROM_LOAD( "ct_ep7.rom",  0x280000, 0x10000, CRC(c602ac97) SHA1(44440739636b684c6dcac837f59664120c9ba5f3) )
	ROM_LOAD( "ct_mr0.rom",  0x300000, 0x80000, CRC(ba73c49e) SHA1(830099027ede1f7c56bb0bf3cdef3018b92e0b87) )
	ROM_LOAD( "ct_ep8.rom",  0x380000, 0x10000, CRC(4da1d8e5) SHA1(568e9e8d00f1b1ca27c28df5fc0ffc74ad91da7e) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* ADPCM Samples */
	ROM_LOAD( "ct_mr8.rom",   0x020000, 0x20000, CRC(9963a6be) SHA1(b09b8f52b7fe5ceac34bc7d70c235d60d808fcbf) )
	ROM_CONTINUE(			  0x000000, 0x20000 )
ROM_END

ROM_START( ctribeb )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )	/* 64k for cpu code */
	ROM_LOAD16_BYTE( "ct_ep1.rom", 0x00001, 0x20000, CRC(9cfa997f) SHA1(ee49b4b9e9cd29616f244fdf3912ef743e2404ce) )
	ROM_LOAD16_BYTE( "ct_ep3.rom", 0x00000, 0x20000, CRC(2ece8681) SHA1(17ee2ceb893e2eb08fa4cabcdebcec02bee16cda) )
	ROM_LOAD16_BYTE( "ct_ep2.rom", 0x40001, 0x10000, CRC(8c2c6dbd) SHA1(b99b9be6e0bdc8340fedd258819c4df587926a84) )
	/* No EVEN rom! */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for sound cpu code */
	ROM_LOAD( "ct_ep4.rom",   0x00000, 0x8000, CRC(4346de13) SHA1(67c6de90ba31a325f03e64d28c9391a315ee359c) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "ct_mr7.rom",  0x000000, 0x40000, CRC(a8b773f1) SHA1(999e41dfeb3fb937da769c4a33bb29bf4076dc63) )    /* Background */
	ROM_LOAD( "ct_mr6.rom",  0x040000, 0x40000, CRC(617530fc) SHA1(b9155ed0ae1437bf4d0b7a95e769bc05a820ecec) )
	ROM_LOAD( "ct_mr5.rom",  0x080000, 0x40000, CRC(cef0a821) SHA1(c7a35048d5ebf3f09abf9d27f91d12adc03befeb) )
	ROM_LOAD( "ct_mr4.rom",  0x0c0000, 0x40000, CRC(b84fda09) SHA1(3ae0c0ec6c398dea17e248b017ea3e2f6c3571e1) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "ct_mr3.rom",  0x000000, 0x80000, CRC(1ac2a461) SHA1(17436f5dcf29041ca5f470dfae538e4fc12153cc) )    /* Sprites */
	ROM_LOAD( "ct_ep5.rom",  0x080000, 0x10000, CRC(972faddb) SHA1(f2b211e8f8301667e6c9a3ce9612e39b16e66a67) )
	ROM_LOAD( "ct_mr2.rom",  0x100000, 0x80000, CRC(8c796707) SHA1(7417ad0413083876ed65a8612845ccb0d2717530) )
	ROM_LOAD( "ct_ep6.rom",  0x180000, 0x10000, CRC(eb3ab374) SHA1(db66cb7976c111fa76a3a211e96ad1d7b78ce0ad) )
	ROM_LOAD( "ct_mr1.rom",  0x200000, 0x80000, CRC(1c9badbd) SHA1(d28f6d684d88448eaa3feae0bba2f5a836d89bd7) )
	ROM_LOAD( "ct_ep7.rom",  0x280000, 0x10000, CRC(c602ac97) SHA1(44440739636b684c6dcac837f59664120c9ba5f3) )
	ROM_LOAD( "ct_mr0.rom",  0x300000, 0x80000, CRC(ba73c49e) SHA1(830099027ede1f7c56bb0bf3cdef3018b92e0b87) )
	ROM_LOAD( "ct_ep8.rom",  0x380000, 0x10000, CRC(4da1d8e5) SHA1(568e9e8d00f1b1ca27c28df5fc0ffc74ad91da7e) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 )	/* ADPCM Samples */
	ROM_LOAD( "ct_mr8.rom",   0x020000, 0x20000, CRC(9963a6be) SHA1(b09b8f52b7fe5ceac34bc7d70c235d60d808fcbf) )
	ROM_CONTINUE(			  0x000000, 0x20000 )
ROM_END

/**************************************************************************/

GAMEX( 1990, ddragon3, 0,		 ddragon3, ddragon3, 0, ROT0, "Technos", "Double Dragon 3 - The Rosetta Stone (US)", GAME_NO_COCKTAIL )
GAMEX( 1990, ddrago3b, ddragon3, ddrago3b, ddrago3b, 0, ROT0, "bootleg", "Double Dragon 3 - The Rosetta Stone (bootleg)", GAME_NO_COCKTAIL )
GAMEX( 1990, ctribe,   0,		 ctribe,   ctribe,	 0, ROT0, "Technos", "The Combatribes (US)", GAME_NO_COCKTAIL )
GAMEX( 1990, ctribeb,  ctribe,	 ctribe,   ctribe,	 0, ROT0, "bootleg", "The Combatribes (bootleg)", GAME_NO_COCKTAIL )

