idf_component_register(SRCS "bt_audio.c"
                    INCLUDE_DIRS "include"
                    REQUIRES bt nvs_flash
                    REQUIRES audio_player)
