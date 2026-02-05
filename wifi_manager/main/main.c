/**
 * @file main.c
 * @brief Aplicación WiFi con CycloneTCP - Modo Dual AP+STA
 * @details
 * Implementa un sistema WiFi dual que funciona simultáneamente como:
 * - Estación (STA): Conecta a un router WiFi externo
 * - Punto de Acceso (AP): Crea su propia red WiFi
 *
 * Características:
 * - Máquina de estados con reconexión automática
 * - Backoff exponencial para reintentos
 * - Logs detallados en español
 * - DHCP cliente y servidor
 * - Soporte IPv4 e IPv6
 */

/* ========================================================================== */
/*                              INCLUSIONES                                   */
/* ========================================================================== */

#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_manager.h"
#include "include/wifi_config.h"
#include <string.h>
#include "http/http_server.h"
#include "http/mime.h"
#include "http/http_common.h"
#include "path.h"
#include "resource_manager.h"
#include "debug.h"
/* ========================================================================== */
/*                               CONSTANTES                                   */
/* ========================================================================== */
static const char *TAG = "Main";
#define APP_HTTP_MAX_CONNECTIONS 2
/* ========================================================================== */
/*                          VARIABLES GLOBALES                                */
/* ========================================================================== */
HttpServerSettings httpServerSettings;
HttpServerContext httpServerContext;
HttpConnection httpConnections[APP_HTTP_MAX_CONNECTIONS];
/* ========================================================================== */
/*                      PROTOTIPOS DE FUNCIONES                               */
/* ========================================================================== */

error_t httpServerCgiCallback(HttpConnection *connection,
                              const char_t *param);

error_t httpServerRequestCallback(HttpConnection *connection,
                                  const char_t *uri);

error_t httpServerUriNotFoundCallback(HttpConnection *connection,
                                      const char_t *uri);
/* ========================================================================== */
/*                      IMPLEMENTACIÓN DE FUNCIONES                           */
/* ========================================================================== */

/**
 * @brief Inicializa la memoria NVS (Non-Volatile Storage)
 * @details Requerida por el stack WiFi de ESP-IDF
 */
static void init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NVS corrupta, formateando...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Memoria NVS inicializada correctamente");
    }
    else
    {
        ESP_LOGE(TAG, "Error al inicializar NVS: %d", ret);
    }
}

/**
 * @brief Punto de entrada principal de la aplicación
 */
static void build_wifi_config(WifiManagerConfig_t *config)
{
    if (config == NULL)
    {
        return;
    }

    memset(config, 0, sizeof(WifiManagerConfig_t));

    strncpy(config->sta_ssid, WIFI_STA_SSID, WIFI_MANAGER_SSID_MAX_LEN - 1U);
    strncpy(config->sta_password, WIFI_STA_PASSWORD,
            WIFI_MANAGER_PASSWORD_MAX_LEN - 1U);
    config->sta_use_dhcp = (APP_IF0_USE_DHCP_CLIENT == ENABLED);

    ipv4StringToAddr(APP_IF0_IPV4_HOST_ADDR, &config->sta_ipv4_addr);
    ipv4StringToAddr(APP_IF0_IPV4_SUBNET_MASK, &config->sta_subnet_mask);
    ipv4StringToAddr(APP_IF0_IPV4_DEFAULT_GATEWAY, &config->sta_gateway);
    ipv4StringToAddr(APP_IF0_IPV4_PRIMARY_DNS, &config->sta_dns1);
    ipv4StringToAddr(APP_IF0_IPV4_SECONDARY_DNS, &config->sta_dns2);

    strncpy(config->ap_ssid, WIFI_AP_SSID, WIFI_MANAGER_SSID_MAX_LEN - 1U);
    strncpy(config->ap_password, WIFI_AP_PASSWORD,
            WIFI_MANAGER_PASSWORD_MAX_LEN - 1U);
    config->ap_use_dhcp_server = (APP_IF1_USE_DHCP_SERVER == ENABLED);
    config->ap_max_connections = WIFI_AP_MAX_CONNECTIONS;

    ipv4StringToAddr(APP_IF1_IPV4_HOST_ADDR, &config->ap_ipv4_addr);
    ipv4StringToAddr(APP_IF1_IPV4_SUBNET_MASK, &config->ap_subnet_mask);
    ipv4StringToAddr(APP_IF1_IPV4_DEFAULT_GATEWAY, &config->ap_gateway);
    ipv4StringToAddr(APP_IF1_IPV4_PRIMARY_DNS, &config->ap_dns1);
    ipv4StringToAddr(APP_IF1_IPV4_SECONDARY_DNS, &config->ap_dns2);
    ipv4StringToAddr(APP_IF1_IPV4_ADDR_RANGE_MIN,
                     &config->ap_dhcp_range_min);
    ipv4StringToAddr(APP_IF1_IPV4_ADDR_RANGE_MAX,
                     &config->ap_dhcp_range_max);
}

