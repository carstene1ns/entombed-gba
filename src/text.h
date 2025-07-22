#ifndef TEXT_H
#define TEXT_H

#include <tonc.h>

// --- Tilemap (SE) text  ---
void txt_init(int bgnr, int sbb);

inline void txt_putc(int x, int y, int c)
{
	tte_set_pos(x, y);
	tte_putc(c);
}

inline void txt_fillc(int x, int y, int c, int amt)
{
	tte_set_pos(x, y);
	for (int i = 0; i < amt; i++)
	{
		tte_putc(c);
	}
}

inline void txt_puts(int x, int y, const char* str)
{
	tte_write_ex(x, y, str, NULL);
}

inline void txt_puts_faded(int x, int y, const char* str)
{
	tte_set_pos(x, y);
	tte_write("#{cx:0xE000}"); // select palette 14
	tte_write(str);
	tte_write("#{cx:0}");
}

inline void txt_write(const char* str)
{
	tte_write(str);
}

inline void txt_default_margins()
{
	// full screen
	tte_set_margins(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

inline void txt_margin_left(int ml)
{
	tte_get_context()->marginLeft = ml;
}

inline void txt_clear_rect(int x, int y, int dx, int dy)
{
	tte_erase_rect(x, y, x + dx, y + dy);
}

inline void txt_clear_screen()
{
	tte_erase_screen();
}

inline void txt_clear_bg()
{
	// assuming a 32tx32t background
	tte_erase_rect(0, 0, 32 * 8, 32 * 8);
}

#endif
