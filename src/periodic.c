#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>

#include "listener.h"

void periodic_thread(void *parameters)
{
	esp_task_wdt_delete(xTaskGetCurrentTaskHandle());

	TickType_t current_tick = xTaskGetTickCount();
	TickType_t previous_tick;

	while(1)
	{
		previous_tick = current_tick;
		vTaskDelayUntil(&current_tick, 1);

		if(previous_tick + 1 != current_tick)
			printf("#\n"); // Overrun
	}
}
