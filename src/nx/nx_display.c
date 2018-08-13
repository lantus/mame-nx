 
#include "nx_mame.h"

#include "osd_cpu.h"
#include "osdepend.h"
#include "driver.h"
#include "usrintrf.h"
 
#include <switch/gfx/gfx.h> 

static INT32 frameCount = 0;
static float g_desiredFPS = 0.0f;
static struct osd_create_params		g_createParams = {0};

UINT32	g_pal32Lookup[65536] = {0};

//---------------------------------------------------------------------
//	osd_create_display
//---------------------------------------------------------------------
int osd_create_display( const struct osd_create_params *params, UINT32 *rgb_components )
{
  
	debugload("osd_create_display \n");
	gfxInitDefault();
	
	set_ui_visarea( 0,0,0,0 );
	
	debugload("set_ui_visarea \n");
	
	if(Machine->color_depth == 16)
	{
      /* 32bpp only */
		rgb_components[0] = 0x7C00;
		rgb_components[1] = 0x03E0;
		rgb_components[2] = 0x001F;  
	}
	else if(Machine->color_depth == 32)
	{
		rgb_components[0] = 0xFF0000;
		rgb_components[1] = 0x00FF00;
		rgb_components[2] = 0x0000FF;
	}

	
	// Store the creation params
	memcpy( &g_createParams, params, sizeof(g_createParams) );

    // Fill out the orientation from the game driver
	g_createParams.orientation = (Machine->gamedrv->flags & ORIENTATION_MASK);
	g_desiredFPS = params->fps;
	
		// Flip the width and height
	if( g_createParams.orientation & ORIENTATION_SWAP_XY )
	{
		INT32 temp = g_createParams.height;
		g_createParams.height = g_createParams.width;
		g_createParams.width = temp;

		temp = g_createParams.aspect_x;
		g_createParams.aspect_x = g_createParams.aspect_y;
		g_createParams.aspect_y = temp;
	}
	   
  nx_SetResolution(g_createParams.width,g_createParams.height);
  
  return 0;
}

//---------------------------------------------------------------------
//	osd_close_display
//---------------------------------------------------------------------
void osd_close_display(void)
{
 
}

//---------------------------------------------------------------------
//	osd_skip_this_frame
//---------------------------------------------------------------------
int osd_skip_this_frame(void)
{
	return 0;
}

