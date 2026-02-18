/**
 * @file main.cpp
 * @brief Aplicación WiFi con CycloneTCP - WiFi Manager
 */

/* ========================================================================== */
/*                              INCLUSIONES                                   */
/* ========================================================================== */

#include "ArduinoJson.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <string.h>

extern "C" {
#include "core/ethernet.h" // Para ipv4AddrToString
#include "debug.h"
#include "http/http_common.h"
#include "http/http_server.h"
#include "http/mime.h"
#include "include/wifi_config.h"
#include "path.h"
#include "resource_manager.h"
#include "wifi_manager.h"
}

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
WifiManagerContext_t wifi_context;
SemaphoreHandle_t xHttpBufferMutex = NULL;

/* ========================================================================== */
/*                      PROTOTIPOS DE FUNCIONES                               */
/* ========================================================================== */

error_t httpServerCgiCallback(HttpConnection *connection, const char_t *param);
error_t httpServerRequestCallback(HttpConnection *connection,
                                  const char_t *uri);
error_t httpServerUriNotFoundCallback(HttpConnection *connection,
                                      const char_t *uri);

/* ========================================================================== */
/*                      IMPLEMENTACIÓN DE FUNCIONES                           */
/* ========================================================================== */

static void init_nvs(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  ESP_LOGI(TAG, "NVS Inicializada");
}

static void build_wifi_config(WifiManagerConfig_t *config) {
  memset(config, 0, sizeof(WifiManagerConfig_t));
  // Default STA config
  strncpy(config->sta_ssid, WIFI_STA_SSID, WIFI_MANAGER_SSID_MAX_LEN - 1);
  strncpy(config->sta_password, WIFI_STA_PASSWORD,
          WIFI_MANAGER_PASSWORD_MAX_LEN - 1);
  config->sta_use_dhcp = (APP_IF0_USE_DHCP_CLIENT == ENABLED);
  ipv4StringToAddr(APP_IF0_IPV4_HOST_ADDR, &config->sta_ipv4_addr);
  ipv4StringToAddr(APP_IF0_IPV4_SUBNET_MASK, &config->sta_subnet_mask);
  ipv4StringToAddr(APP_IF0_IPV4_DEFAULT_GATEWAY, &config->sta_gateway);
  ipv4StringToAddr(APP_IF0_IPV4_PRIMARY_DNS, &config->sta_dns1);
  ipv4StringToAddr(APP_IF0_IPV4_SECONDARY_DNS, &config->sta_dns2);

  // Default AP config
  strncpy(config->ap_ssid, WIFI_AP_SSID, WIFI_MANAGER_SSID_MAX_LEN - 1);
  strncpy(config->ap_password, WIFI_AP_PASSWORD,
          WIFI_MANAGER_PASSWORD_MAX_LEN - 1);
  config->ap_max_connections = WIFI_AP_MAX_CONNECTIONS;
  config->ap_use_dhcp_server = (APP_IF1_USE_DHCP_SERVER == ENABLED);
  ipv4StringToAddr(APP_IF1_IPV4_HOST_ADDR, &config->ap_ipv4_addr);
  ipv4StringToAddr(APP_IF1_IPV4_SUBNET_MASK, &config->ap_subnet_mask);
  ipv4StringToAddr(APP_IF1_IPV4_DEFAULT_GATEWAY, &config->ap_gateway);
  ipv4StringToAddr(APP_IF1_IPV4_PRIMARY_DNS, &config->ap_dns1);
  ipv4StringToAddr(APP_IF1_IPV4_SECONDARY_DNS, &config->ap_dns2);
  ipv4StringToAddr(APP_IF1_IPV4_ADDR_RANGE_MIN, &config->ap_dhcp_range_min);
  ipv4StringToAddr(APP_IF1_IPV4_ADDR_RANGE_MAX, &config->ap_dhcp_range_max);

  config->current_mode =
      WM_WIFI_MODE_APSTA; // Modo por defecto, usando el nuevo enum
}

/**
 * @brief Imprime la configuración WiFi en los logs
 */
