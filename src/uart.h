#pragma once

struct uart_thread_parameters
{
	int uart;
};

void command_line_handler(int num, char *line);
void uart_thread(struct uart_thread_parameters *parameters);
