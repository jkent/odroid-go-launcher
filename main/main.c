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
#include "statusbar.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void app_main(void)
{
    backlight_init();
    struct gbuf *fb = display_init();
    xSemaphoreTake(fb->mutex, portMAX_DELAY);
    memset(fb->pixel_data, 0, fb->width * fb->height * fb->bytes_per_pixel);
    display_update();
    xSemaphoreGive(fb->mutex);

    nvs_flash_init();
    sdcard_init("/sd");
    statusbar_init(fb);
    keypad_init();

    struct tf *tf = tf_new();
    tf->font = &font_OpenSans_Regular_11X12;

    char s[64];

    strcpy(s, "Press A to boot hello-world.bin app.");
    struct tf_metrics m = tf_get_str_metrics(tf, s);
    short x =  DISPLAY_WIDTH/2 - m.width/2;
    short y = DISPLAY_HEIGHT/2 - m.height/2;
    xSemaphoreTake(fb->mutex, portMAX_DELAY);
    tf_draw_str(fb, tf, s, x, y);
    display_update_rect(x, y, m.width, m.height);
    xSemaphoreGive(fb->mutex);

    uint16_t keypad = 0;
    while (!(keypad & KEYPAD_A)) {
        uint16_t sample = keypad_sample();
        keypad = keypad_debounce(sample, NULL);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    memset(fb->pixel_data, 0, fb->width * fb->height * fb->bytes_per_pixel);
    strcpy(s, "Loading...");
    m = tf_get_str_metrics(tf, s);
    x = DISPLAY_WIDTH/2 - m.width/2;
    y = DISPLAY_HEIGHT/2 - m.height/2;
    xSemaphoreTake(fb->mutex, portMAX_DELAY);
    tf_draw_str(fb, tf, s, x, y);
    display_update();
    xSemaphoreGive(fb->mutex);

    app_run("/sd/apps/hello-world.bin");

    memset(fb->pixel_data, 0, fb->width * fb->height * fb->bytes_per_pixel);
    strcpy(s, "App not found.");
    m = tf_get_str_metrics(tf, s);
    x = DISPLAY_WIDTH/2 - m.width/2;
    y = DISPLAY_HEIGHT/2 - m.height/2;
    xSemaphoreTake(fb->mutex, portMAX_DELAY);
    tf_draw_str(fb, tf, s, x, y);
    display_update();
    xSemaphoreGive(fb->mutex);
}
