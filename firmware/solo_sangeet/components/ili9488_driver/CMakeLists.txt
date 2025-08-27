idf_component_register(SRCS "ili9488_driver.c"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_spi
                       REQUIRES esp_driver_gpio)
