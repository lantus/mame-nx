/***************************************************************************

	Atari GT hardware

*****************************************************************************

	MO data has 12 bits total: MVID0-11
	MVID9-11 form the priority
	MVID0-9 form the color bits

	PF data has 13 bits total: PF.VID0-12
	PF.VID10-12 form the priority
	PF.VID0-9 form the color bits

	Upper bits come from the low 5 bits of the HSCROLL value in alpha RAM
	Playfield bank comes from low 2 bits of the VSCROLL value in alpha RAM
	For GX2, there are 4 bits of bank

****************************************************************************/


#include "driver.h"
#include "machine/atarigen.h"
#include "atarigt.h"

#define DEBUG_ATARIGT		0



/*************************************
 *
 *	Constants
 *
 *************************************/

#define CRAM_ENTRIES		0x4000
#define TRAM_ENTRIES		0x4000
#define MRAM_ENTRIES		0x8000



/*************************************
 *
 *	Globals we own
 *
 *************************************/

data16_t *atarigt_colorram;

UINT16 atarigt_motion_object_mask;



/*************************************
 *
 *	Statics
 *
 *************************************/

static struct mame_bitmap *pf_bitmap;

static UINT8 playfield_tile_bank;
static UINT8 playfield_color_bank;
static UINT16 playfield_xscroll;
static UINT16 playfield_yscroll;

static UINT32 tram_checksum;
static UINT16 *cram, *tram;
static UINT32 *mram;

static UINT32 *expanded_mram;

static UINT8 rshift, gshift, bshift;

#if DEBUG_ATARIGT
static void dump_video_memory(struct mame_bitmap *mo_bitmap, struct mame_bitmap *tm_bitmap);
#endif



/*************************************
 *
 *	Tilemap callbacks
 *
 *************************************/

static void get_alpha_tile_info(int tile_index)
{
	UINT16 data = atarigen_alpha32[tile_index / 2] >> (16 * (~tile_index & 1));
	int code = data & 0xfff;
	int color = (data >> 12) & 0x0f;
	int opaque = data & 0x8000;
	SET_TILE_INFO(1, code, color, opaque ? TILE_IGNORE_TRANSPARENCY : 0);
}


static void get_playfield_tile_info(int tile_index)
{
	UINT16 data = atarigen_playfield32[tile_index / 2] >> (16 * (~tile_index & 1));
	int code = (playfield_tile_bank << 12) | (data & 0xfff);
	int color = (data >> 12) & 7;
	SET_TILE_INFO(0, code, color, (data >> 15) & 1);
}


static UINT32 atarigt_playfield_scan(UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows)
{
	int bank = 1 - (col / (num_cols / 2));
	return bank * (num_rows * num_cols / 2) + row * (num_cols / 2) + (col % (num_cols / 2));
}



/*************************************
 *
 *	Video system start
 *
 *************************************/

