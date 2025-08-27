#include "st7735_driver.h"
#include <stdio.h>

spi_device_handle_t st7735_spi;

void st7735_spi_config() {
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,          // Not using MISO (display doesn’t send data back)
        .mosi_io_num = ST7735_PIN_MOSI,
        .sclk_io_num = ST7735_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000, // 10 MHz
        .mode = 0,                          // SPI mode 0
        .spics_io_num = ST7735_PIN_CS,
        .queue_size = 7
    };

    // Initialize SPI bus
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    assert(ret == ESP_OK);

    // Add device to bus
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &st7735_spi);
    assert(ret == ESP_OK);

    // Set up DC and RST as GPIO
    gpio_set_direction(ST7735_PIN_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(ST7735_PIN_RST, GPIO_MODE_OUTPUT);
}

void st7735_send_command(uint8_t cmd) {
    gpio_set_level(ST7735_PIN_DC, 0); // Command mode
    spi_transaction_t t = {
        .length = 8,           // 8 bits
        .tx_buffer = &cmd,
        .flags = 0
    };
    esp_err_t ret = spi_device_transmit(st7735_spi, &t);
    assert(ret == ESP_OK);
}

void st7735_send_data(uint8_t data) {
    gpio_set_level(ST7735_PIN_DC, 1); // Data mode
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data,
        .flags = 0
    };
    esp_err_t ret = spi_device_transmit(st7735_spi, &t);
    assert(ret == ESP_OK);
}

void st7735_send_data_bytes(const uint8_t *data, size_t len)
{
    gpio_set_level(ST7735_PIN_DC, 1); // Data mode
    spi_transaction_t t = {
        .length = len * 8,  // bits
        .tx_buffer = data
    };
    spi_device_transmit(st7735_spi, &t);
}

void st7735_init() {
    // Configure SPI
    st7735_spi_config();
    
    // Reset the display
    gpio_set_level(ST7735_PIN_RST, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(ST7735_PIN_RST, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    // Basic init sequence (from ST7735 datasheet)
    st7735_send_command(0x01); // Software reset
    vTaskDelay(150 / portTICK_PERIOD_MS);

    st7735_send_command(0x11); // Sleep out
    vTaskDelay(500 / portTICK_PERIOD_MS);

    st7735_send_command(0x3A); // Set color mode
    st7735_send_data(0x05);    // 16-bit RGB565

    // Set default rotation
    st7735_set_rotation(ST7735_ROTATION_0);

    st7735_send_command(0x29); // Display on
}

esp_err_t st7735_set_rotation(st7735_rotation_t rotation) {
    
    uint8_t madctl;

    switch (rotation) {
        case ST7735_ROTATION_0:
            madctl = 0x00; // MY=0, MX=0, MV=0
            break;
        case ST7735_ROTATION_90:
            madctl = 0x60; // MY=0, MX=1, MV=1
            break;
        case ST7735_ROTATION_180:
            madctl = 0xC0; // MY=1, MX=1, MV=0
            break;
        case ST7735_ROTATION_270:
            madctl = 0xA0; // MY=1, MX=0, MV=1
            break;
        default:
            return ESP_ERR_INVALID_ARG;
    }

    st7735_send_command(0x36); // MADCTL
    st7735_send_data(madctl);
    return ESP_OK;
}

void st7735_fill_screen(uint16_t color) {
    st7735_send_command(0x2A); // Column address set
    st7735_send_data(0x00); st7735_send_data(0x00); // Start: 0
    st7735_send_data(0x00); st7735_send_data(0x9F); // End: 127 (128 pixels wide) 0x7F

    st7735_send_command(0x2B); // Row address set
    st7735_send_data(0x00); st7735_send_data(0x00); // Start: 0
    st7735_send_data(0x00); st7735_send_data(0x7F); // End: 159 (160 pixels tall) 0x9F

    st7735_send_command(0x2C); // Memory write
    for (int i = 0; i < ST7735_DISP_HOR_RES * ST7735_DISP_VER_RES; i++) {
        st7735_send_data(color >> 8);   // High byte
        st7735_send_data(color & 0xFF); // Low byte
    }
}

void st7735_fill_screen_white() {
    uint16_t color = 0xFFFF;

    st7735_send_command(0x2A); // Column address set
    st7735_send_data(0x00); st7735_send_data(0x00); // Start: 0
    st7735_send_data(0x00); st7735_send_data(0x7F); // End: 127 (128 pixels wide) 0x7F

    st7735_send_command(0x2B); // Row address set
    st7735_send_data(0x00); st7735_send_data(0x00); // Start: 0
    st7735_send_data(0x00); st7735_send_data(0x9F); // End: 159 (160 pixels tall) 0x9F

    st7735_send_command(0x2C); // Memory write
    for (int i = 0; i < ST7735_DISP_HOR_RES * ST7735_DISP_VER_RES; i++) {
        st7735_send_data(color >> 8);   // High byte
        st7735_send_data(color & 0xFF); // Low byte
    }
}
