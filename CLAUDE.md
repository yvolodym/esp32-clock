# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

A low-power round-display desk clock built on an ESP32-PICO-V3, using the Arduino framework via
PlatformIO. Two GC9A01 round TFT displays (240x240) share one SPI bus, selected via separate CS
pins, and show a digital face (currently) / analog face (implemented but disabled). Time comes
from NTP over WiFi; WiFi credentials are provisioned at runtime via WiFiManager's captive portal
(no hardcoded SSID/password). There is a matching KiCad PCB design in `kicad/`.

## Build / flash / monitor

This is a PlatformIO project (`platformio.ini`), single environment `pico32`:

```bash
pio run -e pico32                 # build
pio run -e pico32 -t upload       # build + flash (set upload_port in platformio.ini first, or pass --upload-port)
pio run -e pico32 -t monitor      # serial monitor (115200 baud, exception decoder enabled)
pio run -e pico32 -t upload -t monitor   # flash then immediately monitor
```

`CMakeLists.txt` / `src/CMakeLists.txt` are auto-generated ESP-IDF stub files (for the Espressif
IDF VS Code extension's IntelliSense) and the `build/` directory holds leftover `idf.py`
artifacts — neither is the active build path. Always build/flash through PlatformIO (`pio`), not
`idf.py`.

There is no test suite; `test/` only contains PlatformIO's boilerplate README.

## Architecture

- **`src/main.cpp`** — entry point. Owns both `TFT_eSPI` display instances (via shared bus + CS
  pin toggling) and three `TFT_eSprite` off-screen buffers (`digital_face_hours`,
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
- **`src/WifiTimeLib.cpp` / `include/WifiTimeLib.h`** — thin wrapper around `WiFiManager` + NTP:
  `connectToWiFi()` runs the captive-portal autoConnect flow (drawing status to the TFT via
  `printNetworkInfo()` if a config portal is needed), `getNTPtime()` polls `configTime()`/`time()`
  until a plausible date is seen or a timeout elapses. NTP server and POSIX TZ string are passed
  into the `WifiTimeLib` constructor in `main.cpp` (currently Switzerland/CET).
- **`include/*.h` (font headers)** — `TFT_eSPI`-format bitmap/vector font data (`MaliBold60`,
  `MaliBold90`, `FinalFrontier28`, `NotoSansBold15`, `NotoSansBold36`) loaded into sprites via
  `loadFont()`. Not meant to be hand-edited.
- **TFT_eSPI configuration is entirely via `build_flags`** in `platformio.ini` (driver, pin
  mapping, fonts to compile in, SPI frequency), not via a `User_Setup.h` — this is why
  `USER_SETUP_LOADED=1` is set. Pin/display changes (driver, resolution, GPIO assignment) go there,
  not in a library file.
- **`partitions_custom.csv`** — custom partition table (app + SPIFFS for fonts/assets); referenced
  from `platformio.ini` via `board_build.partitions`.
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

These match the `TFT_*` build flags in `platformio.ini`. If you change the wiring, update both
the flags and this table together.
