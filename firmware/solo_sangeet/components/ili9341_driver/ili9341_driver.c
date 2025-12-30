#include "ili9341_driver.h"
#include "xpt2046_touch_driver.h"
#include <stdio.h>
#include <string.h>

#define MAIN_SPI_HOST SPI2_HOST

#define ILI9341_DMA_FILL_PIXELS 2048  // 2048 pixels = 4096 bytes

static uint16_t *dma_fill_buf = NULL;

spi_device_handle_t ili9341_spi;

void ili9341_alloc_dma_buffers(void)
{
    dma_fill_buf = heap_caps_malloc(
        ILI9341_DMA_FILL_PIXELS * sizeof(uint16_t),
        MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL
    );

    assert(dma_fill_buf != NULL);
}

void ili9341_spi_config() {
    spi_bus_config_t buscfg = {
        .miso_io_num = ILI9341_PIN_MISO,          // Not using MISO (display doesn’t send data back)
        .mosi_io_num = ILI9341_PIN_MOSI,
        .sclk_io_num = ILI9341_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096
    };

    // LCD Display SPI device configuration
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_FREQUENCY,    // 40 / 15.99 MHz
        .mode = 0,                          // SPI mode 0
        .spics_io_num = ILI9341_PIN_CS,
        .queue_size = 1,
        .flags = SPI_DEVICE_HALFDUPLEX,
    };
    
    // Initialize SPI bus
    esp_err_t ret = spi_bus_initialize(MAIN_SPI_HOST, &buscfg, SPI_DMA_CH1);
    assert(ret == ESP_OK);

    // Add device to bus
    ret = spi_bus_add_device(MAIN_SPI_HOST, &devcfg, &ili9341_spi);
    assert(ret == ESP_OK);

    // Set up DC and RST as GPIO
    gpio_set_direction(ILI9341_PIN_DC, GPIO_MODE_OUTPUT);
    // Set up BackLight LED Control GPIO
    gpio_set_direction(ILI9341_PIN_BL, GPIO_MODE_OUTPUT);

    // Touch Screen SPI device configuration
    xpt2046_init(MAIN_SPI_HOST);
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

