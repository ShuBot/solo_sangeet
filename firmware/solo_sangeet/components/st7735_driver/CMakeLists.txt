idf_component_register(SRCS "st7735_driver.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_spi
                       REQUIRES esp_driver_gpio)
