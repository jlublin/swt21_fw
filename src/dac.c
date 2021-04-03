#include <freertos/FreeRTOS.h>
#include <driver/gpio.h> /* Require by driver/dac.h */
#include <driver/dac.h>
#include <nvs_flash.h>

#include <string.h>

#include "errors.h"
#include "dac.h"
#include "listener.h"

int dac_channel[DAC_COUNT] =
{
	DAC_CHANNEL_1,
	DAC_CHANNEL_2
};

struct
{
	float min_1x;
	float max_1x;
	float min_10x;
	float max_10x;
	uint8_t flags; /* initialized */
} dac_config[DAC_COUNT];

const uint8_t DAC_FLAG_INIT = 1 << 0;
const uint8_t DAC_FLAG_AMP10X = 1 << 1;

int dac_init()
{
	esp_err_t err;

	nvs_handle_t nvs_handle;
	err = nvs_open("SWT21 Lab kit", NVS_READWRITE, &nvs_handle);
	if(err != ESP_OK)
	{
		printf("ERR DAC init failed!\n");
		return -1;
	}


	for(int i = 0; i < DAC_COUNT; i++)
	{
		int error_flag = 0;
		char parameter_name[24] = {0};
		union
		{
			float f;
			uint32_t u;
		} value;

		snprintf(parameter_name, 23, "dac%d_min_1x", i);
		err = nvs_get_u32(nvs_handle, parameter_name, &value.u);
		if(err)
		{
			printf("ERR %s not configured\n", parameter_name);
			error_flag = 1;
		}
		dac_config[i].min_1x = value.f;

		snprintf(parameter_name, 23, "dac%d_max_1x", i);
		err = nvs_get_u32(nvs_handle, parameter_name, &value.u);
		if(err)
		{
			printf("ERR %s not configured\n", parameter_name);
			error_flag = 1;
		}
		dac_config[i].max_1x = value.f;

		snprintf(parameter_name, 23, "dac%d_min_10x", i);
		err = nvs_get_u32(nvs_handle, parameter_name, &value.u);
		if(err)
		{
			printf("ERR %s not configured\n", parameter_name);
			error_flag = 1;
		}
		dac_config[i].min_10x = value.f;

		snprintf(parameter_name, 23, "dac%d_max_10x", i);
		err = nvs_get_u32(nvs_handle, parameter_name, &value.u);
		if(err)
		{
			printf("ERR %s not configured\n", parameter_name);
			error_flag = 1;
		}
		dac_config[i].max_10x = value.f;

		/* Enable output */
		dac_output_enable(dac_channel[i]);

		/* Mark as initialized if we did not have any errors */
		if(!error_flag)
			dac_config[0].flags |= DAC_FLAG_INIT;
	}

	nvs_close(nvs_handle);

	return 0;
}

void dac_command(int dac)
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
			"dac voltage - \n"
			"dac raw - \n"
			"dac config amp10x [1/0] - \n");
	}
	else if(strcmp(cmd, "voltage") == 0)
	{
		/* Read value argument */
		const char *arg = strtok(NULL, " ");
		if(!arg)
			goto einval;

		float voltage = strtof(arg, NULL);

		if(dac_config[dac].flags & DAC_FLAG_AMP10X)
		{
			if(voltage < 0 || voltage > 33)
				goto einval;

			uint8_t value =
				(voltage - dac_config[dac].min_10x) * 255 /
				(dac_config[dac].max_10x - dac_config[dac].min_10x);

			dac_output_voltage(dac_channel[dac], value);
		}
		else
		{
			if(voltage < 0 || voltage > 3.3)
				goto einval;

			uint8_t value =
				(voltage - dac_config[dac].min_1x) * 255 /
				(dac_config[dac].max_1x - dac_config[dac].min_1x);

			dac_output_voltage(dac_channel[dac], value);
		}
	}
	else if(strcmp(cmd, "raw") == 0)
	{
		/* Read value argument */
		const char *arg = strtok(NULL, " ");
		if(!arg)
			goto einval;

		int raw = atoi(arg);

		if(raw < 0 || raw > 255)
			goto einval;

		dac_output_voltage(dac_channel[dac], raw);
		printf("OK\n");
	}
	else if(strcmp(cmd, "config") == 0)
	{
		/* Read value argument */
		const char *arg = strtok(NULL, " ");
		if(!arg)
			goto einval;

		if(strcmp(arg, "amp10x") == 0)
		{
			const char *value_str = strtok(NULL, " ");
			if(!value_str)
			{
				if(dac_config[dac].flags & DAC_FLAG_AMP10X)
					printf("OK 1\n");
				else
					printf("OK 0\n");
			}
			else
			{
				int value = atoi(value_str);
				if(value == 1)
					dac_config[dac].flags |= DAC_FLAG_AMP10X;
				else
					dac_config[dac].flags &= ~DAC_FLAG_AMP10X;
			}
		}
		else
			goto einval;
	}
	else
		goto einval;

	return;

einval:
	printf(EINVAL);
	return;
}