/***************************************************************************

  machine/psx.c

***************************************************************************/

#include "driver.h"
#include "includes/psx.h"

#define VERBOSE_LEVEL ( 0 )

static inline void verboselog( int n_level, const char *s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%08x: %s", activecpu_get_pc(), buf );
	}
}

static UINT32 m_n_irqdata;
static UINT32 m_n_irqmask;

static UINT32 m_n_dmabase[ 7 ];
static UINT32 m_n_dmablockcontrol[ 7 ];
static UINT32 m_n_dmachannelcontrol[ 7 ];
static UINT32 m_n_dpcp;
static UINT32 m_n_dicr;

WRITE32_HANDLER( psxirq_w )
{
	switch( offset )
	{
	case 0x00:
		m_n_irqdata = ( m_n_irqdata & mem_mask ) | ( m_n_irqdata & m_n_irqmask & data );
		if( ( m_n_irqdata & m_n_irqmask ) == 0 )
		{
			verboselog( 1, "psxirq_w() irq clear\n" );
			cpu_set_irq_line( 0, 0, CLEAR_LINE );
		}
		break;
	case 0x01:
		m_n_irqmask = ( m_n_irqmask & mem_mask ) | data;
		if( ( m_n_irqmask & ~( 0x1 | 0x08 | 0x20 ) ) != 0 )
		{
			verboselog( 0, "psxirq_w( %08x, %08x, %08x ) unknown irq\n", offset, data, mem_mask );
		}
		if( ( m_n_irqdata & m_n_irqmask ) != 0 )
		{
			verboselog( 1, "psxirq_w() irq assert\n" );
			cpu_set_irq_line( 0, 0, ASSERT_LINE );
		}
		break;
	default:
		verboselog( 0, "psxirq_w( %08x, %08x, %08x ) unknown register\n", offset, data, mem_mask );
		break;
	}
}

READ32_HANDLER( psxirq_r )
{
	switch( offset )
	{
	case 0x00:
		return m_n_irqdata;
	case 0x01:
		return m_n_irqmask;
	default:
		verboselog( 0, "unknown irq read %d\n", offset );
		break;
	}
	return 0;
}

void psxirq_set( UINT32 data )
{
	m_n_irqdata |= data;
	if( ( m_n_irqdata & m_n_irqmask ) != 0 )
	{
		cpu_set_irq_line( 0, 0, ASSERT_LINE );
	}
}

