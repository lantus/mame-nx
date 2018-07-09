/*
  Sega system24 hardware

System 24      68000x2  315-5292   315-5293  315-5294  315-5242        ym2151 dac           315-5195(x3) 315-5296(IO)

  System24:
    The odd one out.  Medium resolution. Entirely ram-based, no
    graphics roms.  4-layer tilemap hardware in two pairs, selection
    on a 8-pixels basis.  Tile-based sprites(!) organised as a linked
    list.  The tilemap chip has been reused for model1 and model2,
    probably because they had it handy and it handles medium res.

*/

/*
 * System 16 color palette formats
 */


/* system24temp_ functions / variables are from shared rewrite files,
   once the rest of the rewrite is complete they can be removed, I
   just made a copy & renamed them for now to avoid any conflicts
*/

#include "driver.h"
#include "state.h"
#include "generic.h"
#include "drawgfx.h"
#include "osdepend.h"
#include "segaic24.h"

#include <math.h>

static int kc = -1;
static int kk = 0;
//static int kz = 0;

static void set_color(int color, unsigned char r, unsigned char g, unsigned char b, int highlight)
{
	palette_set_color (color, r, g, b);

	if(highlight) {
		r = 255-0.6*(255-r);
		g = 255-0.6*(255-g);
		b = 255-0.6*(255-b);
	} else {
		r = 0.6*r;
		g = 0.6*g;
		b = 0.6*b;
	}
	palette_set_color (color+Machine->drv->total_colors/2, r, g, b);
}

// 315-5242
WRITE16_HANDLER (system24temp_sys16_paletteram1_w)
{
	int r, g, b;
	COMBINE_DATA (paletteram16 + offset);
	data = paletteram16[offset];

	r = (data & 0x00f) << 4;
	if(data & 0x1000)
		r |= 8;

	g = data & 0x0f0;
	if(data & 0x2000)
		g |= 8;

	b = (data & 0xf00) >> 4;
	if(data & 0x4000)
		b |= 8;

	r |= r >> 5;
	g |= g >> 5;
	b |= b >> 5;
	set_color (offset, r, g, b, data & 0x8000);
}

// - System 24

enum {SYS24_TILES = 0x4000};

static UINT16 *sys24_char_ram, *sys24_tile_ram;
static UINT16 sys24_tile_mask;
static unsigned char *sys24_char_dirtymap;
static int sys24_char_dirty, sys24_char_gfx_index;
static struct tilemap *sys24_tile_layer[4];

#ifdef LSB_FIRST
static struct GfxLayout sys24_char_layout = {
	8, 8,
	SYS24_TILES,
	4,
	{ 0, 1, 2, 3 },
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};
#else
static struct GfxLayout sys24_char_layout = {
	8, 8,
	SYS24_TILES,
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};
#endif

static void sys24_tile_info_0s(int tile_index)
{
	UINT16 val = sys24_tile_ram[tile_index];
	SET_TILE_INFO(sys24_char_gfx_index, val & sys24_tile_mask, (val >> 7) & 0xff, 0);
}

static void sys24_tile_info_0w(int tile_index)
{
	UINT16 val = sys24_tile_ram[tile_index|0x1000];
	SET_TILE_INFO(sys24_char_gfx_index, val & sys24_tile_mask, (val >> 7) & 0xff, 0);
}

static void sys24_tile_info_1s(int tile_index)
{
	UINT16 val = sys24_tile_ram[tile_index|0x2000];
	SET_TILE_INFO(sys24_char_gfx_index, val & sys24_tile_mask, (val >> 7) & 0xff, 0);
}

static void sys24_tile_info_1w(int tile_index)
{
	UINT16 val = sys24_tile_ram[tile_index|0x3000];
	SET_TILE_INFO(sys24_char_gfx_index, val & sys24_tile_mask, (val >> 7) & 0xff, 0);
}

static void sys24_tile_dirtyall(void)
{
	memset(sys24_char_dirtymap, 1, SYS24_TILES);
	sys24_char_dirty = 1;
}

