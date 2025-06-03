# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/eugen/esp/esp-idf/components/bootloader/subproject"
  "/home/eugen/Dokumente/PlatformIO/Projects/esp32-clock/build/bootloader"
  "/home/eugen/Dokumente/PlatformIO/Projects/esp32-clock/build/bootloader-prefix"
  "/home/eugen/Dokumente/PlatformIO/Projects/esp32-clock/build/bootloader-prefix/tmp"
  "/home/eugen/Dokumente/PlatformIO/Projects/esp32-clock/build/bootloader-prefix/src/bootloader-stamp"
  "/home/eugen/Dokumente/PlatformIO/Projects/esp32-clock/build/bootloader-prefix/src"
  "/home/eugen/Dokumente/PlatformIO/Projects/esp32-clock/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/eugen/Dokumente/PlatformIO/Projects/esp32-clock/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/eugen/Dokumente/PlatformIO/Projects/esp32-clock/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
