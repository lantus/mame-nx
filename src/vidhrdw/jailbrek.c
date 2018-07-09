#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *jailbrek_scroll_x;
unsigned char *jailbrek_scroll_dir;


PALETTE_INIT( jailbrek )
{
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])
	int i;

	for ( i = 0; i < Machine->drv->total_colors; i++ )
	{
		int bit0,bit1,bit2,bit3,r,g,b;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		bit0 = (color_prom[0] >> 4) & 0x01;
		bit1 = (color_prom[0] >> 5) & 0x01;
		bit2 = (color_prom[0] >> 6) & 0x01;
		bit3 = (color_prom[0] >> 7) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	color_prom += Machine->drv->total_colors;

	for ( i = 0; i < TOTAL_COLORS(0); i++ )
		COLOR(0,i) = ( *color_prom++ ) + 0x10;

	for ( i = 0; i < TOTAL_COLORS(1); i++ )
		COLOR(1,i) = *color_prom++;
}

VIDEO_START( jailbrek )
{
	if ( ( dirtybuffer = auto_malloc( videoram_size ) ) == 0 )
		return 1;
	memset( dirtybuffer, 1, videoram_size );

	if ( ( tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width * 2,Machine->drv->screen_height) ) == 0 )
		return 1;

	return 0;
}

static void drawsprites( struct mame_bitmap *bitmap )
{
	int i;

	for ( i = 0; i < spriteram_size; i += 4 ) {
		int tile, color, sx, sy, flipx, flipy;

		/* attributes = ?tyxcccc */

		sx = spriteram[i+2] - ( ( spriteram[i+1] & 0x80 ) << 1 );
		sy = spriteram[i+3];
		tile = spriteram[i] + ( ( spriteram[i+1] & 0x40 ) << 2 );
		flipx = spriteram[i+1] & 0x10;
		flipy = spriteram[i+1] & 0x20;
		color = spriteram[i+1] & 0x0f;

		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[1],
				tile,color,
				flipx,flipy,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_COLOR,0);
	}
}

VIDEO_UPDATE( jailbrek )
{
	int i;

	if ( get_vh_global_attribute_changed() )
		memset( dirtybuffer, 1, videoram_size );

	for ( i = 0; i < videoram_size; i++ )
	{
		if ( dirtybuffer[i] ) {
			int sx,sy, code;

			dirtybuffer[i] = 0;

			sx = ( i % 64 );
			sy = ( i / 64 );

			code = videoram[i] + ( ( colorram[i] & 0xc0 ) << 2 );

			if (flip_screen)
			{
				sx = 63 - sx;
				sy = 31 - sy;
			}

			drawgfx(tmpbitmap,Machine->gfx[0],
					code,
					colorram[i] & 0x0f,
					flip_screen,flip_screen,
					sx*8,sy*8,
					0,TRANSPARENCY_NONE,0);
		}
	}

	{
		int scrollx[32];

		if (flip_screen)
		{
			for ( i = 0; i < 32; i++ )
				scrollx[i] = 256 + ( ( jailbrek_scroll_x[i+32] << 8 ) + jailbrek_scroll_x[i] );
		}
		else
		{
			for ( i = 0; i < 32; i++ )
				scrollx[i] = -( ( jailbrek_scroll_x[i+32] << 8 ) + jailbrek_scroll_x[i] );
		}

		/* added support for vertical scrolling (credits).  23/1/2002  -BR */
		if( jailbrek_scroll_dir[0x00] & 0x04 )  /* bit 2 appears to be horizontal/vertical scroll control */
			copyscrollbitmap(bitmap,tmpbitmap,0,0,32,scrollx,&Machine->visible_area,TRANSPARENCY_NONE,0);
		else
			copyscrollbitmap(bitmap,tmpbitmap,32,scrollx,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}

	drawsprites( bitmap );
}