int sys24_tile_vh_start(UINT16 tile_mask)
{
	sys24_tile_mask = tile_mask;

	for(sys24_char_gfx_index = 0; sys24_char_gfx_index < MAX_GFX_ELEMENTS; sys24_char_gfx_index++)
		if (Machine->gfx[sys24_char_gfx_index] == 0)
			break;
	if(sys24_char_gfx_index == MAX_GFX_ELEMENTS)
		return 1;

	sys24_char_ram = auto_malloc(0x80000);
	if(!sys24_char_ram)
		return 1;

	sys24_tile_ram = auto_malloc(0x10000);
	if(!sys24_tile_ram) {
		free(sys24_char_ram);
		return 1;
	}

	sys24_char_dirtymap = auto_malloc(SYS24_TILES);
	if(!sys24_char_dirtymap) {
		free(sys24_tile_ram);
		free(sys24_char_ram);
		return 1;
	}

	sys24_tile_layer[0] = tilemap_create(sys24_tile_info_0s, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 64, 64);
	sys24_tile_layer[1] = tilemap_create(sys24_tile_info_0w, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 64, 64);
	sys24_tile_layer[2] = tilemap_create(sys24_tile_info_1s, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 64, 64);
	sys24_tile_layer[3] = tilemap_create(sys24_tile_info_1w, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, 64, 64);

	if(!sys24_tile_layer[0] || !sys24_tile_layer[1] || !sys24_tile_layer[2] || !sys24_tile_layer[3]) {
		free(sys24_char_dirtymap);
		free(sys24_tile_ram);
		free(sys24_char_ram);
		return 1;
	}

	tilemap_set_transparent_pen(sys24_tile_layer[0], 0);
	tilemap_set_transparent_pen(sys24_tile_layer[1], 0);

	memset(sys24_char_ram, 0, 0x80000);
	memset(sys24_tile_ram, 0, 0x10000);
	memset(sys24_char_dirtymap, 0, SYS24_TILES);

	Machine->gfx[sys24_char_gfx_index] = decodegfx((unsigned char *)sys24_char_ram, &sys24_char_layout);

	if(!Machine->gfx[sys24_char_gfx_index]) {
		free(sys24_char_dirtymap);
		free(sys24_tile_ram);
		free(sys24_char_ram);
		return 1;
	}

	if (Machine->drv->color_table_len)
	{
		Machine->gfx[sys24_char_gfx_index]->colortable = Machine->remapped_colortable;
		Machine->gfx[sys24_char_gfx_index]->total_colors = Machine->drv->color_table_len / 16;
	}
	else
	{
		Machine->gfx[sys24_char_gfx_index]->colortable = Machine->pens;
		Machine->gfx[sys24_char_gfx_index]->total_colors = Machine->drv->total_colors / 16;
	}

	state_save_register_UINT16("system24 tile", 0, "tile ram", sys24_tile_ram, 0x8000);
	state_save_register_UINT16("system24 tile", 0, "char ram", sys24_char_ram, 0x40000);
	state_save_register_func_postload(sys24_tile_dirtyall);

	return 0;
}

void sys24_tile_update(void)
{
	int i;
/* pretty much trusted now
	if(keyboard_pressed(KEYCODE_L)) {
		memset(sys24_char_dirtymap, 1, SYS24_TILES);
		sys24_char_dirty = 1;
	}
*/
	if(sys24_char_dirty) {
		for(i=0; i<SYS24_TILES; i++) {
			if(sys24_char_dirtymap[i]) {
				sys24_char_dirtymap[i] = 0;
				decodechar(Machine->gfx[sys24_char_gfx_index], i, (unsigned char *)sys24_char_ram, &sys24_char_layout);
			}
		}
		tilemap_mark_all_tiles_dirty(sys24_tile_layer[0]);
		tilemap_mark_all_tiles_dirty(sys24_tile_layer[1]);
		tilemap_mark_all_tiles_dirty(sys24_tile_layer[2]);
		tilemap_mark_all_tiles_dirty(sys24_tile_layer[3]);
		sys24_char_dirty = 0;
	}
/* pretty much trusted now
	if(keyboard_pressed(KEYCODE_K)) {
		tilemap_mark_all_tiles_dirty(sys24_tile_layer[0]);
		tilemap_mark_all_tiles_dirty(sys24_tile_layer[1]);
		tilemap_mark_all_tiles_dirty(sys24_tile_layer[2]);
		tilemap_mark_all_tiles_dirty(sys24_tile_layer[3]);
	}
*/
	for(i=0; i<4; i++) {
		UINT16 hscr = sys24_tile_ram[0x5000+i];
		UINT16 vscr = sys24_tile_ram[0x5004+i];
		if(vscr & 0x8000)
			logerror("Layer %d disabled\n", i);
		if(hscr & 0x8000)
			logerror("Layer %d uses linescroll\n", i);
	}
}

