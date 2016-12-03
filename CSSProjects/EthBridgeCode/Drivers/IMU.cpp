/*
 * IMU.cpp
 *
 *  Created on: Jan 7, 2016
 *      Author: User
 */

#include "IMU.h"

void IMU::Init(void)
{
	m_FirstData = true; // first update is DCM init!

	// Initialize the DCM system. 100 hz sample rate, accel weight = .01, gyro weight = .98, mag weight = .01
	CompDCMInit(&m_sCompDCMInst, 1.0f / 100.0f, 0.001f, 0.998f, 0.001f); // TODO: MODIFY!!!!
}

// acc - [g]
// gyro - [°/s]
// mag - [uT]
void IMU::Update(float accX, float accY, float accZ, float gyroX, float gyroY, float gyroZ, float magX, float magY, float magZ)
{
	// convert acc [m/s^2 to [g]
	//accX *= 0.10193679918f;
	//accY *= 0.10193679918f;
	//accZ *= 0.10193679918f;

	// convert gyro [°/s] to [rad/s]
	gyroX *= 0.017453292f;
	gyroY *= 0.017453292f;
	gyroZ *= 0.017453292f;

	// convert mag [uT] to [T]
	magX *= 1e-6f;
	magY *= 1e-6f;
	magZ *= 1e-6f;

	// Check if this is our first data ever.
	if(m_FirstData == true)
	{
		// Set flag indicating that DCM is started.
		m_FirstData = false;

		CompDCMMagnetoUpdate(&m_sCompDCMInst, magX, magY, magZ);
		CompDCMAccelUpdate(&m_sCompDCMInst, accX, accY, accZ);
		CompDCMGyroUpdate(&m_sCompDCMInst, -gyroX, -gyroY, -gyroZ);

		CompDCMStart(&m_sCompDCMInst);
	}
	else
	{
		// DCM Is already started.  Perform the incremental update.
		CompDCMMagnetoUpdate(&m_sCompDCMInst, magX, magY, magZ);
		CompDCMAccelUpdate(&m_sCompDCMInst, accX, accY, accZ);
		CompDCMGyroUpdate(&m_sCompDCMInst, -gyroX, -gyroY, -gyroZ);

		CompDCMUpdate(&m_sCompDCMInst);
	}

	CompDCMComputeEulers(&m_sCompDCMInst, &Roll, &Pitch, &Yaw);
	// Convert to degrees
	Roll *= 57.295779513082320876798154814105f;
	Pitch *= 57.295779513082320876798154814105f;
	Yaw *= 57.295779513082320876798154814105f;
	if(Yaw < 0)
	{
		Yaw += 360.0f;
	}
}
