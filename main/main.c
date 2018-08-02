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
    backlight_init();
    struct gbuf_t *fb = display_init();
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

    uint16_t keypad = 0;

    strcpy(s, "Press Menu button to show menu. Press B to hide it.");
    struct tf_metrics m = tf_get_str_metrics(tf, s);
    struct point_t p = {DISPLAY_WIDTH/2 - m.width/2, DISPLAY_HEIGHT/2 - m.height/2};
    xSemaphoreTake(fb->mutex, portMAX_DELAY);
    tf_draw_str(fb, tf, s, p);
    struct rect_t r = {p.x, p.y, m.width, m.height};
    display_update_rect(r);
    xSemaphoreGive(fb->mutex);

    while (!(keypad & KEYPAD_A)) {
        uint16_t sample = keypad_sample();
        keypad = keypad_debounce(sample, NULL);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    struct menu_t *menu = menu_new(fb, 300, 200);
    menu_show(menu);

    while (!(keypad & KEYPAD_B)) {
        uint16_t sample = keypad_sample();
        keypad = keypad_debounce(sample, NULL);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    menu_hide(menu);

    strcpy(s, "Press A to boot hello-world.bin app.");
    m = tf_get_str_metrics(tf, s);
    p.x =  DISPLAY_WIDTH/2 - m.width/2;
    p.y = DISPLAY_HEIGHT/2 - m.height/2;
    xSemaphoreTake(fb->mutex, portMAX_DELAY);
    memset(fb->pixel_data, 0, fb->width * fb->height * fb->bytes_per_pixel);
    tf_draw_str(fb, tf, s, p);
    display_update();
    xSemaphoreGive(fb->mutex);

    while (!(keypad & KEYPAD_A)) {
        uint16_t sample = keypad_sample();
        keypad = keypad_debounce(sample, NULL);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    strcpy(s, "Loading...");
    m = tf_get_str_metrics(tf, s);
    p.x = DISPLAY_WIDTH/2 - m.width/2;
    p.y = DISPLAY_HEIGHT/2 - m.height/2;
    xSemaphoreTake(fb->mutex, portMAX_DELAY);
    memset(fb->pixel_data, 0, fb->width * fb->height * fb->bytes_per_pixel);
    tf_draw_str(fb, tf, s, p);
    display_update();
    xSemaphoreGive(fb->mutex);

    app_run("/sd/apps/hello-world.bin");

    strcpy(s, "App not found.");
    m = tf_get_str_metrics(tf, s);
    p.x = DISPLAY_WIDTH/2 - m.width/2;
    p.y = DISPLAY_HEIGHT/2 - m.height/2;
    xSemaphoreTake(fb->mutex, portMAX_DELAY);
    memset(fb->pixel_data, 0, fb->width * fb->height * fb->bytes_per_pixel);
    tf_draw_str(fb, tf, s, p);
    display_update();
    xSemaphoreGive(fb->mutex);
}