WRITE32_HANDLER( psxdma_w )
{
	static int n_channel;
	n_channel = offset / 4;
	if( n_channel < 7 )
	{
		switch( offset % 4 )
		{
		case 0:
			m_n_dmabase[ n_channel ] = data;
			break;
		case 1:
			m_n_dmablockcontrol[ n_channel ] = data;
			break;
		case 2:
			if( ( data & ( 1L << 0x18 ) ) != 0 && ( m_n_dpcp & ( 1 << ( 3 + ( n_channel * 4 ) ) ) ) != 0 )
			{
				static UINT32 n_size;
				static UINT32 n_address;
				static UINT32 n_nextaddress;
				static unsigned char *p_ram;

				p_ram = memory_region( REGION_CPU1 );
				n_address = m_n_dmabase[ n_channel ] & 0xffffff;

				if( n_channel == 2 )
				{
					if( data == 0x01000200 )
					{
						verboselog( 1, "dma 2 read block\n", data );
						n_size = (UINT32)( m_n_dmablockcontrol[ n_channel ] >> 16 ) * (UINT32)( m_n_dmablockcontrol[ n_channel ] & 0xffff );
						while( n_size != 0 )
						{
							*( (unsigned long *)&p_ram[ n_address ] ) = psxgpu_r( 0, 0 );
							n_address += 4;
							n_size--;
						}
						data &= ~( 1L << 0x18 );
					}
					else if( data == 0x01000201 )
					{
						verboselog( 1, "dma 2 write block\n", data );
						n_size = (UINT32)( m_n_dmablockcontrol[ n_channel ] >> 16 ) * (UINT32)( m_n_dmablockcontrol[ n_channel ] & 0xffff );
						while( n_size != 0 )
						{
							psxgpu_w( 0, *( (unsigned long *)&p_ram[ n_address ] ), 0 );
							n_address += 4;
							n_size--;
						}
						data &= ~( 1L << 0x18 );
					}
					else if( data == 0x01000401 )
					{
						verboselog( 1, "dma 2 write linked list\n", data );
						do
						{
							n_nextaddress = *( (unsigned long *)&p_ram[ n_address ] );
							n_address += 4;
							n_size = ( n_nextaddress >> 24 );
							while( n_size != 0 )
							{
								psxgpu_w( 0, *( (unsigned long *)&p_ram[ n_address ] ), 0 );
								n_address += 4;
								n_size--;
							}
							n_address = ( n_nextaddress & 0xffffff );
						} while( n_address != 0xffffff );
						data &= ~( 1L << 0x18 );
					}
					else
					{
						verboselog( 0, "dma 2 unknown mode %08x\n", data );
					}
				}
				else if( n_channel == 6 && data == 0x11000002 )
				{
					verboselog( 1, "dma 6 reverse clear\n", data );
					n_size = m_n_dmablockcontrol[ n_channel ];
					while( n_size != 0 )
					{
						n_nextaddress = ( n_address - 4 ) & 0xffffff;
						*( (unsigned long *)&p_ram[ n_address ] ) = n_nextaddress;
						n_address = n_nextaddress;
						n_size--;
					}
					*( (unsigned long *)&p_ram[ n_address ] ) = 0xffffff;

					data &= ~( 1L << 0x18 );
				}
				else
				{
					verboselog( 0, "unknown dma channel / mode (%d / %08x)\n", n_channel, data );
				}
				if( ( m_n_dicr & ( 1 << ( 16 + n_channel ) ) ) != 0 )
				{
					m_n_dicr |= 0x80000000 | ( 1 << ( 24 + n_channel ) );
					psxirq_set( 0x0008 );
					verboselog( 1, "psxdma_w( %04x, %08x, %08x ) interrupt triggered\n", offset, data, mem_mask );
				}
				else
				{
					verboselog( 1, "psxdma_w( %04x, %08x, %08x ) interrupt not enabled\n", offset, data, mem_mask );
				}
			}
			else
			{
				verboselog( 1, "psxdma_w( %04x, %08x, %08x ) channel not enabled\n", offset, data, mem_mask );
			}
			m_n_dmachannelcontrol[ n_channel ] = data;
			break;
		default:
			verboselog( 1, "psxdma_w( %04x, %08x, %08x ) Unknown dma channel register\n", offset, data, mem_mask );
			break;
		}
	}
	else
	{
		switch( offset % 4 )
		{
		case 0x0:
			m_n_dpcp = ( m_n_dpcp & mem_mask ) | data;
			break;
		case 0x1:
			m_n_dicr = ( m_n_dicr & mem_mask ) | ( data & 0xffffff );
			break;
		default:
			verboselog( 1, "psxdma_w( %04x, %08x, %08x ) Unknown dma control register\n", offset, data, mem_mask );
			break;
		}
	}
}

READ32_HANDLER( psxdma_r )
{
	static int n_channel;
	n_channel = offset / 4;
	if( n_channel < 7 )
	{
		switch( offset % 4 )
		{
		case 0:
			verboselog( 1, "psxdma_r dmabase[ %d ] ( %08x )\n", n_channel, m_n_dmabase[ n_channel ] );
			return m_n_dmabase[ n_channel ];
		case 1:
			verboselog( 1, "psxdma_r dmablockcontrol[ %d ] ( %08x )\n", n_channel, m_n_dmablockcontrol[ n_channel ] );
			return m_n_dmablockcontrol[ n_channel ];
		case 2:
			verboselog( 1, "psxdma_r dmachannelcontrol[ %d ] ( %08x )\n", n_channel, m_n_dmachannelcontrol[ n_channel ] );
			return m_n_dmachannelcontrol[ n_channel ];
		default:
			verboselog( 0, "psxdma_r( %08x, %08x ) Unknown dma channel register\n", offset, mem_mask );
			break;
		}
	}
	else
	{
		switch( offset % 4 )
		{
		case 0x0:
			verboselog( 1, "psxdma_r dpcp ( %08x )\n", m_n_dpcp );
			return m_n_dpcp;
		case 0x1:
			verboselog( 1, "psxdma_r dicr ( %08x )\n", m_n_dicr );
			return m_n_dicr;
		default:
			verboselog( 0, "psxdma_r( %08x, %08x ) Unknown dma control register\n", offset, mem_mask );
			break;
		}
	}
	return 0;
}

READ32_HANDLER( psxcounter_r )
{
	return activecpu_gettotalcycles() & 0xffff;
}

WRITE32_HANDLER( psxcounter_w )
{
	verboselog( 1, "psxcounter_w %08x %08x %08x\n", data, offset, mem_mask );
}

MACHINE_INIT( psx )
{
	/* irq */
	m_n_irqdata = 0;
	m_n_irqmask = 0;

	/* dma */
	m_n_dpcp = 0;
	m_n_dicr = 0;

	psxgpu_reset();
}
