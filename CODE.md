```cpp
#include <stdbool.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_lcd_types.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <esp_heap_caps.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <driver/i2c.h>
#include "esp_lcd_panel_rm67162.h"
#include "sdkconfig.h"
#include "lvgl.h"
#include "esp_lcd_touch_cst816s.h"
#include "ui/ui.h"
#include "esp_lvgl_port.h"
#include "esp_check.h"
#include <driver/i2c_master.h>

#define TAG "lvgl_esplcd"

#if defined(CONFIG_HWE_DISPLAY_SPI1_HOST)

# define SPIx_HOST SPI1_HOST

#elif defined(CONFIG_HWE_DISPLAY_SPI2_HOST)

# define SPIx_HOST SPI2_HOST

#else

# error "SPI host 1 or 2 must be selected"

#endif

#if defined(CONFIG_HWE_DISPLAY_SPI_MODE0)

# define SPI_MODEx (0)

#elif defined(CONFIG_HWE_DISPLAY_SPI_MODE3)

# define SPI_MODEx (2)

#else

# error "SPI MODE0 or MODE3 must be selected"

#endif

#define SEND_BUF_SIZE ((CONFIG_HWE_DISPLAY_WIDTH _ CONFIG_HWE_DISPLAY_HEIGHT \
 _ LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)) / 10)

#define LV_TICK_PERIOD_MS 1

/_ LCD settings _/
#define APP_LCD_LVGL_FULL_REFRESH (0)
#define APP_LCD_LVGL_DIRECT_MODE (1)
#define APP_LCD_LVGL_AVOID_TEAR (1)
#define APP_LCD_RGB_BOUNCE_BUFFER_MODE (1)
#define APP_LCD_DRAW_BUFF_DOUBLE (0)
#define APP_LCD_DRAW_BUFF_HEIGHT (100)
#define APP_LCD_RGB_BUFFER_NUMS (2)
#define APP_LCD_RGB_BOUNCE_BUFFER_HEIGHT (10)

#define EXAMPLE_LCD_COLOR_SPACE (ESP_LCD_COLOR_SPACE_BGR)
#define EXAMPLE_LCD_BITS_PER_PIXEL (16)
#define EXAMPLE_LCD_DRAW_BUFF_DOUBLE (1)
#define EXAMPLE_LCD_DRAW_BUFF_HEIGHT (50)
#define EXAMPLE_LCD_BL_ON_LEVEL (1)

#define I2C_MASTER_FREQ_HZ 400000 /_!< I2C master clock frequency _/

SemaphoreHandle_t xGuiSemaphore;

// static SemaphoreHandle_t touch_mux;
// esp_lcd_panel_io_handle_t io_handle;
// esp_lcd_panel_io_handle_t io_touch_handle;
// esp_lcd_touch_handle_t tp;

// static lv_indev_t \*lvgl_touch_indev = NULL;

static esp_lcd_panel_handle_t lcd_panel = NULL;

static i2c_master_bus_handle_t my_bus = NULL;
static esp_lcd_panel_io_handle_t touch_io_handle = NULL;
static esp_lcd_touch_handle_t touch_handle = NULL;

esp_lcd_panel_io_handle_t io_handle = NULL;

/_ LVGL display and touch _/
static lv_display_t _ lvgl_disp = NULL;
static lv_indev_t _ lvgl_touch_indev = NULL;

extern void example_lvgl_demo_ui(lv_display_t \*disp);

static bool IRAM_ATTR color_trans_done(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
lv_display_t _disp = (lv_display_t_)user_ctx;
lv_display_flush_ready(disp);
// Whether a high priority task has been waken up by this function
return false;
}

static void lv_tick_task(void \*arg) {
lv_tick_inc(LV_TICK_PERIOD_MS);
}

static void disp_flush(lv_display_t *disp_drv, const lv_area_t *area,
uint8_t _px_map)
{
esp_lcd_panel_handle_t panel_handle =
(esp_lcd_panel_handle_t)lv_display_get_user_data(disp_drv);
ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle,
area->x1, area->y1,
area->x2 + 1, area->y2 + 1,
(uint16_t _) px_map));
}

static esp_err_t app_amoled_init(esp_lcd_panel_handle_t _lp, esp_lcd_panel_io_handle_t _ io_handle)
{
esp_err_t ret = ESP_OK;

    ESP_LOGI(TAG, "Power up AMOLED");
    ESP_ERROR_CHECK(gpio_set_direction(CONFIG_HWE_DISPLAY_PWR,
    			GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(CONFIG_HWE_DISPLAY_PWR,
    			CONFIG_HWE_DISPLAY_PWR_ON_LEVEL));
    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_LOGI(TAG, "Initialize SPI bus");
    ESP_ERROR_CHECK(spi_bus_initialize(SPIx_HOST,
    	& (spi_bus_config_t) {
    		.data0_io_num = CONFIG_HWE_DISPLAY_SPI_D0,
    		.data1_io_num = CONFIG_HWE_DISPLAY_SPI_D1,
    		.sclk_io_num = CONFIG_HWE_DISPLAY_SPI_SCK,
    		.data2_io_num = CONFIG_HWE_DISPLAY_SPI_D2,
    		.data3_io_num = CONFIG_HWE_DISPLAY_SPI_D3,
    		.max_transfer_sz = SEND_BUF_SIZE + 8,
    		.flags = SPICOMMON_BUSFLAG_MASTER
    			| SPICOMMON_BUSFLAG_GPIO_PINS
    			| SPICOMMON_BUSFLAG_QUAD,
    	},
    	SPI_DMA_CH_AUTO
    ));
    ESP_LOGI(TAG, "Attach panel IO handle to SPI");

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
    	(esp_lcd_spi_bus_handle_t)SPIx_HOST,
           	& (esp_lcd_panel_io_spi_config_t) {
    		    .cs_gpio_num = CONFIG_HWE_DISPLAY_SPI_CS,
    		    .pclk_hz = CONFIG_HWE_DISPLAY_SPI_FREQUENCY,
    		    .lcd_cmd_bits = 32,
    		    .lcd_param_bits = 8,

#if defined(CONFIG_HWE_DISPLAY_SPI_SPI)
.spi_mode = 0,
#elif defined(CONFIG_HWE_DISPLAY_SPI_QSPI)
.spi_mode = 0,
.flags.quad_mode = 1,
#elif defined(CONFIG_HWE_DISPLAY_SPI_OSPI)
.spi_mode = 3,
.flags.octal_mode = 1,
#else

# error "SPI single, quad and octal modes are supported"

#endif
.trans_queue_depth = 17,
},
io_handle
));

    assert(io_handle != NULL);

    ESP_LOGI(TAG, "Attach vendor specific module");

    esp_lcd_panel_dev_config_t panel_config =  {
        .reset_gpio_num = CONFIG_HWE_DISPLAY_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };


    ESP_ERROR_CHECK(esp_lcd_new_panel_rm67162(
    	*io_handle,
    	&panel_config,
    	lp
    ));

    assert(lp != NULL);

    ESP_LOGI(TAG, "Init panel");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_init(*lp), err, TAG, "Init panel failed");

    ESP_LOGI(TAG, "Reset panel");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_reset(*lp), err, TAG, "Cannot reset AMOLED panel");

    ESP_LOGI(TAG, "Turn on the screen");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_disp_on_off(*lp, true), err, TAG, "Cannot turn on display");

    ESP_GOTO_ON_ERROR(esp_lcd_panel_invert_color(*lp, true), err, TAG, "Cannot invert panel colors");
    // Rotate 90 degrees clockwise:
    ESP_GOTO_ON_ERROR(esp_lcd_panel_swap_xy(*lp, true), err, TAG, "Cannot swap xy");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_mirror(*lp, true, false), err, TAG, "Cannot mirror the panel");

    ESP_LOGI(TAG, "Turn on backlight");
    ESP_ERROR_CHECK(gpio_set_level(CONFIG_HWE_DISPLAY_PWR,
    			CONFIG_HWE_DISPLAY_PWR_ON_LEVEL));

    // ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(
    // 	io_handle,
    // 	&(esp_lcd_panel_io_callbacks_t) {
    // 		color_trans_done
    // 	},
    //        	disp));

err:
if (*lp)
{
esp_lcd_panel_del(*lp);
}
return ret;

}

static esp_err_t app_touch_init(i2c_master_bus_handle_t *bus,
esp_lcd_panel_io_handle_t *tp_io,
esp_lcd_touch_handle_t *tp)
{
if (!*bus)
{
ESP_LOGI(TAG, "creating i2c master bus");
const i2c_master_bus_config_t i2c_conf = {
.i2c_port = -1,
.sda_io_num = CONFIG_LCD_TOUCH_SDA,
.scl_io_num = CONFIG_LCD_TOUCH_SCL,
.clk_source = I2C_CLK_SRC_DEFAULT,
.glitch_ignore_cnt = 7,
.flags.enable_internal_pullup = 1,
};
ESP_RETURN_ON_ERROR(i2c_new_master_bus(&i2c_conf, bus),
TAG, "failed to create i2c master bus");
}

    if (!*tp_io)
    {
        ESP_LOGI(TAG, "creating touch panel io");
        esp_lcd_panel_io_i2c_config_t tp_io_cfg =
            ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG();
        tp_io_cfg.scl_speed_hz = I2C_MASTER_FREQ_HZ;
        ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c_v2(*bus, &tp_io_cfg, tp_io),
                            TAG, "Failed to crate touch panel io");
    }

    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = CONFIG_HWE_DISPLAY_WIDTH,
        .y_max = CONFIG_HWE_DISPLAY_HEIGHT,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = CONFIG_LCD_TOUCH_INT,
    };

    return esp_lcd_touch_new_i2c_cst816s(*tp_io, &tp_cfg, tp);

}

static esp_err_t app_lvgl_init(esp_lcd_panel_handle_t lp, esp_lcd_panel_io_handle_t \* io_handle, esp_lcd_touch_handle_t tp,
lv_display_t **lv_disp, lv_indev_t **lv_touch_indev) {
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
        // .monochrome = true,

#if LVGL_VERSION_MAJOR >= 9
.color_format = LV_COLOR_FORMAT_RGB565,
#endif
.rotation = {
.swap_xy = true,
.mirror_x = true,
.mirror_y = false,
},
.flags = {
#if LVGL_VERSION_MAJOR >= 9
.swap_bytes = true,
#endif
.sw_rotate = false,
}
};

    *lv_disp = lvgl_port_add_disp(&disp_cfg);

    return ESP_OK;

}

void app_main(void)
{
// ESP_LOGI(TAG, "Launching gui task");
/_ Pinned to core 1. Core 0 will run bluetooth/wifi jobs. _/
// xTaskCreatePinnedToCore(gui_task, "gui", 4096\*2, NULL, 0, NULL, 1);
// xTaskCreatePinnedToCore(touch_task, "touch", 4096, NULL, 0, NULL, 1);

    esp_err_t ret = ESP_OK;

    xGuiSemaphore = xSemaphoreCreateMutex();

    ESP_ERROR_CHECK(app_amoled_init(&lcd_panel, &io_handle));
    assert(lcd_panel != NULL);
    assert(io_handle != NULL);
    ESP_ERROR_CHECK(app_touch_init(&my_bus, &touch_io_handle, &touch_handle));
    assert(touch_handle != NULL);
    assert(touch_io_handle != NULL);
    ESP_ERROR_CHECK(app_lvgl_init(lcd_panel, &io_handle, touch_handle, &lvgl_disp, &lvgl_touch_indev));

}
```