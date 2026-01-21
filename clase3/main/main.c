/**
 * @file main.c
 * @brief Ejemplo Blink (archivo principal)
 * @details
 * Ejemplo mínimo que mantiene la tarea principal en un bucle con retardo.
 * Este archivo está estructurado para que la documentación Doxygen
 * pueda generar secciones claras: inclusiones, macros, typedefs,
 * variables globales, prototipos y definiciones de funciones.
 */
/**
 * dependeny injections
 */
/* ------------------------- Inclusiones ------------------------- */
#include <stdio.h>
#include "os_port.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "include/app_config.h"
#include "core/net.h"
#include "drivers/wifi/esp32_wifi_driver.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include <string.h>
/* --------------------------- Macros ---------------------------- */
/** Retardo entre toggles (ms) */
#define BLINK_DELAY_MS 1000

/* --------------------------- Typedefs -------------------------- */
/* (ninguno por ahora) */

/* ---------------------- Variables globales --------------------- */
/** Etiqueta para logging */
static const char *TAG = "example";
OsTaskId wifiTask = NULL;
OsTaskId sensorTask = NULL;
/* --------------------- Prototipos de funciones ----------------- */
/**
 * @brief Punto de entrada principal de la aplicación
 * @details
 * Función llamada por el runtime del sistema. Se encarga de la
 * inicialización necesaria y del bucle principal de la aplicación.
 */
void app_main(void);

static app_wifi_task_entry(void *params);
static app_sensor_task_entry(void *params);
/* -------------------- Definición de funciones ------------------ */

/**
 * @brief Punto de entrada de la aplicación
 * @note Mantiene la misma funcionalidad que la versión original.
 */
void app_main(void)
{
    OsTaskParameters params = OS_TASK_DEFAULT_PARAMS;

    /* Configuración de periféricos y tareas (si aplica) */

    // Crear tarea WiFi
    params.priority = 10;
    params.stackSize = 1024;

    wifiTask = osCreateTask("WiFi Task", app_wifi_task_entry, NULL, &params);

    if (wifiTask == OS_INVALID_TASK_ID)
    {
        ESP_LOGE(TAG, "Error al crear la tarea WiFi");
    }

    // Crear tarea Sensor
    params.priority = 8;
    params.stackSize = 1024;
    sensorTask = osCreateTask("Sensor Task", app_sensor_task_entry, NULL, &params);
    if (sensorTask == OS_INVALID_TASK_ID)
    {
        ESP_LOGE(TAG, "Error al crear la tarea Sensor");
    }
    /* Bucle principal */
    ESP_LOGI(TAG, "Inicio del bucle principal");
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(BLINK_DELAY_MS));
    }
}

static app_wifi_task_entry(void *params)
{
    error_t ret = NO_ERROR;
    NetInterface *interface;
    DhcpClientContext dhcpContext;
    DhcpClientSettings dhcpSettings;

    /* Inicializar la pila TCP/IP (CycloneTCP) */
    ret = netInit();

    if (ret != NO_ERROR)
    {
        ESP_LOGE(TAG, "Error inicializando la pila TCP/IP: %d", ret);
        return;
    }

    interface = &netInterface[0];

    /* Crear loop de eventos necesario para los handlers del driver */
    esp_event_loop_create_default();

    /* Asociar el driver ESP32 (STA) a la interfaz */
    ret = netSetDriver(interface, &esp32WifiStaDriver);
    if (ret != NO_ERROR)
    {
        ESP_LOGE(TAG, "Error al asignar driver WiFi: %d", ret);
        return;
    }

    /* Nombre y hostname de la interfaz (opcional) */
    netSetInterfaceName(interface, "wlan0");
    netSetHostname(interface, WIFI_HOSTNAME);

    /* Configurar e iniciar la interfaz (llama a esp32WifiInit internamente) */
    ret = netConfigInterface(interface);
    if (ret != NO_ERROR)
    {
        ESP_LOGE(TAG, "Error al configurar interfaz: %d", ret);
        return;
    }

    /* Al inicializar el driver, esp_wifi_init() habrá sido invocado.
       Ahora configuramos el modo STA y los parámetros de conexión */
    esp_err_t err;

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_set_mode failed: %d", err);
    }

    wifi_config_t wifi_cfg;
    memset(&wifi_cfg, 0, sizeof(wifi_cfg));
    strncpy((char *)wifi_cfg.sta.ssid, WIFI_SSID, sizeof(wifi_cfg.sta.ssid) - 1);
    strncpy((char *)wifi_cfg.sta.password, WIFI_PSK, sizeof(wifi_cfg.sta.password) - 1);

    err = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_cfg);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_set_config failed: %d", err);
    }

    err = esp_wifi_start();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_start failed: %d", err);
    }

    err = esp_wifi_connect();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_wifi_connect failed: %d", err);
    }

    /* Arrancar cliente DHCP (CycloneTCP) para obtener IP dinámica */
    dhcpClientGetDefaultSettings(&dhcpSettings);
    dhcpSettings.interface = interface;
    dhcpSettings.ipAddrIndex = 0;

    ret = dhcpClientInit(&dhcpContext, &dhcpSettings);
    if (ret != NO_ERROR)
    {
        ESP_LOGE(TAG, "dhcpClientInit failed: %d", ret);
    }
    else
    {
        ret = dhcpClientStart(&dhcpContext);
        if (ret != NO_ERROR)
        {
            ESP_LOGE(TAG, "dhcpClientStart failed: %d", ret);
        }
        else
        {
            ESP_LOGI(TAG, "DHCP client started, waiting for lease...");

            /* Esperar a que DHCP otorgue la dirección */
            for (int i = 0; i < 60; i++)
            {
                if (dhcpClientGetState(&dhcpContext) == DHCP_STATE_BOUND)
                {
                    Ipv4Addr ipAddr;
                    if (ipv4GetHostAddr(interface, &ipAddr) == NO_ERROR)
                    {
                        char_t str[16];
                        ipv4AddrToString(ipAddr, str);
                        ESP_LOGI(TAG, "IP obtenida por DHCP: %s", str);
                    }
                    break;
                }
                osDelayTask(1000);
            }
        }
    }

    /* Iniciar la interfaz para que empiece a enviar/recibir paquetes */
    ret = netStartInterface(interface);
    if (ret != NO_ERROR)
    {
        ESP_LOGE(TAG, "Error al iniciar interfaz: %d", ret);
    }

    for (;;)
    {
        osDelayTask(1000);
    }
}
static app_sensor_task_entry(void *params)
{

    for (;;)
    {

        osDelayTask(1000);
    }
}