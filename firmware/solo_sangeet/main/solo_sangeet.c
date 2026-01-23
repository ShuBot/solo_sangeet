#include <stdio.h>
#include <unistd.h>
#include <sys/lock.h>
#include <sys/param.h>

#include "lvgl.h"
#include "lv_examples.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "ili9341_driver.h"
#include "xpt2046_touch_driver.h"
#include "bt_audio.h"
#include "audio_player.h"

#define TAG                     "MAIN_APP"
#define LV_TICK_PERIOD_MS       1
#define LV_TASK_MAX_DELAY_MS    500
#define LV_TASK_MIN_DELAY_MS    1000 / CONFIG_FREERTOS_HZ

lv_display_rotation_t display_rotation = LV_DISPLAY_ROTATION_180;

// LVGL library is not thread-safe, this example will call LVGL APIs from different tasks, so use a mutex to protect it
static _lock_t lvgl_api_lock;

/**********************
 * Touch Debug Dot
 **********************/
static lv_obj_t * touch_dot = NULL;
void ui_touch_debug_init(void)
{
    lv_obj_t * scr = lv_scr_act();

    /* Create a small dot */
    touch_dot = lv_obj_create(scr);
    lv_obj_set_size(touch_dot, 12, 12);
    lv_obj_set_style_radius(touch_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(touch_dot, lv_color_white(), 0);
    lv_obj_set_style_border_width(touch_dot, 0, 0);

    /* Start hidden */
    lv_obj_add_flag(touch_dot, LV_OBJ_FLAG_HIDDEN);
}

/**********************
 * LVGL flush callback
 **********************/
static void ili9341_flush_cb(lv_display_t * display, const lv_area_t * area, uint8_t * px_map)
{
    // ili9341_flush_spi(area->x1, area->y1, area->x2, area->y2, px_map);
    ili9341_flush_spi_dma(area->x1, area->y1, area->x2, area->y2, px_map);
    //Inform LVGL that flushing is complete so buffer can be modified again.
    lv_display_flush_ready(display);
}

/**********************
 * LVGL touchpad read callback
 **********************/
static inline int map_u16(uint16_t v, uint16_t in_min, uint16_t in_max, int out_max)
{
    if (v < in_min) v = in_min;
    if (v > in_max) v = in_max;
    return (int)((v - in_min) * out_max / (in_max - in_min));
}

static void xpt2046_touchpad_read_cb(lv_indev_t * indev, lv_indev_data_t * data)
{
    uint16_t xr, yr;

    if (!xpt2046_read_raw(&xr, &yr)) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    #if TOUCH_SWAP_XY
        uint16_t t = xr;
        xr = yr;
        yr = t;
    #endif

        int x = map_u16(xr, TOUCH_X_MIN, TOUCH_X_MAX, 240);
        int y = map_u16(yr, TOUCH_Y_MIN, TOUCH_Y_MAX, 320);

    #if TOUCH_INVERT_X
        x = 239 - x;
    #endif
    #if TOUCH_INVERT_Y
        y = 319 - y;
    #endif

    /* Clamp */
    if (x < 0) x = 0;
    if (x > 239) x = 239;
    if (y < 0) y = 0;
    if (y > 319) y = 319;

    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = x;
    data->point.y = y;
    // ESP_LOGI(TAG, "RAW X=%u  Y=%u State: PRESSED", data->point.x, data->point.y);
    
    // Move the dot, for Touch Test UI
    if (touch_dot) {
        lv_obj_clear_flag(touch_dot, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(touch_dot, x - 6, y - 6);
    }
}

/********************************************
 * LVGL Task to Updte/Refresh the display
 ********************************************/
void lvgl_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Starting LVGL Task...");
    
    // while (1) {
    //     lv_timer_handler();     // LVGL tasks (animations, redraw) 
    //     vTaskDelay(pdMS_TO_TICKS()); // Yield to avoid WDT reset
    //     LV_TASK_MAX_DELAY_MS
    // }
    uint32_t time_till_next_ms = 0;
    while (1) {
        _lock_acquire(&lvgl_api_lock);
        time_till_next_ms = lv_timer_handler();
        _lock_release(&lvgl_api_lock);
        // in case of triggering a task watch dog time out
        time_till_next_ms = MAX(time_till_next_ms, LV_TASK_MIN_DELAY_MS);
        // in case of lvgl display not ready yet
        time_till_next_ms = MIN(time_till_next_ms, LV_TASK_MAX_DELAY_MS);
        usleep(1000 * time_till_next_ms);
    }
}

/********************************************
 * LVGL Tick Initialization
 ********************************************/
static void lv_tick_cb(void *arg)
{
    lv_tick_inc(LV_TICK_PERIOD_MS);  // 1 ms tick
}

void lvgl_tick_init(void)
{
    const esp_timer_create_args_t tick_args = {
        .callback = &lv_tick_cb,
        .name = "lvgl_tick"
    };

    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, 1000)); // 1 ms
}

