idf_component_register(SRCS "app_main.c" 
                       INCLUDE_DIRS "."
                       REQUIRES lvgl esp_lcd esp_driver_spi esp_driver_i2c esp_driver_ledc esp_driver_gpio)
