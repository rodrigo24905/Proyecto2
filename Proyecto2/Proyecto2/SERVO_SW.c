#include "config.h"
#include "SERVO_SW.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define SERVO3_PIN PD6

static volatile uint8_t servo3_pulse_ticks = SERVO_CENTER_US / SOFT_SERVO_TICK_US;

static uint16_t clamp_pulse(uint16_t us)
{
	if (us < SERVO_MIN_US) us = SERVO_MIN_US;
	if (us > SERVO_MAX_US) us = SERVO_MAX_US;
	return us;
}

void SERVO_SW_Init(void)
{
	DDRD |= (1 << SERVO3_PIN);
	PORTD &= ~(1 << SERVO3_PIN);

	// Timer0 CTC, 20us
	// 16MHz / 8 = 2MHz, tick = 0.5us
	// OCR0A = 39 -> 40 counts -> 20us
	TCCR0A = (1 << WGM01);
	TCCR0B = (1 << CS01);
	OCR0A = 39;
	TIMSK0 |= (1 << OCIE0A);

	SERVO3_SetPulse_us(SERVO_CENTER_US);
}

void SERVO3_SetPulse_us(uint16_t us)
{
	us = clamp_pulse(us);
	servo3_pulse_ticks = (uint8_t)(us / SOFT_SERVO_TICK_US);
}

void SERVO3_SetAngle(uint8_t angle)
{
	uint16_t us;
	if (angle > 180) angle = 180;
	us = SERVO_MIN_US + ((uint32_t)angle * (SERVO_MAX_US - SERVO_MIN_US)) / 180UL;
	SERVO3_SetPulse_us(us);
}

ISR(TIMER0_COMPA_vect)
{
	static uint16_t frame_tick = 0;

	if (frame_tick == 0)
	{
		PORTD |= (1 << SERVO3_PIN);
	}

	frame_tick++;

	if (frame_tick >= servo3_pulse_ticks)
	{
		PORTD &= ~(1 << SERVO3_PIN);
	}

	if (frame_tick >= SOFT_SERVO_FRAME_TICKS)
	{
		frame_tick = 0;
	}
}
