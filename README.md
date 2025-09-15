# VicharakProjectFinal

This is the final project repository for Vicharak Computers LLP, designed for robust file downloads and storage on ESP32-based hardware using Arduino. The code is modular, optimized for embedded systems, and features advanced memory and performance management.

---

## Features

- **Dynamic Buffer Management**: Allocates download/write buffers based on available memory; supports double buffering for performance.
- **Performance Monitoring**: Tracks download speeds, connection/transfer timings, and efficiency.
- **HTTP Download Engine**: Supports retry logic, file streaming, and download resumption.
- **SPIFFS File System Integration**: All downloads are stored using the SPIFFS filesystem.
- **Humanized Serial Debug Output**: All major events and errors are logged to the serial console for easy debugging.

---

## Hardware Requirements

- Microcontroller: ESP32 (Tested with ESP32 Dev boards)
- Storage: Internal ESP32 flash (SPIFFS)
- Connectivity: WiFi

---

## Required Libraries

All libraries are available via the Arduino Library Manager or included with the ESP32 platform:

- [`Arduino.h`](https://www.arduino.cc/reference/en/)
- [`WiFi.h`](https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi)
- [`SPIFFS.h`](https://github.com/espressif/arduino-esp32/tree/master/libraries/SPIFFS) (ESP32 SPIFFS filesystem)
- [`FS.h`](https://github.com/espressif/arduino-esp32/tree/master/libraries/FS) (Filesystem abstraction)
- [`HTTPClient.h`](https://github.com/espressif/arduino-esp32/tree/master/libraries/HTTPClient) (HTTP client library)
- `esp_heap_caps.h` (for advanced memory diagnostics, included with ESP32 Arduino core)

---

## Project Structure

- `main.ino` – Main program logic. Handles WiFi setup, SPIFFS mount, and performs downloads.
- `network_and_http.h/cpp` – WiFi and HTTP utilities (connect/disconnect, HTTP HEAD, hostname setup).
- `download_engines.h/cpp` – Download engine classes (`HttpDownloader`, `ResumeDownloader`). Handles all aspects of file download and resumption.
- `buffer_and_performance.h/cpp` – Buffer allocation, memory diagnostics, and performance monitoring.
- `spiffs_management.h/cpp` – SPIFFS file and storage management utilities.

---

## Setup Instructions

### 1. Arduino IDE & Board Setup

- Install [Arduino IDE](https://www.arduino.cc/en/software)
- Add ESP32 board support:
    - Open Arduino IDE → Tools → Board → Boards Manager
    - Search for "esp32" and install by Espressif Systems

### 2. Library Installation

Most libraries are installed automatically with ESP32 board support. If needed, use:
- Arduino IDE → Tools → Manage Libraries → Search and install: `WiFi`, `SPIFFS`, `FS`, `HTTPClient`

### 3. Project Configuration

- Clone this repository:
    ```sh
    git clone https://github.com/infiniteaurapower/VicharakProjectFinal.git
    ```
- Open `main.ino` in Arduino IDE

- Set your WiFi credentials and target download URL:
    ```cpp
    const char* WIFI_SSID = "YourNetwork";
    const char* WIFI_PASS = "YourPassword";
    const String DOWNLOAD_URL = "https://example.com/yourfile.bin";
    const String TARGET_PATH = "/downloaded.bin";
    ```

### 4. Upload & Run

- Connect your ESP32 via USB
- Select the correct board and port in Arduino IDE
- Click **Upload**
- Open Serial Monitor at 9600 baud to view diagnostic output

---

## SPIFFS Management

Functions are provided for listing, reading, saving, and deleting files in SPIFFS. See `spiffs_management.h/cpp` for all helper utilities.

---

## Advanced Diagnostics

The project includes advanced memory diagnostics and buffer management, accessible via serial output. It detects if double buffering is possible and prints heap statistics.

---

## Customization

- **Buffer Sizes**: Can be tuned in `buffer_and_performance.h` for different memory footprints.
- **Performance Thresholds**: Target speeds and update intervals are adjustable.
- **Download Logic**: Extend `HttpDownloader` or use `ResumeDownloader` for more features.

---

## Typical Serial Output

- System startup and buffer allocation diagnostics
- WiFi connection status
- Download progress, speed, and timing summaries
- SPIFFS file management results

---

## License

*No license specified. Please contact Vicharak Computers LLP for usage terms.*

---

## Author

Developed by [infiniteaurapower](https://github.com/infiniteaurapower) for Vicharak Computers LLP.
