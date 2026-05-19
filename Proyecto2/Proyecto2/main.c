/*
 * Proyecto2.c
 *
 * Created: 19/05/2026
 * Author: Juan Rodrigo Donis Alvizures
 * Description: Proyecto 2 - Carro, 3 servos + Dc, tracción, dirección y puertas
 */
/****************************************/

#define F_CPU 16000000UL

#include "config.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ADC.h"
#include "UART.h"
#include "SERVO_HW.h"
#include "SERVO_SW.h"
#include "MOTOR.h"
#include "EEPROM_APP.h"

#define LED_MODE PB5

#define MODE_MANUAL 0
#define MODE_EEPROM 1
#define MODE_UART   2

static uint8_t current_mode = MODE_MANUAL;

static uint16_t current_dir_us = DIR_CENTER_US;
static uint16_t current_left_us = LEFT_DOOR_CLOSE_US;
static uint16_t current_right_us = RIGHT_DOOR_CLOSE_US;

static char rx_buffer[32];
static uint8_t rx_index = 0;

void IO_Init(void);
void LED_Update(void);
void ApplyPose(const CarPose_t *pose);
void BuildCurrentPose(CarPose_t *pose);
void Manual_Update(void);
void PrintHelp(void);
void PrintPots(void);
void ProcessSerial(void);
void ExecuteCommand(char *cmd);
void PlayEEPROM(void);
void SequenceDemo(void);
void OpenDoors(void);
void CloseDoors(void);
uint16_t AngleToPulse(uint8_t angle);
uint16_t MapADCToPulseRange(uint16_t adc_value, uint16_t start_us, uint16_t end_us);
uint8_t ClampAngle(uint8_t value, uint8_t limit_a, uint8_t limit_b);
uint8_t ParseNumberAfterComma(char *cmd, uint8_t default_value);

int main(void)
{
	IO_Init();
	ADC_Init();
	UART_Init(UART_BAUD);
	SERVO_HW_Init();
	SERVO_SW_Init();
	MOTOR_Init();
	sei();

	UART_Println("Firmware final - carro IE2023");
	UART_Println("Servos D9/D10 por Timer1 + servo D6 por software PWM");
	UART_Println("Escriba HELP para comandos.");

	while (1)
	{
		ProcessSerial();

		if (current_mode == MODE_MANUAL)
		{
			Manual_Update();
		}

		LED_Update();
		_delay_ms(20);
	}
}

void IO_Init(void)
{
	DDRB |= (1 << LED_MODE);
	PORTB &= ~(1 << LED_MODE);
}

void LED_Update(void)
{
	static uint16_t counter = 0;
	counter++;

	if (current_mode == MODE_MANUAL)
	{
		PORTB |= (1 << LED_MODE);
	}
	else if (current_mode == MODE_EEPROM)
	{
		if (counter >= 25)
		{
			PORTB ^= (1 << LED_MODE);
			counter = 0;
		}
	}
	else // MODE_UART
	{
		if (counter >= 8)
		{
			PORTB ^= (1 << LED_MODE);
			counter = 0;
		}
	}
}

void ApplyPose(const CarPose_t *pose)
{
	current_dir_us = pose->dir_us;
	current_left_us = pose->left_us;
	current_right_us = pose->right_us;

	SERVO1_SetPulse_us(current_dir_us);   // D9: direccion
	SERVO2_SetPulse_us(current_left_us);  // D10: puerta izquierda
	SERVO3_SetPulse_us(current_right_us); // D6: puerta derecha

	if (pose->motor_dir == 1)
	{
		MOTOR_Forward(pose->motor_speed);
	}
	else if (pose->motor_dir == 2)
	{
		MOTOR_Reverse(pose->motor_speed);
	}
	else
	{
		MOTOR_Stop();
	}
}

void BuildCurrentPose(CarPose_t *pose)
{
	pose->motor_speed = MOTOR_GetSpeed();
	pose->motor_dir = MOTOR_GetDirection();
	pose->dir_us = current_dir_us;
	pose->left_us = current_left_us;
	pose->right_us = current_right_us;
}

