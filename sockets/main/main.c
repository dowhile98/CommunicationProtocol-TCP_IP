/* ============================================================================
   WiFi Station Example - ESP32
   ============================================================================
   Este código conecta la ESP32 a una red WiFi y obtiene una dirección IP.
   La configuración se realiza mediante menuconfig.
============================================================================ */

/* ============================================================================
   INCLUSIONES
============================================================================ */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/ip4_addr.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
/* ============================================================================
   DEFINES
============================================================================ */
/* Configuración WiFi desde menuconfig */
#define EJEMPLO_SSID_WIFI CONFIG_ESP_WIFI_SSID
#define EJEMPLO_CONTRASEÑA_WIFI CONFIG_ESP_WIFI_PASSWORD
#define EJEMPLO_MAXIMO_REINTENTOS CONFIG_ESP_MAXIMUM_RETRY

/* Configuración de autenticación WPA3 */
#if CONFIG_ESP_STATION_EXAMPLE_WPA3_SAE_PWE_HUNT_AND_PECK
#define MODO_WIFI_SAE WPA3_SAE_PWE_HUNT_AND_PECK
#define EJEMPLO_ID_H2E ""
#elif CONFIG_ESP_STATION_EXAMPLE_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define MODO_WIFI_SAE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EJEMPLO_ID_H2E CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_STATION_EXAMPLE_WPA3_SAE_PWE_BOTH
#define MODO_WIFI_SAE WPA3_SAE_PWE_BOTH
#define EJEMPLO_ID_H2E CONFIG_ESP_WIFI_PW_ID
#endif

/* Configuración de modo de escaneo por autenticación */
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define MODO_AUTENTICACION_WIFI WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define MODO_AUTENTICACION_WIFI WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define MODO_AUTENTICACION_WIFI WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define MODO_AUTENTICACION_WIFI WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define MODO_AUTENTICACION_WIFI WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define MODO_AUTENTICACION_WIFI WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define MODO_AUTENTICACION_WIFI WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define MODO_AUTENTICACION_WIFI WIFI_AUTH_WAPI_PSK
#endif

/* Bits de eventos para WiFi */
#define BIT_WIFI_CONECTADO BIT0
#define BIT_WIFI_ERROR BIT1

/* Configuración del servidor TCP */
#define PUERTO_SERVIDOR_TCP 80
#define TAMAÑO_BUFFER_TCP 1024
#define TAMAÑO_PILA_TAREA_CLIENTE 4096
#define COLA_ESPERA_CONEXIONES 5

/* ============================================================================
   VARIABLES
============================================================================ */
static EventGroupHandle_t grupo_eventos_wifi = NULL;
static const char *TAG = "WiFi Estación";
static const char *TAG_SERVIDOR = "Servidor TCP";
static int contador_reintentos = 0;
static uint32_t direccion_ip = 0;
static int socket_servidor = -1;

/* ============================================================================
   FUNCIONES
============================================================================ */

/**
 * @brief Manejador de eventos WiFi e IP
 *
 * Esta función maneja los siguientes eventos:
 * - WIFI_EVENT_STA_START: Inicia la conexión WiFi
 * - WIFI_EVENT_STA_DISCONNECTED: Reintentos de conexión
 * - IP_EVENT_STA_GOT_IP: Obtiene y guarda la dirección IP
 */
static void manejador_eventos(void *arg, esp_event_base_t base_evento,
                              int32_t id_evento, void *datos_evento)
{
    if (base_evento == WIFI_EVENT && id_evento == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "Iniciando conexión WiFi...");
        esp_wifi_connect();
    }
    else if (base_evento == WIFI_EVENT && id_evento == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (contador_reintentos < EJEMPLO_MAXIMO_REINTENTOS)
        {
            esp_wifi_connect();
            contador_reintentos++;
            ESP_LOGW(TAG, "Reintentando conexión a la red WiFi (intento %d/%d)",
                     contador_reintentos, EJEMPLO_MAXIMO_REINTENTOS);
        }
        else
        {
            xEventGroupSetBits(grupo_eventos_wifi, BIT_WIFI_ERROR);
            ESP_LOGE(TAG, "Error: No se puede conectar a la red WiFi");
        }
    }
    else if (base_evento == IP_EVENT && id_evento == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *evento = (ip_event_got_ip_t *)datos_evento;
        direccion_ip = evento->ip_info.ip.addr;

        ESP_LOGI(TAG, "✓ Conectado a la red WiFi");
        ESP_LOGI(TAG, "Dirección IP obtenida: " IPSTR, IP2STR(&evento->ip_info.ip));
        ESP_LOGI(TAG, "Máscara de red: " IPSTR, IP2STR(&evento->ip_info.netmask));
        ESP_LOGI(TAG, "Puerta de enlace: " IPSTR, IP2STR(&evento->ip_info.gw));

        contador_reintentos = 0;
        xEventGroupSetBits(grupo_eventos_wifi, BIT_WIFI_CONECTADO);
    }
}

