idf_component_register(SRCS "ui_manager.c"
                            "ss_ui_theme.c"
                        INCLUDE_DIRS "include"
                        REQUIRES lvgl bt_manager audio_player file_manager
                    )
