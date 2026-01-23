/**
 * @file wifi_config.h
 * @brief Configuración de WiFi para modo dual AP+STA
 * @details
 * Contiene todos los defines de configuración para ambas interfaces:
 * - Interfaz 0: Modo STA (Cliente WiFi)
 * - Interfaz 1: Modo AP (Punto de Acceso)
 */

#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

/* ========================================================================== */
/*                   CONFIGURACIÓN INTERFAZ STA (wlan0)                       */
/* ========================================================================== */

/** Nombre de la interfaz STA */
#define APP_IF0_NAME "wlan0"

/** Hostname para la interfaz STA */
#define APP_IF0_HOST_NAME "esp32-curso-tcp"

/** Dirección MAC personalizada (00-00-00-00-00-00 = usar MAC de fábrica) */
#define APP_IF0_MAC_ADDR "00-00-00-00-00-00"

/** Habilitar cliente DHCP (ENABLED/DISABLED) */
#define APP_IF0_USE_DHCP_CLIENT ENABLED

/** Configuración IPv4 estática (solo si DHCP está DISABLED) */
#define APP_IF0_IPV4_HOST_ADDR "192.168.1.20"
#define APP_IF0_IPV4_SUBNET_MASK "255.255.255.0"
#define APP_IF0_IPV4_DEFAULT_GATEWAY "192.168.1.1"
#define APP_IF0_IPV4_PRIMARY_DNS "8.8.8.8"
#define APP_IF0_IPV4_SECONDARY_DNS "8.8.4.4"

/** Habilitar autoconfiguración IPv6 (SLAAC) */
#define APP_IF0_USE_SLAAC ENABLED

/** Dirección IPv6 link-local */
#define APP_IF0_IPV6_LINK_LOCAL_ADDR "fe80::32:1"

/* ========================================================================== */
/*                    CONFIGURACIÓN INTERFAZ AP (wlan1)                       */
/* ========================================================================== */

/** Nombre de la interfaz AP */
#define APP_IF1_NAME "wlan1"

/** Hostname para la interfaz AP */
#define APP_IF1_HOST_NAME "esp32-ap"

/** Dirección MAC personalizada */
#define APP_IF1_MAC_ADDR "00-00-00-00-00-00"

/** Habilitar servidor DHCP en el AP */
#define APP_IF1_USE_DHCP_SERVER ENABLED

/** Configuración IPv4 del AP (dirección estática) */
#define APP_IF1_IPV4_HOST_ADDR "192.168.8.1"
#define APP_IF1_IPV4_SUBNET_MASK "255.255.255.0"
#define APP_IF1_IPV4_DEFAULT_GATEWAY "0.0.0.0"
#define APP_IF1_IPV4_PRIMARY_DNS "0.0.0.0"
#define APP_IF1_IPV4_SECONDARY_DNS "0.0.0.0"

/** Rango de IPs para clientes DHCP */
#define APP_IF1_IPV4_ADDR_RANGE_MIN "192.168.8.10"
#define APP_IF1_IPV4_ADDR_RANGE_MAX "192.168.8.99"

/** Habilitar Router Advertisement (IPv6) */
#define APP_IF1_USE_ROUTER_ADV ENABLED

/** Configuración IPv6 del AP */
#define APP_IF1_IPV6_LINK_LOCAL_ADDR "fe80::32:2"
#define APP_IF1_IPV6_PREFIX "fd00:1:2:3::"
#define APP_IF1_IPV6_PREFIX_LENGTH 64
#define APP_IF1_IPV6_GLOBAL_ADDR "fd00:1:2:3::32:2"

/* ========================================================================== */
/*                      CREDENCIALES WIFI (MODO STA)                          */
/* ========================================================================== */

/** SSID del router al que se conectará el ESP32 en modo STA */
#ifndef WIFI_STA_SSID
#define WIFI_STA_SSID "Quino B."
#endif

/** Contraseña del router (modo STA) */
#ifndef WIFI_STA_PASSWORD
#define WIFI_STA_PASSWORD "987654321"
#endif

/* ========================================================================== */
/*                      CREDENCIALES WIFI (MODO AP)                           */
/* ========================================================================== */

/** SSID del AP que creará el ESP32 */
#ifndef WIFI_AP_SSID
#define WIFI_AP_SSID "ESP32_CURSO_TCP"
#endif

/** Contraseña del AP (mínimo 8 caracteres) */
#ifndef WIFI_AP_PASSWORD
#define WIFI_AP_PASSWORD "curso2025"
#endif

/** Máximo de estaciones simultáneas conectadas al AP */
#define WIFI_AP_MAX_CONNECTIONS 4

/* ========================================================================== */
/*                   CONFIGURACIÓN DE RECONEXIÓN (BACKOFF)                    */
/* ========================================================================== */

/** Retardo inicial de reconexión (milisegundos) */
#define WIFI_RECONNECT_DELAY_INITIAL_MS 1000

/** Retardo máximo de reconexión (milisegundos) */
#define WIFI_RECONNECT_DELAY_MAX_MS 16000

/** Número máximo de intentos antes de reportar fallo permanente (0 = infinito) */
#define WIFI_MAX_RECONNECT_ATTEMPTS 10

#endif /* WIFI_CONFIG_H */
