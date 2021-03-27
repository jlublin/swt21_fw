#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_task_wdt.h>
#include <driver/uart.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "listener.h"
//#include "uart.h"

static void listener_line_handler(char *line);

static const char *TAG = "listener";
static const int uart = UART_NUM_0;

/*******************************************************************************
 *
 ******************************************************************************/
void print_str(const char *format, ...)
{
	va_list arglist;
	va_start(arglist, format);

	char buf[256];
	vsnprintf(buf, 255, format, arglist);
	buf[255] = 0;
	uart_write_bytes(uart, buf, strlen(buf));

	va_end(arglist);
}

/*******************************************************************************
 *
 ******************************************************************************/
void listener_thread(void *parameters)
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

	uart_param_config(uart, &uart_config);
    uart_driver_install(uart, 1024 * 2, 0, 0, NULL, 0);

	/* Start listener */
	ESP_LOGI(TAG, "\nStarting listener thread");
	esp_task_wdt_delete(xTaskGetCurrentTaskHandle());

	const int buffer_size = 256;

	char line[buffer_size];
	int index = 0;

	while(1)
	{
		uint8_t c;
		int ret = uart_read_bytes(uart, &c, 1, 100 / portTICK_RATE_MS);

		/* If we did not receive any character then try again */
		if(ret != 1)
			continue;

		/* Convert DEL (0x7f) into backspace (0x08), needed in some terminals */
		if(c == '\x7f')
			c = '\b';

		/* Handle backspace specifically */
		if(c == '\b')
		{
			if(index > 0)
			{
				index -= 1;
				printf("\b \b"); /* Back, overwrite with space and back again */
			}
		}
		/* Handle new line */
		else if(c == '\n')
		{
			printf("\n");
			line[index] = 0;
			listener_line_handler(line);
			index = 0;
		}
		/* Ignore all other control characters */
		else if(c < 0x20)
		{
			continue;
		}
		/* Otherwise echo character written */
		else
		{
			uart_write_bytes(uart, (char*)&c, 1);
			line[index++] = c;
		}
	}
}

/*******************************************************************************
 *
 ******************************************************************************/
static void listener_line_handler(char *line)
{
	char *cmd = strtok(line, " ");

	/* Make sure we have a command */
	if(cmd == NULL)
		return;

	/*
	 * Compare commande against known commands and handle them, otherwise tell
	 * the user it was invalid.
	 */
	if(strcmp(cmd, "help") == 0)
	{
		printf("OK\n");
		printf(
			"Available commands:\n"
			"\n"
			"help - write this text\n"
			"echo - echo all arguments\n"
			"hello - say hello\n");
	}
	else if(strcmp(cmd, "echo") == 0)
	{
		printf("Echo: %s\n", strtok(NULL, ""));
	}
	else if(strncmp(cmd, "CAN", 4) == 0)
	{
//		cmd_uart(cmd[4], line);
	}
	else if(strncmp(cmd, "UART", 4) == 0)
	{
//		cmd_uart(cmd[4], line);
	}
	else
		printf("ERR Unknown command\n");
}
