#pragma once

#include <stdint.h>

struct gfx_image {
  unsigned int 	 width;
  unsigned int 	 height;
  unsigned int 	 bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ 
  unsigned char	 pixel_data[];
};

void gfx_blit_image(uint16_t *fb, struct gfx_image *img, short x, short y);