//---------------------------------------------------------------------
//	osd_update_video_and_audio
//---------------------------------------------------------------------
void osd_update_video_and_audio(struct mame_display *display)
{
	static cycles_t lastFrameEndTime = 0;
	const struct performance_info *performance = mame_get_performance_info();
  
	if( display->changed_flags & GAME_VISIBLE_AREA_CHANGED )
	{
				
			// Pass the new coords on to the UI
		set_ui_visarea( display->game_visible_area.min_x,
										display->game_visible_area.min_y,
										display->game_visible_area.max_x,
										display->game_visible_area.max_y );
	}

	if( display->changed_flags & GAME_PALETTE_CHANGED )
	{	
		nx_UpdatePalette( display );
	}
	
	
	if( display->changed_flags & GAME_BITMAP_CHANGED )
	{		 
		//nx_SoftRender(	display->game_bitmap, &display->game_bitmap_update,NULL );
		
		uint32_t width, height;
		uint32_t pos;
		uint32_t *framebuf = (uint32_t*) gfxGetFramebuffer((uint32_t*)&width, (uint32_t*)&height);
	 		 
         const uint32_t x = display->game_visible_area.min_x;
         const uint32_t y = display->game_visible_area.min_y;
         const uint32_t pitch = display->game_bitmap->rowpixels;

         // Copy pixels
		 
		 char depth[50];
		 sprintf(depth,"depth = %d\n", display->game_bitmap->depth);
		 //svcOutputDebugString(depth);
		 
         //if(display->game_bitmap->depth == 16)			
         {            	
	 			 	
            const uint16_t* input = &((uint16_t*)display->game_bitmap->base)[y * pitch + x];

            for(int i = 0; i < height; i ++)
            {
               for (int j = 0; j < width; j ++)
               {
					const uint32_t color = g_pal32Lookup[*input++];                												
					
					unsigned char r = (color >> 16 ) & 0xFF;
					unsigned char g = (color >> 8 ) & 0xFF;
					unsigned char b = (color ) & 0xFF;
					 									
					framebuf[(uint32_t) gfxGetFramebufferDisplayOffset((uint32_t) j  , (uint32_t) i  )] = RGBA8_MAXALPHA(r,g,b);
					
               }
			   
               input += pitch - width;
            }
			             
         }
 
		// Wait out the remaining time for this frame
		if( lastFrameEndTime &&         
			performance->game_speed_percent >= 99.0f  )
		{
			// Only wait for 99.5% of the frame time to elapse, as there's still some stuff that
			// needs to be done before we return to MAME
			cycles_t targetFrameCycles = (cycles_t)( (double)osd_cycles_per_second() / (g_desiredFPS*1.001));
			cycles_t actualFrameCycles = osd_cycles() - lastFrameEndTime;

	 
			while( actualFrameCycles < targetFrameCycles )
			{
			  // Catch wraparound (which won't happen for a long time :))
				if( osd_cycles() < lastFrameEndTime )
					break;
				actualFrameCycles = osd_cycles() - lastFrameEndTime;
			}
		}
		  // Tag the end of this frame
		lastFrameEndTime = osd_cycles();
	}

	gfxFlushBuffers();
	gfxSwapBuffers();
	gfxWaitForVsync();

 
}


//============================================================
//	osd_override_snapshot
//============================================================

struct mame_bitmap *osd_override_snapshot(struct mame_bitmap *bitmap, struct rectangle *bounds)
{
	 
	return NULL;
}

 
const char *osd_get_fps_text( const struct performance_info *performance )
{
 
	return NULL;
}
 
 
 
int nx_SoftRender(	struct mame_bitmap *bitmap,
					const struct rectangle *bounds,
					void *vector_dirty_pixels )
{

	uint32_t width, height;
	uint32_t pos;
	uint8_t *framebuf = (uint8_t*) gfxGetFramebuffer((uint32_t*)&width, (uint32_t*)&height);
	frameCount ++;
	     	
	if( vector_dirty_pixels )
	{
	 
	}
	else
	{
		  // Blit the bitmap to the texture
		if( bitmap->depth == 15 || bitmap->depth == 16 )
		{
			if( g_createParams.video_attributes & VIDEO_RGB_DIRECT )
			{
				  // Destination buffer is in 15 bit X1R5G5B5
				nx_RenderDirect16( framebuf, bitmap, bounds );
				
				svcOutputDebugString("nx_RenderDirect16",20);
			}
			else
			{
				  // Have to translate the colors through the palette lookup table
				nx_RenderPalettized16( framebuf, bitmap, bounds );
								
			}
		}
		else if( bitmap->depth == 32 )
		{
				//svcOutputDebugString("32 bit",20);
				nx_RenderDirect32( framebuf, bitmap, bounds );
		}	
		//else
	//		fatalerror( "Attempt to render with unknown depth %lu!\nPlease report this game immediately!", bitmap->depth );
	}
  	 
	gfxFlushBuffers();
	gfxSwapBuffers();
	gfxWaitForVsync();
 
	return 0;
 
}  


