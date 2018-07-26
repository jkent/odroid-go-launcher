#include "gbuf.h"

#include <stdint.h>
#include <stdlib.h>


struct gbuf *gbuf_new(uint16_t width, uint16_t height, uint16_t bytes_per_pixel, uint16_t endian)
{
    struct gbuf *g = malloc(sizeof(struct gbuf) + width * height * bytes_per_pixel);
    if (!g) abort();
    g->width = width;
    g->height = height;
    g->bytes_per_pixel = bytes_per_pixel;
    g->endian = endian;
    return g;
}

void gbuf_free(struct gbuf *g)
{
    free(g);
}

void gbuf_blit(struct gbuf *dst, struct gbuf *src, short x, short y)
{
    short xstart = x < 0 ? -x : 0;
    short xend = x + src->width > dst->width ? dst->width - x : src->width;
    short ystart = y < 0 ? -x : 0;
    short yend = y + src->height > dst->height ? dst->height - y : src->height;

    if (src->bytes_per_pixel == 2 && dst->bytes_per_pixel == 2) {
        if (src->endian == dst->endian) {
            for (short yoff = ystart; yoff < yend; yoff++) {
                for (short xoff = xstart; xoff < xend; xoff++) {
                    uint16_t pixel = ((uint16_t *)src->pixel_data)[yoff * src->width + xoff];
                    ((uint16_t *)dst->pixel_data)[(y + yoff) * dst->width + x + xoff] = pixel;
                }
            }
        } else {
            for (short yoff = ystart; yoff < yend; yoff++) {
                for (short xoff = xstart; xoff < xend; xoff++) {
                    uint16_t pixel = ((uint16_t *)src->pixel_data)[yoff * src->width + xoff];
                    pixel = pixel << 8 | pixel >> 8;
                    ((uint16_t *)dst->pixel_data)[(y + yoff) * dst->width + x + xoff] = pixel;
                }
            }
        }
    }
}

void gbuf_blit_gimp_image(struct gbuf *g, struct gimp_image *img, short x, short y)
{
    short xstart = x < 0 ? -x : 0;
    short xend = x + img->width > g->width ? g->width - x : img->width;
    short ystart = y < 0 ? -y : 0;
    short yend = y + img->height > g->height ? g->height - y : img->height;

    if (img->bytes_per_pixel == 2 && g->bytes_per_pixel == 2) {
        if (g->endian == BIG_ENDIAN) {
            for (short yoff = ystart; yoff < yend; yoff++) {
                for (short xoff = xstart; xoff < xend; xoff++) {
                    uint16_t pixel = ((uint16_t *)img->pixel_data)[yoff * img->width + xoff];
                    pixel = pixel << 8 | pixel >> 8;
                    ((uint16_t *)g->pixel_data)[(y + yoff) * g->width + x + xoff] = pixel;
                }
            }
        } else if (g->endian == LITTLE_ENDIAN) {
            for (short yoff = ystart; yoff < yend; yoff++) {
                for (short xoff = xstart; xoff < xend; xoff++) {
                    uint16_t pixel = ((uint16_t *)img->pixel_data)[yoff * img->width + xoff];
                    ((uint16_t *)g->pixel_data)[(y + yoff) * g->width + x + xoff] = pixel;
                }
            }
        }
    }
}
