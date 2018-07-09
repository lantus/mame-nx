/***************************************************************************

	PSX GPU - CXD8538Q

	Preliminary software renderer.
	Supports 1024x512 & 1024x1024 framebuffer.
	No dithering.
	Press M when running a debug build to view a polygon mesh.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
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

static UINT16 *m_p_vram;
static UINT16 *m_p_p_vram[ 1024 ];
static UINT8 m_n_gpu_buffer_offset;
static UINT32 m_n_vramx;
static UINT32 m_n_vramy;
static UINT32 m_n_twy;
static UINT32 m_n_twx;
static UINT32 m_n_twh;
static UINT32 m_n_tww;
static UINT32 m_n_drawarea_x1;
static UINT32 m_n_drawarea_y1;
static UINT32 m_n_drawarea_x2;
static UINT32 m_n_drawarea_y2;
static UINT32 m_n_horiz_disstart;
static UINT32 m_n_horiz_disend;
static UINT32 m_n_vert_disstart;
static UINT32 m_n_vert_disend;
static UINT32 m_n_drawoffset_x;
static UINT32 m_n_drawoffset_y;
static UINT32 m_n_displaystartx;
static UINT32 m_n_displaystarty;
static UINT32 m_n_gpustatus;
static UINT32 m_n_gpuinfo;
static int m_n_screenwidth;
static int m_n_screenheight;

#define MAX_LEVEL ( 32 )
#define MID_LEVEL ( ( MAX_LEVEL / 2 ) << 8 )
#define MAX_SHADE ( 0x100 )
#define MID_SHADE ( 0x80 )

UINT16 m_p_n_redshade[ MAX_LEVEL * MAX_SHADE ];
UINT16 m_p_n_greenshade[ MAX_LEVEL * MAX_SHADE ];
UINT16 m_p_n_blueshade[ MAX_LEVEL * MAX_SHADE ];
UINT16 m_p_n_redlevel[ 0x10000 ];
UINT16 m_p_n_greenlevel[ 0x10000 ];
UINT16 m_p_n_bluelevel[ 0x10000 ];

UINT16 m_p_n_f025[ MAX_LEVEL * MAX_SHADE ];
UINT16 m_p_n_f05[ MAX_LEVEL * MAX_SHADE ];
UINT16 m_p_n_f1[ MAX_LEVEL * MAX_SHADE ];
UINT16 m_p_n_redb05[ 0x10000 ];
UINT16 m_p_n_greenb05[ 0x10000 ];
UINT16 m_p_n_blueb05[ 0x10000 ];
UINT16 m_p_n_redb1[ 0x10000 ];
UINT16 m_p_n_greenb1[ 0x10000 ];
UINT16 m_p_n_blueb1[ 0x10000 ];
UINT16 m_p_n_redaddtrans[ MAX_LEVEL * MAX_LEVEL ];
UINT16 m_p_n_greenaddtrans[ MAX_LEVEL * MAX_LEVEL ];
UINT16 m_p_n_blueaddtrans[ MAX_LEVEL * MAX_LEVEL ];
UINT16 m_p_n_redsubtrans[ MAX_LEVEL * MAX_LEVEL ];
UINT16 m_p_n_greensubtrans[ MAX_LEVEL * MAX_LEVEL ];
UINT16 m_p_n_bluesubtrans[ MAX_LEVEL * MAX_LEVEL ];

#define SINT12( x ) ( ( (INT32)( x ) << 21 ) >> 21 )

#define SET_COORD_X( a, b ) a.w.l = b
#define SET_COORD_Y( a, b ) a.w.h = b

#define COORD_X( a ) (INT16)( a.w.l )
#define COORD_Y( a ) (INT16)( a.w.h )
#define SIZE_W( a ) ( a.w.l )
#define SIZE_H( a ) ( a.w.h )
#define BGR_C( a ) ( a.b.h3 )
#define BGR_B( a ) ( a.b.h2 )
#define BGR_G( a ) ( a.b.h )
#define BGR_R( a ) ( a.b.l )
#define TEXTURE_V( a ) (INT16)( a.b.h )
#define TEXTURE_U( a ) (INT16)( a.b.l )

struct FLATVERTEX
{
	PAIR n_coord;
};

struct GOURAUDVERTEX
{
	PAIR n_bgr;
	PAIR n_coord;
};

struct FLATTEXTUREDVERTEX
{
	PAIR n_coord;
	PAIR n_texture;
};

struct GOURAUDTEXTUREDVERTEX
{
	PAIR n_bgr;
	PAIR n_coord;
	PAIR n_texture;
};

union
{
	UINT32 n_entry[ 16 ];

	struct
	{
		PAIR n_bgr;
		PAIR n_coord;
		PAIR n_size;
	} FlatRectangle;

	struct
	{
		PAIR n_bgr;
		PAIR n_coord;
		PAIR n_texture;
	} Sprite8x8;

	struct
	{
		PAIR n_bgr;
		PAIR n_coord;
		PAIR n_texture;
		PAIR n_size;
	} FlatTexturedRectangle;

	struct
	{
		PAIR n_bgr;
		struct FLATVERTEX vertex[ 4 ];
	} FlatPolygon;

	struct
	{
		struct GOURAUDVERTEX vertex[ 4 ];
	} GouraudPolygon;

	struct
	{
		PAIR n_bgr;
		struct FLATVERTEX vertex[ 2 ];
	} MonochromeLine;

	struct
	{
		PAIR n_bgr;
		struct FLATTEXTUREDVERTEX vertex[ 4 ];
	} FlatTexturedPolygon;

	struct
	{
		struct GOURAUDTEXTUREDVERTEX vertex[ 4 ];
	} GouraudTexturedPolygon;

} m_packet;

PALETTE_INIT( psxgpu )
{
	UINT32 n_r;
	UINT32 n_g;
	UINT32 n_b;
	UINT32 n_colour;

	for( n_colour = 0; n_colour < 0x10000; n_colour++ )
	{
		n_r = ( ( n_colour & 0x1f ) * 0xff ) / 0x1f;
		n_g = ( ( ( n_colour >> 5 ) & 0x1f ) * 0xff ) / 0x1f;
		n_b = ( ( ( n_colour >> 10 ) & 0x1f ) * 0xff ) / 0x1f;

		palette_set_color( n_colour, n_r, n_g, n_b );
	}
}

#if defined( MAME_DEBUG )

static struct mame_bitmap *debugmesh;
static int m_b_debugclear;
static int m_b_debugmesh;
static int m_n_debugcoord;
static int m_n_debugcoordx[ 10 ];
static int m_n_debugcoordy[ 10 ];

#define DEBUG_MAX ( 512 )

static void DebugMeshInit( void )
{
	m_b_debugmesh = 0;
	m_b_debugclear = 1;
	m_n_debugcoord = 0;
	debugmesh = auto_bitmap_alloc_depth( 480, 480, 16 );
}

static void DebugMesh( int n_coordx, int n_coordy )
{
	int n_coord;
	int n_colour;

	if( m_b_debugclear )
	{
		fillbitmap( debugmesh, 0x0000, NULL );
		m_b_debugclear = 0;
	}

	if( m_n_debugcoord < 10 )
	{
		n_coordx += m_n_displaystartx;
		n_coordy += m_n_displaystarty;

		n_coordx *= 239;
		n_coordx /= DEBUG_MAX - 1;
		n_coordx += 120;
		n_coordy *= 239;
		n_coordy /= DEBUG_MAX - 1;
		n_coordy += 120;

		if( n_coordx > 479 )
		{
			n_coordx = 479;
		}
		if( n_coordx < 0 )
		{
			n_coordx = 0;
		}

		if( n_coordy > 479 )
		{
			n_coordy = 479;
		}
		if( n_coordy < 0 )
		{
			n_coordy = 0;
		}

		m_n_debugcoordx[ m_n_debugcoord ] = n_coordx;
		m_n_debugcoordy[ m_n_debugcoord ] = n_coordy;
		m_n_debugcoord++;
	}

	n_colour = 0x1f;
	for( n_coord = 0; n_coord < m_n_debugcoord; n_coord++ )
	{
		if( n_coordx != m_n_debugcoordx[ n_coord ] ||
			n_coordy != m_n_debugcoordy[ n_coord ] )
		{
			break;
		}
	}
	if( n_coord == m_n_debugcoord && m_n_debugcoord > 1 )
	{
		n_colour = 0xffff;
	}
	for( n_coord = 0; n_coord < m_n_debugcoord; n_coord++ )
	{
		PAIR n_x;
		PAIR n_y;
		INT32 n_xstart;
		INT32 n_ystart;
		INT32 n_xend;
		INT32 n_yend;
		INT32 n_xlen;
		INT32 n_ylen;
		INT32 n_len;
		INT32 n_dx;
		INT32 n_dy;

		n_xstart = m_n_debugcoordx[ n_coord ];
		n_xend = n_coordx;
		if( n_xend > n_xstart )
		{
			n_xlen = n_xend - n_xstart;
		}
		else
		{
			n_xlen = n_xstart - n_xend;
		}

		n_ystart = m_n_debugcoordy[ n_coord ];
		n_yend = n_coordy;
		if( n_yend > n_ystart )
		{
			n_ylen = n_yend - n_ystart;
		}
		else
		{
			n_ylen = n_ystart - n_yend;
		}

		if( n_xlen > n_ylen )
		{
			n_len = n_xlen;
		}
		else
		{
			n_len = n_ylen;
		}

		n_x.w.h = n_xstart; n_x.w.l = 0;
		n_y.w.h = n_ystart; n_y.w.l = 0;

		if( n_len == 0 )
		{
			n_len = 1;
		}

		n_dx = (INT32)( ( n_xend << 16 ) - n_x.d ) / n_len;
		n_dy = (INT32)( ( n_yend << 16 ) - n_y.d ) / n_len;
		while( n_len > 0 )
		{
			/* moving this outside the loop would be good */
			if( /* n_x.w.h >= 0 &&
				n_y.w.h >= 0 && */
				n_x.w.h <= 479 &&
				n_y.w.h <= 479 )
			{
				if( read_pixel( debugmesh, n_x.w.h, n_y.w.h ) != 0xffff )
				{
					plot_pixel( debugmesh, n_x.w.h, n_y.w.h, n_colour );
				}
			}
			n_x.d += n_dx;
			n_y.d += n_dy;
			n_len--;
		}
	}
}

static void DebugMeshEnd( void )
{
	m_n_debugcoord = 0;
}

static int DebugMeshDisplay( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	if( keyboard_pressed_memory( KEYCODE_M ) )
	{
		m_b_debugmesh = !m_b_debugmesh;
	}
	if( m_b_debugmesh )
	{
		set_visible_area( 0, 479, 0, 479 );
		copybitmap( bitmap, debugmesh, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0 );
	}
	m_b_debugclear = 1;
	return m_b_debugmesh;
}
#endif

