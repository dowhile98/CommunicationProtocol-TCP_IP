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
#include "drivers/wifi/esp32_wifi_driver.h"
#include "ipv4/ipv4.h"
#include "ipv6/ipv6.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include <string.h>

/* ========================================================================== */
/*                          VARIABLES GLOBALES                                */
/* ========================================================================== */

/** Tag para logging */
static const char *TAG = "WiFiManager";

/** Contexto global del gestor WiFi */
static WifiManagerContext_t g_wifi_ctx;

/* ========================================================================== */
/*                       PROTOTIPOS FUNCIONES PRIVADAS                        */
/* ========================================================================== */

static error_t init_interface_sta(void);
static error_t init_interface_ap(void);
static esp_err_t start_wifi_ap(void);
static esp_err_t start_wifi_sta(void);
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);
static void change_wifi_state(WifiEstado_t new_state);
static void calculate_backoff(void);
static void reset_backoff(void);
static const char *disconnect_reason_to_string(uint8_t reason);

/* ========================================================================== */
/*                       IMPLEMENTACIÓN FUNCIONES PÚBLICAS                    */
/* ========================================================================== */

/**
 * @brief Inicializa el gestor WiFi y la pila TCP/IP
 */
error_t wifi_manager_init(void)
{
    error_t error;

    ESP_LOGI(TAG, "=================================================");
    ESP_LOGI(TAG, "  Inicializando Gestor WiFi Dual (AP+STA)");
    ESP_LOGI(TAG, "=================================================");

    /* Inicializar contexto */
    memset(&g_wifi_ctx, 0, sizeof(WifiManagerContext_t));
    g_wifi_ctx.estado = WIFI_DESCONECTADO;
    g_wifi_ctx.delay_backoff_actual_ms = WIFI_RECONNECT_DELAY_INITIAL_MS;

    /* Inicializar pila TCP/IP (CycloneTCP) */
    ESP_LOGI(TAG, "Inicializando pila TCP/IP...");
    error = netInit();
    if (error != NO_ERROR)
    {
        ESP_LOGE(TAG, "Error al inicializar pila TCP/IP: %d", error);
        return error;
    }
    ESP_LOGI(TAG, "Pila TCP/IP inicializada correctamente");

    /* Crear event loop de ESP-IDF */
    ESP_LOGI(TAG, "Creando event loop ESP-IDF...");
    esp_err_t ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "Error al crear event loop: %d", ret);
        return ERROR_FAILURE;
    }

    /* Registrar manejador de eventos WiFi */
    ESP_LOGI(TAG, "Registrando manejador de eventos WiFi...");
    ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                     wifi_event_handler, NULL);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error al registrar manejador de eventos: %d", ret);
        return ERROR_FAILURE;
    }

    /* Inicializar interfaz STA (wlan0) */
    ESP_LOGI(TAG, "Inicializando interfaz STA (wlan0)...");
    error = init_interface_sta();
    if (error != NO_ERROR)
    {
        ESP_LOGE(TAG, "Error al inicializar interfaz STA: %d", error);
        return error;
    }
    ESP_LOGI(TAG, "Interfaz STA configurada: %s", APP_IF0_NAME);

    /* Inicializar interfaz AP (wlan1) */
    ESP_LOGI(TAG, "Inicializando interfaz AP (wlan1)...");
    error = init_interface_ap();
    if (error != NO_ERROR)
    {
        ESP_LOGE(TAG, "Error al inicializar interfaz AP: %d", error);
        return error;
    }
    ESP_LOGI(TAG, "Interfaz AP configurada: %s", APP_IF1_NAME);

    ESP_LOGI(TAG, "Gestor WiFi inicializado correctamente");
    ESP_LOGI(TAG, "=================================================");

    return NO_ERROR;
}

/**
 * @brief Inicia la conexión WiFi en modo dual
 */
error_t wifi_manager_start(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Iniciando WiFi en modo dual (AP+STA)...");

    /* Iniciar AP primero (siguiendo patrón del main.c de referencia) */
    ESP_LOGI(TAG, "Iniciando modo AP...");
    ret = start_wifi_ap();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error al iniciar WiFi AP: %d", ret);
        return ERROR_FAILURE;
    }

    /* Iniciar STA después (siguiendo patrón del main.c de referencia) */
    ESP_LOGI(TAG, "Iniciando modo STA...");
    ret = start_wifi_sta();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error al iniciar WiFi STA: %d", ret);
        return ERROR_FAILURE;
    }

    change_wifi_state(WIFI_CONECTANDO);

    ESP_LOGI(TAG, "WiFi dual iniciado correctamente");
    return NO_ERROR;
}

