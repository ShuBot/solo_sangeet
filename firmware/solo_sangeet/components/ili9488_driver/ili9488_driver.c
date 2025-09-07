#include "ili9488_driver.h"
#include <stdio.h>

spi_device_handle_t ili9488_spi;

void ili9488_spi_config() {
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,          // Not using MISO (display doesn’t send data back)
        .mosi_io_num = ILI9488_PIN_MOSI,
        .sclk_io_num = ILI9488_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 307200 // 320*480*2 bytes
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 20 * 1000 * 1000, // 20 MHz
        .mode = 0,                          // SPI mode 0
        .spics_io_num = ILI9488_PIN_CS,
        .queue_size = 7,                    // Max 7 transactions in queue
        .flags = SPI_DEVICE_NO_DUMMY,
        .pre_cb = NULL,
        .post_cb = NULL
    };

    // Initialize SPI bus
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    assert(ret == ESP_OK);

    // Add device to bus
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &ili9488_spi);
    assert(ret == ESP_OK);

    // Set up DC and RST as GPIO
    gpio_set_direction(ILI9488_PIN_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(ILI9488_PIN_RST, GPIO_MODE_OUTPUT);
}

void ili9488_send_command(uint8_t cmd) {
    gpio_set_level(ILI9488_PIN_DC, 0); // Command mode
    spi_transaction_t t = {
        .length = 8,           // 8 bits
        .tx_buffer = &cmd,
        .flags = 0
    };
    esp_err_t ret = spi_device_transmit(ili9488_spi, &t);
    assert(ret == ESP_OK);
}

void ili9488_send_data(uint8_t data) {
    gpio_set_level(ILI9488_PIN_DC, 1); // Data mode
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data,
        .flags = 0
    };
    esp_err_t ret = spi_device_transmit(ili9488_spi, &t);
    assert(ret == ESP_OK);
}

void ili9488_send_data_buffer(const uint8_t *data, size_t len)
{
    gpio_set_level(ILI9488_PIN_DC, 1); // Data mode
    spi_transaction_t t = {
        .length = len * 8,  // bits
        .tx_buffer = data
    };
    spi_device_transmit(ili9488_spi, &t);
}

