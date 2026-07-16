# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

A low-power round-display desk clock built on an ESP32-PICO-V3, using the Arduino framework as an
ESP-IDF component (not PlatformIO). Two GC9A01 round TFT displays (240x240) share one SPI bus,
selected via separate CS pins, and show a digital face (currently) / analog face (implemented but
disabled). Time comes from NTP over WiFi; WiFi credentials are provisioned at runtime via
WiFiManager's captive portal (no hardcoded SSID/password). There is a matching KiCad PCB design in
`kicad/`.

## Build / flash / monitor

This is a native ESP-IDF project (`idf.py`), target `esp32`:

```bash
idf.py set-target esp32           # one-time, already reflected in sdkconfig
idf.py build                      # build
idf.py -p /dev/ttyUSB0 flash      # build + flash (adjust port)
idf.py -p /dev/ttyUSB0 monitor    # serial monitor (exception decoder built in)
idf.py -p /dev/ttyUSB0 flash monitor
```

Requires the ESP-IDF v6.x environment sourced (`. $IDF_PATH/export.sh` or `. ~/.espressif/vX.Y.Z/esp-idf/export.sh`)
before invoking `idf.py`. First configure/build will fetch a large managed-component dependency
tree into `managed_components/` (gitignored, pinned by the committed `dependencies.lock`) — this
requires network access once.

`platformio.ini` is legacy from the pre-migration PlatformIO/Arduino setup and is no longer the
active build path; it expects a `src/` directory that no longer exists (renamed to `main/`). Kept
only for historical reference — do not try to build with `pio`.

There is no test suite; `test/` only contains PlatformIO's boilerplate README.

## Architecture

- **`main/main.cpp`** — entry point. Because `CONFIG_AUTOSTART_ARDUINO=y` (see `sdkconfig.defaults`),
  arduino-esp32 provides `app_main()` itself and calls this file's plain Arduino-style `setup()`/
  `loop()` — no ESP-IDF boilerplate needed here. Owns both `TFT_eSPI` display instances (via shared
  bus + CS pin toggling) and three `TFT_eSprite` off-screen buffers (`digital_face_hours`,
  `digital_face_minutes`, `analog_face`) used to avoid full-screen flicker on redraw.
  - `setupDisplays()` configures CS pins, creates/loads fonts into sprites, inits the shared TFT bus.
  - `renderDigitalFace(t, bg_color)` — redraws the hours sprite only when the hour changes, redraws
    minutes/seconds every second.
  - `renderAnalogFace(t, bg_color)` — full analog clock face (numerals, hour/minute/second hands)
    into `analog_face`, then pushed with `TFT_TRANSPARENT`. Currently not called from `loop()`
    (commented out) — digital face is rendered to both displays instead.
  - `loop()` throttles rendering via `targetTime` (millis-based, ~3ms cadence) and only touches a
    given display after pulling its CS pin low, restoring it high afterward — required because both
    displays are on one SPI bus (`display_cs_pins[2] = {20, 20}` — currently both aliased to the
    same pin; distinguish them before wiring a second physical display).
- **`main/WifiTimeLib.cpp` / `include/WifiTimeLib.h`** — thin wrapper around `WiFiManager` + NTP:
  `connectToWiFi()` runs the captive-portal autoConnect flow (drawing status to the TFT via
  `printNetworkInfo()` if a config portal is needed), `getNTPtime()` polls `configTime()`/`time()`
  until a plausible date is seen or a timeout elapses. NTP server and POSIX TZ string are passed
  into the `WifiTimeLib` constructor in `main.cpp` (currently Switzerland/CET).
- **`include/*.h` (font headers)** — `TFT_eSPI`-format bitmap/vector font data (`MaliBold60`,
  `MaliBold90`, `FinalFrontier28`, `NotoSansBold15`, `NotoSansBold36`) loaded into sprites via
  `loadFont()`. Not meant to be hand-edited. `main/CMakeLists.txt` adds `../include` as an
  INCLUDE_DIRS entry so these resolve unchanged from the pre-migration layout.
- **`components/`** — vendored Arduino-as-ESP-IDF-component libraries (not managed-component
  fetches, so they're plain tracked source, each stripped of examples/docs/`.git` to cut size):
  - `arduino/` — espressif/arduino-esp32 3.3.10 (matches installed ESP-IDF 6.0.2; supported range
    5.3.0–6.0.99 per its own `CMakeLists.txt` version check). Two local patches on top of upstream,
    both needed to compile on esp32 target under IDF 6.x — see comments in the file:
    - added `esp_wifi` and `espressif__network_provisioning` to its `REQUIRES` (upstream's
      `WiFiType.h`/`WiFiGeneric.h` need their headers unconditionally once WiFi is enabled, but
      upstream only wires the requirement in when the unrelated `WiFiProv` library is selected).
  - `TFT_eSPI/`, `WiFiManager/` — used as-is (both already ship their own ESP-IDF
    `CMakeLists.txt`/`Kconfig`), except `WiFiManager/CMakeLists.txt` relaxes `-Werror=format` for
    that one file (its debug prints use `%u` for `uint32_t`, which the ESP-IDF build's default
    `-Werror=all` treats as fatal; PlatformIO/Arduino IDE builds never enabled `-Werror` here).
  - Only a subset of arduino-esp32's libraries are actually compiled — see
    `CONFIG_ARDUINO_SELECTIVE_*` in `sdkconfig.defaults` (SPI/Wire/FS/SPIFFS/Network/WiFi/WebServer/
    DNSServer/Update/Hash/AsyncUDP). This keeps the image inside the 0x1D0000 app partition in
    `partitions_custom.csv` — building all of arduino-esp32's libraries (BLE/Matter/Zigbee/etc.)
    would not fit and isn't used by this project.
- **TFT_eSPI configuration is entirely via `sdkconfig.defaults`** (driver, pin mapping, fonts,
  backlight), reached through TFT_eSPI's own Kconfig menu (`CONFIG_TFT_eSPI_ESPIDF` makes
  `TFT_config.h` read `CONFIG_TFT_*` instead of a `User_Setup.h`). Pin/display changes go there, not
  in a library file. Two settings can't be expressed as Kconfig and instead are global compiler
  defines set in the top-level `CMakeLists.txt` (`EXTRA_CPPFLAGS`): `-mfix-esp32-psram-cache-issue`
  (carried over unchanged from the old `platformio.ini` build_flags) and `-DVSPI_HOST=SPI3_HOST
  -DHSPI_HOST=SPI2_HOST` (compatibility defines — TFT_eSPI's ESP32 DMA path still uses the
  `VSPI_HOST`/`HSPI_HOST` aliases that ESP-IDF 6.x's driver headers dropped).
- **`partitions_custom.csv`** — custom partition table (app + SPIFFS for fonts/assets); referenced
  from `sdkconfig.defaults` via `CONFIG_PARTITION_TABLE_CUSTOM_FILENAME`.
- **`kicad/`** — PCB/schematic for the physical clock (ESP32-PICO-V3, CR2032-backed). Backups
  (`esp32-clock-backups/`) and JLCPCB export dirs are gitignored. `mapping.csv` maps KiCad
  footprints to LCSC part numbers for BOM/ordering.

## Hardware pinout (current wiring)

| Pin  | Signal |
|------|--------|
| IO19 | RST    |
| IO10 | SCL    |
| IO20 | CS     |
| IO21 | DC     |
| IO22 | MOSI   |

These match the `CONFIG_TFT_*` settings in `sdkconfig.defaults`. If you change the wiring, update
both the config and this table together.
