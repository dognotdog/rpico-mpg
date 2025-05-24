# RP2350 Manual Pulse Generator

## Description

This is to create a "manual pulse generator" for a single stepper motor with a incremental encoder knob or handwheel.

This is done completely on the PIO peripherals by using two DMA connected state machines. The first (inc_decoder) interprets the incremental encoder A/B inputs, and sends increment/decrement signals to the second (incremental_mpg) to generate STEP/DIR signals

## Build

Get `pico-sdk` and place it next to this project, or modify `PICO_SDK_PATH` in `CMakeLists.txt` to point to the correct path. The path is relative to the build folder.
```
mkdir build
cd build
cmake ..
make
picotool load main.uf2 -f
```
Re-run `cmake ..` when adding dependencies in `CMakeLists.txt`, or just always run `cmake .. && make`, though some changes might require cleaning `build/`.