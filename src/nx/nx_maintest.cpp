 #include<switch.h>
// Include the only file
#include "CustomUI.h"
#include "nx_RomList.h"



extern "C" {
#include "osd_cpu.h"
#include "driver.h"
#include "mame.h"

}

CRomList romList;	
 
 
int main()
{	
	//socketInitializeDefault();
	//pcvInitialize();
	 	
	romList.InitRomList();
	romList.RefreshRomList();
	 
	UI::Init();
 
	options.rotateVertical = true;
	options.samplerate = 48000;
	
	options.use_filter = false;
	
	options.brightness = 1.0f;
	options.pause_bright = 0.65f;
	options.gamma = 1.0f;
	options.color_depth = 0;
	
	options.use_artwork = ARTWORK_USE_BACKDROPS | ARTWORK_USE_OVERLAYS | ARTWORK_USE_BEZELS;
	options.artwork_res = 0;
	options.artwork_crop = false;	
	
    while(appletMainLoop())
    {			 
        UI::Loop();
		Gfx::flush();
    }
 
    UI::Exit();
	
    return 0;
}

extern "C" void RenderMessage(char *name)
{ 	 
	Gfx::drawBgImage();	 
	Gfx::drawText(300,360, name,  { 255, 255, 255, 255 }, 20);
	Gfx::flush();
}


extern "C" void RenderProgress(const char *name, struct rom_load_data *romdata)
{
 
	char title[128];

	Gfx::drawBgImage();

	if( name )
	{
		sprintf( title, "Loading \"%s\"",name );		 
		sprintf( &title[strlen(title)], " (%d/ %d)", romdata->romsloaded, romdata->romstotal );
	}
	else
		strcpy( title, "Loading complete!" );
 	
	
	Gfx::drawText(420,360, title,  { 255, 255, 255, 255 }, 20);
	Gfx::flush();
 	
}