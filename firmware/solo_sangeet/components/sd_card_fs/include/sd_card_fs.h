#ifndef SD_CARD_FS_H
#define SD_CARD_FS_H

#include <stdint.h>

typedef struct {
    const char** names;
    const int* pins;
#if CONFIG_EXAMPLE_ENABLE_ADC_FEATURE
    const int *adc_channels;
#endif
} pin_configuration_t;


// void check_sd_card_pins(pin_configuration_t *config, const int pin_count);
void sd_test_func(void);

#endif //SD_CARD_FS_H