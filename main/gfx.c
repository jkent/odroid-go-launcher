#include "gfx.h"

#include "../components/hardware/display.h"


void gfx_blit_image(uint16_t *fb, struct gfx_image *img, short x, short y)
{
    if (img->bytes_per_pixel == 2) {
        for (short yoff = 0; yoff < img->height; yoff++) {
            for (short xoff = 0; xoff < img->width; xoff++) {
                uint16_t pixel = ((uint16_t *)img->pixel_data)[yoff * img->width + xoff];
                fb[(y + yoff) * DISPLAY_WIDTH + x + xoff] = (pixel << 8) | (pixel >> 8);
            }
        }
    }
}