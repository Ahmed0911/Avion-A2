/*
 * Timer.cpp
 *
 *  Created on: Jul 3, 2014
 *      Author: User
 */

#include "Timer.h"
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/fpu.h"
#include "driverlib/timer.h"

extern uint32_t g_ui32SysClock;
unsigned int Timer::CountsPerSec = 120000000;

void Timer::Init()
{
	// User Timer 7, Full length
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER7);
	TimerConfigure(TIMER7_BASE, TIMER_CFG_PERIODIC_UP);
	TimerEnable(TIMER7_BASE, TIMER_A);

	CountsPerSec = g_ui32SysClock;
}

void Timer::Start()
{
	unsigned int count = TimerValueGet(TIMER7_BASE, TIMER_A);
	LastCount = count;
}

//return elapsed time in msec
float Timer::GetUS()
{
	unsigned int newCount = TimerValueGet(TIMER7_BASE, TIMER_A);
	unsigned int delta = newCount - LastCount; // up counter

	float time = 1000000.0f * (float)delta/CountsPerSec; // [us]

	return time;
}
