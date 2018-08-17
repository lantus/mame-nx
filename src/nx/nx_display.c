 
#include "nx_mame.h"

#include "osd_cpu.h"
#include "osdepend.h"
#include "driver.h"
#include "usrintrf.h"
 
#include <switch.h> 

static INT32 frameCount = 0;
static float g_desiredFPS = 0.0f;
static struct osd_create_params		g_createParams = {0};
int offsetx, offsety;
int newx, newy;

UINT32	g_pal32Lookup[65536] = {0};

//---------------------------------------------------------------------
//	osd_create_display
//---------------------------------------------------------------------
int osd_create_display( const struct osd_create_params *params, UINT32 *rgb_components )
{
  
	debugload("osd_create_display \n");
	gfxInitDefault();
	gfxSetMode(GfxMode_TiledDouble);
	
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
	
	
				
	const float vidScrnAspect = (float)1280.0 / 720.0;
	const float gameAspect = (float)g_createParams.aspect_x/g_createParams.aspect_y;
	
	float newWidth, newHeight;
	
	newWidth = fabs(g_createParams.width*gameAspect);
	newHeight = g_createParams.height;
	
	if((newWidth - floor(newWidth) > 0.5))
		newx = ceil(newWidth);
	else 
		newx = floor(newWidth);
	
	if ((newHeight - floor(newHeight) > 0.5))
		newy = ceil(newHeight);
	else 
		newy = floor(newHeight);
	 
	offsetx = ceil(newx - g_createParams.width)/2;
	offsety = 0;
	
	newx = newx - (newx % 4);
	newy = newy - (newy % 4);
 
	nx_SetResolution(newx ,newy);
		
  
	return 0;
}

//---------------------------------------------------------------------
//	osd_close_display
//---------------------------------------------------------------------
void osd_close_display(void)
{
	gfxInitDefault();
	nx_SetResolution(1280,720);
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
 		
		uint32_t width, height;
		uint32_t pos;
 		
		uint32_t *framebuf = (uint32_t*) gfxGetFramebuffer((uint32_t*)&width, (uint32_t*)&height);
	 		 
         const uint32_t x = display->game_visible_area.min_x;
         const uint32_t y = display->game_visible_area.min_y;
         const uint32_t pitch = display->game_bitmap->rowpixels;

         // Copy pixels
 
         //if(display->game_bitmap->depth == 16)			
         {            	
	 		
			// theres probably a much cleaner way of doing this
			
            const uint16_t* input = &((uint16_t*)display->game_bitmap->base)[y * pitch + x];

            for(int i = 0; i < height; i ++)
            {
               for (int j = offsetx; j < width - offsetx; j ++)
               {
					const uint32_t color = g_pal32Lookup[*input++];
					
					unsigned char r = (color >> 16 ) & 0xFF;
					unsigned char g = (color >> 8 ) & 0xFF;
					unsigned char b = (color ) & 0xFF;
					 											
					framebuf[(uint32_t) gfxGetFramebufferDisplayOffset((uint32_t) j, (uint32_t) i)] = RGBA8_MAXALPHA(r,g,b);
														
               }
 
               input += pitch - (g_createParams.width);
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