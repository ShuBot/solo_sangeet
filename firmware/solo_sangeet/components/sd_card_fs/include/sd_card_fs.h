#ifndef SD_CARD_FS_H
#define SD_CARD_FS_H

#include <stdint.h>

typedef struct {
    const char** names;
    const int* pins;
} pin_configuration_t;

// void check_sd_card_pins(pin_configuration_t *config, const int pin_count);
void sd_fs_init(void);

#endif //SD_CARD_FS_H