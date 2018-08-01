#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <stdbool.h>
#include <stdint.h>
#include <machine/endian.h>


struct point_t {
    short x, y;
};

struct rect_t {
    short x, y, width, height;
};

struct gbuf_t {
    uint16_t width;
    uint16_t height;
    uint16_t bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */  
    uint16_t endian;
    SemaphoreHandle_t mutex;
    uint8_t pixel_data[];
};

struct image_t {
  uint32_t width;
  uint32_t height;
  uint32_t bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ 
  uint8_t  pixel_data[];
};

enum draw_type_t {
    DRAW_TYPE_FILL = 0,
    DRAW_TYPE_OUTLINE
};

struct gbuf_t *gbuf_new(uint16_t width, uint16_t height, uint16_t bytes_per_pixel, uint16_t endian);
void gbuf_free(struct gbuf_t *g);
void blit(struct gbuf_t *dst, struct rect_t dst_rect, struct gbuf_t *src, struct rect_t src_rect);
void draw_image(struct gbuf_t *dst, struct image_t *src, short x, short y);
void draw_rectangle(struct gbuf_t *dst, struct rect_t rect, enum draw_type_t draw_type, uint16_t color);
