# Board porting

The initial build target is `esp32s3`, with a 4 MiB flash layout and no physical
peripherals. No board pin map has been selected. A build is not hardware validation.

To add a board, document chip, module, flash size, console transport, sensor bus,
output polarity, boot-sensitive pins, and safe output behavior. Use `main/boards/esp32s3/board_config.hpp` and `main/Kconfig.projbuild` and select wiring there rather than scattering pin numbers across
application code. Add the board profile to CI only after its build works.

Chip target and board profile are different axes. `idf.py set-target` selects the
chip; it does not select a custom PCB. ESP32 family members have different radio
and peripheral capabilities. Do not promise every chip supports the same features.

Use distinct build and sdkconfig paths for concurrent target builds. Generated
sdkconfig can retain old choices; start a clean configuration when testing defaults.
A `boards/.../sdkconfig.defaults` file is not loaded automatically: explicitly
supply it through SDKCONFIG_DEFAULTS when board profiles are introduced.
