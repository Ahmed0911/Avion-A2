/*
 * ADCDrv.h
 *
 *  Created on: Nov 7, 2014
 *      Author: User
 */

#ifndef ADCDRV_H_
#define ADCDRV_H_

#define ADCBATTVOLT 0
#define ADCBATTCURRENT 1
#define ADCTEMP 2

class ADCDrv
{
private:
	unsigned int m_Values[3];

public:
	void Init();
	void Update();
	unsigned int GetValue(int channel);

	// Board specific!
	float CPUTemperature();
	float BATTCurrent();
	float BATTVoltage();
};

#endif /* ADCDRV_H_ */
