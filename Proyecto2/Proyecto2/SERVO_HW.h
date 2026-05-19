#ifndef SERVO_HW_H_
#define SERVO_HW_H_

#include <stdint.h>


void SERVO_HW_Init(void);
void SERVO1_SetPulse_us(uint16_t us);
void SERVO2_SetPulse_us(uint16_t us);
void SERVO1_SetAngle(uint8_t angle);
void SERVO2_SetAngle(uint8_t angle);

#endif 
