#pragma once

#include "graphics.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

enum tf_flags_t {
    TF_ALIGN_RIGHT = 1,
    TF_ALIGN_CENTER = 2,
    TF_WORDWRAP = 4,
    TF_ELIDE = 8,
};

struct tf_font_t;

struct tf_t {
    const struct tf_font_t *font;
    uint16_t color;
    short width;
    uint16_t flags;
    struct rect_t rect;
};

struct tf_font_t {
   const unsigned char *p;
   short width;
   short height;
   char first;
   char last;
   const short *widths;
};

struct tf_metrics_t {
    short width;
    short height;
};

struct tf_t *tf_new(const struct tf_font_t *font, short width, uint32_t flags);
void tf_free(struct tf_t *tf);
struct tf_metrics_t tf_get_str_metrics(struct tf_t *tf, const char *s);
short tf_draw_glyph(struct gbuf_t *g, struct tf_t *tf, char c, struct point_t p);
void tf_draw_str(struct gbuf_t *g, struct tf_t *tf, const char *s, struct point_t p);
