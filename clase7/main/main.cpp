/**
 * @file main.cpp
 * @brief Aplicación WiFi con CycloneTCP - WiFi Manager
 */

/* ========================================================================== */
/*                              INCLUSIONES                                   */
/* ========================================================================== */

#include "ArduinoJson.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include <string.h>

extern "C"
{
#include "core/net.h" // Para ipv4AddrToString y tipos de red (MacAddr, Eui64, etc.)
#include "debug.h"
#include "coap/coap_server.h"
#include "coap/coap_server_request.h"
#include "include/wifi_config.h"
#include "path.h"
#include "resource_manager.h"
#include "wifi_manager.h"
}

/* ========================================================================== */
/*                               CONSTANTES                                   */
/* ========================================================================== */
static const char *TAG = "Main";

#define APP_COAP_SERVER_PORT 5683

/* ========================================================================== */
/*                          VARIABLES GLOBALES                                */
/* ========================================================================== */
CoapServerSettings coapServerSettings;
CoapServerContext coapServerContext;
WifiManagerContext_t wifi_context;
SemaphoreHandle_t dhcpFlag;

/* Estado mutable de recursos CoAP */
static bool s_led_on = false;  /**< Estado del LED simulado */
static uint32_t s_counter = 0; /**< Contador accesible vía CoAP */
/* ========================================================================== */
/*                      PROTOTIPOS DE FUNCIONES                               */
/* ========================================================================== */
error_t coapServerRequestCallback(CoapServerContext *context,
                                  CoapCode method, const char_t *uri);

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

  /*wait for flag*/
  xSemaphoreTake(dhcpFlag, portMAX_DELAY);

  ESP_LOGI(TAG, "Starting CoAP server...");
  // Get default settings
  coapServerGetDefaultSettings(&coapServerSettings);
  coapServerSettings.task.stackSize = 1024*8;
  // Bind CoAP server to the desired interface
  coapServerSettings.interface = &netInterface[0];
  // Listen to port
  coapServerSettings.port = APP_COAP_SERVER_PORT;
  // Callback functions
  coapServerSettings.requestCallback = coapServerRequestCallback;

  // CoAP server initialization
  error = coapServerInit(&coapServerContext, &coapServerSettings);
  // Failed to initialize CoAP server?
  if (error)
  {
    // Debug message
    ESP_LOGE(TAG, "Failed to initialize CoAP server!\r\n");
  }

  // Start CoAP server
  error = coapServerStart(&coapServerContext);
  // Failed to start CoAP server?
  if (error)
  {
    // Debug message
    ESP_LOGE(TAG, "Failed to start CoAP server!\r\n");
  }

  ESP_LOGI(TAG, "CoAP server started on port %d", APP_COAP_SERVER_PORT);

  while (1)
  {
    osDelayTask(5000);
  }
}

/* ========================================================================== */
/*              HELPERS INTERNOS DEL CALLBACK CoAP                           */
/* ========================================================================== */

/**
 * @brief  Envía una respuesta JSON al cliente CoAP.
 * @param[in] context       Contexto CoAP.
 * @param[in] code          Código de respuesta CoAP (ej. COAP_CODE_CONTENT).
 * @param[in] json_payload  Buffer con el JSON serializado (null-terminated).
 * @return error_t
 */
static error_t send_json_response(CoapServerContext *context,
                                  CoapCode code,
                                  const char *json_payload)
{
  error_t err;
  err = coapServerSetResponseCode(context, code);
  if (err)
    return err;
  err = coapServerSetUintOption(context, COAP_OPT_CONTENT_FORMAT, 0,
                                COAP_CONTENT_FORMAT_APP_JSON);
  if (err)
    return err;
  return coapServerSetPayload(context, json_payload, strlen(json_payload));
}

