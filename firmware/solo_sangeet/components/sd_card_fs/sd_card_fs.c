#include <stdio.h>
#include "sd_card_fs.h"

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
// #include "sd_test_io.h"
#include "ff.h"
#include "esp_log.h"

#define EXAMPLE_MAX_CHAR_SIZE    64

#define SD_PIN_NUM_MISO     19
#define SD_PIN_NUM_MOSI     23
#define SD_PIN_NUM_CLK      18
#define SD_PIN_NUM_CS       5

static const char *TAG = "example";

#define MOUNT_POINT "/sdcard"

const char* names[] = {"CLK ", "MOSI", "MISO", "CS  "};
const int pins[] = {SD_PIN_NUM_CLK,
                    SD_PIN_NUM_MOSI,
                    SD_PIN_NUM_MISO,
                    SD_PIN_NUM_CS};
const int pin_count = sizeof(pins)/sizeof(pins[0]);

pin_configuration_t config = {
    .names = names,
    .pins = pins,
};

static esp_err_t s_example_write_file(const char *path, char *data)
{
    ESP_LOGI(TAG, "Opening file %s", path);
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fprintf(f, data);
    fclose(f);
    ESP_LOGI(TAG, "File written");

    return ESP_OK;
}

static esp_err_t s_example_read_file(const char *path)
{
    ESP_LOGI(TAG, "Reading file %s", path);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    char line[EXAMPLE_MAX_CHAR_SIZE];
    fgets(line, sizeof(line), f);
    fclose(f);

    // strip newline
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);

    return ESP_OK;
}

static void list_files(const char *path, int level)
{
    FRESULT res;
    FF_DIR dir;
    FILINFO fno;

    res = f_opendir(&dir, path);
    if (res != FR_OK) {
        ESP_LOGE(TAG, "f_opendir failed (%d): %s", res, path);
        return;
    }

    while (true) {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) {
            break;  // End of directory
        }

        // Skip . and ..
        if (strcmp(fno.fname, ".") == 0 ||
            strcmp(fno.fname, "..") == 0) {
            continue;
        }

        for (int i = 0; i < level; i++) {
            printf("  ");
        }

        // if (fno.fattrib & AM_DIR) {
        //     printf("[DIR ] %s\n", fno.fname);

        //     char full_path[256];
        //     snprintf(full_path, sizeof(full_path),
        //              "%s/%s", path, fno.fname);

        //     list_files(full_path, level + 1);
        // } else {
        //     printf("[FILE] %s (%lu bytes)\n",
        //            fno.fname, fno.fsize);
        // }

        if (strstr(fno.fname, ".wav") || strstr(fno.fname, ".WAV")) {
            // audio file
            ESP_LOGI(TAG, "Found wav audio file: %s", fno.fname);
        }
        
        /*
        if (strstr(fno.fname, ".mp3") || strstr(fno.fname, ".MP3")) {
            // audio file
            ESP_LOGI(TAG, "Found mp3 audio file: %s", fno.fname);
        }

        if (strstr(fno.fname, ".txt") || strstr(fno.fname, ".TXT")) {
            // text file
            ESP_LOGI(TAG, "Found text file: %s", fno.fname);
        }
        */
        
    }

    f_closedir(&dir);
}

void sd_test_func(void)
{
    esp_err_t ret;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
// #ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
//         .format_if_mount_failed = true,
// #else
// #endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.
    ESP_LOGI(TAG, "Using SPI peripheral");

    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 20MHz for SDSPI)
    // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI3_HOST;
    host.max_freq_khz = 2000;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_PIN_NUM_MOSI,
        .miso_io_num = SD_PIN_NUM_MISO,
        .sclk_io_num = SD_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    // ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_DISABLED);
    // if (ret != ESP_OK) {
    //     ESP_LOGE(TAG, "Failed to initialize bus.");
    //     return;
    // }

    ESP_ERROR_CHECK(spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CH2));

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    // Use POSIX and C standard library functions to work with files.

    ESP_LOGI(TAG, "Listing files:");
    // list_files(mount_point, 0);
    list_files("0:/", 0);

    /*
    // First create a file.
    const char *file_hello = MOUNT_POINT"/hello.txt";
    char data[EXAMPLE_MAX_CHAR_SIZE];
    snprintf(data, EXAMPLE_MAX_CHAR_SIZE, "%s %s!\n", "Hello", card->cid.name);
    ret = s_example_write_file(file_hello, data);
    if (ret != ESP_OK) {
        return;
    }

    const char *file_foo = MOUNT_POINT"/foo.txt";

    // Check if destination file exists before renaming
    struct stat st;
    if (stat(file_foo, &st) == 0) {
        // Delete it if it exists
        unlink(file_foo);
    }
    
    // Rename original file
    ESP_LOGI(TAG, "Renaming file %s to %s", file_hello, file_foo);
    if (rename(file_hello, file_foo) != 0) {
        ESP_LOGE(TAG, "Rename failed");
        return;
    }

    ret = s_example_read_file(file_foo);
    if (ret != ESP_OK) {
        return;
    }

    // Format FATFS
    #ifdef CONFIG_EXAMPLE_FORMAT_SD_CARD
    ret = esp_vfs_fat_sdcard_format(mount_point, card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to format FATFS (%s)", esp_err_to_name(ret));
        return;
    }
    
    if (stat(file_foo, &st) == 0) {
        ESP_LOGI(TAG, "file still exists");
        return;
    } else {
        ESP_LOGI(TAG, "file doesn't exist, formatting done");
    }
    #endif // CONFIG_EXAMPLE_FORMAT_SD_CARD

    const char *file_nihao = MOUNT_POINT"/nihao.txt";
    memset(data, 0, EXAMPLE_MAX_CHAR_SIZE);
    snprintf(data, EXAMPLE_MAX_CHAR_SIZE, "%s %s!\n", "Nihao", card->cid.name);
    ret = s_example_write_file(file_nihao, data);
    if (ret != ESP_OK) {
        return;
    }
    
    //Open file for reading
    ret = s_example_read_file(file_nihao);
    if (ret != ESP_OK) {
        return;
    }
    */
    
    // All done, unmount partition and disable SPI peripheral
    // esp_vfs_fat_sdcard_unmount(mount_point, card);
    // ESP_LOGI(TAG, "Card unmounted");
    
    //deinitialize the bus after all devices are removed
    // spi_bus_free(host.slot);
    // ESP_LOGI(TAG, "SPI bus deinitialized");
}
