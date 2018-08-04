#pragma once

#include "graphics.h"
#include "tf.h"

struct menu_t;
typedef void (*menu_callback_t)(struct menu_t *menu, int index, void *arg);

struct menu_t *menu_new(struct gbuf_t *fb, short width, short height);
void menu_free(struct menu_t *menu);
void menu_showmodal(struct menu_t *menu);
void menu_draw(struct menu_t *menu);
void menu_insert_text(struct menu_t *menu, int offset, const char *label, menu_callback_t on_select, void *arg);
void menu_append_text(struct menu_t *menu, const char *label, menu_callback_t on_select, void *arg);
void menu_insert_list(struct menu_t *menu, int index, const char **list, int value, menu_callback_t on_select, void *arg);
void menu_append_list(struct menu_t *menu, const char **list, int value, menu_callback_t on_select, void *arg);
void menu_insert_divider(struct menu_t *menu, int index);
void menu_append_divider(struct menu_t *menu);
void menu_insert_title(struct menu_t *menu, int index, const char *label);
void menu_append_title(struct menu_t *menu, const char *label);
void menu_remove(struct menu_t *menu, int index);
void menu_dismiss(struct menu_t *menu);
int menu_get_value(struct menu_t *menu, int index);
void menu_set_value(struct menu_t *menu, int index, int value);
int menu_get_list_size(struct menu_t *menu, int index);
void menu_list_cycle(struct menu_t *menu, int index, void *arg);
