 
#include <iostream>
#include <string>
#include <vector>
#include <functional>
using namespace std;

#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h> 
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include "nx_romlist.h"
 
extern std::vector<std::string> m_vecAvailRomList;
extern CRomList romList;	
 
int dr = 0;
int dg = 0;
int db = 0;
		
namespace UI
{
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

    static vector<string> options;
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

	
        SDL_RenderClear(sdl_render);
        DrawBack(sdls_Back, sdlt_Back);
     
        int ox = Opt1X;
        int oy = Opt1Y;
        for(int i = 0; i < options.size(); i++)
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
				
                DrawText(fntLarge, ox, oy, { 140, 255, 255, 255 }, options[i].c_str());
                if(i == 0)
                {
                    int fx = 450;
                    int fy = 235;
                    for(int j = 0; j < m_vecAvailRomList.size(); j++)
                    {
                        if(j == fselected)
                        {
                            DrawText(fntLarge, fx, fy, { selRed, selGreen, selBlue, 255 }, m_vecAvailRomList[j].c_str());
                        }
                        else DrawText(fntLarge, fx, fy, txtcolor, m_vecAvailRomList[j].c_str());
                        fy += 35;
                    }
                }
                else if(i == 1)
                {
                    DrawText(fntMedium, 475, 200, txtcolor, "NXPlay - Multimedia player, by XorTroll");
                    DrawText(fntMedium, 500, 250, txtcolor, "Move between pages using L-stick.");
                    DrawText(fntMedium, 500, 275, txtcolor, "Move between audio files using R-stick on the player page.");
                    DrawText(fntMedium, 500, 300, txtcolor, "Start playing an audio or play another one pressing A.");
                    DrawText(fntMedium, 500, 325, txtcolor, "Change volume up/down using D-pad.");
                    DrawText(fntMedium, 500, 350, txtcolor, "Pause/resume playing audio pressing Y.");
                    DrawText(fntMedium, 500, 375, txtcolor, "Restart audio playback pressing X.");
                    DrawText(fntMedium, 500, 400, txtcolor, "Stop audio playback pressing B.");
                    DrawText(fntMedium, 500, 425, txtcolor, "Exit app pressing Plus or Minus.");
                    DrawText(fntMedium, 475, 475, txtcolor, "Supported types: MP3, WAV, OGG, FLAC, MOD audios");
                    DrawText(fntMedium, 475, 500, txtcolor, "Video support coming soon!");
                }
            }
            else DrawText(fntLarge, ox, oy, txtcolor, options[i].c_str());
            oy += 50;
        }
        DrawText(fntLarge, TitleX, 672, txtcolor, RomCountText);
        SDL_RenderPresent(sdl_render);
    }
    
    void Loop()
    {
		Draw();
    }

    void Init()
    {
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
        sdls_Back = InitSurface(Back);
        sdlt_Back = InitTexture(sdls_Back);
        options.push_back("Playable ROM(s)");
        options.push_back("Options");
		options.push_back("About mame-nx");
		options.push_back("Quit");
         
		sprintf(RomCountText,"%d/%d Games Found",romList.AvRoms(), romList.totalMAMEGames);	 

		
		selRed = 255;
		selGreen = 0;
		selBlue = 0;
		selAlpha = 255;		
		
        Draw();
    }
}