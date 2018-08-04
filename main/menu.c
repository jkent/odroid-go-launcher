#include "menu.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "../components/hardware/display.h"
#include "../components/hardware/keypad.h"

#include "graphics.h"
#include "OpenSans_Regular_11X12.h"
#include "tf.h"

#include <string.h>

struct menu_item_t;

struct menu_t {
    struct gbuf_t *g;
    struct gbuf_t *g_saved;
    struct rect_t rect;
    short border_width;
    struct tf_t *tf_text;
    size_t item_count;
    struct menu_item_t *items;
    int item_selected;
    int item_first;
    int item_displayed;
    short item_height;
    short yshift;
    bool dismissed;
};

enum menu_item_type_t {
    MENU_ITEM_TYPE_TEXT,
    MENU_ITEM_TYPE_LIST,
};

struct menu_item_t {
    enum menu_item_type_t type;
    union {
        const char *label;
        const char **list;
    };
    int value;
    size_t list_size;
    menu_callback_t on_select;
    void *arg;
};


struct menu_t *menu_new(struct gbuf_t *g, short width, short height)
{
    struct menu_t *menu = calloc(1, sizeof(struct menu_t));
    assert(menu != NULL);

    menu->g = g;
    menu->g_saved = gbuf_new(width, height, 2, BIG_ENDIAN);
    menu->rect.x = g->width/2 - width/2;
    menu->rect.y = g->height/2 - height/2;
    menu->rect.width = width;
    menu->rect.height = height;
    menu->border_width = 2;

    menu->tf_text = tf_new(&font_OpenSans_Regular_11X12, width - 2 * menu->border_width, TF_ELIDE);
    menu->tf_text->clip.x = menu->rect.x + menu->border_width;
    menu->tf_text->clip.y = menu->rect.y + menu->border_width;
    menu->tf_text->clip.width = menu->rect.width - 2 * menu->border_width;
    menu->tf_text->clip.height = menu->rect.height - 2 * menu->border_width;

    menu->item_height = menu->tf_text->font->height + 2;
    menu->item_displayed = (menu->rect.height - 2 * menu->border_width + menu->item_height - 1) / menu->item_height;

    return menu;
}

void menu_free(struct menu_t *menu)
{
    assert(menu != NULL);

    while (menu->item_count > 0) {
        menu_remove(menu, -1);
    }

    tf_free(menu->tf_text);

    gbuf_free(menu->g_saved);

    free(menu);
}

void menu_draw(struct menu_t *menu)
{
    draw_rectangle(menu->g, menu->rect, DRAW_TYPE_FILL, 0x0000);
    draw_rectangle(menu->g, menu->rect, DRAW_TYPE_OUTLINE, 0xFFFF);

    for (unsigned int row = 0; row < menu->item_displayed; row++) {
        if (row >= menu->item_count) {
            break;
        }

        struct menu_item_t *item = &menu->items[menu->item_first + row];

        struct rect_t r = {
            .x = menu->rect.x + menu->border_width,
            .y = menu->rect.y + menu->border_width - menu->yshift + row * menu->item_height,
            .width = menu->rect.width - 2 * menu->border_width,
            .height = menu->item_height,
        };
        struct point_t p = {
            .x = r.x,
            .y = r.y + 2,
        };
        if (menu->item_first + row == menu->item_selected) {
            draw_rectangle(menu->g, r, DRAW_TYPE_FILL, 0x001F);
        }

        const char *s;
        if (item->type == MENU_ITEM_TYPE_LIST) {
            s = item->list[item->value];
        } else {
            s = item->label;
        }

        tf_draw_str(menu->g, menu->tf_text, s, p);
    }

    display_update_rect(menu->rect);
}

static void menu_hide(struct menu_t *menu)
{
    struct rect_t src_rect = {0, 0, menu->rect.width, menu->rect.height};

    blit(menu->g, menu->rect, menu->g_saved, src_rect);
    display_update_rect(menu->rect);
}

