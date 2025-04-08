#ifndef ST7735_DRIVER_H
#define ST7735_DRIVER_H

#include "driver/spi_master.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

void spi_init();
void send_command(uint8_t cmd);
void send_data(uint8_t data);
void st7735_init();

void fill_screen_white();
void fill_screen(uint16_t color);


#ifdef __cplusplus
}
#endif

#endif // ST7735_DRIVER_H
