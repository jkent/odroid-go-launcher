#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"

#include "../components/hardware/display.h"
#include "../components/hardware/backlight.h"
#include "../components/hardware/keypad.h"

#include "gbuf.h"
#include "tf.h"
#include "OpenSans_Regular_11X12.h"

#include "image_cone.h"

#include <stdint.h>
#include <string.h>

static volatile uint16_t keypad_debounced = 0;

static void keypad_task(void *arg)
{
    uint16_t sample;
    
    while (true) {
        sample = keypad_sample();
        keypad_debounced = keypad_debounce(sample, NULL);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    struct gbuf *fb = display_init();
    memset(fb->pixel_data, 0, fb->width * fb->height * fb->bytes_per_pixel);
    backlight_init();
    keypad_init();

    struct tf *tf = tf_new();
    tf->font = &font_OpenSans_Regular_11X12;

    char s[64];
    strcpy(s, "Press A to boot hello-world.bin app.");
    struct tf_metrics m = tf_get_str_metrics(tf, s);
    tf_draw_str(fb, tf, s, DISPLAY_WIDTH/2 - m.width/2, DISPLAY_HEIGHT/2 - m.height/2);
    display_draw();

    xTaskCreatePinnedToCore(&keypad_task, "keypad_task", 1024, NULL, 5, NULL, APP_CPU_NUM);

    while (!(keypad_debounced & KEYPAD_A)) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    memset(fb->pixel_data, 0, fb->width * fb->height * fb->bytes_per_pixel);
    strcpy(s, "Loading...");
    m = tf_get_str_metrics(tf, s);
    tf_draw_str(fb, tf, s, DISPLAY_WIDTH/2 - m.width/2, DISPLAY_HEIGHT/2 - m.height/2);

    display_draw();
}
