#include <freertos/FreeRTOS.h>
#include <nvs_flash.h>

#include <string.h>

#include "errors.h"
#include "listener.h"

enum
{
	PARAMETER_ADC0_MIN = 0,
	PARAMETER_ADC0_MAX,
	PARAMETER_ADC1_MIN,
	PARAMETER_ADC1_MAX,
	PARAMETER_COUNT
};

const char * parameter_names[PARAMETER_COUNT] =
{
	"adc0_min",
	"adc0_max",
	"adc1_min",
	"adc1_max"
};

/*******************************************************************************
 *
 ******************************************************************************/
int read_parameter_value(const char *parameter, uint32_t *value)
{
	esp_err_t err;
	nvs_handle_t nvs_handle;
	int found = 0;

	for(int i = 0; i < PARAMETER_COUNT; i++)
	{
		if(strcmp(parameter_names[i], parameter) == 0)
		{
			found = 1;

			err = nvs_open("SWT21 Lab kit", NVS_READWRITE, &nvs_handle);
			if(err)
				goto open_err;

			err = nvs_get_u32(nvs_handle, parameter, value);
			if(err)
				goto get_err;

			nvs_close(nvs_handle);

			break;
		}
	}

	if(!found)
	{
		printf(ENOPARAM);
		return -1;
	}

	return 0;

get_err:
	nvs_close(nvs_handle);

open_err:
	printf("ERR Could not read parameter\n");
	return -1;

}

/*******************************************************************************
 *
 ******************************************************************************/
int write_parameter_value(const char *parameter, uint32_t value)
{
	esp_err_t err;
	nvs_handle_t nvs_handle;
	int found = 0;

	for(int i = 0; i < PARAMETER_COUNT; i++)
	{
		if(strcmp(parameter_names[i], parameter) == 0)
		{
			found = 1;

			err = nvs_open("SWT21 Lab kit", NVS_READWRITE, &nvs_handle);
			if(err)
				goto open_err;

			err = nvs_set_u32(nvs_handle, parameter, value);
			if(err)
				goto set_err;

			err = nvs_commit(nvs_handle);
			if(err)
				goto set_err;

			nvs_close(nvs_handle);

			break;
		}
	}

	if(!found)
	{
		printf(ENOPARAM);
		return -1;
	}

	return 0;

set_err:
	nvs_close(nvs_handle);

open_err:
	printf("ERR Could not read parameter\n");
	return -1;
}

/*******************************************************************************
 *
 ******************************************************************************/
void calibration_command()
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
			"calibration list - print out all calibration parameters\n"
			"calibration write <parameter> <value> - write value to parameter\n"
			"calibration read <parameter> - print out current parameter value\n");
	}
	else if(strcmp(cmd, "list") == 0)
	{
		printf("OK %d\n", PARAMETER_COUNT);

		for(int i = 0; i < PARAMETER_COUNT; i++)
		{
			const char *parameter = parameter_names[i];
			uint32_t value;

			if(read_parameter_value(parameter, &value) < 0)
				printf("%s Invalid\n", parameter);
			else
				printf("%s %u\n", parameter, value);
		}
	}
	else if(strcmp(cmd, "write") == 0)
	{
		char *parameter;
		char *value_str;
		uint32_t value;

		/* Read parameter argument */
		parameter = strtok(NULL, " ");
		if(!parameter)
			goto einval;

		/* Read value argument */
		value_str = strtok(NULL, " ");

		if(value_str)
			value = strtoul(value_str, NULL, 10);
		else
			goto einval;

		/* Send command */
		if(write_parameter_value(parameter, value) < 0)
			return;

		printf("OK\n");
	}
	else if(strcmp(cmd, "read") == 0)
	{
		char *parameter;

		/* Read period argument */
		parameter = strtok(NULL, " ");
		if(!parameter)
			goto einval;

		/* Read parameter value */
		uint32_t value;
		if(read_parameter_value(parameter, &value) < 0)
			return;

		printf("OK %d\n", value);
	}
	else
		goto einval;

	return;

einval:
	printf(EINVAL);
	return;
}
