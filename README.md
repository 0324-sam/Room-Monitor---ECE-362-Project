# Room Monitor — matches existing wiring

## Existing wiring (GPIO labels, not physical pin positions)

| Part | Signal | GPIO / supply |
|---|---|---|
| BME280 | SDA / SDI | GP0 |
| BME280 | SCL / SCK | GP1 |
| BME280 | VCC, GND | 3.3V, GND |
| BME280 | CSB, SDO (if exposed) | 3.3V, GND (0x76) |
| TFT | SDO/MISO | GP16 (not read by this driver) |
| TFT | CS | GP17 |
| TFT | SCK | GP18 |
| TFT | SDI/MOSI | GP19 |
| TFT | DC/RS | GP20 |
| TFT | RESET | GP21 |
| TFT | VCC, LED, GND | Existing 3.3V, 3.3V, common GND |
| LDR1–4 | Divider midpoints | GP40, GP41, GP42, GP43 |
| Speaker driver | Reserved PWM control | GP22, DISABLED |

Each LDR: 3.3V -> LDR -> ADC midpoint -> 1 kOhm resistor -> GND.
Four independent midpoints. The 1000-count dark threshold must be calibrated.
The TFT driver assumes an ILI9341-compatible controller, landscape 320x240.
The SD-card header is unused. Keep old lab programs and duplicate main files
out of this fresh project.

## Install / run

1. Extract ZIP into a NEW folder; leave the original project intact.
2. Open the folder containing platformio.ini with VS Code / PlatformIO.
3. Keep existing sensor/display wiring. Keep the low-resistance speaker disconnected.
4. Use the same Picoprobe/SWD upload connection that already worked.
5. For Serial Monitor, connect a DATA-capable USB cable to the PROTON board's
   own USB port. This firmware uses native USB serial, not the probe UART.
6. After upload, select the Proton USB CDC COM port (not automatically COM3).
   A new port may appear after Windows installs/enumerates the device. Close
   an old monitor, use PlatformIO Devices to identify ports, and select the
   Proton port. Baud is set to 115200; native USB does not use an electrical
   UART baud rate.

## Test environments

Select in PlatformIO Project Tasks:

- ldr_test: Build / Upload, then Monitor. Prints LDR1–4 every 500 ms.
- bme_test: tests BME280 alone (no TFT required). BME_OK=1 indicates a good read.
- tft_test: cycles simulated normal/hot/humid/dark screens every 2 seconds.
- proton: integrated BME280 + four LDRs + TFT, no physical sound.

Command equivalents:

    pio run -e ldr_test -t upload
    pio device monitor -p YOUR_PROTON_COM_PORT -b 115200

Use bme_test, tft_test, or proton in place of ldr_test as needed.
If Monitor auto-selects the probe COM port, stop it and explicitly select the
Proton USB port. If necessary add monitor_port = COMx to [env:proton], replacing
COMx with the ACTUAL Proton USB CDC port, not an assumed number.

T10 and H10 are tenths: T10=245 means 24.5 C; H10=450 means 45.0% RH.
A dark warning while using 1 kOhm resistors is not by itself a sensor failure.
Block/illuminate each LDR separately and check the matching reading changes.
If L3/L4 stay near 4095, inspect those dividers and GND before calibration.

## Changes from supplied ZIP

- Centralized pin configuration in src/room_config.h.
- BME remains on GP0/GP1; TFT remapped to SPI0 GP17–21.
- Native USB stdio selected with the platform's PIO_NO_STDIO_UART flag; no
  stdio_init_all UART pin takeover on GP0/GP1.
- Explicit RP2350B define for the 48-GPIO / eight-external-ADC variant.
- Alarm reserved on GP22 and disabled by default (no alarm_init/play calls).
- BME transactions have 20 ms timeouts; sensor reset / calibration-ready poll.
- Four test environments and debugger-visible readings.
- PlatformIO is the sole supported build route in this package; the original
  incomplete standalone CMake reference was removed to avoid two conflicting setups.

## Speaker limitation

The user's PC motherboard speaker measured about 15.4 Ohm DC resistance.
DO NOT connect it directly to GP22. This code update does not add a physical
speaker driver and does NOT complete the audible PWM checkoff. Keep
ROOM_ENABLE_ALARM=0 until a correctly designed/rated driver and suitable supply
are connected. LM324 is not a drop-in substitute for that driver.

## Validation

See VALIDATION.md. Actual physical sensor response, TFT controller compatibility,
native USB enumeration on the user's Windows PC and audible alert operation
must be checked on the hardware. No hardware success is claimed by this package.
