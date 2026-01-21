/**
 * @file app_config.h
 * @brief Configuración de la aplicación (WiFi y parámetros)
 *
 * Valores por defecto para el SSID, PSK y hostname. Se pueden cambiar
 * editando este archivo o definiendo las macros en la configuración
 * del build (por ejemplo en `sdkconfig.h`).
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* SSID de la red WiFi STA */
#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_SSID"
#endif

/* Contraseña de la red WiFi STA (PSK) */
#ifndef WIFI_PSK
#define WIFI_PSK "YOUR_PSK"
#endif

/* Hostname que se registrará en la interfaz de red */
#ifndef WIFI_HOSTNAME
#define WIFI_HOSTNAME "esp32-device"
#endif

#endif // APP_CONFIG_H
