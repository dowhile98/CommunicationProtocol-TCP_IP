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

/* ========================================================================== */
/*                               CONSTANTES                                   */
/* ========================================================================== */

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

/* ========================================================================== */
/*                      PROTOTIPOS DE FUNCIONES                               */
/* ========================================================================== */

static void led_task(void *params);
static void init_gpio(void);
static void init_nvs(void);

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