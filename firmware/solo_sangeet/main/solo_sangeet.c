#include <stdio.h>

#include "st7735_driver.h"

void app_main(void) {
    spi_init();
    st7735_init();
    fill_screen_white(); // White in RGB565
	vTaskDelay(500 / portTICK_PERIOD_MS);
    fill_screen(0xF818); // Pink in RGB565 F818
}
