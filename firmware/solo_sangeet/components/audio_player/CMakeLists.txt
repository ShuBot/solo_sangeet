idf_component_register(SRCS "audio_player.c"
                            "audio_player_ui.c"
                            "ss_ui_theme.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_ringbuf sd_card_fs lvgl
                    REQUIRES ili9341_driver bt_audio)
