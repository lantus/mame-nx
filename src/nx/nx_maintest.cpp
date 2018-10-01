
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
	socketInitializeDefault();
	 	
	romList.InitRomList();
	romList.RefreshRomList();
	 
	UI::Init();
 
	options.rotateVertical = true;
	options.samplerate = 48000;
	options.use_samples = true;
	
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
    }
 
    UI::Exit();
	
    return 0;
}

extern "C" void RenderMessage(char *name)
{
	SDL_SetRenderDrawColor(UI::sdl_render, 0, 0, 0, 255);
	SDL_RenderClear(UI::sdl_render);
	UI::DrawText(UI::fntLarge, 300, 360, UI::txtcolor, name);
	SDL_RenderPresent(UI::sdl_render);		

	SDL_Delay(5000);
}


extern "C" void RenderProgress(const char *name, struct rom_load_data *romdata)
{
	SDL_SetRenderDrawColor(UI::sdl_render, 0, 0, 0, 255);
	SDL_RenderClear(UI::sdl_render);
 
	char title[128];
	 
	if( name )
	{
		sprintf( title, "Loading \"%s\"",name );		 
		sprintf( &title[strlen(title)], " (%d/ %d)", romdata->romsloaded, romdata->romstotal );
	}
	else
		strcpy( title, "Loading complete!" );
 
	UI::DrawText(UI::fntLarge, 420, 360, UI::txtcolor, title);
	SDL_RenderPresent(UI::sdl_render);			
}