/**
 * @brief Inicializa la conexión WiFi en modo estación
 *
 * Configura el WiFi, registra los manejadores de eventos y espera
 * a que se establezca la conexión o se agoten los reintentos.
 *
 * @return uint32_t Dirección IP obtenida (0 si no se conectó)
 */
uint32_t wifi_inicializar_estacion(void)
{
    grupo_eventos_wifi = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t config_wifi = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config_wifi));

    esp_event_handler_instance_t instancia_cualquier_id;
    esp_event_handler_instance_t instancia_obtuvo_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &manejador_eventos,
                                                        NULL,
                                                        &instancia_cualquier_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &manejador_eventos,
                                                        NULL,
                                                        &instancia_obtuvo_ip));

    wifi_config_t config_wifi_sta = {
        .sta = {
            .ssid = EJEMPLO_SSID_WIFI,
            .password = EJEMPLO_CONTRASEÑA_WIFI,
            .threshold.authmode = MODO_AUTENTICACION_WIFI,
            .sae_pwe_h2e = MODO_WIFI_SAE,
            .sae_h2e_identifier = EJEMPLO_ID_H2E,
#ifdef CONFIG_ESP_WIFI_WPA3_COMPATIBLE_SUPPORT
            .disable_wpa3_compatible_mode = 0,
#endif
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config_wifi_sta));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Inicializando WiFi en modo estación...");

    /* Esperamos a que se establezca la conexión (BIT_WIFI_CONECTADO)
       o se agoten los reintentos (BIT_WIFI_ERROR) */
    EventBits_t bits = xEventGroupWaitBits(grupo_eventos_wifi,
                                           BIT_WIFI_CONECTADO | BIT_WIFI_ERROR,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & BIT_WIFI_CONECTADO)
    {
        ESP_LOGI(TAG, "Conexión exitosa a SSID: %s", EJEMPLO_SSID_WIFI);
        return direccion_ip;
    }
    else if (bits & BIT_WIFI_ERROR)
    {
        ESP_LOGE(TAG, "Error: Fallo en la conexión a SSID: %s", EJEMPLO_SSID_WIFI);
        return 0;
    }
    else
    {
        ESP_LOGE(TAG, "Error inesperado en la conexión WiFi");
        return 0;
    }
}

/**
 * @brief Tarea para manejar conexiones de clientes TCP
 *
 * @param pvParameters Puntero al socket del cliente
 */
static void tarea_manejador_cliente(void *pvParameters)
{
    int socket_cliente = (int)pvParameters;
    uint8_t buffer[TAMAÑO_BUFFER_TCP];
    int bytes_recibidos;

    ESP_LOGI(TAG_SERVIDOR, "Nuevo cliente conectado - Socket: %d", socket_cliente);

    while (1)
    {
        /* Recibir datos del cliente */
        bytes_recibidos = recv(socket_cliente, buffer, TAMAÑO_BUFFER_TCP - 1, 0);

        if (bytes_recibidos < 0)
        {
            ESP_LOGE(TAG_SERVIDOR, "Error al recibir datos: %d", errno);
            break;
        }
        else if (bytes_recibidos == 0)
        {
            ESP_LOGI(TAG_SERVIDOR, "Cliente desconectado");
            break;
        }

        /* Null-terminar el buffer para tratarlo como string */
        buffer[bytes_recibidos] = '\0';
        ESP_LOGI(TAG_SERVIDOR, "Datos recibidos (%d bytes): %s", bytes_recibidos, (char *)buffer);

        /* Respuesta HTTP simple */
        const char *respuesta_http =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 40\r\n"
            "Connection: close\r\n"
            "\r\n"
            "¡Hola desde el servidor TCP ESP32!\n";

        if (send(socket_cliente, respuesta_http, strlen(respuesta_http), 0) < 0)
        {
            ESP_LOGE(TAG_SERVIDOR, "Error al enviar respuesta");
            break;
        }

        ESP_LOGI(TAG_SERVIDOR, "Respuesta enviada al cliente");
        break; /* Cerrar después de procesar una solicitud */
    }

    /* Cerrar el socket del cliente */
    shutdown(socket_cliente, 0);
    close(socket_cliente);
    ESP_LOGI(TAG_SERVIDOR, "Socket del cliente cerrado");

    /* Terminar la tarea */
    vTaskDelete(NULL);
}

/**
 * @brief Tarea para el servidor TCP - Acepta conexiones entrantes
 *
 * @param pvParameters No usado
 */