static void sys24_tile_draw_rect(struct mame_bitmap *bm, struct mame_bitmap *tm, struct mame_bitmap *dm, const UINT16 *mask,
								 int win, int sx, int sy, int xx1, int yy1, int xx2, int yy2)
{
	int y;
	const UINT16 *source = ((UINT16 *)bm->base) + sx + sy*bm->rowpixels;
	const UINT8  *trans  = ((UINT8 *) tm->base) + sx + sy*tm->rowpixels;
	UINT16       *dest   = dm->base;

	dest += yy1*dm->rowpixels + xx1;
	mask += yy1*4;
	yy2 -= yy1;

	while(xx1 >= 128) {
		xx1 -= 128;
		xx2 -= 128;
		mask++;
	}

	for(y=0; y<yy2; y++) {
		const UINT16 *src = source + bm->rowpixels*y;
		const UINT8  *srct = trans + tm->rowpixels*y;
		UINT16 *dst = dest + dm->rowpixels*y;
		const UINT16 *mask1 = mask+4*y;
		int llx = xx2;
		int cur_x = xx1;

		while(llx > 0) {
			UINT16 m = *mask1++;

			if(win)
				m = ~m;

			if(!cur_x && llx>=128) {
				// Fast paths for the 128-pixels without side clipping case

				if(!m) {
					// 1- 128 pixels from this layer
					int x;
					for(x=0; x<128; x++) {
						if(*srct++)
							*dst = *src;
						src++;
						dst++;
					}

				} else if(m == 0xffff) {
					// 2- 128 pixels from the other layer
					src += 128;
					dst += 128;
					srct += 128;

				} else {
					// 3- 128 pixels from both layers
					int x;
					for(x=0; x<128; x+=8) {
						if(!(m & 0x8000)) {
							int xx;
							for(xx=0; xx<8; xx++)
								if(srct[xx])
									dst[xx] = src[xx];
						}
						dst += 8;
						src += 8;
						srct += 8;
						m <<= 1;
					}
				}
			} else {
				// Clipped path
				int llx1 = llx >= 128 ? 128 : llx;

				if(!m) {
					// 1- 128 pixels from this layer
					int x;
					for(x = cur_x; x<llx1; x++) {
						if(*srct++)
							*dst = *src;
						src++;
						dst++;
					}

				} else if(m == 0xffff) {
					// 2- 128 pixels from the other layer
					src += 128 - cur_x;
					dst += 128 - cur_x;
					srct += 128 - cur_x;

				} else {
					// 3- 128 pixels from both layers
					int x;
					for(x=cur_x; x<llx1; x++) {

						if(*srct++ && !(m & (0x8000 >> (x >> 3))))
							*dst = *src;

						dst ++;
						src ++;
					}
				}
			}
			llx -= 128;
			cur_x = 0;
		}
	}
}

