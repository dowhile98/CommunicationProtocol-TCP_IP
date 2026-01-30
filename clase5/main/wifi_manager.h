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

#include "core/net.h"
#include "dhcp/dhcp_client.h"
#include "dhcp/dhcp_server.h"
#include "ipv6/slaac.h"
#include "ipv6/ndp_router_adv.h"
#include "esp_wifi.h"
#include "esp_event.h"

/* ========================================================================== */
/*                           TIPOS Y ESTRUCTURAS                              */
/* ========================================================================== */

/**
 * @brief Estados de la máquina de estados WiFi
 */
typedef enum
{
    WIFI_DESCONECTADO = 0, /**< WiFi no iniciado o desconectado */
    WIFI_CONECTANDO,       /**< Intentando conectar al AP */
    WIFI_CONECTADO,        /**< Conectado exitosamente */
    WIFI_ERROR,            /**< Error en la conexión */
    WIFI_BACKOFF           /**< Esperando backoff antes de reintentar */
} WifiEstado_t;

/**
 * @brief Contexto del gestor WiFi
 * @details Contiene toda la información de estado y contextos de servicios
 */
typedef struct
{
    /* Máquina de estados */
    WifiEstado_t estado;               /**< Estado actual */
    WifiEstado_t estado_anterior;      /**< Estado anterior (para debug) */
    uint32_t intentos_reconexion;      /**< Contador de intentos */
    uint32_t delay_backoff_actual_ms;  /**< Delay actual de backoff */
    uint32_t timestamp_ultimo_intento; /**< Timestamp del último intento */

    /* Interfaces de red */
    NetInterface *interface_sta; /**< Interfaz STA (wlan0) */
    NetInterface *interface_ap;  /**< Interfaz AP (wlan1) */

    /* Contextos de servicios para STA */
    DhcpClientContext dhcp_client_ctx;       /**< Contexto cliente DHCP */
    DhcpClientSettings dhcp_client_settings; /**< Configuración DHCP cliente */
    SlaacContext slaac_ctx;                  /**< Contexto SLAAC (IPv6) */
    SlaacSettings slaac_settings;            /**< Configuración SLAAC */

    /* Contextos de servicios para AP */
    DhcpServerContext dhcp_server_ctx;            /**< Contexto servidor DHCP */
    DhcpServerSettings dhcp_server_settings;      /**< Configuración DHCP servidor */
    NdpRouterAdvContext ndp_router_adv_ctx;       /**< Contexto Router Advertisement */
    NdpRouterAdvSettings ndp_router_adv_settings; /**< Configuración RA */
    NdpRouterAdvPrefixInfo ndp_prefix_info[1];    /**< Info de prefijos IPv6 */

    /* Flags de estado */
    bool dhcp_obtenido;             /**< Flag: DHCP lease obtenido */
    bool ap_iniciado;               /**< Flag: AP iniciado correctamente */
    uint8_t clientes_ap_conectados; /**< Número de clientes en el AP */

} WifiManagerContext_t;

/* ========================================================================== */
/*                           PROTOTIPOS DE FUNCIONES                          */
/* ========================================================================== */

/**
 * @brief Inicializa el gestor WiFi y la pila TCP/IP
 * @details
 * Inicializa:
 * - Pila TCP/IP (CycloneTCP)
 * - Event loop de ESP-IDF
 * - Interfaz STA (wlan0)
 * - Interfaz AP (wlan1)
 * - Manejador de eventos WiFi
 *
 * @return error_t NO_ERROR si la inicialización fue exitosa
 */
error_t wifi_manager_init(void);

/**
 * @brief Inicia la conexión WiFi en modo dual (AP+STA)
 * @details
 * Configura y arranca:
 * - Modo STA: conecta al router configurado
 * - Modo AP: crea red WiFi propia
 *
 * @return error_t NO_ERROR si el inicio fue exitoso
 */
error_t wifi_manager_start(void);

/**
 * @brief Obtiene el estado actual del gestor WiFi
 * @return WifiEstado_t Estado actual de la máquina de estados
 */
WifiEstado_t wifi_manager_get_estado(void);

/**
 * @brief Obtiene el número de intentos de reconexión
 * @return uint32_t Número de intentos acumulados
 */
uint32_t wifi_manager_get_intentos_reconexion(void);

/**
 * @brief Obtiene el delay actual de backoff
 * @return uint32_t Delay en milisegundos
 */
uint32_t wifi_manager_get_delay_backoff(void);

/**
 * @brief Obtiene si el DHCP ha obtenido una IP
 * @return bool true si se obtuvo IP por DHCP
 */
bool wifi_manager_dhcp_obtenido(void);

/**
 * @brief Obtiene el número de clientes conectados al AP
 * @return uint8_t Número de clientes
 */
uint8_t wifi_manager_get_clientes_ap(void);

/**
 * @brief Convierte un estado WiFi a string para debug
 * @param estado Estado a convertir
 * @return const char* String descriptivo en español
 */
const char *wifi_manager_estado_to_string(WifiEstado_t estado);

/**
 * @brief Tarea principal del gestor WiFi
 * @details
 * Implementa la máquina de estados y gestiona:
 * - Transiciones de estado
 * - Reintentos con backoff exponencial
 * - Monitoreo de conexión
 *
 * @param params Parámetros de la tarea (no utilizados)
 */
void wifi_manager_task(void *params);

#endif /* WIFI_MANAGER_H */
