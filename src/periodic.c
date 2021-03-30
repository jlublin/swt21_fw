#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_task_wdt.h>

#include "listener.h"

/*
 * Periodic configuration does not keep the actual configuration but only a copy
 * of the values it needs.
 */
static struct
{
	struct
	{
		uint32_t next;
		uint16_t period;
	} adc[2];
} periodic_conf;

static uint16_t run_flags;
static const uint16_t RUN_FLAG_ADC0 = 1 << 0;
//static const uint16_t RUN_FLAG_ADC1 = 1 << 1;

static QueueHandle_t periodic_queue;

struct periodic_event
{
	uint8_t event;
	union
	{
		struct
		{
			uint16_t period;
			uint16_t offset;
		} adc_periodic;
	};
};

enum
{
	EVENT_ADC_OFF = 0,
	EVENT_ADC_PERIODIC = 1
};

/*******************************************************************************
 *
 ******************************************************************************/
void adc_off()
{
	struct periodic_event event =
	{
		.event = EVENT_ADC_OFF,
	};

	xQueueSendToBack(periodic_queue, &event, 0);
}

/*******************************************************************************
 *
 ******************************************************************************/
void adc_periodic(uint16_t period, uint16_t offset)
{
	struct periodic_event event =
	{
		.event = EVENT_ADC_PERIODIC,
		.adc_periodic.period = period,
		.adc_periodic.offset = offset
	};

	xQueueSendToBack(periodic_queue, &event, 0);
}

/*******************************************************************************
 *
 ******************************************************************************/
void periodic_thread(void *parameters)
{
	esp_task_wdt_delete(xTaskGetCurrentTaskHandle());

	periodic_queue = xQueueCreate(10, sizeof(struct periodic_event));

	TickType_t current_tick = xTaskGetTickCount();
	TickType_t previous_tick;

	while(1)
	{
		previous_tick = current_tick;
		vTaskDelayUntil(&current_tick, 1);

		if(previous_tick + 1 != current_tick)
			printf("#\n"); // Overrun

		if(run_flags & RUN_FLAG_ADC0)
		{
			if(current_tick >= periodic_conf.adc[0].next)
			{
				periodic_conf.adc[0].next += periodic_conf.adc[0].period;
				printf("ADC0: ...\n");
			}
		}

		struct periodic_event event;
		if(xQueueReceive(periodic_queue, &event, 0))
		{
			if(event.event == EVENT_ADC_OFF)
			{
				run_flags &= ~RUN_FLAG_ADC0;
				printf("OK\n");
			}

			if(event.event == EVENT_ADC_PERIODIC)
			{
				uint16_t period = event.adc_periodic.period;
				uint16_t offset = event.adc_periodic.offset;

				uint32_t current_offset = current_tick % period;

				periodic_conf.adc[0].period = period;
				periodic_conf.adc[0].next = current_tick - current_offset + period + offset;

				run_flags |= RUN_FLAG_ADC0;
				printf("OK\n");
			}
		}

	}
}
