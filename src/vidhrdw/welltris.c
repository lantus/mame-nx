#include "driver.h"
#include "vidhrdw/generic.h"


extern data16_t *welltris_charvideoram;
static struct tilemap *char_tilemap;
static unsigned char gfxbank[8];
static UINT16 charpalettebank;
extern data16_t *welltris_spriteram;
extern size_t welltris_spriteram_size;
extern data16_t *welltris_pixelram;

/* Sprite Drawing is pretty much the same as fromance.c */
static void wel_draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	UINT8 zoomtable[16] = { 0,7,14,20,25,30,34,38,42,46,49,52,54,57,59,61 };
	int offs;

	/* draw the sprites */
	for (offs = 0; offs < 0x200; offs += 4)
	{
		int data2 = welltris_spriteram[offs + 2];

		if (data2 & 0x80)
		{
			int data0 = welltris_spriteram[offs + 0];
			int data1 = welltris_spriteram[offs + 1];
			int data3 = welltris_spriteram[offs + 3];
			int code = data3 & 0xffff;
			int color = data2 & 0x0f;
			int y = (data0 & 0x1ff) - 8;
			int x = (data1 & 0x1ff) - 8;
			int yzoom = (data0 >> 12) & 15;
			int xzoom = (data1 >> 12) & 15;
			int zoomed = (xzoom | yzoom);
			int ytiles = ((data2 >> 12) & 7) + 1;
			int xtiles = ((data2 >> 8) & 7) + 1;
			int yflip = (data2 >> 15) & 1;
			int xflip = (data2 >> 11) & 1;
			int xt, yt;

			/* compute the zoom factor -- stolen from aerofgt.c */
			xzoom = 16 - zoomtable[xzoom] / 8;
			yzoom = 16 - zoomtable[yzoom] / 8;

			/* wrap around */
			if (x > Machine->visible_area.max_x) x -= 0x200;
			if (y > Machine->visible_area.max_y) y -= 0x200;

			/* normal case */
			if (!xflip && !yflip)
			{
				for (yt = 0; yt < ytiles; yt++)
					for (xt = 0; xt < xtiles; xt++, code++)
						if (!zoomed)
							drawgfx(bitmap, Machine->gfx[1], code, color, 0, 0,
									x + xt * 16, y + yt * 16, cliprect, TRANSPARENCY_PEN, 15);
						else
					drawgfxzoom(bitmap, Machine->gfx[1], code, color, 0, 0,
									x + xt * xzoom, y + yt * yzoom, cliprect, TRANSPARENCY_PEN, 15,
									0x1000 * xzoom, 0x1000 * yzoom);
			}

			/* xflipped case */
			else if (xflip && !yflip)
			{
				for (yt = 0; yt < ytiles; yt++)
					for (xt = 0; xt < xtiles; xt++, code++)
						if (!zoomed)
							drawgfx(bitmap, Machine->gfx[1], code, color, 1, 0,
									x + (xtiles - 1 - xt) * 16, y + yt * 16, cliprect, TRANSPARENCY_PEN, 15);
						else
							drawgfxzoom(bitmap, Machine->gfx[1], code, color, 1, 0,
									x + (xtiles - 1 - xt) * xzoom, y + yt * yzoom, cliprect, TRANSPARENCY_PEN, 15,
									0x1000 * xzoom, 0x1000 * yzoom);
			}

			/* yflipped case */
			else if (!xflip && yflip)
			{
				for (yt = 0; yt < ytiles; yt++)
					for (xt = 0; xt < xtiles; xt++, code++)
						if (!zoomed)
							drawgfx(bitmap, Machine->gfx[1], code, color, 0, 1,
									x + xt * 16, y + (ytiles - 1 - yt) * 16, cliprect, TRANSPARENCY_PEN, 15);
						else
							drawgfxzoom(bitmap, Machine->gfx[1], code, color, 0, 1,
									x + xt * xzoom, y + (ytiles - 1 - yt) * yzoom, cliprect, TRANSPARENCY_PEN, 15,
									0x1000 * xzoom, 0x1000 * yzoom);
			}

			/* x & yflipped case */
			else
			{
				for (yt = 0; yt < ytiles; yt++)
					for (xt = 0; xt < xtiles; xt++, code++)
						if (!zoomed)
							drawgfx(bitmap, Machine->gfx[1], code, color, 1, 1,
									x + (xtiles - 1 - xt) * 16, y + (ytiles - 1 - yt) * 16, cliprect, TRANSPARENCY_PEN, 15);
						else
							drawgfxzoom(bitmap, Machine->gfx[1], code, color, 1, 1,
									x + (xtiles - 1 - xt) * xzoom, y + (ytiles - 1 - yt) * yzoom, cliprect, TRANSPARENCY_PEN, 15,
									0x1000 * xzoom, 0x1000 * yzoom);
			}
		}
	}
}

static void setbank(struct tilemap *tmap,int num,int bank)
{
	if (gfxbank[num] != bank)
	{
		gfxbank[num] = bank;
		tilemap_mark_all_tiles_dirty(tmap);
	}
}


/* Not really enough evidence here */

WRITE16_HANDLER( welltris_palette_bank_w )
{
	if (ACCESSING_LSB)
	{
		if (charpalettebank != (data & 0x03) )
		{
			charpalettebank = (data & 0x03) ;
			tilemap_mark_all_tiles_dirty(char_tilemap);
		}

		flip_screen_set(data & 0x80);
	}
}



WRITE16_HANDLER( welltris_gfxbank_w )
{
	if (ACCESSING_LSB)
	{
		setbank(char_tilemap,0,(data & 0xf0) >> 4);
		setbank(char_tilemap,1,data & 0x0f);
	}
}

static void get_welltris_tile_info(int tile_index)
{
	UINT16 code = welltris_charvideoram[tile_index];
	int bank = (code & 0x1000) >> 12;
	SET_TILE_INFO(
			0,
			(code & 0x0fff) + (gfxbank[bank] << 12),
			((code & 0xe000) >> 13)+8*charpalettebank,
			0)
}


WRITE16_HANDLER( welltris_charvideoram_w )
{
	int oldword = welltris_charvideoram[offset];
	COMBINE_DATA(&welltris_charvideoram[offset]);
	if (oldword != welltris_charvideoram[offset])
		tilemap_mark_tile_dirty(char_tilemap,offset);
}

VIDEO_START( welltris )
{

	char_tilemap = tilemap_create(get_welltris_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);
	tilemap_set_transparent_pen(char_tilemap,15);


	return 0;
}



void welltris_drawbackground(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	const int xoffset = 8;
	const int yoffset = 8;

	int x,y;

	for (x=cliprect->min_x;x<cliprect->max_x/2;x++) {

	for  (y=cliprect->min_y;y<=cliprect->max_y;y++) {

		int pixdata;

		pixdata = welltris_pixelram[ ((x+xoffset)&0xff)+((y+yoffset)&0xff)*256];

		plot_pixel(bitmap,x*2,  y,(pixdata >> 8)+0x500);
		plot_pixel(bitmap,x*2+1,  y,(pixdata & 0xff)+0x500);


		}
	}
}

VIDEO_UPDATE( welltris )
{

	welltris_drawbackground(bitmap,cliprect);
	tilemap_draw(bitmap,cliprect,char_tilemap,0,0);
//	usrintf_showmessage("%04X %04X %04X %04X %04X %04X %04X %04X",welltris_registers[0],welltris_registers[1],welltris_registers[2],welltris_registers[3],welltris_registers[4],welltris_registers[5],welltris_registers[6],welltris_registers[7]);

	wel_draw_sprites(bitmap,cliprect);

}
