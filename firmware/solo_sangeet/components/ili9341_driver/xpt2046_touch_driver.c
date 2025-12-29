#include "xpt2046_touch_driver.h"
#include <stdio.h>
#include <string.h>

// xpt2046_touchpad_read
#define TAG "XPT2046"

spi_device_handle_t touch_spi;

/* -------------------- INIT -------------------- */
void xpt2046_init(spi_host_device_t host)
{
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000,   // 1 MHz (safe)
        .mode = 0,
        .spics_io_num = XPT2046_PIN_CS,
        .queue_size = 1,
        .flags = 0,
    };

    ESP_ERROR_CHECK(spi_bus_add_device(host, &devcfg, &touch_spi));

    // Set Touch Driver Interrupt Pin
    gpio_set_direction(XPT2046_PIN_IRQ, GPIO_MODE_INPUT);
}

static uint16_t xpt2046_read_cmd(uint8_t cmd)
{
    uint8_t tx[3] = { cmd, 0x00, 0x00 };
    uint8_t rx[3] = { 0 };

    spi_transaction_t t = {
        .length = 3 * 8,
        .tx_buffer = tx,
        .rx_buffer = rx,
    };

    ESP_ERROR_CHECK(spi_device_transmit(touch_spi, &t));

    // Combine 12-bit result (ignore lowest 3 bits)
    uint16_t val = ((rx[1] << 8) | rx[2]) >> 3;
    return val;
}

bool xpt2046_read_raw(uint16_t *x, uint16_t *y)
{
    // IRQ is LOW when touched
    if (gpio_get_level(XPT2046_PIN_IRQ) == 1) {
        return false;  // not pressed
    }

    *x = xpt2046_read_cmd(CMD_READ_X);
    *y = xpt2046_read_cmd(CMD_READ_Y);

    return true;
}

uint16_t xpt2046_read_xy(uint16_t *touchpad_x, uint16_t *touchpad_y)
{
    // Implementation of reading touch coordinates from XPT2046
    // This is a placeholder implementation
    *touchpad_x = 0;
    *touchpad_y = 0;
    return 0;
}