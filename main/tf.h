#pragma once

#include "graphics.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


enum tf_align {
    ALIGN_LEFT = 0,
    ALIGN_RIGHT, 
    ALIGN_CENTER,
};

struct tf_font;

struct tf {
    const struct tf_font *font;
    uint16_t fg_color;
    uint16_t bg_color;
    bool fill_bg;
    short width;
    enum tf_align align;
};

struct tf_font {
   const unsigned char *p;
   short width;
   short height;
   char first;
   char last;
   const short *widths;
};

struct tf_metrics {
    short width;
    short height;
};

struct tf *tf_new();
void tf_free(struct tf *tf);
struct tf_metrics tf_get_str_metrics(struct tf *tf, const char *s);
short tf_draw_glyph(struct gbuf_t *g, struct tf *tf, char c, struct point_t p);
void tf_draw_str(struct gbuf_t *g, struct tf *tf, const char *s, struct point_t p);
