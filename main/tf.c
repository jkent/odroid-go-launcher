#include "tf.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct tf *tf_new()
{
    struct tf *tf = calloc(1, sizeof(struct tf));
    if (!tf) {
        return NULL;
    }

    tf->font = NULL;
    tf->fg_color = 0xffff;
    tf->width = -1;
    return tf;
}

void tf_free(struct tf *tf)
{
    free(tf);
}

struct tf_iterinfo {
    const char *s;
    size_t len;
    short width;
};

static struct tf_iterinfo tf_iter_lines(struct tf *tf, const char *start)
{
    static const char *s = NULL;
    struct tf_iterinfo ii = { 0 };
    short width = 0;

    if (!tf || !tf->font) abort();

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

        if (tf->width >= 0 && width + char_width > tf->width) {
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

short tf_draw_glyph(struct gbuf *g, struct tf *tf, char c, short x, short y)
{
    if (!g || !tf || !tf->font || c < tf->font->first || c > tf->font->last) abort();

    short width = tf->font->widths ? tf->font->widths[c - tf->font->first] : tf->font->width;

    short xstart = x < 0 ? -x : 0;
    short xend = x + width > g->width ? g->width - x : width;
    short ystart = y < 0 ? -y : 0;
    short yend = y + tf->font->height > g->height ? g->height - y : tf->font->height;

    uint16_t fg = tf->fg_color;
    uint16_t bg = tf->bg_color;
    if (g->endian == BIG_ENDIAN) {
        fg = tf->fg_color << 8 | tf->fg_color >> 8;
        bg = tf->bg_color << 8 | tf->bg_color >> 8;
    }

    const unsigned char *glyph = tf->font->p + ((tf->font->width + 7) / 8) * tf->font->height * (c - tf->font->first);

    for (short yoff = ystart; yoff < yend; yoff++) {
        for (short xoff = xstart; xoff < xend; xoff++) {
            if (glyph[yoff * ((tf->font->width + 7) / 8) + (xoff / 8)] & (1 << (xoff % 8))) {
                ((uint16_t *)g->pixel_data)[(y + yoff) * g->width + x + xoff] = fg;
            } else if (tf->fill_bg) {
                ((uint16_t *)g->pixel_data)[(y + yoff) * g->width + x + xoff] = bg;
            }
        }
    }

    return width;
}

void tf_draw_str(struct gbuf *g, struct tf *tf, const char *s, short x, short y)
{
    if (!g || !tf || !tf->font) {
        return;
    }

    short xoff = 0;
    short yoff = 0;

    int line = 1;

    uint16_t bg = tf->bg_color;

    if (g->endian == BIG_ENDIAN) {
        bg = bg << 8 | bg >> 8;
    }

    struct tf_iterinfo ii = tf_iter_lines(tf, s);
    while (true) {
        short ystart = y + yoff < 0 ? -(y + yoff) : yoff;
        short yend = y + yoff + tf->font->height > g->height ? g->height - y : yoff + tf->font->height;
        if (ystart >= line * tf->font->height || yend <= 0) {
            break;
        }

        if (tf->width < 0 || tf->align == ALIGN_LEFT) {
            xoff = 0;
            if (tf->fill_bg) {
                short xstart = x + ii.width < 0 ? -(x + ii.width) : ii.width;
                short xend = x + tf->width > g->width ? g->width - (x + tf->width) : tf->width;
                for (short by = ystart; by < yend; by++) {
                    for (short bx = xstart; bx < xend; bx++) {
                        ((uint16_t *)g->pixel_data)[(y + by) * g->width + x + bx] = bg;
                    }
                }
            }
        } else if (tf->align == ALIGN_RIGHT) {
            xoff = tf->width - ii.width; 
            if (tf->fill_bg) {
                short xstart = x < 0 ? -x : 0;
                short xend = x + xoff > g->width ? g->width - x : xoff;
                for (short by = ystart; by < yend; by++) {
                    for (short bx = xstart; bx < xend; bx++) {
                        ((uint16_t *)g->pixel_data)[(y + by) * g->width + x + bx] = bg;
                    }
                }
            } 
        } else if (tf->align == ALIGN_CENTER) {
            xoff = (tf->width - ii.width) / 2;
            if (tf->fill_bg) {
                short xstart = x < 0 ? -x : 0;
                short xend = x + xoff > g->width ? g->width - x : xoff;
                for (short by = ystart; by < yend; by++) {
                    for (short bx = xstart; bx < xend; bx++) {
                        ((uint16_t *)g->pixel_data)[(y + by) * g->width + x + bx] = bg;
                    }
                }
                xstart = x + xoff + ii.width < 0 ? -x : xoff + ii.width;
                xend = x + tf->width > g->width ? g->width - x : tf->width;
                for (short by = ystart; by < yend; by++) {
                    for (short bx = xstart; bx < xend; bx++) {
                        ((uint16_t *)g->pixel_data)[(y + by) * g->width + x + bx] = bg;
                    }
                } 
            }
        }

        for (int i = 0; i < ii.len; i++) {
            xoff += tf_draw_glyph(g, tf, ii.s[i], x + xoff, y + yoff);
        }

        ii = tf_iter_lines(tf, NULL);
        if (ii.len == 0) {
            break;
        }

        yoff += tf->font->height;
        line++;
    }
}
