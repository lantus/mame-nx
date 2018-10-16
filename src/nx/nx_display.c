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

#define GL_MATRIX_MODE				0x0BA0
#define GL_MODELVIEW				0x1700
#define GL_PROJECTION				0x1701
#define GL_TEXTURE					0x1702


static int nTextureWidth = 0;
static int nTextureHeight = 0;

static INT32 frameCount = 0;
static float g_desiredFPS = 0.0f;
static struct osd_create_params		g_createParams = {0};
int offsetx, offsety;
int newx, newy;

UINT32	g_pal32Lookup[65536] = {0};
 
// These will hold our original (pre filtered/scaled) width/height
static int                        g_OrigRenderWidth;
static int                        g_OrigRenderHeight;
 
unsigned char pixels[100000];
 
//-----------------------------------------------------------------------------
// EGL initialization
//-----------------------------------------------------------------------------

static EGLDisplay s_display;
static EGLContext s_context;
static EGLSurface s_surface;


static inline int VidGetTextureSize(int size)
{
	int textureSize = 128;
	while (textureSize < size) {
		textureSize <<= 1;
	}
	return textureSize;
}




static void Helper_RenderPalettized16( void *dest, struct mame_bitmap *bitmap, const struct rectangle *bnds )
{
	struct rectangle bounds = *bnds;
	++bounds.max_x;
	++bounds.max_y;

	UINT32 *destBuffer;
	UINT16 *sourceBuffer = (UINT16*)bitmap->base;
  
	destBuffer = (UINT32*)dest;
 
	sourceBuffer += (bounds.min_y * bitmap->rowpixels) + bounds.min_x;		 
	destBuffer += (g_OrigRenderWidth);
	 
	for( UINT32 y = bounds.min_y; y < bounds.max_y; ++y )
	{
		UINT32	*offset = destBuffer;
		UINT16  *sourceOffset = sourceBuffer;

		for( UINT32 x = bounds.min_x; x < bounds.max_x; ++x )
		{
			// Offset is in RGBX format	
			*(offset++) = g_pal32Lookup[ *(sourceOffset++) ];
		}

		destBuffer += g_OrigRenderWidth;
		sourceBuffer += bitmap->rowpixels;
		
		 
	}
 
}

bool initEgl()
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

void deinitEgl()
{
    if (s_display)
    {
		eglMakeCurrent(s_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		  
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

static const char* const vertexShaderSource = R"text(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec3 ourColor;
out vec2 TexCoord;

void main()
{
	gl_Position = vec4(aPos, 1.0);
	ourColor = aColor;
	TexCoord = vec2(aTexCoord.x, aTexCoord.y);
}
)text";

static const char* const fragmentShaderSource = R"text(
#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

// texture sampler
uniform sampler2D texture1;

void main()
{
	FragColor = texture(texture1, TexCoord);
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

    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(handle, sizeof(msg), NULL, msg);
        //TRACE("%u: %s\n", type, msg);
        glDeleteShader(handle);
        return 0;
    }

    return handle;
}
 
float vertices[] = {
    // positions          // colors           // texture coords
     0.76f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,   // top right
     0.76f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,   // bottom right
    -0.76f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,   // bottom left
    -0.76f,  1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 0.0f    // top left 
};

float rotvertices[] = {
    // positions          // colors           // texture coords
	-0.4f,  1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   1.0f, 0.0f,    // top left 
     0.4f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
     0.4f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,   // bottom right
    -0.4f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left    
};

float rotvertices90[] = {
    // positions          // colors           // texture coords
	 0.4f, -1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   1.0f, 0.0f,    // top left 
    -0.4f, -1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
    -0.4f,  1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,   // bottom right
     0.4f,   1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left    
};
 
unsigned int indices[] = {  
	0, 1, 3, // first triangle
	1, 2, 3  // second triangle
};
	
static GLuint s_program;
static unsigned int VBO, VAO, EBO;
static GLuint s_tex;
 
//---------------------------------------------------------------------
//	osd_create_display
//---------------------------------------------------------------------
int osd_create_display( const struct osd_create_params *params, UINT32 *rgb_components )
{	 

	if (!s_display)
		initEgl();
	
	// Store the creation params
	memcpy( &g_createParams, params, sizeof(g_createParams) );

    // Fill out the orientation from the game driver
	g_createParams.orientation = (Machine->gamedrv->flags & ORIENTATION_MASK);
	g_desiredFPS = params->fps;
 
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
    if (success == GL_FALSE)
    {
        char buf[512];
        glGetProgramInfoLog(s_program, sizeof(buf), NULL, buf);
        return EXIT_FAILURE;
    }
    glDeleteShader(vsh);
    glDeleteShader(fsh);
 
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
	
	if (g_createParams.orientation  & ORIENTATION_FLIP_Y)
		glBufferData(GL_ARRAY_BUFFER, sizeof(rotvertices), rotvertices, GL_STATIC_DRAW);
	else if (g_createParams.orientation  & ORIENTATION_FLIP_X)
		glBufferData(GL_ARRAY_BUFFER, sizeof(rotvertices), rotvertices90, GL_STATIC_DRAW);	
	else
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
 

    // Textures
    glGenTextures(1, &s_tex);
    glActiveTexture(GL_TEXTURE0); // activate the texture unit first before binding texture
    glBindTexture(GL_TEXTURE_2D, s_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
 	 
	glUseProgram(s_program);
 
	set_ui_visarea( 0,0,0,0 );
 	
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
 
	  // Store our original width and height
	g_OrigRenderWidth  = g_createParams.width;
	g_OrigRenderHeight = g_createParams.height;
	
	nTextureWidth =  VidGetTextureSize(g_createParams.width);
	nTextureHeight =  VidGetTextureSize(g_createParams.height);
 	
 		
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
 
  
	return 0;
}

//---------------------------------------------------------------------
//	osd_close_display
//---------------------------------------------------------------------
void osd_close_display(void)
{
	
	// clean up opengl
	
	glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO); 	
    glDeleteVertexArrays(1, &VAO);    
	glDeleteProgram(s_program);
	
	//deinitEgl();
		 
 	
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
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 
 
        if(display->game_bitmap->depth == 16)			
        {            	
	 		
			Helper_RenderPalettized16(pixels, display->game_bitmap, &display->game_bitmap_update );
			
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_createParams.width, g_createParams.height, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels);	

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
 
    // draw our textured cube
    glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
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
