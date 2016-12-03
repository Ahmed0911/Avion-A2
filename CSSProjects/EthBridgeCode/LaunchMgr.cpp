/*
 * LaunchMgr.cpp
 *
 *  Created on: Mar 7, 2016
 *      Author: User
 */

#include "LaunchMgr.h"

void LaunchMgr::Init()
{
	WpnOutputs::Init(); //call base

	m_Timers[0] = 0;
	m_Timers[1] = 0;
	WpnState[0] = LAUNCHSTATE_IDLE;
	WpnState[1] = LAUNCHSTATE_IDLE;
}

void LaunchMgr::Arm(int index, unsigned int accessCode)
{
	if( accessCode == LAUNCHWPN_CODE && WpnState[index] != LAUNCHSTATE_FIRING )
	{
		WpnState[index] = LAUNCHSTATE_ARMED;
	}
}

void LaunchMgr::Fire(int index, unsigned int timer)
{
	if( WpnState[index] == LAUNCHSTATE_ARMED )
	{
		Set(index+1);
		WpnState[index] = LAUNCHSTATE_FIRING;
		m_Timers[index] = timer;
	}
}

void LaunchMgr::Dearm(int index, unsigned int accessCode)
{
	if( accessCode == LAUNCHWPN_CODE)
	{
		WpnState[index] = LAUNCHSTATE_IDLE;
		Reset(index+1);
	}
}

void LaunchMgr::Update()
{
	for(int index = 0; index !=2; index ++)
	{
		if( WpnState[index] == LAUNCHSTATE_FIRING )
		{
			m_Timers[index]--;
			if( m_Timers[index]  <= 0)
			{
				Reset(index+1);
				WpnState[index] = LAUNCHSTATE_ARMED;
				m_Timers[index] = 0;
			}
		}
	}
}
