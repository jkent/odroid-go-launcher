#pragma once

#include "graphics.h"
#include "tf.h"

#include <stdint.h>

struct menu_t;

struct menu_item_t {
    char *label;
};

struct menu_t *menu_new(struct gbuf_t *fb, short width, short height);
void menu_free(struct menu_t *menu);
void menu_show(struct menu_t *menu);
void menu_hide(struct menu_t *menu);
void menu_insert(struct menu_t *menu, int offset, struct menu_item_t *item);
void menu_append(struct menu_t *menu, struct menu_item_t *item);
void menu_remove(struct menu_t *menu, int index);
