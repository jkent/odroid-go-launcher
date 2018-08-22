#include <math.h>
#include <string.h>

#include "keypad.h"
#include "display.h"
#include "gbuf.h"

#include "OpenSans_Regular_11X12.h"
#include "periodic.h"
#include "tf.h"
#include "ui_dialog.h"
#include "ui_theme.h"


static ui_dialog_t *top = NULL;

ui_dialog_t *ui_dialog_new(ui_dialog_t *parent, rect_t r, const char *title)
{
    ui_dialog_t *d = calloc(1, sizeof(ui_dialog_t));
    d->parent = parent;
    d->r = r;
    d->g = gbuf_new(r.width, r.height, fb->bytes_per_pixel, fb->endian);
    if (parent) {
        d->keypad = parent->keypad;
    }
    if (title) {
        d->title = strdup(title);
    }
    ui_dialog_layout(d);
    return d;
}

void ui_dialog_destroy(ui_dialog_t *d)
{
    for (int i = 0; i < d->controls_size; i++) {
        ui_control_t *control = d->controls[i];
        if (control != NULL && control->free) {
            control->free(control);
        }
    }
    gbuf_free(d->g);
    if (d->tf) {
        tf_free(d->tf);
    }
    free(d);
}

void ui_dialog_layout(ui_dialog_t *d)
{
    d->cr.x = d->r.x + 1;
    d->cr.y = d->r.y + 1;
    d->cr.width = d->r.width - 2;
    d->cr.height = d->r.height - 2;

    if (d->title) {
        if (!d->tf) {
            d->tf = tf_new(ui_theme->font, ui_theme->text_color, d->cr.width - 2, TF_ALIGN_CENTER | TF_ELIDE);
        }
        tf_metrics_t m = tf_get_str_metrics(d->tf, d->title);
        d->cr.y += m.height + 2*ui_theme->padding + 1;
        d->cr.height -= m.height + 2 * ui_theme->padding + 1;
    }
}

void ui_dialog_draw(ui_dialog_t *d)
{
    assert(d->visible);

    fill_rectangle(fb, d->r, ui_theme->window_color);
    draw_rectangle3d(fb, d->r, ui_theme->border3d_light_color, ui_theme->border3d_dark_color);

    if (d->title) {
        tf_metrics_t m = tf_get_str_metrics(d->tf, d->title);
        rect_t r = d->cr;
        r.y = d->r.y + 1;
        r.height = m.height + 2*ui_theme->padding;
        fill_rectangle(fb, r, ui_theme->active_highlight_color);

        point_t start = {
            .x = r.x,
            .y = r.y + r.height,
        };
        point_t end = {
            .x = r.x + r.width,
            .y = r.y + r.height,
        };
        draw_line(fb, start, end, DRAW_STYLE_SOLID, ui_theme->border3d_light_color);

        point_t p = {
            .x = d->r.x + ui_theme->padding,
            .y = d->r.y + ui_theme->padding + 1,
        };
        tf_draw_str(fb, d->tf, d->title, p);
    }
}

