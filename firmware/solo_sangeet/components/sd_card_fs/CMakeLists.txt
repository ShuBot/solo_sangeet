idf_component_register(SRCS "sd_card_fs.c"
                    INCLUDE_DIRS "include"
                    REQUIRES fatfs
                    WHOLE_ARCHIVE)