static int psxgpuinit( int n_width, int n_height )
{
	int n_line;
	int n_level;
	int n_shade;
	int n_shaded;

#if defined( MAME_DEBUG )
	DebugMeshInit();
#endif

	m_n_gpustatus = 0x14802000;
	m_n_gpuinfo = 0;
	m_n_gpu_buffer_offset = 0;

	m_p_vram = auto_malloc( n_width * n_height * 2 );
	if( m_p_vram == NULL )
	{
		return 1;
	}
	memset( m_p_vram, 0x00, n_width * n_height * 2 );

	for( n_line = 0; n_line < 1024; n_line++ )
	{
		m_p_p_vram[ n_line ] = &m_p_vram[ ( n_line % n_height ) * n_width ];
	}

	for( n_level = 0; n_level < MAX_LEVEL; n_level++ )
	{
		for( n_shade = 0; n_shade < MAX_SHADE; n_shade++ )
		{
			/* shaded */
			n_shaded = ( n_level * n_shade ) / MID_SHADE;
			if( n_shaded > MAX_LEVEL - 1 )
			{
				n_shaded = MAX_LEVEL - 1;
			}
			m_p_n_redshade[ ( n_level * MAX_SHADE ) | n_shade ] = n_shaded;
			m_p_n_greenshade[ ( n_level * MAX_SHADE ) | n_shade ] = n_shaded << 5;
			m_p_n_blueshade[ ( n_level * MAX_SHADE ) | n_shade ] = n_shaded << 10;

			/* 1/4 x transparency */
			n_shaded = ( n_level * n_shade ) / MID_SHADE;
			n_shaded >>= 2;
			if( n_shaded > MAX_LEVEL - 1 )
			{
				n_shaded = MAX_LEVEL - 1;
			}
			m_p_n_f025[ ( n_level * MAX_SHADE ) | n_shade ] = n_shaded;

			/* 1/2 x transparency */
			n_shaded = ( n_level * n_shade ) / MID_SHADE;
			n_shaded >>= 1;
			if( n_shaded > MAX_LEVEL - 1 )
			{
				n_shaded = MAX_LEVEL - 1;
			}
			m_p_n_f05[ ( n_level * MAX_SHADE ) | n_shade ] = n_shaded;

			/* 1 x transparency */
			n_shaded = ( n_level * n_shade ) / MID_SHADE;
			if( n_shaded > MAX_LEVEL - 1 )
			{
				n_shaded = MAX_LEVEL - 1;
			}
			m_p_n_f1[ ( n_level * MAX_SHADE ) | n_shade ] = n_shaded;
		}
	}

	for( n_level = 0; n_level < 0x10000; n_level++ )
	{
		m_p_n_redlevel[ n_level ] = ( n_level & ( MAX_LEVEL - 1 ) ) * MAX_SHADE;
		m_p_n_greenlevel[ n_level ] = ( ( n_level >> 5 ) & ( MAX_LEVEL - 1 ) ) * MAX_SHADE;
		m_p_n_bluelevel[ n_level ] = ( ( n_level >> 10 ) & ( MAX_LEVEL - 1 ) ) * MAX_SHADE;

		/* 0.5 * background */
		m_p_n_redb05[ n_level ] = ( ( n_level & ( MAX_LEVEL - 1 ) ) / 2 ) * MAX_LEVEL;
		m_p_n_greenb05[ n_level ] = ( ( ( n_level >> 5 ) & ( MAX_LEVEL - 1 ) ) / 2 ) * MAX_LEVEL;
		m_p_n_blueb05[ n_level ] = ( ( ( n_level >> 10 ) & ( MAX_LEVEL - 1 ) ) / 2 ) * MAX_LEVEL;

		/* 1 * background */
		m_p_n_redb1[ n_level ] = ( n_level & ( MAX_LEVEL - 1 ) ) * MAX_LEVEL;
		m_p_n_greenb1[ n_level ] = ( ( n_level >> 5 ) & ( MAX_LEVEL - 1 ) ) * MAX_LEVEL;
		m_p_n_blueb1[ n_level ] = ( ( n_level >> 10 ) & ( MAX_LEVEL - 1 ) ) * MAX_LEVEL;

	}

	for( n_level = 0; n_level < MAX_LEVEL; n_level++ )
	{
		int n_level2;
		for( n_level2 = 0; n_level2 < MAX_LEVEL; n_level2++ )
		{
			/* add transparency */
			n_shaded = ( n_level + n_level2 );
			if( n_shaded > MAX_LEVEL - 1 )
			{
				n_shaded = MAX_LEVEL - 1;
			}
			m_p_n_redaddtrans[ ( n_level * MAX_LEVEL ) | n_level2 ] = n_shaded;
			m_p_n_greenaddtrans[ ( n_level * MAX_LEVEL ) | n_level2 ] = n_shaded << 5;
			m_p_n_blueaddtrans[ ( n_level * MAX_LEVEL ) | n_level2 ] = n_shaded << 10;

			/* sub transparency */
			n_shaded = ( n_level - n_level2 );
			if( n_shaded < 0 )
			{
				n_shaded = 0;
			}
			m_p_n_redsubtrans[ ( n_level * MAX_LEVEL ) | n_level2 ] = n_shaded;
			m_p_n_greensubtrans[ ( n_level * MAX_LEVEL ) | n_level2 ] = n_shaded << 5;
			m_p_n_bluesubtrans[ ( n_level * MAX_LEVEL ) | n_level2 ] = n_shaded << 10;
		}
	}
	return 0;
}

VIDEO_START( psxgpu1024x1024 )
{
	return psxgpuinit( 1024, 1024 );
}

VIDEO_START( psxgpu1024x512 )
{
	return psxgpuinit( 1024, 512 );
}

VIDEO_STOP( psxgpu )
{
}

VIDEO_UPDATE( psxgpu )
{
	UINT32 n_y;
	int width;
	int xoffs;

#if defined( MAME_DEBUG )
	if( DebugMeshDisplay( bitmap, cliprect ) )
	{
		return;
	}
#endif

	set_visible_area( 0, m_n_screenwidth - 1, 0, m_n_screenheight - 1 );

	xoffs = cliprect->min_x;
	width = cliprect->max_x - xoffs + 1;

	for( n_y = 0; n_y < bitmap->height; n_y++ )
	{
		draw_scanline16( bitmap, xoffs, n_y, width, m_p_p_vram[ n_y + cliprect->min_y + m_n_displaystarty ] + xoffs + m_n_displaystartx, Machine->pens, -1 );
	}
}

static void FlatPolygon( int n_points )
{
	/* Draws a flat polygon. No transparency / dithering & probably not pixel perfect. */

	INT16 n_y;
	INT16 n_x;

	PAIR n_r;
	PAIR n_g;
	PAIR n_b;
	PAIR n_cx1;
	PAIR n_cx2;
	INT32 n_dx1;
	INT32 n_dx2;
	UINT32 n_bgr;

	INT32 n_distance;

	UINT16 n_point;
	UINT16 n_rightpoint;
	UINT16 n_leftpoint;
	UINT16 p_n_nextpointlist4[] = { 1, 3, 0, 2 };
	UINT16 p_n_prevpointlist4[] = { 2, 0, 3, 1 };
	UINT16 p_n_nextpointlist3[] = { 1, 2, 0 };
	UINT16 p_n_prevpointlist3[] = { 2, 0, 1 };
	UINT16 *p_n_nextpointlist;
	UINT16 *p_n_prevpointlist;
	UINT16 *p_n_rightpointlist;
	UINT16 *p_n_leftpointlist;

	UINT16 *p_vram;

	if( n_points == 4 )
	{
		p_n_nextpointlist = p_n_nextpointlist4;
		p_n_prevpointlist = p_n_prevpointlist4;
	}
	else
	{
		p_n_nextpointlist = p_n_nextpointlist3;
		p_n_prevpointlist = p_n_prevpointlist3;
	}

	for( n_point = 0; n_point < n_points; n_point++ )
	{
		SET_COORD_X( m_packet.FlatPolygon.vertex[ n_point ].n_coord, SINT12( COORD_X( m_packet.FlatPolygon.vertex[ n_point ].n_coord ) + m_n_drawoffset_x ) );
		SET_COORD_Y( m_packet.FlatPolygon.vertex[ n_point ].n_coord, SINT12( COORD_Y( m_packet.FlatPolygon.vertex[ n_point ].n_coord ) + m_n_drawoffset_y ) );
#if defined( MAME_DEBUG )
		DebugMesh( COORD_X( m_packet.FlatPolygon.vertex[ n_point ].n_coord ), COORD_Y( m_packet.FlatPolygon.vertex[ n_point ].n_coord ) );
#endif
	}
#if defined( MAME_DEBUG )
	DebugMeshEnd();
#endif

	n_leftpoint = 0;
	for( n_point = 0; n_point < n_points; n_point++ )
	{
		if( (INT32)m_packet.FlatPolygon.vertex[ n_point ].n_coord.d < (INT32)m_packet.FlatPolygon.vertex[ n_leftpoint ].n_coord.d )
		{
			n_leftpoint = n_point;
		}
	}
	n_rightpoint = n_leftpoint;

	if( COORD_X( m_packet.FlatPolygon.vertex[ p_n_nextpointlist[ n_rightpoint ] ].n_coord ) > COORD_X( m_packet.FlatPolygon.vertex[ p_n_prevpointlist[ n_rightpoint ] ].n_coord ) )
	{
		p_n_rightpointlist = p_n_nextpointlist;
		p_n_leftpointlist = p_n_prevpointlist;
	}
	else
	{
		p_n_rightpointlist = p_n_prevpointlist;
		p_n_leftpointlist = p_n_nextpointlist;
	}

	n_r.w.h = BGR_R( m_packet.FlatPolygon.n_bgr ); n_r.w.l = 0;
	n_g.w.h = BGR_G( m_packet.FlatPolygon.n_bgr ); n_g.w.l = 0;
	n_b.w.h = BGR_B( m_packet.FlatPolygon.n_bgr ); n_b.w.l = 0;

	n_dx1 = 0;
	n_dx2 = 0;

	n_y = COORD_Y( m_packet.FlatPolygon.vertex[ n_rightpoint ].n_coord );

	for( ;; )
	{
		if( n_y == COORD_Y( m_packet.FlatPolygon.vertex[ n_leftpoint ].n_coord ) )
		{
			while( n_y == COORD_Y( m_packet.FlatPolygon.vertex[ p_n_leftpointlist[ n_leftpoint ] ].n_coord ) )
			{
				n_leftpoint = p_n_leftpointlist[ n_leftpoint ];
				if( n_leftpoint == n_rightpoint )
				{
					break;
				}
			}
			n_cx1.w.h = COORD_X( m_packet.FlatPolygon.vertex[ n_leftpoint ].n_coord ); n_cx1.w.l = 0;
			n_leftpoint = p_n_leftpointlist[ n_leftpoint ];
			n_distance = COORD_Y( m_packet.FlatPolygon.vertex[ n_leftpoint ].n_coord ) - n_y;
			if( n_distance < 1 )
			{
				break;
			}
			n_dx1 = (INT32)( ( COORD_X( m_packet.FlatPolygon.vertex[ n_leftpoint ].n_coord ) << 16 ) - n_cx1.d ) / n_distance;
		}
		if( n_y == COORD_Y( m_packet.FlatPolygon.vertex[ n_rightpoint ].n_coord ) )
		{
			while( n_y == COORD_Y( m_packet.FlatPolygon.vertex[ p_n_rightpointlist[ n_rightpoint ] ].n_coord ) )
			{
				n_rightpoint = p_n_rightpointlist[ n_rightpoint ];
				if( n_rightpoint == n_leftpoint )
				{
					break;
				}
			}
			n_cx2.w.h = COORD_X( m_packet.FlatPolygon.vertex[ n_rightpoint ].n_coord ); n_cx2.w.l = 0;
			n_rightpoint = p_n_rightpointlist[ n_rightpoint ];
			n_distance = COORD_Y( m_packet.FlatPolygon.vertex[ n_rightpoint ].n_coord ) - n_y;
			if( n_distance < 1 )
			{
				break;
			}
			n_dx2 = (INT32)( ( COORD_X( m_packet.FlatPolygon.vertex[ n_rightpoint ].n_coord ) << 16 ) - n_cx2.d ) / n_distance;
		}
		n_x = n_cx1.w.h;
		n_distance = (INT16)n_cx2.w.h - n_x;
		if( n_distance > 0 && n_y >= m_n_drawarea_y1 && n_y <= m_n_drawarea_y2 )
		{
			if( (INT32)( m_n_drawarea_x1 - n_x ) > 0 )
			{
				n_distance -= ( m_n_drawarea_x1 - n_x );
				n_x = m_n_drawarea_x1;
			}
			if( n_distance > ( m_n_drawarea_x2 - n_x ) + 1 )
			{
				n_distance = ( m_n_drawarea_x2 - n_x ) + 1;
			}
			p_vram = m_p_p_vram[ n_y ] + n_x;
			while( n_distance > 0 )
			{
				n_bgr = ( m_p_n_redshade[ MID_LEVEL | n_r.w.h ] |
							m_p_n_greenshade[ MID_LEVEL | n_g.w.h ] |
							m_p_n_blueshade[ MID_LEVEL | n_b.w.h ] );
				*( p_vram ) = n_bgr;
				p_vram++;
				n_distance--;
			}
		}
		n_cx1.d += n_dx1;
		n_cx2.d += n_dx2;
		n_y++;
	}
}

