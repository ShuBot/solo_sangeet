#include "ili9341_driver.h"
#include <stdio.h>

spi_device_handle_t ili9341_spi;

void ili9341_spi_config() {
    spi_bus_config_t buscfg = {
        .miso_io_num = ILI9341_PIN_MISO,          // Not using MISO (display doesn’t send data back)
        .mosi_io_num = ILI9341_PIN_MOSI,
        .sclk_io_num = ILI9341_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 15999999,         //SPI_FREQUENCY,    // 15.99 MHz
        .mode = 0,                          // SPI mode 0
        .spics_io_num = ILI9341_PIN_CS,
        .queue_size = 7
    };

    // Initialize SPI bus
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    assert(ret == ESP_OK);

    // Add device to bus
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &ili9341_spi);
    assert(ret == ESP_OK);

    // Set up DC and RST as GPIO
    gpio_set_direction(ILI9341_PIN_DC, GPIO_MODE_OUTPUT);
    // Set up BackLight LED Control GPIO
    gpio_set_direction(ILI9341_PIN_BL, GPIO_MODE_OUTPUT);
}

void ili9341_send_command(uint8_t cmd) {
    gpio_set_level(ILI9341_PIN_DC, 0); // Command mode
    spi_transaction_t t = {
        .length = 8,           // 8 bits
        .tx_buffer = &cmd,
        .flags = 0
    };
    esp_err_t ret = spi_device_transmit(ili9341_spi, &t);
    assert(ret == ESP_OK);
}

void ili9341_send_data(uint8_t data) {
    gpio_set_level(ILI9341_PIN_DC, 1); // Data mode
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &data,
        .flags = 0
    };
    esp_err_t ret = spi_device_transmit(ili9341_spi, &t);
    assert(ret == ESP_OK);
}

void ili9341_send_data_bytes(uint16_t *data, size_t len)
{
    gpio_set_level(ILI9341_PIN_DC, 1); // Data mode
    spi_transaction_t t = {
        .length = len * 16,  // len = pixel count, 16 bits per pixel
        .tx_buffer = data,
        .flags = 0
    };
    spi_device_transmit(ili9341_spi, &t);
}

