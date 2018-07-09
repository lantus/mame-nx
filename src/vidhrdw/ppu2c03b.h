/******************************************************************************

	Nintendo 2C03B PPU emulation.

	Written by Ernesto Corvi.
	This code is heavily based on Brad Oliver's MESS implementation.

******************************************************************************/
#ifndef __PPU_2C03B_H__
#define __PPU_2C03B_H__

#include "driver.h"

/* increment to use more chips */
#define MAX_PPU 2

/* mirroring types */
#define PPU_MIRROR_NONE		0
#define PPU_MIRROR_VERT		1
#define PPU_MIRROR_HORZ		2
#define PPU_MIRROR_HIGH		3
#define PPU_MIRROR_LOW		4

/* callback datatypes */
typedef void (*ppu2c03b_scanline_cb)( int num, int scanline, int vblank, int blanked );
typedef void (*ppu2c03b_irq_cb)( int num );
typedef int  (*ppu2c03b_vidaccess_cb)( int num, int address, int data );

struct ppu2c03b_interface
{
	int				num;							/* number of chips ( 1 to MAX_PPU ) */
	int				vrom_region[MAX_PPU];			/* region id of gfx vrom (or REGION_INVALID if none) */
	int				gfx_layout_number[MAX_PPU];		/* gfx layout number used by each chip */
	int				color_base[MAX_PPU];			/* color base to use per ppu */
	int				mirroring[MAX_PPU];				/* mirroring options (PPU_MIRROR_* flag) */
	ppu2c03b_irq_cb	handler[MAX_PPU];				/* IRQ handler */
};

/* routines */
void ppu2c03b_init_palette( int first_entry );
int ppu2c03b_init( struct ppu2c03b_interface *interface );

void ppu2c03b_reset( int num, int scan_scale );
void ppu2c03b_set_videorom_bank( int num, int start_page, int num_pages, int bank, int bank_size );
void ppu2c03b_spriteram_dma( int num, const UINT8 *source );
void ppu2c03b_render( int num, struct mame_bitmap *bitmap, int flipx, int flipy, int sx, int sy );
int ppu2c03b_get_pixel( int num, int x, int y );
int ppu2c03b_get_colorbase( int num );
void ppu2c03b_set_mirroring( int num, int mirroring );
void ppu2c03b_set_irq_callback( int num, ppu2c03b_irq_cb cb );
void ppu2c03b_set_scanline_callback( int num, ppu2c03b_scanline_cb cb );
void ppu2c03b_set_vidaccess_callback( int num, ppu2c03b_vidaccess_cb cb );

//27/12/2002
extern void (*ppu_latch)( offs_t offset );

void ppu2c03b_w( int num, int offset, int data );
int ppu2c03b_r( int num, int offset );

/* accesors */
READ_HANDLER( ppu2c03b_0_r );
READ_HANDLER( ppu2c03b_1_r );

WRITE_HANDLER( ppu2c03b_0_w );
WRITE_HANDLER( ppu2c03b_1_w );

#endif /* __PPU_2C03B_H__ */
