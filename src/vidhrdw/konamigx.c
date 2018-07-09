#define VERBOSE 0

/*
 * vidhrdw/konamigx.c - Konami GX video hardware (here there be dragons)
 *
 */

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "machine/konamigx.h"

/*
> the color DAC has a 8 bits r/g/b and a 8bits brightness
> but there is 1 ram per color channel for the palette and no ram for brightness
> I guess you have a global brightness control in the '338 ?
> hmmm, to add insult to injury, the 5x5 has inputs for 4 tilemaps and 3 sprite-ish thingies
> but only 2 of the spriteish thingies are actually connected
> one is the output
> it outputs a spriteish thingy, probably so that you can cascade them
> specifically, you have 4 10bpp tilemaps inputs
> 2 8bpp color, 8bpp priority, 2 mix, 2 bri, 2 shadow inputs
         and one 17bpp, 8bpp pri, 2 mix and 2 bri that goes to the rom board
> one of the two pure sprite ones is grounded
> and you have a 17bpp, 8 pri, 1 dot on, 2 mix, 2 bri, 2 shadow output
> I guess the dot on tells the 338 when to insert its own background color
> the 338 gets in 13 of the color bits, and all the meta
> I guess in case of blending the 5x5 outputs more than one color per pixel

> pasc4 = 56540
> the structure seems to be psac2-vram-h/c/rom-psac4 in that order
> the psac 2 computing the coordinates
> ok, the psac2 output 2 11 bits coordinates
> bits 4-11 of each plus a double buffering bit lookup in 3 8bits rams to the get 24 bits data
> the top 8 bits are weird, the bottom 16 are directly an address, once you shift in the 8 remaining bits
> i.e., it's 16x16 tiles
> in fact the top 8 are xored with the bottom 8 bits, i.e. they fuzz the coordinates
> oh no, they don't fuzz
> they're flipx/y

*/

static int layer_colorbase[4];
static int gx_tilebanks[8], gx_oldbanks[8];
static int gx_invertlayersBC;
static int gx_tilemode;


static void (*game_tile_callback)(int, int *, int *);

static void konamigx_type2_tile_callback(int layer, int *code, int *color)
{
	int d = *code;

	*code = (gx_tilebanks[(d & 0xe000)>>13]<<13) + (d & 0x1fff);
	K055555GX_decode_vmixcolor(layer, color);
}

static void konamigx_alpha_tile_callback(int layer, int *code, int *color)
{
	int mixcode;
	int d = *code;

	mixcode = K055555GX_decode_vmixcolor(layer, color);

	if (mixcode < 0)
		*code = (gx_tilebanks[(d & 0xe000)>>13]<<13) + (d & 0x1fff);
	else
	{
		/* save mixcode and mark tile alpha (unimplemented) */
		*code = 0;

		#if VERBOSE
			usrintf_showmessage("skipped alpha tile(layer=%d mix=%d)", layer, mixcode);
		#endif
	}
}

/*
> bits 8-13 are the low priority bits
> i.e. pri 0-5
> pri 6-7 can be either 1, bits 14,15 or bits 16,17
> contro.bit 2 being 0 forces the 1
> when control.bit 2 is 1, control.bit 3 selects between the two
> 0 selects 16,17
> that gives you the entire 8 bits of the sprite priority
> ok, lemme see if I've got this.  bit2 = 0 means the top bits are 11, bit2=1 means the top bits are bits 14/15 (of the whatever word?) else
+16+17?
> bit3=1 for the second

 *   6  | ---------xxxxxxx | "color", but depends on external connections


> there are 8 color lines entering the 5x5
> that means the palette is 4 bits, not 5 as you currently have
> the bits 4-9 are the low priority bits
> bits 10/11 or 12/13 are the two high priority bits, depending on the control word
> and bits 14/15 are the shadow bits
> mix0/1 and brit0/1 come from elsewhere
> they come from the '673 all right, but not from word 6
> and in fact the top address bits are highly suspect
> only 18 of the address bits go to the roms
> the next 2 go to cai0/1 and the next 4 to bk0-3
> (the '246 indexes the roms, the '673 reads the result)
> the roms are 64 bits wide
> so, well, the top bits of the code are suspicious
*/