/**********************
 * Main application
 **********************/
void app_main(void)
{
    ESP_LOGI(TAG, "Initializing Display SPI Drivers...");
    ili9341_init();

    // Set Display Rotation
    ili9341_set_rotation(display_rotation);
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for the display to stabilize

    /*
    // Test Colors with solid fills
    ESP_LOGI(TAG, "Testing Display with Solid Color Fills...");
    ili9341_fill_screen(display_rotation,COLOR_BLACK);
    vTaskDelay(pdMS_TO_TICKS(500)); // Wait for the display to stabilize
    
    // Draw rectangles
    ili9341_fill_rect_dma(display_rotation, 20, 20, 100, 60, 0xF800); // red
    vTaskDelay(pdMS_TO_TICKS(300));

    ili9341_fill_rect_dma(display_rotation, 60, 80, 120, 80, 0x07E0); // green
    vTaskDelay(pdMS_TO_TICKS(300));

    ili9341_fill_rect_dma(display_rotation, 10, 200, 200, 30, 0x001F); // blue
    vTaskDelay(pdMS_TO_TICKS(300));
    
    ili9341_fill_screen(display_rotation, COLOR_RED);
    vTaskDelay(pdMS_TO_TICKS(500)); // Wait for the display to stabilize

    ili9341_fill_screen(display_rotation, COLOR_GREEN);
    vTaskDelay(pdMS_TO_TICKS(500)); // Wait for the display to stabilize
    
    ili9341_fill_screen(display_rotation, COLOR_BLUE);
    vTaskDelay(pdMS_TO_TICKS(500)); // Wait for the display to stabilize
    
    ili9341_fill_screen_white(display_rotation);
    vTaskDelay(pdMS_TO_TICKS(500)); // Wait for the display to stabilize
    */
   
    // Audio Player Initialization
    audio_player_init();

    // Start BT Audio task
    bt_audio_task();

    // Start LVGL
    ESP_LOGI(TAG, "Initializing LVGL...");
    lv_init();
    lvgl_tick_init();
    
    // Display buffer
    size_t draw_buffer_sz = ILI9341_DISPLAY_BUFFER_SIZE * sizeof(lv_color_t);
    static lv_color_t * draw_buf1;
    static lv_color_t * draw_buf2;

    draw_buf1 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    draw_buf2 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    // Use assert to ensure memory allocation was successful
    assert(draw_buf1);
    assert(draw_buf2);

    // Register the display driver with LVGL
    lv_display_t * active_disp = lv_display_create(ILI9341_DISP_HOR_RES, ILI9341_DISP_VER_RES);
    assert(active_disp != NULL);
    // set display rotation
    lv_display_set_rotation(active_disp, display_rotation); // Adjust rotation as needed
    // initialize LVGL draw buffers
    lv_display_set_buffers(active_disp, draw_buf1, draw_buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
    // set color depth
    lv_display_set_color_format(active_disp, LV_COLOR_FORMAT_RGB565);
    // set the callback which can copy the rendered image to an area of the display
    lv_display_set_flush_cb(active_disp, ili9341_flush_cb);
    
    // Setup input device (touchpad)
    lv_indev_t * active_indev = lv_indev_create();
    assert(active_indev != NULL);
    lv_indev_set_type(active_indev, LV_INDEV_TYPE_POINTER);         // Touch pad is a pointer-like device.
    lv_indev_set_read_cb(active_indev, xpt2046_touchpad_read_cb);   // Set driver function.
    lv_indev_set_display(active_indev, active_disp);

    // Fill background with BLACK
    // lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex3(COLOR_CYAN), LV_PART_MAIN);
    
    // UI Initialization 
    _lock_acquire(&lvgl_api_lock);
    // ui_touch_debug_init();
    audio_player_ui_init(active_disp);
    _lock_release(&lvgl_api_lock);
    
    // Start LVGL task
    xTaskCreatePinnedToCore(lvgl_task, "lvgl_task", 1024 * 32, NULL, configMAX_PRIORITIES - 1 , NULL, 1);
}