 
#include <iostream>
#include <string>
#include <vector>
#include <map> 
#include <functional>
using namespace std;

#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h> 
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include "nx_romlist.h"


extern "C" {
#include "osd_cpu.h"
#include "driver.h"
#include "mame.h"

 
bool initEgl();
void deinitEgl();

}


#define	GAMESELECT 1
#define	CONFIGSCREEN 2
 
extern std::vector<std::string> m_vecAvailRomList;
extern std::map<std::string, int> mapRoms;
extern CRomList romList;	
 
const int GAMESEL_MaxWindowList	= 16;		 
const int GAMESEL_WindowMiddle = 8;	

static bool bGLInited = false;
int MenuState;
SDL_Renderer *menu_render;
TTF_Font *menuFntLarge;


float fGameSelect;
float fCursorPos;
float fMaxCount;
float m_fFrameTime;			// amount of time per frame

int	  iGameSelect;
int	  iCursorPos;
int	  iNumGames;
int	  LastPos;
int	  iMaxWindowList;
int	  iWindowMiddle;
 
int dr = 0;
int dg = 0;
int db = 0;
		
namespace UI
{
	std::string currentGame;
	
	int selRed,selGreen,selBlue,selAlpha;
	int optRed,optGreen,optBlue,optAlpha;
	
	char RomCountText[60];
    static SDL_Window *sdl_wnd;
    static SDL_Surface *sdl_surf;
    static SDL_Renderer *sdl_render;
    static TTF_Font *fntLarge;
    static TTF_Font *fntMedium;
    static TTF_Font *fntSmall;
     

    static string Back;
    static SDL_Color txtcolor;
    static SDL_Surface *sdls_Back;
    static SDL_Texture *sdlt_Back;
    static int TitleX = 60;
    static int TitleY = 235;
    static int Opt1X = 55;
    static int Opt1Y = 275;

    static vector<string> optionsvec;
    static int selected = 0;
    static vector<string> foptions;
    static int fselected = 0;
    static string FooterText;

    static int vol = 64;
    static string curfilename = ":nofile:";
    static string curfile = ":nofile:";

	
    SDL_Surface *InitSurface(string Path)
    {
        SDL_Surface *srf = IMG_Load(Path.c_str());
        if(srf)
        {
            Uint32 colorkey = SDL_MapRGB(srf->format, 0, 0, 0);
            SDL_SetColorKey(srf, SDL_TRUE, colorkey);
        }
        return srf;
    }

    SDL_Texture *InitTexture(SDL_Surface *surf)
    {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(sdl_render, surf);
        return tex;
    }

    void DrawText(TTF_Font *font, int x, int y, SDL_Color colour, const char *text)
    {
        SDL_Surface *surface = TTF_RenderText_Blended_Wrapped(font, text, colour, 1280);
        SDL_SetSurfaceAlphaMod(surface, 255);
        SDL_Rect position = { x, y, surface->w, surface->h };
        SDL_BlitSurface(surface, NULL, sdl_surf, &position);
        SDL_FreeSurface(surface);
    }

    void DrawRect(int x, int y, int w, int h, SDL_Color colour)
    {
        SDL_Rect rect;
        rect.x = x; rect.y = y; rect.w = w; rect.h = h;
        SDL_SetRenderDrawColor(sdl_render, colour.r, colour.g, colour.b, colour.a);
        SDL_RenderFillRect(sdl_render, &rect);
    }

    void DrawBackXY(SDL_Surface *surf, SDL_Texture *tex, int x, int y)
    {
        SDL_Rect position;
        position.x = x;
        position.y = y;
        position.w = surf->w;
        position.h = surf->h;
        SDL_RenderCopy(sdl_render, tex, NULL, &position);
    }

    void DrawBack(SDL_Surface *surf, SDL_Texture *tex)
    {
        DrawBackXY(surf, tex, 0, 0);	
    }

    void Exit()
    {
        TTF_Quit();
        IMG_Quit();         
        SDL_DestroyRenderer(sdl_render);
        SDL_FreeSurface(sdl_surf);
        SDL_DestroyWindow(sdl_wnd);
 
        SDL_Quit();		
        romfsExit();
        exit(0);
    }

