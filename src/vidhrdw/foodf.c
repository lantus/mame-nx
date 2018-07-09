/***************************************************************************

	Atari Food Fight hardware

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "foodf.h"


static UINT8 pf_flip;


/*************************************
 *
 *	Cocktail flip
 *
 *************************************/

void foodf_set_flip(int flip)
{
	if (flip != pf_flip)
	{
		pf_flip = flip;
		memset(dirtybuffer, 1, videoram_size / 2);
	}
}



/*************************************
 *
 *	Playfield RAM write
 *
 *************************************/

WRITE16_HANDLER( foodf_playfieldram_w )
{
	int oldword = videoram16[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);

	if (oldword != newword)
	{
		videoram16[offset] = newword;
		dirtybuffer[offset] = 1;
	}
}



/*************************************
 *
 *	Palette RAM write
 *
 *************************************/

WRITE16_HANDLER( foodf_paletteram_w )
{
	int newword, r, g, b, bit0, bit1, bit2;

	COMBINE_DATA(&paletteram16[offset]);
	newword = paletteram16[offset];

	/* only the bottom 8 bits are used */
	/* red component */
	bit0 = (newword >> 0) & 0x01;
	bit1 = (newword >> 1) & 0x01;
	bit2 = (newword >> 2) & 0x01;
	r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	/* green component */
	bit0 = (newword >> 3) & 0x01;
	bit1 = (newword >> 4) & 0x01;
	bit2 = (newword >> 5) & 0x01;
	g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	/* blue component */
	bit0 = 0;
	bit1 = (newword >> 6) & 0x01;
	bit2 = (newword >> 7) & 0x01;
	b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	palette_set_color(offset, r, g, b);
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

VIDEO_UPDATE( foodf )
{
	int offs;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size / 2 - 1; offs >= 0; offs--)
		if (dirtybuffer[offs])
		{
			int data = videoram16[offs];
			int color = (data >> 8) & 0x3f;
			int pict = (data & 0xff) | ((data >> 7) & 0x100);
			int sx,sy;

			dirtybuffer[offs] = 0;

			sx = (offs / 32 + 1) % 32;
			sy = offs % 32;

			drawgfx(tmpbitmap, Machine->gfx[0], pict, color, pf_flip, pf_flip,
					8 * sx, 8 * sy, &Machine->visible_area, TRANSPARENCY_NONE, 0);
		}

	/* copy that as the background */
	copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);

	/* walk the motion object list. */
	for (offs = 0; offs < spriteram_size / 4; offs += 2)
	{
		int data1 = spriteram16[offs];
		int data2 = spriteram16[offs+1];

		int pict = data1 & 0xff;
		int color = (data1 >> 8) & 0x1f;
		int xpos = (data2 >> 8) & 0xff;
		int ypos = (0xff - data2 - 16) & 0xff;
		int hflip = (data1 >> 15) & 1;
		int vflip = (data1 >> 14) & 1;

		drawgfx(bitmap, Machine->gfx[1], pict, color, hflip, vflip,
				xpos, ypos, cliprect, TRANSPARENCY_PEN, 0);

		/* draw again with wraparound (needed to get the end of level animation right) */
		drawgfx(bitmap, Machine->gfx[1], pict, color, hflip, vflip,
				xpos - 256, ypos, cliprect, TRANSPARENCY_PEN, 0);
	}
}
