#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/segaic24.h"

static int mode = 0;

VIDEO_START(system24)
{
	if(sys24_tile_vh_start(0xfff))
		return 1;

	if(sys24_sprite_vh_start())
		return 1;

	mode = 0;

	return 0;
}


VIDEO_UPDATE(system24)
{
	sys24_tile_update();

#ifdef MAME_DEBUG
	if(keyboard_pressed(KEYCODE_I))
		mode = 0;
	else if(keyboard_pressed(KEYCODE_O))
		mode = 1;
#endif

	fillbitmap(priority_bitmap, 0, 0);
	fillbitmap(bitmap, Machine->pens[0], &Machine->visible_area);

	if(mode) {
		sys24_tile_draw(bitmap, cliprect, 1, 0);
		sys24_tile_draw(bitmap, cliprect, 0, 0);
		sys24_tile_draw(bitmap, cliprect, 3, 0);
		sys24_tile_draw(bitmap, cliprect, 2, 0);
	} else {
		sys24_tile_draw(bitmap, cliprect, 3, 0);
		sys24_tile_draw(bitmap, cliprect, 2, 0);
		sys24_tile_draw(bitmap, cliprect, 1, 0);
		sys24_tile_draw(bitmap, cliprect, 0, 0);
	}

	sys24_sprite_draw(bitmap, cliprect);

}
