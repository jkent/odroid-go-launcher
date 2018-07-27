#pragma once

#include "gbuf.h"

#include <stdint.h>

#define DISPLAY_WIDTH (320)
#define DISPLAY_HEIGHT (240)

void display_init(void);
void display_poweroff(void);
void display_clear(uint16_t color);
void display_draw(struct gbuf *fb);
void display_draw_rect(struct gbuf *g, short x, short y);
void display_drain(void);
