#include <string.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "control.h"
#include "dialog.h"
#include "display.h"
#include "keypad.h"
#include "OpenSans_Regular_11X12.h"
#include "osk.h"
#include "statusbar.h"
#include "tf.h"


static void draw(control_t *control)
{
    control_edit_t *edit = (control_edit_t *)control;

    tf_t *tf = tf_new(&font_OpenSans_Regular_11X12, edit->r.width - 4, 0);
    tf->clip = edit->r;
    tf->clip.x += edit->d->cr.x;
    tf->clip.y += edit->d->cr.y;

    fill_rectangle(fb, tf->clip, 0x3186);
    if (control == control->d->active) {
        draw_rectangle(fb, tf->clip, DRAW_STYLE_SOLID, 0xFFFF);
    }

    if (edit->text) {
        tf_metrics_t m = tf_get_str_metrics(tf, edit->text);
        point_t p = {
            .x = edit->d->cr.x + edit->r.x + 2,
            .y = edit->d->cr.y + edit->r.y + edit->r.height/2 - m.height/2 + 1,
        };
        tf_draw_str(fb, tf, edit->text, p);
    }

    tf_free(tf);
    edit->dirty = true;
}

static void onselect(control_t *control)
{
    control_edit_t *edit = (control_edit_t *)control;

    osk_t *osk = osk_new(edit);
    osk_showmodal(osk);
    osk_free(osk);

    control->draw(control);
    display_update_rect(control->d->cr);
}

control_edit_t *control_edit_new(dialog_t *d, rect_t r, const char *text, size_t text_len)
{
    control_edit_t *edit = calloc(1, sizeof(control_edit_t));

    edit->type = CONTROL_EDIT;
    edit->d = d;
    edit->r = r;
    edit->draw = draw;
    edit->onselect = onselect;
    edit->text = malloc(text_len);
    assert(edit->text != NULL);
    strncpy(edit->text, text, text_len);
    edit->text[text_len - 1] = '\0';
    edit->text_len = text_len;

    return edit;
}

void control_edit_delete(control_edit_t *edit)
{
    if (edit->text) {
        free(edit->text);
    }
    free(edit);
}
