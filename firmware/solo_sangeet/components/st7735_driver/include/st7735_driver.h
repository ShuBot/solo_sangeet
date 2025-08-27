#ifndef ST7735_DRIVER_H
#define ST7735_DRIVER_H

#include "driver/spi_master.h"
#include "driver/gpio.h"

/*
* Pin definitions for the ST7735 display
* Adjust these according to your hardware setup
*/
#define ST7735_PIN_MISO -1      // Not used, display does not send data back
#define ST7735_PIN_SCLK 14     // VSPI - 18  HSPI - 14
#define ST7735_PIN_MOSI 13     // VSPI - 23  HSPI - 13
#define ST7735_PIN_CS   15     // VSPI - 5   HSPI - 15
#define ST7735_PIN_DC   27     // VSPI - 2   HSPI - 27
#define ST7735_PIN_RST  26     // VSPI - 4   HSPI - 26

/*
* Display dimensions
* Adjust these according to your display model
*/
#define ST7735_DISP_HOR_RES         128
#define ST7735_DISP_VER_RES         160
#define ST7735_DISPLAY_SIZE         (ST7735_DISP_HOR_RES * ST7735_DISP_VER_RES)
#define ST7735_DISPLAY_BUFFER_SIZE  (ST7735_DISPLAY_SIZE * sizeof(uint16_t))

/*
* Marcos for color definitions in RGB565 format
* These can be used to fill the screen with specific colors
*/
#define COLOR_BLACK         0x0000
#define COLOR_WHITE         0xFFFF
#define COLOR_GREY          0xbdf7
#define COLOR_LIGHT_GREY    0xe73c
#define COLOR_DARK_GREY     0x8410
#define COLOR_ORANGE        0xFD20
#define COLOR_BROWN         0xBC40
#define COLOR_PURPLE        0x780F
#define COLOR_NAVY          0x0010
#define COLOR_MAROON        0x8000
#define COLOR_RED           0xF800
#define COLOR_GREEN         0x07E0
#define COLOR_BLUE          0x001F
#define COLOR_YELLOW        0xFFE0
#define COLOR_CYAN          0x07FF
#define COLOR_MAGENTA       0xF81F
#define COLOR_PINK          0xF818

typedef enum {
    ST7735_ROTATION_0 = 0,
    ST7735_ROTATION_90,
    ST7735_ROTATION_180,
    ST7735_ROTATION_270
} st7735_rotation_t;

/*
* Function definitions for the ST7735 driver
* These functions handle SPI communication and display initialization
*/

void st7735_spi_config();
void st7735_send_command(uint8_t cmd);
void st7735_send_data(uint8_t data);
void st7735_send_data_bytes(const uint8_t *data, size_t len);
void st7735_init();
esp_err_t st7735_set_rotation(st7735_rotation_t rotation);

void st7735_fill_screen_white();
void st7735_fill_screen(uint16_t color);

#endif // ST7735_DRIVER_H