/**
 * @brief Obtiene el estado actual
 */
WifiEstado_t wifi_manager_get_estado(void)
{
    return g_wifi_ctx.estado;
}

/**
 * @brief Obtiene intentos de reconexión
 */
uint32_t wifi_manager_get_intentos_reconexion(void)
{
    return g_wifi_ctx.intentos_reconexion;
}

/**
 * @brief Obtiene delay de backoff
 */
uint32_t wifi_manager_get_delay_backoff(void)
{
    return g_wifi_ctx.delay_backoff_actual_ms;
}

/**
 * @brief Verifica si DHCP obtuvo IP
 */
bool wifi_manager_dhcp_obtenido(void)
{
    return g_wifi_ctx.dhcp_obtenido;
}

/**
 * @brief Obtiene número de clientes en AP
 */
uint8_t wifi_manager_get_clientes_ap(void)
{
    return g_wifi_ctx.clientes_ap_conectados;
}

/**
 * @brief Convierte estado a string
 */
const char *wifi_manager_estado_to_string(WifiEstado_t estado)
{
    switch (estado)
    {
    case WIFI_DESCONECTADO:
        return "DESCONECTADO";
    case WIFI_CONECTANDO:
        return "CONECTANDO";
    case WIFI_CONECTADO:
        return "CONECTADO";
    case WIFI_ERROR:
        return "ERROR";
    case WIFI_BACKOFF:
        return "ESPERANDO_BACKOFF";
    default:
        return "DESCONOCIDO";
    }
}

/**
 * @brief Tarea principal del gestor WiFi
 */
void wifi_manager_task(void *params)
{
    ESP_LOGI(TAG, "Tarea del gestor WiFi iniciada");

    while (1)
    {
        /* Procesamiento según el estado actual */
        switch (g_wifi_ctx.estado)
        {
        case WIFI_DESCONECTADO:
            /* Esperar a que se inicie la conexión */
            osDelayTask(1000);
            break;

        case WIFI_CONECTANDO:
            /* Monitorear el proceso de conexión */
            /* Los eventos WiFi cambiarán el estado a CONECTADO o ERROR */
            osDelayTask(500);
            break;

        case WIFI_CONECTADO:
            /* Monitorear estado de DHCP */
            if (!g_wifi_ctx.dhcp_obtenido)
            {
                DhcpState dhcp_state = dhcpClientGetState(&g_wifi_ctx.dhcp_client_ctx);

                if (dhcp_state == DHCP_STATE_BOUND)
                {
                    Ipv4Addr ipv4_addr;
                    if (ipv4GetHostAddr(g_wifi_ctx.interface_sta, &ipv4_addr) == NO_ERROR)
                    {
                        char_t str[16];
                        ipv4AddrToString(ipv4_addr, str);
                        ESP_LOGI(TAG, "✓ IP obtenida por DHCP: %s", str);

                        /* Obtener y mostrar gateway */
                        Ipv4Addr gateway;
                        if (ipv4GetDefaultGateway(g_wifi_ctx.interface_sta, &gateway) == NO_ERROR)
                        {
                            ipv4AddrToString(gateway, str);
                            ESP_LOGI(TAG, "✓ Gateway: %s", str);
                        }

                        /* Obtener y mostrar DNS */
                        Ipv4Addr dns;
                        if (ipv4GetDnsServer(g_wifi_ctx.interface_sta, 0, &dns) == NO_ERROR)
                        {
                            ipv4AddrToString(dns, str);
                            ESP_LOGI(TAG, "✓ DNS primario: %s", str);
                        }

                        g_wifi_ctx.dhcp_obtenido = true;

                        /* Iniciar interfaz de red */
                        error_t error = netStartInterface(g_wifi_ctx.interface_sta);
                        if (error == NO_ERROR)
                        {
                            ESP_LOGI(TAG, "✓ Interfaz STA iniciada correctamente");
                        }
                        else
                        {
                            ESP_LOGW(TAG, "Advertencia al iniciar interfaz STA: %d", error);
                        }

                        /* Resetear backoff al conectar exitosamente */
                        reset_backoff();
                    }
                }
            }

            /* Mostrar estadísticas periódicas */
            static uint32_t contador_stats = 0;
            contador_stats++;
            if (contador_stats >= 30) /* Cada 30 segundos */
            {
                contador_stats = 0;
                ESP_LOGI(TAG, "Estado STA: %s | Clientes AP: %d",
                         wifi_manager_estado_to_string(g_wifi_ctx.estado),
                         g_wifi_ctx.clientes_ap_conectados);
            }

            osDelayTask(1000);
            break;

        case WIFI_ERROR:
            /* Este estado es transitorio, cambiar a BACKOFF */
            change_wifi_state(WIFI_BACKOFF);
            break;

        case WIFI_BACKOFF:
            /* Esperar el tiempo de backoff antes de reintentar */
            ESP_LOGI(TAG, "Esperando %lu ms antes de reintentar (intento %lu)...",
                     g_wifi_ctx.delay_backoff_actual_ms,
                     g_wifi_ctx.intentos_reconexion + 1);

            osDelayTask(g_wifi_ctx.delay_backoff_actual_ms);

            /* Incrementar contador e intentar reconectar */
            g_wifi_ctx.intentos_reconexion++;

            /* Verificar si alcanzamos el máximo de intentos */
            if (WIFI_MAX_RECONNECT_ATTEMPTS > 0 &&
                g_wifi_ctx.intentos_reconexion >= WIFI_MAX_RECONNECT_ATTEMPTS)
            {
                ESP_LOGE(TAG, "Máximo de intentos alcanzado (%d). Deteniendo reconexiones.",
                         WIFI_MAX_RECONNECT_ATTEMPTS);
                change_wifi_state(WIFI_DESCONECTADO);
                break;
            }

            /* Calcular nuevo backoff exponencial */
            calculate_backoff();

            /* Intentar reconectar */
            ESP_LOGI(TAG, "Reintentando conexión WiFi...");
            esp_wifi_connect();
            change_wifi_state(WIFI_CONECTANDO);
            break;

        default:
            ESP_LOGW(TAG, "Estado desconocido: %d", g_wifi_ctx.estado);
            osDelayTask(1000);
            break;
        }
    }
}