static void FlatTexturedPolygon( int n_points )
{
	UINT32 n_tp;
	UINT32 n_tx;
	UINT32 n_ty;
	UINT32 n_abr;

	UINT32 n_clutx;
	UINT32 n_cluty;

	INT16 n_y;
	INT16 n_x;

	UINT8 n_cmd;
	UINT32 n_bgr;

	UINT16 *p_n_f;
	UINT16 *p_n_redb;
	UINT16 *p_n_greenb;
	UINT16 *p_n_blueb;
	UINT16 *p_n_redtrans;
	UINT16 *p_n_greentrans;
	UINT16 *p_n_bluetrans;

	PAIR n_r;
	PAIR n_g;
	PAIR n_b;
	PAIR n_u;
	PAIR n_v;

	PAIR n_cx1;
	PAIR n_cx2;
	PAIR n_cu1;
	PAIR n_cv1;
	PAIR n_cu2;
	PAIR n_cv2;
	INT32 n_du;
	INT32 n_dv;
	INT32 n_dx1;
	INT32 n_dx2;
	INT32 n_du1;
	INT32 n_dv1;
	INT32 n_du2;
	INT32 n_dv2;

	INT32 n_distance;

	UINT16 n_point;
	UINT16 n_rightpoint;
	UINT16 n_leftpoint;
	UINT16 p_n_nextpointlist4[] = { 1, 3, 0, 2 };
	UINT16 p_n_prevpointlist4[] = { 2, 0, 3, 1 };
	UINT16 p_n_nextpointlist3[] = { 1, 2, 0 };
	UINT16 p_n_prevpointlist3[] = { 2, 0, 1 };
	UINT16 *p_n_nextpointlist;
	UINT16 *p_n_prevpointlist;
	UINT16 *p_n_rightpointlist;
	UINT16 *p_n_leftpointlist;
	UINT16 *p_clut;
	UINT16 *p_vram;

	if( n_points == 4 )
	{
		p_n_nextpointlist = p_n_nextpointlist4;
		p_n_prevpointlist = p_n_prevpointlist4;
	}
	else
	{
		p_n_nextpointlist = p_n_nextpointlist3;
		p_n_prevpointlist = p_n_prevpointlist3;
	}

	for( n_point = 0; n_point < n_points; n_point++ )
	{
		SET_COORD_X( m_packet.FlatTexturedPolygon.vertex[ n_point ].n_coord, SINT12( COORD_X( m_packet.FlatTexturedPolygon.vertex[ n_point ].n_coord ) + m_n_drawoffset_x ) );
		SET_COORD_Y( m_packet.FlatTexturedPolygon.vertex[ n_point ].n_coord, SINT12( COORD_Y( m_packet.FlatTexturedPolygon.vertex[ n_point ].n_coord ) + m_n_drawoffset_y ) );
#if defined( MAME_DEBUG )
		DebugMesh( COORD_X( m_packet.FlatTexturedPolygon.vertex[ n_point ].n_coord ), COORD_Y( m_packet.FlatTexturedPolygon.vertex[ n_point ].n_coord ) );
#endif
	}
#if defined( MAME_DEBUG )
	DebugMeshEnd();
#endif

	n_leftpoint = 0;
	for( n_point = 0; n_point < n_points; n_point++ )
	{
		if( (INT32)m_packet.FlatTexturedPolygon.vertex[ n_point ].n_coord.d < (INT32)m_packet.FlatTexturedPolygon.vertex[ n_leftpoint ].n_coord.d )
		{
			n_leftpoint = n_point;
		}
	}
	n_rightpoint = n_leftpoint;

	if( COORD_X( m_packet.FlatTexturedPolygon.vertex[ p_n_nextpointlist[ n_rightpoint ] ].n_coord ) > COORD_X( m_packet.FlatTexturedPolygon.vertex[ p_n_prevpointlist[ n_rightpoint ] ].n_coord ) )
	{
		p_n_rightpointlist = p_n_nextpointlist;
		p_n_leftpointlist = p_n_prevpointlist;
	}
	else
	{
		p_n_rightpointlist = p_n_prevpointlist;
		p_n_leftpointlist = p_n_nextpointlist;
	}

	n_clutx = ( m_packet.FlatTexturedPolygon.vertex[ 0 ].n_texture.w.h & 0x3f ) << 4;
	n_cluty = ( m_packet.FlatTexturedPolygon.vertex[ 0 ].n_texture.w.h >> 6 ) & 0x3ff;
	p_clut = m_p_p_vram[ n_cluty ] + n_clutx;

	n_tp = ( m_packet.FlatTexturedPolygon.vertex[ 1 ].n_texture.w.h & 0x180 ) >> 7;
	n_tx = ( m_packet.FlatTexturedPolygon.vertex[ 1 ].n_texture.w.h & 0x0f ) << 6;
	n_ty = ( m_packet.FlatTexturedPolygon.vertex[ 1 ].n_texture.w.h & 0x10 ) << 4;

	n_abr = ( m_packet.FlatTexturedPolygon.vertex[ 1 ].n_texture.w.h & 0x60 ) >> 5;
	n_cmd = BGR_C( m_packet.FlatTexturedPolygon.n_bgr );

	p_n_f = m_p_n_f1;
	p_n_redb = m_p_n_redb1;
	p_n_greenb = m_p_n_greenb1;
	p_n_blueb = m_p_n_blueb1;
	p_n_redtrans = m_p_n_redaddtrans;
	p_n_greentrans = m_p_n_greenaddtrans;
	p_n_bluetrans = m_p_n_blueaddtrans;

	switch( n_cmd & 0x02 )
	{
	case 0x02:
		switch( n_abr & 0x03 )
		{
		case 0x00:
			p_n_f = m_p_n_f05;
			p_n_redb = m_p_n_redb05;
			p_n_greenb = m_p_n_greenb05;
			p_n_blueb = m_p_n_blueb05;
			p_n_redtrans = m_p_n_redaddtrans;
			p_n_greentrans = m_p_n_greenaddtrans;
			p_n_bluetrans = m_p_n_blueaddtrans;
			verboselog( 2, "Transparency Mode: 0.5*B + 0.5*F\n" );
			break;
		case 0x01:
			p_n_f = m_p_n_f1;
			p_n_redb = m_p_n_redb1;
			p_n_greenb = m_p_n_greenb1;
			p_n_blueb = m_p_n_blueb1;
			p_n_redtrans = m_p_n_redaddtrans;
			p_n_greentrans = m_p_n_greenaddtrans;
			p_n_bluetrans = m_p_n_blueaddtrans;
			verboselog( 2, "Transparency Mode: 1.0*B + 1.0*F\n" );
			break;
		case 0x02:
			p_n_f = m_p_n_f1;
			p_n_redb = m_p_n_redb1;
			p_n_greenb = m_p_n_greenb1;
			p_n_blueb = m_p_n_blueb1;
			p_n_redtrans = m_p_n_redsubtrans;
			p_n_greentrans = m_p_n_greensubtrans;
			p_n_bluetrans = m_p_n_bluesubtrans;
			verboselog( 2, "Transparency Mode: 1.0*B - 1.0*F\n" );
			break;
		case 0x03:
			p_n_f = m_p_n_f025;
			p_n_redb = m_p_n_redb1;
			p_n_greenb = m_p_n_greenb1;
			p_n_blueb = m_p_n_blueb1;
			p_n_redtrans = m_p_n_redaddtrans;
			p_n_greentrans = m_p_n_greenaddtrans;
			p_n_bluetrans = m_p_n_blueaddtrans;
			verboselog( 2, "Transparency Mode: 1.0*B + 0.25*F\n" );
			break;
		}
		break;
	}

	n_r.w.h = BGR_R( m_packet.FlatTexturedPolygon.n_bgr ); n_r.w.l = 0;
	n_g.w.h = BGR_G( m_packet.FlatTexturedPolygon.n_bgr ); n_g.w.l = 0;
	n_b.w.h = BGR_B( m_packet.FlatTexturedPolygon.n_bgr ); n_b.w.l = 0;

	n_y = COORD_Y( m_packet.FlatTexturedPolygon.vertex[ n_rightpoint ].n_coord );

	n_dx1 = 0;
	n_dx2 = 0;
	n_du1 = 0;
	n_du2 = 0;
	n_dv1 = 0;
	n_dv2 = 0;

	for( ;; )
	{
		if( n_y == COORD_Y( m_packet.FlatTexturedPolygon.vertex[ n_leftpoint ].n_coord ) )
		{
			while( n_y == COORD_Y( m_packet.FlatTexturedPolygon.vertex[ p_n_leftpointlist[ n_leftpoint ] ].n_coord ) )
			{
				n_leftpoint = p_n_leftpointlist[ n_leftpoint ];
				if( n_leftpoint == n_rightpoint )
				{
					break;
				}
			}
			n_cx1.w.h = COORD_X( m_packet.FlatTexturedPolygon.vertex[ n_leftpoint ].n_coord ); n_cx1.w.l = 0;
			n_cu1.w.h = TEXTURE_U( m_packet.FlatTexturedPolygon.vertex[ n_leftpoint ].n_texture ); n_cu1.w.l = 0;
			n_cv1.w.h = TEXTURE_V( m_packet.FlatTexturedPolygon.vertex[ n_leftpoint ].n_texture ); n_cv1.w.l = 0;
			n_leftpoint = p_n_leftpointlist[ n_leftpoint ];
			n_distance = COORD_Y( m_packet.FlatTexturedPolygon.vertex[ n_leftpoint ].n_coord ) - n_y;
			if( n_distance < 1 )
			{
				break;
			}
			n_dx1 = (INT32)( ( COORD_X( m_packet.FlatTexturedPolygon.vertex[ n_leftpoint ].n_coord ) << 16 ) - n_cx1.d ) / n_distance;
			n_du1 = (INT32)( ( TEXTURE_U( m_packet.FlatTexturedPolygon.vertex[ n_leftpoint ].n_texture ) << 16 ) - n_cu1.d ) / n_distance;
			n_dv1 = (INT32)( ( TEXTURE_V( m_packet.FlatTexturedPolygon.vertex[ n_leftpoint ].n_texture ) << 16 ) - n_cv1.d ) / n_distance;
		}
		if( n_y == COORD_Y( m_packet.FlatTexturedPolygon.vertex[ n_rightpoint ].n_coord ) )
		{
			while( n_y == COORD_Y( m_packet.FlatTexturedPolygon.vertex[ p_n_rightpointlist[ n_rightpoint ] ].n_coord ) )
			{
				n_rightpoint = p_n_rightpointlist[ n_rightpoint ];
				if( n_rightpoint == n_leftpoint )
				{
					break;
				}
			}
			n_cx2.w.h = COORD_X( m_packet.FlatTexturedPolygon.vertex[ n_rightpoint ].n_coord ); n_cx2.w.l = 0;
			n_cu2.w.h = TEXTURE_U( m_packet.FlatTexturedPolygon.vertex[ n_rightpoint ].n_texture ); n_cu2.w.l = 0;
			n_cv2.w.h = TEXTURE_V( m_packet.FlatTexturedPolygon.vertex[ n_rightpoint ].n_texture ); n_cv2.w.l = 0;
			n_rightpoint = p_n_rightpointlist[ n_rightpoint ];
			n_distance = COORD_Y( m_packet.FlatTexturedPolygon.vertex[ n_rightpoint ].n_coord ) - n_y;
			if( n_distance < 1 )
			{
				break;
			}
			n_dx2 = (INT32)( ( COORD_X( m_packet.FlatTexturedPolygon.vertex[ n_rightpoint ].n_coord ) << 16 ) - n_cx2.d ) / n_distance;
			n_du2 = (INT32)( ( TEXTURE_U( m_packet.FlatTexturedPolygon.vertex[ n_rightpoint ].n_texture ) << 16 ) - n_cu2.d ) / n_distance;
			n_dv2 = (INT32)( ( TEXTURE_V( m_packet.FlatTexturedPolygon.vertex[ n_rightpoint ].n_texture ) << 16 ) - n_cv2.d ) / n_distance;
		}
		n_x = n_cx1.w.h;
		n_distance = (INT16)n_cx2.w.h - n_x;
		if( n_distance > 0 && n_y >= m_n_drawarea_y1 && n_y <= m_n_drawarea_y2 )
		{
			n_u.d = n_cu1.d;
			n_v.d = n_cv1.d;
			n_du = (INT32)( n_cu2.d - n_cu1.d ) / n_distance;
			n_dv = (INT32)( n_cv2.d - n_cv1.d ) / n_distance;

			if( (INT32)( m_n_drawarea_x1 - n_x ) > 0 )
			{
				n_u.d += n_du * ( m_n_drawarea_x1 - n_x );
				n_v.d += n_dv * ( m_n_drawarea_x1 - n_x );
				n_distance -= ( m_n_drawarea_x1 - n_x );
				n_x = m_n_drawarea_x1;
			}
			if( n_distance > (INT32)( m_n_drawarea_x2 - n_x ) )
			{
				n_distance = m_n_drawarea_x2 - n_x;
			}
			p_vram = m_p_p_vram[ n_y ] + n_x;
			switch( n_cmd & 0x02 )
			{
			case 0x00:
				/* shading */
				switch( n_tp )
				{
				case 0:
					/* 4 bit clut */
					while( n_distance > 0 )
					{
						n_bgr = p_clut[ ( *( m_p_p_vram[ n_ty + n_v.w.h ] + n_tx + ( n_u.w.h >> 2 ) ) >> ( ( n_u.w.h & 0x03 ) << 2 ) ) & 0x0f ];
						if( n_bgr != 0 )
						{
							*( p_vram ) = ( m_p_n_redshade[ m_p_n_redlevel[ n_bgr ] | n_r.w.h ] |
										m_p_n_greenshade[ m_p_n_greenlevel[ n_bgr ] | n_g.w.h ] |
										m_p_n_blueshade[ m_p_n_bluelevel[ n_bgr ] | n_b.w.h ] );
						}
						p_vram++;
						n_u.d += n_du;
						n_v.d += n_dv;
						n_distance--;
					}
					break;
				case 1:
					/* 8 bit clut */
					while( n_distance > 0 )
					{
						n_bgr = p_clut[ ( *( m_p_p_vram[ n_ty + n_v.w.h ] + n_tx + ( n_u.w.h >> 1 ) ) >> ( ( n_u.w.h & 0x01 ) << 3 ) ) & 0xff ];
						if( n_bgr != 0 )
						{
							*( p_vram ) = ( m_p_n_redshade[ m_p_n_redlevel[ n_bgr ] | n_r.w.h ] |
										m_p_n_greenshade[ m_p_n_greenlevel[ n_bgr ] | n_g.w.h ] |
										m_p_n_blueshade[ m_p_n_bluelevel[ n_bgr ] | n_b.w.h ] );
						}
						p_vram++;
						n_u.d += n_du;
						n_v.d += n_dv;
						n_distance--;
					}
					break;
				case 2:
					/* 15 bit */
					while( n_distance > 0 )
					{
						n_bgr = *( m_p_p_vram[ n_ty + n_v.w.h ] + n_tx + n_u.w.h );
						if( n_bgr != 0 )
						{
							*( p_vram ) = ( m_p_n_redshade[ m_p_n_redlevel[ n_bgr ] | n_r.w.h ] |
										m_p_n_greenshade[ m_p_n_greenlevel[ n_bgr ] | n_g.w.h ] |
										m_p_n_blueshade[ m_p_n_bluelevel[ n_bgr ] | n_b.w.h ] );
						}
						p_vram++;
						n_u.d += n_du;
						n_v.d += n_dv;
						n_distance--;
					}
					break;
				}
				break;
			case 0x02:
				/* semi transparency */
				switch( n_tp )
				{
				case 0:
					/* 4 bit clut */
					while( n_distance > 0 )
					{
						n_bgr = p_clut[ ( *( m_p_p_vram[ n_ty + n_v.w.h ] + n_tx + ( n_u.w.h >> 2 ) ) >> ( ( n_u.w.h & 0x03 ) << 2 ) ) & 0x0f ];
						if( n_bgr != 0 )
						{
							if( ( n_bgr & 0x8000 ) != 0 )
							{
								*( p_vram ) =
									p_n_redtrans[ p_n_f[ m_p_n_redlevel[ n_bgr ] | n_r.w.h ] | p_n_redb[ *( p_vram ) ] ] |
									p_n_greentrans[ p_n_f[ m_p_n_greenlevel[ n_bgr ] | n_g.w.h ] | p_n_greenb[ *( p_vram ) ] ] |
									p_n_bluetrans[ p_n_f[ m_p_n_bluelevel[ n_bgr ] | n_b.w.h ] | p_n_blueb[ *( p_vram ) ] ];
							}
							else
							{
								*( p_vram ) = ( m_p_n_redshade[ m_p_n_redlevel[ n_bgr ] | n_r.w.h ] |
											m_p_n_greenshade[ m_p_n_greenlevel[ n_bgr ] | n_g.w.h ] |
											m_p_n_blueshade[ m_p_n_bluelevel[ n_bgr ] | n_b.w.h ] );
							}
						}
						p_vram++;
						n_u.d += n_du;
						n_v.d += n_dv;
						n_distance--;
					}
					break;
				case 1:
					/* 8 bit clut */
					while( n_distance > 0 )
					{
						n_bgr = p_clut[ ( *( m_p_p_vram[ n_ty + n_v.w.h ] + n_tx + ( n_u.w.h >> 1 ) ) >> ( ( n_u.w.h & 0x01 ) << 3 ) ) & 0xff ];
						if( n_bgr != 0 )
						{
							if( ( n_bgr & 0x8000 ) != 0 )
							{
								*( p_vram ) =
									p_n_redtrans[ p_n_f[ m_p_n_redlevel[ n_bgr ] | n_r.w.h ] | p_n_redb[ *( p_vram ) ] ] |
									p_n_greentrans[ p_n_f[ m_p_n_greenlevel[ n_bgr ] | n_g.w.h ] | p_n_greenb[ *( p_vram ) ] ] |
									p_n_bluetrans[ p_n_f[ m_p_n_bluelevel[ n_bgr ] | n_b.w.h ] | p_n_blueb[ *( p_vram ) ] ];
							}
							else
							{
								*( p_vram ) = ( m_p_n_redshade[ m_p_n_redlevel[ n_bgr ] | n_r.w.h ] |
											m_p_n_greenshade[ m_p_n_greenlevel[ n_bgr ] | n_g.w.h ] |
											m_p_n_blueshade[ m_p_n_bluelevel[ n_bgr ] | n_b.w.h ] );
							}
						}
						p_vram++;
						n_u.d += n_du;
						n_v.d += n_dv;
						n_distance--;
					}
					break;
				case 2:
					/* 15 bit */
					while( n_distance > 0 )
					{
						n_bgr = *( m_p_p_vram[ n_ty + n_v.w.h ] + n_tx + n_u.w.h );
						if( n_bgr != 0 )
						{
							if( ( n_bgr & 0x8000 ) != 0 )
							{
								*( p_vram ) =
									p_n_redtrans[ p_n_f[ m_p_n_redlevel[ n_bgr ] | n_r.w.h ] | p_n_redb[ *( p_vram ) ] ] |
									p_n_greentrans[ p_n_f[ m_p_n_greenlevel[ n_bgr ] | n_g.w.h ] | p_n_greenb[ *( p_vram ) ] ] |
									p_n_bluetrans[ p_n_f[ m_p_n_bluelevel[ n_bgr ] | n_b.w.h ] | p_n_blueb[ *( p_vram ) ] ];
							}
							else
							{
								*( p_vram ) = ( m_p_n_redshade[ m_p_n_redlevel[ n_bgr ] | n_r.w.h ] |
											m_p_n_greenshade[ m_p_n_greenlevel[ n_bgr ] | n_g.w.h ] |
											m_p_n_blueshade[ m_p_n_bluelevel[ n_bgr ] | n_b.w.h ] );
							}
						}
						p_vram++;
						n_u.d += n_du;
						n_v.d += n_dv;
						n_distance--;
					}
					break;
				}
				break;
			}
		}
		n_cx1.d += n_dx1;
		n_cu1.d += n_du1;
		n_cv1.d += n_dv1;
		n_cx2.d += n_dx2;
		n_cu2.d += n_du2;
		n_cv2.d += n_dv2;
		n_y++;
	}
}