static int _gxcommoninitnosprites(void)
{
	int i;

	K054338_vh_start();
	K055555_vh_start();

	if (konamigx_mixer_init(0)) return 1;

	for (i = 0; i < 8; i++)
	{
		gx_tilebanks[i] = gx_oldbanks[i] = 0;
	}

	state_save_register_INT32("KGXVideo", 0, "tilebanks", gx_tilebanks, 8);

	gx_invertlayersBC = 0;
	gx_tilemode = 0;

	// Documented relative offsets of non-flipped games are (-2, 0, 2, 3),(0, 0, 0, 0).
	// (+ve values move layers to the right and -ve values move layers to the left)
	// In most cases only a constant is needed to add to the X offsets to yield correct
	// displacement. This should be done by the CCU but the CRT timings have not been
	// figured out.
	K056832_set_LayerOffset(0, -2, 0);
	K056832_set_LayerOffset(1,  0, 0);
	K056832_set_LayerOffset(2,  2, 0);
	K056832_set_LayerOffset(3,  3, 0);

	return 0;
}

static int _gxcommoninit(void)
{
	// (+ve values move objects to the right and -ve values move objects to the left)
	if (K055673_vh_start(REGION_GFX2, K055673_LAYOUT_GX, -26, -23, konamigx_type2_sprite_callback))
	{
		return 1;
	}

	return _gxcommoninitnosprites();
}


VIDEO_START(konamigx_5bpp)
{
	if (!strcmp(Machine->gamedrv->name,"sexyparo"))
		game_tile_callback = konamigx_alpha_tile_callback;
	else
		game_tile_callback = konamigx_type2_tile_callback;

	if (K056832_vh_start(REGION_GFX1, K056832_BPP_5, 0, NULL, game_tile_callback))
	{
		return 1;
	}

	if (_gxcommoninit()) return 1;

	/* here are some hand tuned per game scroll offsets to go with the per game visible areas,
	   i see no better way of doing this for now... */

	if (!strcmp(Machine->gamedrv->name,"tbyahhoo"))
	{
		K056832_set_UpdateMode(1);
		gx_tilemode = 1;
	} else

	if (!strcmp(Machine->gamedrv->name,"puzldama"))
	{
		K053247GP_set_SpriteOffset(-46, -23);
		konamigx_mixer_primode(5);
	} else

	if (!strcmp(Machine->gamedrv->name,"daiskiss"))
	{
		konamigx_mixer_primode(4);
	} else

	if (!strcmp(Machine->gamedrv->name,"gokuparo") || !strcmp(Machine->gamedrv->name,"fantjour"))
 	{
		K053247GP_set_SpriteOffset(-46, -23);
	} else

	if (!strcmp(Machine->gamedrv->name,"sexyparo"))
	{
		K053247GP_set_SpriteOffset(-42, -23);
	}

	return 0;
}

VIDEO_START(dragoonj)
{
	if (K056832_vh_start(REGION_GFX1, K056832_BPP_5, 1, NULL, konamigx_type2_tile_callback))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, K055673_LAYOUT_RNG, -53, -23, konamigx_dragoonj_sprite_callback))
	{
		return 1;
	}

	if (_gxcommoninitnosprites()) return 1;

	K056832_set_LayerOffset(0, -2+1, 0);
	K056832_set_LayerOffset(1,  0+1, 0);
	K056832_set_LayerOffset(2,  2+1, 0);
	K056832_set_LayerOffset(3,  3+1, 0);

	return 0;
}

VIDEO_START(le2)
{
	if (K056832_vh_start(REGION_GFX1, K056832_BPP_8, 1, NULL, konamigx_type2_tile_callback))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, K055673_LAYOUT_LE2, -46, -23, konamigx_le2_sprite_callback))
	{
		return 1;
	}

	if (_gxcommoninitnosprites()) return 1;

	gx_invertlayersBC = 1;
	konamigx_mixer_primode(-1); // swapped layer B and C priorities?

	return 0;
}

VIDEO_START(konamigx_6bpp)
{
	if (K056832_vh_start(REGION_GFX1, K056832_BPP_6, 0, NULL, konamigx_type2_tile_callback))
	{
		return 1;
	}

	if (_gxcommoninit()) return 1;

	if (!strcmp(Machine->gamedrv->name,"tokkae") || !strcmp(Machine->gamedrv->name,"tkmmpzdm"))
	{
		K053247GP_set_SpriteOffset(-46, -23);
		konamigx_mixer_primode(5);
	}

	return 0;
}

