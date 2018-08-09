#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "keypad.h"
#include "display.h"
#include "gbuf.h"

#include "dialog.h"
#include "OpenSans_Regular_11X12.h"
#include "statusbar.h"
#include "tf.h"


dialog_t *dialog_new(rect_t r, const char *title)
{
    dialog_t *d = calloc(1, sizeof(dialog_t));
    memcpy(&d->r, &r, sizeof(rect_t));
    d->g = gbuf_new(r.width, r.height, fb->bytes_per_pixel, fb->endian);
    if (title) {
        d->title = strdup(title);
    }
    return d;
}

void dialog_destroy(dialog_t *d)
{
    gbuf_free(d->g);
    free(d);
}

void ui_dialog_draw(dialog_t *d)
{
    assert(d->visible);

    d->cr.x = d->r.x + 2;
    d->cr.y = d->r.y + 2;
    d->cr.width = d->r.width - 4;
    d->cr.height = d->r.height - 4;

    draw_rectangle(fb, d->r, DRAW_TYPE_FILL, 0x0000);
    draw_rectangle(fb, d->r, DRAW_TYPE_OUTLINE, 0xFFFF);

    if (d->title) {
        tf_t *tf = tf_new(&font_OpenSans_Regular_11X12, d->cr.width, TF_ALIGN_CENTER | TF_ELIDE);
        tf_metrics_t m = tf_get_str_metrics(tf, d->title);
        rect_t r = {
            .x = d->r.x,
            .y = d->r.y,
            .width = d->r.width,
            .height = m.height + 4,
        };
        draw_rectangle(fb, r, DRAW_TYPE_FILL, 0x001F);
        draw_rectangle(fb, r, DRAW_TYPE_OUTLINE, 0xFFFF);

        point_t p = {
            .x = d->r.x + 2,
            .y = d->r.y + 2,
        };
        tf_draw_str(fb, tf, d->title, p);

        d->cr.y += m.height + 3;
        d->cr.height -= m.height + 3;
    }
}

void dialog_showmodal(dialog_t *d)
{
    d->close = false;

    rect_t r = {
        .x = 0,
        .y = 0,
        .width = d->r.width,
        .height = d->r.height,
    };
    blit(d->g, r, fb, d->r);

    d->visible = true;
    ui_dialog_draw(d);

    for (int i = 0; i < d->num_controls; i++) {
        control_t *control = d->controls[i];
        control->draw(control);
        control->dirty = false;
    }

    //display_update_rect(d->r);
    display_update();

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

void dialog_hide(dialog_t *d)
{
    d->close = true;
}

void dialog_insert_control(dialog_t *d, int index, control_t *control)
{
    assert(control != NULL);
    assert(d->num_controls < MAX_CONTROLS);
    if (index < 0) {
        index = d->num_controls + index;
    }
    assert(index >= 0 && index <= d->num_controls);

    memmove(&d->controls[index + 1], &d->controls[index], sizeof(struct control_t *) * (d->num_controls - index));
    d->controls[index] = control;
    control->d = d;
    d->num_controls += 1;
}

void dialog_append_control(dialog_t *d, control_t *control)
{
    dialog_insert_control(d, d->num_controls, control);
}

control_t *dialog_remove_control(dialog_t *d, int index)
{
    assert(d->num_controls > 0);
    if (index < 0) {
        index = d->num_controls + index;
    }
    assert(index >= 0 && index <= d->num_controls);

    control_t *control = d->controls[index];
    memmove(&d->controls[index], &d->controls[index + 1], sizeof(struct control_t *) * (d->num_controls - index - 1));
    control->d = NULL;
    return control;
}
