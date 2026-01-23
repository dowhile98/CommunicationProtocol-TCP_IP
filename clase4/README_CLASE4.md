# Clase 4: WiFi Dual (AP+STA) con Máquina de Estados

## 📋 Descripción

Este proyecto implementa un sistema WiFi dual para ESP32 utilizando CycloneTCP, que funciona simultáneamente como:

- **Estación (STA)**: Conecta a un router WiFi externo
- **Punto de Acceso (AP)**: Crea su propia red WiFi

Incluye una máquina de estados robusta con reconexión automática usando backoff exponencial.

## 🎯 Objetivos del Proyecto

✅ Configurar máquina de estados para manejar desconexiones WiFi  
✅ Implementar algoritmo de backoff exponencial  
✅ Habilitar modo dual (AP+STA) simultáneo  
✅ Logs detallados en español  
✅ Código modular y bien documentado

## 🏗️ Arquitectura del Sistema

```
┌─────────────────────────────────────────────────┐
│                   main.c                        │
│  - Inicialización del sistema                   │
│  - Tarea LED (indicador visual)                 │
│  - Bucle principal con estadísticas             │
└────────────────┬────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────┐
│              wifi_manager.c                     │
│  - Máquina de estados WiFi                      │
│  - Reconexión con backoff exponencial           │
│  - Inicialización interfaces STA y AP           │
│  - Manejador de eventos WiFi                    │
└─────────────────────────────────────────────────┘
                 │
                 ├──► STA (Cliente WiFi)
                 │    - DHCP Cliente
                 │    - SLAAC (IPv6)
                 │
                 └──► AP (Punto de Acceso)
                      - DHCP Servidor
                      - Router Advertisement (IPv6)
```

## 📁 Estructura de Archivos

```
main/
├── main.c                      # Punto de entrada, inicialización
├── wifi_manager.c              # Gestor WiFi con máquina de estados
├── wifi_manager.h              # Definiciones públicas del gestor
└── include/
    ├── wifi_config.h           # Configuración WiFi (credenciales, red)
    └── app_config.h            # Configuración general de la app
```

## 🔧 Configuración

### Credenciales WiFi (STA)

Editar en `main/include/wifi_config.h`:

```c
#define WIFI_STA_SSID "TU_RED_WIFI"
#define WIFI_STA_PASSWORD "TU_CONTRASEÑA"
```

### Red del Punto de Acceso (AP)

Editar en `main/include/wifi_config.h`:

```c
#define WIFI_AP_SSID "ESP32_CURSO_TCP"
#define WIFI_AP_PASSWORD "curso2025"
#define WIFI_AP_MAX_CONNECTIONS 4
```

### Configuración de Backoff

```c
#define WIFI_RECONNECT_DELAY_INITIAL_MS 1000   // 1 segundo inicial
#define WIFI_RECONNECT_DELAY_MAX_MS 16000      // 16 segundos máximo
#define WIFI_MAX_RECONNECT_ATTEMPTS 10         // 0 = infinito
```

## 🚀 Compilación y Flash

```bash
# Configurar target
idf.py set-target esp32

# Configurar opciones (opcional)
idf.py menuconfig

# Compilar
idf.py build

# Flash y monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

## 🔄 Máquina de Estados

El sistema implementa 5 estados:

| Estado           | Descripción                        | LED            |
| ---------------- | ---------------------------------- | -------------- |
| **DESCONECTADO** | WiFi no iniciado o detenido        | Rápido (200ms) |
| **CONECTANDO**   | Intentando conectar al router      | Medio (500ms)  |
| **CONECTADO**    | Conectado exitosamente             | Lento (1000ms) |
| **ERROR**        | Error en la conexión (transitorio) | Rápido (200ms) |
| **BACKOFF**      | Esperando antes de reintentar      | Medio (500ms)  |

### Flujo de Estados

```
DESCONECTADO
    │
    ▼
CONECTANDO ◄─────┐
    │            │
    ├─(éxito)──► CONECTADO
    │
    └─(fallo)──► ERROR ──► BACKOFF ──┘
                              │
                              └─(max intentos)──► DESCONECTADO
