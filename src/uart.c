#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>
#include <driver/uart.h>

#include "uart.h"
#include "hci.h"

void command_line_handler(int num, char *line)
{
}

void uart_thread(struct uart_thread_parameters *parameters)
{
	/* Setup UART */

	uart_config_t uart_config =
	{
		.baud_rate = 115200,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	};

	uart_param_config(parameters->uart, &uart_config);
    uart_driver_install(parameters->uart, 1024 * 2, 0, 0, NULL, 0);

	esp_task_wdt_delete(xTaskGetCurrentTaskHandle());

	const int buffer_size = 256;

	char line[buffer_size];
	int index = 0;

	while(1)
	{
		uint8_t c;
		int ret = uart_read_bytes(parameters->uart, &c, 1, 100 / portTICK_RATE_MS);
	}
}
