/**
 * @file wifi_manager.c
 * @brief Implementación del gestor WiFi con máquina de estados
 */

/* ========================================================================== */
/*                              INCLUSIONES                                   */
/* ========================================================================== */

#include "wifi_manager.h"
#include "include/wifi_config.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "lwip/netif.h" // Para netif_set_addr
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
// static error_t wifi_manager_start_wifi(WifiManagerContext_t *context); // Eliminar si no se usa o mover
static void wifi_manager_copy_str(char *dest, uint32_t dest_len,
								  const char *src);
static void wifi_manager_event_handler(void *arg, esp_event_base_t event_base,
									   int32_t event_id, void *event_data);
// static wifi_mode_t esp_wifi_mode_to_wifi_mode_t(wifi_mode_t esp_mode); // Eliminar
static esp_err_t wifi_manager_apply_sta_config(WifiManagerContext_t *context);
static esp_err_t wifi_manager_apply_ap_config(WifiManagerContext_t *context);

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
	context->config.current_mode = config->current_mode; // Guardar el modo de operación

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

	// Inicializar interfaces CycloneTCP, pero no las activar aún
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

	// Establecer el modo de operación inicial
	return WifiManager_SetOperatingMode(context, context->config.current_mode);
}

error_t WifiManager_SetOperatingMode(WifiManagerContext_t *context, wm_wifi_mode_t mode)
{
	esp_err_t ret;
	wifi_mode_t esp_mode; // Usamos el enum de ESP-IDF aquí para la función esp_wifi_set_mode

	if (context == NULL)
	{
		return ERROR_INVALID_PARAMETER;
	}

	ESP_LOGI(TAG, "Estableciendo modo WiFi a: %d", mode);

	switch (mode)
	{
	case WM_WIFI_MODE_OFF:
		esp_mode = WIFI_MODE_NULL;
		break;
	case WM_WIFI_MODE_STA:
		esp_mode = WIFI_MODE_STA;
		break;
	case WM_WIFI_MODE_AP:
		esp_mode = WIFI_MODE_AP;
		break;
	case WM_WIFI_MODE_APSTA:
		esp_mode = WIFI_MODE_APSTA;
		break;
	default:
		return ERROR_INVALID_PARAMETER;
	}

	// Detener WiFi si está activo
	esp_wifi_stop();

	// Establecer el modo
	ret = esp_wifi_set_mode(esp_mode);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Error esp_wifi_set_mode: %d", ret);
		return ERROR_FAILURE;
	}

	// Aplicar configuraciones de interfaz según el modo
	// Solo aplicar si el modo no es OFF
	if (mode == WM_WIFI_MODE_STA || mode == WM_WIFI_MODE_APSTA)
	{
		wifi_manager_apply_sta_config(context);
	}
	if (mode == WM_WIFI_MODE_AP || mode == WM_WIFI_MODE_APSTA)
	{
		wifi_manager_apply_ap_config(context);
	}

	// Iniciar WiFi
	ret = esp_wifi_start();
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Error esp_wifi_start: %d", ret);
		return ERROR_FAILURE;
	}

	if (mode == WM_WIFI_MODE_STA || mode == WM_WIFI_MODE_APSTA)
	{
		esp_wifi_connect();
	}

	context->config.current_mode = mode; // Actualizar el modo en la configuración
	return NO_ERROR;
}

