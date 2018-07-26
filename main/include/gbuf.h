#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <machine/endian.h>

struct gimp_image {
  uint32_t width;
  uint32_t height;
  uint32_t bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ 
  uint8_t  pixel_data[];
};

struct gbuf {
  uint16_t width;
  uint16_t height;
  uint16_t bytes_per_pixel; /* 16:RGB16, 24:RGB, 32:RGBA */  
  uint16_t endian;
  uint8_t  pixel_data[];
};

struct gbuf *gbuf_new(uint16_t width, uint16_t height, uint16_t bytes_per_pixel, uint16_t endian, bool spiflash);
void gbuf_free(struct gbuf *g);
void gbuf_blit(struct gbuf *dst, struct gbuf *src, short x, short y);
void gbuf_blit_gimp_image(struct gbuf *g, struct gimp_image *img, short x, short y);
