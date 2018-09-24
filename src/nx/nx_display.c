#include "osd_cpu.h"
#include "osdepend.h"
#include "driver.h"
#include "usrintrf.h"
#include "nx_mame.h" 
#include <stdio.h>
#include <stdlib.h>
#include <switch.h> 


#include <EGL/egl.h>    // EGL library
#include <EGL/eglext.h> // EGL extensions
#include <glad/glad.h>  // glad library (OpenGL loader)


static INT32 frameCount = 0;
static float g_desiredFPS = 0.0f;
static struct osd_create_params		g_createParams = {0};
int offsetx, offsety;
int newx, newy;

UINT32	g_pal32Lookup[65536] = {0};
 
typedef struct Vertex
{
	float position[3];
	float color[3];
} Vertex;

static const Vertex vertices[] =
{
	{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
	{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
	{ {  0.0f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
};
	
//-----------------------------------------------------------------------------
// EGL initialization
//-----------------------------------------------------------------------------

static EGLDisplay s_display;
static EGLContext s_context;
static EGLSurface s_surface;

static bool initEgl()
{
    // Connect to the EGL default display
    s_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!s_display)
    {
        //TRACE("Could not connect to display! error: %d", eglGetError());
        goto _fail0;
    }

    // Initialize the EGL display connection
    eglInitialize(s_display, NULL, NULL);

    // Select OpenGL (Core) as the desired graphics API
    if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE)
    {
        //TRACE("Could not set API! error: %d", eglGetError());
        goto _fail1;
    }

    // Get an appropriate EGL framebuffer configuration
    EGLConfig config;
    EGLint numConfigs;
    static const EGLint framebufferAttributeList[] =
    {
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_NONE
    };
    eglChooseConfig(s_display, framebufferAttributeList, &config, 1, &numConfigs);
    if (numConfigs == 0)
    {
        //TRACE("No config found! error: %d", eglGetError());
        goto _fail1;
    }

    // Create an EGL window surface
    s_surface = eglCreateWindowSurface(s_display, config, (char*)"", NULL);
    if (!s_surface)
    {
        //TRACE("Surface creation failed! error: %d", eglGetError());
        goto _fail1;
    }

    // Create an EGL rendering context
    static const EGLint contextAttributeList[] =
    {
        EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
        EGL_CONTEXT_MAJOR_VERSION_KHR, 4,
        EGL_CONTEXT_MINOR_VERSION_KHR, 3,
        EGL_NONE
    };
    s_context = eglCreateContext(s_display, config, EGL_NO_CONTEXT, contextAttributeList);
    if (!s_context)
    {
        //TRACE("Context creation failed! error: %d", eglGetError());
        goto _fail2;
    }

    // Connect the context to the surface
    eglMakeCurrent(s_display, s_surface, s_surface, s_context);
    return true;

_fail2:
    eglDestroySurface(s_display, s_surface);
    s_surface = NULL;
_fail1:
    eglTerminate(s_display);
    s_display = NULL;
_fail0:
    return false;
}

static void deinitEgl()
{
    if (s_display)
    {
        if (s_context)
        {
            eglDestroyContext(s_display, s_context);
            s_context = NULL;
        }
        if (s_surface)
        {
            eglDestroySurface(s_display, s_surface);
            s_surface = NULL;
        }
        eglTerminate(s_display);
        s_display = NULL;
    }
}

//-----------------------------------------------------------------------------
// Main program
//-----------------------------------------------------------------------------

static void setMesaConfig()
{
    // Uncomment below to disable error checking and save CPU time (useful for production):
    //setenv("MESA_NO_ERROR", "1", 1);

    // Uncomment below to enable Mesa logging:
    //setenv("EGL_LOG_LEVEL", "debug", 1);
    setenv("MESA_VERBOSE", "all", 1);
    //setenv("NOUVEAU_MESA_DEBUG", "1", 1);

    // Uncomment below to enable shader debugging in Nouveau:
    //setenv("NV50_PROG_OPTIMIZE", "0", 1);
    //setenv("NV50_PROG_DEBUG", "1", 1);
    //setenv("NV50_PROG_CHIPSET", "0x120", 1);
}

static const char* const vertexShaderSource = R"text(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aColor;
    out vec3 ourColor;
    void main()
    {
        gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
        ourColor = aColor;
    }
)text";

static const char* const fragmentShaderSource = R"text(
    #version 330 core
    in vec3 ourColor;
    out vec4 fragColor;
    void main()
    {
        fragColor = vec4(ourColor, 1.0f);
    }
)text";

