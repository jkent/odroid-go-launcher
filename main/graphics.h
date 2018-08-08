#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <stdbool.h>
#include <stdint.h>
#include <machine/endian.h>

#include "gbuf.h"
#include "point.h"
#include "rect.h"


enum draw_type_t {
    DRAW_TYPE_FILL = 0,
    DRAW_TYPE_OUTLINE
};

void blit(gbuf_t *dst, rect_t dst_rect, gbuf_t *src, rect_t src_rect);
void draw_rectangle(gbuf_t *dst, rect_t rect, enum draw_type_t draw_type, uint16_t color);
