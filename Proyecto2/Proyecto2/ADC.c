#include "ADC.h"

void ADC_Init(void)
{
	ADMUX = (1 << REFS0);

	// prescaler 128 -> 16MHz/128 = 125kHz
	ADCSRA = (1 << ADEN) |
	         (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

	DIDR0 = (1 << ADC0D) | (1 << ADC1D) | (1 << ADC2D) | (1 << ADC3D);
}

uint16_t ADC_Read(uint8_t channel)
{
	channel &= 0x07;
	ADMUX = (1 << REFS0) | channel;

	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));

	return ADC;
}

uint16_t ADC_MapToPulse(uint16_t adc_value, uint16_t min_us, uint16_t max_us)
{
	return (uint16_t)(min_us + ((uint32_t)adc_value * (max_us - min_us)) / 1023UL);
}

uint8_t ADC_MapToPWM(uint16_t adc_value)
{
	return (uint8_t)(((uint32_t)adc_value * 255UL) / 1023UL);
}

uint8_t ADC_MapToAngle(uint16_t adc_value)
{
	return (uint8_t)(((uint32_t)adc_value * 180UL) / 1023UL);
}
