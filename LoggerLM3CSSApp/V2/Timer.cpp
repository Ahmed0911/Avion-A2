#include "Timer.h"
#include <inc/lm3s9d90.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <driverlib/debug.h>
#include <driverlib/pin_map.h>
#include <driverlib/rom.h>
#include <driverlib/sysctl.h>
#include <driverlib/interrupt.h>
#include <driverlib/timer.h>


void Timer::Init()
{
	// configure periodic timer
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, (TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PWM | TIMER_CFG_B_PERIODIC )); // XXX-> change for other projects!!!
	TimerPrescaleSet(TIMER0_BASE,TIMER_B, 100); // divide 80MHz by 1000
	TimerEnable(TIMER0_BASE, TIMER_BOTH); // A is used in PumpDriver

	LastCount = 0;
}

void Timer::Start()
{
	unsigned short count = (unsigned short)TimerValueGet(TIMER0_BASE, TIMER_B);
	LastCount = count;
}

//return elapsed time in msec
float Timer::GetMS()
{
	unsigned short newCount = (unsigned short)TimerValueGet(TIMER0_BASE, TIMER_B);
	unsigned short delta = LastCount - newCount; // down counter->inverse

	float time = 100.0f*delta/80000000.0f * 1000; // MS [MAX ~80ms]!

	return time;
}

//return elapsed time in msec
float Timer::GetMSAndReset()
{
	float time = GetMS();
	Start(); // reset

	return time;
}
