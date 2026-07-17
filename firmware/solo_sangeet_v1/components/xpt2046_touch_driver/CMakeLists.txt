idf_component_register(SRCS "xpt2046_touch_driver.c"
                        INCLUDE_DIRS "include"
                        REQUIRES esp_driver_spi
                        REQUIRES esp_driver_gpio
                    )
