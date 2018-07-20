#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "../components/hardware/display.h"
#include "../components/hardware/backlight.h"

#include "tf.h"
#include "OpenSans_Regular_11X12.h"

#include "gfx.h"
#include "image_cone.h"

void app_main(void)
{
    nvs_flash_init();
    display_init();
    backlight_init();

    uint16_t *fb = calloc(1, DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t));
    gfx_blit_image(fb, (struct gfx_image *)&image_cone, DISPLAY_WIDTH/2 - image_cone.width/2, DISPLAY_HEIGHT/2 - image_cone.height/2);

    struct tf *tf = tf_new();
    tf->font = &font_OpenSans_Regular_11X12;
    tf->bbox_width = 100;
    tf->fb = fb;
    tf->align = ALIGN_CENTER;
    tf_draw_str(tf, "The quick brown fox jumps over the lazy dog.", 0, 0);
    tf_free(tf);

    display_write_all(fb);
}
