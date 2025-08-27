#ifndef ILI9488_DRIVER_H
#define ILI9488_DRIVER_H

#include "driver/spi_master.h"
#include "driver/gpio.h"

/*
* Pin definitions for the ILI9488 display
* Adjust these according to your hardware setup
*/
#define ILI9488_PIN_MISO        -1      // Not used, display does not send data back
#define ILI9488_PIN_SCLK        14     // VSPI - 18  HSPI - 14
#define ILI9488_PIN_MOSI        13     // VSPI - 23  HSPI - 13
#define ILI9488_PIN_CS          15     // VSPI - 5   HSPI - 15
#define ILI9488_PIN_DC          27     // VSPI - 2   HSPI - 27
#define ILI9488_PIN_RST         26     // VSPI - 4   HSPI - 26

/*
* Display dimensions
* Adjust these according to your display model
*/
#define ILI9488_DISP_HOR_RES            128
#define ILI9488_DISP_VER_RES            160
#define ILI9488_DISPLAY_SIZE            (ILI9488_DISP_HOR_RES * ILI9488_DISP_VER_RES)
#define ILI9488_DISPLAY_BUFFER_SIZE     (ILI9488_DISPLAY_SIZE * sizeof(uint16_t))

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
    ILI9488_ROTATION_0 = 0,
    ILI9488_ROTATION_90,
    ILI9488_ROTATION_180,
    ILI9488_ROTATION_270
} ili9488_rotation_t;

/*
* Function definitions for the ILI9488 driver
* These functions handle SPI communication and display initialization
*/

void ili9488_spi_config();
void ili9488_send_command(uint8_t cmd);
void ili9488_send_data(uint8_t data);
void ili9488_send_data_bytes(const uint8_t *data, size_t len);
void ili9488_init();
esp_err_t ili9488_set_rotation(ili9488_rotation_t rotation);

void ili9488_fill_screen_white();
void ili9488_fill_screen(uint16_t color);

#endif // ILI9488_DRIVER_H
