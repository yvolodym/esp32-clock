cmake_minimum_required(VERSION 3.16.0)

# Matches platformio.ini's build_flags -mfix-esp32-psram-cache-issue
# VSPI_HOST/HSPI_HOST are legacy driver/spi_common.h aliases removed in ESP-IDF 6.x;
# TFT_eSPI's ESP32 DMA path (Processors/TFT_eSPI_ESP32.c) still uses them.
set(ENV{EXTRA_CPPFLAGS} "-mfix-esp32-psram-cache-issue -DVSPI_HOST=SPI3_HOST -DHSPI_HOST=SPI2_HOST")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32-clock)