/* ========================================================================== */
/*                      IMPLEMENTACIÓN FUNCIONES PRIVADAS                     */
/* ========================================================================== */

/**
 * @brief Inicializa interfaz STA (wlan0)
 */
static error_t init_interface_sta(void)
{
    error_t error;
    MacAddr mac_addr;

    /* Obtener puntero a la primera interfaz */
    g_wifi_ctx.interface_sta = &netInterface[0];

    /* Configurar nombre de la interfaz */
    netSetInterfaceName(g_wifi_ctx.interface_sta, APP_IF0_NAME);

    /* Configurar hostname */
    netSetHostname(g_wifi_ctx.interface_sta, APP_IF0_HOST_NAME);

    /* Configurar MAC address si no es la por defecto */
    if (strcmp(APP_IF0_MAC_ADDR, "00-00-00-00-00-00") != 0)
    {
        macStringToAddr(APP_IF0_MAC_ADDR, &mac_addr);
        netSetMacAddr(g_wifi_ctx.interface_sta, &mac_addr);
    }

    /* Asignar driver WiFi STA */
    netSetDriver(g_wifi_ctx.interface_sta, &esp32WifiStaDriver);

    /* Configurar interfaz */
    error = netConfigInterface(g_wifi_ctx.interface_sta);
    if (error != NO_ERROR)
    {
        return error;
    }

#if (IPV4_SUPPORT == ENABLED)
#if (APP_IF0_USE_DHCP_CLIENT == ENABLED)
    /* Configurar cliente DHCP */
    dhcpClientGetDefaultSettings(&g_wifi_ctx.dhcp_client_settings);
    g_wifi_ctx.dhcp_client_settings.interface = g_wifi_ctx.interface_sta;
    g_wifi_ctx.dhcp_client_settings.rapidCommit = FALSE;

    error = dhcpClientInit(&g_wifi_ctx.dhcp_client_ctx,
                           &g_wifi_ctx.dhcp_client_settings);
    if (error != NO_ERROR)
    {
        ESP_LOGE(TAG, "Error al inicializar cliente DHCP: %d", error);
        return error;
    }

    error = dhcpClientStart(&g_wifi_ctx.dhcp_client_ctx);
    if (error != NO_ERROR)
    {
        ESP_LOGE(TAG, "Error al iniciar cliente DHCP: %d", error);
        return error;
    }

    ESP_LOGI(TAG, "Cliente DHCP iniciado para interfaz STA");
#else
    /* Configuración estática IPv4 */
    Ipv4Addr ipv4_addr;

    ipv4StringToAddr(APP_IF0_IPV4_HOST_ADDR, &ipv4_addr);
    ipv4SetHostAddr(g_wifi_ctx.interface_sta, ipv4_addr);

    ipv4StringToAddr(APP_IF0_IPV4_SUBNET_MASK, &ipv4_addr);
    ipv4SetSubnetMask(g_wifi_ctx.interface_sta, ipv4_addr);

    ipv4StringToAddr(APP_IF0_IPV4_DEFAULT_GATEWAY, &ipv4_addr);
    ipv4SetDefaultGateway(g_wifi_ctx.interface_sta, ipv4_addr);

    ipv4StringToAddr(APP_IF0_IPV4_PRIMARY_DNS, &ipv4_addr);
    ipv4SetDnsServer(g_wifi_ctx.interface_sta, 0, ipv4_addr);

    ipv4StringToAddr(APP_IF0_IPV4_SECONDARY_DNS, &ipv4_addr);
    ipv4SetDnsServer(g_wifi_ctx.interface_sta, 1, ipv4_addr);

    ESP_LOGI(TAG, "IPv4 estático configurado para interfaz STA");
#endif
#endif

#if (IPV6_SUPPORT == ENABLED)
#if (APP_IF0_USE_SLAAC == ENABLED)
    /* Configurar SLAAC para autoconfiguración IPv6 */
    slaacGetDefaultSettings(&g_wifi_ctx.slaac_settings);
    g_wifi_ctx.slaac_settings.interface = g_wifi_ctx.interface_sta;

    error = slaacInit(&g_wifi_ctx.slaac_ctx, &g_wifi_ctx.slaac_settings);
    if (error != NO_ERROR)
    {
        ESP_LOGE(TAG, "Error al inicializar SLAAC: %d", error);
        return error;
    }

    error = slaacStart(&g_wifi_ctx.slaac_ctx);
    if (error != NO_ERROR)
    {
        ESP_LOGE(TAG, "Error al iniciar SLAAC: %d", error);
        return error;
    }

    ESP_LOGI(TAG, "SLAAC iniciado para interfaz STA");
#else
    /* Configuración estática IPv6 */
    Ipv6Addr ipv6_addr;
    ipv6StringToAddr(APP_IF0_IPV6_LINK_LOCAL_ADDR, &ipv6_addr);
    ipv6SetLinkLocalAddr(g_wifi_ctx.interface_sta, &ipv6_addr);

    ESP_LOGI(TAG, "IPv6 estático configurado para interfaz STA");
#endif
#endif

    return NO_ERROR;
}