    void Draw()
    {		
		char currentName[120];
		int	iTempGameSel;
		int	iGameidx;
	
        SDL_RenderClear(sdl_render);
        DrawBack(sdls_Back, sdlt_Back);
     
        int ox = Opt1X;
        int oy = Opt1Y;
        for(int i = 0; i < optionsvec.size(); i++)
        {
            if(i == selected)
            {
 
				selRed += dr;
				selGreen += dg;
				selBlue += db;

				if(selRed == 255 && selGreen == 0 && selBlue == 0)
				{
					dr = 0; dg = 1; db = 0;
				}

				if(selRed == 255 && selGreen == 255 && selBlue == 0)
				{
					dr = -1; dg = 0; db = 0;
				}

				if(selRed == 0 && selGreen == 255 && selBlue == 0)
				{
					dr = 0; dg = 0; db = 1;
				}

				if(selRed == 0 && selGreen == 255 && selBlue == 255)
				{
					dr = 0; dg = -1; db = 0;
				}

				if(selRed == 0 && selGreen == 0 && selBlue == 255)
				{
					dr = 1; dg = 0; db = 0;
				}

				if(selRed == 255 && selGreen == 0 && selBlue == 255)
				{
					dr = 0; dg = 0; db = -1;
				} 
				
                DrawText(fntLarge, ox, oy, { 140, 255, 255, 255 }, optionsvec[i].c_str());
                
				if(i == 0)
                {
                    int fx = 450;
                    int fy = 235;
					 
					
					// draw	game list entries

					iGameSelect	= fGameSelect;
					iCursorPos = fCursorPos;
					iTempGameSel = iGameSelect;

					if (iNumGames == 0)	
						DrawText(fntLarge, fx, 202, txtcolor, "No Roms Found");	 
					for	(iGameidx=0; iGameidx<iMaxWindowList;	iGameidx++)
					{
						
						sprintf( currentName, "");
						sprintf( currentName, "%s", m_vecAvailRomList[iTempGameSel++].c_str() );


						if (iGameidx==iCursorPos){
							 
							DrawText(fntLarge, fx, fy + (28*iGameidx), { selRed, selGreen, selBlue, 255 }, currentName);
							
							currentGame = currentName; 
						}
						else
						{
							DrawText(fntLarge, fx, fy + (28*iGameidx), txtcolor, currentName);
						}

					}


                }
                else if(i == 1)
                {
					//DrawText(fntLarge, 450, 400, txtcolor, "Rotate Vertical Games");
					//DrawText(fntLarge, 450, 420, txtcolor, "Keep Aspect Ratio");
                     
                }
				else if(i == 2)
                {
                   DrawText(fntLarge, 450, 400, txtcolor, "Release 2. Ported by MVG in 2018");
                }
				else if(i == 3)
                {
                   
                }
            }
            else DrawText(fntLarge, ox, oy, txtcolor, optionsvec[i].c_str());
            oy += 50;
        }
        DrawText(fntLarge, TitleX, 672, txtcolor, RomCountText);
        SDL_RenderPresent(sdl_render);
    }
    
