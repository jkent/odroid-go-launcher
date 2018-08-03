#pragma once

#include "graphics.h"
#include "tf.h"

#include <stdint.h>

struct menu_t;


struct menu_t *menu_new(struct gbuf_t *fb, short width, short height);
void menu_free(struct menu_t *menu);
void menu_showmodal(struct menu_t *menu);
void menu_insert(struct menu_t *menu, int offset, const char *label, void (*on_select)(void *arg), void *arg);
void menu_append(struct menu_t *menu, const char *label, void (*on_select)(void *arg), void *arg);
void menu_remove(struct menu_t *menu, int index);
