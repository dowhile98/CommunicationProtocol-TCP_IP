# CommunicationProtocol-TCP_IP (clase3)

Resumen

- Proyecto de ejemplo integrando la pila CycloneTCP con drivers para ESP32.
- Objetivo: demostrar conexión WiFi en modo STA y obtención de IP mediante cliente DHCP.

Estructura del proyecto

- `main/`
  - `main.c`: Punto de entrada. Crea tareas (`WiFi Task`, `Sensor Task`), inicializa la pila TCP/IP (CycloneTCP), configura el driver WiFi ESP32 en modo STA y arranca el cliente DHCP.
  - `include/app_config.h`: Configuración de la aplicación (SSID, PSK, hostname). Valores por defecto que se pueden cambiar.
  - `cyclone_tcp/`: Código de la pila CycloneTCP (core, drivers, DHCP, IPv4, etc.).
    - `drivers/wifi/esp32_wifi_driver.c(.h)`: Driver que integra las APIs WiFi de ESP-IDF con la capa NIC de CycloneTCP.
    - `dhcp/`: Implementación del cliente DHCP utilizada para obtener dirección IPv4 dinámica.
    - `core/net.c/h`: Núcleo de la pila, gestión de interfaces, arranque/parada de interfaces.

Arquitectura

- Sistema multitarea (FreeRTOS mediante el puerto incluido en `common/os_port_freertos`).
- Tareas principales:
  - `WiFi Task`: inicializa la pila de red, asigna el driver WiFi, configura y conecta a la red WiFi (STA). Luego arranca el cliente DHCP de CycloneTCP para obtener una IP dinámica.
  - `Sensor Task`: placeholder para lectura/gestión de sensores (se ejecuta paralelamente).
- Pila de red:
  - CycloneTCP actúa como la pila TCP/IP: abstracción de interfaces (`NetInterface`), drivers NIC y módulos (IPv4, DHCP, DNS, etc.).
  - El driver `esp32_wifi_driver` usa las funciones internas de ESP-IDF (`esp_wifi_*`) para enviar/recibir tramas y notificar cambios de enlace a CycloneTCP.
  - DHCP client (CycloneTCP) gestiona el proceso de descubrimiento/solicitud y configura la IP en la interfaz virtual del stack.

Cómo configurar WiFi

- Por defecto, los valores están en `main/include/app_config.h`:
  - `WIFI_SSID` - Nombre de la red
  - `WIFI_PSK` - Contraseña
  - `WIFI_HOSTNAME` - Hostname del dispositivo
- Puedes sobrescribir estas macros desde `sdkconfig` o ajustando `app_config.h` antes de compilar.

Build & Flash (ESP-IDF)

1. Configurar el entorno ESP-IDF y exportar variables (ver documentación oficial de ESP-IDF).
2. Desde la carpeta del proyecto (`clase3/`):

```bash
idf.py set-target esp32
idf.py menuconfig   # (opcional) revisar sdkconfig
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

Notas y próximos pasos sugeridos

- Registrar eventos WiFi en `esp32_wifi_driver.c` para manejar reconexiones y notificaciones hacia CycloneTCP.
- Exponer la configuración WiFi en `menuconfig` (usar `Kconfig` y `sdkconfig`) para no editar headers manualmente.
- Añadir logs más detallados y manejo de errores (reintentos de conexión y recuperación del cliente DHCP).

Si quieres, puedo:

- Extraer la configuración WiFi a `sdkconfig` y añadir `Kconfig`.
- Añadir manejo de reconexión y callbacks más robustos en `esp32_wifi_driver.c`.

**_ Fin del README _**