static void GouraudPolygon( int n_points )
{
	/* Draws a shaded polygon. No transparency / dithering & probably not pixel perfect. */
	INT16 n_y;
	INT16 n_x;

	PAIR n_r;
	PAIR n_g;
	PAIR n_b;
	PAIR n_cx1;
	PAIR n_cx2;
	PAIR n_cr1;
	PAIR n_cg1;
	PAIR n_cb1;
	PAIR n_cr2;
	PAIR n_cg2;
	PAIR n_cb2;
	INT32 n_dr;
	INT32 n_dg;
	INT32 n_db;
	INT32 n_dx1;
	INT32 n_dx2;
	INT32 n_dr1;
	INT32 n_dg1;
	INT32 n_db1;
	INT32 n_dr2;
	INT32 n_dg2;
	INT32 n_db2;
	UINT32 n_bgr;

	INT32 n_distance;

	UINT16 n_point;
	UINT16 n_rightpoint;
	UINT16 n_leftpoint;
	UINT16 p_n_nextpointlist4[] = { 1, 3, 0, 2 };
	UINT16 p_n_prevpointlist4[] = { 2, 0, 3, 1 };
	UINT16 p_n_nextpointlist3[] = { 1, 2, 0 };
	UINT16 p_n_prevpointlist3[] = { 2, 0, 1 };
	UINT16 *p_n_nextpointlist;
	UINT16 *p_n_prevpointlist;
	UINT16 *p_n_rightpointlist;
	UINT16 *p_n_leftpointlist;

	UINT16 *p_vram;

	if( n_points == 4 )
	{
		p_n_nextpointlist = p_n_nextpointlist4;
		p_n_prevpointlist = p_n_prevpointlist4;
	}
	else
	{
		p_n_nextpointlist = p_n_nextpointlist3;
		p_n_prevpointlist = p_n_prevpointlist3;
	}

	for( n_point = 0; n_point < n_points; n_point++ )
	{
		SET_COORD_X( m_packet.GouraudPolygon.vertex[ n_point ].n_coord, SINT12( COORD_X( m_packet.GouraudPolygon.vertex[ n_point ].n_coord ) + m_n_drawoffset_x ) );
		SET_COORD_Y( m_packet.GouraudPolygon.vertex[ n_point ].n_coord, SINT12( COORD_Y( m_packet.GouraudPolygon.vertex[ n_point ].n_coord ) + m_n_drawoffset_y ) );
#if defined( MAME_DEBUG )
		DebugMesh( COORD_X( m_packet.GouraudPolygon.vertex[ n_point ].n_coord ), COORD_Y( m_packet.GouraudPolygon.vertex[ n_point ].n_coord ) );
#endif
	}
#if defined( MAME_DEBUG )
	DebugMeshEnd();
#endif

	n_leftpoint = 0;
	for( n_point = 0; n_point < n_points; n_point++ )
	{
		if( (INT32)m_packet.GouraudPolygon.vertex[ n_point ].n_coord.d < (INT32)m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_coord.d )
		{
			n_leftpoint = n_point;
		}
	}
	n_rightpoint = n_leftpoint;

	if( COORD_X( m_packet.GouraudPolygon.vertex[ p_n_nextpointlist[ n_rightpoint ] ].n_coord ) > COORD_X( m_packet.GouraudPolygon.vertex[ p_n_prevpointlist[ n_rightpoint ] ].n_coord ) )
	{
		p_n_rightpointlist = p_n_nextpointlist;
		p_n_leftpointlist = p_n_prevpointlist;
	}
	else
	{
		p_n_rightpointlist = p_n_prevpointlist;
		p_n_leftpointlist = p_n_nextpointlist;
	}

	n_dx1 = 0;
	n_dx2 = 0;
	n_dr1 = 0;
	n_dr2 = 0;
	n_dg1 = 0;
	n_dg2 = 0;
	n_db1 = 0;
	n_db2 = 0;

	n_y = COORD_Y( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_coord );

	for( ;; )
	{
		if( n_y == COORD_Y( m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_coord ) )
		{
			while( n_y == COORD_Y( m_packet.GouraudPolygon.vertex[ p_n_leftpointlist[ n_leftpoint ] ].n_coord ) )
			{
				n_leftpoint = p_n_leftpointlist[ n_leftpoint ];
				if( n_leftpoint == n_rightpoint )
				{
					break;
				}
			}
			n_cx1.w.h = COORD_X( m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_coord ); n_cx1.w.l = 0;
			n_cr1.w.h = BGR_R( m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_bgr ); n_cr1.w.l = 0;
			n_cg1.w.h = BGR_G( m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_bgr ); n_cg1.w.l = 0;
			n_cb1.w.h = BGR_B( m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_bgr ); n_cb1.w.l = 0;
			n_leftpoint = p_n_leftpointlist[ n_leftpoint ];
			n_distance = COORD_Y( m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_coord ) - n_y;
			if( n_distance < 1 )
			{
				break;
			}
			n_dx1 = (INT32)( ( COORD_X( m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_coord ) << 16 ) - n_cx1.d ) / n_distance;
			n_dr1 = (INT32)( ( BGR_R( m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_bgr ) << 16 ) - n_cr1.d ) / n_distance;
			n_dg1 = (INT32)( ( BGR_G( m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_bgr ) << 16 ) - n_cg1.d ) / n_distance;
			n_db1 = (INT32)( ( BGR_B( m_packet.GouraudPolygon.vertex[ n_leftpoint ].n_bgr ) << 16 ) - n_cb1.d ) / n_distance;
		}
		if( n_y == COORD_Y( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_coord ) )
		{
			while( n_y == COORD_Y( m_packet.GouraudPolygon.vertex[ p_n_rightpointlist[ n_rightpoint ] ].n_coord ) )
			{
				n_rightpoint = p_n_rightpointlist[ n_rightpoint ];
				if( n_rightpoint == n_leftpoint )
				{
					break;
				}
			}
			n_cx2.w.h = COORD_X( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_coord ); n_cx2.w.l = 0;
			n_cr2.w.h = BGR_R( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_bgr ); n_cr2.w.l = 0;
			n_cg2.w.h = BGR_G( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_bgr ); n_cg2.w.l = 0;
			n_cb2.w.h = BGR_B( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_bgr ); n_cb2.w.l = 0;
			n_rightpoint = p_n_rightpointlist[ n_rightpoint ];
			n_distance = COORD_Y( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_coord ) - n_y;
			if( n_distance < 1 )
			{
				break;
			}
			n_dx2 = (INT32)( ( COORD_X( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_coord ) << 16 ) - n_cx2.d ) / n_distance;
			n_dr2 = (INT32)( ( BGR_R( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_bgr ) << 16 ) - n_cr2.d ) / n_distance;
			n_dg2 = (INT32)( ( BGR_G( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_bgr ) << 16 ) - n_cg2.d ) / n_distance;
			n_db2 = (INT32)( ( BGR_B( m_packet.GouraudPolygon.vertex[ n_rightpoint ].n_bgr ) << 16 ) - n_cb2.d ) / n_distance;
		}
		n_x = n_cx1.w.h;
		n_distance = (INT16)n_cx2.w.h - n_x;
		if( n_distance > 0 && n_y >= m_n_drawarea_y1 && n_y <= m_n_drawarea_y2 )
		{
			n_r.d = n_cr1.d;
			n_g.d = n_cg1.d;
			n_b.d = n_cb1.d;
			n_dr = (INT32)( n_cr2.d - n_cr1.d ) / n_distance;
			n_dg = (INT32)( n_cg2.d - n_cg1.d ) / n_distance;
			n_db = (INT32)( n_cb2.d - n_cb1.d ) / n_distance;

			if( (INT32)( m_n_drawarea_x1 - n_x ) > 0 )
			{
				n_r.d += n_dr * ( m_n_drawarea_x1 - n_x );
				n_g.d += n_dg * ( m_n_drawarea_x1 - n_x );
				n_b.d += n_db * ( m_n_drawarea_x1 - n_x );
				n_distance -= ( m_n_drawarea_x1 - n_x );
				n_x = m_n_drawarea_x1;
			}
			if( n_distance > ( m_n_drawarea_x2 - n_x ) + 1 )
			{
				n_distance = ( m_n_drawarea_x2 - n_x ) + 1;
			}
			p_vram = m_p_p_vram[ n_y ] + n_x;
			while( n_distance > 0 )
			{
				n_bgr = ( m_p_n_redshade[ MID_LEVEL | n_r.w.h ] |
							m_p_n_greenshade[ MID_LEVEL | n_g.w.h ] |
							m_p_n_blueshade[ MID_LEVEL | n_b.w.h ] );
				*( p_vram ) = n_bgr;
				p_vram++;
				n_r.d += n_dr;
				n_g.d += n_dg;
				n_b.d += n_db;
				n_distance--;
			}
		}
		n_cx1.d += n_dx1;
		n_cr1.d += n_dr1;
		n_cg1.d += n_dg1;
		n_cb1.d += n_db1;
		n_cx2.d += n_dx2;
		n_cr2.d += n_dr2;
		n_cg2.d += n_dg2;
		n_cb2.d += n_db2;
		n_y++;
	}
}

