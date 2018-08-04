#include "graphics.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct gbuf_t *gbuf_new(uint16_t width, uint16_t height, uint16_t bytes_per_pixel, uint16_t endian)
{
    struct gbuf_t *g = malloc(sizeof(struct gbuf_t) + width * height * bytes_per_pixel);
    if (!g) abort();

    g->width = width;
    g->height = height;
    g->bytes_per_pixel = bytes_per_pixel;
    g->endian = endian;

    return g;
}

void gbuf_free(struct gbuf_t *g)
{
    free(g);
}

void blit(struct gbuf_t *dst, struct rect_t dst_rect, struct gbuf_t *src, struct rect_t src_rect)
{
    assert(dst_rect.width == src_rect.width);
    assert(dst_rect.height == src_rect.height);
    assert(dst_rect.width >= 0);
    assert(dst_rect.height >= 0);

    /* src_rect.width and src_rect.height are not used after this point */

    if (dst_rect.x < 0) {
        short clip = -dst_rect.x;
        dst_rect.x += clip;
        src_rect.x += clip;
        dst_rect.width -= clip;
    }

    if (dst_rect.y < 0) {
        short clip = -dst_rect.y;
        dst_rect.y += clip;
        src_rect.y += clip;
        dst_rect.height -= clip;
    }

    if (dst_rect.x + dst_rect.width > dst->width) {
        dst_rect.width -= (dst_rect.x + dst_rect.width) - dst->width;
    }

    if (dst_rect.y + dst_rect.height > dst->height) {
        dst_rect.height -= (dst_rect.y + dst_rect.height) - dst->height;
    }

    if (src_rect.x < 0) {
        short clip = -src_rect.x;
        dst_rect.x += clip;
        src_rect.x += clip;
        dst_rect.width -= clip;
    }

    if (src_rect.y < 0) {
        short clip = -src_rect.y;
        dst_rect.y += clip;
        src_rect.y += clip;
        dst_rect.height -= clip;
    }

    if (src_rect.x + dst_rect.width > src->width) {
        dst_rect.width -= (src_rect.x + dst_rect.width) - src->width;
    }

    if (src_rect.y + dst_rect.height > src->height) {
        dst_rect.height -= (src_rect.y + dst_rect.height) - src->height;
    }

    if (src->bytes_per_pixel == 2 && dst->bytes_per_pixel == 2) {
        if (src->endian == dst->endian) {
            for (short yoff = 0; yoff < dst_rect.height; yoff++) {
                uint16_t *dst_addr = ((uint16_t *)dst->pixel_data) + (dst_rect.y + yoff) * dst->width + dst_rect.x;
                uint16_t *src_addr = ((uint16_t *)src->pixel_data) + (src_rect.y + yoff) * src->width + src_rect.x;
                memcpy(dst_addr, src_addr, dst_rect.width * dst->bytes_per_pixel);
            }
        } else {
            for (short yoff = 0; yoff < dst_rect.height; yoff++) {
                uint16_t *dst_addr = ((uint16_t *)dst->pixel_data) + (dst_rect.y + yoff) * dst->width + dst_rect.x;
                uint16_t *src_addr = ((uint16_t *)src->pixel_data) + (src_rect.y + yoff) * src->width + src_rect.x;
                for (short xoff = 0; xoff < dst_rect.width; xoff++) {
                    *(dst_addr + xoff) = *(src_addr + xoff) << 8 | *(src_addr + xoff) >> 8;
                }
            }
        }
    }
}

void draw_image(struct gbuf_t *dst, struct image_t *src, short x, short y)
{
    short xstart = x < 0 ? -x : 0;
    short xend = x + src->width > dst->width ? dst->width - x : src->width;
    short ystart = y < 0 ? -y : 0;
    short yend = y + src->height > dst->height ? dst->height - y : src->height;

    if (src->bytes_per_pixel == 2 && dst->bytes_per_pixel == 2) {
        if (dst->endian == BIG_ENDIAN) {
            for (short yoff = ystart; yoff < yend; yoff++) {
                uint16_t *dst_addr  = ((uint16_t *)dst->pixel_data) + (y + yoff) * dst->width + x;
                uint16_t *src_addr = ((uint16_t *)src->pixel_data) + yoff * src->width;
                for (short xoff = xstart; xoff < xend; xoff++) {
                    *(dst_addr + xoff) = *(src_addr + xoff) << 8 | *(src_addr + xoff) >> 8;
                }
            }
        } else if (dst->endian == LITTLE_ENDIAN) {
            for (short yoff = ystart; yoff < yend; yoff++) {
                uint16_t *dst_addr  = ((uint16_t *)dst->pixel_data) + (y + yoff) * dst->width + x;
                uint16_t *src_addr = ((uint16_t *)src->pixel_data) + yoff * src->width;
                memcpy(dst_addr, src_addr, (xend - xstart) * dst->bytes_per_pixel);
            }
        }
    }
}

void draw_rectangle(struct gbuf_t *dst, struct rect_t rect, enum draw_type_t draw_type, uint16_t color)
{
    assert(rect.x >= 0);
    assert(rect.y >= 0);
    assert(rect.width > 0);
    assert(rect.height > 0);
    assert(rect.x + rect.width < dst->width);
    assert(rect.y + rect.height < dst->height);

    if (dst->endian == BIG_ENDIAN) {
        color = color << 8 | color >> 8;
    }

    if (draw_type == DRAW_TYPE_FILL) {
        for (short yoff = 0; yoff < rect.height; yoff++) {
            uint16_t *dst_addr  = ((uint16_t *)dst->pixel_data) + (rect.y + yoff) * dst->width + rect.x;
            for (short xoff = 0; xoff < rect.width; xoff++) {
                *(dst_addr + xoff) = color;
            }
        }
    } else if (draw_type == DRAW_TYPE_OUTLINE) {
        uint16_t *dst_addr = ((uint16_t *)dst->pixel_data) + rect.y * dst->width + rect.x;
        for (short xoff = 0; xoff < rect.width; xoff++) {
            *(dst_addr + xoff) = color;
        }

        for (short yoff = 1; yoff < rect.height - 1; yoff++) {
            dst_addr = ((uint16_t *)dst->pixel_data) + (rect.y + yoff) * dst->width + rect.x;
            *dst_addr = color;
            *(dst_addr + rect.width - 1) = color;
        }

        dst_addr = ((uint16_t *)dst->pixel_data) + (rect.y + rect.height - 1) * dst->width + rect.x;
        for (short xoff = 0; xoff < rect.width; xoff++) {
            *(dst_addr + xoff) = color;
        }
    }
}