VIDEO_START( atarigt )
{
	extern UINT32 direct_rgb_components[3];
	static const struct atarirle_desc modesc =
	{
		REGION_GFX3,/* region where the GFX data lives */
		256,		/* number of entries in sprite RAM */
		0,			/* left clip coordinate */
		0,			/* right clip coordinate */

		0x0000,		/* base palette entry */
		0x1000,		/* maximum number of colors */

		{{ 0x7fff,0,0,0,0,0,0,0 }},	/* mask for the code index */
		{{ 0,0x0ff0,0,0,0,0,0,0 }},	/* mask for the color */
		{{ 0,0,0xffc0,0,0,0,0,0 }},	/* mask for the X position */
		{{ 0,0,0,0xffc0,0,0,0,0 }},	/* mask for the Y position */
		{{ 0,0,0,0,0xffff,0,0,0 }},	/* mask for the scale factor */
		{{ 0x8000,0,0,0,0,0,0,0 }},	/* mask for the horizontal flip */
		{{ 0,0,0,0,0,0,0x00ff,0 }},	/* mask for the order */
		{{ 0,0x0e00,0,0,0,0,0,0 }},	/* mask for the priority */
		{{ 0,0x8000,0,0,0,0,0,0 }}	/* mask for the VRAM target */
	};
	struct atarirle_desc adjusted_modesc = modesc;
	UINT32 temp;
	int i;

	/* blend the playfields and free the temporary one */
	atarigen_blend_gfx(0, 2, 0x0f, 0x30);

	/* initialize the playfield */
	atarigen_playfield_tilemap = tilemap_create(get_playfield_tile_info, atarigt_playfield_scan, TILEMAP_OPAQUE, 8,8, 128,64);
	if (!atarigen_playfield_tilemap)
		return 1;

	/* initialize the motion objects */
	adjusted_modesc.colormask.data[1] &= atarigt_motion_object_mask;
	if (!atarirle_init(0, &adjusted_modesc))
		return 1;

	/* initialize the alphanumerics */
	atarigen_alpha_tilemap = tilemap_create(get_alpha_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,8, 64,32);
	if (!atarigen_alpha_tilemap)
		return 1;
	tilemap_set_transparent_pen(atarigen_alpha_tilemap, 0);
	tilemap_set_palette_offset(atarigen_alpha_tilemap, 0x8000);

	/* allocate temp bitmaps */
	pf_bitmap = auto_bitmap_alloc_depth(Machine->drv->screen_width, Machine->drv->screen_height, 16);
	if (!pf_bitmap)
		return 1;

	/* allocate memory */
	expanded_mram = auto_malloc(sizeof(*expanded_mram) * MRAM_ENTRIES * 3);

	/* map pens 1:1 */
	for (i = 0; i < Machine->drv->total_colors; i++)
		Machine->pens[i] = i;

	/* compute shift values */
	rshift = gshift = bshift = 0;
	for (temp = direct_rgb_components[0]; (temp & 1) == 0; temp >>= 1) rshift++;
	for (temp = direct_rgb_components[1]; (temp & 1) == 0; temp >>= 1) gshift++;
	for (temp = direct_rgb_components[2]; (temp & 1) == 0; temp >>= 1) bshift++;

	/* reset statics */
	playfield_tile_bank = 0;
	playfield_color_bank = 0;
	playfield_xscroll = 0;
	playfield_yscroll = 0;
	tram_checksum = 0;
	memset(atarigt_colorram, 0, 0x80000);
	return 0;
}



/*************************************
 *
 *	Color RAM access
 *
 *************************************/

void atarigt_colorram_w(offs_t address, data16_t data, data16_t mem_mask)
{
	data16_t olddata;

	/* update the raw data */
	address = (address & 0x7ffff) / 2;
	olddata = atarigt_colorram[address];
	COMBINE_DATA(&atarigt_colorram[address]);

	/* update the TRAM checksum */
	if (address >= 0x10000 && address < 0x14000)
		tram_checksum += atarigt_colorram[address] - olddata;

	/* update expanded MRAM */
	else if (address >= 0x20000 && address < 0x28000)
	{
		expanded_mram[0 * MRAM_ENTRIES + (address & 0x7fff)] = (atarigt_colorram[address] >> 8) << rshift;
		expanded_mram[1 * MRAM_ENTRIES + (address & 0x7fff)] = (atarigt_colorram[address] & 0xff) << gshift;
	}
	else if (address >= 0x30000 && address < 0x38000)
		expanded_mram[2 * MRAM_ENTRIES + (address & 0x7fff)] = (atarigt_colorram[address] & 0xff) << bshift;
}


data16_t atarigt_colorram_r(offs_t address)
{
	address &= 0x7ffff;
	return atarigt_colorram[address / 2];
}



/*************************************
 *
 *	Periodic scanline updater
 *
 *************************************/

