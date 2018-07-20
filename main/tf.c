#include "tf.h"

#include <string.h>
#include <stdlib.h>

#include "../components/hardware/display.h"

struct tf *tf_new()
{
    struct tf *tf = calloc(1, sizeof(struct tf));
    if (!tf) {
        return NULL;
    }

    tf->font = NULL;
    tf->fg_color = 0xffff;
    tf->transparent_bg = true;
    return tf;
}

void tf_free(struct tf *tf)
{
    free(tf);
}

struct tf_iterinfo {
    const char *s;
    size_t len;
    size_t width;
};

static struct tf_iterinfo tf_iter_lines(struct tf *tf, const char *start)
{
    static const char *s = NULL;
    struct tf_iterinfo ii = { 0 };
    short width = 0;

    if (start) {
        s = start;
    } else {
        while (*s == ' ') {
            s++;
        }
    }

    if (!s) {
        return ii;
    }

    const char *p = s;
    while (*p) {
        if (*p < tf->font->first || *p > tf->font->last) {
            continue;
        }

        short char_width = tf->font->widths ? tf->font->widths[*p - tf->font->first] : tf->font->width;

        if (tf->bbox_width && width + char_width > tf->bbox_width) {
            const char *q = p;
            short sub = 0;
            while (p--) {
                if (*p < tf->font->first || *p > tf->font->last) {
                    continue;
                }
                sub += tf->font->widths ? tf->font->widths[*p - tf->font->first] : tf->font->width;
                if (*p == ' ') {
                    break;
                }
                if (width <= sub) {
                    p = q;
                    sub = 0;
                    break;
                }
            }
            width -= sub;    
            break;
        }
        width += char_width;
        p++;
    }

    ii.s = s;
    ii.len = p - s;
    ii.width = width;
    s = p;

    return ii;
}

struct tf_metrics tf_get_str_metrics(struct tf *tf, const char *s)
{
    struct tf_metrics m = { 0 };
    struct tf_iterinfo ii = tf_iter_lines(tf, s);

    while (ii.len) {
        m.height += tf->font->height;
        m.width = ii.width > m.width ? ii.width : m.width; 

        ii = tf_iter_lines(tf, NULL);
    }

    return m;
}

short tf_draw_glyph(struct tf *tf, char c, short x, short y)
{
    if (!tf->fb || c < tf->font->first || c > tf->font->last) {
        return 0;
    }

    const unsigned char *glyph = tf->font->p + ((tf->font->width + 7) / 8) * tf->font->height * (c - tf->font->first);
    short width = tf->font->widths ? tf->font->widths[c - tf->font->first] : tf->font->width;

    for (short yoff = 0; yoff < tf->font->height; yoff++) {
        for (short xoff = 0; xoff < width; xoff++) {
            if (glyph[yoff * ((tf->font->width + 7) / 8) + (xoff / 8)] & (1 << (xoff % 8))) {
                tf->fb[(y + yoff) * DISPLAY_WIDTH + x + xoff] = tf->fg_color;
            } else if (!tf->transparent_bg) {
                tf->fb[(y + yoff) * DISPLAY_WIDTH + x + xoff] = tf->bg_color;
            }
        }
    }

    return width;
}

void tf_draw_str(struct tf *tf, const char *s, short x, short y)
{
    if (!tf->font || !tf->fb) {
        return;
    }

    short xoff = 0;
    short yoff = 0;

    struct tf_iterinfo ii = tf_iter_lines(tf, s);
    while (true) {
        if (tf->bbox_width == 0 || tf->align == ALIGN_LEFT) {
            xoff = 0;
            if (!tf->transparent_bg && tf->bbox_width) {
                for (short by = yoff; by < yoff + tf->font->height; by++) {
                    for (short bx = ii.width; bx < tf->bbox_width; bx++) {
                        tf->fb[(y + by) * DISPLAY_WIDTH + x + bx] = tf->bg_color;
                    }
                }
            }
        } else if (tf->align == ALIGN_RIGHT) {
           xoff = tf->bbox_width - ii.width; 
           if (!tf->transparent_bg) {
                for (short by = yoff; by < yoff + tf->font->height; by++) {
                    for (short bx = 0; bx < xoff; bx++) {
                        tf->fb[(y + by) * DISPLAY_WIDTH + x + bx] = tf->bg_color;
                    }
                }
            } 
        } else if (tf->align == ALIGN_CENTER) {
            xoff = (tf->bbox_width - ii.width) / 2;
            if (!tf->transparent_bg) {
                for (short by = yoff; by < yoff + tf->font->height; by++) {
                    for (short bx = 0; bx < xoff; bx++) {
                        tf->fb[(y + by) * DISPLAY_WIDTH + x + bx] = tf->bg_color;
                    }
                    for (short bx = ii.width - xoff; bx < tf->bbox_width; bx++) {
                        tf->fb[(y + by) * DISPLAY_WIDTH + x + bx] = tf->bg_color;
                    }
 
                }
            }
        }

        for (int i = 0; i < ii.len; i++) {
            xoff += tf_draw_glyph(tf, ii.s[i], x + xoff, y + yoff);
        }

        ii = tf_iter_lines(tf, NULL);
        if (ii.len == 0) {
            break;
        }
        yoff += tf->font->height;
    }
}