//-------------------------------------------------------------
//	nx_RenderDirect16
//-------------------------------------------------------------
void nx_RenderDirect16( void *dest, struct mame_bitmap *bitmap, const struct rectangle *bnds )
{
	struct rectangle bounds = *bnds;
	++bounds.max_x;
	++bounds.max_y;

	UINT16 *destBuffer;
	UINT16 *sourceBuffer = (UINT16*)bitmap->base;

	destBuffer = (UINT16*)dest;

	if( g_createParams.orientation & ORIENTATION_SWAP_XY )
	{
		sourceBuffer += (bounds.min_y * bitmap->rowpixels) + bounds.min_x;

		// SwapXY 
		destBuffer += bounds.min_y;  // The bounds.min_y value gives us our starting X coord
		destBuffer += (bounds.min_x * g_createParams.width); // The bounds.min_x value gives us our starting Y coord

		// Render, treating sourceBuffer as normal (x and y not swapped)
		for( UINT32 y = bounds.min_y; y < bounds.max_y; ++y )
		{
			UINT16	*offset = destBuffer;
			UINT16  *sourceOffset = sourceBuffer;

			for( UINT32 x = bounds.min_x; x < bounds.max_x; ++x )
			{
				*offset = *(sourceOffset++);
				offset += g_createParams.width;   // Increment the output Y value
			}

			sourceBuffer += bitmap->rowpixels;
			++destBuffer;          // Come left ("down") one row
		}
	}
	else
	{ 
		sourceBuffer += (bounds.min_y * bitmap->rowpixels) + bounds.min_x;
		destBuffer += ((bounds.min_y * g_createParams.width) + bounds.min_x);

		UINT32 scanLen = (bounds.max_x - bounds.min_x) << 1;

		for( UINT32 y = bounds.min_y; y < bounds.max_y; ++y )
		{
			memcpy( destBuffer, sourceBuffer, scanLen );
			destBuffer += g_createParams.width;
			sourceBuffer += bitmap->rowpixels;
		}
	}

	 
}


