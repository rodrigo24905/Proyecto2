#include "config.h"
#include "MOTOR.h"
#include <avr/io.h>

static volatile uint8_t motor_speed = 0;
static volatile uint8_t motor_direction = 0; 

void MOTOR_Init(void)
{
	DDRD |= (1 << DDD3) | (1 << DDD4) | (1 << DDD5);

	TCCR2A = (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);
	TCCR2B = (1 << CS22);
	OCR2B = 0;

	MOTOR_Stop();
}

void MOTOR_SetSpeed(uint8_t speed)
{
	motor_speed = speed;
	OCR2B = speed;
}

void MOTOR_Forward(uint8_t speed)
{
	if (speed < MOTOR_MIN_ACTIVE_PWM)
	{
		MOTOR_Stop();
		return;
	}
	PORTD |= (1 << PD4);
	PORTD &= ~(1 << PD5);
	motor_direction = 1;
	MOTOR_SetSpeed(speed);
}

void MOTOR_Reverse(uint8_t speed)
{
	if (speed < MOTOR_MIN_ACTIVE_PWM)
	{
		MOTOR_Stop();
		return;
	}
	PORTD &= ~(1 << PD4);
	PORTD |= (1 << PD5);
	motor_direction = 2;
	MOTOR_SetSpeed(speed);
}

void MOTOR_Stop(void)
{
	PORTD &= ~(1 << PD4);
	PORTD &= ~(1 << PD5);
	motor_direction = 0;
	MOTOR_SetSpeed(0);
}

uint8_t MOTOR_GetSpeed(void)
{
	return motor_speed;
}

uint8_t MOTOR_GetDirection(void)
{
	return motor_direction;
}
