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

#include <stdio.h>
#include "os_port.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_manager.h"
#include "http/http_client.h"
#include "http/http_server.h"
#include "http/http_client_misc.h"
#include "http/http_common.h"
#include "http/mime.h"
#include "debug.h"
#include "esp_log.h"
/* ========================================================================== */
/*                               CONSTANTES                                   */
/* ========================================================================== */
// Application configuration
#define APP_HTTP_SERVER_NAME "192.168.10.5"
#define APP_HTTP_SERVER_PORT 80
#define APP_HTTP_URI "/anything"
/** GPIO del LED integrado */
#define LED_GPIO 2

/** Retardo del LED en milisegundos */
#define LED_BLINK_DELAY_MS 1000

/* ========================================================================== */
/*                          VARIABLES GLOBALES                                */
/* ========================================================================== */

/** Tag para logging */
static const char *TAG = "Main";

/** Handle de la tarea WiFi */
static OsTaskId wifi_task_handle = NULL;

/** Handle de la tarea LED */
static OsTaskId led_task_handle = NULL;

HttpClientContext httpClientContext;
/* ========================================================================== */
/*                      PROTOTIPOS DE FUNCIONES                               */
/* ========================================================================== */

static void led_task(void *params);
static void init_gpio(void);
static void init_nvs(void);

error_t httpClientTest(void)
{
    error_t error;
    size_t length;
    uint_t status;
    const char_t *value;
    IpAddr ipAddr;
    char_t buffer[128];

    // Initialize HTTP client context
    httpClientInit(&httpClientContext);

    // Start of exception handling block
    do
    {
        // Debug message
        TRACE_INFO("\r\n\r\nResolving server name...\r\n");

        // Resolve HTTP server name
        error = getHostByName(NULL, APP_HTTP_SERVER_NAME, &ipAddr, 0);
        // Any error to report?
        if (error)
        {
            // Debug message
            TRACE_INFO("Failed to resolve server name!\r\n");
            break;
        }

        // Select HTTP protocol version
        error = httpClientSetVersion(&httpClientContext, HTTP_VERSION_1_1);
        // Any error to report?
        if (error)
            break;

        // Set timeout value for blocking operations
        error = httpClientSetTimeout(&httpClientContext, 20000);
        // Any error to report?
        if (error)
            break;

        // Debug message
        TRACE_INFO("Connecting to HTTP server %s...\r\n",
                   ipAddrToString(&ipAddr, NULL));

        // Connect to the HTTP server
        error = httpClientConnect(&httpClientContext, &ipAddr,
                                  APP_HTTP_SERVER_PORT);
        // Any error to report?
        if (error)
        {
            // Debug message
            TRACE_INFO("Failed to connect to HTTP server!\r\n");
            break;
        }

        // Create an HTTP request
        httpClientCreateRequest(&httpClientContext);
        httpClientSetMethod(&httpClientContext, "POST");
        httpClientSetUri(&httpClientContext, APP_HTTP_URI);

        // Set query string
        httpClientAddQueryParam(&httpClientContext, "param1", "value1");
        httpClientAddQueryParam(&httpClientContext, "param2", "value2");

        // Add HTTP header fields
        httpClientAddHeaderField(&httpClientContext, "Host", APP_HTTP_SERVER_NAME);
        httpClientAddHeaderField(&httpClientContext, "User-Agent", "Mozilla/5.0");
        httpClientAddHeaderField(&httpClientContext, "Content-Type", "text/plain");
        httpClientAddHeaderField(&httpClientContext, "Transfer-Encoding", "chunked");

        // Send HTTP request header
        error = httpClientWriteHeader(&httpClientContext);
        // Any error to report?
        if (error)
        {
            // Debug message
            TRACE_INFO("Failed to write HTTP request header!\r\n");
            break;
        }

        // Send HTTP request body
        error = httpClientWriteBody(&httpClientContext, "Hello World!", 12,
                                    NULL, 0);
        // Any error to report?
        if (error)
        {
            // Debug message
            TRACE_INFO("Failed to write HTTP request body!\r\n");
            break;
        }

        // Receive HTTP response header
        error = httpClientReadHeader(&httpClientContext);
        // Any error to report?
        if (error)
        {
            // Debug message
            TRACE_INFO("Failed to read HTTP response header!\r\n");
            break;
        }

        // Retrieve HTTP status code
        status = httpClientGetStatus(&httpClientContext);
        // Debug message
        TRACE_INFO("HTTP status code: %u\r\n", status);

        // Retrieve the value of the Content-Type header field
        value = httpClientGetHeaderField(&httpClientContext, "Content-Type");

        // Header field found?
        if (value != NULL)
        {
            // Debug message
            TRACE_INFO("Content-Type header field value: %s\r\n", value);
        }
        else
        {
            // Debug message
            TRACE_INFO("Content-Type header field not found!\r\n");
        }

        // Receive HTTP response body
        while (!error)
        {
            // Read data
            error = httpClientReadBody(&httpClientContext, buffer,
                                       sizeof(buffer) - 1, &length, 0);

            // Check status code
            if (!error)
            {
                // Properly terminate the string with a NULL character
                buffer[length] = '\0';
                // Dump HTTP response body
                TRACE_INFO("%s", buffer);
            }
        }

        // Terminate the HTTP response body with a CRLF
        TRACE_INFO("\r\n");

        // Any error to report?
        if (error != ERROR_END_OF_STREAM)
            break;

        // Close HTTP response body
        error = httpClientCloseBody(&httpClientContext);
        // Any error to report?
        if (error)
        {
            // Debug message
            TRACE_INFO("Failed to read HTTP response trailer!\r\n");
            break;
        }

        // Gracefully disconnect from the HTTP server
        httpClientDisconnect(&httpClientContext);

        // Debug message
        TRACE_INFO("Connection closed\r\n");

        // End of exception handling block
    } while (0);

    // Release HTTP client context
    httpClientDeinit(&httpClientContext);

    // Return status code
    return error;
}
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
 * @brief Inicializa los GPIOs (LED)
 */
