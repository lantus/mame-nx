/*************************************************************************

	Atari GT hardware

*************************************************************************/

/*----------- defined in vidhrdw/atarigx2.c -----------*/

extern data16_t *atarigt_colorram;
extern UINT16 atarigt_motion_object_mask;

void atarigt_colorram_w(offs_t address, data16_t data, data16_t mem_mask);
data16_t atarigt_colorram_r(offs_t address);

VIDEO_START( atarigt );
VIDEO_UPDATE( atarigt );

void atarigt_scanline_update(int scanline);
