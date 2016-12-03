/*
 * LaunchMgr.h
 *
 *  Created on: Mar 7, 2016
 *      Author: User
 */

#ifndef LAUNCHMGR_H_
#define LAUNCHMGR_H_

#include "Drivers/WpnOutputs.h"

#define LAUNCHWPN_CODE 0x43782843

#define LAUNCHSTATE_IDLE 0
#define LAUNCHSTATE_ARMED 1
#define LAUNCHSTATE_FIRING 2

class LaunchMgr : private WpnOutputs
{
public:
	void Init();
	void Arm(int index, unsigned int accessCode);
	void Fire(int index, unsigned int timer);
	void Dearm(int index, unsigned int accessCode);
	void Update();

public:
	unsigned short WpnState[2]; // 0 - idle, 1 - arm, 2 - firing

private:
	int m_Timers[2];

};

#endif /* LAUNCHMGR_H_ */
