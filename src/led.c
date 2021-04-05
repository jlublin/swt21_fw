#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include <nvs_flash.h>
#include <esp_task_wdt.h>

#include <string.h>

#include "periodic.h"
#include "errors.h"
#include "led.h"
#include "listener.h"

const int led_pin = GPIO_NUM_23;

struct
{
	uint8_t flags; /* initialized, blinking */
	uint16_t period;
} lin_config;

const uint8_t LIN_FLAG_INIT = 1 << 0;
const uint8_t LIN_FLAG_BLINKING = 1 << 1;

int led_init()
{
	gpio_pad_select_gpio(led_pin);

	if(gpio_set_direction(led_pin, GPIO_MODE_OUTPUT) != ESP_OK)
		goto esp_err;

	return 0;

esp_err:
	printf("ERR LED init failed!\n");
	return -1;
}

void led_command()
{
	char *cmd = strtok(NULL, " ");
	esp_err_t err;

	/* Make sure we have a command */
	if(!cmd)
		goto einval;

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
			"led on - \n"
			"led off - \n"
			"lin blink [half period] - half period in ms (int, default: 500)\n");
	}
	else if(strcmp(cmd, "on") == 0)
	{
		led_off(1);
	}
	else if(strcmp(cmd, "off") == 0)
	{
		led_off(0);
	}
	else if(strcmp(cmd, "blink") == 0)
	{
		const char *arg = strtok(NULL, " ");
		int period = 500;

		if(arg)
			period = atoi(arg);

		led_blink(period, 0);
	}
	else
		goto einval;

	return;

einval:
	printf(EINVAL);
	return;
}

void led_set_state(int state)
{
	gpio_set_level(led_pin, state);
}
