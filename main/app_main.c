/**
 * https://github.com/espressif/ESP8266_RTOS_SDK/blob/master/examples/peripherals/spi_oled/main/spi_oled_example_main.c
 */


#include <stdio.h>
#include <math.h>
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/spi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_libc.h"

#define TAG "GC9A01_CLOCK"

// Pin-Definitionen für das GC9A01-Display
#define PIN_NUM_MISO  -1  // Nicht verwendet
#define PIN_NUM_MOSI  GPIO_NUM_13  // GPIO13 (D7) SDA 
#define PIN_NUM_CLK   GPIO_NUM_14  // GPIO14 (D5) SCL
#define PIN_NUM_CS    GPIO_NUM_15  // GPIO15 (D8)
#define PIN_NUM_DC    GPIO_NUM_4   // GPIO4  (D4)


#define OLED_PIN_SEL  (1ULL << PIN_NUM_DC)

// Display-Auflösung
#define GC9A01_WIDTH  240
#define GC9A01_HEIGHT 240

// Farbdefinitionen
#define GC9A01_COLOR_BLACK   0x0000
#define GC9A01_COLOR_WHITE   0xFFFF
#define GC9A01_COLOR_RED     0xF800

// SPI-Konfiguration
#define SPI_HOST    HSPI_HOST
#define SPI_CLK_SPEED_HZ 40000000  // 40 MHz

// Funktionen für das GC9A01-Display
esp_err_t gc9a01_send_command(uint8_t cmd);
void gc9a01_send_data(uint8_t *data, int len);
void gc9a01_init();
void gc9a01_draw_pixel(int x, int y, uint16_t color);
void gc9a01_draw_line(int x0, int y0, int x1, int y1, uint16_t color);
void gc9a01_draw_circle(int x, int y, int radius, uint16_t color);
void gc9a01_fill_screen(uint16_t color);
void printChipInfo();
esp_err_t oled_clear(uint8_t data);
esp_err_t oled_set_pos(uint8_t x_start, uint8_t y_start);
void IRAM_ATTR spi_event_callback(int event, void *arg);
esp_err_t oled_init();
esp_err_t gc9a01_gpio_init();

static uint8_t oled_dc_level = 0;

static esp_err_t oled_delay_ms(uint32_t time) {
    vTaskDelay(time / portTICK_RATE_MS);
    return ESP_OK;
}

void app_main() {
    uint8_t count = 0;
    // Initialisiere das GC9A01-Display
    gc9a01_init();

    ESP_LOGI(TAG, "init hspi");
    printf("\n" "Start programm.\n");
    printChipInfo();

    // Zeichne das Ziffernblatt
    int x = GC9A01_WIDTH / 2;
    int y = GC9A01_HEIGHT / 2;
    int radius = 100;
    gc9a01_draw_circle(x, y, radius, GC9A01_COLOR_WHITE);
    for (int i = 0; i < 12; i++) {
        float angle = i * 30 * 3.14159 / 180;
        int x1 = x + (radius - 10) * cos(angle);
        int y1 = y + (radius - 10) * sin(angle);
        int x2 = x + radius * cos(angle);
        int y2 = y + radius * sin(angle);
        gc9a01_draw_line(x1, y1, x2, y2, GC9A01_COLOR_WHITE);
    }

    // Hauptschleife für die Uhr
    while (1) {
        // Simulierte Zeit (Stunden, Minuten, Sekunden)
        int hours = 10;
        int minutes = 45;
        int seconds = 0;
        oled_clear(count++);

        // Zeichne die Uhrzeiger
        gc9a01_draw_line(x, y, x + 50 * sin((hours % 12 * 30 + minutes / 2) * 3.14159 / 180),
                         y - 50 * cos((hours % 12 * 30 + minutes / 2) * 3.14159 / 180), GC9A01_COLOR_WHITE);
        gc9a01_draw_line(x, y, x + 70 * sin(minutes * 6 * 3.14159 / 180),
                         y - 70 * cos(minutes * 6 * 3.14159 / 180), GC9A01_COLOR_WHITE);
        gc9a01_draw_line(x, y, x + 90 * sin(seconds * 6 * 3.14159 / 180),
                         y - 90 * cos(seconds * 6 * 3.14159 / 180), GC9A01_COLOR_RED);

        // Warte eine Sekunde
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // Lösche die Zeiger für die nächste Aktualisierung
        gc9a01_fill_screen(GC9A01_COLOR_BLACK);
        gc9a01_draw_circle(x, y, radius, GC9A01_COLOR_WHITE);
    }
}

void printChipInfo() {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP8266 chip with %d CPU cores, WiFi, ", chip_info.cores);
}

static esp_err_t oled_set_dc(uint8_t dc) {
    oled_dc_level = dc;
    return ESP_OK;
}

