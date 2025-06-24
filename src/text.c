#include "text.h"

#include "entombed_font_gfx.h"

void txt_init(int bgnr, int sbb, int prio)
{
	//memcpy16(pal_bg_mem + 240, pal_font, pal_font_size/2); //Palbank 15
	LZ77UnCompVram(entombed_font_gfx, tile_mem[2]);

	//Copy palbank 15 to palbank 14 but with faded values. This way
	//we can have faded text on the level selector for levels that
	//have been completed and use txt_puts_faded.
	u16 *Src = pal_bg_mem + 240; //Palbank 15
	u16 *Dest = pal_bg_mem + 224; //Palbank 14

	u16 color;
	s16 r, g, b;
	int n;

	for (n = 240; n < 256; n++)
	{

		color = *(Src++);
		r = (color & 0x1f);				// get component
		g = (color >> 5 & 0x1f);
		b = (color >> 10 & 0x1f);

		//Fade the color, divide by 2
		r = r >> 1;
		g = g >> 1;
		b = b >> 1;

		color = (r) | (g << 5) | (b << 10);
		*(Dest++) = color;
	}

	REG_BGCNT[bgnr] = BG_CBB(2) | BG_SBB(sbb) | BG_PRIO(prio);
}

//!	Print character \a c on a tilemap at pixel (x, y) with base SE \a se0
/*!	\param x	x-coordinate in pixels (rounded down to 8s).
*	\param y	y-coordinate in pixels (rounded down to 8s).
*	\param c	Character to print.
*/
void txt_putc(int x, int y, int c)
{
	if (c == '\n')
		return;

	SCR_ENTRY *dst = &se_mem[26][(y >> 3) * 32 + (x >> 3)];
	*dst = (c - 32) + SE_PALBANK(15);
}

//!	Print string \a str on a tilemap at pixel (x, y) with base SE \a se0
/*!	\param x	x-coordinate in pixels (rounded down to 8s).
*	\param y	y-coordinate in pixels (rounded down to 8s).
*	\param str	String to print.
*/
void txt_puts(int x, int y, const char* str)
{
	int c;
	SCR_ENTRY *dst = &se_mem[26][(y >> 3) * 32 + (x >> 3)];

	x = 0;
	while ((c = *str++) != 0)
	{
		if (c == '\n')	// line break
		{
			dst += (x & ~31) + 32;
			x = 0;
		}
		else
			dst[x++] = (c - 32) + SE_PALBANK(15);
	}
}

void txt_puts_faded(int x, int y, const char* str)
{
	int c;
	SCR_ENTRY *dst = &se_mem[26][(y >> 3) * 32 + (x >> 3)];

	x = 0;
	while ((c = *str++) != 0)
	{
		if (c == '\n')	// line break
		{
			dst += (x & ~31) + 32;
			x = 0;
		}
		else
			dst[x++] = (c - 32) + SE_PALBANK(14);
	}
}

//!	Clear string \a str from a tilemap at pixel (x, y) with SE \a se0
/*!	\param x	x-coordinate in pixels (rounded down to 8s).
*	\param y	y-coordinate in pixels (rounded down to 8s).
*	\param str	String indicating which area is used.
*/
void txt_clrs(int x, int y, const char* str)
{
	int c;
	SCR_ENTRY *dst = &se_mem[26][(y >> 3) * 32 + (x >> 3)];

	x = 0;
	while ((c = *str++) != 0)
	{
		if (c == '\n')	// line break
		{
			dst += (x & ~31) + 32;
			x = 0;
		}
		else
			dst[x++] = SE_PALBANK(15);
	}
}
