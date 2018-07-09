/******************************************************************************

	Video Hardware for Video System Mahjong series.

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 2000/06/10 -

******************************************************************************/

#include "driver.h"


static data8_t *ojankohs_videoram;
static data8_t *ojankohs_colorram;
static data8_t *ojankohs_paletteram;
static int ojankohs_gfxreg;
static int ojankohs_flipscreen;
static int ojankohs_scrollx, ojankohs_scrolly;
static struct tilemap *ojankohs_tilemap;


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

INLINE void ojankohs_get_tile_info(int tile_index)
{
	int tile, color;

	tile = (ojankohs_videoram[tile_index] | ((ojankohs_colorram[tile_index] & 0x0f) << 8));
	color = ((ojankohs_colorram[tile_index] & 0xe0) >> 5);

	if (ojankohs_colorram[tile_index] & 0x10)
	{
		tile |= ((ojankohs_gfxreg & 0x07) << 12);
		color |= ((ojankohs_gfxreg & 0xe0) >> 2);
	}

	SET_TILE_INFO(0, tile, color, 0)
}

INLINE void ojankoy_get_tile_info(int tile_index)
{
	int tile, color, flipx, flipy;

	tile = (ojankohs_videoram[tile_index] | (ojankohs_videoram[tile_index + 0x1000] << 8));
	color = (ojankohs_colorram[tile_index] & 0x3f);
	flipx = (((ojankohs_colorram[tile_index] & 0x40) >> 6) ? TILEMAP_FLIPX : 0);
	flipy = (((ojankohs_colorram[tile_index] & 0x80) >> 7) ? TILEMAP_FLIPY : 0);

	SET_TILE_INFO(0, tile, color, (flipx | flipy))
}


/******************************************************************************


******************************************************************************/
PALETTE_INIT( ojankoy )
{
	int i;
	int bit0, bit1, bit2, bit3, bit4, r, g, b;

	for (i = 0; i < Machine->drv->total_colors; i++)
	{
		bit0 = (color_prom[0] >> 2) & 0x01;
		bit1 = (color_prom[0] >> 3) & 0x01;
		bit2 = (color_prom[0] >> 4) & 0x01;
		bit3 = (color_prom[0] >> 5) & 0x01;
		bit4 = (color_prom[0] >> 6) & 0x01;
		r = 0x08 * bit0 + 0x11 * bit1 + 0x21 * bit2 + 0x43 * bit3 + 0x82 * bit4;
		bit0 = (color_prom[Machine->drv->total_colors] >> 5) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 6) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 7) & 0x01;
		bit3 = (color_prom[0] >> 0) & 0x01;
		bit4 = (color_prom[0] >> 1) & 0x01;
		g = 0x08 * bit0 + 0x11 * bit1 + 0x21 * bit2 + 0x43 * bit3 + 0x82 * bit4;
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		bit4 = (color_prom[Machine->drv->total_colors] >> 4) & 0x01;
		b = 0x08 * bit0 + 0x11 * bit1 + 0x21 * bit2 + 0x43 * bit3 + 0x82 * bit4;

		palette_set_color(i,r,g,b);
		color_prom++;
	}
}

/******************************************************************************


******************************************************************************/
READ_HANDLER( ojankohs_palette_r )
{
	return ojankohs_paletteram[offset];
}

WRITE_HANDLER( ojankohs_palette_w )
{
	int r, g, b;

	ojankohs_paletteram[offset] = data;

	offset &= 0x7fe;

	r = ((ojankohs_paletteram[offset + 0] & 0x7c) >> 2);
	g = (((ojankohs_paletteram[offset + 0] & 0x03) << 3) |
		((ojankohs_paletteram[offset + 1] & 0xe0) >> 5));
	b = ((ojankohs_paletteram[offset + 1] & 0x1f) >> 0);

	r = ((r << 3) | (r >> 2));
	g = ((g << 3) | (g >> 2));
	b = ((b << 3) | (b >> 2));

	palette_set_color((offset >> 1), r, g, b);
}

/******************************************************************************


******************************************************************************/

READ_HANDLER( ojankohs_videoram_r )
{
	return ojankohs_videoram[offset];
}

WRITE_HANDLER( ojankohs_videoram_w )
{
	if (ojankohs_videoram[offset] != data)
	{
		ojankohs_videoram[offset] = data;
		tilemap_mark_tile_dirty(ojankohs_tilemap, offset);
	}
}

READ_HANDLER( ojankohs_colorram_r )
{
	return ojankohs_colorram[offset];
}

WRITE_HANDLER( ojankohs_colorram_w )
{
//	if (ojankohs_colorram[offset] != data)
	{
		ojankohs_colorram[offset] = data;
		tilemap_mark_tile_dirty(ojankohs_tilemap, offset);
	}
}

WRITE_HANDLER( ojankohs_gfxreg_w )
{
	if (ojankohs_gfxreg != data)
	{
		ojankohs_gfxreg = data;
		tilemap_mark_all_tiles_dirty(ojankohs_tilemap);
	}
}

WRITE_HANDLER( ojankohs_flipscreen_w )
{
	if (ojankohs_flipscreen != (data & 0x01))
	{
		ojankohs_flipscreen = (data & 0x01);

		tilemap_set_flip(ALL_TILEMAPS, ojankohs_flipscreen ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0);

		if (ojankohs_flipscreen)
		{
			ojankohs_scrollx = -0xe0;
			ojankohs_scrolly = -0x20;
		}
		else
		{
			ojankohs_scrollx = 0;
			ojankohs_scrolly = 0;
		}
	}
}

/******************************************************************************

  Start the video hardware emulation.

******************************************************************************/

VIDEO_START( ojankohs )
{
	ojankohs_tilemap = tilemap_create(ojankohs_get_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 4, 64, 64);

	ojankohs_videoram = auto_malloc(0x2000);
	ojankohs_colorram = auto_malloc(0x1000);
	ojankohs_paletteram = auto_malloc(0x800);

	if (!ojankohs_tilemap || !ojankohs_videoram || !ojankohs_colorram || !ojankohs_paletteram) return 1;

	return 0;
}

VIDEO_START( ojankoy )
{
	ojankohs_tilemap = tilemap_create(ojankoy_get_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 4, 64, 64);

	ojankohs_videoram = auto_malloc(0x2000);
	ojankohs_colorram = auto_malloc(0x1000);
	ojankohs_paletteram = auto_malloc(0x800);

	if (!ojankohs_tilemap || !ojankohs_videoram || !ojankohs_colorram || !ojankohs_paletteram) return 1;

	return 0;
}

/******************************************************************************

  Display refresh

******************************************************************************/

VIDEO_UPDATE( ojankohs )
{
	tilemap_set_scrollx(ojankohs_tilemap, 0, ojankohs_scrollx);
	tilemap_set_scrolly(ojankohs_tilemap, 0, ojankohs_scrolly);

	tilemap_draw(bitmap,cliprect, ojankohs_tilemap, 0, 0);
}