void Manual_Update(void)
{
	uint16_t adc_motor = ADC_Read(0); // A0
	uint16_t adc_dir   = ADC_Read(1); // A1
	uint16_t adc_left  = ADC_Read(2); // A2
	uint16_t adc_right = ADC_Read(3); // A3

	uint8_t speed = ADC_MapToPWM(adc_motor);

	// Mapeo calibrado:
	// Direccion: 10 a 140, centro 75
	// Puerta izquierda: 180 cerrada a 150 abierta
	// Puerta derecha: 0 cerrada a 30 abierta
	current_dir_us = MapADCToPulseRange(adc_dir, DIR_MIN_US, DIR_MAX_US);
	current_left_us = MapADCToPulseRange(adc_left, LEFT_DOOR_CLOSE_US, LEFT_DOOR_OPEN_US);
	current_right_us = MapADCToPulseRange(adc_right, RIGHT_DOOR_CLOSE_US, RIGHT_DOOR_OPEN_US);

	SERVO1_SetPulse_us(current_dir_us);
	SERVO2_SetPulse_us(current_left_us);
	SERVO3_SetPulse_us(current_right_us);

	if (speed < MOTOR_MIN_ACTIVE_PWM)
	{
		MOTOR_Stop();
	}
	else
	{
		MOTOR_Forward(speed);
	}
}

void PrintHelp(void)
{
	UART_Println("Comandos disponibles:");
	UART_Println("M / MANUAL    -> modo manual con pots");
	UART_Println("U / UART      -> modo UART/Adafruit");
	UART_Println("E / EEPROM    -> modo EEPROM");
	UART_Println("POTS          -> imprimir ADC");
	UART_Println("SAVE0..SAVE3  -> guardar pose actual");
	UART_Println("PLAY          -> reproducir EEPROM");
	UART_Println("SEQ1          -> secuencia demo");
	UART_Println("OPEN / CLOSE  -> abrir/cerrar puertas");
	UART_Println("F,vel         -> avanzar vel 0-255");
	UART_Println("B,vel         -> retroceder vel 0-255");
	UART_Println("STOP          -> detener motor");
	UART_Println("DIR,ang       -> direccion 10-140, centro 75");
	UART_Println("LDOOR,ang     -> puerta izq 150-180");
	UART_Println("RDOOR,ang     -> puerta der 0-30");
}

void PrintPots(void)
{
	UART_Print("A0 motor: "); UART_PrintUInt16(ADC_Read(0)); UART_Print("\r\n");
	UART_Print("A1 dir: "); UART_PrintUInt16(ADC_Read(1)); UART_Print("\r\n");
	UART_Print("A2 puerta izq: "); UART_PrintUInt16(ADC_Read(2)); UART_Print("\r\n");
	UART_Print("A3 puerta der: "); UART_PrintUInt16(ADC_Read(3)); UART_Print("\r\n");
}

void ProcessSerial(void)
{
	
	static uint8_t idle_ticks = 0;
	uint8_t received = 0;

	while (UART_Available())
	{
		char c = UART_RxChar();
		received = 1;
		idle_ticks = 0;

		
		if (c == 8 || c == 127)
		{
			if (rx_index > 0)
			{
				rx_index--;
				UART_Print("\b \b");
			}
		}
		else if (c == '\r' || c == '\n')
		{
			UART_Print("\r\n");
			if (rx_index > 0)
			{
				rx_buffer[rx_index] = '\0';
				ExecuteCommand(rx_buffer);
				rx_index = 0;
			}
		}
		else
		{
			// Convertir a mayuscula para aceptar help, Help, HELP
			if (c >= 'a' && c <= 'z')
			{
				c = c - 32;
			}

			if (rx_index < (sizeof(rx_buffer) - 1))
			{
				rx_buffer[rx_index++] = c;
				UART_TxChar(c); // eco visual
			}
		}
	}

	// Si no llega CR/LF, ejecutar despues de aprox. 300 ms sin nuevos caracteres.
	if (!received && rx_index > 0)
	{
		idle_ticks++;
		if (idle_ticks >= 15) // 15 * 20 ms aprox. por el delay del main
		{
			UART_Print("\r\n");
			rx_buffer[rx_index] = '\0';
			ExecuteCommand(rx_buffer);
			rx_index = 0;
			idle_ticks = 0;
		}
	}
}

