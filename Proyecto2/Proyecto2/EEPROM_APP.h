#ifndef EEPROM_APP_H_
#define EEPROM_APP_H_

#include <stdint.h>

typedef struct
{
	uint8_t motor_speed;
	uint8_t motor_dir;
	uint16_t dir_us;
	uint16_t left_us;
	uint16_t right_us;
} CarPose_t;

void EEPROM_SavePose(uint8_t index, const CarPose_t *pose);
void EEPROM_LoadPose(uint8_t index, CarPose_t *pose);

#endif 
