# esp32-clock

This project is intended as training for esp8266. The aim of this project is to develop a low-energy clock, developed in C by [espressif](https://www.espressif.com/) with associated PCB in [KiCad](https://www.kicad.org/).

### URI
* https://docs.lvgl.io/master/intro/introduction.html
* https://github.com/espressif/esp-bsp/tree/master/components/lcd/esp_lcd_gc9a01
* https://github.com/espressif/esp-bsp
* https://github.com/lvgl/lv_port_esp32
* https://github.com/lvgl/lvgl_esp32_drivers/blob/master/README.md

### Software
* https://github.com/espressif/esp-idf/blob/master/examples/peripherals/lcd/spi_lcd_touch/README.md
-------------------------------
* https://github.com/espressif/esp-idf/tree/v5.4/examples/peripherals/lcd/i2c_oled/main
* https://github.com/UsefulElectronics/esp32s3-gc9a01-lvgl/blob/main/main/Kconfig.projbuild
* https://github.com/liyanboy74/gc9a01-esp-idf
* https://github.com/somebox/esp32-GC9A01-round/blob/main/src/main.cpp

### Schematik
* https://embeddedprojects101.com/design-a-battery-powered-stm32-board-with-usb/

### JLCPCB
* https://jlcpcb.com/parts/componentSearch

### Video
* https://www.youtube.com/watch?v=S_p0YV-JlfU


### WIFI Provisioning
* https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/provisioning/wifi_provisioning.html#
* https://github.com/espressif/esp-idf/blob/master/examples/provisioning/wifi_prov_mgr/README.md

| Pin | Name |
|-----|------|
|IO19 |RST   |
|IO10 |SCL   |
|IO20 |CS    |
|IO21 |DC    |
|IO22 |MOSI  |
