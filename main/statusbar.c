#include "statusbar.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "../components/hardware/display.h"
#include "../components/hardware/sdcard.h"

#include "gbuf.h"
#include "OpenSans_Regular_11X12.h"
#include "icons_16X16.h"
#include "tf.h"

#include <string.h>


#define STATUSBAR_HEIGHT (16)

static bool task_running = false;
static struct {
    struct gbuf *fb;
    struct tf *tf;
} task_data;

static void statusbar_task(void *arg)
{
    while (task_running) {
        xSemaphoreTake(task_data.fb->mutex, portMAX_DELAY);
        memset(task_data.fb->pixel_data, 0, task_data.fb->width *
                STATUSBAR_HEIGHT * task_data.fb->bytes_per_pixel);

        task_data.tf->font = &font_icons_16X16;

        short x = task_data.fb->width - 16;
        tf_draw_glyph(task_data.fb, task_data.tf, FONT_ICON_BATTERY5, x, 0);
        x -= 16;
        if (sdcard_present()) {
            tf_draw_glyph(task_data.fb, task_data.tf, FONT_ICON_SDCARD, x, 0);
            x -= 16;
        }
        if (/*wifi_enabled()*/ 1) {
            tf_draw_glyph(task_data.fb, task_data.tf, FONT_ICON_WIFI4, x, 0);
            x -= 16;            
        }
        tf_draw_glyph(task_data.fb, task_data.tf, FONT_ICON_SPEAKER3, x, 0);
        x -= 16;

        display_update_rect(0, 0, task_data.fb->width, STATUSBAR_HEIGHT);
        xSemaphoreGive(task_data.fb->mutex);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    tf_free(task_data.tf);
}

void statusbar_init(struct gbuf *fb)
{
    task_running = true;

    task_data.fb = fb;
    task_data.tf = tf_new();
    task_data.tf->font = &font_OpenSans_Regular_11X12;

    xTaskCreate(statusbar_task, "statusbar", 1024, NULL, 5, NULL);
}

void statusbar_deinit(void)
{
    task_running = false;
}
