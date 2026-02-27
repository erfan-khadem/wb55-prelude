# wb55-prelude

STM32WB55 firmware project for multi-sensor environmental monitoring with:
- FreeRTOS-based scheduling
- ST STM32_WPAN BLE stack
- USB CDC logging
- I2C sensor drivers

## Hardware/Platform
- MCU: `STM32WB55` (CPU1 app + CPU2 wireless stack)
- GPIO: `GP0` is BLE-controlled (`'a'` = ON, `'b'` = OFF), `GP1` is available for diagnostics.
- Sensor bus: `I2C1`
- USB: Full-speed CDC (virtual COM port)

## Capabilities
- Sensor acquisition: `SHT40` (temperature, humidity), `SCD41` (CO2, temperature, humidity), `MLX90614` (ambient/object temperature).
- BLE GATT: write characteristic for `GP0` control and read characteristics for latest sensor values.
- USB diagnostics: `USBVCP_Printf()` is non-blocking, gated by CDC DTR (terminal-open state), and keeps only the latest pending log line when USB is unavailable/busy.

## BLE Data Format
Custom characteristic values are little-endian packed integers:

- SHT40 char (`...ED1A`, 4 bytes): `temp_c_x100` (`int16`), `rh_x100` (`uint16`)
- SCD41 char (`...ED1B`, 6 bytes): `co2_ppm` (`uint16`), `temp_c_x100` (`int16`), `rh_x100` (`uint16`)
- MLX90614 char (`...ED1C`, 4 bytes): `ta_c_x100` (`int16`), `to_c_x100` (`int16`)

`x100` means fixed-point value with 2 decimal digits (for example `2534 -> 25.34`).

## Runtime Architecture
- Boot/init: `main()` configures clocks/peripherals, initializes FreeRTOS, then starts STM32_WPAN (`MX_APPE_Init`) and scheduler.
- Tasks: `StartDefaultTask()` runs the sensor loop (`Sensors_Task`) every 10 ms.
- BLE processing: handled in STM32_WPAN transport threads (`ShciUserEvtProcess`, HCI user event thread).
- Sensor timing: fast loop (~1 s) for SHT40/MLX90614, SCD41 readiness poll (~500 ms) and read-on-ready.

## Code Organization
- `Core/Src/main.c`: HAL init, peripheral init, RTOS task creation
- `Core/Src/app_entry.c`: STM32_WPAN transport/system startup
- `STM32_WPAN/App/app_ble.c`: GAP/GATT init, advertising, BLE event routing
- `STM32_WPAN/App/custom_stm.c/.h`: custom GATT service/characteristics and `GP0` write control
- `Core/Src/sensors/app_sensors.c`: sensor scheduling, BLE value updates, USB logging
- `Core/Src/sensors/sht40.c/.h`: SHT40 driver
- `Core/Src/sensors/scd41.c/.h`: SCD41 driver
- `Core/Src/sensors/mlx90614.c/.h`: MLX90614 driver
- `Core/Src/sensors/usb_vcp.c/.h`: non-blocking USB CDC logging wrapper
- `USB_Device/App/usbd_cdc_if.c`: CDC class callbacks and TX bridge
- `CMakeLists.txt` and `cmake/stm32cubemx/`: build definitions and CubeMX integration

## Build
```bash
cmake --preset Debug
cmake --build --preset Debug
```
