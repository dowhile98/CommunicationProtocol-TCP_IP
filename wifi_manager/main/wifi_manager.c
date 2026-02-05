/**
 * @file wifi_manager.c
 * @brief Implementación del gestor WiFi con máquina de estados
 * @details
 * Gestiona la conexión WiFi en modo dual (AP+STA) con reconexión
 * automática y backoff exponencial.
 */

/* ========================================================================== */
/*                              INCLUSIONES                                   */
/* ========================================================================== */

#include "wifi_manager.h"
#include "include/wifi_config.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include <string.h>

/* ========================================================================== */
/*                          VARIABLES GLOBALES                                */
/* ========================================================================== */

/** Tag para logging */
static const char *TAG = "WiFiManager";

/* ========================================================================== */
/*                       PROTOTIPOS FUNCIONES PRIVADAS                        */
/* ========================================================================== */

static error_t wifi_manager_init_sta_interface(WifiManagerContext_t *context);
static error_t wifi_manager_init_ap_interface(WifiManagerContext_t *context);
static error_t wifi_manager_start_wifi(WifiManagerContext_t *context);
static void wifi_manager_copy_str(char *dest, uint32_t dest_len,
	const char *src);
static void wifi_manager_event_handler(void *arg, esp_event_base_t event_base,
	int32_t event_id, void *event_data);


/* ========================================================================== */
/*                       IMPLEMENTACIÓN FUNCIONES PÚBLICAS                    */
/* ========================================================================== */

error_t WifiManager_SetConfig(WifiManagerContext_t *context,
	const WifiManagerConfig_t *config)
{
	if (context == NULL || config == NULL)
	{
		return ERROR_INVALID_PARAMETER;
	}

	memset(&context->config, 0, sizeof(WifiManagerConfig_t));
	wifi_manager_copy_str(context->config.sta_ssid,
		WIFI_MANAGER_SSID_MAX_LEN, config->sta_ssid);
	wifi_manager_copy_str(context->config.sta_password,
		WIFI_MANAGER_PASSWORD_MAX_LEN, config->sta_password);
	context->config.sta_use_dhcp = config->sta_use_dhcp;
	context->config.sta_ipv4_addr = config->sta_ipv4_addr;
	context->config.sta_subnet_mask = config->sta_subnet_mask;
	context->config.sta_gateway = config->sta_gateway;
	context->config.sta_dns1 = config->sta_dns1;
	context->config.sta_dns2 = config->sta_dns2;

	wifi_manager_copy_str(context->config.ap_ssid,
		WIFI_MANAGER_SSID_MAX_LEN, config->ap_ssid);
	wifi_manager_copy_str(context->config.ap_password,
		WIFI_MANAGER_PASSWORD_MAX_LEN, config->ap_password);
	context->config.ap_use_dhcp_server = config->ap_use_dhcp_server;
	context->config.ap_max_connections = config->ap_max_connections;
	context->config.ap_ipv4_addr = config->ap_ipv4_addr;
	context->config.ap_subnet_mask = config->ap_subnet_mask;
	context->config.ap_gateway = config->ap_gateway;
	context->config.ap_dns1 = config->ap_dns1;
	context->config.ap_dns2 = config->ap_dns2;
	context->config.ap_dhcp_range_min = config->ap_dhcp_range_min;
	context->config.ap_dhcp_range_max = config->ap_dhcp_range_max;

	context->config_set = true;

	return NO_ERROR;
}

error_t WifiManager_Init(WifiManagerContext_t *context)
{
	error_t error;
	esp_err_t ret;

	if (context == NULL)
	{
		return ERROR_INVALID_PARAMETER;
	}

	if (!context->config_set)
	{
		return ERROR_INVALID_REQUEST;
	}

	ESP_LOGI(TAG, "Inicializando gestor WiFi...");

	ret = esp_event_loop_create_default();
	if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
	{
		ESP_LOGE(TAG, "Error creando event loop: %d", ret);
		return ERROR_FAILURE;
	}

	ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
		wifi_manager_event_handler, context);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Error registrando handler WiFi: %d", ret);
		return ERROR_FAILURE;
	}

	error = netInit();
	if (error)
	{
		ESP_LOGE(TAG, "Error inicializando CycloneTCP: %d", error);
		return error;
	}

	error = wifi_manager_init_sta_interface(context);
	if (error)
	{
		ESP_LOGE(TAG, "Error inicializando interfaz STA: %d", error);
		return error;
	}

	error = wifi_manager_init_ap_interface(context);
	if (error)
	{
		ESP_LOGE(TAG, "Error inicializando interfaz AP: %d", error);
		return error;
	}

	error = wifi_manager_start_wifi(context);
	if (error)
	{
		ESP_LOGE(TAG, "Error iniciando WiFi: %d", error);
		return error;
	}

	return NO_ERROR;
}


