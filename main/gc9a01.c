#include "gc9a01.h"
#include "driver/gpio.h"
#include "driver/spi.h"
#include "esp_log.h"

#define TAG "GC9A01"

typedef struct {
    spi_device_handle_t spi;
    gpio_num_t cs_pin;
    gpio_num_t dc_pin;
    gpio_num_t rst_pin;
} gc9a01_dev_t;

static void gc9a01_send_command(gc9a01_dev_t* dev, uint8_t cmd) {
    gpio_set_level(dev->dc_pin, 0);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    spi_device_transmit(dev->spi, &t);
}

static void gc9a01_send_data(gc9a01_dev_t* dev, uint8_t* data, int len) {
    gpio_set_level(dev->dc_pin, 1);
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    spi_device_transmit(dev->spi, &t);
}

gc9a01_handle_t gc9a01_init() {
    gc9a01_dev_t* dev = malloc(sizeof(gc9a01_dev_t));
    if (dev == NULL) return NULL;

    // SPI-Konfiguration
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = GPIO_NUM_13,
        .sclk_io_num = GPIO_NUM_14,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4094,
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = GPIO_NUM_15,
        .queue_size = 7,
    };
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    spi_bus_add_device(SPI2_HOST, &devcfg, &dev->spi);

    // GPIO-Konfiguration
    dev->cs_pin = GPIO_NUM_15;
    dev->dc_pin = GPIO_NUM_2;
    dev->rst_pin = GPIO_NUM_4;

    gpio_set_direction(dev->cs_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(dev->dc_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(dev->rst_pin, GPIO_MODE_OUTPUT);

    // Display-Initialisierung
    gpio_set_level(dev->rst_pin, 0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(dev->rst_pin, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Initialisierungsbefehle senden
    gc9a01_send_command(dev, 0x01); // Software Reset
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Weitere Initialisierungsbefehle hier...

    return dev;
}

void gc9a01_draw_pixel(gc9a01_handle_t dev, int x, int y, uint16_t color) {
    gc9a01_dev_t* d = (gc9a01_dev_t*)dev;
    uint8_t data[2] = {color >> 8, color & 0xFF};
    gc9a01_send_command(d, 0x2A); // Set Column Address
    gc9a01_send_data(d, (uint8_t[]){x >> 8, x & 0xFF}, 2);
    gc9a01_send_command(d, 0x2B); // Set Page Address
    gc9a01_send_data(d, (uint8_t[]){y >> 8, y & 0xFF}, 2);
    gc9a01_send_command(d, 0x2C); // Memory Write
    gc9a01_send_data(d, data, 2);
}

void gc9a01_draw_line(gc9a01_handle_t dev, int x0, int y0, int x1, int y1, uint16_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;) {
        gc9a01_draw_pixel(dev, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void gc9a01_draw_circle(gc9a01_handle_t dev, int x, int y, int radius, uint16_t color) {
    int f = 1 - radius;
    int ddF_x = 1;
    int ddF_y = -2 * radius;
    int x0 = 0;
    int y0 = radius;

    gc9a01_draw_pixel(dev, x, y + radius, color);
    gc9a01_draw_pixel(dev, x, y - radius, color);
    gc9a01_draw_pixel(dev, x + radius, y, color);
    gc9a01_draw_pixel(dev, x - radius, y, color);

    while (x0 < y0) {
        if (f >= 0) {
            y0--;
            ddF_y += 2;
            f += ddF_y;
        }
        x0++;
        ddF_x += 2;
        f += ddF_x;
        gc9a01_draw_pixel(dev, x + x0, y + y0, color);
        gc9a01_draw_pixel(dev, x - x0, y + y0, color);
        gc9a01_draw_pixel(dev, x + x0, y - y0, color);
        gc9a01_draw_pixel(dev, x - x0, y - y0, color);
        gc9a01_draw_pixel(dev, x + y0, y + x0, color);
        gc9a01_draw_pixel(dev, x - y0, y + x0, color);
        gc9a01_draw_pixel(dev, x + y0, y - x0, color);
        gc9a01_draw_pixel(dev, x - y0, y - x0, color);
    }
}

void gc9a01_fill_screen(gc9a01_handle_t dev, uint16_t color) {
    gc9a01_dev_t* d = (gc9a01_dev_t*)dev;
    gc9a01_send_command(d, 0x2A); // Set Column Address
    gc9a01_send_data(d, (uint8_t[]){0, 0, GC9A01_WIDTH >> 8, GC9A01_WIDTH & 0xFF}, 4);
    gc9a01_send_command(d, 0x2B); // Set Page Address
    gc9a01_send_data(d, (uint8_t[]){0, 0, GC9A01_HEIGHT >> 8, GC9A01_HEIGHT & 0xFF}, 4);
    gc9a01_send_command(d, 0x2C); // Memory Write
    for (int i = 0; i < GC9A01_WIDTH * GC9A01_HEIGHT; i++) {
        uint8_t data[2] = {color >> 8, color & 0xFF};
        gc9a01_send_data(d, data, 2);
    }
}