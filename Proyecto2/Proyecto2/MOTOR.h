#ifndef MOTOR_H_
#define MOTOR_H_

#include <stdint.h>


void MOTOR_Init(void);
void MOTOR_Forward(uint8_t speed);
void MOTOR_Reverse(uint8_t speed);
void MOTOR_Stop(void);
void MOTOR_SetSpeed(uint8_t speed);
uint8_t MOTOR_GetSpeed(void);
uint8_t MOTOR_GetDirection(void);

#endif 
