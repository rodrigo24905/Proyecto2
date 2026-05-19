#ifndef SERVO_SW_H_
#define SERVO_SW_H_

#include <stdint.h>

void SERVO_SW_Init(void);
void SERVO3_SetPulse_us(uint16_t us);
void SERVO3_SetAngle(uint8_t angle);

#endif 
