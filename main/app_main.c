/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

/*

#define BSP_LCD_SPI_MOSI      (GPIO_NUM_23)  SDA
#define BSP_LCD_SPI_MISO      (GPIO_NUM_25)  not used
#define BSP_LCD_SPI_CLK       (GPIO_NUM_19)  SCL
#define BSP_LCD_SPI_CS        (GPIO_NUM_22)  CS
#define BSP_LCD_DC            (GPIO_NUM_21)  DC
#define BSP_LCD_RST           (GPIO_NUM_18)  RST
#define BSP_LCD_BACKLIGHT     (GPIO_NUM_5)

*/
#include <stdio.h>
#include <time.h>
#include <esp_err.h>
#include <esp_log.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"

static const char *TAG = "Analog Clock";

static lv_obj_t *screen;
static lv_obj_t *hour_line;
static lv_obj_t *minute_line;
static lv_obj_t *second_line;

// Pin-Konfiguration für das GC9A01-Display
#define GC9A01_SPI_HOST SPI2_HOST
#define GC9A01_PIN_NUM_MISO GPIO_NUM_NC // Nicht verwendet
#define GC9A01_PIN_NUM_MOSI GPIO_NUM_23
#define GC9A01_PIN_NUM_CLK GPIO_NUM_19
#define GC9A01_PIN_NUM_CS GPIO_NUM_22
#define GC9A01_PIN_NUM_DC GPIO_NUM_21
#define GC9A01_PIN_NUM_RST GPIO_NUM_18
#define GC9A01_PIN_NUM_BCKL GPIO_NUM_5

// Display-Auflösung
#define GC9A01_WIDTH 240
#define GC9A01_HEIGHT 240

// LVGL Display- und Touch-Treiber
static lv_display_t *lvgl_disp = NULL;
static esp_lcd_panel_handle_t lcd_panel = NULL;

// Funktion zur Initialisierung des SPI für das Display
void spi_init(){
    spi_bus_config_t buscfg = {
        .miso_io_num = GC9A01_PIN_NUM_MISO,
        .mosi_io_num = GC9A01_PIN_NUM_MOSI,
        .sclk_io_num = GC9A01_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = GC9A01_WIDTH * GC9A01_HEIGHT * 2,
    };
    spi_bus_initialize(GC9A01_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
    	(esp_lcd_spi_bus_handle_t)SPIx_HOST,
           	& (esp_lcd_panel_io_spi_config_t) {
    		    .cs_gpio_num = CONFIG_HWE_DISPLAY_SPI_CS,
    		    .pclk_hz = CONFIG_HWE_DISPLAY_SPI_FREQUENCY,
    		    .lcd_cmd_bits = 32,
    		    .lcd_param_bits = 8,
                .spi_mode = 0,
                .trans_queue_depth = 17,
            },
            io_handle
        )
    );

}

// Funktion zur Initialisierung des GC9A01-Displays
void gc9a01_init() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GC9A01_PIN_NUM_CS) | (1ULL << GC9A01_PIN_NUM_DC) | (1ULL << GC9A01_PIN_NUM_RST) | (1ULL << GC9A01_PIN_NUM_BCKL),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Reset Display
    gpio_set_level(GC9A01_PIN_NUM_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(GC9A01_PIN_NUM_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Display-Konfiguration (hier müssen die spezifischen GC9A01-Kommandos hin)
    // ...
}

// Funktion zur Aktualisierung der Uhr
/*
void update_clock(lv_task_t *timer) {
    static uint32_t last_second = 0;
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    if (last_second != timeinfo.tm_sec)
    {
        last_second = timeinfo.tm_sec;

        // Stundenzeiger aktualisieren
        lv_img_set_angle(hour_line, (timeinfo.tm_hour % 12) * 300 + (timeinfo.tm_min * 5));

        // Minutenzeiger aktualisieren
        lv_img_set_angle(minute_line, timeinfo.tm_min * 60);

        // Sekundenzeiger aktualisieren
        lv_img_set_angle(second_line, timeinfo.tm_sec * 60);
    }
}
*/
/*
static esp_err_t app_lvgl_init(esp_lcd_panel_handle_t lp, esp_lcd_panel_io_handle_t *io_handle, 
    esp_lcd_touch_handle_t tp, lv_display_t **lv_disp, lv_indev_t **lv_touch_indev){

    ESP_LOGI(TAG, "Initialize LVGL");

    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "LVGL port initialization failed");

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = *io_handle,
        .panel_handle = lp,
        .buffer_size = SEND_BUF_SIZE,
        .double_buffer = true,
        .hres = CONFIG_HWE_DISPLAY_WIDTH,
        .vres = CONFIG_HWE_DISPLAY_HEIGHT,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .color_space = LV_COLOR_SPACE_RGB,
        .color_order = LV_COLOR_ORDER_BGR,
        .rotation = {
            .swap_xy = true,
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {
            .swap_bytes = true
        }};

    *lv_disp = lvgl_port_add_disp(&disp_cfg);

    return ESP_OK;
}
*/

void app_main(void){
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    esp_err_t err = lvgl_port_init(&lvgl_cfg);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "LVGL port initialization failed");
        return;
    }
    ESP_LOGI(TAG, "LVGL port initialized");
    ESP_LOGI(TAG, "Initializing Analog Clock...");
    // SPI initialisieren
    spi_init();

    // Display initialisieren
    gc9a01_init();

    // LVGL initialisieren
    lv_init();

    // LVGL-Puffer erstellen
    void *lvgl_buffer = heap_caps_malloc(GC9A01_WIDTH * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_disp_buf_init(&draw_buf, lvgl_buffer, NULL, GC9A01_WIDTH * 20);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = GC9A01_WIDTH;
    disp_drv.ver_res = GC9A01_HEIGHT;
    disp_drv.flush_cb = gc9a01_flush;
    disp_drv.buffer = &draw_buf;
    disp_drv.user_data = NULL;
    lv_disp_drv_register(&disp_drv);

    // Hintergrundfarbe setzen
    screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    // Stundenzeiger erstellen
    /*
    hour_line = lv_line_create(screen);
    static lv_point_t hour_points[] = {{0, -50}, {0, 0}};
    lv_line_set_points(hour_line, hour_points, 2);
    lv_obj_set_style_line_width(hour_line, 5, 0);
    lv_obj_set_style_line_color(hour_line, lv_color_hex(0xFF0000), 0);
    lv_obj_align(hour_line, LV_ALIGN_CENTER, 0, 0);
    */

    // Minutenzeiger erstellen
    /*
    minute_line = lv_line_create(screen);
    static lv_point_t minute_points[] = {{0, -70}, {0, 0}};
    lv_line_set_points(minute_line, minute_points, 2);
    lv_obj_set_style_line_width(minute_line, 3, 0);
    lv_obj_set_style_line_color(minute_line, lv_color_hex(0x00FF00), 0);
    lv_obj_align(minute_line, LV_ALIGN_CENTER, 0, 0);
    */

    // Sekundenzeiger erstellen
    /*
    second_line = lv_line_create(screen);    
    static lv_point_t second_points[] = {{0, -90}, {0, 0}};
    lv_line_set_points(second_line, second_points, 2);
    lv_obj_set_style_line_width(second_line, 2, 0);
    lv_obj_set_style_line_color(second_line, lv_color_hex(0x0000FF), 0);
    lv_obj_align(second_line, LV_ALIGN_CENTER, 0, 0);
    */

    // Timer erstellen, um die Uhr jede Sekunde zu aktualisieren
    //lv_timer_create(update_clock, 1000, NULL);

    ESP_LOGI(TAG, "Analog clock initialized");
}