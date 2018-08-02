#include "menu.h"

#include "../components/hardware/display.h"
#include "OpenSans_Regular_11X12.h"

#include "graphics.h"
#include "tf.h"

#include <string.h>


struct menu_t {
    struct gbuf_t *g;
    struct gbuf_t *g_saved;
    struct rect_t rect;
    size_t item_count;
    struct menu_item_t *items;
};


struct menu_t *menu_new(struct gbuf_t *g, short width, short height)
{
    struct menu_t *menu = malloc(sizeof(struct menu_t));
    assert(menu != NULL);

    menu->g = g;
    menu->g_saved = gbuf_new(width, height, 2, BIG_ENDIAN);
    menu->rect.x = g->width/2 - width/2;
    menu->rect.y = g->height/2 - height/2;
    menu->rect.width = width;
    menu->rect.height = height;

    menu->item_count = 0;
    menu->items = NULL;

    struct menu_item_t item;
    item.label = strdup("Hello world!");
    menu_append(menu, &item);

    return menu;
}

void menu_free(struct menu_t *menu)
{
    assert(menu != NULL);

    while (menu->item_count > 0) {
        menu_remove(menu, -1);
    }

    gbuf_free(menu->g_saved);
}

static void menu_draw(struct menu_t *menu)
{
    struct tf *tf = tf_new();
    tf->font = &font_OpenSans_Regular_11X12;

    xSemaphoreTake(menu->g->mutex, portMAX_DELAY);
    draw_rectangle(menu->g, menu->rect, DRAW_TYPE_FILL, 0x0000);
    draw_rectangle(menu->g, menu->rect, DRAW_TYPE_OUTLINE, 0xFFFF);

    for (unsigned int i = 0; i < menu->item_count; i++) {
        struct point_t p = {menu->rect.x + 2, menu->rect.y + 3 + (2 + tf->font->height) * i};
        tf_draw_str(menu->g, tf, menu->items[i].label, p);
    }

    display_update_rect(menu->rect);
    xSemaphoreGive(menu->g->mutex);

    tf_free(tf);
}

void menu_show(struct menu_t *menu)
{
    struct rect_t dst_rect = {0, 0, menu->rect.width, menu->rect.height};

    xSemaphoreTake(menu->g->mutex, portMAX_DELAY);
    blit(menu->g_saved, dst_rect, menu->g, menu->rect);
    xSemaphoreGive(menu->g->mutex);
    menu_draw(menu);
}

void menu_hide(struct menu_t *menu)
{
    struct rect_t src_rect = {0, 0, menu->rect.width, menu->rect.height};

    xSemaphoreTake(menu->g->mutex, portMAX_DELAY);
    blit(menu->g, menu->rect, menu->g_saved, src_rect);
    display_update_rect(menu->rect);
    xSemaphoreGive(menu->g->mutex);
}

/* takes ownership of item's variables and adds item to the menu before index */
void menu_insert(struct menu_t *menu, int index, struct menu_item_t *item)
{
    if (index < 0) {
        index = menu->item_count + index;
    }
    assert(index <= menu->item_count);

    if (menu->item_count == 0) {
        menu->items = malloc(sizeof(struct menu_item_t));
        assert(menu->items != NULL);
    } else {
        menu->items = realloc(menu->items, sizeof(struct menu_item_t) * menu->item_count);
        assert(menu->items != NULL);
        if (index < menu->item_count) {
            memmove(&menu->items[index + 1], &menu->items[index], sizeof(struct menu_item_t) * (menu->item_count - index));
        }
    }

    memcpy(&menu->items[index], item, sizeof(struct menu_item_t));
    menu->item_count += 1;
}

/* takes ownership of item's variables and adds item to the menu's bottom */
void menu_append(struct menu_t *menu, struct menu_item_t *item)
{
    menu_insert(menu, menu->item_count, item);
}

/* remove menu item at index, freeing its varables */
void menu_remove(struct menu_t *menu, int index)
{
    if (index < 0) {
        index = menu->item_count + index;
    }
    assert(index < menu->item_count);

    free(menu->items[index].label);
    if (menu->item_count == 0) {
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
