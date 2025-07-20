#include "text.h"

#include "entombed_font_gfx.h"

void txt_init(int bgnr, int sbb, int prio)
{
	//memcpy16(pal_bg_mem + 240, pal_font, pal_font_size/2); //Palbank 15
	LZ77UnCompVram(entombed_font_gfx, tile_mem[2]);

	//Copy palbank 15 to palbank 14 but with faded values. This way
	//we can have faded text on the level selector for levels that
	//have been completed and use txt_puts_faded.
	clr_fade_fast(pal_bg_bank[15], CLR_BLACK, pal_bg_bank[14], 16, 15);

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
