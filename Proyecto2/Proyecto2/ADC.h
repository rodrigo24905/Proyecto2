#ifndef ADC_H_
#define ADC_H_

#include <avr/io.h>
#include <stdint.h>

void ADC_Init(void);
uint16_t ADC_Read(uint8_t channel);
uint16_t ADC_MapToPulse(uint16_t adc_value, uint16_t min_us, uint16_t max_us);
uint8_t ADC_MapToPWM(uint16_t adc_value);
uint8_t ADC_MapToAngle(uint16_t adc_value);

#endif 
