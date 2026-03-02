/*  Integrantes:
    - Johan Masson Vanegas
    - Julian Rapello Olejua 
    - Jesús Ortiz Rodríguez
    - Josue Gutierrez Cantillo
    - Andrea Orozco Carrillo
*/

#include <stdio.h>      // printf, snprintf
#include <string.h>     // strlen, strcmp, strncmp, strpbrk
#include <ctype.h>      // isdigit

#include "freertos/FreeRTOS.h"  // Sistema operativo en tiempo real
#include "freertos/task.h"      // Manejo de tareas
#include "driver/uart.h"        // Driver oficial de UART ESP32

#define UART_PORT UART_NUM_0   // Usamos UART0 (la del monitor serie)
#define BUF_SIZE 128           // Tamaño máximo de buffer para una trama

/* 
   ESTRUCTURA PARA GUARDAR ESTADISTICAS
   Estas variables deben persistir mientras el sistema esté activo */

typedef struct {
    int ultimo;        // Último valor válido recibido
    int mayor;         // Valor máximo histórico
    int menor;         // Valor mínimo histórico
    long long suma;    // Suma acumulada (para calcular promedio)
    int cantidad;      // Cantidad total de valores válidos recibidos
} EstadisticasCaudal;

/* Instancia global de estadísticas.
   Se inicializa en cero y vive durante toda la ejecución */
static EstadisticasCaudal est = {0, 0, 0, 0, 0};

/* 
   Valida estrictamente el formato:
   {'caudal': VALOR}

   - VALOR debe ser entero
   - Debe estar entre 0 y 99
   - No se permiten espacios extra ni texto adicional

   Devuelve:
     1 -> si la trama es válida
     0 -> si la trama debe ignorarse */

static int parsear_trama_caudal(const char *linea, int *valor) {

    // Formato exacto requerido por la consigna
    const char *formato = "{'caudal': ";

    // Longitud del prefijo para compararlo exactamente
    size_t prefijo_len = strlen(formato);

    // Verifica que la línea comience EXACTAMENTE con el prefijo
    if (strncmp(linea, formato, prefijo_len) != 0)
        return 0;  // Formato incorrecto

    // Puntero que avanza justo después del prefijo
    const char *p = linea + prefijo_len;

    // Debe comenzar con un dígito
    if (!isdigit((unsigned char)*p))
        return 0;

    int numero = 0;

    // Construcción manual del número (solo acepta dígitos)
    while (isdigit((unsigned char)*p)) {
        numero = (numero * 10) + (*p - '0');
        p++;  // Avanza al siguiente carácter
    }

    // Después del número solo puede venir el carácter '}'
    if (strcmp(p, "}") != 0)
        return 0;

    // Validación de rango según la consigna
    if (numero < 0 || numero > 99)
        return 0;

    // Si pasó todas las validaciones, guardamos el valor
    *valor = numero;
    return 1;
}

/* 
   FUNCION: actualizar_estadisticas
   Actualiza:
   - último valor
   - máximo histórico
   - mínimo histórico
   - suma acumulada
   - cantidad total  */

static void actualizar_estadisticas(int valor) {

    // Guardamos el último valor recibido
    est.ultimo = valor;

    // Si es el primer dato recibido
    if (est.cantidad == 0) {
        est.mayor = valor;
        est.menor = valor;
    } else {
        // Actualización de máximo histórico
        if (valor > est.mayor)
            est.mayor = valor;

        // Actualización de mínimo histórico
        if (valor < est.menor)
            est.menor = valor;
    }

    // Acumulamos para calcular promedio
    est.suma += valor;

    // Incrementamos cantidad de datos válidos
    est.cantidad++;
} 

static void enviar_respuesta_uart(void) {

    char buffer[128];

    // Cálculo del promedio con decimales
    double promedio = (double)est.suma / (double)est.cantidad;

    // Construimos la cadena final con el formato exacto pedido
    snprintf(buffer, sizeof(buffer),
             "{'ultimo': %d, 'mayor': %d, 'menor': %d, 'promedio': %.2f}\n",
             est.ultimo,
             est.mayor,
             est.menor,
             promedio);

    // Enviamos la respuesta por UART
    uart_write_bytes(UART_PORT, buffer, strlen(buffer));
}

void app_main(void)
{
    /* Configuración básica de UART */

    uart_config_t uart_config = {
        .baud_rate = 115200,              // Velocidad de comunicación
        .data_bits = UART_DATA_8_BITS,    // 8 bits de datos
        .parity    = UART_PARITY_DISABLE, // Sin paridad
        .stop_bits = UART_STOP_BITS_1,    // 1 bit de stop
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    // Instala el driver UART
    uart_driver_install(UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0);

    // Aplica la configuración
    uart_param_config(UART_PORT, &uart_config);

    char linea[BUF_SIZE];

    // Bucle infinito: el sistema debe estar siempre escuchando
    while (1) {

        // Intentamos leer datos desde UART
        int len = uart_read_bytes(UART_PORT,
                                  (uint8_t *)linea,
                                  BUF_SIZE - 1,
                                  pdMS_TO_TICKS(1000));

        // Si se recibieron datos
        if (len > 0) {

            // Terminamos la cadena con '\0'
            linea[len] = '\0';

            // Eliminamos posibles \r o \n
            char *nl = strpbrk(linea, "\r\n");
            if (nl) *nl = '\0';

            int valor;

            // Validamos la trama
            if (parsear_trama_caudal(linea, &valor)) {

                // Solo si es válida actualizamos estadísticas
                actualizar_estadisticas(valor);

                // Enviamos respuesta
                enviar_respuesta_uart();
            }

            // Si no es válida → se ignora completamente (robustez)
        }

        // Pequeña pausa para no saturar CPU
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}