/*
 * EEPROMDrv.h
 *
 *  Created on: Oct 23, 2016
 *      Author: Ivan
 */

#ifndef DRIVERS_EEPROMDRV_H_
#define DRIVERS_EEPROMDRV_H_

#define EEPROMKEY 0xA5A58832

class EEPROMDrv
{
public:
	bool ReadParamsFromFlash(void* dataToRead, int length);
	void WriteParamsFromFlash(void* dataToWrite, int length);
};

#endif /* DRIVERS_EEPROMDRV_H_ */