static void sys24_tile_draw_rect_rgb(struct mame_bitmap *bm, struct mame_bitmap *tm, struct mame_bitmap *dm, const UINT16 *mask,
									 int win, int sx, int sy, int xx1, int yy1, int xx2, int yy2)
{
	int y;
	const UINT16 *source = ((UINT16 *)bm->base) + sx + sy*bm->rowpixels;
	const UINT8  *trans  = ((UINT8 *) tm->base) + sx + sy*tm->rowpixels;
	UINT16       *dest   = dm->base;
	pen_t        *pens   = Machine->pens;

	dest += yy1*dm->rowpixels + xx1;
	mask += yy1*4;
	yy2 -= yy1;

	while(xx1 >= 128) {
		xx1 -= 128;
		xx2 -= 128;
		mask++;
	}

	for(y=0; y<yy2; y++) {
		const UINT16 *src = source + bm->rowpixels*y;
		const UINT8  *srct = trans + tm->rowpixels*y;
		UINT16 *dst = dest + dm->rowpixels*y;
		const UINT16 *mask1 = mask+4*y;
		int llx = xx2;
		int cur_x = xx1;

		while(llx > 0) {
			UINT16 m = *mask1++;

			if(win)
				m = ~m;

			if(!cur_x && llx>=128) {
				// Fast paths for the 128-pixels without side clipping case

				if(!m) {
					// 1- 128 pixels from this layer
					int x;
					for(x=0; x<128; x++) {
						if(*srct++)
							*dst = pens[*src];
						src++;
						dst++;
					}

				} else if(m == 0xffff) {
					// 2- 128 pixels from the other layer
					src += 128;
					dst += 128;
					srct += 128;

				} else {
					// 3- 128 pixels from both layers
					int x;
					for(x=0; x<128; x+=8) {
						if(!(m & 0x8000)) {
							int xx;
							for(xx=0; xx<8; xx++)
								if(srct[xx])
									dst[xx] = pens[src[xx]];
						}
						dst += 8;
						src += 8;
						srct += 8;
						m <<= 1;
					}
				}
			} else {
				// Clipped path
				int llx1 = llx >= 128 ? 128 : llx;

				if(!m) {
					// 1- 128 pixels from this layer
					int x;
					for(x = cur_x; x<llx1; x++) {
						if(*srct++)
							*dst = pens[*src];
						src++;
						dst++;
					}

				} else if(m == 0xffff) {
					// 2- 128 pixels from the other layer
					src += 128 - cur_x;
					dst += 128 - cur_x;
					srct += 128 - cur_x;

				} else {
					// 3- 128 pixels from both layers
					int x;
					for(x=cur_x; x<llx1; x++) {

						if(*srct++ && !(m & (0x8000 >> (x >> 3))))
							*dst = pens[*src];

						dst ++;
						src ++;
					}
				}
			}
			llx -= 128;
			cur_x = 0;
		}
	}
}