error_t WifiManager_ScanNetworks(WifiManagerContext_t *context)
{
	esp_err_t ret;
	uint16_t ap_count = 0;
	uint16_t ap_num = MAX_SCANNED_NETWORKS;
	wifi_ap_record_t ap_records[MAX_SCANNED_NETWORKS];

	if (context == NULL)
	{
		return ERROR_INVALID_PARAMETER;
	}

	ESP_LOGI(TAG, "Iniciando escaneo de redes WiFi...");
	context->scanned_networks_count = 0;

	// Verificar modo WiFi actual
	wifi_mode_t current_mode;
	ret = esp_wifi_get_mode(&current_mode);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Error obteniendo modo WiFi: 0x%x", ret);
		return ERROR_FAILURE;
	}
	ESP_LOGI(TAG, "Modo WiFi actual: %d", current_mode);

	// El escaneo solo funciona en modo STA o APSTA
	if (current_mode == WIFI_MODE_NULL || current_mode == WIFI_MODE_AP)
	{
		ESP_LOGW(TAG, "WiFi debe estar en modo STA o APSTA para escanear. Modo actual: %d", current_mode);
		return ERROR_INVALID_REQUEST;
	}

	// Configurar escaneo
	wifi_scan_config_t scan_config = {
		.ssid = NULL,
		.bssid = NULL,
		.channel = 0,
		.show_hidden = false,
		.scan_type = WIFI_SCAN_TYPE_ACTIVE,
		.scan_time.active.min = 100,
		.scan_time.active.max = 300};

	// Iniciar escaneo síncrono (true = bloquea hasta completar)
	ret = esp_wifi_scan_start(&scan_config, true);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Error al iniciar escaneo: 0x%x (%s)", ret, esp_err_to_name(ret));
		return ERROR_FAILURE;
	}

	// Primero obtener número de APs encontrados
	ret = esp_wifi_scan_get_ap_num(&ap_count);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Error obteniendo número de APs: 0x%x", ret);
		return ERROR_FAILURE;
	}

	ESP_LOGI(TAG, "Encontrados %d APs", ap_count);

	if (ap_count == 0)
	{
		ESP_LOGW(TAG, "No se encontraron redes WiFi");
		context->scanned_networks_count = 0;
		return NO_ERROR;
	}

	// Limitar al máximo de redes
	if (ap_count > MAX_SCANNED_NETWORKS)
	{
		ap_count = MAX_SCANNED_NETWORKS;
	}

	// Obtener registros de APs
	ap_num = ap_count;
	ret = esp_wifi_scan_get_ap_records(&ap_num, ap_records);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Error al obtener AP records: 0x%x (%s)", ret, esp_err_to_name(ret));
		return ERROR_FAILURE;
	}

	ESP_LOGI(TAG, "Obtenidos %d AP records", ap_num);

	// Copiar datos al contexto
	for (uint16_t i = 0; i < ap_num; i++)
	{
		wifi_manager_copy_str(context->scanned_networks[i].ssid,
							  WIFI_MANAGER_SSID_MAX_LEN,
							  (const char *)ap_records[i].ssid);
		context->scanned_networks[i].rssi = ap_records[i].rssi;
		context->scanned_networks[i].authmode = ap_records[i].authmode;
		ESP_LOGI(TAG, "[%d] SSID: %s, RSSI: %d, Auth: %d",
				 i,
				 context->scanned_networks[i].ssid,
				 context->scanned_networks[i].rssi,
				 context->scanned_networks[i].authmode);
	}

	context->scanned_networks_count = ap_num;
	ESP_LOGI(TAG, "Escaneo completado: %d redes encontradas", context->scanned_networks_count);

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

	// Inicializar cliente DHCP para la interfaz STA
	dhcpClientGetDefaultSettings(&context->dhcp_client_settings);
	context->dhcp_client_settings.interface = interface;
	context->dhcp_client_settings.ipAddrIndex = 0;

	error = dhcpClientInit(&context->dhcp_client_ctx, &context->dhcp_client_settings);
	if (error)
	{
		ESP_LOGE(TAG, "Error inicializando cliente DHCP: %d", error);
		return error;
	}

	return NO_ERROR;
}

static error_t wifi_manager_init_ap_interface(WifiManagerContext_t *context)
{
	error_t error;
	NetInterface *interface;
	MacAddr mac_addr;

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

	return NO_ERROR;
}

