/*
 * EEPROMDrv.cpp
 *
 *  Created on: Oct 23, 2016
 *      Author: Ivan
 */

#include "EEPROMDrv.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/eeprom.h"

// Read Flash Data
bool EEPROMDrv::ReadParamsFromFlash(void* dataToRead, int length)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);
	if( EEPROMInit() == EEPROM_INIT_OK )
	{
		unsigned int key;
		EEPROMRead((uint32_t*)&key, 0x0000, sizeof(unsigned int) );
		if( key == EEPROMKEY )
		{
			// eeprom data ok, load config
			EEPROMRead((uint32_t*)dataToRead, sizeof(unsigned int), length );

			return true;
		}
	}

	return false; //read fail
}

void EEPROMDrv::WriteParamsFromFlash(void* dataToWrite, int length)
{
	// Store to flash (and restart?)
	unsigned int key = EEPROMKEY;

	EEPROMProgram((uint32_t*)&key, 0x0000, sizeof(unsigned int) );
	EEPROMProgram((uint32_t*)dataToWrite, sizeof(unsigned int), length );
}
