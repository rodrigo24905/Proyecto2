#include "config.h"
#include "SERVO_HW.h"
#include <avr/io.h>

static uint16_t clamp_pulse(uint16_t us)
{
	if (us < SERVO_MIN_US) us = SERVO_MIN_US;
	if (us > SERVO_MAX_US) us = SERVO_MAX_US;
	return us;
}

void SERVO_HW_Init(void)
{
	DDRB |= (1 << DDB1) | (1 << DDB2);

	// TOP = ICR1, prescaler = 8
	// 16MHz/8 = 2MHz -> 0.5us
	TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM11);
	TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);

	// 20ms: 20,000us / 0.5us = 40,000 counts -> TOP 39999
	ICR1 = 39999;

	SERVO1_SetPulse_us(SERVO_CENTER_US);
	SERVO2_SetPulse_us(SERVO_CENTER_US);
}

void SERVO1_SetPulse_us(uint16_t us)
{
	us = clamp_pulse(us);
	OCR1A = us * 2;
}

void SERVO2_SetPulse_us(uint16_t us)
{
	us = clamp_pulse(us);
	OCR1B = us * 2;
}

void SERVO1_SetAngle(uint8_t angle)
{
	uint16_t us;
	if (angle > 180) angle = 180;
	us = SERVO_MIN_US + ((uint32_t)angle * (SERVO_MAX_US - SERVO_MIN_US)) / 180UL;
	SERVO1_SetPulse_us(us);
}

void SERVO2_SetAngle(uint8_t angle)
{
	uint16_t us;
	if (angle > 180) angle = 180;
	us = SERVO_MIN_US + ((uint32_t)angle * (SERVO_MAX_US - SERVO_MIN_US)) / 180UL;
	SERVO2_SetPulse_us(us);
}
