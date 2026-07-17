idf_component_register(SRCS "audio_player.c"
                        INCLUDE_DIRS "include"
                        REQUIRES esp_ringbuf file_manager lvgl bt_manager ui_manager
                    )

