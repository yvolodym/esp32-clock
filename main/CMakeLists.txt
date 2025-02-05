idf_component_register(SRCS "app_main.c" "wifi-manager.c"
                    PRIV_REQUIRES esp_wifi nvs_flash wifi_provisioning esp_driver_gpio
                    PRIV_INCLUDE_DIRS "../components"
                    INCLUDE_DIRS ".")