static void MonochromeLine( void )
{
	int n_r;
	int n_g;
	int n_b;
	PAIR n_x;
	PAIR n_y;
	INT32 n_dx;
	INT32 n_dy;
	UINT32 n_bgr;
	int n_len;
	int n_xlen;
	int n_ylen;
	int n_xstart;
	int n_ystart;
	int n_xend;
	int n_yend;
	UINT16 *p_vram;

#if defined( MAME_DEBUG )
	DebugMesh( SINT12( COORD_X( m_packet.MonochromeLine.vertex[ 0 ].n_coord ) + m_n_drawoffset_x ), SINT12( COORD_Y( m_packet.MonochromeLine.vertex[ 0 ].n_coord ) + m_n_drawoffset_x ) );
	DebugMesh( SINT12( COORD_X( m_packet.MonochromeLine.vertex[ 1 ].n_coord ) + m_n_drawoffset_x ), SINT12( COORD_Y( m_packet.MonochromeLine.vertex[ 1 ].n_coord ) + m_n_drawoffset_x ) );
	DebugMeshEnd();
#endif

	n_r = BGR_R( m_packet.MonochromeLine.n_bgr );
	n_g = BGR_G( m_packet.MonochromeLine.n_bgr );
	n_b = BGR_B( m_packet.MonochromeLine.n_bgr );

	n_xstart = SINT12( COORD_X( m_packet.MonochromeLine.vertex[ 0 ].n_coord ) + m_n_drawoffset_x );
	n_xend = SINT12( COORD_X( m_packet.MonochromeLine.vertex[ 1 ].n_coord ) + m_n_drawoffset_x );
	if( n_xend > n_xstart )
	{
		n_xlen = n_xend - n_xstart;
	}
	else
	{
		n_xlen = n_xstart - n_xend;
	}

	n_ystart = SINT12( COORD_Y( m_packet.MonochromeLine.vertex[ 0 ].n_coord ) + m_n_drawoffset_y );
	n_yend = SINT12( COORD_Y( m_packet.MonochromeLine.vertex[ 1 ].n_coord ) + m_n_drawoffset_y );
	if( n_yend > n_ystart )
	{
		n_ylen = n_yend - n_ystart;
	}
	else
	{
		n_ylen = n_ystart - n_yend;
	}

	if( n_xlen > n_ylen )
	{
		n_len = n_xlen;
	}
	else
	{
		n_len = n_ylen;
	}

	n_x.w.h = n_xstart; n_x.w.l = 0;
	n_y.w.h = n_ystart; n_y.w.l = 0;

	if( n_len == 0 )
	{
		n_len = 1;
	}

	n_dx = (INT32)( ( n_xend << 16 ) - n_x.d ) / n_len;
	n_dy = (INT32)( ( n_yend << 16 ) - n_y.d ) / n_len;

	while( n_len > 0 )
	{
		/* moving this outside the loop would be good */
		if( n_x.w.h >= m_n_drawarea_x1 &&
			n_y.w.h >= m_n_drawarea_y1 &&
			n_x.w.h <= m_n_drawarea_x2 &&
			n_y.w.h <= m_n_drawarea_y2 )
		{
			p_vram = m_p_p_vram[ n_y.w.h ] + n_x.w.h;
			n_bgr = ( m_p_n_redshade[ MID_LEVEL | n_r ] |
						m_p_n_greenshade[ MID_LEVEL | n_g ] |
						m_p_n_blueshade[ MID_LEVEL | n_b ] );
			*( p_vram ) = n_bgr;
		}
		n_x.d += n_dx;
		n_y.d += n_dy;
		n_len--;
	}
}

static void FrameBufferRectangleDraw( void )
{
	PAIR n_r;
	PAIR n_g;
	PAIR n_b;
	UINT32 n_bgr;
	INT32 n_distance;
	INT32 n_w;
	INT32 n_h;
	INT16 n_y;
	INT16 n_x;
	UINT16 *p_vram;

#if defined( MAME_DEBUG )
	DebugMesh( SINT12( COORD_X( m_packet.FlatRectangle.n_coord ) ), SINT12( COORD_Y( m_packet.FlatRectangle.n_coord ) ) );
	DebugMesh( SINT12( COORD_X( m_packet.FlatRectangle.n_coord ) + SIZE_W( m_packet.FlatRectangle.n_size ) ), SINT12( COORD_Y( m_packet.FlatRectangle.n_coord ) ) );
	DebugMesh( SINT12( COORD_X( m_packet.FlatRectangle.n_coord ) ), SINT12( COORD_Y( m_packet.FlatRectangle.n_coord ) + SIZE_H( m_packet.FlatRectangle.n_size ) ) );
	DebugMesh( SINT12( COORD_X( m_packet.FlatRectangle.n_coord ) + SIZE_W( m_packet.FlatRectangle.n_size ) ), SINT12( COORD_Y( m_packet.FlatRectangle.n_coord ) + SIZE_H( m_packet.FlatRectangle.n_size ) ) );
	DebugMeshEnd();
#endif

	n_r.w.h = BGR_R( m_packet.FlatRectangle.n_bgr ); n_r.w.l = 0;
	n_g.w.h = BGR_G( m_packet.FlatRectangle.n_bgr ); n_g.w.l = 0;
	n_b.w.h = BGR_B( m_packet.FlatRectangle.n_bgr ); n_b.w.l = 0;

	n_x = SINT12( COORD_X( m_packet.FlatRectangle.n_coord ) );
	n_y = SINT12( COORD_Y( m_packet.FlatRectangle.n_coord ) );
	n_w = SIZE_W( m_packet.FlatRectangle.n_size );
	n_h = SIZE_H( m_packet.FlatRectangle.n_size );

	if( (INT32)( 0 - n_x ) > 0 )
	{
		n_w -= ( 0 - n_x );
		n_x = 0;
	}
	if( n_w > ( 1023 - n_x ) + 1 )
	{
		n_w = ( 1023 - n_x ) + 1;
	}
	if( (INT32)( 0 - n_y ) > 0 )
	{
		n_h -= ( 0 - n_y );
		n_y = 0;
	}
	if( n_h > ( 1023 - n_y ) + 1 )
	{
		n_h = ( 1023 - n_y ) + 1;
	}

	if( n_w > 0 )
	{
		while( n_h > 0 )
		{
			p_vram = m_p_p_vram[ n_y ] + n_x;
			n_distance = n_w;
			while( n_distance > 0 )
			{
				n_bgr = ( m_p_n_redshade[ MID_LEVEL | n_r.w.h ] |
							m_p_n_greenshade[ MID_LEVEL | n_g.w.h ] |
							m_p_n_blueshade[ MID_LEVEL | n_b.w.h ] );
				*( p_vram ) = n_bgr;
				p_vram++;
				n_distance--;
			}
			n_y++;
			n_h--;
		}
	}
}