void app_main(void)
{
    error_t error;
    WifiManagerContext_t wifi_context;
    WifiManagerConfig_t wifi_config;

    /* Banner de inicio */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   Curso ESP32 con CycloneTCP - Clase 5        ║");
    ESP_LOGI(TAG, "║   WiFi Dual (AP+STA) con Máquina de Estados   ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Compilado: %s %s", __DATE__, __TIME__);
    ESP_LOGI(TAG, "");

    /* Inicializar NVS */
    ESP_LOGI(TAG, "→ Inicializando subsistemas...");
    init_nvs();

    memset(&wifi_context, 0, sizeof(WifiManagerContext_t));
    build_wifi_config(&wifi_config);

    error = WifiManager_SetConfig(&wifi_context, &wifi_config);
    if (error)
    {
        ESP_LOGE(TAG, "Error configurando WiFi: %d", error);
    }

    error = WifiManager_Init(&wifi_context);
    if (error)
    {
        ESP_LOGE(TAG, "Error inicializando WiFi: %d", error);
    }

    // HTTP server init
    // Get default settings
    httpServerGetDefaultSettings(&httpServerSettings);
    // Bind HTTP server to the desired interface
    httpServerSettings.interface = NULL;
    // Listen to port 80
    httpServerSettings.port = HTTP_PORT;
    // Client connections
    httpServerSettings.maxConnections = APP_HTTP_MAX_CONNECTIONS;
    httpServerSettings.connections = httpConnections;
    // Specify the server's root directory
    strcpy(httpServerSettings.rootDirectory, "/www/");
    // Set default home page
    strcpy(httpServerSettings.defaultDocument, "index.html");
    // Callback functions
    httpServerSettings.cgiCallback = httpServerCgiCallback;
    httpServerSettings.requestCallback = httpServerRequestCallback;
    httpServerSettings.uriNotFoundCallback = httpServerUriNotFoundCallback;

    // HTTP server initialization
    error = httpServerInit(&httpServerContext, &httpServerSettings);
    // Failed to initialize HTTP server?
    if (error)
    {
        // Debug message
        TRACE_ERROR("Failed to initialize HTTP server!\r\n");
    }

    // Start HTTP server
    error = httpServerStart(&httpServerContext);
    // Failed to start HTTP server?
    if (error)
    {
        // Debug message
        TRACE_ERROR("Failed to start HTTP server!\r\n");
    }

    while (1)
    {
        osDelayTask(5000); /* Delay de 5 segundos */
    }
}

/**
 * @brief CGI callback function
 * @param[in] connection Handle referencing a client connection
 * @param[in] param NULL-terminated string that contains the CGI parameter
 * @return Error code
 **/

error_t httpServerCgiCallback(HttpConnection *connection,
                              const char_t *param)
{

    return ERROR_INVALID_TAG;
}

/**
 * @brief HTTP request callback
 * @param[in] connection Handle referencing a client connection
 * @param[in] uri NULL-terminated string containing the path to the requested resource
 * @return Error code
 **/

error_t httpServerRequestCallback(HttpConnection *connection,
                                  const char_t *uri)
{
    ESP_LOGI(TAG, "uri: %s", uri);
    // api/control/led -> payload JSON
    if (strcasecmp(uri, "/api/control/led") == 0)
    {
        httpReadStream(connection, data, 128, &received, HTTP_FLAG_WAIT_ALL);
        //->libreria json (ArduinoJson v6.xxx)
        //-> sdk thinsboard
        // write header

        // write stream

        // close

        ESP_LOGI(TAG, "se solicita la api wifi scan");
    }
    // Not implemented
    return ERROR_NOT_FOUND;
}

/**
 * @brief URI not found callback
 * @param[in] connection Handle referencing a client connection
 * @param[in] uri NULL-terminated string containing the path to the requested resource
 * @return Error code
 **/

error_t httpServerUriNotFoundCallback(HttpConnection *connection,
                                      const char_t *uri)
{
    // Not implemented
    ESP_LOGI(TAG, "uri: %s not fond", uri);

    return ERROR_NOT_FOUND;
}