void sys24_tile_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int layer, int flags)
{
	UINT16 hscr = sys24_tile_ram[0x5000+layer];
	UINT16 vscr = sys24_tile_ram[0x5004+layer];
	UINT16 ctrl = sys24_tile_ram[0x5004+(layer & 2)];
	UINT16 *mask = sys24_tile_ram + (layer & 2 ? 0x6800 : 0x6000);

#ifdef MAME_DEBUG
	if(0 && !layer)
		usrintf_showmessage("%04x %04x %04x %04x",
							sys24_tile_ram[0x5004+0],
							sys24_tile_ram[0x5004+1],
							sys24_tile_ram[0x5004+2],
							sys24_tile_ram[0x5004+3]);

#endif

	// Layer disable
	if(vscr & 0x8000)
		return;

	if(ctrl & 0x6000) {
		// Special window/scroll modes
		if(layer & 1)
			return;

		tilemap_set_scrolly(sys24_tile_layer[layer],   0, +(vscr & 0x1ff));
		tilemap_set_scrolly(sys24_tile_layer[layer|1], 0, +(vscr & 0x1ff));

		if(hscr & 0x8000) {
#ifdef MAME_DEBUG
			usrintf_showmessage("Linescroll with special mode %04x", ctrl);
			//			return;
#endif
		} else {
			tilemap_set_scrollx(sys24_tile_layer[layer],   0, -(hscr & 0x1ff));
			tilemap_set_scrollx(sys24_tile_layer[layer|1], 0, -(hscr & 0x1ff));
		}

		switch((ctrl & 0x6000) >> 13) {
		case 1: {
			struct rectangle c1 = *cliprect;
			struct rectangle c2 = *cliprect;
			UINT16 v;
			v = (-vscr) & 0x1ff;
			if(c1.max_y >= v)
				c1.max_y = v-1;
			if(c2.min_y < v)
				c2.min_y = v;
			if(!((-vscr) & 0x200))
				layer ^= 1;

			tilemap_draw(bitmap, &c1, sys24_tile_layer[layer],   0, 0);
			tilemap_draw(bitmap, &c2, sys24_tile_layer[layer^1], 0, 0);
			break;
		}
		case 2: {
			struct rectangle c1 = *cliprect;
			struct rectangle c2 = *cliprect;
			UINT16 h;
			h = (+hscr) & 0x1ff;
			if(c1.max_x >= h)
				c1.max_x = h-1;
			if(c2.min_x < h)
				c2.min_x = h;
			if(!((+hscr) & 0x200))
				layer ^= 1;

			tilemap_draw(bitmap, &c1, sys24_tile_layer[layer],   0, 0);
			tilemap_draw(bitmap, &c2, sys24_tile_layer[layer^1], 0, 0);
			break;
		}
		case 3:
#ifdef MAME_DEBUG
			usrintf_showmessage("Mode 3, please scream");
#endif
			break;
		};

	} else {
		struct mame_bitmap *bm, *tm;
		void (*draw)(struct mame_bitmap *, struct mame_bitmap *, struct mame_bitmap *, const UINT16 *, int, int, int, int, int, int, int);

		if(Machine->drv->video_attributes & VIDEO_RGB_DIRECT)
			draw = sys24_tile_draw_rect_rgb;
		else
			draw = sys24_tile_draw_rect;

		bm = tilemap_get_pixmap(sys24_tile_layer[layer]);
		tm = tilemap_get_transparency_bitmap(sys24_tile_layer[layer]);

		if(hscr & 0x8000) {
			int y;
			UINT16 *hscrtb = sys24_tile_ram + 0x4000 + 0x200*layer;
			vscr &= 0x1ff;

			for(y=0; y<384; y++) {
				// Whether it's tilemap-relative or screen-relative is unknown
				hscr = (-hscrtb[vscr]) & 0x1ff;
				if(hscr + 496 <= 512) {
					// Horizontal split unnecessary
					draw(bm, tm, bitmap, mask, layer & 1, hscr, vscr,        0,        y,      496,      y+1);
				} else {
					// Horizontal split necessary
					draw(bm, tm, bitmap, mask, layer & 1, hscr, vscr,        0,        y, 512-hscr,      y+1);
					draw(bm, tm, bitmap, mask, layer & 1,    0, vscr, 512-hscr,        y,      496,      y+1);
				}
				vscr = (vscr + 1) & 0x1ff;
			}
		} else {
			hscr = (-hscr) & 0x1ff;
			vscr = (+vscr) & 0x1ff;

			if(hscr + 496 <= 512) {
				// Horizontal split unnecessary
				if(vscr + 384 <= 512) {
					// Vertical split unnecessary
					draw(bm, tm, bitmap, mask, layer & 1, hscr, vscr,        0,        0,      496,      384);
				} else {
					// Vertical split necessary
					draw(bm, tm, bitmap, mask, layer & 1, hscr, vscr,        0,        0,      496, 512-vscr);
					draw(bm, tm, bitmap, mask, layer & 1, hscr,    0,        0, 512-vscr,      496,      384);

				}
			} else {
				// Horizontal split necessary
				if(vscr + 384 <= 512) {
					// Vertical split unnecessary
					draw(bm, tm, bitmap, mask, layer & 1, hscr, vscr,        0,        0, 512-hscr,      384);
					draw(bm, tm, bitmap, mask, layer & 1,    0, vscr, 512-hscr,        0,      496,      384);
				} else {
					// Vertical split necessary
					draw(bm, tm, bitmap, mask, layer & 1, hscr, vscr,        0,        0, 512-hscr, 512-vscr);
					draw(bm, tm, bitmap, mask, layer & 1,    0, vscr, 512-hscr,        0,      496, 512-vscr);
					draw(bm, tm, bitmap, mask, layer & 1, hscr,    0,        0, 512-vscr, 512-hscr,      384);
					draw(bm, tm, bitmap, mask, layer & 1,    0,    0, 512-hscr, 512-vscr,      496,      384);
				}
			}
		}
	}
}

READ16_HANDLER(sys24_tile_r)
{
	return sys24_tile_ram[offset];
}

READ16_HANDLER(sys24_char_r)
{
	return sys24_char_ram[offset];
}

WRITE16_HANDLER(sys24_tile_w)
{
	UINT16 old = sys24_tile_ram[offset];
	COMBINE_DATA(sys24_tile_ram + offset);
	if(offset < 0x4000 && old != sys24_tile_ram[offset])
		tilemap_mark_tile_dirty(sys24_tile_layer[offset >> 12], offset & 0xfff);
}

WRITE16_HANDLER(sys24_char_w)
{
	UINT16 old = sys24_char_ram[offset];
	COMBINE_DATA(sys24_char_ram + offset);
	if(old != sys24_char_ram[offset]) {
		sys24_char_dirtymap[offset / 16] = 1;
		sys24_char_dirty = 1;
	}
}

