#pragma once

enum adc
{
	ADC0 = 0,
	ADC1,
	ADC_COUNT
};

int adc_init();
void adc_print_value(enum adc, uint16_t raw_value);
void adc_command(int adc);
