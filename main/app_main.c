#include <stdio.h>
#include "esp_system.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

#define TAG "GC9A01_CLOCK"

// Pin-Definitionen für das GC9A01-Display
#define PIN_NUM_MISO  -1  // Nicht verwendet
#define PIN_NUM_SDA  13  // GPIO13 (D7)
#define PIN_NUM_SCL   14  // GPIO14 (D5)
#define PIN_NUM_CS    15  // GPIO15 (D8)
#define PIN_NUM_DC    2   // GPIO2  (D4)
#define PIN_NUM_RST   4   // GPIO4  (D2)

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
void gc9a01_send_command(uint8_t cmd);
void gc9a01_send_data(uint8_t *data, int len);
void gc9a01_init();
void gc9a01_draw_pixel(int x, int y, uint16_t color);
void gc9a01_draw_line(int x0, int y0, int x1, int y1, uint16_t color);
void gc9a01_draw_circle(int x, int y, int radius, uint16_t color);
void gc9a01_fill_screen(uint16_t color);

void app_main() {
    // Initialisiere das GC9A01-Display
    gc9a01_init();

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

// SPI-Sendefunktionen
void gc9a01_send_command(uint8_t cmd) {
    uint16_t cmd_value = cmd;
    spi_trans_t trans = {
        .cmd = &cmd_value,
        .bits.cmd = 8,
    };
    gpio_set_level(PIN_NUM_DC, 0);  // DC auf LOW für Befehle
    spi_trans(SPI_HOST, &trans);
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

// Display-Initialisierung
void gc9a01_init() {
    spi_config_t spi_config = {
        .interface.val = SPI_DEFAULT_INTERFACE,
        .mode = SPI_MASTER_MODE,
        .clk_div = SPI_8MHz_DIV,
    };
    spi_init(SPI_HOST, &spi_config);

    gpio_set_direction(PIN_NUM_CS, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);

    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(PIN_NUM_RST, 1);
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

    gc9a01_fill_screen(GC9A01_COLOR_BLACK);
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