static GLuint createAndCompileShader(GLenum type, const char* source)
{
    GLint success;
    GLchar msg[512];

    GLuint handle = glCreateShader(type);
    if (!handle)
    {
        //TRACE("%u: cannot create shader", type);
        return 0;
    }
    glShaderSource(handle, 1, &source, NULL);
    glCompileShader(handle);
    glGetShaderiv(handle, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        glGetShaderInfoLog(handle, sizeof(msg), NULL, msg);
        //TRACE("%u: %s\n", type, msg);
        glDeleteShader(handle);
        return 0;
    }

    return handle;
}

static GLuint s_program;
static GLuint s_vao, s_vbo;

//---------------------------------------------------------------------
//	osd_create_display
//---------------------------------------------------------------------
int osd_create_display( const struct osd_create_params *params, UINT32 *rgb_components )
{
	setMesaConfig();

    // Initialize EGL
    if (!initEgl())
        return EXIT_FAILURE;

    // Load OpenGL routines using glad
    gladLoadGL();
	
	GLint vsh = createAndCompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLint fsh = createAndCompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    s_program = glCreateProgram();
    glAttachShader(s_program, vsh);
    glAttachShader(s_program, fsh);
    glLinkProgram(s_program);

    GLint success;
    glGetProgramiv(s_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char buf[512];
        glGetProgramInfoLog(s_program, sizeof(buf), NULL, buf);
        //TRACE("Link error: %s", buf);
    }
    glDeleteShader(vsh);
    glDeleteShader(fsh);


    glGenVertexArrays(1, &s_vao);
    glGenBuffers(1, &s_vbo);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(s_vao);

    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);
	
	//debugload("osd_create_display \n");
	//gfxInitDefault();
	//gfxSetMode(GfxMode_TiledDouble);
	
	set_ui_visarea( 0,0,0,0 );
	
	//debugload("set_ui_visarea \n");
	
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
		options.rotateVertical = true;
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
 
	//nx_SetResolution(newx ,newy);
		
  
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
 		
		if (options.rotateVertical)
		{
				//svcOutputDebugString("rotating",20);
				// gfxConfigureResolution(newy, newx);				 
				 //gfxConfigureTransform(NATIVE_WINDOW_TRANSFORM_FLIP_V| NATIVE_WINDOW_TRANSFORM_ROT_90);
		}
			 
		//uint32_t *framebuf = (uint32_t*) gfxGetFramebuffer((uint32_t*)&width, (uint32_t*)&height);
	 		 
         const uint32_t x = display->game_visible_area.min_x;
         const uint32_t y = display->game_visible_area.min_y;
         const uint32_t pitch = display->game_bitmap->rowpixels;
		  
         // Copy pixels
 
         if(display->game_bitmap->depth == 16)			
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
					 											
					//framebuf[(uint32_t) gfxGetFramebufferDisplayOffset((uint32_t) j, (uint32_t) i)] = RGBA8_MAXALPHA(r,g,b);
														
               }
 
               input += pitch - (g_createParams.height);
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

	//gfxFlushBuffers();
	//gfxSwapBuffers();
	//gfxWaitForVsync();
	
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // draw our first triangle
    glUseProgram(s_program);
    glBindVertexArray(s_vao); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
    glDrawArrays(GL_TRIANGLES, 0, 3);	
	eglSwapBuffers(s_display, s_surface);

 
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