/* ========================================================================== */
/*                      IMPLEMENTACIÓN FUNCIONES PRIVADAS                     */
/* ========================================================================== */

static error_t wifi_manager_init_sta_interface(WifiManagerContext_t *context)
{
	error_t error;
	NetInterface *interface;
	MacAddr mac_addr;
	Ipv4Addr ipv4_addr;

	if (context == NULL)
	{
		return ERROR_INVALID_PARAMETER;
	}

	interface = &netInterface[0];
	context->interface_sta = interface;

	netSetInterfaceName(interface, APP_IF0_NAME);
	netSetHostname(interface, APP_IF0_HOST_NAME);
	macStringToAddr(APP_IF0_MAC_ADDR, &mac_addr);
	netSetMacAddr(interface, &mac_addr);
	netSetDriver(interface, &esp32WifiStaDriver);

	error = netConfigInterface(interface);
	if (error)
	{
		return error;
	}

	if (context->config.sta_use_dhcp)
	{
		dhcpClientGetDefaultSettings(&context->dhcp_client_settings);
		context->dhcp_client_settings.interface = interface;
		context->dhcp_client_settings.rapidCommit = FALSE;

		error = dhcpClientInit(&context->dhcp_client_ctx,
			&context->dhcp_client_settings);
		if (error)
		{
			return error;
		}

		error = dhcpClientStart(&context->dhcp_client_ctx);
		if (error)
		{
			return error;
		}
	}
	else
	{
		ipv4_addr = context->config.sta_ipv4_addr;
		ipv4SetHostAddr(interface, ipv4_addr);

		ipv4_addr = context->config.sta_subnet_mask;
		ipv4SetSubnetMask(interface, ipv4_addr);

		ipv4_addr = context->config.sta_gateway;
		ipv4SetDefaultGateway(interface, ipv4_addr);

		ipv4_addr = context->config.sta_dns1;
		ipv4SetDnsServer(interface, 0, ipv4_addr);
		ipv4_addr = context->config.sta_dns2;
		ipv4SetDnsServer(interface, 1, ipv4_addr);
	}

	return NO_ERROR;
}

static error_t wifi_manager_init_ap_interface(WifiManagerContext_t *context)
{
	error_t error;
	NetInterface *interface;
	MacAddr mac_addr;
	Ipv4Addr ipv4_addr;

	if (context == NULL)
	{
		return ERROR_INVALID_PARAMETER;
	}

	interface = &netInterface[1];
	context->interface_ap = interface;

	netSetInterfaceName(interface, APP_IF1_NAME);
	netSetHostname(interface, APP_IF1_HOST_NAME);
	macStringToAddr(APP_IF1_MAC_ADDR, &mac_addr);
	netSetMacAddr(interface, &mac_addr);
	netSetDriver(interface, &esp32WifiApDriver);

	error = netConfigInterface(interface);
	if (error)
	{
		return error;
	}

	ipv4_addr = context->config.ap_ipv4_addr;
	ipv4SetHostAddr(interface, ipv4_addr);

	ipv4_addr = context->config.ap_subnet_mask;
	ipv4SetSubnetMask(interface, ipv4_addr);

	ipv4_addr = context->config.ap_gateway;
	ipv4SetDefaultGateway(interface, ipv4_addr);

	ipv4_addr = context->config.ap_dns1;
	ipv4SetDnsServer(interface, 0, ipv4_addr);
	ipv4_addr = context->config.ap_dns2;
	ipv4SetDnsServer(interface, 1, ipv4_addr);

	if (context->config.ap_use_dhcp_server)
	{
		dhcpServerGetDefaultSettings(&context->dhcp_server_settings);
		context->dhcp_server_settings.interface = interface;
		context->dhcp_server_settings.leaseTime = 3600U;
		context->dhcp_server_settings.ipAddrRangeMin =
			context->config.ap_dhcp_range_min;
		context->dhcp_server_settings.ipAddrRangeMax =
			context->config.ap_dhcp_range_max;
		context->dhcp_server_settings.subnetMask =
			context->config.ap_subnet_mask;
		context->dhcp_server_settings.defaultGateway =
			context->config.ap_gateway;
		context->dhcp_server_settings.dnsServer[0] = context->config.ap_dns1;
		context->dhcp_server_settings.dnsServer[1] = context->config.ap_dns2;

		error = dhcpServerInit(&context->dhcp_server_ctx,
			&context->dhcp_server_settings);
		if (error)
		{
			return error;
		}

		error = dhcpServerStart(&context->dhcp_server_ctx);
		if (error)
		{
			return error;
		}
	}

	return NO_ERROR;
}

