# RP2350 Manual Pulse Generator

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