#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#define PIN_SCLK 18
#define PIN_MOSI 23
#define PIN_CS   5
#define PIN_DC   2
#define PIN_RST  4

spi_device_handle_t spi;

void spi_init() {
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,          // Not using MISO (display doesn’t send data back)
        .mosi_io_num = PIN_MOSI,
        .sclk_io_num = PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000, // 10 MHz
        .mode = 0,                          // SPI mode 0
        .spics_io_num = PIN_CS,
        .queue_size = 7
    };

    // Initialize SPI bus
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    assert(ret == ESP_OK);

    // Add device to bus
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
    assert(ret == ESP_OK);

    // Set up DC and RST as GPIO
    gpio_set_direction(PIN_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_RST, GPIO_MODE_OUTPUT);
}

void send_command(uint8_t cmd) {
    gpio_set_level(PIN_DC, 0); // Command mode
    spi_transaction_t t = {
        .length = 8,           // 8 bits
        .tx_buffer = &cmd,
        .flags = 0
    };
    esp_err_t ret = spi_device_transmit(spi, &t);
    assert(ret == ESP_OK);
}

void send_data(uint8_t data) {
    gpio_set_level(PIN_DC, 1); // Data mode
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data,
        .flags = 0
    };
    esp_err_t ret = spi_device_transmit(spi, &t);
    assert(ret == ESP_OK);
}

void st7735_init() {
    // Reset the display
    gpio_set_level(PIN_RST, 0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(PIN_RST, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Basic init sequence (from ST7735 datasheet)
    send_command(0x01); // Software reset
    vTaskDelay(150 / portTICK_PERIOD_MS);

    send_command(0x11); // Sleep out
    vTaskDelay(500 / portTICK_PERIOD_MS);

    send_command(0x3A); // Set color mode
    send_data(0x05);    // 16-bit RGB565

    send_command(0x29); // Display on
}

void fill_screen(uint16_t color) {
    send_command(0x2A); // Column address set
    send_data(0x00); send_data(0x00); // Start: 0
    send_data(0x00); send_data(0x7F); // End: 127 (128 pixels wide) 0x7F

    send_command(0x2B); // Row address set
    send_data(0x00); send_data(0x00); // Start: 0
    send_data(0x00); send_data(0x9F); // End: 159 (160 pixels tall) 0x9F

    send_command(0x2C); // Memory write
    for (int i = 0; i < 128 * 160; i++) {
        send_data(color >> 8);   // High byte
        send_data(color & 0xFF); // Low byte
    }
}

void fill_screen_white() {
    uint16_t color = 0xFFFF;

    send_command(0x2A); // Column address set
    send_data(0x00); send_data(0x00); // Start: 0
    send_data(0x00); send_data(0x7F); // End: 127 (128 pixels wide) 0x7F

    send_command(0x2B); // Row address set
    send_data(0x00); send_data(0x00); // Start: 0
    send_data(0x00); send_data(0x9F); // End: 159 (160 pixels tall) 0x9F

    send_command(0x2C); // Memory write
    for (int i = 0; i < 128 * 160; i++) {
        send_data(color >> 8);   // High byte
        send_data(color & 0xFF); // Low byte
    }
}