/**
 * @brief Inicializa interfaz AP (wlan1)
 */
static error_t init_interface_ap(void)
{
    error_t error;
    MacAddr mac_addr;
    Ipv4Addr ipv4_addr;
    Ipv6Addr ipv6_addr;

    /* Obtener puntero a la segunda interfaz */
    g_wifi_ctx.interface_ap = &netInterface[1];

    /* Configurar nombre de la interfaz */
    netSetInterfaceName(g_wifi_ctx.interface_ap, APP_IF1_NAME);

    /* Configurar hostname */
    netSetHostname(g_wifi_ctx.interface_ap, APP_IF1_HOST_NAME);

    /* Configurar MAC address si no es la por defecto */
    if (strcmp(APP_IF1_MAC_ADDR, "00-00-00-00-00-00") != 0)
    {
        macStringToAddr(APP_IF1_MAC_ADDR, &mac_addr);
        netSetMacAddr(g_wifi_ctx.interface_ap, &mac_addr);
    }

    /* Asignar driver WiFi AP */
    netSetDriver(g_wifi_ctx.interface_ap, &esp32WifiApDriver);

    /* Configurar interfaz */
    error = netConfigInterface(g_wifi_ctx.interface_ap);
    if (error != NO_ERROR)
    {
        return error;
    }

#if (IPV4_SUPPORT == ENABLED)
    /* Configuración estática IPv4 para el AP */
    ipv4StringToAddr(APP_IF1_IPV4_HOST_ADDR, &ipv4_addr);
    ipv4SetHostAddr(g_wifi_ctx.interface_ap, ipv4_addr);

    ipv4StringToAddr(APP_IF1_IPV4_SUBNET_MASK, &ipv4_addr);
    ipv4SetSubnetMask(g_wifi_ctx.interface_ap, ipv4_addr);

    ipv4StringToAddr(APP_IF1_IPV4_DEFAULT_GATEWAY, &ipv4_addr);
    ipv4SetDefaultGateway(g_wifi_ctx.interface_ap, ipv4_addr);

    ipv4StringToAddr(APP_IF1_IPV4_PRIMARY_DNS, &ipv4_addr);
    ipv4SetDnsServer(g_wifi_ctx.interface_ap, 0, ipv4_addr);

    ipv4StringToAddr(APP_IF1_IPV4_SECONDARY_DNS, &ipv4_addr);
    ipv4SetDnsServer(g_wifi_ctx.interface_ap, 1, ipv4_addr);

    ESP_LOGI(TAG, "IPv4 configurado para AP: %s", APP_IF1_IPV4_HOST_ADDR);

#if (APP_IF1_USE_DHCP_SERVER == ENABLED)
    /* Configurar servidor DHCP */
    dhcpServerGetDefaultSettings(&g_wifi_ctx.dhcp_server_settings);
    g_wifi_ctx.dhcp_server_settings.interface = g_wifi_ctx.interface_ap;
    g_wifi_ctx.dhcp_server_settings.leaseTime = 3600;

    ipv4StringToAddr(APP_IF1_IPV4_ADDR_RANGE_MIN,
                     &g_wifi_ctx.dhcp_server_settings.ipAddrRangeMin);
    ipv4StringToAddr(APP_IF1_IPV4_ADDR_RANGE_MAX,
                     &g_wifi_ctx.dhcp_server_settings.ipAddrRangeMax);
    ipv4StringToAddr(APP_IF1_IPV4_SUBNET_MASK,
                     &g_wifi_ctx.dhcp_server_settings.subnetMask);
    ipv4StringToAddr(APP_IF1_IPV4_DEFAULT_GATEWAY,
                     &g_wifi_ctx.dhcp_server_settings.defaultGateway);
    ipv4StringToAddr(APP_IF1_IPV4_PRIMARY_DNS,
                     &g_wifi_ctx.dhcp_server_settings.dnsServer[0]);
    ipv4StringToAddr(APP_IF1_IPV4_SECONDARY_DNS,
                     &g_wifi_ctx.dhcp_server_settings.dnsServer[1]);

    error = dhcpServerInit(&g_wifi_ctx.dhcp_server_ctx,
                           &g_wifi_ctx.dhcp_server_settings);
    if (error != NO_ERROR)
    {
        ESP_LOGE(TAG, "Error al inicializar servidor DHCP: %d", error);
        return error;
    }

    error = dhcpServerStart(&g_wifi_ctx.dhcp_server_ctx);
    if (error != NO_ERROR)
    {
        ESP_LOGE(TAG, "Error al iniciar servidor DHCP: %d", error);
        return error;
    }

    ESP_LOGI(TAG, "Servidor DHCP iniciado (rango: %s - %s)",
             APP_IF1_IPV4_ADDR_RANGE_MIN, APP_IF1_IPV4_ADDR_RANGE_MAX);
#endif
#endif

#if (IPV6_SUPPORT == ENABLED)
    /* Configuración IPv6 para el AP */
    ipv6StringToAddr(APP_IF1_IPV6_LINK_LOCAL_ADDR, &ipv6_addr);
    ipv6SetLinkLocalAddr(g_wifi_ctx.interface_ap, &ipv6_addr);

    ipv6StringToAddr(APP_IF1_IPV6_PREFIX, &ipv6_addr);
    ipv6SetPrefix(g_wifi_ctx.interface_ap, 0, &ipv6_addr, APP_IF1_IPV6_PREFIX_LENGTH);

    ipv6StringToAddr(APP_IF1_IPV6_GLOBAL_ADDR, &ipv6_addr);
    ipv6SetGlobalAddr(g_wifi_ctx.interface_ap, 0, &ipv6_addr);

    ESP_LOGI(TAG, "IPv6 configurado para AP: %s", APP_IF1_IPV6_GLOBAL_ADDR);

#if (APP_IF1_USE_ROUTER_ADV == ENABLED)
    /* Configurar Router Advertisement */
    ipv6StringToAddr(APP_IF1_IPV6_PREFIX, &g_wifi_ctx.ndp_prefix_info[0].prefix);
    g_wifi_ctx.ndp_prefix_info[0].length = APP_IF1_IPV6_PREFIX_LENGTH;
    g_wifi_ctx.ndp_prefix_info[0].onLinkFlag = TRUE;
    g_wifi_ctx.ndp_prefix_info[0].autonomousFlag = TRUE;
    g_wifi_ctx.ndp_prefix_info[0].validLifetime = 3600;
    g_wifi_ctx.ndp_prefix_info[0].preferredLifetime = 1800;

    ndpRouterAdvGetDefaultSettings(&g_wifi_ctx.ndp_router_adv_settings);
    g_wifi_ctx.ndp_router_adv_settings.interface = g_wifi_ctx.interface_ap;
    g_wifi_ctx.ndp_router_adv_settings.maxRtrAdvInterval = 60000;
    g_wifi_ctx.ndp_router_adv_settings.minRtrAdvInterval = 20000;
    g_wifi_ctx.ndp_router_adv_settings.defaultLifetime = 0;
    g_wifi_ctx.ndp_router_adv_settings.prefixList = g_wifi_ctx.ndp_prefix_info;
    g_wifi_ctx.ndp_router_adv_settings.prefixListLength = 1;

    error = ndpRouterAdvInit(&g_wifi_ctx.ndp_router_adv_ctx,
                             &g_wifi_ctx.ndp_router_adv_settings);
    if (error != NO_ERROR)
    {
        ESP_LOGE(TAG, "Error al inicializar Router Advertisement: %d", error);
        return error;
    }

    error = ndpRouterAdvStart(&g_wifi_ctx.ndp_router_adv_ctx);
    if (error != NO_ERROR)
    {
        ESP_LOGE(TAG, "Error al iniciar Router Advertisement: %d", error);
        return error;
    }

    ESP_LOGI(TAG, "Router Advertisement iniciado para AP");
#endif
#endif

    return NO_ERROR;
}

