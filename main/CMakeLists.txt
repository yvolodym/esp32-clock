idf_component_register(SRCS "app_main.c" 
                       INCLUDE_DIRS "."
                       REQUIRES esp_driver_spi esp_driver_i2c esp_driver_ledc esp_driver_gpio)
