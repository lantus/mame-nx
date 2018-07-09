/***************************************************************************

  Video Hardware for Double Dragon 3

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "tilemap.h"

data16_t *ddragon3_bg_videoram16;
static data16_t ddragon3_bg_scrollx;
static data16_t ddragon3_bg_scrolly;

static data16_t ddragon3_bg_tilebase;
static data16_t old_ddragon3_bg_tilebase;

data16_t *ddragon3_fg_videoram16;
static data16_t ddragon3_fg_scrollx;
static data16_t ddragon3_fg_scrolly;
data16_t ddragon3_vreg;

static struct tilemap *background, *foreground;

/* scroll write function */
WRITE16_HANDLER( ddragon3_scroll16_w )
{
	switch (offset)
	{
	case 0: /* Scroll X, BG1 */
	COMBINE_DATA(&ddragon3_fg_scrollx);
	break;

	case 1: /* Scroll Y, BG1 */
	COMBINE_DATA(&ddragon3_fg_scrolly);
	break;

	case 2: /* Scroll X, BG0 */
	COMBINE_DATA(&ddragon3_bg_scrollx);
	break;

	case 3: /* Scroll Y, BG0 */
	COMBINE_DATA(&ddragon3_bg_scrolly);
	break;

	case 6: /* BG Tile Base */
	COMBINE_DATA(&ddragon3_bg_tilebase);
	ddragon3_bg_tilebase &= 0x1ff;
	return;

	default:  /* Unknown */
	logerror("OUTPUT c00[%02x] %02x \n", offset,data);
	break;
	}
}

/* background */
static void get_bg_tile_info(int tile_index)
{
	data16_t data = ddragon3_bg_videoram16[tile_index];
	SET_TILE_INFO(
			0,
			(data&0xfff) | ((ddragon3_bg_tilebase&1)<<12),
			((data&0xf000)>>12)+16,
			0)
}

WRITE16_HANDLER( ddragon3_bg_videoram16_w )
{
	data16_t oldword = ddragon3_bg_videoram16[offset];
	COMBINE_DATA(&ddragon3_bg_videoram16[offset]);
	if( oldword != ddragon3_bg_videoram16[offset] )
		tilemap_mark_tile_dirty(background,offset);
}

/* foreground */
static void get_fg_tile_info(int tile_index)
{
	data16_t data0 = ddragon3_fg_videoram16[2*tile_index];
	data16_t data1 = ddragon3_fg_videoram16[2*tile_index+1];
	SET_TILE_INFO(
			0,
			data1&0x1fff,
			data0&0xf,
			(data0&0x40) ? TILE_FLIPX : 0)
}

WRITE16_HANDLER( ddragon3_fg_videoram16_w )
{
	data16_t oldword = ddragon3_fg_videoram16[offset];
	COMBINE_DATA(&ddragon3_fg_videoram16[offset]);
	if( oldword != ddragon3_fg_videoram16[offset] )
		tilemap_mark_tile_dirty(foreground,offset/2);
}

/* start & stop */
VIDEO_START( ddragon3 )
{
	ddragon3_bg_tilebase = 0;
	old_ddragon3_bg_tilebase = -1;

	background = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,	   16,16,32,32);
	foreground = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);

	if (!background || !foreground)
		return 1;

	tilemap_set_transparent_pen(foreground,0);
	return 0;
}

/*
 * Sprite Format
 * ----------------------------------
 *
 * Word | Bit(s)		   | Use
 * -----+-fedcba9876543210-+----------------
 *	 0	| --------xxxxxxxx | ypos (signed)
 * -----+------------------+
 *	 1	| --------xxx----- | height
 *	 1	| -----------xx--- | yflip, xflip
 *	 1	| -------------x-- | msb x
 *	 1	| --------------x- | msb y?
 *	 1	| ---------------x | enable
 * -----+------------------+
 *	 2	| --------xxxxxxxx | tile number
 * -----+------------------+
 *	 3	| --------xxxxxxxx | bank
 * -----+------------------+
 *	 4	| ------------xxxx |color
 * -----+------------------+
 *	 5	| --------xxxxxxxx | xpos
 * -----+------------------+
 *	 6,7| unused
 */

static void draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	const struct GfxElement *gfx = Machine->gfx[1];
	data16_t *source = spriteram16;
	data16_t *finish = source + 0x800;

	while( source<finish )
	{
		data16_t attributes = source[1];
		if( attributes&0x01 )	/* enable */
		{
			int flipx = attributes&0x10;
			int flipy = attributes&0x08;
			int height = (attributes>>5)&0x7;

			int sy = source[0]&0xff;
			int sx = source[5]&0xff;
			int tile_number = source[2]&0xff;
			int color = source[4]&0xf;
			int bank = source[3]&0xff;
			int i;

			if (attributes&0x04) sx|=0x100;
			if (attributes&0x02) sy=239+(0x100-sy); else sy=240-sy;
			if (sx>0x17f) sx=0-(0x200-sx);

			tile_number += (bank*256);

			for( i=0; i<=height; i++ )
			{
				int tile_index = tile_number + i;

				drawgfx(bitmap,gfx,
					tile_index,
					color,
					flipx,flipy,
					sx,sy-i*16,
					cliprect,TRANSPARENCY_PEN,0);
			}
		}
		source+=8;
	}
}

VIDEO_UPDATE( ddragon3 )
{
	if( ddragon3_bg_tilebase != old_ddragon3_bg_tilebase )
	{
		old_ddragon3_bg_tilebase = ddragon3_bg_tilebase;
		tilemap_mark_all_tiles_dirty( background );
	}

	tilemap_set_scrolly( background, 0, ddragon3_bg_scrolly );
	tilemap_set_scrollx( background, 0, ddragon3_bg_scrollx );

	tilemap_set_scrolly( foreground, 0, ddragon3_fg_scrolly );
	tilemap_set_scrollx( foreground, 0, ddragon3_fg_scrollx );

	if (ddragon3_vreg & 0x40)
	{
		tilemap_draw( bitmap,cliprect, background, 0 ,0);
		tilemap_draw( bitmap,cliprect, foreground, 0 ,0);
		draw_sprites( bitmap,cliprect );
	}
	else
	{
		tilemap_draw( bitmap,cliprect, background, 0 ,0);
		draw_sprites( bitmap,cliprect );
		tilemap_draw( bitmap,cliprect, foreground, 0 ,0);
	}
}

VIDEO_UPDATE( ctribe )
{
	if( ddragon3_bg_tilebase != old_ddragon3_bg_tilebase )
	{
		old_ddragon3_bg_tilebase = ddragon3_bg_tilebase;
		tilemap_mark_all_tiles_dirty( background );
	}

	tilemap_set_scrolly( background, 0, ddragon3_bg_scrolly );
	tilemap_set_scrollx( background, 0, ddragon3_bg_scrollx );
	tilemap_set_scrolly( foreground, 0, ddragon3_fg_scrolly );
	tilemap_set_scrollx( foreground, 0, ddragon3_fg_scrollx );

	tilemap_draw( bitmap,cliprect, background, 0 ,0);
	tilemap_draw( bitmap,cliprect, foreground, 0 ,0);
	draw_sprites( bitmap,cliprect );
}