static void init_gpio(void)
{
    /* Configurar GPIO del LED como salida */
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0);

    ESP_LOGI(TAG, "GPIO inicializado: LED en GPIO%d", LED_GPIO);
}

/**
 * @brief Tarea que hace parpadear el LED
 * @param params Parámetros de la tarea (no utilizados)
 */
static void led_task(void *params)
{
    uint8_t led_state = 0;

    ESP_LOGI(TAG, "Tarea LED iniciada");

    while (1)
    {
        /* Toggle LED */
        led_state = !led_state;
        gpio_set_level(LED_GPIO, led_state);

        /* Modificar patrón de parpadeo según estado WiFi */
        WifiEstado_t estado = wifi_manager_get_estado();

        switch (estado)
        {
        case WIFI_DESCONECTADO:
        case WIFI_ERROR:
            /* Parpadeo rápido: desconectado */
            osDelayTask(200);
            break;

        case WIFI_CONECTANDO:
        case WIFI_BACKOFF:
            /* Parpadeo medio: conectando */
            osDelayTask(500);
            break;

        case WIFI_CONECTADO:
            /* Parpadeo lento: conectado */
            if (wifi_manager_dhcp_obtenido())
            {
                osDelayTask(LED_BLINK_DELAY_MS);

                httpClientTest();
            }
            else
            {
                /* DHCP en progreso */
                osDelayTask(300);
            }
            break;

        default:
            osDelayTask(LED_BLINK_DELAY_MS);
            break;
        }
    }
}

/**
 * @brief Punto de entrada principal de la aplicación
 */