static void FlatRectangle( void )
{
	PAIR n_r;
	PAIR n_g;
	PAIR n_b;
	UINT32 n_bgr;
	INT32 n_distance;
	INT32 n_w;
	INT32 n_h;
	INT16 n_y;
	INT16 n_x;
	UINT16 *p_vram;

#if defined( MAME_DEBUG )
	DebugMesh( SINT12( COORD_X( m_packet.FlatRectangle.n_coord ) + m_n_drawoffset_x ), SINT12( COORD_Y( m_packet.FlatRectangle.n_coord ) + m_n_drawoffset_x ) );
	DebugMesh( SINT12( COORD_X( m_packet.FlatRectangle.n_coord ) + m_n_drawoffset_x + SIZE_W( m_packet.FlatRectangle.n_size ) ), SINT12( COORD_Y( m_packet.FlatRectangle.n_coord ) + m_n_drawoffset_x ) );
	DebugMesh( SINT12( COORD_X( m_packet.FlatRectangle.n_coord ) + m_n_drawoffset_x ), SINT12( COORD_Y( m_packet.FlatRectangle.n_coord ) + m_n_drawoffset_x + SIZE_H( m_packet.FlatRectangle.n_size ) ) );
	DebugMesh( SINT12( COORD_X( m_packet.FlatRectangle.n_coord ) + m_n_drawoffset_x + SIZE_W( m_packet.FlatRectangle.n_size ) ), SINT12( COORD_Y( m_packet.FlatRectangle.n_coord ) + m_n_drawoffset_x  + SIZE_H( m_packet.FlatRectangle.n_size ) ) );
	DebugMeshEnd();
#endif

	n_r.w.h = BGR_R( m_packet.FlatRectangle.n_bgr ); n_r.w.l = 0;
	n_g.w.h = BGR_G( m_packet.FlatRectangle.n_bgr ); n_g.w.l = 0;
	n_b.w.h = BGR_B( m_packet.FlatRectangle.n_bgr ); n_b.w.l = 0;

	n_x = SINT12( COORD_X( m_packet.FlatRectangle.n_coord ) + m_n_drawoffset_x );
	n_y = SINT12( COORD_Y( m_packet.FlatRectangle.n_coord ) + m_n_drawoffset_y );
	n_w = SIZE_W( m_packet.FlatRectangle.n_size );
	n_h = SIZE_H( m_packet.FlatRectangle.n_size );

	if( (INT32)( m_n_drawarea_x1 - n_x ) > 0 )
	{
		n_w -= ( m_n_drawarea_x1 - n_x );
		n_x = m_n_drawarea_x1;
	}
	if( n_w > ( m_n_drawarea_x2 - n_x ) + 1 )
	{
		n_w = ( m_n_drawarea_x2 - n_x ) + 1;
	}
	if( (INT32)( m_n_drawarea_y1 - n_y ) > 0 )
	{
		n_h -= ( m_n_drawarea_y1 - n_y );
		n_y = m_n_drawarea_y1;
	}
	if( n_h > ( m_n_drawarea_y2 - n_y ) + 1 )
	{
		n_h = ( m_n_drawarea_y2 - n_y ) + 1;
	}

	if( n_w > 0 )
	{
		while( n_h > 0 )
		{
			p_vram = m_p_p_vram[ n_y ] + n_x;
			n_distance = n_w;
			while( n_distance > 0 )
			{
				n_bgr = ( m_p_n_redshade[ MID_LEVEL | n_r.w.h ] |
							m_p_n_greenshade[ MID_LEVEL | n_g.w.h ] |
							m_p_n_blueshade[ MID_LEVEL | n_b.w.h ] );
				*( p_vram ) = n_bgr;
				p_vram++;
				n_distance--;
			}
			n_y++;
			n_h--;
		}
	}
}

static void FlatTexturedRectangle( void )
{
	PAIR n_r;
	PAIR n_g;
	PAIR n_b;
	UINT32 n_tp;
	INT16 n_tx;
	INT16 n_ty;

	UINT32 n_clutx;
	UINT32 n_cluty;

	INT16 n_u;
	INT16 n_v;
	INT16 n_distance;
	UINT32 n_h;
	INT16 n_y;
	INT16 n_x;
	UINT16 *p_vram;
	UINT16 *p_clut;
	UINT16 n_bgr;

#if defined( MAME_DEBUG )
	DebugMesh( SINT12( COORD_X( m_packet.FlatTexturedRectangle.n_coord ) + m_n_drawoffset_x ), SINT12( COORD_Y( m_packet.FlatTexturedRectangle.n_coord ) + m_n_drawoffset_x ) );
	DebugMesh( SINT12( COORD_X( m_packet.FlatTexturedRectangle.n_coord ) + m_n_drawoffset_x + SIZE_W( m_packet.FlatTexturedRectangle.n_size ) ), SINT12( COORD_Y( m_packet.FlatTexturedRectangle.n_coord ) + m_n_drawoffset_x ) );
	DebugMesh( SINT12( COORD_X( m_packet.FlatTexturedRectangle.n_coord ) + m_n_drawoffset_x ), SINT12( COORD_Y( m_packet.FlatTexturedRectangle.n_coord ) + m_n_drawoffset_x + SIZE_H( m_packet.FlatTexturedRectangle.n_size ) ) );
	DebugMesh( SINT12( COORD_X( m_packet.FlatTexturedRectangle.n_coord ) + m_n_drawoffset_x + SIZE_W( m_packet.FlatTexturedRectangle.n_size ) ), SINT12( COORD_Y( m_packet.FlatTexturedRectangle.n_coord ) + m_n_drawoffset_x  + SIZE_H( m_packet.FlatTexturedRectangle.n_size ) ) );
	DebugMeshEnd();
#endif

	n_r.w.h = BGR_R( m_packet.FlatTexturedRectangle.n_bgr ); n_r.w.l = 0;
	n_g.w.h = BGR_G( m_packet.FlatTexturedRectangle.n_bgr ); n_g.w.l = 0;
	n_b.w.h = BGR_B( m_packet.FlatTexturedRectangle.n_bgr ); n_b.w.l = 0;

	n_clutx = ( m_packet.FlatTexturedRectangle.n_texture.w.h & 0x3f ) << 4;
	n_cluty = ( m_packet.FlatTexturedRectangle.n_texture.w.h >> 6 ) & 0x3ff;
	p_clut = m_p_p_vram[ n_cluty ] + n_clutx;

	n_tp = ( m_n_gpustatus & 0x180 ) >> 7;
	n_tx = ( m_n_gpustatus & 0x0f ) << 6;
	n_ty = ( m_n_gpustatus & 0x10 ) << 4;

	n_v = TEXTURE_V( m_packet.FlatTexturedRectangle.n_texture );
	n_y = SINT12( COORD_Y( m_packet.FlatTexturedRectangle.n_coord ) + m_n_drawoffset_y );

	n_h = SIZE_H( m_packet.FlatTexturedRectangle.n_size );
	while( n_h != 0 )
	{
		n_x = SINT12( COORD_X( m_packet.FlatTexturedRectangle.n_coord ) + m_n_drawoffset_x );
		n_distance = SIZE_W( m_packet.FlatTexturedRectangle.n_size );
		if( n_distance > 0 && n_y >= m_n_drawarea_y1 && n_y <= m_n_drawarea_y2 )
		{
			n_u = TEXTURE_U( m_packet.FlatTexturedRectangle.n_texture );

			if( (INT32)( m_n_drawarea_x1 - n_x ) > 0 )
			{
				n_u += ( m_n_drawarea_x1 - n_x );
				n_distance -= ( m_n_drawarea_x1 - n_x );
				n_x = m_n_drawarea_x1;
			}
			if( n_distance > ( m_n_drawarea_x2 - n_x ) + 1 )
			{
				n_distance = ( m_n_drawarea_x2 - n_x ) + 1;
			}
			p_vram = m_p_p_vram[ n_y ] + n_x;
			switch( n_tp )
			{
			case 0:
				/* 4 bit clut */
				while( n_distance > 0 )
				{
					n_bgr = p_clut[ ( *( m_p_p_vram[ n_ty + n_v ] + n_tx + ( n_u >> 2 ) ) >> ( ( n_u & 0x03 ) << 2 ) ) & 0x0f ];
					n_bgr = ( m_p_n_redshade[ m_p_n_redlevel[ n_bgr ] | n_r.w.h ] |
								m_p_n_greenshade[ m_p_n_greenlevel[ n_bgr ] | n_g.w.h ] |
								m_p_n_blueshade[ m_p_n_bluelevel[ n_bgr ] | n_b.w.h ] );
					if( n_bgr != 0 )
					{
						*( p_vram ) = n_bgr;
					}
					p_vram++;
					n_u++;
					n_distance--;
				}
				break;
			case 1:
				/* 8 bit clut */
				while( n_distance > 0 )
				{
					n_bgr = p_clut[ ( *( m_p_p_vram[ n_ty + n_v ] + n_tx + ( n_u >> 1 ) ) >> ( ( n_u & 0x01 ) << 3 ) ) & 0xff ];
					n_bgr = ( m_p_n_redshade[ m_p_n_redlevel[ n_bgr ] | n_r.w.h ] |
								m_p_n_greenshade[ m_p_n_greenlevel[ n_bgr ] | n_g.w.h ] |
								m_p_n_blueshade[ m_p_n_bluelevel[ n_bgr ] | n_b.w.h ] );
					if( n_bgr != 0 )
					{
						*( p_vram ) = n_bgr;
					}
					p_vram++;
					n_u++;
					n_distance--;
				}
				break;
			case 2:
				/* 15 bit */
				while( n_distance > 0 )
				{
					n_bgr = *( m_p_p_vram[ n_ty + n_v ] + n_tx + n_u );
					n_bgr = ( m_p_n_redshade[ m_p_n_redlevel[ n_bgr ] | n_r.w.h ] |
								m_p_n_greenshade[ m_p_n_greenlevel[ n_bgr ] | n_g.w.h ] |
								m_p_n_blueshade[ m_p_n_bluelevel[ n_bgr ] | n_b.w.h ] );
					if( n_bgr != 0 )
					{
						*( p_vram ) = n_bgr;
					}
					p_vram++;
					n_u++;
					n_distance--;
				}
				break;
			}
		}
		n_v++;
		n_y++;
		n_h--;
	}
}

