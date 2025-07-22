#include "text.h"

// gfx data
#include "pal_font.h"
#include "entombed_font_gfx.h"

static TFont entombedFont =
{
	.data = NULL, // unused
	.widths = NULL, // unused
	.heights = NULL, // unused
	.charOffset = 32,
	.charCount = 112, // 96 TXT + 4 HUD + 10 empty Elements
	.charW = 8,
	.charH = 8,
	.cellW = 8,
	.cellH = 8,
	.cellSize = 8 * 8, // unused
	.bpp = 4,  // unused
	.extra = 0  // unused
};

void txt_init(int bgnr, int sbb)
{
	tte_init_se(bgnr, BG_CBB(2) | BG_SBB(sbb) | BG_PRIO(0), SE_ID(0) | SE_PALBANK(14),
	            CLR_BLACK, 0, NULL, se_drawg_w8h8);

	//memcpy16(pal_bg_bank[15], pal_font, pal_font_size/2);
	LZ77UnCompVram(entombed_font_gfx, tile_mem[2]);
	tte_set_font(&entombedFont); // Attach custom font
	tte_set_special(SE_ID(0) | SE_PALBANK(15)); // Set special to tile 0, palette 15
	tte_set_paper(112); // set background to last (empty) tile

	//Copy palbank 15 to palbank 14 but with faded values. This way
	//we can have faded text on the level selector for levels that
	//have been completed and use txt_puts_faded.
	clr_fade_fast(pal_bg_bank[15], CLR_BLACK, pal_bg_bank[14], 16, 15);
}