void ili9488_init() {
    // Configure SPI
    ili9488_spi_config();
    
    // Chip Select HIGH
    gpio_set_level(ILI9488_PIN_CS, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    
    // Set DC HIGH for data mode by default
    gpio_set_level(ILI9488_PIN_DC, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    
    // Reset the display
    gpio_set_level(ILI9488_PIN_RST, 0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(ILI9488_PIN_RST, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    
    // Basic init sequence (from ILI9488 datasheet)
    
    // Software reset
    ili9488_send_command(0x01);
    vTaskDelay(150 / portTICK_PERIOD_MS);

    // Gamma settings
    ili9488_send_command(0xE0); // Positive gamma
    ili9488_send_data(0x00);
    ili9488_send_data(0x03);
    ili9488_send_data(0x09);
    ili9488_send_data(0x08);
    ili9488_send_data(0x16);
    ili9488_send_data(0x0A);
    ili9488_send_data(0x3F);
    ili9488_send_data(0x78);
    ili9488_send_data(0x4C);
    ili9488_send_data(0x09);
    ili9488_send_data(0x0A);
    ili9488_send_data(0x08);
    ili9488_send_data(0x16);
    ili9488_send_data(0x1A);
    ili9488_send_data(0x0F);

    ili9488_send_command(0xE1); // Negative gamma
    ili9488_send_data(0x00);
    ili9488_send_data(0x16);
    ili9488_send_data(0x19);
    ili9488_send_data(0x03);
    ili9488_send_data(0x0F);
    ili9488_send_data(0x05);
    ili9488_send_data(0x32);
    ili9488_send_data(0x45);
    ili9488_send_data(0x46);
    ili9488_send_data(0x04);
    ili9488_send_data(0x0E);
    ili9488_send_data(0x0D);
    ili9488_send_data(0x35);
    ili9488_send_data(0x37);
    ili9488_send_data(0x0F);

    // Power control
    ili9488_send_command(0xC0); // Power Control 1
    ili9488_send_data(0x17);
    ili9488_send_data(0x15);
    ili9488_send_command(0xC1); // Power Control 2
    ili9488_send_data(0x41);
    ili9488_send_command(0xC5); // VCOM Control
    ili9488_send_data(0x00);
    ili9488_send_data(0x12);
    ili9488_send_data(0x80);

    ili9488_send_command(0x36); // Memory Access Control
    ili9488_send_data(0x48);          // MX, BGR
    
    // Pixel format (16-bit RGB565)
    ili9488_send_command(0x3A);
    ili9488_send_data(0x55); // 16-bit/pixel
    // ili9488_send_data(0x66);  // 18-bit colour for SPI
    
    ili9488_send_command(0xB0); // Interface Mode Control
    ili9488_send_data(0x00);

    ili9488_send_command(0xB1); // Frame Rate Control
    ili9488_send_data(0xA0);

    ili9488_send_command(0xB4); // Display Inversion Control
    ili9488_send_data(0x02);

    ili9488_send_command(0xB6); // Display Function Control
    ili9488_send_data(0x02);
    ili9488_send_data(0x02);
    ili9488_send_data(0x3B);

    ili9488_send_command(0xB7); // Entry Mode Set
    ili9488_send_data(0xC6);

    ili9488_send_command(0xF7); // Adjust Control 3
    ili9488_send_data(0xA9);
    ili9488_send_data(0x51);
    ili9488_send_data(0x2C);
    ili9488_send_data(0x82);

    // Set default rotation
    // ili9488_set_rotation(ILI9488_ROTATION_0);

    // Exit sleep mode
    ili9488_send_command(0x11);
    vTaskDelay(150 / portTICK_PERIOD_MS);

    // Display on
    ili9488_send_command(0x29);
    vTaskDelay(50 / portTICK_PERIOD_MS);
}

esp_err_t ili9488_set_rotation(ili9488_rotation_t rotation) {
    
    uint8_t madctl;
    
    switch (rotation) {
        case ILI9488_ROTATION_0:
            madctl = (MADCTL_MX | MADCTL_RGB); // MY=0, MX=1, MV=0
            break;
        case ILI9488_ROTATION_90:
            madctl = (MADCTL_MV | MADCTL_RGB);; // MY=0, MX=0, MV=1
            break;
        case ILI9488_ROTATION_180:
            madctl = (MADCTL_MY | MADCTL_RGB);; // MY=1, MX=0, MV=0
            break;
        case ILI9488_ROTATION_270:
            madctl = (MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_RGB);; // MY=1, MX=1, MV=1
            break;
        default:
            return ESP_ERR_INVALID_ARG;
    }

    ili9488_send_command(ILI9488_MADCTL); // MADCTL
    ili9488_send_data(madctl);
    return ESP_OK;
}

void ili9488_fill_screen(uint16_t color) {
    ili9488_send_command(0x2A); // Column address set
    ili9488_send_data(0x00); ili9488_send_data(0x00); // Start: 0
    ili9488_send_data(0x01); ili9488_send_data(0x3F); // End: 319 (320 pixels wide) 0x3F

    ili9488_send_command(0x2B); // Row address set
    ili9488_send_data(0x00); ili9488_send_data(0x00); // Start: 0
    ili9488_send_data(0x01); ili9488_send_data(0xDF); // End: 479 (490 pixels tall) 0xDF

    ili9488_send_command(0x2C); // Memory write

    // Prepare pixel data (big-endian for SPI)
    uint16_t color_bytes = __builtin_bswap16(color);
    uint8_t color_high = color_bytes >> 8;
    uint8_t color_low = color_bytes & 0xFF;

    // Send pixel-by-pixel
    size_t total_pixels = ILI9488_DISPLAY_SIZE;
    for (size_t i = 0; i < total_pixels; i++) {
        ili9488_send_data(color_high);
        ili9488_send_data(color_low);
        if ((i % 100) == 0) { // Yield every 100 pixels
            vTaskDelay(1 / portTICK_PERIOD_MS);
            ESP_LOGD(DRV_TAG, "Sent %d/%d pixels", i, total_pixels);
        }
    }

    // for (int i = 0; i < 10 * ILI9488_DISP_VER_RES; i++) {
    //     ili9488_send_data(color >> 8);   // High byte
    //     ili9488_send_data(color & 0xFF); // Low byte
    // }
    
    // Create a small buffer to send data in chunks
    // #define CHUNK_SIZE 1024 // 4KB chunks
    // // uint8_t buffer[CHUNK_SIZE];
    // uint8_t *buffer = (uint8_t *)malloc(CHUNK_SIZE);
    // // if (!buffer) {
    // //     ESP_LOGE(DRV_TAG, "Failed to allocate buffer");
    // //     return -1; // Return error if malloc fails
    // // }

    // // Fill buffer with color (big-endian for SPI)
    // uint16_t color_bytes = __builtin_bswap16(color);
    // for (int i = 0; i < CHUNK_SIZE / 2; i++) {
    //     buffer[i * 2] = color_bytes >> 8;
    //     buffer[i * 2 + 1] = color_bytes & 0xFF;
    // }

    // // Send data in chunks to avoid WDT timeout
    // size_t total_pixels = ILI9488_DISPLAY_SIZE;
    // size_t pixels_per_chunk = CHUNK_SIZE / 2;
    // for (size_t i = 0; i < total_pixels; i += pixels_per_chunk) {
    //     size_t pixels_to_send = (i + pixels_per_chunk > total_pixels) ? (total_pixels - i) : pixels_per_chunk;
    //     ili9488_send_data_buffer(buffer, pixels_to_send * 2);
    //     vTaskDelay(1 / portTICK_PERIOD_MS); // Yield to reset WDT
    // }
    
    // free(buffer); // Free heap memory
    // return ESP_OK;
}

void ili9488_fill_screen_white() {
    uint16_t color = 0xFFFF;
    ili9488_fill_screen(color);
}

esp_err_t ili9488_draw_thick_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t thickness, uint16_t color)
{
    
    if (x1 >= ILI9488_DISP_HOR_RES || x2 >= ILI9488_DISP_HOR_RES || y1 >= ILI9488_DISP_VER_RES || y2 >= ILI9488_DISP_VER_RES) {
        ESP_LOGE(DRV_TAG, "Line coordinates out of bounds");
        return ESP_ERR_INVALID_ARG;
    }
    if (thickness == 0) {
        ESP_LOGE(DRV_TAG, "Invalid line thickness");
        return ESP_ERR_INVALID_ARG;
    }

    // Log stack and heap usage
    ESP_LOGI(DRV_TAG, "Drawing thick line: (%d,%d) to (%d,%d), thickness=%d, color=0x%04X", x1, y1, x2, y2, thickness, color);
    ESP_LOGI(DRV_TAG, "Stack high water mark: %d bytes", uxTaskGetStackHighWaterMark(NULL));
    ESP_LOGI(DRV_TAG, "Free heap: %d bytes", heap_caps_get_free_size(MALLOC_CAP_8BIT));

    // Determine line orientation and bounds
    uint16_t x_start, x_end, y_start, y_end;
    bool is_horizontal = (y1 == y2);
    if (is_horizontal) {
        x_start = (x1 < x2) ? x1 : x2;
        x_end = (x1 < x2) ? x2 : x1;
        y_start = (y1 > thickness / 2) ? y1 - thickness / 2 : 0;
        y_end = (y1 + thickness / 2 < ILI9488_DISP_VER_RES) ? y1 + thickness / 2 : ILI9488_DISP_VER_RES - 1;
    } else if (x1 == x2) {
        y_start = (y1 < y2) ? y1 : y2;
        y_end = (y1 < y2) ? y2 : y1;
        x_start = (x1 > thickness / 2) ? x1 - thickness / 2 : 0;
        x_end = (x1 + thickness / 2 < ILI9488_DISP_HOR_RES) ? x1 + thickness / 2 : ILI9488_DISP_HOR_RES - 1;
    } else {
        ESP_LOGE(DRV_TAG, "Only horizontal or vertical lines supported");
        return ESP_ERR_INVALID_ARG;
    }

    // Set column address
    ili9488_send_command(0x2A);
    ili9488_send_data(x_start >> 8);
    ili9488_send_data(x_start & 0xFF);
    ili9488_send_data(x_end >> 8);
    ili9488_send_data(x_end & 0xFF);

    // Set row address
    ili9488_send_command(0x2B);
    ili9488_send_data(y_start >> 8);
    ili9488_send_data(y_start & 0xFF);
    ili9488_send_data(y_end >> 8);
    ili9488_send_data(y_end & 0xFF);

    // Memory write
    ili9488_send_command(0x2C);

    // Allocate buffer for chunked transfer
    #define CHUNK_SIZE 1024 // 1KB (512 pixels)
    uint8_t *buffer = (uint8_t *)malloc(CHUNK_SIZE);
    if (!buffer) {
        ESP_LOGE(DRV_TAG, "Failed to allocate buffer");
        return ESP_ERR_NO_MEM;
    }

    // Fill buffer with color (big-endian)
    uint16_t color_bytes = __builtin_bswap16(color);
    for (int i = 0; i < CHUNK_SIZE / 2; i++) {
        buffer[i * 2] = color_bytes >> 8;
        buffer[i * 2 + 1] = color_bytes & 0xFF;
    }

    // Calculate total pixels in the line region
    size_t total_pixels = (x_end - x_start + 1) * (y_end - y_start + 1);
    size_t pixels_per_chunk = CHUNK_SIZE / 2;
    for (size_t i = 0; i < total_pixels; i += pixels_per_chunk) {
        size_t pixels_to_send = (i + pixels_per_chunk > total_pixels) ? (total_pixels - i) : pixels_per_chunk;
        ili9488_send_data_buffer(buffer, pixels_to_send * 2);
        vTaskDelay(2 / portTICK_PERIOD_MS);
        ESP_LOGD(DRV_TAG, "Sent %d/%d pixels for line", i + pixels_to_send, total_pixels);
    }

    free(buffer);
    ESP_LOGI(DRV_TAG, "Line drawn. Stack high water mark: %d bytes", uxTaskGetStackHighWaterMark(NULL));
    return ESP_OK;
}