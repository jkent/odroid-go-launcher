#include <string.h>

#include "control.h"
#include "dialog.h"
#include "display.h"
#include "OpenSans_Regular_11X12.h"
#include "tf.h"


static void draw(control_t *control)
{
    control_label_t *label = (control_label_t *)control;
    
    tf_t *tf = tf_new(&font_OpenSans_Regular_11X12, label->r.width, TF_ELIDE);
    tf->clip = label->r;
    tf->clip.x += label->d->cr.x;
    tf->clip.y += label->d->cr.y;

    if (label->text) {
        tf_metrics_t m = tf_get_str_metrics(tf, label->text);
        point_t p = {
            .x = label->d->cr.x + label->r.x,
            .y = label->d->cr.y + label->r.y + label->r.height/2 - m.height/2,
        };
        tf_draw_str(fb, tf, label->text, p);
    }

    tf_free(tf);
    label->dirty = true;
}

control_label_t *control_label_new(dialog_t *d, rect_t r, const char *text)
{
    control_label_t *label = calloc(1, sizeof(control_label_t));

    label->type = CONTROL_LABEL;
    label->d = d;
    label->r = r;
    label->draw = draw;
    label->text = strdup(text);

    return label;
}

void control_label_delete(control_label_t *label)
{
    if (label->text) {
        free(label->text);
    }
    free(label);
}
