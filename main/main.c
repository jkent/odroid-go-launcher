#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "../components/hardware/display.h"
#include "../components/hardware/backlight.h"

#include "gbuf.h"
#include "tf.h"
#include "OpenSans_Regular_11X12.h"

#include "image_cone.h"

void app_main(void)
{
    nvs_flash_init();
    display_init();
    backlight_init();

    struct gbuf *fb = gbuf_new(DISPLAY_WIDTH, DISPLAY_HEIGHT, 2, BIG_ENDIAN);
    short x = DISPLAY_WIDTH/2 - image_cone.width/2;
    short y = DISPLAY_HEIGHT/2 - image_cone.height/2;
    gbuf_blit_gimp_image(fb, (struct gimp_image *)&image_cone, x, y);

    struct tf *tf = tf_new();
    tf->font = &font_OpenSans_Regular_11X12;
    tf->width = 150;
    tf->align = ALIGN_CENTER;
    tf->bg_color = 0b1111100000000000;
    tf->fill_bg = true;

    x = DISPLAY_WIDTH/2 - tf->width/2;
    y = 10;

    tf_draw_str(fb, tf, "The quick brown fox jumps over the lazy dog.", x, y);
    tf_free(tf);

    display_write_all(fb);
}
