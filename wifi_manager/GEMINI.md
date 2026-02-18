# GEMINI.md - Project Context: wifi_manager (ESP32 + CycloneTCP)

## Project Overview
This project is an ESP32 application that implements a **Wi-Fi Manager** using the **CycloneTCP** stack instead of the standard ESP-IDF LwIP. It is designed to work in **Dual Mode (AP+STA)**, allowing the device to connect to an existing network as a station while simultaneously acting as an access point.

Key features include:
- **Dual Wi-Fi Mode**: Simultaneous Access Point (AP) and Station (STA) operation.
- **CycloneTCP Integration**: Custom networking stack implementation on ESP32.
- **HTTP Server**: Serves an embedded web interface for device control (e.g., LED control) and status monitoring.
- **DHCP Support**: Includes both a DHCP Client (for STA mode) and a DHCP Server (for AP mode).
- **State Machine**: Managed Wi-Fi connection logic with automatic reconnection.

## Key Technologies
- **Hardware**: ESP32 (developed/tested on ESP32 DevKitC).
- **Framework**: ESP-IDF (recommended v4.2 or superior).
- **Networking**: CycloneTCP (v2.5.4) by Oryx Embedded.
- **RTOS**: FreeRTOS (integrated with ESP-IDF).
- **Frontend**: HTML5, CSS3, and Vanilla JavaScript (served from internal flash).

## Project Structure
- `main/`: Core application source code.
    - `main.c`: Application entry point (`app_main`), initialization of NVS and HTTP server, and API callback handling.
    - `wifi_manager.c/h`: Logic for managing Wi-Fi interfaces, DHCP services, and connectivity state.
    - `http_server_callbacks.c`: (Note: Contains template code for firmware updates and data handling, check for overlaps with `main.c`).
    - `res.c`: Embedded web resources (HTML/CSS/JS) compiled as a C array.
    - `cyclone_tcp/`: The CycloneTCP stack source and drivers.
    - `common/`: OS porting layer and utility functions (debug, error, string handling).
    - `include/`: Configuration headers (`wifi_config.h`, `app_config.h`).
- `resources/www/`: Source files for the web interface (HTML, assets).
- `prompts/`: Project specifications and UI design guidelines (`wifi-manager-ui-spec.md`).

## Building and Running
The project uses the standard ESP-IDF build system (CMake).

```bash
# Set up the ESP-IDF environment (if not already done)
. $HOME/esp/esp-idf/export.sh

# Build the project
idf.py build

# Flash the firmware to the ESP32
idf.py flash

# Open the serial monitor to view logs
idf.py monitor
```

## Development Conventions
- **Language**: C (C99/C11 compatible).
- **Error Handling**: Uses `error_t` from CycloneTCP and `esp_err_t` from ESP-IDF. Always check return values of initialization functions.
- **Logging**: Uses `ESP_LOG` macros for output. Logs are primarily in Spanish.
- **Naming Conventions**:
    - Public Functions: `Prefix_FunctionName` (e.g., `WifiManager_Init`).
    - Private Functions/Variables: `snake_case` (e.g., `wifi_manager_event_handler`).
- **Web Resources**: Web files located in `resources/www/` are embedded into the binary via `main/res.c`. Updates to the UI require regenerating or updating `res.c`.

## Important Configuration Files
- `main/net_config.h`: CycloneTCP stack configuration (IPv4/IPv6, protocols enabled).
- `main/os_port_config.h`: OS abstraction layer configuration (FreeRTOS).
- `sdkconfig`: ESP-IDF project configuration (can be modified via `idf.py menuconfig`).
- `main/include/wifi_config.h`: Default SSIDs, passwords, and IP settings.
