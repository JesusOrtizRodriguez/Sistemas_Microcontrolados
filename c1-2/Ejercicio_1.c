/*  Integrantes:
    - Johan Masson Vanegas
    - Julian Rapello Olejua 
    - Jesús Ortiz Rodríguez
    - Josue Gutierrez Cantillo
    - Andrea Orozco Carrillo
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUF_SIZE 1024

static void uart_init(int BAUD_RATE) {
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, 1, 3, 22, 19));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL, ESP_INTR_FLAG_IRAM));
}

void app_main(void) {
    uart_init(115200);

    uint8_t data[BUF_SIZE];
    char respuesta[BUF_SIZE];

    while (1) {
        int len = uart_read_bytes(UART_NUM_0, data, BUF_SIZE - 1, 100 / portTICK_PERIOD_MS);

        if (len > 0) {
            data[len] = '\0';

            while (len > 0 && (data[len - 1] == '\n' || data[len - 1] == '\r')) {
                data[--len] = '\0';
            }

            if (len == 0) continue;

            int valido = 1;
            for (int i = 0; i < len; i++) {
                if (data[i] < '0' || data[i] > '9') {
                    valido = 0;
                    break;
                }
            }

            if (!valido) continue;

            int NUMERO = atoi((char *)data);

            if (NUMERO <= 0) continue;

            int suma = 0;
            int impar = 1;
            for (int i = 0; i < NUMERO; i++) {
                suma += impar;
                impar += 2;
            }

            snprintf(respuesta, sizeof(respuesta), "{'Numero': %d, 'Resultado': %d}\n", NUMERO, suma);
            uart_write_bytes(UART_NUM_0, respuesta, strlen(respuesta));
        }
    }
}