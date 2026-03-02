/*  Integrantes:
    - Johan Masson Vanegas
    - Julian Rapello Olejua 
    - Jesús Ortiz Rodríguez
    - Josue Gutierrez Cantillo
    - Andrea Orozco Carrillo
*/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

#define UART_PORT UART_NUM_0
#define BUF_SIZE 1024

static void uart_init(int BAUD_RATE)
{
    uart_config_t uart_config ={
        .baud_rate  = BAUD_RATE,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
ESP_ERROR_CHECK(uart_set_pin(UART_PORT,1, 3, 22, 19));
ESP_ERROR_CHECK(uart_driver_install(UART_PORT, BUF_SIZE *2, 0, 0, NULL, ESP_INTR_FLAG_IRAM ));
}
void analizar_json()
{
    char data[BUF_SIZE];
    int len = uart_read_bytes(UART_PORT, (uint8_t*) data, BUF_SIZE, 20 / portTICK_PERIOD_MS);
    if(len != 0)
    {
        data[len] = '\0';
        char *ptr_id = strstr(data, "\"ID\"");
        char *ptr_temp = strstr(data, "\"Temperatura\"");
        char *ptr_hum = strstr(data, "\"Humedad\"");
        char *ptr_dist = strstr(data, "\"Distancia\"");

        if(ptr_id==NULL || ptr_temp==NULL || ptr_hum==NULL || ptr_dist==NULL)
        {
            char error_msg[] = "Error: Falta una clave y es obligatoria\n";
            uart_write_bytes(UART_PORT, error_msg, strlen(error_msg));
            return;
        }
        char id[50];
        float temperatura;
        float humedad;
        float distancia;

        sscanf(ptr_id, "\"ID\":\"%[^\"]\"", id);
        sscanf(ptr_temp, "\"Temperatura\":%f", &temperatura);
        sscanf(ptr_hum, "\"Humedad\":%f", &humedad);
        sscanf(ptr_dist, "\"Distancia\":%f", &distancia);


        char buffer[200];
        sprintf(buffer, "ID: %s\n", id);
        uart_write_bytes(UART_PORT, buffer, strlen(buffer));

        sprintf(buffer, "Temperatura: %.2f\n", temperatura);
        uart_write_bytes(UART_PORT, buffer, strlen(buffer));

        sprintf(buffer, "Humedad: %.2f\n", humedad);
        uart_write_bytes(UART_PORT, buffer, strlen(buffer));

        sprintf(buffer, "Distancia: %.2f\n\n", distancia);
        uart_write_bytes(UART_PORT, buffer, strlen(buffer));

    }
}
void app_main()
{
  uart_init(115200);

  printf("Iniciando analizador JSON...\n");

  while(1)
  {
    analizar_json();
  }
}