```

### Backoff Exponencial

Cada reintento duplica el tiempo de espera:

- Intento 1: 1 segundo
- Intento 2: 2 segundos
- Intento 3: 4 segundos
- Intento 4: 8 segundos
- Intento 5+: 16 segundos (máximo)

## 📊 Logs del Sistema

### Ejemplo de Inicio

```
I (532) Main: ╔════════════════════════════════════════════════╗
I (538) Main: ║   Curso ESP32 con CycloneTCP - Clase 4        ║
I (545) Main: ║   WiFi Dual (AP+STA) con Máquina de Estados   ║
I (552) Main: ╚════════════════════════════════════════════════╝
I (560) WiFiManager: Inicializando Gestor WiFi Dual (AP+STA)
I (567) WiFiManager: Inicializando pila TCP/IP...
I (573) WiFiManager: Pila TCP/IP inicializada correctamente
```

### Ejemplo de Conexión Exitosa

```
I (1245) WiFiManager: → Evento: STA conectado exitosamente
I (1246) WiFiManager:   SSID: RQUINOB
I (1248) WiFiManager:   Canal: 6
I (1251) WiFiManager:   BSSID: 4c:ed:fb:aa:bb:cc
I (2354) WiFiManager: ✓ IP obtenida por DHCP: 192.168.1.105
I (2355) WiFiManager: ✓ Gateway: 192.168.1.1
I (2356) WiFiManager: ✓ DNS primario: 192.168.1.1
```

### Ejemplo de Desconexión y Reconexión

```
W (15678) WiFiManager: → Evento: STA desconectado
W (15679) WiFiManager:   Razón: Timeout de beacon (señal perdida)
W (15680) WiFiManager:   RSSI: -78 dBm
I (15681) WiFiManager: Cambio de estado: CONECTADO → ERROR
I (15682) WiFiManager: Cambio de estado: ERROR → BACKOFF
I (15683) WiFiManager: Esperando 1000 ms antes de reintentar (intento 1)...
I (16684) WiFiManager: Reintentando conexión WiFi...
```

## 📡 Información de Red

### Interfaz STA (Cliente)

- **Nombre**: wlan0
- **Hostname**: esp32-curso-tcp
- **IPv4**: Obtenida por DHCP (default)
- **IPv6**: Autoconfiguración SLAAC

### Interfaz AP (Punto de Acceso)

- **Nombre**: wlan1
- **Hostname**: esp32-ap
- **SSID**: ESP32_CURSO_TCP
- **Contraseña**: curso2025
- **IPv4**: 192.168.8.1
- **Rango DHCP**: 192.168.8.10 - 192.168.8.99
- **IPv6**: fd00:1:2:3::32:2

## 🔍 API Pública del WiFi Manager

```c
/* Inicialización */
error_t wifi_manager_init(void);
error_t wifi_manager_start(void);

/* Consulta de estado */
WifiEstado_t wifi_manager_get_estado(void);
uint32_t wifi_manager_get_intentos_reconexion(void);
uint32_t wifi_manager_get_delay_backoff(void);
bool wifi_manager_dhcp_obtenido(void);
uint8_t wifi_manager_get_clientes_ap(void);

/* Utilidades */
const char* wifi_manager_estado_to_string(WifiEstado_t estado);

/* Tarea principal */
void wifi_manager_task(void *params);
```

## 🎓 Conceptos Implementados

1. **Máquina de Estados Finitos (FSM)**: Gestión robusta de estados WiFi
2. **Backoff Exponencial**: Evita saturar el router con reintentos
3. **Modo Dual WiFi**: STA y AP simultáneos en ESP32
4. **Event-Driven Architecture**: Manejadores de eventos asíncronos
5. **Separación de Responsabilidades**: Módulos bien definidos
6. **DHCP Cliente/Servidor**: Gestión automática de IPs
7. **IPv6 Ready**: Soporte completo para SLAAC y Router Advertisement

## 📝 Notas Importantes

- **Memoria**: El stack WiFi dual requiere ~4KB para la tarea principal
- **Prioridades**: WiFi Manager tiene prioridad alta para respuesta rápida
- **Thread-Safe**: El contexto WiFi es accedido desde múltiples tareas
- **NVS**: Requerida para el stack WiFi de ESP-IDF

## 🐛 Debug

Para aumentar el nivel de logging, modificar en `sdkconfig`:

```
CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y
```

O en código:

```c
esp_log_level_set("WiFiManager", ESP_LOG_DEBUG);
```

## 📚 Referencias

- [CycloneTCP Documentation](https://www.oryx-embedded.com/doc/cyclone_tcp/)
- [ESP-IDF WiFi API](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html)
- [RFC 4862 - IPv6 SLAAC](https://datatracker.ietf.org/doc/html/rfc4862)

## 👨‍💻 Autor

Proyecto educativo para curso de protocolos de comunicación con ESP32 y CycloneTCP.

---

**Versión**: 1.0  
**Fecha**: Enero 2026  
**Target**: ESP32 DevKit C
