/***************************************************************************

	Atari Canyon Bomber hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *bg_tilemap;

WRITE_HANDLER( canyon_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static void get_bg_tile_info(int tile_index)
{
	int code = videoram[tile_index] & 0x3f;
	int color = (videoram[tile_index] & 0x80) >> 7;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( canyon )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

static void canyon_draw_sprites( struct mame_bitmap *bitmap )
{
	int offs;

	for (offs = 0; offs < 2; offs++)
	{
		int sx = 27 * 8 - spriteram[(offs * 2) + 1];
		int sy = 30 * 8 - spriteram[(offs * 2) + 8];
		int attr = spriteram[(offs * 2) + 9];
		int code = (attr & 0x18) >> 3;
		int flipx = (attr & 0x80) ? 0 : 1;

        drawgfx(bitmap, Machine->gfx[1],
            code, offs,
			flipx, 0,
			sx, sy,
			&Machine->visible_area,
			TRANSPARENCY_PEN, 0);
	}
}

static void canyon_draw_bombs( struct mame_bitmap *bitmap )
{
	int offs;

	for (offs = 2; offs < 4; offs++)
	{
		int sx = 31 * 8 - spriteram[(offs * 2) + 1];
		int sy = 31 * 8 - spriteram[(offs * 2) + 8];

        drawgfx(bitmap, Machine->gfx[2],
			0, offs,
			0, 0, 
			sx, sy,
			&Machine->visible_area,
			TRANSPARENCY_PEN, 0);
	}
}

VIDEO_UPDATE( canyon )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
	canyon_draw_sprites(bitmap);
	canyon_draw_bombs(bitmap);
}
