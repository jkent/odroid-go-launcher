#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "../components/hardware/display.h"
#include "../components/hardware/backlight.h"
#include "../components/hardware/keypad.h"
#include "../components/hardware/sdcard.h"

#include "app.h"
#include "graphics.h"
#include "menu.h"
#include "tf.h"
#include "OpenSans_Regular_11X12.h"
#include "statusbar.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void app_main(void)
{
    struct gbuf_t *fb = display_init();
    backlight_init();

    nvs_flash_init();
    sdcard_init("/sd");
    statusbar_init(fb);
    keypad_init();

    struct tf_t *tf = tf_new(&font_OpenSans_Regular_11X12, 240, TF_ALIGN_CENTER | TF_WORDWRAP);

    const char *s;
    uint16_t keypad = 0;
    struct tf_metrics_t m;
    struct point_t p;

    while (true) {
        s = "Press Menu button for menu or A to boot hello-world.bin app.";
        m = tf_get_str_metrics(tf, s);
        p.x = DISPLAY_WIDTH/2 - tf->width/2;
        p.y = DISPLAY_HEIGHT/2 - m.height/2;
        xSemaphoreTake(fb->mutex, portMAX_DELAY);
        memset(fb->pixel_data + fb->width * 16 * fb->bytes_per_pixel, 0, fb->width * (fb->height - 32) * fb->bytes_per_pixel);
        tf_draw_str(fb, tf, s, p);
        display_update();
        xSemaphoreGive(fb->mutex);

        do {
            uint16_t sample = keypad_sample();
            keypad = keypad_debounce(sample, NULL);
            vTaskDelay(10 / portTICK_PERIOD_MS);
        } while (!(keypad & KEYPAD_MENU) && !(keypad & KEYPAD_A));

        if (keypad & KEYPAD_A) {
            break;
        }

        struct menu_t *menu = menu_new(fb, 240, 180);
        menu_append(menu, "The quick brown fox jumps over the lazy dog.", NULL, NULL);
        menu_append(menu, "Hello World!", NULL, NULL);
        menu_append(menu, "3nd line", NULL, NULL);
        menu_showmodal(menu);
        menu_free(menu);
    }

    s = "Loading...";
    m = tf_get_str_metrics(tf, s);
    p.x = DISPLAY_WIDTH/2 - tf->width/2;
    p.y = DISPLAY_HEIGHT/2 - m.height/2;
    xSemaphoreTake(fb->mutex, portMAX_DELAY);
    memset(fb->pixel_data + fb->width * 16 * fb->bytes_per_pixel , 0, fb->width * (fb->height - 32) * fb->bytes_per_pixel);
    tf_draw_str(fb, tf, s, p);
    display_update();
    xSemaphoreGive(fb->mutex);

    app_run("/sd/apps/hello-world.bin");

    s = "App not found.";
    m = tf_get_str_metrics(tf, s);
    p.x = DISPLAY_WIDTH/2 - tf->width/2;
    p.y = DISPLAY_HEIGHT/2 - m.height/2;
    xSemaphoreTake(fb->mutex, portMAX_DELAY);
    memset(fb->pixel_data + fb->width * 16 * fb->bytes_per_pixel , 0, fb->width * (fb->height - 32) * fb->bytes_per_pixel);
    tf_draw_str(fb, tf, s, p);
    display_update();
    xSemaphoreGive(fb->mutex);
}
