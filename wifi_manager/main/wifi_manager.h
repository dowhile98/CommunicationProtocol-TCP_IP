/**
 * @file wifi_manager.h
 * @brief Gestor WiFi con máquina de estados y backoff exponencial
 * @details
 * Implementa una máquina de estados para gestionar la conexión WiFi
 * en modo dual (AP+STA) con reconexión automática usando backoff exponencial.
 *
 * Estados:
 * - WIFI_DESCONECTADO: Estado inicial, WiFi no iniciado
 * - WIFI_CONECTANDO: Intentando conectar al router
 * - WIFI_CONECTADO: Conectado exitosamente
 * - WIFI_ERROR: Error en la conexión
 * - WIFI_BACKOFF: Esperando antes de reintentar (backoff exponencial)
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "rom/gpio.h"
#include "core/net.h"
#include "drivers/wifi/esp32_wifi_driver.h"
#include "dhcp/dhcp_client.h"
#include "ipv6/slaac.h"

/* ========================================================================== */
/*                           TIPOS Y ESTRUCTURAS                              */
/* ========================================================================== */

/**
 * @brief Contexto del gestor WiFi
 * @details Contiene toda la información de estado y contextos de servicios
 */
#define WIFI_MANAGER_SSID_MAX_LEN 32U
#define WIFI_MANAGER_PASSWORD_MAX_LEN 64U

/**
 * @brief Configuración del gestor WiFi
 * @details Parámetros IPv4 y credenciales para STA/AP
 */
typedef struct
{
    char sta_ssid[WIFI_MANAGER_SSID_MAX_LEN];
    char sta_password[WIFI_MANAGER_PASSWORD_MAX_LEN];
    bool sta_use_dhcp;
    Ipv4Addr sta_ipv4_addr;
    Ipv4Addr sta_subnet_mask;
    Ipv4Addr sta_gateway;
    Ipv4Addr sta_dns1;
    Ipv4Addr sta_dns2;

    char ap_ssid[WIFI_MANAGER_SSID_MAX_LEN];
    char ap_password[WIFI_MANAGER_PASSWORD_MAX_LEN];
    bool ap_use_dhcp_server;
    uint8_t ap_max_connections;
    Ipv4Addr ap_ipv4_addr;
    Ipv4Addr ap_subnet_mask;
    Ipv4Addr ap_gateway;
    Ipv4Addr ap_dns1;
    Ipv4Addr ap_dns2;
    Ipv4Addr ap_dhcp_range_min;
    Ipv4Addr ap_dhcp_range_max;
} WifiManagerConfig_t;

/**
 * @brief Contexto del gestor WiFi
 * @details Contiene configuración, estado e interfaces de red
 */
typedef struct
{
    WifiManagerConfig_t config;
    bool config_set;

    /* Interfaces de red */
    NetInterface *interface_sta; /**< Interfaz STA (wlan0) */
    NetInterface *interface_ap;  /**< Interfaz AP (wlan1) */

    /* Contextos de servicios para STA */
    DhcpClientContext dhcp_client_ctx;       /**< Contexto cliente DHCP */
    DhcpClientSettings dhcp_client_settings; /**< Configuración DHCP cliente */

    /* Contextos de servicios para AP */
    DhcpServerContext dhcp_server_ctx;       /**< Contexto servidor DHCP */
    DhcpServerSettings dhcp_server_settings; /**< Configuración DHCP servidor */
} WifiManagerContext_t;

/* ========================================================================== */
/*                           PROTOTIPOS DE FUNCIONES                          */
/* ========================================================================== */

/**
 * @brief Inyecta o actualiza la configuración del gestor WiFi
 * @param[in,out] context Contexto del gestor WiFi
 * @param[in] config Configuración deseada
 * @return error_t NO_ERROR si la operación fue exitosa
 */
error_t WifiManager_SetConfig(WifiManagerContext_t *context,
                              const WifiManagerConfig_t *config);

/**
 * @brief Inicializa el gestor WiFi y la pila TCP/IP
 * @details
 * Inicializa:
 * - Pila TCP/IP (CycloneTCP)
 * - Event loop de ESP-IDF
 * - Interfaz STA (wlan0)
 * - Interfaz AP (wlan1)
 * - Wi-Fi en modo AP+STA
 *
 * @param[in,out] context Contexto del gestor WiFi
 * @return error_t NO_ERROR si la inicialización fue exitosa
 */
error_t WifiManager_Init(WifiManagerContext_t *context);

#endif /* WIFI_MANAGER_H */