void atarigt_scanline_update(int scanline)
{
	data32_t *base = &atarigen_alpha32[(scanline / 8) * 32 + 24];
	int i;

	if (scanline == 0) logerror("-------\n");

	/* keep in range */
	if (base >= &atarigen_alpha32[0x400])
		return;

	/* update the playfield scrolls */
	for (i = 0; i < 8; i++)
	{
		data32_t word = *base++;

		if (word & 0x80000000)
		{
			int newscroll = (word >> 21) & 0x3ff;
			int newbank = (word >> 16) & 0x1f;
			if (newscroll != playfield_xscroll)
			{
				force_partial_update(scanline + i - 1);
				tilemap_set_scrollx(atarigen_playfield_tilemap, 0, newscroll);
				playfield_xscroll = newscroll;
			}
			if (newbank != playfield_color_bank)
			{
				force_partial_update(scanline + i - 1);
				tilemap_set_palette_offset(atarigen_playfield_tilemap, (newbank & 0x1f) << 8);
				playfield_color_bank = newbank;
			}
		}

		if (word & 0x00008000)
		{
			int newscroll = ((word >> 6) - (scanline + i)) & 0x1ff;
			int newbank = word & 15;
			if (newscroll != playfield_yscroll)
			{
				force_partial_update(scanline + i - 1);
				tilemap_set_scrolly(atarigen_playfield_tilemap, 0, newscroll);
				playfield_yscroll = newscroll;
			}
			if (newbank != playfield_tile_bank)
			{
				force_partial_update(scanline + i - 1);
				tilemap_mark_all_tiles_dirty(atarigen_playfield_tilemap);
				playfield_tile_bank = newbank;
			}
		}
	}
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

INLINE UINT32 blend_pixels(UINT32 craw, UINT32 traw)
{
	UINT32 rgb1, rgb2;
	int r, g, b;

	/* do the first-level lookups */
	rgb1 = cram[craw];
	rgb2 = tram[traw];

	/* build the final lookups */
	if (rgb1 & 0x8000)
	{
		r = (rgb1 >> 10) & 0x1f;
		g = (rgb1 >> 5) & 0x1f;
		b = rgb1 & 0x1f;
	}
	else if (rgb2 & 0x8000)
	{
		r = (rgb2 >> 5) & 0x3e0;
		g = rgb2 & 0x3e0;
		b = (rgb2 << 5) & 0x3e0;
	}
	else
	{
		r = ((rgb1 >> 10) & 0x1f) | ((rgb2 >> 5) & 0x3e0);
		g = ((rgb1 >> 5) & 0x1f) | (rgb2 & 0x3e0);
		b = (rgb1 & 0x1f) | ((rgb2 << 5) & 0x3e0);
	}

	/* do the final lookups */
	return mram[0 * MRAM_ENTRIES + r] |
	       mram[1 * MRAM_ENTRIES + g] |
	       mram[2 * MRAM_ENTRIES + b];
}


INLINE UINT32 blend_pixels_no_tram(UINT32 craw)
{
	UINT32 rgb1;
	int r, g, b;

	/* do the first-level lookups */
	rgb1 = cram[craw];

	/* build the final lookups */
	r = (rgb1 >> 10) & 0x1f;
	g = (rgb1 >> 5) & 0x1f;
	b = rgb1 & 0x1f;

	/* do the final lookups */
	return mram[0 * MRAM_ENTRIES + r] |
	       mram[1 * MRAM_ENTRIES + g] |
	       mram[2 * MRAM_ENTRIES + b];
}


VIDEO_UPDATE( atarigt )
{
	struct mame_bitmap *mo_bitmap = atarirle_get_vram(0, 0);
	struct mame_bitmap *tm_bitmap = atarirle_get_vram(0, 1);
	int color_latch;
	int x, y;

	/* draw the playfield and alpha layers */
	tilemap_draw(pf_bitmap, cliprect, atarigen_playfield_tilemap, 0, 0);
	tilemap_draw(pf_bitmap, cliprect, atarigen_alpha_tilemap, 0, 0);

	/* cache pointers */
	color_latch = atarigt_colorram[0x30000/2];
	cram = (UINT16 *)&atarigt_colorram[0x00000/2] + 0x2000 * ((color_latch >> 3) & 1);
	tram = (UINT16 *)&atarigt_colorram[0x20000/2] + 0x1000 * ((color_latch >> 4) & 3);
	mram = expanded_mram + 0x2000 * ((color_latch >> 6) & 3);

	/* debugging */
#if DEBUG_ATARIGT
{
	extern int atarirle_hilite_index;

	if (keyboard_pressed(KEYCODE_Q))
	{
		while (keyboard_pressed(KEYCODE_Q)) ;
		dump_video_memory(mo_bitmap, tm_bitmap);
	}

	atarirle_hilite_index = -1;
	if (keyboard_pressed(KEYCODE_N))
	{
		atarirle_hilite_index--;
		while (keyboard_pressed(KEYCODE_N)) ;
	}
	if (keyboard_pressed(KEYCODE_M))
	{
		atarirle_hilite_index++;
		while (keyboard_pressed(KEYCODE_M)) ;
	}
}
#endif

	/* now do the nasty blend */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *pf = (UINT16 *)pf_bitmap->base + y * pf_bitmap->rowpixels;
		UINT16 *mo = (UINT16 *)mo_bitmap->base + y * mo_bitmap->rowpixels;
		UINT32 *dst = (UINT32 *)bitmap->base + y * bitmap->rowpixels;

		/* fast case: no TRAM, no effects */
		if (tram_checksum == 0 && (color_latch & 7) == 0)
		{
			for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			{
				/* start with the playfield/alpha value */
				UINT16 cra = pf[x] & 0xfff;

				/* alpha always gets priority, but MO's override playfield */
				if (!(pf[x] & 0x8000))
					if (mo[x])
						cra = 0x1000 | (mo[x] & ATARIRLE_DATA_MASK);

				/* do the lookups and store the result */
				dst[x] = blend_pixels_no_tram(cra);
			}
		}

		/* slow case: TRAM blending, no effects */
		else if ((color_latch & 7) == 0)
		{
			UINT16 *tm = (UINT16 *)tm_bitmap->base + y * tm_bitmap->rowpixels;
			for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			{
				/* start with the playfield/alpha value */
				UINT16 cra = pf[x] & 0xfff;
				UINT16 tra = cra;

				/* alpha always gets priority, but MO's override playfield */
				if (!(pf[x] & 0x8000))
				{
					if (mo[x])
						cra = 0x1000 | (mo[x] & ATARIRLE_DATA_MASK);
					if (tm[x])
						tra = tm[x] & ATARIRLE_DATA_MASK;
				}

				/* do the lookups and store the result */
				dst[x] = blend_pixels(cra, tra);
			}
		}

		/* slowest case: TRAM blending, with effects */
		else
		{
			UINT16 *tm = (UINT16 *)tm_bitmap->base + y * tm_bitmap->rowpixels;
			for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			{
				/* start with the playfield/alpha value */
				UINT16 cra = pf[x] & 0xfff;
				UINT16 tra = cra;

				/* alpha always gets priority, but MO's override playfield */
				if ((cra & 0x3f) == 0 || !(pf[x] & 0x1000))
					dst[x] = (0xff << rshift) | (0xff << gshift) | (0xff << bshift);
				else
				{
					if (!(pf[x] & 0x8000))
					{
						if (mo[x])
							cra = 0x1000 | (mo[x] & ATARIRLE_DATA_MASK);
						if (tm[x])
							tra = tm[x] & ATARIRLE_DATA_MASK;
					}

					/* do the lookups and store the result */
					dst[x] = blend_pixels(cra, tra);
				}
			}
		}
	}
}



