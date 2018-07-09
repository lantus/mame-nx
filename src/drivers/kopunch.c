#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *bsvideoram;
size_t bsvideoram_size;

static INT8 scroll[2];
static int gfxbank,gfxflip;


PALETTE_INIT( kopunch )
{
	int i;


	color_prom+=24;	/* first 24 colors are black */
	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;


		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);
		color_prom++;
	}
}


WRITE_HANDLER( kopunch_scroll_w )
{
	scroll[offset] = data;
}

static WRITE_HANDLER( kopunch_gfxbank_w )
{
//	usrintf_showmessage("bank = %02x",data);

	gfxbank = data & 0x07;
	gfxflip = data & 0x08;
}

VIDEO_UPDATE( kopunch )
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					0,
					0,0,
					8*sx,8*sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);


	for (offs = bsvideoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy;


		sx = offs % 16;
		sy = offs / 16;

		drawgfx(bitmap,Machine->gfx[1],
				(bsvideoram[offs] & 0x7f) + 128 * gfxbank,
				0,
				0,gfxflip,
				8*(sx+8)+scroll[0],8*(8+(gfxflip ? 15-sy : sy))+scroll[1],
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}


INTERRUPT_GEN( kopunch_interrupt )
{
	if (cpu_getiloops() == 0)
	{
		if (~input_port_1_r(0) & 0x80)	/* coin 1 */
		{
			cpu_set_irq_line_and_vector(0,0,HOLD_LINE,0xf7);	/* RST 30h */
			return;
		}
		else if (~input_port_1_r(0) & 0x08)	/* coin 2 */
		{
			cpu_set_irq_line_and_vector(0,0,HOLD_LINE,0xef);	/* RST 28h */
			return;
		}
	}

	cpu_set_irq_line_and_vector(0,0,HOLD_LINE,0xff);	/* RST 38h */
}

static READ_HANDLER( kopunch_in_r )
{
	/* port 31 + low 3 bits of port 32 contain the punch strength */
	if (offset == 0)
		return rand();
	else
		return (rand() & 0x07) | input_port_1_r(0);
}

static WRITE_HANDLER( kopunch_lamp_w )
{
	set_led_status(0,~data & 0x80);

//	if ((data & 0x7f) != 0x7f)
//		usrintf_showmessage("port 38 = %02x",data);
}

static WRITE_HANDLER( kopunch_coin_w )
{
	coin_counter_w(0,~data & 0x80);
	coin_counter_w(1,~data & 0x40);

//	if ((data & 0x3f) != 0x3f)
//		usrintf_showmessage("port 34 = %02x",data);
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x23ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x6000, 0x63ff, videoram_w, &videoram, &videoram_size },
	{ 0x7000, 0x70ff, MWA_RAM, &bsvideoram, &bsvideoram_size },
	{ 0x7100, 0x73ff, MWA_RAM },	/* more video ram? */
MEMORY_END

static READ_HANDLER( pip_r )
{
	return rand();
}

static PORT_READ_START( readport )
	{ 0x30, 0x30, input_port_0_r },
	{ 0x31, 0x32, kopunch_in_r },
	{ 0x3a, 0x3a, input_port_2_r },
	{ 0x3e, 0x3e, input_port_3_r },
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x34, 0x34, kopunch_coin_w },
	{ 0x38, 0x38, kopunch_lamp_w },
	{ 0x3c, 0x3d, kopunch_scroll_w },
	{ 0x3e, 0x3e, kopunch_gfxbank_w },
PORT_END




INPUT_PORTS_START( kopunch )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )

	PORT_START
	PORT_BIT( 0x07, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* punch strength (high 3 bits) */
	PORT_BIT_IMPULSE( 0x08, IP_ACTIVE_LOW, IPT_COIN2, 1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT_IMPULSE( 0x80, IP_ACTIVE_LOW, IPT_COIN1, 1 )

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
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 1 },
	{ REGION_GFX2, 0, &charlayout, 0, 1 },
	{ -1 } /* end of array */
};



static MACHINE_DRIVER_START( kopunch )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)	/* 4 MHz ???? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(kopunch_interrupt,4)	/* ??? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8)

	MDRV_PALETTE_INIT(kopunch)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(kopunch)

	/* sound hardware */
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( kopunch )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "epr1105.x",    0x0000, 0x1000, CRC(34ef5e79) )
	ROM_LOAD( "epr1106.x",    0x1000, 0x1000, CRC(25a5c68b) )

	ROM_REGION( 0x1800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "epr1102",      0x0000, 0x0800, CRC(8a52de96) )
	ROM_LOAD( "epr1103",      0x0800, 0x0800, CRC(bae5e054) )
	ROM_LOAD( "epr1104",      0x1000, 0x0800, CRC(7b119a0e) )

	ROM_REGION( 0x6000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "epr1107",      0x0000, 0x1000, CRC(ca00244d) )
	ROM_LOAD( "epr1108",      0x1000, 0x1000, CRC(cc17c5ed) )
	ROM_LOAD( "epr1110",      0x2000, 0x1000, CRC(ae0aff15) )
	ROM_LOAD( "epr1109",      0x3000, 0x1000, CRC(625446ba) )
	ROM_LOAD( "epr1112",      0x4000, 0x1000, CRC(ef6994df) )
	ROM_LOAD( "epr1111",      0x5000, 0x1000, CRC(28530ec9) )

	ROM_REGION( 0x0060, REGION_PROMS, 0 )
	ROM_LOAD( "epr1101",      0x0000, 0x0020, CRC(15600f5d) )	/* palette */
	ROM_LOAD( "epr1099",      0x0020, 0x0020, CRC(fc58c456) )	/* unknown */
	ROM_LOAD( "epr1100",      0x0040, 0x0020, CRC(bedb66b1) )	/* unknown */
ROM_END


static DRIVER_INIT( kopunch )
{
	unsigned char *rom = memory_region(REGION_CPU1);

	/* It looks like there is a security chip, that changes instruction of the form:
		0334: 3E 0C       ld   a,$0C
		0336: 30 FB       jr   nc,$0333
	   into something else (maybe just a nop) with the effect of resuming execution
	   from the operand of the JR  NC instruction (in the example above, 0337).
	   For now, I'm just patching the affected instructions. */

	rom[0x119] = 0;
	rom[0x336] = 0;
	rom[0x381] = 0;
	rom[0xf0b] = 0;
	rom[0xf33] = 0;
}


GAMEX( 1981, kopunch, 0, kopunch, kopunch, kopunch, ROT270, "Sega", "KO Punch", GAME_NO_SOUND )
