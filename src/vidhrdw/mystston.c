/***************************************************************************

	vidhrdw.c

	Functions to emulate the video hardware of the machine.

	There are only a few differences between the video hardware of Mysterious
	Stones and Mat Mania. The tile bank select bit is different and the sprite
	selection seems to be different as well. Additionally, the palette is stored
	differently. I'm also not sure that the 2nd tile page is really used in
	Mysterious Stones.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *mystston_fgvideoram;
unsigned char *mystston_bgvideoram;

static int textcolor;
static struct tilemap *fg_tilemap, *bg_tilemap;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Mysterious Stones has both palette RAM and a PROM. The PROM is used for
  text.

***************************************************************************/

PALETTE_INIT( mystston )
{
	int i;

	/* first 24 colors are RAM */

	for (i = 0;i < 32;i++)
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

		palette_set_color(i+24,r,g,b);
		color_prom++;
	}
}


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

UINT32 get_memory_offset( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	return (num_cols - 1 - col) * num_rows + row;
}

static void get_fg_tile_info(int tile_index)
{
	int code;

	code = mystston_fgvideoram[tile_index] + ((mystston_fgvideoram[tile_index + 0x400] & 0x07) << 8);
	SET_TILE_INFO(
			0,
			code,
			textcolor,
			0)
}

static void get_bg_tile_info(int tile_index)
{
	int code;

	code = mystston_bgvideoram[tile_index] + ((mystston_bgvideoram[tile_index + 0x200] & 0x01) << 8);
	SET_TILE_INFO(
			1,
			code,
			0,
			(tile_index & 0x10) ? TILE_FLIPY : 0)	/* the right (lower) side of the screen is flipped */
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( mystston )
{
	fg_tilemap = tilemap_create(get_fg_tile_info,get_memory_offset,TILEMAP_TRANSPARENT, 8, 8,32,32);
	bg_tilemap = tilemap_create(get_bg_tile_info,get_memory_offset,TILEMAP_OPAQUE,     16,16,16,32);

	if (!fg_tilemap || !bg_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);

	return 0;
}


/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE_HANDLER( mystston_fgvideoram_w )
{
	mystston_fgvideoram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap,offset & 0x3ff);
}

WRITE_HANDLER( mystston_bgvideoram_w )
{
	mystston_bgvideoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap,offset & 0x1ff);
}


WRITE_HANDLER( mystston_scroll_w )
{
	tilemap_set_scrolly(bg_tilemap,0,data);
}


WRITE_HANDLER( mystston_2000_w )
{
	int new_textcolor;


	/* bits 0 and 1 are text color */
	new_textcolor = ((data & 0x01) << 1) | ((data & 0x02) >> 1);
	if (textcolor != new_textcolor)
	{
		tilemap_mark_all_tiles_dirty(fg_tilemap);
		textcolor = new_textcolor;
	}

	/* bits 4 and 5 are coin counters */
	coin_counter_w(0,data & 0x10);
	coin_counter_w(1,data & 0x20);

	/* bit 7 is screen flip */
	flip_screen_set(data & 0x80);

	/* other bits unused? */
	logerror("PC %04x: 2000 = %02x\n",activecpu_get_pc(),data);
}


/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int offs;


	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		if (spriteram[offs] & 0x01)
		{
			int sx,sy,flipx,flipy;


			sx = 240 - spriteram[offs+3];
			sy = (240 - spriteram[offs+2]) & 0xff;
			flipx = spriteram[offs] & 0x04;
			flipy = spriteram[offs] & 0x02;
			if (flip_screen)
			{
				sx = 240 - sx;
				sy = 240 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(bitmap,Machine->gfx[2],
					spriteram[offs+1] + ((spriteram[offs] & 0x10) << 4),
					(spriteram[offs] & 0x08) >> 3,
					flipx,flipy,
					sx,sy,
					cliprect,TRANSPARENCY_PEN,0);
		}
	}
}

VIDEO_UPDATE( mystston )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	draw_sprites(bitmap,cliprect);
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
}