static void log_wifi_config(const WifiManagerConfig_t *config) {
  char sta_ip[16], sta_mask[16], sta_gw[16], sta_dns1[16], sta_dns2[16];
  char ap_ip[16], ap_mask[16], ap_gw[16], ap_dns1[16], ap_dns2[16];
  char ap_dhcp_min[16], ap_dhcp_max[16];

  ipv4AddrToString(config->sta_ipv4_addr, sta_ip);
  ipv4AddrToString(config->sta_subnet_mask, sta_mask);
  ipv4AddrToString(config->sta_gateway, sta_gw);
  ipv4AddrToString(config->sta_dns1, sta_dns1);
  ipv4AddrToString(config->sta_dns2, sta_dns2);

  ipv4AddrToString(config->ap_ipv4_addr, ap_ip);
  ipv4AddrToString(config->ap_subnet_mask, ap_mask);
  ipv4AddrToString(config->ap_gateway, ap_gw);
  ipv4AddrToString(config->ap_dns1, ap_dns1);
  ipv4AddrToString(config->ap_dns2, ap_dns2);
  ipv4AddrToString(config->ap_dhcp_range_min, ap_dhcp_min);
  ipv4AddrToString(config->ap_dhcp_range_max, ap_dhcp_max);

  ESP_LOGI(TAG, "===== CONFIGURACIÓN WIFI =====");
  ESP_LOGI(TAG, "Modo Operativo: %d (0=OFF, 1=STA, 2=AP, 3=APSTA)",
           config->current_mode);
  ESP_LOGI(TAG, "--- Configuración STA ---");
  ESP_LOGI(TAG, "  SSID: %s", config->sta_ssid);
  ESP_LOGI(TAG, "  Password: %s", config->sta_password[0] ? "***" : "(vacío)");
  ESP_LOGI(TAG, "  Usar DHCP: %s", config->sta_use_dhcp ? "SÍ" : "NO");
  ESP_LOGI(TAG, "  IP: %s", sta_ip);
  ESP_LOGI(TAG, "  Máscara: %s", sta_mask);
  ESP_LOGI(TAG, "  Gateway: %s", sta_gw);
  ESP_LOGI(TAG, "  DNS1: %s", sta_dns1);
  ESP_LOGI(TAG, "  DNS2: %s", sta_dns2);
  ESP_LOGI(TAG, "--- Configuración AP ---");
  ESP_LOGI(TAG, "  SSID: %s", config->ap_ssid);
  ESP_LOGI(TAG, "  Password: %s", config->ap_password[0] ? "***" : "(vacío)");
  ESP_LOGI(TAG, "  Max Conexiones: %d", config->ap_max_connections);
  ESP_LOGI(TAG, "  Servidor DHCP: %s",
           config->ap_use_dhcp_server ? "SÍ" : "NO");
  ESP_LOGI(TAG, "  IP: %s", ap_ip);
  ESP_LOGI(TAG, "  Máscara: %s", ap_mask);
  ESP_LOGI(TAG, "  Gateway: %s", ap_gw);
  ESP_LOGI(TAG, "  DNS1: %s", ap_dns1);
  ESP_LOGI(TAG, "  DNS2: %s", ap_dns2);
  ESP_LOGI(TAG, "  Rango DHCP: %s - %s", ap_dhcp_min, ap_dhcp_max);
  ESP_LOGI(TAG, "==============================");
  ESP_LOGW(TAG, "NOTA: Configuración NO persistente (se pierde al reiniciar)");
}

#ifdef __cplusplus
extern "C" void app_main(void);
#endif

void app_main(void) {
  error_t error;
  WifiManagerConfig_t wifi_config;

  ESP_LOGI(TAG, "Iniciando Wi-Fi Manager...");
  init_nvs();

  build_wifi_config(&wifi_config);
  log_wifi_config(&wifi_config); // Mostrar configuración antes de aplicar

  WifiManager_SetConfig(&wifi_context, &wifi_config);

  error = WifiManager_Init(&wifi_context);
  if (error) {
    ESP_LOGE(TAG, "Error inicializando WiFi: %d", error);
  }

  // Crear Mutex para proteger buffers JSON compartidos
  xHttpBufferMutex = xSemaphoreCreateMutex();
  if (xHttpBufferMutex == NULL) {
    ESP_LOGE(TAG, "Error: No se pudo crear el Mutex HTTP");
  }

  // Configuración Servidor HTTP
  httpServerGetDefaultSettings(&httpServerSettings);

  for (uint8_t i = 0; i < APP_HTTP_MAX_CONNECTIONS; i++) {
    httpServerSettings.connectionTask[i].stackSize = 4096;
  }
  httpServerSettings.maxConnections = APP_HTTP_MAX_CONNECTIONS;
  httpServerSettings.connections = httpConnections;
  strcpy(httpServerSettings.rootDirectory, "/www/");
  strcpy(httpServerSettings.defaultDocument, "index.html");
  httpServerSettings.requestCallback = httpServerRequestCallback;
  httpServerSettings.uriNotFoundCallback = httpServerUriNotFoundCallback;

  error = httpServerInit(&httpServerContext, &httpServerSettings);
  if (!error) {
    httpServerStart(&httpServerContext);
    ESP_LOGI(TAG, "Servidor HTTP iniciado en puerto %d",
             httpServerSettings.port);
  }

  while (1) {
    osDelayTask(5000);
  }
}