void app_main(void)
{
    error_t error;
    OsTaskParameters task_params;

    /* Banner de inicio */
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   Curso ESP32 con CycloneTCP - Clase 4        ║");
    ESP_LOGI(TAG, "║   WiFi Dual (AP+STA) con Máquina de Estados   ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Compilado: %s %s", __DATE__, __TIME__);
    ESP_LOGI(TAG, "");

    /* Inicializar NVS */
    ESP_LOGI(TAG, "→ Inicializando subsistemas...");
    init_nvs();

    /* Inicializar GPIO */
    init_gpio();
    
    DirEntry file;
    if (resSearchFile("/text.txt", &file) == NO_ERROR)
    {   
        const uint8_t *text;
        size_t size;
        resGetData("/text.txt", &text, &size);


        ESP_LOGI("TEST", "data: %s", (char *)text);
    }
    
    /* Inicializar gestor WiFi */
    ESP_LOGI(TAG, "→ Inicializando gestor WiFi...");
    error = wifi_manager_init();
    if (error != NO_ERROR)
    {
        ESP_LOGE(TAG, "✗ Error fatal al inicializar WiFi Manager: %d", error);
        ESP_LOGE(TAG, "Sistema detenido.");
        return;
    }

    /* Crear tarea del gestor WiFi */
    task_params = OS_TASK_DEFAULT_PARAMS;
    task_params.stackSize = 4096;
    task_params.priority = OS_TASK_PRIORITY_HIGH;

    wifi_task_handle = osCreateTask("WiFi Manager",
                                    (OsTaskCode)wifi_manager_task,
                                    NULL,
                                    &task_params);

    if (wifi_task_handle == OS_INVALID_TASK_ID)
    {
        ESP_LOGE(TAG, "✗ Error al crear tarea WiFi Manager");
        return;
    }
    ESP_LOGI(TAG, "✓ Tarea WiFi Manager creada");

    /* Crear tarea LED */
    task_params.stackSize = 2048;
    task_params.priority = 10;

    led_task_handle = osCreateTask("LED Task",
                                   (OsTaskCode)led_task,
                                   NULL,
                                   &task_params);

    if (led_task_handle == OS_INVALID_TASK_ID)
    {
        ESP_LOGW(TAG, "Advertencia: No se pudo crear tarea LED");
    }
    else
    {
        ESP_LOGI(TAG, "✓ Tarea LED creada");
    }

    /* Iniciar WiFi en modo dual */
    ESP_LOGI(TAG, "→ Iniciando WiFi en modo dual...");
    error = wifi_manager_start();
    if (error != NO_ERROR)
    {
        ESP_LOGE(TAG, "✗ Error al iniciar WiFi: %d", error);
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "═══════════════════════════════════════════════");
    ESP_LOGI(TAG, "  Sistema iniciado correctamente");
    ESP_LOGI(TAG, "  Estado: Monitoreo activo");
    ESP_LOGI(TAG, "═══════════════════════════════════════════════");
    ESP_LOGI(TAG, "");

    /* Bucle principal: monitoreo y estadísticas */
    uint32_t contador = 0;

    // Protolo de red

    while (1)
    {
        osDelayTask(5000); /* Delay de 5 segundos */
        contador++;

        /* Mostrar estadísticas cada 30 segundos (6 iteraciones) */
        if (contador >= 6)
        {
            contador = 0;

            WifiEstado_t estado = wifi_manager_get_estado();

            ESP_LOGI(TAG, "");
            ESP_LOGI(TAG, "┌─────────────────────────────────────┐");
            ESP_LOGI(TAG, "│  ESTADÍSTICAS DEL SISTEMA           │");
            ESP_LOGI(TAG, "├─────────────────────────────────────┤");
            ESP_LOGI(TAG, "│ Estado STA:    %-20s │",
                     wifi_manager_estado_to_string(estado));
            ESP_LOGI(TAG, "│ DHCP obtenido: %-20s │",
                     wifi_manager_dhcp_obtenido() ? "Sí" : "No");
            ESP_LOGI(TAG, "│ Clientes AP:   %-20d │",
                     wifi_manager_get_clientes_ap());

            if (estado == WIFI_BACKOFF || estado == WIFI_ERROR)
            {
                ESP_LOGI(TAG, "│ Intentos:      %-20lu │",
                         wifi_manager_get_intentos_reconexion());
                ESP_LOGI(TAG, "│ Próx. backoff: %-20lu ms │",
                         wifi_manager_get_delay_backoff());
            }

            ESP_LOGI(TAG, "└─────────────────────────────────────┘");
            ESP_LOGI(TAG, "");
        }
    }
}