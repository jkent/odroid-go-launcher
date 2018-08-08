#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "display.h"
#include "keypad.h"

#include "graphics.h"
#include "OpenSans_Regular_11X12.h"
#include "statusbar.h"
#include "tf.h"

#include "menu.h"


struct menu_item_t;

struct menu_t {
    struct gbuf_t *g;
    struct rect_t rect;
    struct menu_t *parent;
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
    menu_task_t task_fn;
};

enum menu_item_type_t {
    MENU_ITEM_TYPE_TEXT,
    MENU_ITEM_TYPE_STRDUP,
    MENU_ITEM_TYPE_LIST,
    MENU_ITEM_TYPE_DIVIDER,
    MENU_ITEM_TYPE_TITLE,
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

struct menu_t *menu_top = NULL;

struct menu_t *menu_new(struct rect_t rect, menu_task_t task_fn)
{
    struct menu_t *menu = calloc(1, sizeof(struct menu_t));
    assert(menu != NULL);

    menu->g = gbuf_new(rect.width, rect.height, 2, BIG_ENDIAN);
    menu->rect = rect;
    menu->border_width = 2;

    menu->tf_text = tf_new(&font_OpenSans_Regular_11X12, menu->rect.width - 2 * menu->border_width, TF_ELIDE);
    menu->tf_text->clip.x = menu->rect.x + menu->border_width;
    menu->tf_text->clip.y = menu->rect.y + menu->border_width;
    menu->tf_text->clip.width = menu->rect.width - 2 * menu->border_width;
    menu->tf_text->clip.height = menu->rect.height - 2 * menu->border_width;

    menu->item_height = menu->tf_text->font->height + 2;
    menu->item_displayed = (menu->rect.height - 2 * menu->border_width + menu->item_height - 1) / menu->item_height;

    menu->task_fn = task_fn;