/* ========================================================================== */
/*                    DOCUMENTOS JSON ESTÁTICOS (BSS)                        */
/* ========================================================================== */
// StaticJsonDocument<N> usa memoria estática (BSS) en vez de heap/stack
// Nota: ArduinoJson v7 marca StaticJsonDocument como deprecated, pero sigue
//       siendo la ÚNICA forma de evitar heap en embedded sin custom allocator
static StaticJsonDocument<512> s_json_doc_status;
static StaticJsonDocument<1536> s_json_doc_config;
static StaticJsonDocument<2048> s_json_doc_scan;
static StaticJsonDocument<1024> s_json_doc_post;
static char s_response_buffer[2048]; // Buffer de serialización HTTP

/**
 * @brief Helper: Construye respuesta JSON para GET /api/wifi/status
 * @note Sin alocación dinámica, sin stack overflow (usa BSS global)
 */
static error_t handle_get_status(HttpConnection *connection) {
  if (xSemaphoreTake(xHttpBufferMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
    connection->response.statusCode = 503; // Service Unavailable
    httpWriteHeader(connection);
    return httpCloseStream(connection);
  }

  s_json_doc_status.clear();
  char ip_str[16] = "0.0.0.0";

  // Obtener estado de interfaz STA
  bool_t sta_connected = FALSE;
  if (wifi_context.interface_sta != NULL) {
    sta_connected = wifi_context.interface_sta->linkState;
    if (sta_connected) {
      ipv4AddrToString(wifi_context.interface_sta->ipv4Context.addrList[0].addr,
                       ip_str);
    }
  }

  // Construir JSON
  s_json_doc_status["sta_connected"] = (bool)sta_connected;
  s_json_doc_status["sta_ssid"] = wifi_context.config.sta_ssid;
  s_json_doc_status["sta_ip"] = ip_str;
  s_json_doc_status["ap_clients"] = 0;
  s_json_doc_status["current_mode"] = (int)wifi_context.config.current_mode;

  // Serializar y enviar
  size_t n = serializeJson(s_json_doc_status, s_response_buffer,
                           sizeof(s_response_buffer));
  if (n == 0 || n >= sizeof(s_response_buffer)) {
    ESP_LOGE(TAG, "Error serializando JSON status");
    connection->response.statusCode = 500;
    httpWriteHeader(connection);
    return httpCloseStream(connection);
  }

  connection->response.contentType = "application/json";
  connection->response.contentLength = n;
  connection->response.statusCode = 200;
  httpWriteHeader(connection);
  httpWriteStream(connection, s_response_buffer, n);
  xSemaphoreGive(xHttpBufferMutex);
  return httpCloseStream(connection);
}

/**
 * @brief Helper: Construye respuesta JSON para GET /api/wifi/config
 * @note Sin alocación dinámica, sin stack overflow (usa BSS global)
 */
static error_t handle_get_config(HttpConnection *connection) {
  if (xSemaphoreTake(xHttpBufferMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
    connection->response.statusCode = 503;
    httpWriteHeader(connection);
    return httpCloseStream(connection);
  }

  s_json_doc_config.clear();

  // Buffers temporales para conversión de IPs (en stack, pero son pequeños)
  char sta_ip[16], sta_mask[16], sta_gw[16], sta_dns1[16], sta_dns2[16];
  char ap_ip[16], ap_mask[16], ap_gw[16], ap_dns1[16], ap_dns2[16];
  char ap_dhcp_min[16], ap_dhcp_max[16];

  // Convertir direcciones IP a strings
  ipv4AddrToString(wifi_context.config.sta_ipv4_addr, sta_ip);
  ipv4AddrToString(wifi_context.config.sta_subnet_mask, sta_mask);
  ipv4AddrToString(wifi_context.config.sta_gateway, sta_gw);
  ipv4AddrToString(wifi_context.config.sta_dns1, sta_dns1);
  ipv4AddrToString(wifi_context.config.sta_dns2, sta_dns2);

  ipv4AddrToString(wifi_context.config.ap_ipv4_addr, ap_ip);
  ipv4AddrToString(wifi_context.config.ap_subnet_mask, ap_mask);
  ipv4AddrToString(wifi_context.config.ap_gateway, ap_gw);
  ipv4AddrToString(wifi_context.config.ap_dns1, ap_dns1);
  ipv4AddrToString(wifi_context.config.ap_dns2, ap_dns2);
  ipv4AddrToString(wifi_context.config.ap_dhcp_range_min, ap_dhcp_min);
  ipv4AddrToString(wifi_context.config.ap_dhcp_range_max, ap_dhcp_max);

  // Construir objeto STA
  JsonObject sta_obj = s_json_doc_config["sta"].to<JsonObject>();
  sta_obj["ssid"] = wifi_context.config.sta_ssid;
  sta_obj["password"] = wifi_context.config.sta_password;
  sta_obj["use_dhcp"] = wifi_context.config.sta_use_dhcp;
  sta_obj["ip"] = sta_ip;
  sta_obj["mask"] = sta_mask;
  sta_obj["gw"] = sta_gw;
  sta_obj["dns1"] = sta_dns1;
  sta_obj["dns2"] = sta_dns2;

  // Construir objeto AP
  JsonObject ap_obj = s_json_doc_config["ap"].to<JsonObject>();
  ap_obj["ssid"] = wifi_context.config.ap_ssid;
  ap_obj["password"] = wifi_context.config.ap_password;
  ap_obj["max_connections"] = wifi_context.config.ap_max_connections;
  ap_obj["use_dhcp_server"] = wifi_context.config.ap_use_dhcp_server;
  ap_obj["ip"] = ap_ip;
  ap_obj["mask"] = ap_mask;
  ap_obj["gw"] = ap_gw;
  ap_obj["dns1"] = ap_dns1;
  ap_obj["dns2"] = ap_dns2;
  ap_obj["dhcp_range_min"] = ap_dhcp_min;
  ap_obj["dhcp_range_max"] = ap_dhcp_max;

  s_json_doc_config["current_mode"] = (int)wifi_context.config.current_mode;

  // Serializar y enviar
  size_t n = serializeJson(s_json_doc_config, s_response_buffer,
                           sizeof(s_response_buffer));
  if (n == 0 || n >= sizeof(s_response_buffer)) {
    ESP_LOGE(TAG, "Error serializando JSON config");
    connection->response.statusCode = 500;
    httpWriteHeader(connection);
    return httpCloseStream(connection);
  }

  connection->response.contentType = "application/json";
  connection->response.contentLength = n;
  connection->response.statusCode = 200;
  httpWriteHeader(connection);
  httpWriteStream(connection, s_response_buffer, n);
  xSemaphoreGive(xHttpBufferMutex);
  return httpCloseStream(connection);
}

/**
 * @brief Helper: Maneja GET /api/wifi/scan
 * @note Sin alocación dinámica, sin stack overflow (usa BSS global)
 */
static error_t handle_get_scan(HttpConnection *connection) {
  // Ejecutar escaneo
  error_t error = WifiManager_ScanNetworks(&wifi_context);
  if (error != NO_ERROR) {
    ESP_LOGE(TAG, "Error al escanear redes (error_t: %d)", error);
    connection->response.statusCode = 500;
    httpWriteHeader(connection);
    return httpCloseStream(connection);
  }

  ESP_LOGI(TAG, "Escaneo exitoso. Redes encontradas: %d",
           wifi_context.scanned_networks_count);

  ESP_LOGI(TAG, "Escaneo exitoso. Redes encontradas: %d",
           wifi_context.scanned_networks_count);

  if (xSemaphoreTake(xHttpBufferMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
    connection->response.statusCode = 503;
    httpWriteHeader(connection);
    return httpCloseStream(connection);
  }

  s_json_doc_scan.clear();
  JsonArray arr = s_json_doc_scan.to<JsonArray>();

  // Construir array de redes escaneadas
  for (int i = 0; i < wifi_context.scanned_networks_count; i++) {
    JsonObject net = arr.add<JsonObject>();
    net["ssid"] = wifi_context.scanned_networks[i].ssid;
    net["rssi"] = wifi_context.scanned_networks[i].rssi;

    // Mapear authmode a string legible
    const char *authmode_str = "UNKNOWN";
    switch (wifi_context.scanned_networks[i].authmode) {
    case WIFI_AUTH_OPEN:
      authmode_str = "OPEN";
      break;
    case WIFI_AUTH_WEP:
      authmode_str = "WEP";
      break;
    case WIFI_AUTH_WPA_PSK:
      authmode_str = "WPA_PSK";
      break;
    case WIFI_AUTH_WPA2_PSK:
      authmode_str = "WPA2_PSK";
      break;
    case WIFI_AUTH_WPA_WPA2_PSK:
      authmode_str = "WPA_WPA2_PSK";
      break;
    case WIFI_AUTH_WPA2_ENTERPRISE:
      authmode_str = "WPA2_ENTERPRISE";
      break;
    case WIFI_AUTH_WPA3_PSK:
      authmode_str = "WPA3_PSK";
      break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
      authmode_str = "WPA2_WPA3_PSK";
      break;
    default:
      break;
    }
    net["authmode"] = authmode_str;
  }

  // Serializar y enviar
  size_t n = serializeJson(s_json_doc_scan, s_response_buffer,
                           sizeof(s_response_buffer));
  if (n == 0 || n >= sizeof(s_response_buffer)) {
    ESP_LOGE(TAG, "Error serializando JSON scan");
    connection->response.statusCode = 500;
    httpWriteHeader(connection);
    return httpCloseStream(connection);
  }

  connection->response.contentType = "application/json";
  connection->response.contentLength = n;
  connection->response.statusCode = 200;
  httpWriteHeader(connection);
  httpWriteStream(connection, s_response_buffer, n);
  xSemaphoreGive(xHttpBufferMutex);
  return httpCloseStream(connection);
}

/**
 * @brief Helper: Maneja POST /api/wifi/config
 * @note Sin alocación dinámica, sin stack overflow (usa BSS global)
 */
static error_t handle_post_config(HttpConnection *connection) {
  char recv_buffer[1024];
  size_t received = 0;

  // Leer body de la petición POST
  error_t error = httpReadStream(connection, recv_buffer, sizeof(recv_buffer),
                                 &received, HTTP_FLAG_WAIT_ALL);
  if (error != NO_ERROR && error != ERROR_END_OF_STREAM) {
    ESP_LOGE(TAG, "Error leyendo POST body");
    connection->response.statusCode = 400;
    httpWriteHeader(connection);
    return httpCloseStream(connection);
  }

  // Parsear JSON
  if (xSemaphoreTake(xHttpBufferMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
    connection->response.statusCode = 503;
    httpWriteHeader(connection);
    return httpCloseStream(connection);
  }

  s_json_doc_post.clear();
  DeserializationError err =
      deserializeJson(s_json_doc_post, recv_buffer, received);
  if (err) {
    ESP_LOGE(TAG, "Error parsing JSON: %s", err.c_str());
    xSemaphoreGive(xHttpBufferMutex); // Liberar antes de salir
    connection->response.statusCode = 400;
    httpWriteHeader(connection);
    return httpCloseStream(connection);
  }

  // Clonar configuración actual y aplicar cambios
  WifiManagerConfig_t new_config = wifi_context.config;

  // Actualizar configuración STA
  if (s_json_doc_post["sta"].is<JsonObject>()) {
    JsonObject sta_json = s_json_doc_post["sta"];
    strlcpy(new_config.sta_ssid, sta_json["ssid"] | "",
            WIFI_MANAGER_SSID_MAX_LEN);
    strlcpy(new_config.sta_password, sta_json["password"] | "",
            WIFI_MANAGER_PASSWORD_MAX_LEN);
    new_config.sta_use_dhcp = sta_json["use_dhcp"] | true;

    // Configuración IP estática
    if (!new_config.sta_use_dhcp) {
      ipv4StringToAddr(sta_json["ip"] | "0.0.0.0", &new_config.sta_ipv4_addr);
      ipv4StringToAddr(sta_json["mask"] | "0.0.0.0",
                       &new_config.sta_subnet_mask);
      ipv4StringToAddr(sta_json["gw"] | "0.0.0.0", &new_config.sta_gateway);
      ipv4StringToAddr(sta_json["dns1"] | "0.0.0.0", &new_config.sta_dns1);
      ipv4StringToAddr(sta_json["dns2"] | "0.0.0.0", &new_config.sta_dns2);
    }
  }

  // Actualizar configuración AP
  if (s_json_doc_post["ap"].is<JsonObject>()) {
    JsonObject ap_json = s_json_doc_post["ap"];
    strlcpy(new_config.ap_ssid, ap_json["ssid"] | "",
            WIFI_MANAGER_SSID_MAX_LEN);
    strlcpy(new_config.ap_password, ap_json["password"] | "",
            WIFI_MANAGER_PASSWORD_MAX_LEN);
    new_config.ap_max_connections =
        ap_json["max_connections"] | WIFI_AP_MAX_CONNECTIONS;
    new_config.ap_use_dhcp_server = ap_json["use_dhcp_server"] | true;
    ipv4StringToAddr(ap_json["ip"] | "0.0.0.0", &new_config.ap_ipv4_addr);
    ipv4StringToAddr(ap_json["mask"] | "0.0.0.0", &new_config.ap_subnet_mask);
    ipv4StringToAddr(ap_json["gw"] | "0.0.0.0", &new_config.ap_gateway);
    ipv4StringToAddr(ap_json["dns1"] | "0.0.0.0", &new_config.ap_dns1);
    ipv4StringToAddr(ap_json["dns2"] | "0.0.0.0", &new_config.ap_dns2);
    ipv4StringToAddr(ap_json["dhcp_range_min"] | "0.0.0.0",
                     &new_config.ap_dhcp_range_min);
    ipv4StringToAddr(ap_json["dhcp_range_max"] | "0.0.0.0",
                     &new_config.ap_dhcp_range_max);
  }

  // Actualizar modo operativo
  if (s_json_doc_post["current_mode"].is<int>()) {
    new_config.current_mode =
        (wm_wifi_mode_t)(int)(s_json_doc_post["current_mode"]);
  }

  ESP_LOGI(TAG, "Aplicando nueva configuración. Modo: %d, STA SSID: %s",
           new_config.current_mode, new_config.sta_ssid);

  // Aplicar configuración
  WifiManager_SetConfig(&wifi_context, &new_config);
  WifiManager_SetOperatingMode(&wifi_context, new_config.current_mode);

  connection->response.statusCode = 200;
  httpWriteHeader(connection);
  xSemaphoreGive(xHttpBufferMutex);
  return httpCloseStream(connection);
}

/**
 * @brief Callback principal para peticiones HTTP
 * @note Refactorizado para ArduinoJson v7 con buffers estáticos (sin heap/stack
 * overflow)
 */
error_t httpServerRequestCallback(HttpConnection *connection,
                                  const char_t *uri) {
  if (connection == NULL || uri == NULL) {
    return ERROR_INVALID_PARAMETER;
  }

  connection->response.version = connection->request.version;
  connection->response.keepAlive = connection->request.keepAlive;

  // --- Endpoint: GET /api/wifi/status ---
  if (strcasecmp(uri, "/api/wifi/status") == 0 &&
      strcasecmp(connection->request.method, "GET") == 0) {
    return handle_get_status(connection);
  }

  // --- Endpoint: GET /api/wifi/config ---
  if (strcasecmp(uri, "/api/wifi/config") == 0 &&
      strcasecmp(connection->request.method, "GET") == 0) {
    return handle_get_config(connection);
  }

  // --- Endpoint: GET /api/wifi/scan ---
  if (strcasecmp(uri, "/api/wifi/scan") == 0 &&
      strcasecmp(connection->request.method, "GET") == 0) {
    return handle_get_scan(connection);
  }

  // --- Endpoint: POST /api/wifi/config ---
  if (strcasecmp(uri, "/api/wifi/config") == 0 &&
      strcasecmp(connection->request.method, "POST") == 0) {
    return handle_post_config(connection);
  }

  return ERROR_NOT_FOUND;
}

error_t httpServerUriNotFoundCallback(HttpConnection *connection,
                                      const char_t *uri) {
  return ERROR_NOT_FOUND;
}

error_t httpServerCgiCallback(HttpConnection *connection, const char_t *param) {
  return ERROR_INVALID_TAG;
}