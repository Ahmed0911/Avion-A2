/*
 * IMU.h
 *
 *  Created on: Jan 7, 2016
 *      Author: User
 */

#ifndef DRIVERS_IMU_H_
#define DRIVERS_IMU_H_

#include "sensorlib/comp_dcm.h"

class IMU
{
public:
	void Init(void);
	void Update(float accX, float accY, float accZ, float gyroX, float gyroY, float gyroZ, float magX, float magY, float magZ);
	float Roll, Pitch, Yaw;

private:
	tCompDCM m_sCompDCMInst;
	bool m_FirstData;
};

#endif /* DRIVERS_IMU_H_ */