// - System 24

static UINT16 *sys24_sprite_ram;

int sys24_sprite_vh_start(void)
{
	sys24_sprite_ram = auto_malloc(0x40000);
	if(!sys24_sprite_ram)
		return 1;

	state_save_register_UINT16("system24 sprite", 0, "ram", sys24_sprite_ram, 0x20000);
	kc = 0;
	return 0;
}

/* System24 sprites
      Normal sprite:
    0   00Znnnnn    nnnnnnnn    zoom mode (1 = separate x and y), next sprite
    1   xxxxxxxx    yyyyyyyy    zoom x, zoom y (zoom y is used for both when mode = 0)
	2   --TTTTTT    TTTTTTTT    sprite number
    3   -CCCCCCC    CCCCCCCC    indirect palette base
    4   FSSSYYYY    YYYYYYYY    flipy, y size, top
    5   FSSSXXXX    XXXXXXXX    flipx, x size, left

      Clip?
    0   01-nnnnn    nnnnnnnn    next sprite
    1   ????????    ????????

      Skipped entry
    0   10-nnnnn    nnnnnnnn    next sprite

      End of sprite list
    0   11------    --------
*/

#define writepix								\
	{											\
		c = colors[c];							\
		if(c) {									\
			if(c==1)							\
				*dst = (*dst) | 0x2000;			\
			else								\
				*dst = c;						\
		}										\
		dst++;									\
	}

static void sys24_sprite_draw8_g(UINT16 *src, UINT16 *dst, int pitch, UINT16 *colors)
{
	UINT16 y, c, val;
	for(y=0; y<8; y++) {
		val = *src++;
		c =  val >> 12;
		writepix;
		c = (val >>  8) & 0xf;
		writepix;
		c = (val >>  4) & 0xf;
		writepix;
		c =  val        & 0xf;
		writepix;

		val = *src++;
		c =  val >> 12;
		writepix;
		c = (val >>  8) & 0xf;
		writepix;
		c = (val >>  4) & 0xf;
		writepix;
		c =  val        & 0xf;
		writepix;
		dst += pitch;
	}
}

static void sys24_sprite_draw8(UINT16 *src, UINT16 *dst, int pitch, UINT16 *colors)
{
	sys24_sprite_draw8_g(src, dst, pitch-8, colors);
}

static void sys24_sprite_draw8_y(UINT16 *src, UINT16 *dst, int pitch, UINT16 *colors)
{
	sys24_sprite_draw8_g(src, dst+7*pitch, -pitch-8, colors);
}

static void sys24_sprite_draw8_gx(UINT16 *src, UINT16 *dst, int pitch, UINT16 *colors)
{
	UINT16 y, c, val;

	for(y=0; y<8; y++) {
		val = src[1];
		c =  val        & 0xf;
		writepix;
		c = (val >>  4) & 0xf;
		writepix;
		c = (val >>  8) & 0xf;
		writepix;
		c =  val >> 12;
		writepix;

		val = src[0];
		c =  val        & 0xf;
		writepix;
		c = (val >>  4) & 0xf;
		writepix;
		c = (val >>  8) & 0xf;
		writepix;
		c =  val >> 12;
		writepix;

		src += 2;
		dst += pitch;
	}
}

static void sys24_sprite_draw8_x(UINT16 *src, UINT16 *dst, int pitch, UINT16 *colors)
{
	sys24_sprite_draw8_gx(src, dst, pitch-8, colors);
}

static void sys24_sprite_draw8_xy(UINT16 *src, UINT16 *dst, int pitch, UINT16 *colors)
{
	sys24_sprite_draw8_gx(src, dst+7*pitch, -pitch-8, colors);
}

#undef writepix

