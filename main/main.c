#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "../components/hardware/display.h"
#include "../components/hardware/backlight.h"
#include "../components/hardware/keypad.h"
#include "../components/hardware/sdcard.h"

#include "app.h"
#include "gbuf.h"
#include "tf.h"
#include "OpenSans_Regular_11X12.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void app_main(void)
{
    nvs_flash_init();
    struct gbuf *fb = gbuf_new(DISPLAY_WIDTH, DISPLAY_HEIGHT, 2, BIG_ENDIAN, true);
    memset(fb->pixel_data, 0, fb->width * fb->height * fb->bytes_per_pixel);
    display_init();

    backlight_init();
    keypad_init();

    struct tf *tf = tf_new();
    tf->font = &font_OpenSans_Regular_11X12;

    char s[64];

    esp_err_t err = sdcard_init("/sd");
    if (err) {
        strcpy(s, "Card error.");
        struct tf_metrics m = tf_get_str_metrics(tf, s);
        tf_draw_str(fb, tf, s, DISPLAY_WIDTH/2 - m.width/2, DISPLAY_HEIGHT/2 - m.height/2);
        display_draw(fb);
        while (true) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }

    strcpy(s, "Press A to boot hello-world.bin app.");
    struct tf_metrics m = tf_get_str_metrics(tf, s);
    tf_draw_str(fb, tf, s, DISPLAY_WIDTH/2 - m.width/2, DISPLAY_HEIGHT/2 - m.height/2);
    display_draw(fb);

    uint16_t keypad = 0;
    while (!(keypad & KEYPAD_A)) {
        uint16_t sample = keypad_sample();
        keypad = keypad_debounce(sample, NULL);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    memset(fb->pixel_data, 0, fb->width * fb->height * fb->bytes_per_pixel);
    strcpy(s, "Loading...");
    m = tf_get_str_metrics(tf, s);
    tf_draw_str(fb, tf, s, DISPLAY_WIDTH/2 - m.width/2, DISPLAY_HEIGHT/2 - m.height/2);
    display_draw(fb);

    app_run("/sd/apps/hello-world.bin");
}
