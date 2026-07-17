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

// #include "ili9341_driver.h"
#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
// #include "esp_lcd_panel_st7789.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "xpt2046_touch_driver.h"
#include "ui_manager.h"
#include "bt_manager.h"
#include "audio_player.h"

#include "solo_sangeet.h"

#define TAG                     "MAIN_APP"
#define LV_TICK_PERIOD_MS       1
#define LV_TASK_MAX_DELAY_MS    500
#define LV_TASK_MIN_DELAY_MS    1000 / CONFIG_FREERTOS_HZ

// New code, from demo/example
// Using SPI2 in the example
#define LCD_HOST  SPI2_HOST

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (20 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_MISO           12
#define EXAMPLE_PIN_NUM_MOSI           13
#define EXAMPLE_PIN_NUM_SCLK           14
#define EXAMPLE_PIN_NUM_LCD_CS         15
#define EXAMPLE_PIN_NUM_LCD_DC         2
#define EXAMPLE_PIN_NUM_LCD_RST        -1
#define EXAMPLE_PIN_NUM_BK_LIGHT       27

// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES              240
#define EXAMPLE_LCD_V_RES              320
// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

#define EXAMPLE_LVGL_DRAW_BUF_LINES    20 // number of display lines in each draw buffer
#define EXAMPLE_LVGL_TICK_PERIOD_MS    2
#define EXAMPLE_LVGL_TASK_MAX_DELAY_MS 500
#define EXAMPLE_LVGL_TASK_MIN_DELAY_MS 1000 / CONFIG_FREERTOS_HZ

static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

/* Rotate display and touch, when rotated screen in LVGL. Called when driver parameters are updated. */
static void example_lvgl_port_update_callback(lv_display_t *disp)
{
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
    lv_display_rotation_t rotation = lv_display_get_rotation(disp);

    switch (rotation) {
    case LV_DISPLAY_ROTATION_0:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, true, false);
        break;
    case LV_DISPLAY_ROTATION_90:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, true, true);
        break;
    case LV_DISPLAY_ROTATION_180:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, false, true);
        break;
    case LV_DISPLAY_ROTATION_270:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, false, false);
        break;
    }
}

static void example_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    example_lvgl_port_update_callback(disp);
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // because SPI LCD is big-endian, we need to swap the RGB bytes order
    lv_draw_sw_rgb565_swap(px_map, (offsetx2 + 1 - offsetx1) * (offsety2 + 1 - offsety1));
    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
}

// Old code
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
 *********************
 static void ili9341_flush_cb(lv_display_t * display, const lv_area_t * area, uint8_t * px_map)
 {
    // Update screen with ili9341_flush_spi_dma_async for better performance, especially for larger areas.
    ili9341_flush_spi_dma_async(area->x1, area->y1, area->x2, area->y2, px_map);
    // Inform LVGL that flushing is complete so buffer can be modified again.
    lv_display_flush_ready(display);
}
*/

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
    ESP_LOGI(TAG, "RAW X=%u  Y=%u State: PRESSED", data->point.x, data->point.y);
    
    // Move the dot, for Touch Test UI
    if (touch_dot) {
        lv_obj_clear_flag(touch_dot, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(touch_dot, x - 6, y - 6);
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

/********************************************
 * LVGL Task to Updte/Refresh the display
 ********************************************/
void lvgl_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Starting LVGL Task...");
    
    ESP_LOGI(TAG, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    ESP_LOGI(TAG, "Initialize SPI bus");
    spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_PIN_NUM_SCLK,
        .mosi_io_num = EXAMPLE_PIN_NUM_MOSI,
        .miso_io_num = EXAMPLE_PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,    // EXAMPLE_LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = EXAMPLE_PIN_NUM_LCD_DC,
        .cs_gpio_num = EXAMPLE_PIN_NUM_LCD_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_HOST, &io_config, &io_handle));

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB, // LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_LOGI(TAG, "Install ILI9341 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    // ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    
    // Touch Screen SPI device configuration
    xpt2046_init(LCD_HOST);

    // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);

    /************************************************************************************* */
    // Start LVGL
    ESP_LOGI(TAG, "Initializing LVGL...");
    lv_init();
    lvgl_tick_init();
    
    // Display buffer
    size_t draw_buffer_sz = EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_DRAW_BUF_LINES * sizeof(lv_color16_t);

    void *buf1 = spi_bus_dma_memory_alloc(LCD_HOST, draw_buffer_sz, 0);
    void *buf2 = spi_bus_dma_memory_alloc(LCD_HOST, draw_buffer_sz, 0);
    // Use assert to ensure memory allocation was successful
    assert(buf1);
    assert(buf2);

    // Register the display driver with LVGL
    lv_display_t * active_disp = lv_display_create(EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    assert(active_disp != NULL);
    // set display rotation
    lv_display_set_rotation(active_disp, display_rotation); // Adjust rotation as needed
    // initialize LVGL draw buffers, and set render mode to PARTIAL (buffer size can be smaller than screen size)
    lv_display_set_buffers(active_disp, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);

    // associate the mipi panel handle to the display
    lv_display_set_user_data(active_disp, panel_handle);
    // set color depth
    lv_display_set_color_format(active_disp, LV_COLOR_FORMAT_RGB565);
    // set the callback which can copy the rendered image to an area of the display
    lv_display_set_flush_cb(active_disp, example_lvgl_flush_cb);

    ESP_LOGI(TAG, "Register io panel event callback for LVGL flush ready notification");
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = example_notify_lvgl_flush_ready,
    };
    /* Register done callback */
    ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, active_disp));
    
    // Setup input device (touchpad)
    lv_indev_t * active_indev = lv_indev_create();
    assert(active_indev != NULL);
    lv_indev_set_type(active_indev, LV_INDEV_TYPE_POINTER);         // Touch pad is a pointer-like device.
    lv_indev_set_read_cb(active_indev, xpt2046_touchpad_read_cb);   // Set driver function.
    lv_indev_set_display(active_indev, active_disp);

    // Fill background with BLACK
    // lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex3(COLOR_CYAN), LV_PART_MAIN);

    ESP_LOGI(TAG, "Memory Usage:");
    log_mem(TAG);

    // UI Initialization 
    _lock_acquire(&lvgl_api_lock);
    // ui_touch_debug_init();
    audio_player_ui_init(active_disp);
    // test_ui_init(active_disp);
    _lock_release(&lvgl_api_lock);

    log_mem(TAG);

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

/**********************
 * Main application
 **********************/
void app_main(void)
{
    ESP_LOGI(TAG, "Main stack free: %u\n", uxTaskGetStackHighWaterMark(NULL));

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
    
    // Start LVGL task
    xTaskCreatePinnedToCore(lvgl_task, "lvgl_task", 1024 * 64, NULL, configMAX_PRIORITIES - 1 , NULL, 1);
}