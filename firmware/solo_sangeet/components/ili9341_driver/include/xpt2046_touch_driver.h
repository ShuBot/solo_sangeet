
#define TOUCH_CS 33     // Chip select pin (T_CS) of touch screen

// Optional reduced SPI frequency for reading TFT
#define SPI_READ_FREQUENCY  20000000
// The XPT2046 requires a lower SPI clock rate of 2.5MHz so we define that here:
#define SPI_TOUCH_FREQUENCY  600000