static esp_err_t wifi_manager_apply_sta_config(WifiManagerContext_t *context)
{
	wifi_config_t sta_config;
	memset(&sta_config, 0, sizeof(wifi_config_t));

	wifi_manager_copy_str((char *)sta_config.sta.ssid, sizeof(sta_config.sta.ssid), context->config.sta_ssid);
	wifi_manager_copy_str((char *)sta_config.sta.password, sizeof(sta_config.sta.password), context->config.sta_password);

	if (strlen(context->config.sta_password) == 0)
	{
		sta_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
	}
	else
	{
		sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK; // Por defecto
	}

	// Aplicar configuración de IP estática o DHCP
	if (!context->config.sta_use_dhcp)
	{
		NetInterface *interface = context->interface_sta;
		// Detener cliente DHCP si estaba corriendo
		if (context->dhcp_client_ctx.running)
		{
			dhcpClientStop(&context->dhcp_client_ctx);
		}
		// Configurar IP estática
		ipv4SetHostAddr(interface, context->config.sta_ipv4_addr);
		ipv4SetSubnetMask(interface, context->config.sta_subnet_mask);
		ipv4SetDefaultGateway(interface, context->config.sta_gateway);
		ipv4SetDnsServer(interface, 0, context->config.sta_dns1);
		ipv4SetDnsServer(interface, 1, context->config.sta_dns2);
	}
	else
	{
		// Iniciar cliente DHCP si no está corriendo
		if (!context->dhcp_client_ctx.running)
		{
			error_t error = dhcpClientStart(&context->dhcp_client_ctx);
			if (error)
			{
				ESP_LOGE(TAG, "Error iniciando cliente DHCP: %d", error);
			}
		}
	}

	return esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config);
}

static esp_err_t wifi_manager_apply_ap_config(WifiManagerContext_t *context)
{
	wifi_config_t ap_config;
	memset(&ap_config, 0, sizeof(wifi_config_t));

	wifi_manager_copy_str((char *)ap_config.ap.ssid, sizeof(ap_config.ap.ssid), context->config.ap_ssid);
	wifi_manager_copy_str((char *)ap_config.ap.password, sizeof(ap_config.ap.password), context->config.ap_password);
	ap_config.ap.max_connection = context->config.ap_max_connections;

	if (strlen(context->config.ap_password) == 0)
	{
		ap_config.ap.authmode = WIFI_AUTH_OPEN;
	}
	else
	{
		ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK; // Por defecto
	}

	// Configuración de IP estática para AP
	NetInterface *interface = context->interface_ap;
	ipv4SetHostAddr(interface, context->config.ap_ipv4_addr);
	ipv4SetSubnetMask(interface, context->config.ap_subnet_mask);
	ipv4SetDefaultGateway(interface, context->config.ap_gateway);
	ipv4SetDnsServer(interface, 0, context->config.ap_dns1);
	ipv4SetDnsServer(interface, 1, context->config.ap_dns2);

	// Iniciar/Detener DHCP Server
	if (context->config.ap_use_dhcp_server)
	{
		dhcpServerStart(&context->dhcp_server_ctx);
	}
	else
	{
		dhcpServerStop(&context->dhcp_server_ctx);
	}

	return esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config);
}

// TODO: Función reservada para uso futuro - actualmente la lógica está en WifiManager_Init
#if 0
static error_t wifi_manager_start_wifi(WifiManagerContext_t *context)
{
    esp_err_t ret;
    wifi_mode_t current_esp_mode; // Usar wifi_mode_t de ESP-IDF
    
    // Obtener el modo actual configurado en el ESP-IDF
    esp_wifi_get_mode(&current_esp_mode);

    if (context == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (current_esp_mode == WIFI_MODE_STA || current_esp_mode == WIFI_MODE_APSTA)
    {
        ret = wifi_manager_apply_sta_config(context);
        if (ret != ESP_OK) return ERROR_FAILURE;
    }

    if (current_esp_mode == WIFI_MODE_AP || current_esp_mode == WIFI_MODE_APSTA)
    {
        ret = wifi_manager_apply_ap_config(context);
        if (ret != ESP_OK) return ERROR_FAILURE;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK)
    {
        return ERROR_FAILURE;
    }

    if (current_esp_mode == WIFI_MODE_STA || current_esp_mode == WIFI_MODE_APSTA)
    {
        esp_wifi_connect();
    }

    return NO_ERROR;
}
#endif

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