void ExecuteCommand(char *cmd)
{
	CarPose_t pose;
	uint8_t value;

	if (!strcmp(cmd, "HELP"))
	{
		PrintHelp();
	}
	else if (!strcmp(cmd, "M") || !strcmp(cmd, "MANUAL"))
	{
		current_mode = MODE_MANUAL;
		UART_Println("Modo MANUAL");
	}
	else if (!strcmp(cmd, "U") || !strcmp(cmd, "UART") || !strcmp(cmd, "A"))
	{
		current_mode = MODE_UART;
		UART_Println("Modo UART/Adafruit");
	}
	else if (!strcmp(cmd, "E") || !strcmp(cmd, "EEPROM"))
	{
		current_mode = MODE_EEPROM;
		UART_Println("Modo EEPROM");
	}
	else if (!strcmp(cmd, "POTS"))
	{
		PrintPots();
	}
	else if (!strncmp(cmd, "SAVE", 4) && cmd[4] >= '0' && cmd[4] <= '3')
	{
		BuildCurrentPose(&pose);
		EEPROM_SavePose((uint8_t)(cmd[4] - '0'), &pose);
		UART_Print("Pose guardada en slot "); UART_TxChar(cmd[4]); UART_Print("\r\n");
	}
	else if (!strcmp(cmd, "PLAY"))
	{
		current_mode = MODE_EEPROM;
		PlayEEPROM();
	}
	else if (!strcmp(cmd, "SEQ1"))
	{
		current_mode = MODE_UART;
		SequenceDemo();
	}
	else if (!strcmp(cmd, "OPEN"))
	{
		current_mode = MODE_UART;
		OpenDoors();
	}
	else if (!strcmp(cmd, "CLOSE"))
	{
		current_mode = MODE_UART;
		CloseDoors();
	}
	else if (!strncmp(cmd, "F,", 2))
	{
		current_mode = MODE_UART;
		value = ParseNumberAfterComma(cmd, 160);
		MOTOR_Forward(value);
		UART_Println("Motor forward");
	}
	else if (!strncmp(cmd, "B,", 2))
	{
		current_mode = MODE_UART;
		value = ParseNumberAfterComma(cmd, 160);
		MOTOR_Reverse(value);
		UART_Println("Motor reverse");
	}
	else if (!strcmp(cmd, "STOP"))
	{
		current_mode = MODE_UART;
		MOTOR_Stop();
		UART_Println("Motor stop");
	}
	else if (!strncmp(cmd, "DIR,", 4))
	{
		current_mode = MODE_UART;
		value = ParseNumberAfterComma(cmd, DIR_CENTER_ANGLE);
		value = ClampAngle(value, DIR_MIN_ANGLE, DIR_MAX_ANGLE);
		current_dir_us = AngleToPulse(value);
		SERVO1_SetPulse_us(current_dir_us);
		UART_Println("Direccion actualizada");
	}
	else if (!strncmp(cmd, "LDOOR,", 6))
	{
		current_mode = MODE_UART;
		value = ParseNumberAfterComma(cmd, LEFT_DOOR_CLOSE_ANGLE);
		value = ClampAngle(value, LEFT_DOOR_OPEN_ANGLE, LEFT_DOOR_CLOSE_ANGLE);
		current_left_us = AngleToPulse(value);
		SERVO2_SetPulse_us(current_left_us);
		UART_Println("Puerta izquierda actualizada");
	}
	else if (!strncmp(cmd, "RDOOR,", 6))
	{
		current_mode = MODE_UART;
		value = ParseNumberAfterComma(cmd, RIGHT_DOOR_CLOSE_ANGLE);
		value = ClampAngle(value, RIGHT_DOOR_CLOSE_ANGLE, RIGHT_DOOR_OPEN_ANGLE);
		current_right_us = AngleToPulse(value);
		SERVO3_SetPulse_us(current_right_us);
		UART_Println("Puerta derecha actualizada");
	}
	else
	{
		UART_Println("Comando no valido. Use HELP.");
	}
}

