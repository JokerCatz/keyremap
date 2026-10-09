cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED ENV{PICO_SDK_PATH})
  message(FATAL_ERROR "Set PICO_SDK_PATH to your Pico SDK checkout")
endif()

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(keyremap C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(keyremap
  src/main.c
  src/config.c
  src/hid_parser.c
  src/host_input.c
  src/macro.c
  src/mapper.c
  src/output_hid.c
  src/raw_capture.c
  src/status_led.c
  src/usb_descriptors.c
)

pico_generate_pio_header(keyremap ${CMAKE_CURRENT_LIST_DIR}/src/ws2812.pio)

target_include_directories(keyremap PRIVATE
  include
  src
)

target_link_libraries(keyremap PRIVATE
  pico_stdlib
  hardware_flash
  hardware_pio
  tinyusb_host
  tinyusb_pico_pio_usb
  tinyusb_device
  tinyusb_board
)

target_compile_definitions(keyremap PRIVATE
  PICO_DEFAULT_LED_PIN=16
)

pico_enable_stdio_usb(keyremap 0)
pico_enable_stdio_uart(keyremap 0)
pico_add_extra_outputs(keyremap)

add_executable(host_probe
  src/host_probe.c
  src/status_led.c
)

pico_generate_pio_header(host_probe ${CMAKE_CURRENT_LIST_DIR}/src/ws2812.pio)

target_include_directories(host_probe PRIVATE
  include
  src
)

target_link_libraries(host_probe PRIVATE
  pico_stdlib
  hardware_pio
  tinyusb_host
  tinyusb_pico_pio_usb
  tinyusb_board
)

target_compile_definitions(host_probe PRIVATE
  CFG_TUSB_CONFIG_FILE="tusb_config_host_probe.h"
  PICO_DEFAULT_LED_PIN=16
)

pico_enable_stdio_usb(host_probe 0)
pico_enable_stdio_uart(host_probe 0)
pico_add_extra_outputs(host_probe)
