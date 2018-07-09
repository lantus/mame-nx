/*************************************************************************

	Sega Pengo

**************************************************************************/

/*----------- defined in vidhrdw/pengo.c -----------*/

PALETTE_INIT( pacman );
PALETTE_INIT( pengo );

VIDEO_START( pacman );
VIDEO_START( pengo );

WRITE_HANDLER( pengo_gfxbank_w );
WRITE_HANDLER( pengo_flipscreen_w );

VIDEO_UPDATE( pengo );

WRITE_HANDLER( vanvan_bgcolor_w );
VIDEO_UPDATE( vanvan );
