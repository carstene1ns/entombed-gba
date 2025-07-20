
#include <maxmod.h>
#include <cstring>
#include "fader.h"

CFader::CFader(COLOR* palette) :
	m_palette(palette)
{
}

void CFader::set_palette(COLOR* palette)
{
	m_palette = palette;
}

void CFader::apply(FadeType type, int frames)
{
	FIXED value = 0;
	FIXED amount = float2fx(32.f / frames);

	if (type == FadeType::IN)
	{
		value = int2fx(32);
		amount = -amount;
	}

	for (int i = 0; i < frames; i++)
	{
		value += amount;
		clr_fade_fast(m_palette, CLR_BLACK, pal_bg_mem, 512, fx2int(value));

		VBlankIntrWait();
		mmFrame();
	}

	// fixup colors, if needed
	if (type == FadeType::IN)
	{
		restore();
	}
	else
	{
		clear();
	}
}

void CFader::clear()
{
	memset(pal_bg_mem, '\0', 512 * sizeof(COLOR));
}

void CFader::restore()
{
	memcpy16(pal_bg_mem, m_palette, 512);
}