    void Loop()
    {
		hidScanInput();
        int k = hidKeysDown(CONTROLLER_P1_AUTO);
        int h = hidKeysHeld(CONTROLLER_P1_AUTO);
        
		if(h & KEY_ZL || k & KEY_DUP)
        {
			// default don`t clamp cursor
			bool bClampCursor =	false;

			fCursorPos --;
			if(	fCursorPos < iWindowMiddle )
			{
				// clamp cursor	position
				bClampCursor = true;

				// backup window pos
				fGameSelect	--;

				// clamp game window range (low)
				if(fGameSelect < 0)
				{
					// clamp to	start
					fGameSelect	= 0;

					// backup cursor pos after all!
					bClampCursor = false;

					// clamp cursor	to end
					if(	fCursorPos < 0 )
						fCursorPos = 0;
				}
			}

			// check for cursor	clamp
			if(	bClampCursor )
				fCursorPos = iWindowMiddle;	

			Draw();
        }
        else if(h & KEY_ZR || k & KEY_DDOWN)
        {
 
			// default don`t clamp cursor
			bool bClampCursor =	false;

			fCursorPos ++;

			if(	fCursorPos > iWindowMiddle )
			{
				// clamp cursor	position
				bClampCursor = true;
				
				//fCursorPos += 0.8f * 0.5f * 5.0f;	// velocity

				// advance gameselect
				if(fGameSelect == 0) fGameSelect +=	(fCursorPos	- iWindowMiddle);
				else fGameSelect ++;
			 
				// clamp game window range (high)
				if((fGameSelect	+ iMaxWindowList)	> iNumGames)
				{

					// clamp to	end
					fGameSelect	= iNumGames	- iMaxWindowList;

					// advance cursor pos after	all!
					bClampCursor = false;

					// clamp cursor	to end
					if((fGameSelect	+ fCursorPos) >= iNumGames)
						fCursorPos = iMaxWindowList-1;
				}
			}

			// check for cursor	clamp
			if(	bClampCursor )
				fCursorPos = iWindowMiddle;	
			
			Draw();
        }
		else if( k & KEY_DRIGHT)
        {
 
			char currName[200];
			char c;
			int oCursorPos;
			int tmp;

			int gameIndex = mapRoms[currentGame]; 
			
			// default don`t clamp cursor
			bool bClampCursor = false;

			oCursorPos = fCursorPos;
			tmp = fCursorPos;

			strcpy(currName, drivers[gameIndex]->description);
			c = currName[0];
									
			while((fGameSelect + tmp) < iNumGames - 1)
			{
				char name[200];
				char n;

				tmp ++;
				
				strcpy(name, m_vecAvailRomList[fGameSelect+tmp].c_str());
				n = name[0];
								
				
				if(n > c)
				{
					fCursorPos = tmp; //(iGameidx - oGameidx);

					if( fCursorPos > iWindowMiddle )
					{
						// clamp cursor position
						bClampCursor = true;
				
						// advance gameselect
						if(fGameSelect == 0) fGameSelect += (fCursorPos - iWindowMiddle);
						else  fGameSelect += (fCursorPos - oCursorPos);

						// clamp game window range (high)
						if((fGameSelect + iMaxWindowList) > iNumGames)
						{						
							fCursorPos = oCursorPos + (fGameSelect - (iNumGames - iMaxWindowList));
							
							// clamp to end
							fGameSelect = iNumGames - iMaxWindowList;
					
							// advance cursor pos after all!
							bClampCursor = false;
					
							// clamp cursor to end
							if(fCursorPos > iMaxWindowList-1)
							fCursorPos = iMaxWindowList-1;
						}
					}
					
					// check for cursor clamp
					if( bClampCursor )
						fCursorPos = iWindowMiddle;	
										
					break;
				}
			}

			Draw();
			
        }
		else if( k & KEY_DLEFT)
        {
 
			char currName[200];
			char c;
			int oCursorPos;
			int tmp;

			int gameIndex = mapRoms[currentGame]; 
			
			// default don`t clamp cursor
			bool bClampCursor = false;

			oCursorPos = fCursorPos;
			tmp = fCursorPos;

			strcpy(currName, drivers[gameIndex]->description);
			c = currName[0];
									
			while((fGameSelect + tmp) > 0)
			{
				char name[200];
				char n;

				tmp--;
				
				strcpy(name, m_vecAvailRomList[fGameSelect+tmp].c_str());
				n = name[0];
												 				
				if(n < c)
				{
					fCursorPos = tmp;

					if( fCursorPos < iWindowMiddle )
					{
						// clamp cursor position
						bClampCursor = true;
				
						// backup window pos
						fGameSelect -= (oCursorPos - fCursorPos);
				
						// clamp game window range (low)
						if(fGameSelect < 0)
						{
							// clamp to start
							fGameSelect = 0;
					
							// backup cursor pos after all!
							bClampCursor = false;
					
							// clamp cursor to end
							if( fCursorPos < 0 )
								fCursorPos = 0;
						}
					}
					
					// check for cursor clamp
					if( bClampCursor )
					fCursorPos = iWindowMiddle;	

					 
					break;
				}

			}

			Draw();
			
        }
		
		
		
        if(k & KEY_LSTICK_UP)
        {
            if(selected > 0) selected -= 1;
            else selected = optionsvec.size() - 1;
            Draw();
        }
        else if(k & KEY_LSTICK_DOWN)
        {
           
			if(selected < optionsvec.size() - 1) selected += 1;
            else selected = 0;
            Draw();
        }
        
        else if(k & KEY_X)
        {
             
            Draw();
        }
        else if(k & KEY_Y)
        {
             
            Draw();
        }
        else if(k & KEY_A)
        {			 	

			//switch (selected)
			//{ 
			//	case 0:
					int gameIndex = mapRoms[currentGame]; 
					SDL_SetRenderDrawColor(sdl_render, 0, 0, 0, 255);
					SDL_RenderClear(sdl_render);
					 
					options.ui_orientation = drivers[gameIndex]->flags & ORIENTATION_MASK;

					if( options.ui_orientation & ORIENTATION_SWAP_XY )
					{
				   
						if( (options.ui_orientation & ROT180) == ORIENTATION_FLIP_X ||
							(options.ui_orientation & ROT180) == ORIENTATION_FLIP_Y)
						options.ui_orientation ^= ROT180;
					}
										 				
					run_game(gameIndex);					 
										 
					SDL_SetRenderDrawColor(sdl_render, 255, 255, 255, 255);		
					SDL_RenderClear(sdl_render);					
					//break;
				//case 1:
				
					
				
					//break;
					
			//}
			
			Draw();
        }
        else if(k & KEY_B)
        {            
            Draw();
        }
        else if(k & KEY_PLUS || k & KEY_MINUS) Exit();
    }
	
 
	