static void Sprite8x8( void )
{
	PAIR n_r;
	PAIR n_g;
	PAIR n_b;
	UINT32 n_tp;
	INT16 n_tx;
	INT16 n_ty;

	UINT32 n_clutx;
	UINT32 n_cluty;

	INT16 n_u;
	INT16 n_v;
	INT16 n_distance;
	UINT32 n_h;
	INT16 n_y;
	INT16 n_x;
	UINT16 *p_vram;
	UINT16 *p_clut;
	UINT16 n_bgr;

#if defined( MAME_DEBUG )
	DebugMesh( SINT12( COORD_X( m_packet.Sprite8x8.n_coord ) + m_n_drawoffset_x ), SINT12( COORD_Y( m_packet.Sprite8x8.n_coord ) + m_n_drawoffset_x ) );
	DebugMesh( SINT12( COORD_X( m_packet.Sprite8x8.n_coord ) + m_n_drawoffset_x + 7 ), SINT12( COORD_Y( m_packet.Sprite8x8.n_coord ) + m_n_drawoffset_x ) );
	DebugMesh( SINT12( COORD_X( m_packet.Sprite8x8.n_coord ) + m_n_drawoffset_x ), SINT12( COORD_Y( m_packet.Sprite8x8.n_coord ) + m_n_drawoffset_x + 7 ) );
	DebugMesh( SINT12( COORD_X( m_packet.Sprite8x8.n_coord ) + m_n_drawoffset_x + 7 ), SINT12( COORD_Y( m_packet.Sprite8x8.n_coord ) + m_n_drawoffset_x  + 7 ) );
	DebugMeshEnd();
#endif

	n_r.w.h = BGR_R( m_packet.Sprite8x8.n_bgr ); n_r.w.l = 0;
	n_g.w.h = BGR_G( m_packet.Sprite8x8.n_bgr ); n_g.w.l = 0;
	n_b.w.h = BGR_B( m_packet.Sprite8x8.n_bgr ); n_b.w.l = 0;

	n_clutx = ( m_packet.Sprite8x8.n_texture.w.h & 0x3f ) << 4;
	n_cluty = ( m_packet.Sprite8x8.n_texture.w.h >> 6 ) & 0x3ff;
	p_clut = m_p_p_vram[ n_cluty ] + n_clutx;

	n_tp = ( m_n_gpustatus & 0x180 ) >> 7;
	n_tx = ( m_n_gpustatus & 0x0f ) << 6;
	n_ty = ( m_n_gpustatus & 0x10 ) << 4;

	n_v = TEXTURE_V( m_packet.Sprite8x8.n_texture );
	n_y = SINT12( COORD_Y( m_packet.Sprite8x8.n_coord ) + m_n_drawoffset_y );

	n_h = 8;
	while( n_h != 0 )
	{
		n_u = TEXTURE_U( m_packet.Sprite8x8.n_texture );
		n_x = SINT12( COORD_X( m_packet.Sprite8x8.n_coord ) + m_n_drawoffset_x );
		n_distance = 8;
		if( n_distance > 0 && n_y >= m_n_drawarea_y1 && n_y <= m_n_drawarea_y2 )
		{
			if( (INT32)( m_n_drawarea_x1 - n_x ) > 0 )
			{
				n_u += ( m_n_drawarea_x1 - n_x );
				n_distance -= ( m_n_drawarea_x1 - n_x );
				n_x = m_n_drawarea_x1;
			}
			if( n_distance > ( m_n_drawarea_x2 - n_x ) + 1 )
			{
				n_distance = ( m_n_drawarea_x2 - n_x ) + 1;
			}
			p_vram = m_p_p_vram[ n_y ] + n_x;

			switch( n_tp )
			{
			case 0:
				/* 4 bit clut */
				while( n_distance > 0 )
				{
					n_bgr = p_clut[ ( *( m_p_p_vram[ n_ty + n_v ] + n_tx + ( n_u >> 2 ) ) >> ( ( n_u & 0x03 ) << 2 ) ) & 0x0f ];
					n_bgr = ( m_p_n_redshade[ m_p_n_redlevel[ n_bgr ] | n_r.w.h ] |
								m_p_n_greenshade[ m_p_n_greenlevel[ n_bgr ] | n_g.w.h ] |
								m_p_n_blueshade[ m_p_n_bluelevel[ n_bgr ] | n_b.w.h ] );
					if( n_bgr != 0 )
					{
						*( p_vram ) = n_bgr;
					}
					p_vram++;
					n_u++;
					n_distance--;
				}
				break;
			case 1:
				/* 8 bit clut */
				while( n_distance > 0 )
				{
					n_bgr = p_clut[ ( *( m_p_p_vram[ n_ty + n_v ] + n_tx + ( n_u >> 1 ) ) >> ( ( n_u & 0x01 ) << 3 ) ) & 0xff ];
					n_bgr = ( m_p_n_redshade[ m_p_n_redlevel[ n_bgr ] | n_r.w.h ] |
								m_p_n_greenshade[ m_p_n_greenlevel[ n_bgr ] | n_g.w.h ] |
								m_p_n_blueshade[ m_p_n_bluelevel[ n_bgr ] | n_b.w.h ] );
					if( n_bgr != 0 )
					{
						*( p_vram ) = n_bgr;
					}
					p_vram++;
					n_u++;
					n_distance--;
				}
				break;
			case 2:
				/* 15 bit */
				while( n_distance > 0 )
				{
					n_bgr = *( m_p_p_vram[ n_ty + n_v ] + n_tx + n_u );
					n_bgr = ( m_p_n_redshade[ m_p_n_redlevel[ n_bgr ] | n_r.w.h ] |
								m_p_n_greenshade[ m_p_n_greenlevel[ n_bgr ] | n_g.w.h ] |
								m_p_n_blueshade[ m_p_n_bluelevel[ n_bgr ] | n_b.w.h ] );
					if( n_bgr != 0 )
					{
						*( p_vram ) = n_bgr;
					}
					p_vram++;
					n_u++;
					n_distance--;
				}
				break;
			}
		}
		n_v++;
		n_y++;
		n_h--;
	}
}

