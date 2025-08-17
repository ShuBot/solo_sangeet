#ifndef ST7735_DRIVER_H
#define ST7735_DRIVER_H

#include "driver/spi_master.h"
#include "driver/gpio.h"

/*
* Pin definitions for the ST7735 display
* Adjust these according to your hardware setup
*/
#define PIN_MISO -1      // Not used, display does not send data back
#define PIN_SCLK 14     // VSPI - 18  HSPI - 14
#define PIN_MOSI 13     // VSPI - 23  HSPI - 13
#define PIN_CS   15     // VSPI - 5   HSPI - 15
#define PIN_DC   27     // VSPI - 2   HSPI - 27
#define PIN_RST  26     // VSPI - 4   HSPI - 26

/*
* Display dimensions
* Adjust these according to your display model
*/
#define DISP_HOR_RES    128
#define DISP_VER_RES    160
#define DISPLAY_SIZE    (DISP_HOR_RES * DISP_VER_RES)
#define DISPLAY_BUFFER_SIZE (DISPLAY_SIZE * sizeof(uint16_t))

/*
* Marcos for color definitions in RGB565 format
* These can be used to fill the screen with specific colors
*/
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_PINK    0xF818

/*
* Function definitions for the ST7735 driver
* These functions handle SPI communication and display initialization
*/

void spi_config();
void st7735_send_command(uint8_t cmd);
void st7735_send_data(uint8_t data);
void st7735_send_data_bytes(const uint8_t *data, size_t len);
void st7735_init();

void fill_screen_white();
void fill_screen(uint16_t color);

#endif // ST7735_DRIVER_H