/**
 * @brief Inicia WiFi en modo AP
 * @details Crea red WiFi (Access Point) - Sigue patrón de wifiEnableAp() del main.c de referencia
 */
static esp_err_t start_wifi_ap(void)
{
    esp_err_t ret;
    wifi_config_t config;

    ESP_LOGI(TAG, "Creando red WiFi (%s)...", WIFI_AP_SSID);

    /* Inicializar estructura de configuración */
    memset(&config, 0, sizeof(wifi_config_t));

    /* Configurar parámetros AP */
    strcpy((char *)config.ap.ssid, WIFI_AP_SSID);
    strcpy((char *)config.ap.password, WIFI_AP_PASSWORD);
    config.ap.ssid_len = strlen(WIFI_AP_SSID);
    config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    config.ap.max_connection = WIFI_AP_MAX_CONNECTIONS;
    config.ap.channel = 0; /* Auto */

    /* Configurar modo WiFi */
    ret = esp_wifi_set_mode(WIFI_MODE_AP);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error en esp_wifi_set_mode(AP): %d", ret);
        return ret;
    }

    /* Configurar interfaz AP */
    ret = esp_wifi_set_config(ESP_IF_WIFI_AP, &config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error en esp_wifi_set_config(AP): %d", ret);
        return ret;
    }

    /* Iniciar WiFi */
    ret = esp_wifi_start();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error en esp_wifi_start(AP): %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "✓ AP creado: SSID=%s, Max_Clientes=%d",
             WIFI_AP_SSID, WIFI_AP_MAX_CONNECTIONS);

    g_wifi_ctx.ap_iniciado = true;

    return ESP_OK;
}

