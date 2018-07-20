#pragma once

#include <stdint.h>

#define DISPLAY_WIDTH (320)
#define DISPLAY_HEIGHT (240)

void display_init(void);
void display_poweroff(void);
void display_clear(uint16_t color);
void display_write_all(uint16_t* buf);
void display_write_rect(uint16_t* buf, short x, short y, short width, short height);
void display_drain(void);