void ili9341_init() {
    // Configure SPI
    ili9341_spi_config();
    
    // Trun On the display Backlight
    gpio_set_level(ILI9341_PIN_BL, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    
    // // Basic init sequence (from ST7735 datasheet)
    // ili9341_send_command(0x01); // Software reset
    // vTaskDelay(150 / portTICK_PERIOD_MS);

    // ili9341_send_command(0x11); // Sleep out
    // vTaskDelay(500 / portTICK_PERIOD_MS);

    // ili9341_send_command(0x3A); // Set color mode
    // ili9341_send_data(0x05);    // 16-bit RGB565
    // // Set default rotation
    // ili9341_set_rotation(ILI9341_ROTATION_0);

    // ili9341_send_command(0x29); // Display on

    ili9341_send_command(0xEF);
    ili9341_send_data(0x03);
    ili9341_send_data(0x80);
    ili9341_send_data(0x02);

    ili9341_send_command(0xCF);
    ili9341_send_data(0x00);
    ili9341_send_data(0XC1);
    ili9341_send_data(0X30);

    ili9341_send_command(0xED);
    ili9341_send_data(0x64);
    ili9341_send_data(0x03);
    ili9341_send_data(0X12);
    ili9341_send_data(0X81);

    ili9341_send_command(0xE8);
    ili9341_send_data(0x85);
    ili9341_send_data(0x00);
    ili9341_send_data(0x78);

    ili9341_send_command(0xCB);
    ili9341_send_data(0x39);
    ili9341_send_data(0x2C);
    ili9341_send_data(0x00);
    ili9341_send_data(0x34);
    ili9341_send_data(0x02);

    ili9341_send_command(0xF7);
    ili9341_send_data(0x20);

    ili9341_send_command(0xEA);
    ili9341_send_data(0x00);
    ili9341_send_data(0x00);

    ili9341_send_command(ILI9341_PWCTR1);    //Power control
    ili9341_send_data(0x23);   //VRH[5:0]

    ili9341_send_command(ILI9341_PWCTR2);    //Power control
    ili9341_send_data(0x10);   //SAP[2:0];BT[3:0]

    ili9341_send_command(ILI9341_VMCTR1);    //VCM control
    ili9341_send_data(0x3e);
    ili9341_send_data(0x28);

    ili9341_send_command(ILI9341_VMCTR2);    //VCM control2
    ili9341_send_data(0x86);  //--

    ili9341_send_command(ILI9341_MADCTL);    // Memory Access Control
    ili9341_send_data(TFT_MAD_MX | TFT_MAD_BGR); // Rotation 0 (portrait mode)

    ili9341_send_command(ILI9341_PIXFMT);
    ili9341_send_data(0x55);

    ili9341_send_command(ILI9341_FRMCTR1);
    ili9341_send_data(0x00);
    ili9341_send_data(0x13); // 0x18 79Hz, 0x1B default 70Hz, 0x13 100Hz

    ili9341_send_command(ILI9341_DFUNCTR);    // Display Function Control
    ili9341_send_data(0x08);
    ili9341_send_data(0x82);
    ili9341_send_data(0x27);

    ili9341_send_command(0xF2);    // 3Gamma Function Disable
    ili9341_send_data(0x00);

    ili9341_send_command(ILI9341_GAMMASET);    //Gamma curve selected
    ili9341_send_data(0x01);

    ili9341_send_command(ILI9341_GMCTRP1);    //Set Gamma
    ili9341_send_data(0x0F);
    ili9341_send_data(0x31);
    ili9341_send_data(0x2B);
    ili9341_send_data(0x0C);
    ili9341_send_data(0x0E);
    ili9341_send_data(0x08);
    ili9341_send_data(0x4E);
    ili9341_send_data(0xF1);
    ili9341_send_data(0x37);
    ili9341_send_data(0x07);
    ili9341_send_data(0x10);
    ili9341_send_data(0x03);
    ili9341_send_data(0x0E);
    ili9341_send_data(0x09);
    ili9341_send_data(0x00);

    ili9341_send_command(ILI9341_GMCTRN1);    //Set Gamma
    ili9341_send_data(0x00);
    ili9341_send_data(0x0E);
    ili9341_send_data(0x14);
    ili9341_send_data(0x03);
    ili9341_send_data(0x11);
    ili9341_send_data(0x07);
    ili9341_send_data(0x31);
    ili9341_send_data(0xC1);
    ili9341_send_data(0x48);
    ili9341_send_data(0x08);
    ili9341_send_data(0x0F);
    ili9341_send_data(0x0C);
    ili9341_send_data(0x31);
    ili9341_send_data(0x36);
    ili9341_send_data(0x0F);

    ili9341_send_command(ILI9341_SLPOUT);    //Exit Sleep
    
    // end_tft_write();
    vTaskDelay(pdMS_TO_TICKS(200));
    // begin_tft_write();
    
    // Set default rotation
    ili9341_set_rotation(ILI9341_ROTATION_0);

    //Display on
    ili9341_send_command(ILI9341_DISPON);    

}

esp_err_t ili9341_set_rotation(ili9341_rotation_t rotation) {

    uint8_t madctl;

    switch (rotation) {
        case ILI9341_ROTATION_0:
            madctl = 0x00; // MY=0, MX=0, MV=0
            break;
        case ILI9341_ROTATION_90:
            madctl = 0x60; // MY=0, MX=1, MV=1
            break;
        case ILI9341_ROTATION_180:
            madctl = 0xC0; // MY=1, MX=1, MV=0
            break;
        case ILI9341_ROTATION_270:
            madctl = 0xA0; // MY=1, MX=0, MV=1
            break;
        default:
            return ESP_ERR_INVALID_ARG;
    }

    ili9341_send_command(TFT_MADCTL); // MADCTL
    ili9341_send_data(madctl);
    return ESP_OK;
}

uint16_t swap_u16(uint16_t v) {
    return (v >> 8) | (v << 8);
}

void ili9341_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    // Column addr set
    gpio_set_level(ILI9341_PIN_DC, 0);
    ili9341_send_command(ILI9341_CASET);  
    gpio_set_level(ILI9341_PIN_DC, 1);
    ili9341_send_data(x0 >> 8);
    ili9341_send_data(x0 & 0xFF);
    ili9341_send_data(x1 >> 8);
    ili9341_send_data(x1 & 0xFF);
    
    // Row addr set
    gpio_set_level(ILI9341_PIN_DC, 0);
    ili9341_send_command(ILI9341_RASET);  
    gpio_set_level(ILI9341_PIN_DC, 1);
    ili9341_send_data(y0 >> 8);
    ili9341_send_data(y0 & 0xFF);
    ili9341_send_data(y1 >> 8);
    ili9341_send_data(y1 & 0xFF);
    // Write to RAM
    gpio_set_level(ILI9341_PIN_DC, 0);
    ili9341_send_command(ILI9341_RAMWR);
}

void ili9341_fill_screen(uint16_t color) {
    // Set window 160 x 128 for landscape mode
    ili9341_set_address_window(0, 0, ILI9341_DISP_VER_RES - 1, ILI9341_DISP_HOR_RES - 1);

    int pixels = ILI9341_DISP_HOR_RES * ILI9341_DISP_VER_RES;
    const int CHUNK_PIXELS = 512;  // 1 KB chunk

    // Small buffer with repeated color
    uint16_t buf[CHUNK_PIXELS];
    for (int i = 0; i < CHUNK_PIXELS; i++) {
        buf[i] = color;
        // buf[i] = swap_u16(color);
    }
    
    // Send in chunks
    while(pixels > 0)
    {
        int chunk = (pixels > CHUNK_PIXELS) ? CHUNK_PIXELS : pixels;
        ili9341_send_data_bytes(buf, chunk);
        pixels -= chunk;
    }
}

void ili9341_fill_screen_white() {
    uint16_t color = 0xFFFF;
    ili9341_fill_screen(color);
}
