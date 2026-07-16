cmake_minimum_required(VERSION 3.5)

idf_component_register(
                       SRCS "WiFiManager.cpp"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES arduino
)

# WiFiManager's debug prints use %u for values that are uint32_t/long unsigned int
# on this toolchain; ESP-IDF's project build enables -Werror=all by default
# (PlatformIO/Arduino IDE builds don't), so relax just this one warning here.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=format)

project(WiFiManager)