//-------------------------------------------------------------
//	nx_RenderDirect32
//-------------------------------------------------------------
void nx_RenderDirect32( void *dest, struct mame_bitmap *bitmap, const struct rectangle *bnds )
{
	struct rectangle bounds = *bnds;
	++bounds.max_x;
	++bounds.max_y;

		// 32 bit direct
	if( !(g_createParams.video_attributes & VIDEO_RGB_DIRECT) )
	{			 
		return;
	}
	
	UINT32 *destBuffer;
	UINT32 *sourceBuffer = (UINT32*)bitmap->base;

	destBuffer = (UINT32*)dest;

  	// Destination buffer is in 32 bit X8R8G8B8
	if( g_createParams.orientation & ORIENTATION_SWAP_XY )
	{
		sourceBuffer += (bounds.min_y * bitmap->rowpixels) + bounds.min_x;

		// SwapXY 
		destBuffer += bounds.min_y;  // The bounds.min_y value gives us our starting X coord
		destBuffer += (bounds.min_x * g_createParams.width); // The bounds.min_x value gives us our starting Y coord

		// Render, treating sourceBuffer as normal (x and y not swapped)
		for( UINT32 y = bounds.min_y; y < bounds.max_y; ++y )
		{
			UINT32	*offset = destBuffer;
			UINT32  *sourceOffset = sourceBuffer;

			for( UINT32 x = bounds.min_x; x < bounds.max_x; ++x )
			{
				*offset = *(sourceOffset++);
				offset += g_createParams.width;   // Increment the output Y value
			}

			sourceBuffer += bitmap->rowpixels;
			++destBuffer;          // Move right ("down") one row
		}
	}
	else
	{ 
		sourceBuffer += (bounds.min_y * bitmap->rowpixels) + bounds.min_x;

		destBuffer += ((bounds.min_y * g_createParams.width) + bounds.min_x);
		UINT32 scanLen = (bounds.max_x - bounds.min_x) << 2;

		for( UINT32 y = bounds.min_y; y < bounds.max_y; ++y )
		{
			memcpy( destBuffer, sourceBuffer, scanLen );
			destBuffer += g_createParams.width;
			sourceBuffer += bitmap->rowpixels;
		}
	}

	 
}

 
//-------------------------------------------------------------
//	nx_RenderPalettized16
//-------------------------------------------------------------
void nx_RenderPalettized16( void *dest, struct mame_bitmap *bitmap, const struct rectangle *bnds )
{

	struct rectangle bounds = *bnds;
	++bounds.max_x;
	++bounds.max_y;

	UINT32 *destBuffer;
	UINT16 *sourceBuffer = (UINT16*)bitmap->base;

	  // If we a filtering render into a temp buffer then filter that buffer
	  // into the actual framebuffer
	 
	destBuffer = (UINT32*)dest;

		// bitmap format is 16 bit indices into the palette
		// Destination buffer is in 32 bit X8R8G8B8
	if( g_createParams.orientation & ORIENTATION_SWAP_XY )
	{ 
		sourceBuffer += (bounds.min_y * bitmap->rowpixels) + bounds.min_x;

		// SwapXY
		destBuffer += bounds.min_y;  // The bounds.min_y value gives us our starting X coord
		destBuffer += (bounds.min_x * g_createParams.width); // The bounds.min_x value gives us our starting Y coord

      // Render, treating sourceBuffer as normal (x and y not swapped)
		for( UINT32 y = bounds.min_y; y < bounds.max_y; ++y )
		{
			UINT32	*offset = destBuffer;
			UINT16  *sourceOffset = sourceBuffer;

			for( UINT32 x = bounds.min_x; x < bounds.max_x; ++x )
			{
			  // Offset is in RGBX format	
				*offset = g_pal32Lookup[ *(sourceOffset++) ];

			  // Skip to the next row
				offset += g_createParams.width;   // Increment the output Y value
			}

			sourceBuffer += bitmap->rowpixels;
			++destBuffer;          // Come left ("down") one row
		}
		
	}
	else
	{
			sourceBuffer += (bounds.min_y * bitmap->rowpixels) + bounds.min_x;
			destBuffer += (bounds.min_y * 720 + bounds.min_x);

			for( UINT32 y = bounds.min_y; y < bounds.max_y; ++y )
			{
				UINT32	*offset = destBuffer;
				UINT16  *sourceOffset = sourceBuffer;

				for( UINT32 x = bounds.min_x; x < bounds.max_x; ++x )
				{
			  // Offset is in RGBX format	
					*(offset++) = g_pal32Lookup[ *(sourceOffset++) ];
				}

				destBuffer += g_createParams.width;
				sourceBuffer += bitmap->rowpixels;
			}		

	}
	 
}


//------------------------------------------------------------
//	nx_UpdatePalette
//------------------------------------------------------------

void nx_UpdatePalette( struct mame_display *display )
{
	UINT32 i, j;
 
		// The game_palette_dirty entry is a bitflag specifying which
		// palette entries need to be updated

	for( i = 0, j = 0; i < display->game_palette_entries; i += 32, ++j )
	{
		UINT32 palDirty = display->game_palette_dirty[j];
		if( palDirty )
		{
			UINT32 idx = 0;
			for( ; idx < 32 && i + idx < display->game_palette_entries; ++idx )
			{
				if( palDirty & (1<<idx) )
					g_pal32Lookup[i+idx] = display->game_palette[i+idx];
			}

			display->game_palette_dirty[ j ] = 0;
		}
	}
}

 
void nx_SetResolution(uint32_t width, uint32_t height)
{
    uint32_t x, y, w, h, i;
    uint32_t *fb;

    // clear framebuffers before switching res
    for (i = 0; i < 2; i++) {

        fb = (uint32_t *) gfxGetFramebuffer(&w, &h);

        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++) {
                fb[gfxGetFramebufferDisplayOffset(x, y)] =
                    (uint32_t) RGBA8_MAXALPHA(0, 0, 0);
            }
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
        gfxWaitForVsync();
    }

    gfxConfigureResolution(width, height);
} 