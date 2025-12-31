#ifndef XPT2046_TOUCH_DRIVER_H
#define XPT2046_TOUCH_DRIVER_H

#include "driver/spi_master.h"
#include "driver/gpio.h"

/*
    Pin Definitions for the XPT2046 Touch Screen Controller
*/
#define XPT2046_PIN_CS    33     // Chip select pin (T_CS) of touch screen
#define XPT2046_PIN_IRQ   36  // Touch pad interrupt pin (TP_IRQ)

// Optional reduced SPI frequency for reading TFT
#define SPI_READ_FREQUENCY  20000000
// The XPT2046 requires a lower SPI clock rate of 2.5MHz so we define that here:
#define SPI_TOUCH_FREQUENCY  600000

// XPT2046 Commands
#define CMD_READ_X  0xD0
#define CMD_READ_Y  0x90

// Calibration values (need to be calibrated per device)
#define TOUCH_X_MIN   320
#define TOUCH_X_MAX   3800
#define TOUCH_Y_MIN   240
#define TOUCH_Y_MAX   3900

#define TOUCH_SWAP_XY   0
#define TOUCH_INVERT_X  1
#define TOUCH_INVERT_Y  0   // very common for XPT2046

void xpt2046_init(spi_host_device_t host);
bool xpt2046_read_raw(uint16_t *x, uint16_t *y);

uint16_t xpt2046_read_xy(uint16_t *touchpad_x, uint16_t *touchpad_y);

#endif // XPT2046_TOUCH_DRIVER_H