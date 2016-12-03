/*
 * PWMDrv.h
 *
 *  Created on: Jun 3, 2014
 *      Author: User
 */

#ifndef PWMDRV_H_
#define PWMDRV_H_

class PWMDrv
{
public:
	void Init();
	void SetDuty(int channel, float duty);
	void SetWidthUS(int channel, float width);
};

#endif /* PWMDRV_H_ */