void ili9341_set_display_backlight(bool state)
{
    // Trun On/Off the display Backlight
    gpio_set_level(ILI9341_PIN_BL, state ? 1 : 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

void ili9341_init() {
    // Configure SPI
    ili9341_spi_config();

    // Basic init sequence (from ILI9341 datasheet)
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
    
    ili9341_send_command(ILI9341_SLPOUT);    //Exit Sleep
    
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // Set default rotation
    ili9341_set_rotation(ILI9341_ROTATION_0);

    //Display on
    ili9341_send_command(ILI9341_DISPON);
    
    // Trun On the display Backlight
    ili9341_set_display_backlight(true);

    ili9341_alloc_dma_buffers();
}

esp_err_t ili9341_set_rotation(ili9341_rotation_t rotation)
{
    uint8_t madctl = 0;

    switch (rotation) {
        case ILI9341_ROTATION_0:   // Portrait
            madctl = 0x48; // MX | BGR
            break;

        case ILI9341_ROTATION_90:  // Landscape
            madctl = 0x28; // MV | BGR
            break;

        case ILI9341_ROTATION_180:
            madctl = 0x88; // MY | BGR
            break;

        case ILI9341_ROTATION_270:
            madctl = 0xE8; // MX | MY | MV | BGR
            break;

        default:
            return ESP_ERR_INVALID_ARG;
    }

    ili9341_send_command(ILI9341_MADCTL);
    ili9341_send_data(madctl);
    return ESP_OK;
}

void ili9341_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    // Clamp
    if (x1 < x0 || y1 < y0) return;

    // Column addr set
    ili9341_send_command(ILI9341_CASET);  
    ili9341_send_data(x0 >> 8);
    ili9341_send_data(x0 & 0xFF);
    ili9341_send_data(x1 >> 8);
    ili9341_send_data(x1 & 0xFF);
    
    // Row addr set
    ili9341_send_command(ILI9341_RASET);  
    ili9341_send_data(y0 >> 8);
    ili9341_send_data(y0 & 0xFF);
    ili9341_send_data(y1 >> 8);
    ili9341_send_data(y1 & 0xFF);
    
    // Write to RAM
    ili9341_send_command(ILI9341_RAMWR);
}

static inline uint16_t ili9341_color_swap(uint16_t v)
{
    return (v >> 8) | (v << 8);  // swap MSB and LSB
}

void ili9341_fill_screen(ili9341_rotation_t rotation, uint16_t color) {
    // Set window 320 x 240 for differnt orientation modes
    switch (rotation)
    {
    case ILI9341_ROTATION_0:
        ili9341_set_address_window(0, 0, ILI9341_DISP_HOR_RES - 1, ILI9341_DISP_VER_RES - 1);
        break;
    case ILI9341_ROTATION_90:
        ili9341_set_address_window(0, 0, ILI9341_DISP_VER_RES - 1, ILI9341_DISP_HOR_RES - 1);
        break;
    case ILI9341_ROTATION_180:
        ili9341_set_address_window(0, 0, ILI9341_DISP_HOR_RES - 1, ILI9341_DISP_VER_RES - 1);
        break;
    case ILI9341_ROTATION_270:
        ili9341_set_address_window(0, 0, ILI9341_DISP_VER_RES - 1, ILI9341_DISP_HOR_RES - 1);
        break;
    
    default:
        ili9341_set_address_window(0, 0, ILI9341_DISP_HOR_RES - 1, ILI9341_DISP_VER_RES - 1);
        break;
    }

    // Send pixel data (RGB565 format)
    int total_pixels = (ILI9341_DISP_HOR_RES) * (ILI9341_DISP_VER_RES);
    
    // Use DMA buffer for filling
    uint16_t out_color = ili9341_color_swap(color);

    // Fill DMA buffer once
    for (int i = 0; i < ILI9341_DMA_FILL_PIXELS; i++) {
        dma_fill_buf[i] = out_color;
    }

    spi_transaction_t t = { 0 };

    while (total_pixels > 0) {

        uint32_t chunk_pixels =
            (total_pixels > ILI9341_DMA_FILL_PIXELS)
            ? ILI9341_DMA_FILL_PIXELS
            : total_pixels;

        t.tx_buffer = dma_fill_buf;
        t.length    = chunk_pixels * 16;  // bits!
        t.user      = NULL;

        // DC = data
        gpio_set_level(ILI9341_PIN_DC, 1);

        esp_err_t ret = spi_device_transmit(ili9341_spi, &t);
        assert(ret == ESP_OK);

        total_pixels -= chunk_pixels;
    }
}

void ili9341_fill_screen_white(ili9341_rotation_t rotation) {
    uint16_t color = 0xFFFF;
    ili9341_fill_screen(rotation, color);
}

void ili9341_fill_rect_dma(ili9341_rotation_t rotation, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    // Bounds check (critical for LVGL later)
    if (x >= ILI9341_DISP_HOR_RES ||
        y >= ILI9341_DISP_VER_RES)
        return;

    if (x + w > ILI9341_DISP_HOR_RES)
        w = ILI9341_DISP_HOR_RES - x;

    if (y + h > ILI9341_DISP_VER_RES)
        h = ILI9341_DISP_VER_RES - y;

    uint32_t total_pixels = w * h;
    uint16_t out_color = (color >> 8) | (color << 8); // SPI endian

    // Fill DMA buffer once
    for (int i = 0; i < ILI9341_DMA_FILL_PIXELS; i++) {
        dma_fill_buf[i] = out_color;
    }

    // Set drawing window
    ili9341_set_address_window(x, y, x + w - 1, y + h - 1);

    spi_transaction_t t = {0};

    while (total_pixels > 0) {

        uint32_t chunk =(total_pixels > ILI9341_DMA_FILL_PIXELS)
                            ? ILI9341_DMA_FILL_PIXELS
                            : total_pixels;

        t.tx_buffer = dma_fill_buf;
        t.length    = chunk * 16;  // bits
        t.user      = NULL;

        gpio_set_level(ILI9341_PIN_DC, 1); // data

        esp_err_t ret = spi_device_transmit(ili9341_spi, &t);
        assert(ret == ESP_OK);

        total_pixels -= chunk;
    }
}

/****************************************************************************************
 * Display driver function to send pixels data to LVGL
 ****************************************************************************************/
void ili9341_flush_spi(int x1, int y1, int x2, int y2, uint8_t * px_map)
{
    #define SPI_MAX_PIXELS_AT_ONCE 1024   // ~2 KB chunk
    
    uint16_t * buf16 = (uint16_t *)px_map; // Let's say it's a 16 bit (RGB565) display

    // Set window 160 x 128 for landscape mode
    ili9341_set_address_window(x1, y1, x2, y2);

    // Send pixel data (RGB565 format)
    int total_pixels = (x2 - x1 + 1) * (y2 - y1 + 1);
    // Temporary buffer for swapped colors
    static uint16_t tx_buf[SPI_MAX_PIXELS_AT_ONCE];

    int32_t sent = 0;
    while (sent < total_pixels) {
        int32_t chunk_pixels = total_pixels - sent;
        if (chunk_pixels > SPI_MAX_PIXELS_AT_ONCE) {
            chunk_pixels = SPI_MAX_PIXELS_AT_ONCE;
        }

        // Swap colors into tx_buf
        for (int i = 0; i < chunk_pixels; i++) {
            tx_buf[i] = ili9341_color_swap(buf16[sent + i]);
        }

        ili9341_send_data_bytes(tx_buf, chunk_pixels);

        sent += chunk_pixels;
    }
}

void ili9341_flush_spi_dma(int x1, int y1, int x2, int y2, uint8_t * px_map)
{
    int32_t w = x2 - x1 + 1;
    int32_t h = y2 - y1 + 1;
    
    // as its a 16 bit (RGB565) display
    uint16_t * buf16 = (uint16_t *)px_map;

    ili9341_set_address_window(x1, y1, x2, y2);
    
    spi_transaction_t t = {0};
    memset(&t, 0, sizeof(t));

    t.length    = w * h * 2 * 8;   // RGB565 bits
    t.tx_buffer = buf16;
    t.user      = NULL;

    // DC = data
    gpio_set_level(ILI9341_PIN_DC, 1);

    esp_err_t ret = spi_device_transmit(ili9341_spi, &t);
    assert(ret == ESP_OK);

}   
