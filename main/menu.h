#pragma once

#include "graphics.h"
#include "tf.h"

#include <stdint.h>

struct menu_t;

struct menu_t *menu_init(struct gbuf_t *fb, short width, short height);
void menu_show(struct menu_t *menu);
void menu_hide(struct menu_t *menu);
