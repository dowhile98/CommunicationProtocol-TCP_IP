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
#include "core/net.h"
#include "drivers/wifi/esp32_wifi_driver.h"
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

    ret = netInit();

    if(ret != NO_ERROR)
    {
        ESP_LOGE(TAG, "Error inicializando la pila TCP/IP: %d", ret);
        return;
    }

    interface = &netInterface[0];

    //nombre

    //host name

    //mac address

    //driver
    ret = netSetDriver(interface, &esp32WifiStaDriver);

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