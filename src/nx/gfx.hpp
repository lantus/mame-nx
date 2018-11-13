
#pragma once
 
#include <switch.h>
#include <string>
#include <fstream>
#include <algorithm>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <iostream>
 
 
using namespace std;
 
namespace Gfx
{
    struct RGBA
    {
        u8 R;
        u8 G;
        u8 B;
        u8 A;
    };
    
    void init();
    void drawPixel(u32 X, u32 Y, RGBA Color);
    void drawRectangle(u32 X, u32 Y, u32 Width, u32 Height, RGBA Color);
    void drawText(u32 X, u32 Y, string Text, RGBA Color, u32 Size);   
    void flush();
    void clear(RGBA Color);
    void exit();
	void drawBgImage();

    static u8 *Framebuffer;
    static u32 FrameWidth;
    static u32 FrameHeight;
}