void menu_showmodal(struct menu_t *menu)
{
    struct rect_t dst_rect = {0, 0, menu->rect.width, menu->rect.height};

    menu->dismissed = false;

    blit(menu->g_saved, dst_rect, menu->g, menu->rect);
    menu_draw(menu);

    uint16_t keys = 0, changes = 0, pressed = 0;
    do {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        keys = keypad_debounce(keypad_sample(), &changes);
        pressed = keys & changes;

        if (pressed & KEYPAD_A) {
            if (menu->items[menu->item_selected].on_select != NULL) {
                menu->items[menu->item_selected].on_select(menu, menu->item_selected, menu->items[menu->item_selected].arg);
            }
        } else if (pressed & KEYPAD_UP) {
            if (menu->item_selected > 0) {
                menu->item_selected -= 1;

                if (menu->item_first >= menu->item_selected) {
                    menu->yshift = 0;
                }
               if (menu->item_first > menu->item_selected) {
                    menu->item_first -= 1;
                }
                menu_draw(menu);
            }
        } else if (pressed & KEYPAD_DOWN) {
            if (menu->item_selected < menu->item_count - 1) {
                menu->item_selected += 1;

                if (menu->item_selected - menu->item_first >= menu->item_displayed - 1) {
                    menu->yshift = menu->item_height - (menu->rect.height - menu->border_width) % menu->item_height;
                    if (menu->yshift >= menu->item_height) {
                        menu->yshift = 0;
                    }
                }
                if (menu->item_selected - menu->item_first >= menu->item_displayed) {
                    menu->item_first += 1;
                }

                menu_draw(menu);
            }
        }
    } while (!(pressed & KEYPAD_B) && !menu->dismissed);

    menu_hide(menu);
}

static struct menu_item_t *menu_insert_new(struct menu_t *menu, int index)
{
    if (index < 0) {
        index = menu->item_count + index;
    }
    assert(index <= menu->item_count);

    if (menu->item_count == 0) {
        menu->items = malloc(sizeof(struct menu_item_t));
        assert(menu->items != NULL);
    } else {
        menu->items = realloc(menu->items, sizeof(struct menu_item_t) * (menu->item_count + 1));
        assert(menu->items != NULL);
        if (index < menu->item_count) {
            memmove(&menu->items[index + 1], &menu->items[index], sizeof(struct menu_item_t) * (menu->item_count - index));
        }
    }

    menu->item_count += 1;

    return &menu->items[index];
}

void menu_insert_text(struct menu_t *menu, int index, const char *label, menu_callback_t on_select, void *arg)
{
    struct menu_item_t *item = menu_insert_new(menu, index);
    item->type = MENU_ITEM_TYPE_TEXT;
    item->label = label;
    item->on_select = on_select;
    item->arg = arg;
}

void menu_append_text(struct menu_t *menu, const char *label, menu_callback_t on_select, void *arg)
{
    menu_insert_text(menu, menu->item_count, label, on_select, arg);
}

void menu_insert_list(struct menu_t *menu, int index, const char **list, int value, menu_callback_t on_select, void *arg)
{
    struct menu_item_t *item = menu_insert_new(menu, index);
    item->type = MENU_ITEM_TYPE_LIST;
    item->list = list;
    item->list_size = 0;
    while (list[item->list_size]) {
        item->list_size += 1;
    }
    printf("list_size = %d\n", item->list_size);
    item->value = value;
    item->on_select = on_select;
    item->arg = arg;
}

void menu_append_list(struct menu_t *menu, const char **list, int value, menu_callback_t on_select, void *arg)
{
    menu_insert_list(menu, menu->item_count, list, value, on_select, arg);
}

void menu_remove(struct menu_t *menu, int index)
{
    if (index < 0) {
        index = menu->item_count + index;
    }
    assert(index < menu->item_count);

    if (menu->item_count == 1) {
        free(menu->items);
        menu->items = NULL;
    } else {
        if (index < menu->item_count - 1) {
            memmove(&menu->items[index], &menu->items[index + 1], sizeof(struct menu_item_t) * (menu->item_count - index - 1));
        }
        menu->items = realloc(menu->items, sizeof(struct menu_item_t) * menu->item_count - 1);
        assert(menu->items != NULL);
    }

    menu->item_count -= 1;
}

void menu_dismiss(struct menu_t *menu)
{
    menu->dismissed = true;
}

int menu_get_value(struct menu_t *menu, int index)
{
    if (index < 0) {
        index = menu->item_count + index;
    }
    assert(index < menu->item_count);

    struct menu_item_t *item = &menu->items[index];
    return item->value;
}

void menu_set_value(struct menu_t *menu, int index, int value)
{
    if (index < 0) {
        index = menu->item_count + index;
    }
    assert(index < menu->item_count);

    struct menu_item_t *item = &menu->items[index];
    if (item->type == MENU_ITEM_TYPE_LIST) {
        assert(value >= 0 && value < item->list_size);
    }
    int old_value = item->value;
    item->value = value;
    if (old_value != value) {
        menu_draw(menu);
    }
}

int menu_get_list_size(struct menu_t *menu, int index)
{
    if (index < 0) {
        index = menu->item_count + index;
    }
    assert(index < menu->item_count);

    struct menu_item_t *item = &menu->items[index];
    return item->list_size;
}

void menu_list_cycle(struct menu_t *menu, int index, void *arg)
{
    struct menu_item_t *item = &menu->items[index];
    item->value++;
    if (item->value >= item->list_size) {
        item->value = 0;
    }
    menu_draw(menu);
}