    void Init()
    {
		MenuState = GAMESELECT;
		
		fGameSelect	= 0.0f;
		iGameSelect	= 0;
		fCursorPos = 0.0f;
		iCursorPos = 0;
	
        romfsInit();
        ColorSetId id;
        setsysInitialize();
        setsysGetColorSetId(&id);
        Back = "romfs:/Graphics/mamelogo-nx.png";
        txtcolor = { 255, 255, 255, 255 };
        setsysExit();
        SDL_Init(SDL_INIT_EVERYTHING);
        SDL_CreateWindowAndRenderer(1280, 720, 0, &sdl_wnd, &sdl_render);
        sdl_surf = SDL_GetWindowSurface(sdl_wnd);
        SDL_SetRenderDrawBlendMode(sdl_render, SDL_BLENDMODE_BLEND);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
        IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
        TTF_Init();
        SDL_SetRenderDrawColor(sdl_render, 255, 255, 255, 255);
       
        fntLarge = TTF_OpenFont("romfs:/Fonts/NintendoStandard.ttf", 25);
        fntMedium = TTF_OpenFont("romfs:/Fonts/NintendoStandard.ttf", 20);
        fntSmall = TTF_OpenFont("romfs:/Fonts/NintendoStandard.ttf", 10);
		
		menuFntLarge = fntLarge;
		
        sdls_Back = InitSurface(Back);
        sdlt_Back = InitTexture(sdls_Back);
        optionsvec.push_back("Playable ROM(s)");
        optionsvec.push_back("Options");
		optionsvec.push_back("About mame-nx");
		optionsvec.push_back("Quit");
         
		sprintf(RomCountText,"%d/%d Games Found",romList.AvRoms(), romList.totalMAMEGames);	 
		
		
		iNumGames =	romList.AvRoms();

		if (iNumGames <	GAMESEL_MaxWindowList)
		{
			iMaxWindowList = iNumGames;
			iWindowMiddle	 = iNumGames/2;
		}
		else
		{
			iMaxWindowList = GAMESEL_MaxWindowList;
			iWindowMiddle	 = GAMESEL_WindowMiddle;
		}
	
		
		selRed = 255;
		selGreen = 0;
		selBlue = 0;
		selAlpha = 255;		
		
		menu_render = sdl_render;
		
        Draw();
    }
}