/*************************************
 *
 *	Debugging
 *
 *************************************/

#if DEBUG_ATARIGT
static void dump_video_memory(struct mame_bitmap *mo_bitmap, struct mame_bitmap *tm_bitmap)
{
	int i, x, y;
	FILE *f;

	f = fopen("gt.log", "w");
	fprintf(f, "\n\nCRAM:\n");
	for (i = 0; i < 0x4000 / 2; i += 16)
	{
		fprintf(f, "%04X: %04X %04X %04X %04X %04X %04X %04X %04X - %04X %04X %04X %04X %04X %04X %04X %04X\n", i,
				cram[i+0], cram[i+1], cram[i+2], cram[i+3],
				cram[i+4], cram[i+5], cram[i+6], cram[i+7],
				cram[i+8], cram[i+9], cram[i+10], cram[i+11],
				cram[i+12], cram[i+13], cram[i+14], cram[i+15]);
	}

	fprintf(f, "\n\nTRAM:\n");
	for (i = 0; i < 0x4000 / 2; i += 16)
	{
		fprintf(f, "%04X: %04X %04X %04X %04X %04X %04X %04X %04X - %04X %04X %04X %04X %04X %04X %04X %04X\n", i,
				tram[i+0], tram[i+1], tram[i+2], tram[i+3],
				tram[i+4], tram[i+5], tram[i+6], tram[i+7],
				tram[i+8], tram[i+9], tram[i+10], tram[i+11],
				tram[i+12], tram[i+13], tram[i+14], tram[i+15]);
	}

	fprintf(f, "\n\nRED:\n");
	for (i = 0; i < 0x10000 / 2; i += 32)
	{
		fprintf(f, "%04X: %02X %02X %02X %02X %02X %02X %02X %02X - %02X %02X %02X %02X %02X %02X %02X %02X", i,
				mram[0 * MRAM_ENTRIES + i+0] >> rshift, mram[0 * MRAM_ENTRIES + i+1] >> rshift,
				mram[0 * MRAM_ENTRIES + i+2] >> rshift, mram[0 * MRAM_ENTRIES + i+3] >> rshift,
				mram[0 * MRAM_ENTRIES + i+4] >> rshift, mram[0 * MRAM_ENTRIES + i+5] >> rshift,
				mram[0 * MRAM_ENTRIES + i+6] >> rshift, mram[0 * MRAM_ENTRIES + i+7] >> rshift,
				mram[0 * MRAM_ENTRIES + i+8] >> rshift, mram[0 * MRAM_ENTRIES + i+9] >> rshift,
				mram[0 * MRAM_ENTRIES + i+10] >> rshift, mram[0 * MRAM_ENTRIES + i+11] >> rshift,
				mram[0 * MRAM_ENTRIES + i+12] >> rshift, mram[0 * MRAM_ENTRIES + i+13] >> rshift,
				mram[0 * MRAM_ENTRIES + i+14] >> rshift, mram[0 * MRAM_ENTRIES + i+15] >> rshift);
		fprintf(f, " - %02X %02X %02X %02X %02X %02X %02X %02X - %02X %02X %02X %02X %02X %02X %02X %02X\n",
				mram[0 * MRAM_ENTRIES + i+16] >> rshift, mram[0 * MRAM_ENTRIES + i+17] >> rshift,
				mram[0 * MRAM_ENTRIES + i+18] >> rshift, mram[0 * MRAM_ENTRIES + i+19] >> rshift,
				mram[0 * MRAM_ENTRIES + i+20] >> rshift, mram[0 * MRAM_ENTRIES + i+21] >> rshift,
				mram[0 * MRAM_ENTRIES + i+22] >> rshift, mram[0 * MRAM_ENTRIES + i+23] >> rshift,
				mram[0 * MRAM_ENTRIES + i+24] >> rshift, mram[0 * MRAM_ENTRIES + i+25] >> rshift,
				mram[0 * MRAM_ENTRIES + i+26] >> rshift, mram[0 * MRAM_ENTRIES + i+27] >> rshift,
				mram[0 * MRAM_ENTRIES + i+28] >> rshift, mram[0 * MRAM_ENTRIES + i+29] >> rshift,
				mram[0 * MRAM_ENTRIES + i+30] >> rshift, mram[0 * MRAM_ENTRIES + i+31] >> rshift);
	}

	fprintf(f, "\n\nGREEN:\n");
	for (i = 0; i < 0x10000 / 2; i += 32)
	{
		fprintf(f, "%04X: %02X %02X %02X %02X %02X %02X %02X %02X - %02X %02X %02X %02X %02X %02X %02X %02X", i,
				mram[1 * MRAM_ENTRIES + i+0] >> gshift, mram[1 * MRAM_ENTRIES + i+1] >> gshift,
				mram[1 * MRAM_ENTRIES + i+2] >> gshift, mram[1 * MRAM_ENTRIES + i+3] >> gshift,
				mram[1 * MRAM_ENTRIES + i+4] >> gshift, mram[1 * MRAM_ENTRIES + i+5] >> gshift,
				mram[1 * MRAM_ENTRIES + i+6] >> gshift, mram[1 * MRAM_ENTRIES + i+7] >> gshift,
				mram[1 * MRAM_ENTRIES + i+8] >> gshift, mram[1 * MRAM_ENTRIES + i+9] >> gshift,
				mram[1 * MRAM_ENTRIES + i+10] >> gshift, mram[1 * MRAM_ENTRIES + i+11] >> gshift,
				mram[1 * MRAM_ENTRIES + i+12] >> gshift, mram[1 * MRAM_ENTRIES + i+13] >> gshift,
				mram[1 * MRAM_ENTRIES + i+14] >> gshift, mram[1 * MRAM_ENTRIES + i+15] >> gshift);
		fprintf(f, " - %02X %02X %02X %02X %02X %02X %02X %02X - %02X %02X %02X %02X %02X %02X %02X %02X\n",
				mram[1 * MRAM_ENTRIES + i+16] >> gshift, mram[1 * MRAM_ENTRIES + i+17] >> gshift,
				mram[1 * MRAM_ENTRIES + i+18] >> gshift, mram[1 * MRAM_ENTRIES + i+19] >> gshift,
				mram[1 * MRAM_ENTRIES + i+20] >> gshift, mram[1 * MRAM_ENTRIES + i+21] >> gshift,
				mram[1 * MRAM_ENTRIES + i+22] >> gshift, mram[1 * MRAM_ENTRIES + i+23] >> gshift,
				mram[1 * MRAM_ENTRIES + i+24] >> gshift, mram[1 * MRAM_ENTRIES + i+25] >> gshift,
				mram[1 * MRAM_ENTRIES + i+26] >> gshift, mram[1 * MRAM_ENTRIES + i+27] >> gshift,
				mram[1 * MRAM_ENTRIES + i+28] >> gshift, mram[1 * MRAM_ENTRIES + i+29] >> gshift,
				mram[1 * MRAM_ENTRIES + i+30] >> gshift, mram[1 * MRAM_ENTRIES + i+31] >> gshift);
	}

	fprintf(f, "\n\nBLUE:\n");
	for (i = 0; i < 0x10000 / 2; i += 32)
	{
		fprintf(f, "%04X: %02X %02X %02X %02X %02X %02X %02X %02X - %02X %02X %02X %02X %02X %02X %02X %02X", i,
				mram[2 * MRAM_ENTRIES + i+0] >> bshift, mram[2 * MRAM_ENTRIES + i+1] >> bshift,
				mram[2 * MRAM_ENTRIES + i+2] >> bshift, mram[2 * MRAM_ENTRIES + i+3] >> bshift,
				mram[2 * MRAM_ENTRIES + i+4] >> bshift, mram[2 * MRAM_ENTRIES + i+5] >> bshift,
				mram[2 * MRAM_ENTRIES + i+6] >> bshift, mram[2 * MRAM_ENTRIES + i+7] >> bshift,
				mram[2 * MRAM_ENTRIES + i+8] >> bshift, mram[2 * MRAM_ENTRIES + i+9] >> bshift,
				mram[2 * MRAM_ENTRIES + i+10] >> bshift, mram[2 * MRAM_ENTRIES + i+11] >> bshift,
				mram[2 * MRAM_ENTRIES + i+12] >> bshift, mram[2 * MRAM_ENTRIES + i+13] >> bshift,
				mram[2 * MRAM_ENTRIES + i+14] >> bshift, mram[2 * MRAM_ENTRIES + i+15] >> bshift);
		fprintf(f, " - %02X %02X %02X %02X %02X %02X %02X %02X - %02X %02X %02X %02X %02X %02X %02X %02X\n",
				mram[2 * MRAM_ENTRIES + i+16] >> bshift, mram[2 * MRAM_ENTRIES + i+17] >> bshift,
				mram[2 * MRAM_ENTRIES + i+18] >> bshift, mram[2 * MRAM_ENTRIES + i+19] >> bshift,
				mram[2 * MRAM_ENTRIES + i+20] >> bshift, mram[2 * MRAM_ENTRIES + i+21] >> bshift,
				mram[2 * MRAM_ENTRIES + i+22] >> bshift, mram[2 * MRAM_ENTRIES + i+23] >> bshift,
				mram[2 * MRAM_ENTRIES + i+24] >> bshift, mram[2 * MRAM_ENTRIES + i+25] >> bshift,
				mram[2 * MRAM_ENTRIES + i+26] >> bshift, mram[2 * MRAM_ENTRIES + i+27] >> bshift,
				mram[2 * MRAM_ENTRIES + i+28] >> bshift, mram[2 * MRAM_ENTRIES + i+29] >> bshift,
				mram[2 * MRAM_ENTRIES + i+30] >> bshift, mram[2 * MRAM_ENTRIES + i+31] >> bshift);
	}

	fprintf(f, "\n\nPF:\n");
	for (y = Machine->visible_area.min_y; y <= Machine->visible_area.max_y; y++)
	{
		UINT16 *pf = (UINT16 *)pf_bitmap->base + y * pf_bitmap->rowpixels;
		for (x = Machine->visible_area.min_x; x <= Machine->visible_area.max_x; x++)
			fprintf(f, "%04X ", pf[x]);
		fprintf(f, "\n");
	}

	fprintf(f, "\n\nMO:\n");
	for (y = Machine->visible_area.min_y; y <= Machine->visible_area.max_y; y++)
	{
		UINT16 *mo = (UINT16 *)mo_bitmap->base + y * mo_bitmap->rowpixels;
		for (x = Machine->visible_area.min_x; x <= Machine->visible_area.max_x; x++)
			fprintf(f, "%04X ", mo[x]);
		fprintf(f, "\n");
	}

	fprintf(f, "\n\nTM:\n");
	for (y = Machine->visible_area.min_y; y <= Machine->visible_area.max_y; y++)
	{
		UINT16 *tm = (UINT16 *)tm_bitmap->base + y * tm_bitmap->rowpixels;
		for (x = Machine->visible_area.min_x; x <= Machine->visible_area.max_x; x++)
			fprintf(f, "%04X ", tm[x]);
		fprintf(f, "\n");
	}

	fclose(f);
}

#endif