void PlayEEPROM(void)
{
	CarPose_t pose;
	uint8_t i, j;

	UART_Println("Reproduciendo 4 poses EEPROM...");
	for (i = 0; i < 4; i++)
	{
		EEPROM_LoadPose(i, &pose);
		ApplyPose(&pose);
		UART_Print("Pose "); UART_PrintUInt8(i); UART_Print("\r\n");
		for (j = 0; j < 50; j++) _delay_ms(20);
	}
	MOTOR_Stop();
	UART_Println("Fin PLAY");
}

void SequenceDemo(void)
{
	uint8_t j;
	UART_Println("Secuencia demo");

	CloseDoors();
	for (j = 0; j < 30; j++) _delay_ms(20);

	OpenDoors();
	for (j = 0; j < 40; j++) _delay_ms(20);

	CloseDoors();
	for (j = 0; j < 40; j++) _delay_ms(20);

	current_dir_us = DIR_MIN_US;
	SERVO1_SetPulse_us(current_dir_us);
	for (j = 0; j < 30; j++) _delay_ms(20);

	current_dir_us = DIR_MAX_US;
	SERVO1_SetPulse_us(current_dir_us);
	for (j = 0; j < 30; j++) _delay_ms(20);

	SERVO1_SetPulse_us(DIR_CENTER_US);
	current_dir_us = DIR_CENTER_US;
	MOTOR_Forward(150);
	for (j = 0; j < 60; j++) _delay_ms(20);
	MOTOR_Stop();

	UART_Println("Fin SEQ1");
}

void OpenDoors(void)
{
	current_left_us = LEFT_DOOR_OPEN_US;
	current_right_us = RIGHT_DOOR_OPEN_US;
	SERVO2_SetPulse_us(current_left_us);
	SERVO3_SetPulse_us(current_right_us);
	UART_Println("Puertas abiertas");
}

void CloseDoors(void)
{
	current_left_us = LEFT_DOOR_CLOSE_US;
	current_right_us = RIGHT_DOOR_CLOSE_US;
	SERVO2_SetPulse_us(current_left_us);
	SERVO3_SetPulse_us(current_right_us);
	UART_Println("Puertas cerradas");
}

uint16_t AngleToPulse(uint8_t angle)
{
	if (angle > 180) angle = 180;
	return (uint16_t)(SERVO_MIN_US + ((uint32_t)angle * (SERVO_MAX_US - SERVO_MIN_US)) / 180UL);
}

uint16_t MapADCToPulseRange(uint16_t adc_value, uint16_t start_us, uint16_t end_us)
{
	if (start_us <= end_us)
	{
		return (uint16_t)(start_us + ((uint32_t)adc_value * (end_us - start_us)) / 1023UL);
	}
	else
	{
		return (uint16_t)(start_us - ((uint32_t)adc_value * (start_us - end_us)) / 1023UL);
	}
}

uint8_t ClampAngle(uint8_t value, uint8_t limit_a, uint8_t limit_b)
{
	uint8_t min_value;
	uint8_t max_value;

	if (limit_a < limit_b)
	{
		min_value = limit_a;
		max_value = limit_b;
	}
	else
	{
		min_value = limit_b;
		max_value = limit_a;
	}

	if (value < min_value) return min_value;
	if (value > max_value) return max_value;
	return value;
}

uint8_t ParseNumberAfterComma(char *cmd, uint8_t default_value)
{
	char *comma = strchr(cmd, ',');
	int value;

	if (comma == 0) return default_value;
	value = atoi(comma + 1);

	if (value < 0) value = 0;
	if (value > 255) value = 255;

	return (uint8_t)value;
}
