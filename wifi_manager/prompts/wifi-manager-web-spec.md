# Especificación Técnica: Wi-Fi Manager Moderno para ESP32 (CycloneTCP)

## 1. Introducción
El objetivo es crear una interfaz web moderna, responsiva y funcional para gestionar la conectividad Wi-Fi de un ESP32 utilizando la pila **CycloneTCP**. La interfaz permitirá configurar tanto el modo Estación (STA) como el Punto de Acceso (AP).

## 2. Requerimientos de la Interfaz (UI/UX)
- **Estética**: Diseño "Clean & Dark" o "Material Design" con transiciones suaves.
- **Componentes**:
    - **Dashboard**: Estado actual de la conexión (IP, SSID, Calidad de señal).
    - **Escaneo de Redes**: Tabla con SSIDs detectados, intensidad de señal y tipo de seguridad.
    - **Configuración STA**: Formulario para SSID, Password y toggle para DHCP/IP Estática.
    - **Configuración AP**: Formulario para SSID del dispositivo, Password, Canal y número máximo de conexiones.
    - **Notificaciones**: Toast messages para feedback (Éxito, Error, Procesando).

## 3. Especificación de la API (Endpoints JSON)

### GET `/api/wifi/config`
Obtiene la configuración actual almacenada en `WifiManagerConfig_t`.
- **Respuesta**:
  ```json
  {
    "sta": {
      "ssid": "MiRedWiFi",
      "dhcp": true,
      "ip": "192.168.1.50",
      "mask": "255.255.255.0",
      "gw": "192.168.1.1",
      "dns1": "8.8.8.8",
      "dns2": "8.8.4.4"
    },
    "ap": {
      "ssid": "ESP32-Config",
      "password": "password123",
      "ip": "192.168.4.1",
      "max_conn": 4,
      "dhcp_server": true,
      "range_min": "192.168.4.2",
      "range_max": "192.168.4.10"
    }
  }
  ```

### POST `/api/wifi/config`
Actualiza la configuración en caliente.
- **Cuerpo (Body)**: Mismo formato que la respuesta de GET.
- **Acción**: Actualiza la estructura en memoria y reinicia las interfaces de red.

### GET `/api/wifi/scan`
Inicia un escaneo y retorna las redes encontradas.
- **Respuesta**:
  ```json
  [
    {"ssid": "Network1", "rssi": -65, "secure": true},
    {"ssid": "FreeCoffee", "rssi": -80, "secure": false}
  ]
  ```

### POST `/api/wifi/restart`
Reinicia el dispositivo o solo el stack de red.

## 4. Tecnologías Web a utilizar
- **HTML5/CSS3**: Con variables CSS para fácil tematización.
- **JavaScript (ES6+)**: Fetch API para comunicación asíncrona.
- **Lucide Icons** o similar para iconografía moderna.

## 5. Implementación en ESP32
- **Backend**: C con CycloneTCP.
- **JSON**: ArduinoJson (v6/v7) para parsear y serializar.
- **Callbacks**: Manejo en `httpServerRequestCallback` discriminando por URI.
