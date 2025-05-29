#include "driver/spi_master.h"
#include "TFT_eSPI.h"

/*

#define BSP_LCD_SPI_MOSI      (GPIO_NUM_23)  SDA
#define BSP_LCD_SPI_MISO      (GPIO_NUM_25)  not used
#define BSP_LCD_SPI_CLK       (GPIO_NUM_19)  SCL
#define BSP_LCD_SPI_CS        (GPIO_NUM_22)  CS
#define BSP_LCD_DC            (GPIO_NUM_21)  DC
#define BSP_LCD_RST           (GPIO_NUM_18)  RST
#define BSP_LCD_BACKLIGHT     (GPIO_NUM_5)

*/

TFT_eSPI tft = TFT_eSPI();

extern "C" void app_main() {
    // SPI-Bus initialisieren (optional)
    spi_bus_config_t buscfg = {
        .mosi_io_num = TFT_MOSI,
        .miso_io_num = -1,  // GC9A01 benötigt kein MISO
        .sclk_io_num = TFT_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO);

    // TFT initialisieren
    tft.init();
    tft.setRotation(0);  // Ausrichtung je nach Display
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("GC9A01 + ESP32", 20, 50, 4);



    tft.fillScreen(TFT_RED);
    tft.drawCircle(120, 120, 50, TFT_WHITE);  // Mittelpunkt (x,y), Radius
    tft.fillRect(50, 50, 60, 60, TFT_BLUE);
}