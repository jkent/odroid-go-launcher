#include "menu.h"

#include "../components/hardware/display.h"
#include "graphics.h"

#include <string.h>


struct menu_t {
    struct gbuf_t *g;
    struct gbuf_t *g_saved;
    struct rect_t rect;
};


struct menu_t *menu_init(struct gbuf_t *g, short width, short height)
{
    struct menu_t *menu = malloc(sizeof(struct menu_t));
    if (menu == NULL) abort();

    menu->g = g;
    menu->rect.x = g->width / 2 - width / 2;
    menu->rect.y = g->height / 2 - height / 2;
    menu->rect.width = width;
    menu->rect.height = height;

    menu->g_saved = gbuf_new(width, height, 2, BIG_ENDIAN);

    return menu;
}

void menu_show(struct menu_t *menu)
{
    struct rect_t dst_rect = {0, 0, menu->rect.width, menu->rect.height};

    xSemaphoreTake(menu->g->mutex, portMAX_DELAY);
    blit(menu->g_saved, dst_rect, menu->g, menu->rect);
    draw_rectangle(menu->g, menu->rect, DRAW_TYPE_FILL, 0xF800);
    draw_rectangle(menu->g, menu->rect, DRAW_TYPE_OUTLINE, 0xFFFF);
    display_update_rect(menu->rect);
    xSemaphoreGive(menu->g->mutex);
}

void menu_hide(struct menu_t *menu)
{
    struct rect_t src_rect = {0, 0, menu->rect.width, menu->rect.height};

    xSemaphoreTake(menu->g->mutex, portMAX_DELAY);
    blit(menu->g, menu->rect, menu->g_saved, src_rect);
    display_update_rect(menu->rect);
    xSemaphoreGive(menu->g->mutex);
}