void ui_dialog_showmodal(ui_dialog_t *d)
{
    d->hide = false;

    rect_t r = {
        .x = 0,
        .y = 0,
        .width = d->r.width,
        .height = d->r.height,
    };
    blit(d->g, r, fb, d->r);

    d->hide = false;
    d->visible = true;
    ui_dialog_draw(d);

    d->parent = top;
    top = d;

    size_t count = 0;
    for (int i = 0; i < d->controls_size; i++) {
        ui_control_t *control = d->controls[i];
        if (control == NULL) {
            continue;
        }
        if (control->type != CONTROL_LABEL) {
            count += 1;
            if (!d->active) {
                d->active = control;
            }
        }
        control->draw(control);
        control->dirty = false;
    }

    if (count == 1 && d->active->type == CONTROL_LIST) {
        ((ui_list_t *)d->active)->selected = true;
        d->active->draw(d->active);
        display_update_rect(d->r);
        d->active->onselect(d->active, d->active->arg);
        d->hide = true;
    } else {
        display_update_rect(d->r);
    }

    keypad_info_t keys;
    while (!d->hide) {
        if (keypad_queue_receive(d->keypad, &keys, 50/portTICK_RATE_MS)) {
            if (keys.pressed & KEYPAD_MENU) {
                ui_dialog_unwind();
                break;
            }

            ui_control_t *new_active = NULL;
            if (keys.pressed & KEYPAD_UP) {
                new_active = ui_dialog_find_control(d, DIRECTION_UP);
            } else if (keys.pressed & KEYPAD_RIGHT) {
                new_active = ui_dialog_find_control(d, DIRECTION_RIGHT);
            } else if (keys.pressed & KEYPAD_DOWN) {
                new_active = ui_dialog_find_control(d, DIRECTION_DOWN);
            } else if (keys.pressed & KEYPAD_LEFT) {
                new_active = ui_dialog_find_control(d, DIRECTION_LEFT);
            }

            if (new_active) {
                ui_control_t *old_active = d->active;
                d->active = new_active;
                old_active->draw(old_active);
                new_active->draw(new_active);
            }

            if (keys.pressed & KEYPAD_A && d->active && d->active->onselect) {
                d->active->onselect(d->active, d->active->arg);
            }

            if (keys.pressed & KEYPAD_B) {
                break;
            }

            bool dirty = false;
            for (int i = 0; i < d->controls_size; i++) {
                ui_control_t *control = d->controls[i];
                if (control == NULL) {
                    continue;
                }
                if (control->dirty) {
                    dirty = true;
                    control->dirty = false;
                }
            }
            if (dirty) {
                display_update_rect(d->r);
            }
        }

        bool dirty = false;
        for (int i = 0; i < d->controls_size; i++) {
            ui_control_t *control = d->controls[i];
            if (control == NULL) {
                continue;
            }
            if (control->dirty) {
                dirty = true;
                control->dirty = false;
            }
        }
        if (dirty) {
            display_update_rect(d->r);
        }

        periodic_tick();
    }

    blit(fb, d->r, d->g, r);
    display_update_rect(d->r);

    top = d->parent;
    d->visible = false;
}

void ui_dialog_hide(ui_dialog_t *d)
{
    d->hide = true;
}

void ui_dialog_unwind(void)
{
    ui_dialog_t *d = top;
    while (d) {
        d->hide = true;
        if (d->active) {
            d->active->hide = true;
        }
        d = d->parent;
    }
}

void ui_dialog_add_control(ui_dialog_t *d, ui_control_t *control)
{
    for (int i = 0; i < d->controls_size; i++) {
        if (d->controls[i] == NULL) {
            d->controls[i] = control;
            return;
        }
    }

    d->controls_size += 1;
    d->controls = realloc(d->controls, sizeof(ui_control_t *) * d->controls_size);
    assert(d->controls != NULL);
    d->controls[d->controls_size - 1] = control;
}

ui_control_t *ui_dialog_find_control(ui_dialog_t *d, direction_t dir)
{
    ui_control_t *active = d->active;
    ui_control_t *hidest = NULL;
    float hidest_distance = INFINITY;

    if (!active) {
        return NULL;
    }

    short active_cx = active->r.x + active->r.width/2;
    short active_cy = active->r.y + active->r.height/2;

    for (size_t i = 0; i < d->controls_size; i++) {
        ui_control_t *control = d->controls[i];
        if (!control || control == active || control->type == CONTROL_LABEL) {
            continue;
        }
        short control_cx = control->r.x + control->r.width/2;
        short control_cy = control->r.y + control->r.height/2;

        if ((dir == DIRECTION_LEFT  && control->r.x + control->r.width - 1 < active->r.x) ||
            (dir == DIRECTION_RIGHT && control->r.x > active->r.x + active->r.width - 1) ||
            (dir == DIRECTION_UP    && control->r.y + control->r.height - 1 < active->r.y) ||
            (dir == DIRECTION_DOWN  && control->r.y > active->r.y + active->r.height - 1)) {
            short dx = abs(active_cx - control_cx);
            short dy = abs(active_cy - control_cy);
            float distance = sqrtf(dx * dx + dy * dy);
            if (hidest == NULL || distance < hidest_distance) {
                hidest = control;
                hidest_distance = distance;
            }
        }
    }
    return hidest;
}

ui_dialog_t *ui_dialog_get_top(void)
{
    return top;
}