void sys24_sprite_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	UINT16 curspr = 0;
	int countspr = 0;
	int pitch = ((UINT16 *)bitmap->line[1]) - ((UINT16 *)bitmap->line[0]);
	void (*sdraw)(UINT16 *src, UINT16 *dst, int p, UINT16 *colors);
	if(++kk == 5) {
		int kx;
		kx = 0;
		kk = 0;
#ifdef MAME_DEBUG
		if(keyboard_pressed(KEYCODE_Y)) {
			kx = 1;
			kc--;
			if(kc<0)
				kc = 100;
		}
		if(keyboard_pressed(KEYCODE_U)) {
			kx = 1;
			kc++;
			if(kc==101)
				kc = 0;
		}
		if(kx)
			usrintf_showmessage("Sprite %x", kc);
#endif
	}

	for(;;) {
		UINT16 *source, *pix;
		UINT16 type, cbase;
		int x, y, sx, sy;
		int px, py, xx, yy;
		UINT16 colors[16];
		int flipx, flipy;
		int zoomx, zoomy;

		cbase = 0x1000;

		source = sys24_sprite_ram + (curspr << 3);

		if(curspr == 0 && source[0] == 0)
			break;

		curspr = source[0];
		type = curspr & 0xc000;
		curspr &= 0x1fff;

		if(type == 0xc000)
			break;

		if(type == 0x8000)
			continue;

		if(type == 0x4000) {
			// Clip
			continue;
		}


		if(source[0] & 0x2000)
			zoomx = zoomy = source[1] & 0xff;
		else {
			zoomx = source[1] >> 8;
			zoomy = source[1] & 0xff;
		}
		zoomx++;
		zoomy++;

		x = source[5] & 0xfff;
		flipx = source[5] & 0x8000;
		if(x & 0x800)
			x -= 0x1000;
		sx = 1 << ((source[5] & 0x7000) >> 12);

		x -= 8;


		y = source[4] & 0xfff;
		if(y & 0x800)
			y -= 0x1000;
		flipy = source[4] & 0x8000;
		sy = 1 << ((source[4] & 0x7000) >> 12);

		cbase = 0x1000;
		pix = sys24_sprite_ram + (source[3] & 0x7fff)* 0x8;
		for(px=0; px<8; px++) {
			int c;
			c = pix[px] >> 8;
			if(c>1)
				c |= cbase;
			colors[px*2]   = c;

			c = pix[px] & 0xff;
			if(c>1)
				c |= cbase;
			colors[px*2+1] = c;
		}

		pix = sys24_sprite_ram + (source[2] & 0x3fff) * 0x10;


		if(flipx)
			if(flipy)
				sdraw = sys24_sprite_draw8_xy;
			else
				sdraw = sys24_sprite_draw8_x;
		else
			if(flipy)
				sdraw = sys24_sprite_draw8_y;
			else
				sdraw = sys24_sprite_draw8;

		for(py=0; py<sy; py++) {
			yy = y + 8*(flipy ? sy-py-1 : py);
			if(yy >= 48*8)
				break;
			if(yy>-8)
				for(px=0; px<sx; px++) {
					xx = x + px*8;
					if(xx >= 62*8)
						break;
					if(xx > -8) {
						//						UINT16 *src = pix + 0x10*px;
						sdraw(pix + 0x10*(flipx ? sx-px-1 : px), ((UINT16 *)bitmap->line[yy]) + xx, pitch, colors);
					}
				}
			pix += sx * 0x10;
		}


		countspr++;
		if(!curspr || countspr >= 0x2000)
			break;
	}
}


WRITE16_HANDLER(sys24_sprite_w)
{
	COMBINE_DATA(sys24_sprite_ram + offset);
}

READ16_HANDLER(sys24_sprite_r)
{
	return sys24_sprite_ram[offset];
}

// Programmable mixers
//   System 24

static UINT16 sys24_mixer_reg[0x10];

int sys24_mixer_vh_start(void)
{
	memset(sys24_mixer_reg, 0, sizeof(sys24_mixer_reg));
	return 0;
}

WRITE16_HANDLER (sys24_mixer_w)
{
	UINT16 old = sys24_mixer_reg[offset];
	COMBINE_DATA(sys24_mixer_reg + offset);
	data = sys24_mixer_reg[offset];
	if(old != data) {
		int i;
		char msg[5*16+1];
		char *p = msg;
		for(i=0; i<16; i++) {
			//			p += sprintf(p, " %04x", sys24_mixer_reg[i]);
			p += sprintf(p, " %d", sys24_mixer_reg[i] & 7);
		}
		logerror("S24Mix:%s\n", msg);
	}
}

READ16_HANDLER (sys24_mixer_r)
{
	return sys24_mixer_reg[offset];
}


