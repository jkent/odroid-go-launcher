#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "keypad.h"
#include "display.h"
#include "gbuf.h"

#include "statusbar.h"
#include "ui_dialog.h"


ui_dialog_t *ui_dialog_new(rect_t r, const char *title)
{
    ui_dialog_t *d = calloc(1, sizeof(ui_dialog_t));
    memcpy(&d->r, &r, sizeof(rect_t));
    d->g = gbuf_new(r.width, r.height, fb->bytes_per_pixel, fb->endian);
    if (title) {
        d->title = strdup(title);
    }
    return d;
}

void ui_dialog_destroy(ui_dialog_t *d)
{
    gbuf_free(d->g);
    free(d);
}

void ui_dialog_showmodal(ui_dialog_t *d)
{
    d->close = false;

    rect_t r = {
        .x = 0,
        .y = 0,
        .width = d->r.width,
        .height = d->r.height,
    };
    blit(d->g, r, fb, d->r);

    draw_rectangle(fb, d->r, DRAW_TYPE_FILL, 0x0000);
    draw_rectangle(fb, d->r, DRAW_TYPE_OUTLINE, 0xFFFF);

    display_update_rect(d->r);
    d->visible = true;

    uint16_t keys = 0, changes = 0, pressed;
    do {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        keys = keypad_debounce(keypad_sample(), &changes);
        pressed = keys & changes;
        statusbar_update();
    } while (!(pressed & KEYPAD_B));

    blit(fb, d->r, d->g, r);
    display_update_rect(d->r);

    d->visible = false;
}

void ui_dialog_hide(ui_dialog_t *d)
{
    d->close = true;
}
