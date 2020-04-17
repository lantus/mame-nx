 
#include <iostream>
#include <string>
#include <vector>
#include <map> 
#include <functional>
using namespace std;

#include <switch.h>
#include "nx_romlist.h"
#include "gfx.hpp"


extern "C" {
#include "osd_cpu.h"
#include "driver.h"
#include "mame.h" 
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
	char CPUSpeed[60];
    
    static string Back; 
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
 
    void Exit()
    {
	   Gfx::exit();
       romfsExit();
       exit(0);
    }

    void Draw()
    {		
		char currentName[120];
		int	iTempGameSel;
		int	iGameidx;
	
		Gfx::drawBgImage();

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
				
				Gfx::drawText(ox,oy, optionsvec[i].c_str() , { selBlue, selGreen, selRed, 255 } , 20);	 
                
				if(i == 0)
                {
                    int fx = 450;
                    int fy = 235;
 
					// draw	game list entries

					iGameSelect	= fGameSelect;
					iCursorPos = fCursorPos;
					iTempGameSel = iGameSelect;

					if (iNumGames == 0)	
					{
						Gfx::drawText( fx, 202, "No Roms Found", { 255, 255, 255, 255 }, 20);	 
						 
					}
					for	(iGameidx=0; iGameidx<iMaxWindowList;	iGameidx++)
					{
						
						sprintf( currentName, "");
						sprintf( currentName, "%s", m_vecAvailRomList[iTempGameSel++].c_str() );


						if (iGameidx==iCursorPos){
							 							 
							Gfx::drawText(fx,fy + (28*iGameidx) , currentName,  { selRed, selGreen, selBlue, 255 }, 20);
							currentGame = currentName; 
						}
						else
						{							 
							Gfx::drawText(fx,fy + (28*iGameidx) , currentName,  { 255, 255, 255, 255 }, 20);
						}

					}


                }
                else if(i == 1)
                {
 
					Gfx::drawText(450, 400,"No Options Yet", { 255, 255, 255, 255 }, 20);
					 
                     
                }
				else if(i == 2)
                {
                   Gfx::drawText(450, 400,"Release 2.3 Ported by MVG in 2020", { 255, 255, 255, 255 }, 20);
				    				  
                }
				else if(i == 3)
                {
                   
                }
            }
            else{
				 			
				Gfx::drawText(ox,oy, optionsvec[i].c_str(),  { 255, 255, 255, 255 }, 20);
			}
            oy += 50;
        }
        	 
		Gfx::drawText(TitleX,672, RomCountText,  { 255, 255, 255, 255 }, 20);
		Gfx::drawText(TitleX+450,20, CPUSpeed,  { 255, 255, 255, 255 }, 20);
       
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
 
					Gfx::clear({ 0, 0, 0, 0 });	

					options.ui_orientation = drivers[gameIndex]->flags & ORIENTATION_MASK;

					if( options.ui_orientation & ORIENTATION_SWAP_XY )
					{
				   
						if( (options.ui_orientation & ROT180) == ORIENTATION_FLIP_X ||
							(options.ui_orientation & ROT180) == ORIENTATION_FLIP_Y)
						options.ui_orientation ^= ROT180;
					}
										 				
					run_game(gameIndex);	
					 				
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
		 
		uint32_t cpuSpeed;
		MenuState = GAMESELECT;
		
		fGameSelect	= 0.0f;
		iGameSelect	= 0;
		fCursorPos = 0.0f;
		iCursorPos = 0;
	
        romfsInit();
 
	 
		Gfx::init();
		
        //ColorSetId id;
        //setsysInitialize();
        //setsysGetColorSetId(&id);

        setsysExit();
		
		//Gfx::drawBgImage();
		Gfx::flush();
		
        optionsvec.push_back("Playable ROM(s)");
        optionsvec.push_back("Options");
		optionsvec.push_back("About mame-nx");
		optionsvec.push_back("Quit");
         
		//pcvGetClockRate(PcvModule_Cpu , &cpuSpeed);
		 
		//1220000000
		
		sprintf(RomCountText,"%d/%d Games Found",romList.AvRoms(), romList.totalMAMEGames);	 
		//sprintf(CPUSpeed, "Switch Frequency %ld Hz", (cpuSpeed));
		
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
		
	 
		
        Draw();
    }
}