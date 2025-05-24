cmake_minimum_required(VERSION 3.13...3.27)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
set(PICO_SDK_PATH "../../pico-sdk")
set(PICO_BOARD "pimoroni_pico_plus2_rp2350")
set(PICO_PLATFORM "rp2350-arm-s")
set(PICOTOOL_FETCH_FROM_GIT_PATH "../..")

include(pico_sdk_import.cmake)

project(rpico_mpg)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(main
   main.c
)

pico_generate_pio_header(main ${CMAKE_CURRENT_LIST_DIR}/inc_decoder_pulse.pio)
pico_generate_pio_header(main ${CMAKE_CURRENT_LIST_DIR}/incremental_mpg.pio)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(main pico_stdlib hardware_pio hardware_dma)

# enable usb output, disable uart output
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)


# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(main)

# Note this example uses the default UART for _stdout_;
# if you want to use the default USB see the [hello-usb](https://github.com/raspberrypi/pico-examples/tree/master/hello_world/usb) example.
