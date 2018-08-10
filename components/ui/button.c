#include <string.h>

#include "control.h"
#include "dialog.h"
#include "display.h"
#include "OpenSans_Regular_11X12.h"
#include "tf.h"


static void draw(control_t *control)
{
    control_button_t *button = (control_button_t *)control;

    tf_t *tf = tf_new(&font_OpenSans_Regular_11X12, button->r.width, TF_ALIGN_CENTER | TF_ELIDE);
    tf->clip = button->r;
    tf->clip.x += button->d->cr.x;
    tf->clip.y += button->d->cr.y;

    fill_rectangle(fb, tf->clip, 0x632C);
    if (control == control->d->active) {
        draw_rectangle(fb, tf->clip, DRAW_STYLE_SOLID, 0xFFFF);
    }

    if (button->text) {
        tf_metrics_t m = tf_get_str_metrics(tf, button->text);
        point_t p = {
            .x = button->d->cr.x + button->r.x,
            .y = button->d->cr.y + button->r.y + button->r.height/2 - m.height/2 + 1,
        };
        tf_draw_str(fb, tf, button->text, p);
    }

    tf_free(tf);
    button->dirty = true;
}

control_button_t *control_button_new(dialog_t *d, rect_t r, const char *text, control_onselect_t onselect)
{
    control_button_t *button = calloc(1, sizeof(control_button_t));

    button->type = CONTROL_BUTTON;
    button->d = d;
    button->r = r;
    button->draw = draw;
    button->text = strdup(text);
    button->onselect = onselect;

    return button;
}

void control_button_delete(control_button_t *button)
{
    if (button->text) {
        free(button->text);
    }
    free(button);
}
