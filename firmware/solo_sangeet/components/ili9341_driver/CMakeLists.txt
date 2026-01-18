idf_component_register(SRCS "ili9341_driver.c"
                            "xpt2046_touch_driver.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_spi
                       REQUIRES esp_driver_gpio esp_driver_ledc)