/**
 * @brief Conecta a red WiFi en modo STA
 * @details Conecta a router WiFi - Sigue patrón de wifiConnect() del main.c de referencia
 */
static esp_err_t start_wifi_sta(void)
{
    esp_err_t ret;
    wifi_config_t config;

    ESP_LOGI(TAG, "Conectando a red WiFi (%s)...", WIFI_STA_SSID);

    /* Inicializar estructura de configuración */
    memset(&config, 0, sizeof(wifi_config_t));

    /* Configurar parámetros STA */
    strcpy((char *)config.sta.ssid, WIFI_STA_SSID);
    strcpy((char *)config.sta.password, WIFI_STA_PASSWORD);
    config.sta.threshold.authmode = WIFI_AUTH_WPA_PSK;

    /* Configurar modo WiFi (esto cambia de WIFI_MODE_AP a WIFI_MODE_STA) */
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error en esp_wifi_set_mode(STA): %d", ret);
        return ret;
    }

    /* Configurar interfaz STA */
    ret = esp_wifi_set_config(ESP_IF_WIFI_STA, &config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error en esp_wifi_set_config(STA): %d", ret);
        return ret;
    }

    /* Iniciar WiFi (esto reinicia el WiFi en modo STA) */
    ret = esp_wifi_start();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error en esp_wifi_start(STA): %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "✓ WiFi STA iniciado, esperando evento STA_START...");

    /* NOTA: La conexión real se hace en el evento WIFI_EVENT_STA_START */

    return ESP_OK;
}

