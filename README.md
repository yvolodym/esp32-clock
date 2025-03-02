# esp32-clock

This project is intended as training for esp8266. The aim of this project is to develop a low-energy clock, developed in C by [espressif](https://www.espressif.com/) with associated PCB in [KiCad](https://www.kicad.org/).

### URI
* https://esp32-server.de/schaltplan/
* https://jlcpcb.com/blog/guide-to-multiboard-pcb-design
* https://tttapa.github.io/ESP8266/Chap02%20-%20Hardware.html

### Software
* https://dronebotworkshop.com/gc9a01/
* https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/

### Schematik
* https://embeddedprojects101.com/design-a-battery-powered-stm32-board-with-usb/

### JLCPCB
* https://jlcpcb.com/parts/componentSearch

### Video
* https://www.youtube.com/watch?v=S_p0YV-JlfU


### WIFI Provisioning
* https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/provisioning/wifi_provisioning.html#
* https://github.com/espressif/esp-idf/blob/master/examples/provisioning/wifi_prov_mgr/README.md


### Flash ESP32 8266 chip
```bash
python -m esptool --chip esp8266 -b 460800 --before default_reset --after hard_reset write_flash "@flash_args"
```
from "esp32-clock\build" directory

### RTOS
* https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/index.html
* https://www.espressif.com/en/support/documents/technical-documents?keys=&field_type_tid%5B%5D=14