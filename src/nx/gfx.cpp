#include "Gfx.hpp"

extern "C" {
	int menu_create_display();
	void menu_render();
	void menu_flush();	 
	void menu_cleanup();	 
	unsigned char menupixels[0x384000];
	unsigned char *bg;
}

typedef union
{
    u32 abgr;
    struct
    {
        u8 r,g,b,a;
    };
} color_t;

typedef struct
{
    u8 width, height;
    int8_t posX, posY, advance;
    const u8* data;
} glyph_t;

typedef struct
{
    u8 magic[4];
    int version;
    u16 npages;
    u8 height;
    u8 baseline;
} ffnt_header_t;

typedef struct
{
    u32 size, offset;
} ffnt_pageentry_t;

typedef struct
{
    u32 pos[0x100];
    u8 widths[0x100];
    u8 heights[0x100];
    int8_t advances[0x100];
    int8_t posX[0x100];
    int8_t posY[0x100];
} ffnt_pagehdr_t;

typedef struct
{
    ffnt_pagehdr_t hdr;
    u8 data[];
} ffnt_page_t;

extern const ffnt_header_t interuiregular20_nxfnt;
 

u8 bcolor(u32 src, u32 dst, u8 alpha)
{
    u8 one_minus_alpha = (u8)255 - alpha;
    return (dst * alpha + src * one_minus_alpha) / (u8)255;
}

void pix(u32 x, u32 y, u8 r, u8 g, u8 b, u8 a)
{
    if(x >= 1280 || y >= 720) return;
    u32 off = (y * Gfx::FrameWidth + x) * 4;
    Gfx::Framebuffer[off] = bcolor(Gfx::Framebuffer[off], r, a);
    Gfx::Framebuffer[off + 1] = bcolor(Gfx::Framebuffer[off + 1], g, a);
    Gfx::Framebuffer[off + 2] = bcolor(Gfx::Framebuffer[off + 2], b, a);
    Gfx::Framebuffer[off + 3] = 0xff;
}

const ffnt_page_t* FontGetPage(const ffnt_header_t* font, u32 page_id)
{
    if(page_id >= font->npages) return NULL;
    ffnt_pageentry_t* ent = &((ffnt_pageentry_t*)(font + 1))[page_id];
    if(ent->size == 0) return NULL;
    return (const ffnt_page_t*)((const u8*)font + ent->offset);
}

bool FontLoadGlyph(glyph_t* glyph, const ffnt_header_t* font, u32 codepoint)
{
    const ffnt_page_t* page = FontGetPage(font, codepoint >> 8);
    if(!page) return false;
    codepoint &= 0xFF;
    u32 off = page->hdr.pos[codepoint];
    if(off == ~(u32)0) return false;
    glyph->width = page->hdr.widths[codepoint];
    glyph->height = page->hdr.heights[codepoint];
    glyph->advance = page->hdr.advances[codepoint];
    glyph->posX = page->hdr.posX[codepoint];
    glyph->posY = page->hdr.posY[codepoint];
    glyph->data = &page->data[off];
    return true;
}

void DrawGlyph(u32 x, u32 y, color_t clr, const glyph_t* glyph)
{
    u32 i, j;
    const u8* data = glyph->data;
    x += glyph->posX;
    y += glyph->posY;
    for(j = 0; j < glyph->height; j ++)
    {
        for(i = 0; i < glyph->width; i ++)
        {
            clr.a = *data++;
            if(!clr.a) continue;
            pix(x + i, y + j, clr.r, clr.g, clr.b, clr.a);
        }
    }
}

u8 DecodeByte(const char** ptr)
{
    u8 c = (u8)**ptr;
    *ptr += 1;
    return c;
}

int8_t DecodeUTF8Cont(const char** ptr)
{
    int c = DecodeByte(ptr);
    return ((c & 0xC0) == 0x80) ? (c & 0x3F) : -1;
}

u32 DecodeUTF8(const char** ptr)
{
    u32 r;
    u8 c;
    int8_t c1, c2, c3;

    c = DecodeByte(ptr);
    if((c & 0x80) == 0) return c;
    if((c & 0xE0) == 0xC0)
    {
        c1 = DecodeUTF8Cont(ptr);
        if(c1 >= 0)
        {
            r = ((c & 0x1F) << 6) | c1;
            if(r >= 0x80) return r;
        }
    }
    else if((c & 0xF0) == 0xE0)
    {
        c1 = DecodeUTF8Cont(ptr);
        if(c1 >= 0)
        {
            c2 = DecodeUTF8Cont(ptr);
            if(c2 >= 0)
            {
                r = ((c & 0x0F) << 12) | (c1 << 6) | c2;
                if(r >= 0x800 && (r < 0xD800 || r >= 0xE000)) return r;
            }
        }
    }
    else if((c & 0xF8) == 0xF0)
    {
        c1 = DecodeUTF8Cont(ptr);
        if(c1 >= 0)
        {
            c2 = DecodeUTF8Cont(ptr);
            if(c2 >= 0)
            {
                c3 = DecodeUTF8Cont(ptr);
                if(c3 >= 0)
                {
                    r = ((c & 0x07) << 18) | (c1 << 12) | (c2 << 6) | c3;
                    if (r >= 0x10000 && r < 0x110000) return r;
                }
            }
        }
    }
    return 0xFFFD;
}

void DrawText_(const ffnt_header_t* font, u32 x, u32 y, color_t clr, const char* text, u32 max_width)
{
    y += font->baseline;
    u32 origX = x;
    while(*text)
    {
        if(max_width && x-origX >= max_width) break;

        glyph_t glyph;
        u32 codepoint = DecodeUTF8(&text);
        if(codepoint == '\n')
        {
            if(max_width) break;
            x = origX;
            y += font->height;
            continue;
        }

        if(!FontLoadGlyph(&glyph, font, codepoint))
        {
            if(!FontLoadGlyph(&glyph, font, '?')) continue;
        }
        DrawGlyph(x, y, clr, &glyph);
        x += glyph.advance;
    } 
}

void Gfx::init()
{ 	
	menu_create_display();
	Gfx::Framebuffer = menupixels;
	Gfx::FrameWidth = 1280;
	Gfx::FrameHeight = 720;     
}

void Gfx::drawPixel(u32 X, u32 Y, Gfx::RGBA Color)
{
  
}

void Gfx::drawRectangle(u32 X, u32 Y, u32 Width, u32 Height, Gfx::RGBA Color)
{
    for(u32 i = 0; i < Width; i++) for(u32 j = 0; j < Height; j++) Gfx::drawPixel(X + i, Y + j, Color);
}


void Gfx::drawBgImage()
{
	u8 *raw = bg;
	for(u32 i = 0; i < 1280; i++)
	{
		for(u32 j = 0; j < 720; j++)
		{
			u32 pos = ((j * 1280) + i) * 3;
			pix(  i,  j, raw[pos], raw[pos + 1], raw[pos + 2], 255);
		}
	}
}

void Gfx::drawText(u32 X, u32 Y, string Text, Gfx::RGBA Color, u32 Size)
{
    color_t clr;
    clr.r = Color.R;
    clr.g = Color.G;
    clr.b = Color.B;
    clr.a = Color.A;
    const ffnt_header_t* font = &interuiregular20_nxfnt;
   
	 
    DrawText_(font, X, Y, clr, Text.c_str(), 0);
}


void Gfx::flush()
{
	menu_flush();
}

void Gfx::clear(Gfx::RGBA Color)
{
    
}

void Gfx::exit()
{
	menu_cleanup();
    //gfxExit();
}