static void tarea_servidor_tcp(void *pvParameters)
{
    struct sockaddr_in direccion_servidor, direccion_cliente;
    int socket_cliente;
    socklen_t tamaño_direccion_cliente;

    ESP_LOGI(TAG_SERVIDOR, "Iniciando servidor TCP en puerto %d", PUERTO_SERVIDOR_TCP);

    /* Crear socket */
    socket_servidor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_servidor < 0)
    {
        ESP_LOGE(TAG_SERVIDOR, "Error: No se pudo crear el socket");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG_SERVIDOR, "✓ Socket creado - FD: %d", socket_servidor);

    /* Preparar dirección del servidor */
    memset(&direccion_servidor, 0, sizeof(direccion_servidor));
    direccion_servidor.sin_family = AF_INET;
    direccion_servidor.sin_addr.s_addr = htonl(INADDR_ANY);
    direccion_servidor.sin_port = htons(PUERTO_SERVIDOR_TCP);

    /* Permitir reutilizar puerto */
    int opt_reutilizar = 1;
    setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &opt_reutilizar, sizeof(opt_reutilizar));

    /* Bind del socket */
    if (bind(socket_servidor, (struct sockaddr *)&direccion_servidor, sizeof(direccion_servidor)) < 0)
    {
        ESP_LOGE(TAG_SERVIDOR, "Error: No se pudo hacer bind al puerto");
        close(socket_servidor);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG_SERVIDOR, "✓ Socket vinculado al puerto %d", PUERTO_SERVIDOR_TCP);

    /* Listen */
    if (listen(socket_servidor, COLA_ESPERA_CONEXIONES) < 0)
    {
        ESP_LOGE(TAG_SERVIDOR, "Error: No se pudo poner en modo escucha");
        close(socket_servidor);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG_SERVIDOR, "✓ Servidor escuchando en puerto %d", PUERTO_SERVIDOR_TCP);
    ESP_LOGI(TAG_SERVIDOR, "Esperando conexiones de clientes...");

    /* Loop de aceptación de conexiones */
    while (1)
    {
        tamaño_direccion_cliente = sizeof(direccion_cliente);

        socket_cliente = accept(socket_servidor,
                                (struct sockaddr *)&direccion_cliente,
                                &tamaño_direccion_cliente);

        if (socket_cliente < 0)
        {
            ESP_LOGE(TAG_SERVIDOR, "Error al aceptar conexión");
            continue;
        }

        /* Mostrar información del cliente */
        char ip_cliente[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &direccion_cliente.sin_addr, ip_cliente, INET_ADDRSTRLEN);
        ESP_LOGI(TAG_SERVIDOR, "Conexión aceptada desde: %s:%d", ip_cliente, ntohs(direccion_cliente.sin_port));

        /* Crear tarea para manejar al cliente */
        if (xTaskCreate(tarea_manejador_cliente,
                        "tarea_cliente",
                        TAMAÑO_PILA_TAREA_CLIENTE,
                        (void *)socket_cliente,
                        5,
                        NULL) != pdPASS)
        {
            ESP_LOGE(TAG_SERVIDOR, "Error: No se pudo crear tarea para el cliente");
            close(socket_cliente);
        }
    }

    /* Esta línea nunca se alcanza, pero por completitud */
    close(socket_servidor);
    vTaskDelete(NULL);
}

/**
 * @brief Inicializa el servidor TCP
 */
static void servidor_tcp_inicializar(void)
{
    if (xTaskCreate(tarea_servidor_tcp,
                    "tarea_servidor_tcp",
                    8192,
                    NULL,
                    5,
                    NULL) != pdPASS)
    {
        ESP_LOGE(TAG_SERVIDOR, "Error: No se pudo crear la tarea del servidor TCP");
    }
}

/* ============================================================================
   APLICACIÓN PRINCIPAL
============================================================================ */

/**
 * @brief Función principal de la aplicación
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=== Iniciando aplicación WiFi ===");

    /* Inicializar NVS (Non-Volatile Storage) */
    esp_err_t resultado = nvs_flash_init();
    if (resultado == ESP_ERR_NVS_NO_FREE_PAGES || resultado == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "Borrando flash de NVS...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        resultado = nvs_flash_init();
    }
    ESP_ERROR_CHECK(resultado);
    ESP_LOGI(TAG, "✓ NVS inicializado");

    /* Configurar nivel de log si es necesario */
    if (CONFIG_LOG_MAXIMUM_LEVEL > CONFIG_LOG_DEFAULT_LEVEL)
    {
        esp_log_level_set("wifi", CONFIG_LOG_MAXIMUM_LEVEL);
    }

    /* Inicializar WiFi */
    ESP_LOGI(TAG, "Configurando WiFi en modo Estación (STA)");
    uint32_t ip_obtenida = wifi_inicializar_estacion();

    if (ip_obtenida != 0)
    {
        ESP_LOGI(TAG, "=== Aplicación lista ===");
        ESP_LOGI(TAG, "IP Obtenida: " IPSTR, IP2STR((ip4_addr_t *)&ip_obtenida));

        /* Inicializar servidor TCP */
        ESP_LOGI(TAG, "Iniciando servidor TCP...");
        servidor_tcp_inicializar();

        ESP_LOGI(TAG, "=== Sistema completamente operativo ===");
    }
    else
    {
        ESP_LOGE(TAG, "=== Error: No se obtuvo dirección IP ===");
        ESP_LOGW(TAG, "Reiniciando el dispositivo...");
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_restart();
    }
}