/**
 * @brief Manejador de eventos WiFi
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        ESP_LOGI(TAG, "→ Evento: STA iniciado");

        /* Conectar al AP cuando STA esté listo (patrón del main.c de referencia) */
        ESP_LOGI(TAG, "  Conectando al AP...");
        esp_wifi_connect();
        break;

    case WIFI_EVENT_STA_CONNECTED:
    {
        wifi_event_sta_connected_t *event =
            (wifi_event_sta_connected_t *)event_data;

        ESP_LOGI(TAG, "→ Evento: STA conectado exitosamente");
        ESP_LOGI(TAG, "  SSID: %s", event->ssid);
        ESP_LOGI(TAG, "  Canal: %d", event->channel);
        ESP_LOGI(TAG, "  BSSID: %02x:%02x:%02x:%02x:%02x:%02x",
                 event->bssid[0], event->bssid[1], event->bssid[2],
                 event->bssid[3], event->bssid[4], event->bssid[5]);

        change_wifi_state(WIFI_CONECTADO);
        break;
    }

    case WIFI_EVENT_STA_DISCONNECTED:
    {
        wifi_event_sta_disconnected_t *event =
            (wifi_event_sta_disconnected_t *)event_data;

        ESP_LOGW(TAG, "→ Evento: STA desconectado");
        ESP_LOGW(TAG, "  Razón: %s",
                 disconnect_reason_to_string(event->reason));
        ESP_LOGW(TAG, "  RSSI: %d dBm", event->rssi);

        /* Cambiar estado a error si estábamos conectados */
        if (g_wifi_ctx.estado == WIFI_CONECTADO)
        {
            g_wifi_ctx.dhcp_obtenido = false;
            change_wifi_state(WIFI_ERROR);
        }
        else
        {
            /* Si no estábamos conectados, reintentar inmediatamente (patrón del main.c) */
            ESP_LOGI(TAG, "  Reintentando conexión...");
            esp_wifi_connect();
        }
        break;
    }

    case WIFI_EVENT_AP_START:
        ESP_LOGI(TAG, "→ Evento: AP iniciado correctamente");
        ESP_LOGI(TAG, "  Red WiFi disponible: %s", WIFI_AP_SSID);
        break;

    case WIFI_EVENT_AP_STACONNECTED:
    {
        wifi_event_ap_staconnected_t *event =
            (wifi_event_ap_staconnected_t *)event_data;

        g_wifi_ctx.clientes_ap_conectados++;

        ESP_LOGI(TAG, "→ Evento: Cliente conectado al AP");
        ESP_LOGI(TAG, "  MAC: %02x:%02x:%02x:%02x:%02x:%02x",
                 event->mac[0], event->mac[1], event->mac[2],
                 event->mac[3], event->mac[4], event->mac[5]);
        ESP_LOGI(TAG, "  AID: %d", event->aid);
        ESP_LOGI(TAG, "  Total clientes conectados: %d",
                 g_wifi_ctx.clientes_ap_conectados);
        break;
    }

    case WIFI_EVENT_AP_STADISCONNECTED:
    {
        wifi_event_ap_stadisconnected_t *event =
            (wifi_event_ap_stadisconnected_t *)event_data;

        if (g_wifi_ctx.clientes_ap_conectados > 0)
        {
            g_wifi_ctx.clientes_ap_conectados--;
        }

        ESP_LOGI(TAG, "→ Evento: Cliente desconectado del AP");
        ESP_LOGI(TAG, "  MAC: %02x:%02x:%02x:%02x:%02x:%02x",
                 event->mac[0], event->mac[1], event->mac[2],
                 event->mac[3], event->mac[4], event->mac[5]);
        ESP_LOGI(TAG, "  AID: %d", event->aid);
        ESP_LOGI(TAG, "  Total clientes conectados: %d",
                 g_wifi_ctx.clientes_ap_conectados);
        break;
    }

    case WIFI_EVENT_AP_STOP:
        ESP_LOGW(TAG, "→ Evento: AP detenido");
        g_wifi_ctx.ap_iniciado = false;
        break;

    default:
        ESP_LOGD(TAG, "→ Evento WiFi no manejado: %ld", event_id);
        break;
    }
}

/**
 * @brief Cambia el estado de la máquina de estados
 */
