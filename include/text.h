#ifndef TEXT_H
#define TEXT_H


#include <tonc.h>
#include "gfx/entombed_font.h"

#ifdef __cplusplus
extern "C" {
#endif

// === PROTOTYPES =====================================================

// --- Tilemap text  ---
void txt_init(int bgnr, int sbb, int prio);
void txt_putc(int x, int y, int c);
void txt_puts(int x, int y, const char *str);
void txt_puts_faded(int x, int y, const char *str);
void txt_clrs(int x, int y, const char *str);

#ifdef __cplusplus
}	   // extern "C"
#endif

#endif

