#include <freertos/FreeRTOS.h>

#include <string.h>

#include "periodic.h"
#include "errors.h"
#include "adc.h"
#include "listener.h"

struct
{
	float min;
	float max;
	uint16_t period;
	uint16_t offset;
	uint8_t flags; /* raw, timestamp */
} adc_config[ADC_COUNT];

int adc_init()
{
	/* Read ADC min/max from EEPROM */

	return 0;
}

void adc_print_value(enum adc adc, uint16_t raw_value)
{
	float value = (adc_config[adc].max - adc_config[adc].min)*raw_value + adc_config[adc].min;

	printf("ADC%d %0.3f\n", adc, value);
}

void adc_command(int adc)
{
	char *cmd = strtok(NULL, " ");

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
			"help - write this text\n"
			"echo - echo all arguments\n"
			"hello - say hello\n");
	}
	else if(strcmp(cmd, "off") == 0)
	{
		adc_off();
	}
	else if(strcmp(cmd, "periodic") == 0)
	{
		int period;
		int offset;
		char *arg;

		/* Read period argument */
		arg = strtok(NULL, " ");
		if(!arg)
			goto einval;

		period = atoi(arg);

		if(period >= 1 << 16)
			goto einval;

		/* Read offset argument */
		arg = strtok(NULL, " ");

		if(arg)
			offset = atoi(arg);
		else
			offset = 0;

		/* Send command */
		adc_periodic(period, offset);
	}
	else
		goto einval;

	return;

einval:
	printf(EINVAL);
	return;
}
