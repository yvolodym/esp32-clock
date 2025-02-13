idf_component_register(SRCS "app_main.c" "lvgl_ui.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_driver_spi esp_lcd)
