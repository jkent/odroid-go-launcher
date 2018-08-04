#pragma once

#include "graphics.h"
#include "tf.h"

struct menu_t;
typedef void (*menu_callback_t)(struct menu_t *menu, int offset, void *arg);

struct menu_t *menu_new(struct gbuf_t *fb, short width, short height);
void menu_free(struct menu_t *menu);
void menu_showmodal(struct menu_t *menu);
void menu_insert(struct menu_t *menu, int offset, const char *label, menu_callback_t on_select, void *arg);
void menu_append(struct menu_t *menu, const char *label, menu_callback_t on_select, void *arg);
void menu_remove(struct menu_t *menu, int index);
void menu_dismiss(struct menu_t *menu);
const char *menu_get_label(struct menu_t *menu, int offset);
void menu_change_label(struct menu_t *menu, int offset, const char *label);
