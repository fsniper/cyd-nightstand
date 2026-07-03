# CYD ESP32 Smart Nightstand Clock

A fully custom, highly optimized smart nightstand clock built natively in C using ESP-IDF and LVGL for the "Cheap Yellow Display" (CYD) ESP32-2432S028R development board. 

This firmware transforms the CYD into a beautiful, self-hosted smart dashboard that features SNTP timekeeping, auto-dimming, Met Éireann weather tracking, Home Assistant sensor integration, and a private Immich photo sync engine.

## Features

- **⏱️ Precision Timekeeping**: SNTP synced via Wi-Fi with the ESP32's internal RTC.
- **🌤️ Live Weather Data**: Periodically fetches and parses XML weather data from Met Éireann, overlaying dynamic weather PNG icons directly onto the UI.
- **🏠 Home Assistant Integration**: Securely connects to your private Home Assistant REST API to fetch and display live room temperature data.
- **📸 Immich Photo Sync (Optional)**: Automatically downloads and caches server-side 250p JPEG thumbnails from a private Immich instance to the SD Card, rotating the background every 2 hours.
- **💡 Auto-Dimming Display**: Reads the CYD's built-in LDR (Light Dependent Resistor) to smoothly scale the backlight PWM duty cycle, preventing glare at night.
- **🛡️ Thread-Safe Architecture**: Heavy network operations (HTTP parsing, Wi-Fi, File I/O) run asynchronously on Core 0, while the LVGL GUI runs flawlessly on Core 1, heavily protected by a global Mutex to prevent watchdog resets and stack overflows.
- **🚀 Splash Screen Boot**: A clean black "Loading..." splash screen hides the UI until all background tasks finish their initial network sync.

## Hardware Requirements
- **Board**: ESP32-2432S028R ("Cheap Yellow Display")
- **Display**: 2.8" ILI9341 TFT SPI Display
- **Storage**: MicroSD Card (formatted to FAT32)

## Software Stack
- **ESP-IDF**: v5.5
- **LVGL**: Light and Versatile Graphics Library
- **Libraries**: `cJSON`, `esp_http_client`, `fatfs`, `esp_timer`

## Setup & Installation

### 1. Configure Secrets
To keep credentials out of version control, this project uses a dedicated `secrets` component. 

1. Copy the example secrets file:
   ```bash
   cp components/secrets/include/secrets_example.h components/secrets/include/secrets.h
   ```
2. Open `components/secrets/include/secrets.h` and fill in your actual credentials:
   - Wi-Fi SSID and Password
   - Home Assistant API Token and Entity URL
   - Immich Share Key, Album ID, and Base URL

*(Note: `secrets.h` is intentionally excluded via `.gitignore` to prevent accidental credential leaks).*

### 2. Build and Flash
With ESP-IDF v5.5 installed and sourced, run:

```bash
# Build the project
idf.py build

# Flash and monitor the output
idf.py flash monitor
```

## Disabling/Enabling Features
If you prefer a clean black background instead of photo rotation, the Immich sync logic can be disabled by wrapping the `immich_sync_run()` call block in `main/main.c` with an `#if 0`. (It is currently disabled by default for a minimalist UI).

## License
MIT License