// SPI-Sendefunktionen
esp_err_t gc9a01_send_command(uint8_t cmd) {
    uint32_t buf = cmd << 24; // In order to improve the transmission efficiency, it is recommended that the external incoming data is (uint32_t *) type data, do not use other type data.
    spi_trans_t trans = {
       .mosi = &buf,
       .bits.mosi = 8
    };
    oled_set_dc(0);
    spi_trans(HSPI_HOST, &trans);
    return ESP_OK;
}

void gc9a01_send_data(uint8_t *data, int len) {
    uint32_t buf[len];  // Temporärer Puffer als uint32_t-Array

    for (int i = 0; i < len; i++) {
        buf[i] = data[i];  // Konvertiere uint8_t zu uint32_t
    }

    spi_trans_t trans = {
        .mosi = buf,         // Übergebe den uint32_t-Puffer
        .bits.mosi = len * 8 // Setze die Bit-Länge
    };

    gpio_set_level(PIN_NUM_DC, 1);  // DC auf HIGH für Daten
    spi_trans(SPI_HOST, &trans);
}


esp_err_t oled_clear(uint8_t data) {
    uint8_t x;
    uint32_t buf[16];
    spi_trans_t trans = {0};
    trans.mosi = buf;
    trans.bits.mosi = 64 * 8;

    for (x = 0; x < 16; x++) {
        buf[x] = data << 24 | data << 16 | data << 8 | data;
    }

    // SPI transfers 64 bytes at a time, transmits twice, increasing the screen refresh rate
    for (x = 0; x < 8; x++) {
        oled_set_pos(0, x);
        oled_set_dc(1);
        spi_trans(HSPI_HOST, &trans);
        spi_trans(HSPI_HOST, &trans);
    }

    return ESP_OK;
}

esp_err_t oled_set_pos(uint8_t x_start, uint8_t y_start) {
    gc9a01_send_command(0xb0 + y_start);
    gc9a01_send_command(((x_start & 0xf0) >> 4) | 0x10);
    gc9a01_send_command((x_start & 0x0f) | 0x01);
    return ESP_OK;
}

esp_err_t gc9a01_gpio_init() {
    ESP_LOGI(TAG, "init gpio");
    gpio_config_t io_conf = {
      .intr_type = GPIO_INTR_DISABLE,
      .mode = GPIO_MODE_OUTPUT,
      .pin_bit_mask = OLED_PIN_SEL,
      .pull_down_en = 0,
      .pull_up_en = 1
    };
    gpio_config(&io_conf);
    return ESP_OK;
}

// Display-Initialisierung
void gc9a01_init() {
    spi_config_t spi_config = {
        .interface.val = SPI_DEFAULT_INTERFACE,
        // Load default interrupt enable
        // TRANS_DONE: true, WRITE_STATUS: false, READ_STATUS: false, WRITE_BUFFER: false, READ_BUFFER: false
        .intr_enable.val = SPI_MASTER_DEFAULT_INTR_ENABLE,
        // Cancel hardware cs
        .interface.cs_en = 0,
        // MISO pin is used for DC
        .interface.miso_en = 0,
        // CPOL: 1, CPHA: 1
        .interface.cpol = 1,
        .interface.cpha = 1,
        // Set SPI to master mode
        // 8266 Only support half-duplex
        .mode = SPI_MASTER_MODE,
        // Set the SPI clock frequency division factor
        .clk_div = SPI_10MHz_DIV,
        // Register SPI event callback function
        .event_cb = spi_event_callback
    };

    spi_init(SPI_HOST, &spi_config);

    gpio_set_direction(PIN_NUM_CS, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);

    vTaskDelay(100 / portTICK_PERIOD_MS);
    
    vTaskDelay(100 / portTICK_PERIOD_MS);

    gc9a01_send_command(0x01);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gc9a01_send_command(0x11);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    uint8_t pixel_format = 0x05;
    gc9a01_send_command(0x3A);
    gc9a01_send_data(&pixel_format, 1);

    gc9a01_send_command(0x29);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    gc9a01_fill_screen(GC9A01_COLOR_WHITE);
    oled_init();
}

