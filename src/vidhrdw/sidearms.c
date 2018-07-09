/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

UINT8 *sidearms_bg_scrollx;
UINT8 *sidearms_bg_scrolly;
int sidearms_vidhrdw;

static int bgon, objon, staron, charon;
//static int flipscreen;
static UINT16 hflop_74a_n, hcount_191, vcount_191, latch_374, hadd_283, vadd_283;

static struct tilemap *bg_tilemap, *fg_tilemap;

WRITE_HANDLER( sidearms_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE_HANDLER( sidearms_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE_HANDLER( sidearms_c804_w )
{
	/* bits 0 and 1 are coin counters */

	coin_counter_w(0, data & 0x01);
	coin_counter_w(1, data & 0x02);

	/* bit 2 and 3 lock the coin chutes */

	if (!sidearms_vidhrdw)
	{
		coin_lockout_w(0, !(data & 0x04));
		coin_lockout_w(1, !(data & 0x08));
	}
	else
	{
		coin_lockout_w(0, data & 0x04);
		coin_lockout_w(1, data & 0x08);
	}

	/* bit 4 resets the sound CPU */

	if (data & 0x10)
	{
		cpuint_reset_cpu(1);
	}

	/* bit 5 enables starfield */

	if (staron != (data & 0x20))
	{
		staron = data & 0x20;
		hflop_74a_n = 1;
		hcount_191 = vcount_191 = 0;
	}

	/* bit 6 enables char layer */

	charon = data & 0x40;

	/* bit 7 flips screen */

	if (flip_screen != (data & 0x80))
	{
		flip_screen_set(data & 0x80);
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

WRITE_HANDLER( sidearms_gfxctrl_w )
{
	objon = data & 0x01;
	bgon = data & 0x02;
}

WRITE_HANDLER( sidearms_star_scrollx_w )
{
	UINT16 last_state = hcount_191;

	hcount_191++;
	hcount_191 &= 0x1ff;

	// invert 74LS74A(flipflop) output on 74LS191(hscan counter) carry's rising edge
	if (hcount_191 & ~last_state & 0x100)
		hflop_74a_n ^= 1;
}

WRITE_HANDLER( sidearms_star_scrolly_w )
{
	vcount_191++;
	vcount_191 &= 0xff;
}

static int tilebank_base[] = { 0, 0x100, 0x200, 0x100 };

static void get_bg_tile_info(int tile_index)
{
	UINT8 *tilerom = memory_region(REGION_GFX4);

	int offs = tile_index;
	int attr = tilerom[offs + 1];
	int flags = ((attr & 0x02) ? TILE_FLIPX : 0) | ((attr & 0x04) ? TILE_FLIPY : 0);
	int code, color;

	if (!sidearms_vidhrdw)
	{
		code = tilerom[offs] | ((attr << 8) & 0x100);
		color = (attr >> 3) & 0x1f;
	}
	else
	{
		code = tilerom[offs] | tilebank_base[((attr >> 6) & 0x02) | (attr & 0x01)];
		color = (attr >> 3) & 0x0f;
	}

	SET_TILE_INFO(1, code, color, flags)
}

static void get_fg_tile_info(int tile_index)
{
	int attr = colorram[tile_index];
	int code = videoram[tile_index] + ((attr << 2) & 0x300);
	int color = attr & 0x3f;

	SET_TILE_INFO(0, code, color, 0)
}

static UINT32 sidearms_tilemap_scan( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	int offset = ((row * num_cols) + col) * 2;

	/* swap bits 1-7 and 8-10 of the address to compensate for the funny layout of the ROM data */
	return ((offset & 0xf801) | ((offset & 0x0700) >> 7) | ((offset & 0x00fe) << 3)) & 0x7fff;
}

VIDEO_START( sidearms )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, sidearms_tilemap_scan,
		TILEMAP_TRANSPARENT, 32, 32, 128, 128);

	if ( !bg_tilemap )
		return 1;

	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows,
		TILEMAP_TRANSPARENT, 8, 8, 64, 64);

	if ( !fg_tilemap )
		return 1;

	if (!sidearms_vidhrdw)
		tilemap_set_transparent_pen(bg_tilemap, 15);
	else
		tilemap_set_transparent_pen(bg_tilemap, -1);

	tilemap_set_transparent_pen(fg_tilemap, 3);

	hflop_74a_n = 1;
	hcount_191 = vcount_191 = latch_374 = hadd_283 = vadd_283 = 0;

	return 0;
}

void sidearms_draw_sprites_region( struct mame_bitmap *bitmap, int start_offset, int end_offset )
{
	int offs;

	for (offs = end_offset - 32; offs >= start_offset; offs -= 32)
	{
		int attr = buffered_spriteram[offs + 1];
		int code = buffered_spriteram[offs] + ((attr << 3) & 0x700);
		int x = buffered_spriteram[offs + 3] + ((attr << 4) & 0x100);
		int y = buffered_spriteram[offs + 2];
		int color = attr & 0xf;
		int flipx = 0;
		int flipy = 0;

		if (!y || buffered_spriteram[offs + 5] == 0xc3) continue;

		if (flip_screen)
		{
			x = (62 * 8) - x;
			y = (30 * 8) - y;
			flipx = 1;
			flipy = 1;
		}

		drawgfx(bitmap, Machine->gfx[2],
			code, color,
			flipx, flipy,
			x, y,
			&Machine->visible_area,
			TRANSPARENCY_PEN, 15);
	}
}

static void sidearms_draw_starfield( struct mame_bitmap *bitmap )
{
	UINT8 *sf_rom = memory_region(REGION_USER1);
	int drawstars, bmpitch, x, y;
	UINT16 last_state, _hflop_74a_n, _hcount_191, _vcount_191, _vadd_283;
	UINT16 *lineptr;
	UINT8 *_sf_rom;
	drawstars = (sidearms_vidhrdw == 0) ? 1 : 0;

	if (drawstars)
	{
		lineptr = (data16_t *)bitmap->line[15] + 64;
		bmpitch = bitmap->rowpixels;

		// clear display
		for (x=224; x; x--) memset(lineptr+=bmpitch, 0, 768);

		// draw starfield if enabled
		if (staron)
		{
			lineptr = (data16_t *)bitmap->line[0];

			// cache read-only globals to stack frame
			_hcount_191 = hcount_191;
			_vcount_191 = vcount_191;
			_vadd_283 = vadd_283;
			_hflop_74a_n = hflop_74a_n;
			_sf_rom = sf_rom;

			//.p2align 4(how in GCC???)
			for (y=0; y<256; y++) // 8-bit V-clock input
			{
				for (x=0; x<512; x++) // 9-bit H-clock input
				{
					last_state = hadd_283;
					hadd_283 = (_hcount_191 & 0xff) + (x & 0xff);  // add lower 8 bits and preserve carry

					if (x<64 || x>447 || y<16 || y>239) continue;  // clip rejection

					_vadd_283 = _vcount_191 + y;                   // add lower 8 bits and discard carry(later)

					if (!((_vadd_283 ^ (x>>3)) & 4)) continue;     // logic rejection 1
					if ((_vadd_283 | (hadd_283>>1)) & 2) continue; // logic rejection 2

					// latch data from starfield EPROM on rising edge of 74LS374's clock input
					if ((last_state & 0x1f) == 0x1f)
					{
						_vadd_283 = _vadd_283<<4 & 0xff0;               // to starfield EPROM A04-A11 (8 bits)
						_vadd_283 |= (_hflop_74a_n^(hadd_283>>8)) << 3; // to starfield EPROM A03     (1 bit)
						_vadd_283 |= hadd_283>>5 & 7;                   // to starfield EPROM A00-A02 (3 bits)
						latch_374 = _sf_rom[_vadd_283 + 0x3000];        // lines A12-A13 are always high
					}

					if ((((latch_374 ^ hadd_283) ^ 1) & 0x1f) != 0x1f) continue; // logic rejection 3

					lineptr[x] = (data16_t)(latch_374>>5 | 0x378); // to color mixer
				}
				lineptr += bmpitch;
			}
		}
	}
}

static void sidearms_draw_sprites( struct mame_bitmap *bitmap )
{
	if (!objon) return;

	if (sidearms_vidhrdw == 2) // Dyger has simple front-to-back sprite priority
		sidearms_draw_sprites_region(bitmap, 0x0000, 0x1000);
	else
	{
		sidearms_draw_sprites_region(bitmap, 0x0700, 0x0800);
		sidearms_draw_sprites_region(bitmap, 0x0e00, 0x1000);
		sidearms_draw_sprites_region(bitmap, 0x0800, 0x0f00);
		sidearms_draw_sprites_region(bitmap, 0x0000, 0x0700);
	}
}

VIDEO_UPDATE( sidearms )
{
	sidearms_draw_starfield(bitmap);

	tilemap_set_scrollx(bg_tilemap, 0, sidearms_bg_scrollx[0] + (sidearms_bg_scrollx[1] << 8 & 0xf00));
	tilemap_set_scrolly(bg_tilemap, 0, sidearms_bg_scrolly[0] + (sidearms_bg_scrolly[1] << 8 & 0xf00));

	if (bgon)
		tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);

	sidearms_draw_sprites(bitmap);

	if (charon)
		tilemap_draw(bitmap, &Machine->visible_area, fg_tilemap, 0, 0);
}

VIDEO_EOF( sidearms )
{
	buffer_spriteram_w(0, 0);
}