static error_t wifi_manager_start_wifi(WifiManagerContext_t *context)
{
	esp_err_t ret;
	wifi_config_t sta_config;
	wifi_config_t ap_config;
	uint32_t pass_len;

	if (context == NULL)
	{
		return ERROR_INVALID_PARAMETER;
	}

	memset(&sta_config, 0, sizeof(wifi_config_t));
	memset(&ap_config, 0, sizeof(wifi_config_t));

	wifi_manager_copy_str((char *)sta_config.sta.ssid,
		sizeof(sta_config.sta.ssid), context->config.sta_ssid);
	wifi_manager_copy_str((char *)sta_config.sta.password,
		sizeof(sta_config.sta.password), context->config.sta_password);

	pass_len = (uint32_t)strlen(context->config.sta_password);
	if (pass_len == 0U)
	{
		sta_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
	}
	else
	{
		sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
	}

	wifi_manager_copy_str((char *)ap_config.ap.ssid,
		sizeof(ap_config.ap.ssid), context->config.ap_ssid);
	wifi_manager_copy_str((char *)ap_config.ap.password,
		sizeof(ap_config.ap.password), context->config.ap_password);
	ap_config.ap.max_connection = context->config.ap_max_connections;

	pass_len = (uint32_t)strlen(context->config.ap_password);
	if (pass_len == 0U)
	{
		ap_config.ap.authmode = WIFI_AUTH_OPEN;
	}
	else
	{
		ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
	}

	ret = esp_wifi_set_mode(WIFI_MODE_APSTA);
	if (ret != ESP_OK)
	{
		return ERROR_FAILURE;
	}

	ret = esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config);
	if (ret != ESP_OK)
	{
		return ERROR_FAILURE;
	}

	ret = esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config);
	if (ret != ESP_OK)
	{
		return ERROR_FAILURE;
	}

	ret = esp_wifi_start();
	if (ret != ESP_OK)
	{
		return ERROR_FAILURE;
	}

	ret = esp_wifi_connect();
	if (ret != ESP_OK)
	{
		return ERROR_FAILURE;
	}

	return NO_ERROR;
}

static void wifi_manager_copy_str(char *dest, uint32_t dest_len,
	const char *src)
{
	uint32_t len;

	if (dest == NULL || src == NULL || dest_len == 0U)
	{
		return;
	}

	len = (uint32_t)strnlen(src, (size_t)(dest_len - 1U));
	memcpy(dest, src, (size_t)len);
	dest[len] = '\0';
}

static void wifi_manager_event_handler(void *arg, esp_event_base_t event_base,
	int32_t event_id, void *event_data)
{
	WifiManagerContext_t *context = (WifiManagerContext_t *)arg;
	(void)event_base;
	(void)event_data;

	if (context == NULL)
	{
		return;
	}

	if (event_id == WIFI_EVENT_STA_START)
	{
		ESP_LOGI(TAG, "WiFi STA iniciado");
		esp_wifi_connect();
	}
	else if (event_id == WIFI_EVENT_STA_CONNECTED)
	{
		ESP_LOGI(TAG, "WiFi STA conectado");
	}
	else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
	{
		ESP_LOGW(TAG, "WiFi STA desconectado, reintentando...");
		esp_wifi_connect();
	}
	else if (event_id == WIFI_EVENT_AP_START)
	{
		ESP_LOGI(TAG, "WiFi AP iniciado");
	}
	else if (event_id == WIFI_EVENT_AP_STACONNECTED)
	{
		ESP_LOGI(TAG, "Cliente conectado al AP");
	}
	else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
	{
		ESP_LOGI(TAG, "Cliente desconectado del AP");
	}
}

