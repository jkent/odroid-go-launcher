#pragma once

#include "graphics.h"

#include <stdint.h>

#define DISPLAY_WIDTH (320)
#define DISPLAY_HEIGHT (240)

struct gbuf_t *display_init(void);
void display_poweroff(void);
void display_clear(uint16_t color);
void display_update(void);
void display_update_rect(struct rect_t rect);
void display_drain(void);
