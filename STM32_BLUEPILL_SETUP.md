# STM32 Blue Pill Setup Guide (Arduino IDE + USB Bootloader)

This guide covers full setup for `STM32F103C8` (Blue Pill) from zero to successful USB uploads.

## 1) Requirements

- Blue Pill board (`STM32F103C8`)
- USB cable (data cable)
- FTDI USB to TTL module (3.3V logic) for first bootloader flash
- Windows PC with Arduino IDE 2.x installed
- STM32CubeProgrammer installed

## 2) Arduino IDE Preferences URL

Open `File > Preferences` and add this to `Additional boards manager URLs`:

```text
https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
```

If you already have other URLs, separate with commas.

## 3) Install STM32 Board Package

1. Open `Tools > Board > Boards Manager`
2. Search: `STM32 MCU based boards`
3. Install package by `STMicroelectronics`

## 4) First-Time Bootloader Flash (using FTDI)

This step installs Maple/STM32duino USB bootloader once.

### 4.1 Wiring (FTDI <-> Blue Pill)

- `FTDI TXD -> Blue Pill PA10 (USART1_RX)`
- `FTDI RXD -> Blue Pill PA9 (USART1_TX)`
- `FTDI GND -> Blue Pill GND`
- `FTDI 3V3 -> Blue Pill 3V3`

Important:
- Use `TXD/RXD` pins, not LED pins like `TXL/RXL`.
- Keep FTDI logic at `3.3V`.

### 4.2 Enter STM32 ROM Boot Mode

1. Set `BOOT0 = 1`
2. Keep `BOOT1 = 0` (default)
3. Press `RESET`

### 4.3 Flash Bootloader in STM32CubeProgrammer

1. Open STM32CubeProgrammer
2. Right side select `UART`
3. Select FTDI COM port, baud `115200`
4. Click `Connect`
5. Open `Erasing & Programming`
6. In `File path`, browse to `generic_boot20_pc13.bin`
7. Set `Start address` = `0x08000000`
8. Click `Start Programming`
9. Wait for success in log

### 4.4 Return to Normal Boot

1. Set `BOOT0 = 0`
2. Press `RESET`
3. Disconnect FTDI
4. Connect Blue Pill with USB

## 5) Arduino IDE Tools Configuration (Blue Pill)

Set these values before upload:

- `Tools > Board` = `Generic STM32F1 series`
- `Tools > Board part number` = `BluePill F103C8` (or `BluePill F103C8 (128k)`)
- `Tools > U(S)ART support` = `Enabled (generic 'Serial')`
- `Tools > USB support` = `CDC (generic 'Serial')`
- `Tools > Upload method` = `Maple DFU Bootloader 2.0`
- `Tools > Port` = Blue Pill COM port (if shown)

Notes:
- `STM32duino bootloader` and `Maple DFU Bootloader 2.0` represent the same upload flow in many versions.
- `CDC (generic 'Serial')` creates a USB virtual COM port when your sketch runs.

## 6) First USB Upload

1. Keep `BOOT0 = 0`
2. Open your sketch and click `Upload`
3. If upload does not auto-trigger DFU, press Blue Pill `RESET` once when `dfu-util` starts in output
4. Wait for upload success

If board appears as `Maple DFU` temporarily, that is normal during bootloader/upload window.

## 7) Why COM Port Sometimes Disappears

- `Maple DFU` mode is bootloader mode and usually has no serial COM port.
- COM port appears when your user sketch starts with USB CDC enabled.
- Add this in `setup()` for stable serial startup:

```cpp
void setup() {
  Serial.begin(115200);
  delay(1500);
}
```

## 8) Troubleshooting

### 8.1 `No DFU capable USB device found`

- Upload started but board did not enter DFU in time.
- Press `RESET` once exactly when upload stage starts (`dfu-util` line appears).

### 8.2 `upload_reset.exe` error

Reinstall STM32 tools:

1. Close Arduino IDE
2. Delete folder:
   `C:\Users\<your_user>\AppData\Local\Arduino15\packages\STMicroelectronics`
3. Reopen IDE and reinstall `STM32 MCU based boards`

### 8.3 Board detected in Device Manager as `Maple DFU` only

- This is expected during bootloader window.
- After successful upload and sketch start, CDC COM port should appear again.

### 8.4 Bootloader seems missing/corrupted

- Reflash `generic_boot20_pc13.bin` over FTDI (Section 4).

## 9) Daily Upload Workflow (after setup)

1. Leave `BOOT0 = 0`
2. Connect USB
3. Upload from Arduino IDE (`Maple DFU Bootloader 2.0`)
4. Press `RESET` once during upload if auto-reset fails

## 10) Optional: Upload via FTDI when USB is unstable

If USB upload is unstable, you can always compile and flash `.bin` from CubeProgrammer over UART:

- Export binary from Arduino IDE
- Set `BOOT0 = 1`, reset
- Flash `.bin` at `0x08000000` via UART
- Set `BOOT0 = 0`, reset

---

Maintainer note: This guide is based on a verified setup session on Windows with Blue Pill + FTDI + STM32CubeProgrammer + Arduino IDE STM32 core.