/* ========================================================================== */
/*                      IMPLEMENTACIÓN DE FUNCIONES                           */
/* ========================================================================== */
/**
 * @brief CoAP request callback — maneja todos los recursos del servidor.
 *
 * Recursos disponibles:
 *   GET  /.well-known/core  → Descubrimiento de recursos (RFC 6690)
 *   GET  /test              → Texto plano "Hello World!"
 *   GET  /info              → JSON con uptime, heap libre y chip_id
 *   GET  /led               → JSON estado actual del LED simulado
 *   PUT  /led               → Modifica estado del LED (body JSON: {"state":"on"})
 *   GET  /counter           → JSON con valor del contador (auto-incrementa)
 *   POST /counter/reset     → Reinicia el contador a 0
 *   DELETE /counter         → Reinicia el contador a 0 (alias)
 *   POST /echo              → Repite el payload recibido
 *
 * @param[in] context  Contexto del servidor CoAP.
 * @param[in] method   Código de método CoAP.
 * @param[in] uri      Ruta del recurso solicitado.
 * @return error_t
 **/
error_t coapServerRequestCallback(CoapServerContext *context,
                                  CoapCode method, const char_t *uri)
{
  error_t error = NO_ERROR;
  /* Buffer de trabajo para respuestas JSON (sin alloc dinámico) */
  static char s_json_buf[256];

  /* ------------------------------------------------------------------ */
  /*  GET                                                                */
  /* ------------------------------------------------------------------ */
  if (method == COAP_CODE_GET)
  {
    /* --- /.well-known/core : descubrimiento de recursos --- */
    if (!strcasecmp(uri, "/.well-known/core"))
    {
      const char_t core[] =
          "</test>;rt=\"demo\";title=\"Hello World\","
          "</info>;rt=\"info\";title=\"Device info (JSON)\","
          "</led>;rt=\"actuator\";title=\"LED simulado (GET/PUT)\","
          "</counter>;rt=\"sensor\";title=\"Contador (GET/DELETE)\","
          "</counter/reset>;rt=\"control\";title=\"Reinicia contador (POST)\","
          "</echo>;rt=\"debug\";title=\"Echo payload (POST)\"";

      error = coapServerSetResponseCode(context, COAP_CODE_CONTENT);
      if (!error)
        error = coapServerSetUintOption(context, COAP_OPT_CONTENT_FORMAT,
                                        0, COAP_CONTENT_FORMAT_APP_LINK_FORMAT);
      if (!error)
        error = coapServerSetPayload(context, core, strlen(core));
    }

    /* --- /test : texto plano --- */
    else if (!strcasecmp(uri, "/test"))
    {
      const char_t msg[] = "Hello World! CoAP server running on ESP32.";
      error = coapServerSetResponseCode(context, COAP_CODE_CONTENT);
      if (!error)
        error = coapServerSetUintOption(context, COAP_OPT_CONTENT_FORMAT,
                                        0, COAP_CONTENT_FORMAT_TEXT_PLAIN);
      if (!error)
        error = coapServerSetPayload(context, msg, strlen(msg));
    }

    /* --- /info : información del dispositivo --- */
    else if (!strcasecmp(uri, "/info"))
    {
      uint64_t uptime_us = (uint64_t)esp_timer_get_time();
      uint32_t uptime_s = (uint32_t)(uptime_us / 1000000ULL);
      uint32_t free_heap = esp_get_free_heap_size();

      JsonDocument doc;
      doc["uptime_s"] = uptime_s;
      doc["free_heap"] = free_heap;
      doc["chip"] = "ESP32";
      serializeJson(doc, s_json_buf, sizeof(s_json_buf));

      error = send_json_response(context, COAP_CODE_CONTENT, s_json_buf);
    }

    /* --- /led : leer estado del LED simulado --- */
    else if (!strcasecmp(uri, "/led"))
    {
      JsonDocument doc;
      doc["led"] = s_led_on ? "on" : "off";
      doc["gpio"] = 2; /* GPIO del LED built-in en muchas placas ESP32 */
      serializeJson(doc, s_json_buf, sizeof(s_json_buf));

      error = send_json_response(context, COAP_CODE_CONTENT, s_json_buf);
    }

    /* --- /counter : leer (e incrementar) el contador --- */
    else if (!strcasecmp(uri, "/counter"))
    {
      s_counter++;

      JsonDocument doc;
      doc["count"] = s_counter;
      serializeJson(doc, s_json_buf, sizeof(s_json_buf));

      error = send_json_response(context, COAP_CODE_CONTENT, s_json_buf);
    }

    else
    {
      error = coapServerSetResponseCode(context, COAP_CODE_NOT_FOUND);
    }
  }

  /* ------------------------------------------------------------------ */
  /*  PUT                                                                */
  /* ------------------------------------------------------------------ */
  else if (method == COAP_CODE_PUT)
  {
    /* --- PUT /led : cambiar estado del LED --- */
    if (!strcasecmp(uri, "/led"))
    {
      const uint8_t *p;
      size_t n;
      error = coapServerGetPayload(context, &p, &n);

      if (!error && n > 0 && n < sizeof(s_json_buf))
      {
        memcpy(s_json_buf, p, n);
        s_json_buf[n] = '\0';

        JsonDocument req;
        DeserializationError de_err = deserializeJson(req, s_json_buf);

        if (de_err == DeserializationError::Ok &&
            req["state"].is<const char *>())
        {
          const char *state_str = req["state"];
          bool changed = false;

          if (!strcasecmp(state_str, "on") && !s_led_on)
          {
            s_led_on = true;
            changed = true;
            ESP_LOGI(TAG, "CoAP PUT /led → ON");
          }
          else if (!strcasecmp(state_str, "off") && s_led_on)
          {
            s_led_on = false;
            changed = true;
            ESP_LOGI(TAG, "CoAP PUT /led → OFF");
          }

          JsonDocument resp;
          resp["led"] = s_led_on ? "on" : "off";
          resp["changed"] = changed;
          serializeJson(resp, s_json_buf, sizeof(s_json_buf));

          error = send_json_response(context, COAP_CODE_CHANGED, s_json_buf);
        }
        else
        {
          /* JSON malformado o campo ausente */
          const char bad[] = "{\"error\":\"campo state requerido: on|off\"}";
          error = send_json_response(context, COAP_CODE_BAD_REQUEST, bad);
        }
      }
      else
      {
        const char bad[] = "{\"error\":\"payload vacio o demasiado grande\"}";
        error = send_json_response(context, COAP_CODE_BAD_REQUEST, bad);
      }
    }
    else
    {
      error = coapServerSetResponseCode(context, COAP_CODE_NOT_FOUND);
    }
  }

  /* ------------------------------------------------------------------ */
  /*  POST                                                               */
  /* ------------------------------------------------------------------ */
  else if (method == COAP_CODE_POST)
  {
    /* --- POST /echo : devuelve el mismo payload --- */
    if (!strcasecmp(uri, "/echo"))
    {
      const uint8_t *p;
      size_t n;
      uint32_t contentFormat;
      bool_t contentFormatFound;

      error = coapServerGetUintOption(context, COAP_OPT_CONTENT_FORMAT,
                                      0, &contentFormat);
      contentFormatFound = (error == NO_ERROR) ? TRUE : FALSE;

      error = coapServerGetPayload(context, &p, &n);
      if (!error)
        error = coapServerSetResponseCode(context, COAP_CODE_CHANGED);
      if (!error && contentFormatFound)
        error = coapServerSetUintOption(context, COAP_OPT_CONTENT_FORMAT,
                                        0, contentFormat);
      if (!error)
        error = coapServerSetPayload(context, p, n);
    }

    /* --- POST /counter/reset : reinicia el contador --- */
    else if (!strcasecmp(uri, "/counter/reset"))
    {
      s_counter = 0;
      ESP_LOGI(TAG, "CoAP POST /counter/reset → contador reiniciado");

      const char resp[] = "{\"count\":0,\"reset\":true}";
      error = send_json_response(context, COAP_CODE_CHANGED, resp);
    }

    else
    {
      error = coapServerSetResponseCode(context, COAP_CODE_NOT_FOUND);
    }
  }

  /* ------------------------------------------------------------------ */
  /*  DELETE                                                             */
  /* ------------------------------------------------------------------ */
  else if (method == COAP_CODE_DELETE)
  {
    /* --- DELETE /counter : reinicia el contador (semántica de borrado) --- */
    if (!strcasecmp(uri, "/counter"))
    {
      s_counter = 0;
      ESP_LOGI(TAG, "CoAP DELETE /counter → contador reiniciado");

      const char resp[] = "{\"count\":0,\"reset\":true}";
      error = send_json_response(context, COAP_CODE_DELETED, resp);
    }
    else
    {
      error = coapServerSetResponseCode(context, COAP_CODE_NOT_FOUND);
    }
  }

  else
  {
    error = coapServerSetResponseCode(context, COAP_CODE_METHOD_NOT_ALLOWED);
  }

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