static void change_wifi_state(WifiEstado_t new_state)
{
    if (g_wifi_ctx.estado != new_state)
    {
        ESP_LOGI(TAG, "Cambio de estado: %s → %s",
                 wifi_manager_estado_to_string(g_wifi_ctx.estado),
                 wifi_manager_estado_to_string(new_state));

        g_wifi_ctx.estado_anterior = g_wifi_ctx.estado;
        g_wifi_ctx.estado = new_state;
        g_wifi_ctx.timestamp_ultimo_intento = osGetSystemTime();
    }
}

/**
 * @brief Calcula el siguiente delay de backoff exponencial
 */
static void calculate_backoff(void)
{
    /* Backoff exponencial: duplicar el delay actual */
    g_wifi_ctx.delay_backoff_actual_ms *= 2;

    /* Limitar al máximo configurado */
    if (g_wifi_ctx.delay_backoff_actual_ms > WIFI_RECONNECT_DELAY_MAX_MS)
    {
        g_wifi_ctx.delay_backoff_actual_ms = WIFI_RECONNECT_DELAY_MAX_MS;
    }

    ESP_LOGI(TAG, "Próximo backoff: %lu ms", g_wifi_ctx.delay_backoff_actual_ms);
}

/**
 * @brief Resetea el contador de backoff
 */
static void reset_backoff(void)
{
    g_wifi_ctx.delay_backoff_actual_ms = WIFI_RECONNECT_DELAY_INITIAL_MS;
    g_wifi_ctx.intentos_reconexion = 0;
    ESP_LOGI(TAG, "Backoff reseteado (conexión exitosa)");
}

/**
 * @brief Convierte razón de desconexión a string
 */
static const char *disconnect_reason_to_string(uint8_t reason)
{
    switch (reason)
    {
    case WIFI_REASON_UNSPECIFIED:
        return "No especificada";
    case WIFI_REASON_AUTH_EXPIRE:
        return "Autenticación expirada";
    case WIFI_REASON_AUTH_LEAVE:
        return "Desautenticado (cliente se desconectó)";
    case WIFI_REASON_ASSOC_EXPIRE:
        return "Asociación expirada";
    case WIFI_REASON_ASSOC_TOOMANY:
        return "Demasiadas asociaciones";
    case WIFI_REASON_NOT_AUTHED:
        return "No autenticado";
    case WIFI_REASON_NOT_ASSOCED:
        return "No asociado";
    case WIFI_REASON_ASSOC_LEAVE:
        return "Desasociado (cliente se desconectó)";
    case WIFI_REASON_ASSOC_NOT_AUTHED:
        return "Asociación sin autenticación";
    case WIFI_REASON_DISASSOC_PWRCAP_BAD:
        return "Capacidad de potencia inaceptable";
    case WIFI_REASON_DISASSOC_SUPCHAN_BAD:
        return "Canales soportados inaceptables";
    case WIFI_REASON_IE_INVALID:
        return "Elemento de información inválido";
    case WIFI_REASON_MIC_FAILURE:
        return "Fallo de MIC (Message Integrity Code)";
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
        return "Timeout en 4-way handshake";
    case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT:
        return "Timeout en actualización de clave de grupo";
    case WIFI_REASON_IE_IN_4WAY_DIFFERS:
        return "IE difiere en 4-way handshake";
    case WIFI_REASON_GROUP_CIPHER_INVALID:
        return "Cifrado de grupo inválido";
    case WIFI_REASON_PAIRWISE_CIPHER_INVALID:
        return "Cifrado por pares inválido";
    case WIFI_REASON_AKMP_INVALID:
        return "AKMP inválido";
    case WIFI_REASON_UNSUPP_RSN_IE_VERSION:
        return "Versión RSN IE no soportada";
    case WIFI_REASON_INVALID_RSN_IE_CAP:
        return "Capacidades RSN IE inválidas";
    case WIFI_REASON_802_1X_AUTH_FAILED:
        return "Autenticación 802.1X fallida";
    case WIFI_REASON_CIPHER_SUITE_REJECTED:
        return "Suite de cifrado rechazada";
    case WIFI_REASON_BEACON_TIMEOUT:
        return "Timeout de beacon (señal perdida)";
    case WIFI_REASON_NO_AP_FOUND:
        return "AP no encontrado";
    case WIFI_REASON_AUTH_FAIL:
        return "Autenticación fallida";
    case WIFI_REASON_ASSOC_FAIL:
        return "Asociación fallida";
    case WIFI_REASON_HANDSHAKE_TIMEOUT:
        return "Timeout en handshake";
    case WIFI_REASON_CONNECTION_FAIL:
        return "Fallo de conexión";
    default:
        return "Razón desconocida";
    }
}
