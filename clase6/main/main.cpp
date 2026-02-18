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

extern "C"
{
#include "core/net.h" // Para ipv4AddrToString y tipos de red (MacAddr, Eui64, etc.)
#include "debug.h"
#include "mqtt/mqtt_client.h"
#include "include/wifi_config.h"
#include "path.h"
#include "resource_manager.h"
#include "wifi_manager.h"
}

/* ========================================================================== */
/*                               CONSTANTES                                   */
/* ========================================================================== */
static const char *TAG = "Main";

// MQTT server name
#define APP_SERVER_NAME "192.168.1.45"
// MQTT server port
#define APP_SERVER_PORT 1883 // MQTT over TCP
// #define APP_SERVER_PORT 8883 //MQTT over TLS
// #define APP_SERVER_PORT 8884 //MQTT over TLS (mutual authentication)
// #define APP_SERVER_PORT 8080 //MQTT over WebSocket
// #define APP_SERVER_PORT 8081 //MQTT over secure WebSocket
/* ========================================================================== */
/*                          VARIABLES GLOBALES                                */
/* ========================================================================== */
MqttClientContext mqttClientContext;
WifiManagerContext_t wifi_context;
SemaphoreHandle_t dhcpFlag;
/* ========================================================================== */
/*                      PROTOTIPOS DE FUNCIONES                               */
/* ========================================================================== */
/**
 * @brief Publish callback function
 * @param[in] context Pointer to the MQTT client context
 * @param[in] topic Topic name
 * @param[in] message Message payload
 * @param[in] length Length of the message payload
 * @param[in] dup Duplicate delivery of the PUBLISH packet
 * @param[in] qos QoS level used to publish the message
 * @param[in] retain This flag specifies if the message is to be retained
 * @param[in] packetId Packet identifier
 **/

void mqttPublishCallback(MqttClientContext *context,
                         const char_t *topic, const uint8_t *message, size_t length,
                         bool_t dup, MqttQosLevel qos, bool_t retain, uint16_t packetId);

error_t mqttTestConnect(void);

#ifdef __cplusplus
extern "C" void dhcpClientStateChangeCallback(DhcpClientContext *context,
                                                   NetInterface *interface, DhcpState state);
#endif
/* ========================================================================== */
/*                      IMPLEMENTACIÓN DE FUNCIONES                           */
/* ========================================================================== */

static void init_nvs(void)
{
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  ESP_LOGI(TAG, "NVS Inicializada");
}

