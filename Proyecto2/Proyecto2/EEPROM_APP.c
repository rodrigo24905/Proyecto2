#include "EEPROM_APP.h"
#include <avr/eeprom.h>

#define POSE_COUNT 4
#define POSE_SIZE sizeof(CarPose_t)

static uint16_t pose_address(uint8_t index)
{
	if (index >= POSE_COUNT) index = 0;
	return (uint16_t)(index * POSE_SIZE);
}

void EEPROM_SavePose(uint8_t index, const CarPose_t *pose)
{
	eeprom_update_block((const void*)pose, (void*)pose_address(index), POSE_SIZE);
}

void EEPROM_LoadPose(uint8_t index, CarPose_t *pose)
{
	eeprom_read_block((void*)pose, (const void*)pose_address(index), POSE_SIZE);
}
