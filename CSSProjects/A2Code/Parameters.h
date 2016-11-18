/*
 * CommData.h
 *
 *  Created on: Oct 23, 2016
 *      Author: Ivan
 */

#ifndef A2CODE_PARAMETERS_H_
#define A2CODE_PARAMETERS_H_

struct SParameters
{
	float GyroOffX;
	float GyroOffY;
	float GyroOffZ;
	float MagOffX;
	float MagOffY;
	float MagOffZ;
	float AttOffRoll;
	float AttOffPitch;
	float RollMax;
	float RollKp;
	float RollKi;
	float RollKd;
	float PitchMax;
	float PitchKp;
	float PitchKi;
	float PitchKd;

	// CRC
	unsigned int CRC32;
};

#endif /* A2CODE_PARAMETERS_H_ */