VIDEO_START(konamigx_6bpp_2)
{
	if (K056832_vh_start(REGION_GFX1, K056832_BPP_6, 1, NULL, konamigx_type2_tile_callback))
	{
		return 1;
	}

	if (!strcmp(Machine->gamedrv->name,"salmndr2"))
	{
		if (K055673_vh_start(REGION_GFX2, K055673_LAYOUT_GX6, -48, -23, konamigx_salmndr2_sprite_callback))
		{
			return 1;
		}

		if (_gxcommoninitnosprites()) return 1;
	}
	else
	{
		if (_gxcommoninit()) return 1;
	}

	return 0;
}

VIDEO_START(konamigx_type1)
{
	if (K056832_vh_start(REGION_GFX1, K056832_BPP_5, 0, NULL, konamigx_type2_tile_callback))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, K055673_LAYOUT_GX6, -53, -23, konamigx_type2_sprite_callback))
	{
		return 1;
	}

	if (_gxcommoninitnosprites()) return 1;

	if (!strcmp(Machine->gamedrv->name,"opengolf"))
	{
		K056832_set_LayerOffset(0, -2+1, 0);
		K056832_set_LayerOffset(1,  0+1, 0);
		K056832_set_LayerOffset(2,  2+1, 0);
		K056832_set_LayerOffset(3,  3+1, 0);
	}

	return 0;
}


VIDEO_UPDATE(konamigx)
{
	int i, newbank, newbase, dirty, unchained, blendmode;

	/* if any banks are different from last render, we need to flush the planes */
	for (dirty = 0, i = 0; i < 8; i++)
	{
		newbank = gx_tilebanks[i];
		if (gx_oldbanks[i] != newbank) { gx_oldbanks[i] = newbank; dirty = 1; }
	}

	if (gx_tilemode == 0)
	{
		// driver approximates tile update in mode 0 for speed
		unchained = K056832_get_LayerAssociation();
		for (i=0; i<4; i++)
		{
			newbase = K055555_get_palette_index(i)<<6;
			if (layer_colorbase[i] != newbase)
			{
				layer_colorbase[i] = newbase;

				if (unchained)
					K056832_mark_plane_dirty(i);
				else
					dirty = 1;
			}
		}
	}
	else
	{
		// K056832 does all the tracking in mode 1 for accuracy (Twinbee needs this)
	}

	if (dirty) K056832_MarkAllTilemapsDirty();

	if (konamigx_cfgport >= 0)
	{
		// background detail tuning
		switch (readinputport(konamigx_cfgport))
		{
			// Low : disable linescroll and all blend effects
			case 0 : blendmode = 0x0000f555; break;

			// Med : only disable linescroll which is the most costly
			case 1 : blendmode = 0x0000f000; break;

			// High: enable all effects
			default: blendmode = 0;
		}

		// character detail tuning
		switch (readinputport(konamigx_cfgport+1))
		{
			// Low : disable shadows and turn off depth buffers
			case 0 : blendmode |= GXMIX_NOSHADOW + GXMIX_NOZBUF; break;

			// Med : only disable shadows
			case 1 : blendmode |= GXMIX_NOSHADOW; break;

			// High: enable all shadows and depth buffers
			default: blendmode |= 0;
		}
	}
	else blendmode = 0;

	konamigx_mixer(bitmap, cliprect, 0, 0, 0, 0, blendmode);

	if( gx_invertlayersBC )
	{
		draw_crosshair( bitmap, readinputport( 9)*287/0xff+24, readinputport(10)*223/0xff+16, cliprect );
		draw_crosshair( bitmap, readinputport(11)*287/0xff+24, readinputport(12)*223/0xff+16, cliprect );
	}
}


WRITE32_HANDLER( konamigx_palette_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram32[offset]);

 	r = (paletteram32[offset] >>16) & 0xff;
	g = (paletteram32[offset] >> 8) & 0xff;
	b = (paletteram32[offset] >> 0) & 0xff;

	palette_set_color(offset,r,g,b);
}

WRITE32_HANDLER( konamigx_tilebank_w )
{
	if (!(mem_mask & 0xff000000))
		gx_tilebanks[offset*4] = (data>>24)&0xff;
	if (!(mem_mask & 0xff0000))
		gx_tilebanks[offset*4+1] = (data>>16)&0xff;
	if (!(mem_mask & 0xff00))
		gx_tilebanks[offset*4+2] = (data>>8)&0xff;
	if (!(mem_mask & 0xff))
		gx_tilebanks[offset*4+3] = data&0xff;
}
