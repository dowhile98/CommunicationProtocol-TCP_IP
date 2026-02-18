# Prompt: WiFi Manager UI & API (ESP32 CycloneTCP)

## Contexto
- Target: ESP32 + ESP-IDF + CycloneTCP, UI servida desde `resources/www/`.
- Backend expone callbacks en `main.c` vía `httpServerRequestCallback` (ejemplo actual `/api/control/led`).
- Configuración WiFi manejada por `WifiManagerConfig_t` (`main/wifi_manager.h`).
- Librería JSON: ArduinoJson v7 (ya integrada).

## Objetivo
Diseñar una página web moderna (SPA ligera, sin build tools) para:
1) Mostrar configuración WiFi actual (STA/AP).
2) Editar y enviar nueva configuración.
3) Ejecutar acciones (scan de redes, reiniciar WiFi, guardar/aplicar).
4) Mostrar estado/feedback (éxito/error/progreso).

## UI Layout (HTML/CSS/JS puros)
- Single-page con secciones en tarjetas:
  - Estado general (modo, IPs, conexión STA/AP).
  - Configuración STA (SSID, password, DHCP on/off, IPv4 estático: IP, máscara, gateway, DNS1/DNS2).
  - Configuración AP (SSID, password, DHCP server on/off, max conexiones, IP, máscara, gateway, DNS1/DNS2, rango DHCP).
  - Acciones rápidas: Scan, Aplicar cambios, Reiniciar WiFi, Exportar/Importar JSON.
- Estilo limpio: fondo claro con acentos azules/teal, tarjetas con sombras suaves, tipografía `"IBM Plex Sans", "Inter", system`.
- Controles:
  - Inputs de texto para SSID/clave (con toggle de visibilidad).
  - Switches (checkbox estilizado) para DHCP/DHCP server.
  - IP inputs como campos de texto validados (IPv4).
  - Botón "Scan" que abre modal/lista con redes encontradas (SSID, RSSI, seguridad) y permite autocompletar STA SSID.
  - Toasts para feedback; spinner para acciones en curso.

## API Contract (propuesto)
- `GET /api/wifi/config` → devuelve JSON de `WifiManagerConfig_t`:
  ```json
  {
    "sta": {
      "ssid": "string",
      "password": "string",
      "useDhcp": true,
      "ipv4": {"addr":"0.0.0.0","mask":"0.0.0.0","gw":"0.0.0.0","dns1":"0.0.0.0","dns2":"0.0.0.0"}
    },
    "ap": {
      "ssid": "string",
      "password": "string",
      "useDhcpServer": true,
      "maxConnections": 4,
      "ipv4": {"addr":"0.0.0.0","mask":"0.0.0.0","gw":"0.0.0.0","dns1":"0.0.0.0","dns2":"0.0.0.0","rangeMin":"0.0.0.0","rangeMax":"0.0.0.0"}
    }
  }
  ```
- `POST /api/wifi/config` (Content-Type: application/json) → aplica nueva config. Body con misma forma. Responde `{ "status": "ok" }` o error.
- `POST /api/wifi/scan` → inicia escaneo STA, responde lista:
  ```json
  {"networks":[{"ssid":"ssid","rssi":-45,"sec":"WPA2"}]}
  ```
- `POST /api/wifi/restart` → reinicia stack WiFi y aplica config actual, responde ok.
- Errores: usar HTTP 400/500 y cuerpo `{ "status": "error", "message": "..." }`.

## Frontend JS (sin build)
- Utilizar `fetch` con `async/await`; manejar timeouts con `AbortController` (p.ej. 8s).
- Validación básica en cliente: SSID no vacío, contraseñas opcionales pero si WPA requiere >=8 chars, IPs válidas cuando DHCP está OFF.
- Funciones clave:
  - `loadConfig()` → GET config, pintar formulario.
  - `submitConfig()` → POST config; deshabilitar botones durante envío; mostrar toast.
  - `scanNetworks()` → POST scan, mostrar modal con lista, permitir click para autocompletar SSID.
  - `restartWifi()` → POST restart, mostrar progreso y resultado.
  - `exportJson()` / `importJson(file)` para backup/restore local (no envía auto hasta confirmar).

## Backend (ESP32) notas
- Callback en `main.c` (`httpServerRequestCallback`) debe:
  - Parsear método y URI (`/api/wifi/config`, `/api/wifi/scan`, `/api/wifi/restart`).
  - Leer body en buffer estático (ArduinoJson deserializa a estructura intermedia, luego mapear a `WifiManagerConfig_t`).
  - Para GET config: serializar `WifiManagerConfig_t` → JSON (ArduinoJson `StaticJsonDocument`).
  - Para POST config: validar longitudes (`WIFI_MANAGER_SSID_MAX_LEN`, `PASSWORD_MAX_LEN`), copiar a `WifiManagerContext_t`, guardar en NVS opcional.
  - Para scan: usar `esp_wifi_scan_start` y recoger resultados; devolver lista.
  - Para restart: parar y reconfigurar WiFi manager (reusar `WifiManager_SetConfig` + reinicio servicios TCP/IP si aplica).
  - Responder con `httpWriteHeader` + `httpWriteStream`; set `Content-Type: application/json`.
- Manejar errores con códigos CycloneTCP (`error_t`) y retornar `ERROR_NOT_FOUND` en rutas no manejadas.

## Flujo UX esperado
1) Al cargar, `loadConfig()` muestra config actual y estado (mostrar IP STA/AP si se exponen vía endpoint futuro `/api/wifi/status`).
2) Usuario edita campos, hace clic en "Aplicar" → POST → feedback.
3) Puede escanear redes, elegir una, rellenar SSID, luego aplicar.
4) Puede reiniciar WiFi para forzar reconexión.

## Entregables
- HTML/CSS/JS single-file `resources/www/index.html` (modernizado) siguiendo esta guía.
- Implementación backend en `main.c` para endpoints de config/scan/restart usando ArduinoJson y CycloneTCP/ESP-IDF APIs.

## Estilo y accesibilidad
- Respetar español en labels y mensajes.
- Botones grandes y contrastados; formularios responsivos (mobile-first).
- Estados de carga y error visibles; evitar bloqueos largos sin feedback.