esp_err_t oled_init() {

    gc9a01_send_command(0xAE);    // Set Display ON/OFF (AEh/AFh)
    gc9a01_send_command(0x00);    // Set Lower Column Start Address for Page Addressing Mode (00h~0Fh)
    gc9a01_send_command(0x10);    // Set Higher Column Start Address for Page Addressing Mode (10h~1Fh)
    gc9a01_send_command(0x40);    // Set Display Start Line (40h~7Fh)
    gc9a01_send_command(0x81);    // Set Contrast Control for BANK0 (81h)
    gc9a01_send_command(0xCF);    //
    gc9a01_send_command(0xA1);    // Set Segment Re-map (A0h/A1h)
    gc9a01_send_command(0xC8);    // Set COM Output Scan Direction (C0h/C8h)
    gc9a01_send_command(0xA6);    // Set Normal/Inverse Display (A6h/A7h)
    gc9a01_send_command(0xA8);    // Set Multiplex Ratio (A8h)
    gc9a01_send_command(0x3F);    //
    gc9a01_send_command(0xD3);    // Set Display Offset (D3h)
    gc9a01_send_command(0x00);    // Set Lower Column Start Address for Page Addressing Mode (00h~0Fh)
    gc9a01_send_command(0xD5);    // Set Display Clock Divide Ratio/ Oscillator Frequency (D5h)
    gc9a01_send_command(0x80);    //
    gc9a01_send_command(0xD9);    // Set Pre-charge Period (D9h)
    gc9a01_send_command(0xF1);    //
    gc9a01_send_command(0xDA);    // Set COM Pins Hardware Configuration (DAh)
    gc9a01_send_command(0x12);    // Set Higher Column Start Address for Page Addressing Mode (10h~1Fh)
    gc9a01_send_command(0xDB);    // Set VCOMH  Deselect Level (DBh)
    gc9a01_send_command(0x40);    // Set Display Start Line (40h~7Fh)
    gc9a01_send_command(0x20);    // Set Memory Addressing Mode (20h)
    gc9a01_send_command(0x02);    // Set Lower Column Start Address for Page Addressing Mode (00h~0Fh)
    gc9a01_send_command(0x8D);    //
    gc9a01_send_command(0x14);    // Set Higher Column Start Address for Page Addressing Mode (10h~1Fh)
    gc9a01_send_command(0xA4);    // Entire Display ON (A4h/A5h)
    gc9a01_send_command(0xA6);    // Set Normal/Inverse Display (A6h/A7h)
    gc9a01_send_command(0xAF);    // Set Display ON/OFF (AEh/AFh)
    return ESP_OK;
}

void IRAM_ATTR spi_event_callback(int event, void *arg) {
    switch (event) {
        case SPI_INIT_EVENT: {

        }
        break;

        case SPI_TRANS_START_EVENT: {
            gpio_set_level(PIN_NUM_DC, oled_dc_level);
        }
        break;

        case SPI_TRANS_DONE_EVENT: {

        }
        break;

        case SPI_DEINIT_EVENT: {
        }
        break;
    }
}

// Zeichne einen Pixel
void gc9a01_draw_pixel(int x, int y, uint16_t color) {
    uint8_t data[2] = {color >> 8, color & 0xFF};
    gc9a01_send_command(0x2A);  // Set Column Address
    gc9a01_send_data((uint8_t[]){x >> 8, x & 0xFF}, 2);
    gc9a01_send_command(0x2B);  // Set Page Address
    gc9a01_send_data((uint8_t[]){y >> 8, y & 0xFF}, 2);
    gc9a01_send_command(0x2C);  // Memory Write
    gc9a01_send_data(data, 2);
}

// Zeichne eine Linie
void gc9a01_draw_line(int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;) {
        gc9a01_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Zeichne einen Kreis
void gc9a01_draw_circle(int x, int y, int radius, uint16_t color) {
    int f = 1 - radius;
    int ddF_x = 1;
    int ddF_y = -2 * radius;
    int x0 = 0;
    int y0 = radius;

    gc9a01_draw_pixel(x, y + radius, color);
    gc9a01_draw_pixel(x, y - radius, color);
    gc9a01_draw_pixel(x + radius, y, color);
    gc9a01_draw_pixel(x - radius, y, color);

    while (x0 < y0) {
        if (f >= 0) {
            y0--;
            ddF_y += 2;
            f += ddF_y;
        }
        x0++;
        ddF_x += 2;
        f += ddF_x;
        gc9a01_draw_pixel(x + x0, y + y0, color);
        gc9a01_draw_pixel(x - x0, y + y0, color);
        gc9a01_draw_pixel(x + x0, y - y0, color);
        gc9a01_draw_pixel(x - x0, y - y0, color);
        gc9a01_draw_pixel(x + y0, y + x0, color);
        gc9a01_draw_pixel(x - y0, y + x0, color);
        gc9a01_draw_pixel(x + y0, y - x0, color);
        gc9a01_draw_pixel(x - y0, y - x0, color);
    }
}

// Fülle den Bildschirm mit einer Farbe
void gc9a01_fill_screen(uint16_t color) {
    for (int i = 0; i < GC9A01_WIDTH * GC9A01_HEIGHT; i++) {
        gc9a01_draw_pixel(i % GC9A01_WIDTH, i / GC9A01_WIDTH, color);
    }
}