static void build_wifi_config(WifiManagerConfig_t *config)
{
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
static void log_wifi_config(const WifiManagerConfig_t *config)
{
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

void app_main(void)
{
  error_t error;
  WifiManagerConfig_t wifi_config;

  ESP_LOGI(TAG, "Iniciando Wi-Fi Manager...");
  init_nvs();

  dhcpFlag = xSemaphoreCreateBinary();
  if (dhcpFlag == NULL)
  {
    ESP_LOGE(TAG, "Error creando semáforo DHCP");
    return;
  }

  build_wifi_config(&wifi_config);
  log_wifi_config(&wifi_config); // Mostrar configuración antes de aplicar

  WifiManager_SetConfig(&wifi_context, &wifi_config);

  error = WifiManager_Init(&wifi_context);
  if (error)
  {
    ESP_LOGE(TAG, "Error inicializando WiFi: %d", error);
  }
  ESP_LOGI(TAG, "WiFi Iniciado");

  /*mqtt*/
  xSemaphoreTake(dhcpFlag, portMAX_DELAY);
  mqttTestConnect();
  while (1)
  {
    osDelayTask(5000);
  }
}

/* ========================================================================== */
/*                      IMPLEMENTACIÓN DE FUNCIONES                           */
/* ========================================================================== */
/**
 * @brief Publish callback function
 * @param[in] context Pointer to the MQTT client context
 * @param[in] topic Topic name
 * @param[in] message Message payload
 * @param[in] length Length of the message payload
 * @param[in] dup Duplicate delivery of the PUBLISH packet
 * @param[in] qos QoS level used to publish the message
 * @param[in] retain This flag specifies if the message is to be retained
 * @param[in] packetId Packet identifier
 **/

void mqttPublishCallback(MqttClientContext *context,
                         const char_t *topic, const uint8_t *message, size_t length,
                         bool_t dup, MqttQosLevel qos, bool_t retain, uint16_t packetId)
{
  // Debug message
  TRACE_INFO("PUBLISH packet received...\r\n");
  TRACE_INFO("  Dup: %u\r\n", dup);
  TRACE_INFO("  QoS: %u\r\n", qos);
  TRACE_INFO("  Retain: %u\r\n", retain);
  TRACE_INFO("  Packet Identifier: %u\r\n", packetId);
  TRACE_INFO("  Topic: %s\r\n", topic);
  TRACE_INFO("  Message (%" PRIuSIZE " bytes):\r\n", length);
  TRACE_INFO_ARRAY("    ", message, length);

  // Check topic name
  if (!strcmp(topic, "board/leds/1"))
  {
  }
  else if (!strcmp(topic, "board/leds/2"))
  {
  }
}

/**
 * @brief Establish MQTT connection
 **/

error_t mqttTestConnect(void)
{
  error_t error;
  IpAddr ipAddr;

  // Debug message
  TRACE_INFO("\r\n\r\nResolving server name...\r\n");

  // Resolve MQTT server name
  error = getHostByName(NULL, APP_SERVER_NAME, &ipAddr, 0);
  // Any error to report?
  if (error)
    return error;

#if (APP_SERVER_PORT == 8080 || APP_SERVER_PORT == 8081)
  // Register RNG callback
  webSocketRegisterRandCallback(webSocketRngCallback);
#endif

  // Set the MQTT version to be used
  mqttClientSetVersion(&mqttClientContext, MQTT_VERSION_3_1_1);

#if (APP_SERVER_PORT == 1883)
  // MQTT over TCP
  mqttClientSetTransportProtocol(&mqttClientContext, MQTT_TRANSPORT_PROTOCOL_TCP);
#elif (APP_SERVER_PORT == 8883 || APP_SERVER_PORT == 8884)
  // MQTT over TLS
  mqttClientSetTransportProtocol(&mqttClientContext, MQTT_TRANSPORT_PROTOCOL_TLS);
  // Register TLS initialization callback
  mqttClientRegisterTlsInitCallback(&mqttClientContext, mqttTestTlsInitCallback);
#elif (APP_SERVER_PORT == 8080)
  // MQTT over WebSocket
  mqttClientSetTransportProtocol(&mqttClientContext, MQTT_TRANSPORT_PROTOCOL_WS);
#elif (APP_SERVER_PORT == 8081)
  // MQTT over secure WebSocket
  mqttClientSetTransportProtocol(&mqttClientContext, MQTT_TRANSPORT_PROTOCOL_WSS);
  // Register TLS initialization callback
  mqttClientRegisterTlsInitCallback(&mqttClientContext, mqttTestTlsInitCallback);
#endif

  // Register publish callback function
  mqttClientRegisterPublishCallback(&mqttClientContext, mqttPublishCallback);

  // Set communication timeout
  mqttClientSetTimeout(&mqttClientContext, 20000);
  // Set keep-alive value
  mqttClientSetKeepAlive(&mqttClientContext, 60);

#if (APP_SERVER_PORT == 8080 || APP_SERVER_PORT == 8081)
  // Set the hostname of the resource being requested
  mqttClientSetHost(&mqttClientContext, APP_SERVER_NAME);
  // Set the name of the resource being requested
  mqttClientSetUri(&mqttClientContext, APP_SERVER_URI);
#endif

  // Set client identifier
  mqttClientSetIdentifier(&mqttClientContext, "client12345678");

  // Set user name and password
  mqttClientSetAuthInfo(&mqttClientContext, "admin", "1713210041");

  // Set Will message
  mqttClientSetWillMessage(&mqttClientContext, "board/status",
                           "{\"esp32\": \"offline\"}", 20, MQTT_QOS_LEVEL_0, FALSE);

  // Debug message
  TRACE_INFO("Connecting to MQTT server %s...\r\n", ipAddrToString(&ipAddr, NULL));

  // Start of exception handling block
  do
  {
    // Establish connection with the MQTT server
    error = mqttClientConnect(&mqttClientContext,
                              &ipAddr, APP_SERVER_PORT, TRUE);
    // Any error to report?
    if (error)
      break;

    // Subscribe to the desired topics
    error = mqttClientSubscribe(&mqttClientContext,
                                "board/leds/", MQTT_QOS_LEVEL_1, NULL);
    // Any error to report?
    if (error)
      break;

    // Send PUBLISH packet
    error = mqttClientPublish(&mqttClientContext, "board/status",
                              "online", 6, MQTT_QOS_LEVEL_1, TRUE, NULL);
    // Any error to report?
    if (error)
      break;

    // End of exception handling block
  } while (0);

  // Check status code
  if (error)
  {
    // Close connection
    mqttClientClose(&mqttClientContext);
  }

  // Return status code
  return error;
}
void dhcpClientStateChangeCallback(DhcpClientContext *context,
                                   NetInterface *interface, DhcpState state)
{
  ESP_LOGI(TAG, "CALLBACK: %d", state);
  if (context == NULL || interface == NULL)
    return;

  if (context->state == DHCP_STATE_BOUND)
  {
    // mqtt connect
    xSemaphoreGive(dhcpFlag);
  }
  return;
}