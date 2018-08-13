
// Include the only file
#include "CustomUI.h"
#include "nx_RomList.h"

CRomList romList;	
 
int main()
{	
	romList.InitRomList();
	romList.RefreshRomList();
	
	UI::Init();
	
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