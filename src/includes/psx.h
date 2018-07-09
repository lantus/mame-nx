/***************************************************************************

  includes/psx.h

***************************************************************************/

/* vidhrdw */
READ32_HANDLER( psxgpu_r );
WRITE32_HANDLER( psxgpu_w );

PALETTE_INIT( psxgpu );
VIDEO_START( psxgpu1024x512 );
VIDEO_START( psxgpu1024x1024 );
VIDEO_UPDATE( psxgpu );
VIDEO_STOP( psxgpu );
INTERRUPT_GEN( psx );
void psxgpu_reset( void );

/* machine */
WRITE32_HANDLER( psxirq_w );
READ32_HANDLER( psxirq_r );
void psxirq_set( UINT32 );
WRITE32_HANDLER( psxdma_w );
READ32_HANDLER( psxdma_r );
READ32_HANDLER( psxcounter_r );
WRITE32_HANDLER( psxcounter_w );
MACHINE_INIT( psx );
