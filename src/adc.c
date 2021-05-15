#include <freertos/FreeRTOS.h>
#include <driver/adc.h>
#include <nvs_flash.h>

#include <string.h>

#include "periodic.h"
#include "errors.h"
#include "adc.h"
#include "hci.h"

int adc_channel[ADC_COUNT] =
{
	ADC1_CHANNEL_4,
	ADC1_CHANNEL_5
};

const int ADC_BITS = 12;

struct
{
	uint16_t v1x_0_1v;
	uint16_t v1x_2v;
	uint16_t v10x_1v;
	uint16_t v10x_20v;
	uint16_t period;
	uint16_t offset;
	uint8_t flags; /* initialized, raw, amp, timestamp */
} adc_config[ADC_COUNT];

const uint8_t ADC_FLAG_INIT = 1 << 0;
const uint8_t ADC_FLAG_RAW =  1 << 1;
const uint8_t ADC_FLAG_AMP10X = 1 << 2;

int adc_init()
{
	esp_err_t err;

	adc1_config_width(ADC_WIDTH_BIT_12);

	nvs_handle_t nvs_handle;
	err = nvs_open("SWT21 Lab kit", NVS_READWRITE, &nvs_handle);
	if(err != ESP_OK)
		goto esp_err;

	cmd_queue = xQueueCreate(10, sizeof(struct cmd_event));
	if(!cmd_queue)
		goto esp_err;

	for(int i = 0; i < ADC_COUNT; i++)
	{
		int error_flag = 0;
		char parameter_name[25] = {0};
		uint32_t value;

		snprintf(parameter_name, 24, "adc%d_1x_0_1v", i);
		err = nvs_get_u32(nvs_handle, parameter_name, &value);
		if(err)
		{
			printf("ERR %s not configured\n", parameter_name);
			error_flag = 1;
		}
		adc_config[i].v1x_0_1v = value;

		snprintf(parameter_name, 24, "adc%d_1x_2v", i);
		err = nvs_get_u32(nvs_handle, parameter_name, &value);
		if(err)
		{
			printf("ERR %s not configured\n", parameter_name);
			error_flag = 1;
		}
		adc_config[i].v1x_2v = value;

		snprintf(parameter_name, 24, "adc%d_10x_0_1v", i);
		err = nvs_get_u32(nvs_handle, parameter_name, &value);
		if(err)
		{
			printf("ERR %s not configured\n", parameter_name);
			error_flag = 1;
		}
		adc_config[i].v10x_1v = value;

		snprintf(parameter_name, 24, "adc%d_10x_20v", i);
		err = nvs_get_u32(nvs_handle, parameter_name, &value);
		if(err)
		{
			printf("ERR %s not configured\n", parameter_name);
			error_flag = 1;
		}
		adc_config[i].v10x_20v = value;

		/* Set channel attenuation */
		adc1_config_channel_atten(adc_channel[i], ADC_ATTEN_DB_11);

		/* Mark as initialized if we did not have any errors */
		if(!error_flag)
			adc_config[0].flags |= ADC_FLAG_INIT;
	}

	nvs_close(nvs_handle);

	return 0;
}

uint16_t adc_single(enum adc adc)
{
	return adc1_get_raw(adc_channel[adc]);
}

void adc_print_value(enum adc adc, uint16_t raw_value)
{
	if(adc_config[adc].flags & ADC_FLAG_RAW)
	{
		printf("ADC%d %d\n", adc, raw_value);
	}
	else
	{
		int amp = adc_config[adc].flags & ADC_FLAG_AMP10X;
		float value;

		if(amp)
			value = 1.0 +
			        19.0/(adc_config[adc].v10x_20v - adc_config[adc].v10x_1v) *
			        (raw_value - adc_config[adc].v10x_1v);
		else
			value = 0.1 +
			        1.9/(adc_config[adc].v1x_2v - adc_config[adc].v1x_0_1v) *
			        (raw_value - adc_config[adc].v1x_0_1v);

		printf("ADC%d %0.3f\n", adc, value);
	}
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
			"adc%d off - turn off periodic adc\n"
			"adc%d single - convert single value\n"
			"adc%d periodic <period (ms)> [offset (ms)] - convert periodically\n"
			"adc%d trig <trig value> <sample rate> <m> <n> - wait for trigger and\n"
			"                                              convert m values before and\n"
			"                                              n values after\n"
			"\n", adc, adc, adc, adc);
			/* TODO: add trig sample rate? */
	}
	else if(strcmp(cmd, "off") == 0)
	{
		adc_off();
	}
	else if(strcmp(cmd, "single") == 0)
	{
		adc_print_value(adc, adc_single(adc));
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