WRITE32_HANDLER( psxgpu_w )
{
	switch( offset )
	{
	case 0x00:
		verboselog( 2, "GPU Buffer Offset: %d\n", m_n_gpu_buffer_offset );
		m_packet.n_entry[ m_n_gpu_buffer_offset ] = data;
		verboselog( 2, "GPU Buffer Write Done\n" );
		switch( m_packet.n_entry[ 0 ] >> 24 )
		{
		case 0x00:
			verboselog( 0, "not handled: GPU Command 0x00: (%08x)\n", data );
			break;
		case 0x01:
			verboselog( 0, "not handled: clear cache\n" );
			break;
		case 0x02:
			if( m_n_gpu_buffer_offset < 2 )
			{
				verboselog( 2, "frame buffer rectangle draw packet #%x\n", m_n_gpu_buffer_offset );
				m_n_gpu_buffer_offset++;
			}
			else
			{
				verboselog( 1, "frame buffer rectangle draw %d,%d %d,%d\n", m_packet.n_entry[ 1 ] & 0xffff, m_packet.n_entry[ 1 ] >> 16, m_packet.n_entry[ 2 ] & 0xffff, m_packet.n_entry[ 2 ] >> 16 );
				FrameBufferRectangleDraw();
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0x20:
			if( m_n_gpu_buffer_offset < 3 )
			{
				verboselog( 2, "flat polygon packet #%x\n", m_n_gpu_buffer_offset );
				m_n_gpu_buffer_offset++;
			}
			else
			{
				verboselog( 1, "flat poly draw\n" );
				FlatPolygon( 3 );
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0x24:
			if( m_n_gpu_buffer_offset < 6 )
			{
				verboselog( 2, "flat textured polygon packet #%x\n", m_n_gpu_buffer_offset );
				m_n_gpu_buffer_offset++;
			}
			else
			{
				verboselog( 1, "flat textured polygon draw\n" );
				FlatTexturedPolygon( 3 );
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0x28:
			if( m_n_gpu_buffer_offset < 4 )
			{
				verboselog( 2, "flat quad packet #%x\n", m_n_gpu_buffer_offset );
				m_n_gpu_buffer_offset++;
			}
			else
			{
				verboselog( 1, "flat quad draw\n" );
				FlatPolygon( 4 );
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0x2c:
			if( m_n_gpu_buffer_offset < 8 )
			{
				verboselog( 2, "flat textured quad packet #%x\n", m_n_gpu_buffer_offset );
				m_n_gpu_buffer_offset++;
			}
			else
			{
				verboselog( 2, "flat textured quad draw\n" );
				FlatTexturedPolygon( 4 );
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0x2e:
			if( m_n_gpu_buffer_offset < 8 )
			{
				verboselog( 2, "flat textured semi-transparent quad packet #%x\n", m_n_gpu_buffer_offset );
				m_n_gpu_buffer_offset++;
			}
			else
			{
				verboselog( 1, "flat textured semi-transparent quad draw\n" );
				FlatTexturedPolygon( 4 );
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0x30:
			if( m_n_gpu_buffer_offset < 5 )
			{
				verboselog( 2, "gouraud polygon packet #%x\n", m_n_gpu_buffer_offset );
				m_n_gpu_buffer_offset++;
			}
			else
			{
				verboselog( 1, "gouraud polygon draw\n" );
				GouraudPolygon( 3 );
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0x38:
			if( m_n_gpu_buffer_offset < 7 )
			{
				verboselog( 2, "gouraud quad packet #%x\n", m_n_gpu_buffer_offset );
				m_n_gpu_buffer_offset++;
			}
			else
			{
				verboselog( 1, "gouraud quad draw\n" );
				GouraudPolygon( 4 );
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0x40:
			if( m_n_gpu_buffer_offset < 2 )
			{
				verboselog( 2, "monochrome line packet #%x\n", m_n_gpu_buffer_offset );
				m_n_gpu_buffer_offset++;
			}
			else
			{
				verboselog( 1, "Monochrome Line\n" );
				MonochromeLine();
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0x48:
			if( data != 0x55555555 )
			{
				verboselog( 2, "monochrome polyline packet #%x\n", m_n_gpu_buffer_offset );
				m_n_gpu_buffer_offset++;
			}
			else
			{
				verboselog( 0, "not handled: Monochrome Polyline\n" );
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0x50:
			if( m_n_gpu_buffer_offset < 3 )
			{
				verboselog( 2, "gouraud packet #%x\n", m_n_gpu_buffer_offset );
				m_n_gpu_buffer_offset++;
			}
			else
			{
				verboselog( 0, "not handled: Gouraud Line\n" );
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0x58:
			if( data != 0x55555555 )
			{
				verboselog( 2, "gouraud polyline packet #%x\n", m_n_gpu_buffer_offset );
				m_n_gpu_buffer_offset++;
			}
			else
			{
				verboselog( 0, "not handled: Gouraud Polyline\n" );
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0x60:
			if( m_n_gpu_buffer_offset < 2 )
			{
				verboselog( 2, "flat rectangle packet #%x\n", m_n_gpu_buffer_offset );
				m_n_gpu_buffer_offset++;
			}
			else
			{
				verboselog( 1, "flat rectangle %d,%d %d,%d\n", m_packet.n_entry[ 1 ] & 0xffff, m_packet.n_entry[ 1 ] >> 16, m_packet.n_entry[ 2 ] & 0xffff, m_packet.n_entry[ 2 ] >> 16 );
				FlatRectangle();
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0x64:
			if( m_n_gpu_buffer_offset < 3 )
			{
				verboselog( 2, "flat textured rectangle packet #%x\n", m_n_gpu_buffer_offset );
				m_n_gpu_buffer_offset++;
			}
			else
			{
				verboselog( 1, "flat textured rectangle%d,%d %d,%d\n", m_packet.n_entry[ 1 ] & 0xffff, m_packet.n_entry[ 1 ] >> 16, m_packet.n_entry[ 3 ] & 0xffff, m_packet.n_entry[ 3 ] >> 16 );
				FlatTexturedRectangle();
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0x74:
			if( m_n_gpu_buffer_offset < 2 )
			{
				verboselog( 2, "8x8 sprite packet #%x\n", m_n_gpu_buffer_offset );
				m_n_gpu_buffer_offset++;
			}
			else
			{
				verboselog( 1, "8x8 sprite %08x %08x %08x\n", m_packet.n_entry[ 0 ], m_packet.n_entry[ 1 ], m_packet.n_entry[ 2 ] );
				Sprite8x8();
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0xa0:
			if( m_n_gpu_buffer_offset < 3 )
			{
				verboselog( 2, "send image to framebuffer packet #%x\n", m_n_gpu_buffer_offset );
				m_n_gpu_buffer_offset++;
			}
			else
			{
				UINT32 n_pixel;
				for( n_pixel = 0; n_pixel < 2; n_pixel++ )
				{
					*( m_p_p_vram[ m_n_vramy + ( m_packet.n_entry[ 1 ] >> 16 ) ] + m_n_vramx + ( m_packet.n_entry[ 1 ] & 0xffff ) ) = data & 0xffff;
					verboselog( 2, "Drawing pixel to %d, %d\n", m_n_vramx, m_n_vramy );
					m_n_vramx++;
					if( m_n_vramx >= ( m_packet.n_entry[ 2 ] & 0xffff ) )
					{
						m_n_vramx = 0;
						m_n_vramy++;
						if( m_n_vramy >= ( m_packet.n_entry[ 2 ] >> 16 ) )
						{
							verboselog( 1, "send image to framebuffer end\n" );
							m_n_gpu_buffer_offset = 0;
							m_n_vramx = 0;
							m_n_vramy = 0;
						}
					}
					data >>= 16;
				}
			}
			break;
		case 0xc0:
			if( m_n_gpu_buffer_offset < 2 )
			{
				verboselog( 2, "copy image from frame buffer packet #%x\n", m_n_gpu_buffer_offset );
				m_n_gpu_buffer_offset++;
			}
			else
			{
				verboselog( 1, "copy image from frame buffer start\n" );
				m_n_gpustatus |= ( 1L << 0x1b );
				m_n_gpu_buffer_offset = 0;
			}
			break;
		case 0xe1:
			verboselog( 1, "draw mode %06x\n", m_packet.n_entry[ 0 ] & 0xffffff );
			m_n_gpustatus = ( m_n_gpustatus & 0xfffff800 ) | ( m_packet.n_entry[ 0 ] & 0x7ff );
			if( ( m_n_gpustatus & ( 1L << 0x09 ) ) != 0 )
			{
				verboselog( 0, "not handled: dither\n" );
			}
			if( ( m_n_gpustatus & ( 1L << 0x0a ) ) != 0 )
			{
				verboselog( 0, "not handled: draw to display area allowed\n" );
			}
			break;
		case 0xe2:
			m_n_twx = ( ( ( m_packet.n_entry[ 0 ] >> 15 ) & 0x1f ) << 3 );
			m_n_twy = ( ( ( m_packet.n_entry[ 0 ] >> 10 ) & 0x1f ) << 3 );
			m_n_tww = 256 - ( ( ( m_packet.n_entry[ 0 ] >> 5 ) & 0x1f ) << 3 );
			m_n_twh = 256 - ( ( m_packet.n_entry[ 0 ] & 0x1f ) << 3 );
			if( m_n_twx != 0 || m_n_twy != 0 || m_n_tww != 256 || m_n_twh != 256 )
			{
				verboselog( 0, "not handled: texture window setting %u %u %u %u\n", m_n_twx, m_n_twy, m_n_tww, m_n_twh );
			}
			else
			{
				verboselog( 1, "not handled: texture window setting %u %u %u %u\n", m_n_twx, m_n_twy, m_n_tww, m_n_twh );
			}
			break;
		case 0xe3:
			m_n_drawarea_x1 = m_packet.n_entry[ 0 ] & 1023;
			m_n_drawarea_y1 = ( m_packet.n_entry[ 0 ] >> 10 ) & 1023;
			verboselog( 1, "drawing area top left %d,%d\n", m_n_drawarea_x1, m_n_drawarea_y1 );
			break;
		case 0xe4:
			m_n_drawarea_x2 = m_packet.n_entry[ 0 ] & 1023;
			m_n_drawarea_y2 = ( m_packet.n_entry[ 0 ] >> 10 ) & 1023;
			verboselog( 1, "drawing area bottom right %d,%d\n", m_n_drawarea_x2, m_n_drawarea_y2 );
			break;
		case 0xe5:
			m_n_drawoffset_x = m_packet.n_entry[ 0 ] & 2047;
			m_n_drawoffset_y = ( m_packet.n_entry[ 0 ] >> 11 ) & 1023;
			verboselog( 1, "drawing offset %d,%d\n", m_n_drawoffset_x, m_n_drawoffset_y );
			break;
		case 0xe6:
			if( ( m_packet.n_entry[ 0 ] & 3 ) != 0 )
			{
				verboselog( 0, "not handled: mask setting %d\n", m_packet.n_entry[ 0 ] & 3 );
			}
			else
			{
				verboselog( 1, "not handled: mask setting %d\n", m_packet.n_entry[ 0 ] & 3 );
			}
			break;
		default:
			verboselog( 0, "unknown gpu packet %08x\n", m_packet.n_entry[ 0 ] );
			m_n_gpu_buffer_offset = 1;
			break;
		}
		break;
	case 0x01:
		switch( data >> 24 )
		{
		case 0x00:
			verboselog( 1, "reset gpu\n" );
			m_n_gpustatus = 0x14802000;
			m_n_drawarea_x1 = 0;
			m_n_drawarea_y1 = 0;
			m_n_drawarea_x2 = 1023;
			m_n_drawarea_y2 = 1023;
			m_n_drawoffset_x = 0;
			m_n_drawoffset_y = 0;
			m_n_displaystartx = 0;
			m_n_displaystarty = 0;
			m_n_screenwidth = 256;
			m_n_screenheight = 240;
			break;
		case 0x01:
			verboselog( 0, "not handled: reset command buffer\n" );
			break;
		case 0x02:
			verboselog( 0, "not handled: reset irq\n" );
			break;
		case 0x03:
			m_n_gpustatus &= ~( 1L << 0x17 );
			m_n_gpustatus |= ( data & 0x01 ) << 0x17;
			if( ( data & 1 ) != 0 )
			{
				verboselog( 0, "not handled: display enable %d\n", data & 1 );
			}
			else
			{
				verboselog( 1, "not handled: display enable %d\n", data & 1 );
			}
			break;
		case 0x04:
			verboselog( 0, "not handled: dma setup %d\n", data & 3 );
			break;
		case 0x05:
			m_n_displaystartx = data & 1023;
			m_n_displaystarty = ( data >> 10 ) & 1023;
			verboselog( 1, "start of display area %d %d\n", m_n_displaystartx, m_n_displaystarty );
			break;
		case 0x06:
			m_n_horiz_disstart = data & 4095;
			m_n_horiz_disend = ( data >> 12 ) & 4095;
			verboselog( 1, "horizontal display range %d %d\n", m_n_horiz_disstart, m_n_horiz_disend );
			break;
		case 0x07:
			m_n_vert_disstart = data & 1023;
			m_n_vert_disend = ( data >> 10 ) & 2047;
			verboselog( 1, "vertical display range %d %d\n", m_n_vert_disstart, m_n_vert_disend );
			break;
		case 0x08:
			verboselog( 1, "display mode %02x\n", data & 0xff );
			m_n_gpustatus &= ~( 127L << 0x10 );
			m_n_gpustatus |= ( data & 0x3f ) << 0x11; /* width 0 + height + videmode + isrgb24 + isinter */
			m_n_gpustatus |= ( ( data & 0x40 ) >> 0x06 ) << 0x10; /* width 1 */
			/* reverseflag? */
			switch( ( m_n_gpustatus >> 0x13 ) & 1 )
			{
			case 0:
				m_n_screenheight = 240;
				break;
			case 1:
				m_n_screenheight = 480;
				break;
			}
			switch( ( m_n_gpustatus >> 0x11 ) & 3 )
			{
			case 0:
				switch( ( m_n_gpustatus >> 0x10 ) & 1 )
				{
				case 0:
					m_n_screenwidth = 256;
					break;
				case 1:
					m_n_screenwidth = 384;
					break;
				}
				break;
			case 1:
				m_n_screenwidth = 320;
				break;
			case 2:
				m_n_screenwidth = 512;
				break;
			case 3:
				m_n_screenwidth = 640;
				break;
			}
			break;
		case 0x09:
			verboselog( 0, "not handled: GPU Control 0x09: %08x\n", data );
			break;
		case 0x10:
			switch( data & 7 )
			{
			case 0x03:
				verboselog( 1, "GPU Info - Draw area top left\n" );
				m_n_gpuinfo = m_n_drawarea_x1 | ( m_n_drawarea_y1 << 10 );
				break;
			case 0x04:
				verboselog( 1, "GPU Info - Draw area bottom right\n" );
				m_n_gpuinfo = m_n_drawarea_x2 | ( m_n_drawarea_y2 << 10 );
				break;
			case 0x05:
				verboselog( 1, "GPU Info - Draw offset\n" );
				m_n_gpuinfo = m_n_drawoffset_x | ( m_n_drawoffset_y << 11 );
				break;
			case 0x07:
				verboselog( 1, "GPU Info - GPU Type\n" );
				m_n_gpuinfo = 0x00000002;
				break;
			default:
				verboselog( 0, "GPU Info - unknown request (%08x)\n", data );
				m_n_gpuinfo = 0;
				break;
			}
			break;
		default:
			verboselog( 0, "gpu_w( %08x ) unknown gpu command\n", data );
			break;
		}
		break;
	default:
		verboselog( 0, "gpu_w( %08x, %08x, %08x ) unknown register\n", offset, data, mem_mask );
		break;
	}
}

READ32_HANDLER( psxgpu_r )
{
	switch( offset )
	{
	case 0x00:
		if( ( m_n_gpustatus & ( 1L << 0x1b ) ) != 0 )
		{
			UINT32 n_pixel;
			PAIR data;

			verboselog( 2, "copy image from frame buffer ( %d, %d )\n", m_n_vramx, m_n_vramy );
			data.d = 0;
			for( n_pixel = 0; n_pixel < 2; n_pixel++ )
			{
				data.w.l = data.w.h;
				data.w.h = *( m_p_p_vram[ m_n_vramy + ( m_packet.n_entry[ 1 ] >> 16 ) ] + m_n_vramx + ( m_packet.n_entry[ 1 ] & 0xffff ) );
				m_n_vramx++;
				if( m_n_vramx >= ( m_packet.n_entry[ 2 ] & 0xffff ) )
				{
					m_n_vramx = 0;
					m_n_vramy++;
					if( m_n_vramy >= ( m_packet.n_entry[ 2 ] >> 16 ) )
					{
						verboselog( 1, "copy image from frame buffer end\n" );
						m_n_gpustatus &= ~( 1L << 0x1b );
						m_n_gpu_buffer_offset = 0;
						m_n_vramx = 0;
						m_n_vramy = 0;
					}
				}
			}
			return data.d;
		}
		verboselog( 2, "Read GPU Info\n" );
		return m_n_gpuinfo;
	case 0x01:
		verboselog( 1, "read gpu status ( %08x )\n", m_n_gpustatus );
		return m_n_gpustatus;
	default:
		verboselog( 0, "gpu_r( %08x, %08x ) unknown register\n", offset, mem_mask );
		break;
	}
	return 0;
}

INTERRUPT_GEN( psx )
{
	m_n_gpustatus ^= ( 1L << 31 );
	psxirq_set( 0x0001 );
}

void psxgpu_reset( void )
{
	psxgpu_w( 1, 0, 0 );
}