    return menu;
}

void menu_free(struct menu_t *menu)
{
    assert(menu != NULL);

    while (menu->item_count > 0) {
        menu_remove(menu, -1);
    }

    tf_free(menu->tf_text);

    gbuf_free(menu->g);

    free(menu);
}

void menu_draw(struct menu_t *menu)
{
    draw_rectangle(fb, menu->rect, DRAW_TYPE_FILL, 0x0000);
    draw_rectangle(fb, menu->rect, DRAW_TYPE_OUTLINE, 0xFFFF);

    struct menu_item_t *item = &menu->items[menu->item_selected];
    while (menu->item_selected < menu->item_count && item->type == MENU_ITEM_TYPE_TITLE) {
        menu->item_selected += 1;
        item = &menu->items[menu->item_selected];
    }

    for (unsigned int row = 0; row < menu->item_displayed; row++) {
        if (row >= menu->item_count) {
            break;
        }

        item = &menu->items[menu->item_first + row];

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

        switch (item->type) {
            case MENU_ITEM_TYPE_TEXT:
            case MENU_ITEM_TYPE_STRDUP:
            case MENU_ITEM_TYPE_LIST:
                if (menu->item_first + row == menu->item_selected) {
                    draw_rectangle(fb, r, DRAW_TYPE_FILL, 0x001F);
                }
                menu->tf_text->flags = TF_ELIDE;
                menu->tf_text->color = 0xFFFF;
                break;

            case MENU_ITEM_TYPE_TITLE:
                draw_rectangle(fb, r, DRAW_TYPE_FILL, 0xFFFF);
                menu->tf_text->flags = TF_ALIGN_CENTER | TF_ELIDE;
                menu->tf_text->color = 0x0000;
                break;

            case MENU_ITEM_TYPE_DIVIDER:
                break;
        }

        switch (item->type) {
            case MENU_ITEM_TYPE_TEXT:
            case MENU_ITEM_TYPE_STRDUP:
            case MENU_ITEM_TYPE_TITLE:
                tf_draw_str(fb, menu->tf_text, item->label, p);
                break;

            case MENU_ITEM_TYPE_LIST:
                tf_draw_str(fb, menu->tf_text, item->list[item->value], p);
                break;

            case MENU_ITEM_TYPE_DIVIDER: {
                struct rect_t line = {
                    .x = r.x,
                    .y = r.y + r.height/2,
                    .width = r.width,
                    .height = 1,
                };
                draw_rectangle(fb, line, DRAW_TYPE_FILL, 0xFFFF);
                break;
            }
        }
    }

    display_update_rect(menu->rect.x, menu->rect.y, menu->rect.width, menu->rect.height);
}

static void menu_hide(struct menu_t *menu)
{
    struct rect_t src_rect = {0, 0, menu->rect.width, menu->rect.height};

    blit(fb, menu->rect, menu->g, src_rect);
    display_update_rect(menu->rect.x, menu->rect.y, menu->rect.width, menu->rect.height);
}

void menu_showmodal(struct menu_t *menu)
{
    struct rect_t dst_rect = {0, 0, menu->rect.width, menu->rect.height};

    menu->parent = menu_top;
    menu_top = menu;

    menu->dismissed = false;

    blit(menu->g, dst_rect, fb, menu->rect);
    menu_draw(menu);

    uint16_t keys = 0, changes = 0, pressed = 0;
    do {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        keys = keypad_debounce(keypad_sample(), &changes);
        pressed = keys & changes;

        struct menu_item_t *item = &menu->items[menu->item_selected];

        if (pressed & KEYPAD_A) {
            if (menu->item_selected < menu->item_count && item->on_select != NULL) {
                item->on_select(menu, menu->item_selected, item->arg);
            }
        } else if (pressed & KEYPAD_UP) {
            if (menu->item_selected > 0) {
                int jump = 0;
                do {
                    jump += 1;
                    item = &menu->items[menu->item_selected - jump];
                } while (menu->item_selected - jump > 0 && (item->type == MENU_ITEM_TYPE_DIVIDER || item->type == MENU_ITEM_TYPE_TITLE));
                menu->item_selected -= jump;
                if (menu->item_first >= menu->item_selected) {
                    menu->yshift = 0;
                }
                if (menu->item_first > menu->item_selected) {
                    menu->item_first -= jump;
                }
                menu_draw(menu);
            }
        } else if (pressed & KEYPAD_DOWN) {
            if (menu->item_selected < menu->item_count - 1) {
                int jump = 0;
                do {
                    jump += 1;
                    item = &menu->items[menu->item_selected + jump];
                } while (menu->item_selected < menu->item_count - 1 + jump && (item->type == MENU_ITEM_TYPE_DIVIDER || item->type == MENU_ITEM_TYPE_TITLE));
                menu->item_selected += jump;
                if (menu->item_selected - menu->item_first >= menu->item_displayed - 1) {
                    menu->yshift = menu->item_height - (menu->rect.height - 2 * menu->border_width) % menu->item_height;
                    if (menu->yshift >= menu->item_height) {
                        menu->yshift = 0;
                    }
                }
                if (menu->item_selected - menu->item_first >= menu->item_displayed) {
                    menu->item_first += jump;
                }

                menu_draw(menu);
            }
        } else if (pressed & KEYPAD_MENU) {
            struct menu_t *m = menu;
            do {
                m->dismissed = true;
                m = m->parent;
            } while (m);
        }

        if (menu->task_fn != NULL) {
            menu->task_fn(menu);
        }

        statusbar_update();
    } while (!(pressed & KEYPAD_B) && !menu->dismissed);

    menu_hide(menu);
    menu_top = menu->parent;
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

    if (menu_top == menu) {
        menu_draw(menu);
    }
}

void menu_append_text(struct menu_t *menu, const char *label, menu_callback_t on_select, void *arg)
{
    menu_insert_text(menu, menu->item_count, label, on_select, arg);
}

void menu_insert_strdup(struct menu_t *menu, int index, const char *label, menu_callback_t on_select, void *arg)
{
    struct menu_item_t *item = menu_insert_new(menu, index);
    item->type = MENU_ITEM_TYPE_STRDUP;
    item->label = strdup(label);
    item->on_select = on_select;
    item->arg = arg;

    if (menu_top == menu) {
        menu_draw(menu);
    }
}

void menu_append_strdup(struct menu_t *menu, const char *label, menu_callback_t on_select, void *arg)
{
    menu_insert_strdup(menu, menu->item_count, label, on_select, arg);
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
    item->value = value;
    item->on_select = on_select;
    item->arg = arg;

    if (menu_top == menu) {
        menu_draw(menu);
    }
}

void menu_append_list(struct menu_t *menu, const char **list, int value, menu_callback_t on_select, void *arg)
{
    menu_insert_list(menu, menu->item_count, list, value, on_select, arg);
}

void menu_insert_divider(struct menu_t *menu, int index)
{
    struct menu_item_t *item = menu_insert_new(menu, index);
    item->type = MENU_ITEM_TYPE_DIVIDER;

    if (menu_top == menu) {
        menu_draw(menu);
    }
}

void menu_append_divider(struct menu_t *menu) {
    menu_insert_divider(menu, menu->item_count);
}

void menu_insert_title(struct menu_t *menu, int index, const char *label)
{
    struct menu_item_t *item = menu_insert_new(menu, index);
    item->type = MENU_ITEM_TYPE_TITLE;
    item->label = label;

    if (menu_top == menu) {
        menu_draw(menu);
    }
}

void menu_append_title(struct menu_t *menu, const char *label)
{
    menu_insert_title(menu, menu->item_count, label);
}

void menu_remove(struct menu_t *menu, int index)
{
    if (index < 0) {
        index = menu->item_count + index;
    }
    assert(index < menu->item_count);

    struct menu_item_t *item = &menu->items[menu->item_selected];
    switch (item->type) {
        case MENU_ITEM_TYPE_STRDUP:
            if (item->label) {
                free((void *)item->label);
            }
            break;

        default:
            break;
    }

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

    if (menu_top == menu) {
        menu_draw(menu);
    }
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
    if (menu_top == menu && old_value != value) {
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
    int count = menu_get_list_size(menu, index);
    int value = menu_get_value(menu, index);

    value += 1;
    if (value >= count) {
        value = 0;
    }
    menu_set_value(menu, index, value);
}
