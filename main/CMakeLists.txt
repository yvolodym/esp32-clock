idf_component_register(SRCS "app_main.cpp"
    INCLUDE_DIRS "."
    REQUIRES TFT_eSPI WiFiManager esp_lcd esp_driver_spi esp_driver_gpio
)
