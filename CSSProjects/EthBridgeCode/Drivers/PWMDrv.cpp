/*
 * DACPWMDrv.cpp
 *
 *  Created on: Jun 3, 2014
 *      Author: User
 */

#include "PWMDrv.h"
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
#include "driverlib/pwm.h"

// 22ms PWM period
//#define PWMFREQ (1/0.022f) /* 22ms->45Hz */
#define PWMFREQ (1/0.004f) /* 4ms->250Hz */
extern uint32_t g_ui32SysClock;

void PWMDrv::Init()
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0); // PWM Module 0
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
	//SysCtlPWMClockSet(SYSCTL_PWMDIV_64); // divide by 64, max period = 80MHz/65536 = 0.052s = 52ms - TM4C123 Only
	PWMClockSet(PWM0_BASE, PWM_SYSCLK_DIV_64); // TM4C129

	GPIOPinConfigure(GPIO_PF0_M0PWM0);
	GPIOPinConfigure(GPIO_PF1_M0PWM1);
	GPIOPinConfigure(GPIO_PF2_M0PWM2);
	GPIOPinConfigure(GPIO_PF3_M0PWM3);
	GPIOPinConfigure(GPIO_PG0_M0PWM4);
	GPIOPinConfigure(GPIO_PG1_M0PWM5);

	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 |GPIO_PIN_3 );
	GPIOPinTypePWM(GPIO_PORTG_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC | PWM_GEN_MODE_DBG_RUN); // PWM3, PWM4
	PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC | PWM_GEN_MODE_DBG_RUN); // PWM2, PWM1
	PWMGenConfigure(PWM0_BASE, PWM_GEN_2, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC | PWM_GEN_MODE_DBG_RUN); // PWM6, PWM5

	// set timers period
	unsigned long ulPeriod = (g_ui32SysClock / 64) / PWMFREQ;
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, ulPeriod);
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, ulPeriod);
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, ulPeriod);

	// set default PWM duty (0%)
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_0, 0);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, 0);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 0);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3, 0);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_4, 0);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, 0);

	// enable PWM outputs to PINs
	PWMOutputState(PWM0_BASE, PWM_OUT_0_BIT, true);
	PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT, true);
	PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT, true);
	PWMOutputState(PWM0_BASE, PWM_OUT_3_BIT, true);
	PWMOutputState(PWM0_BASE, PWM_OUT_4_BIT, true);
	PWMOutputState(PWM0_BASE, PWM_OUT_5_BIT, true);

	// enable PWM modules
	PWMGenEnable(PWM0_BASE, PWM_GEN_0);
	PWMGenEnable(PWM0_BASE, PWM_GEN_1);
	PWMGenEnable(PWM0_BASE, PWM_GEN_2);
}

// NOTE: Index as position on PCB, not real PWM output numbers!
void PWMDrv::SetWidthUS(int channel, float us)
{
	unsigned long width = (us / 1000000.0f) * (g_ui32SysClock / 64);

	if( channel == 0)
	{
		PWMPulseWidthSet( PWM0_BASE, PWM_OUT_1, width);
	}
	else if( channel == 1)
	{
		PWMPulseWidthSet( PWM0_BASE, PWM_OUT_0, width);
	}
	else if( channel == 2)
	{
		PWMPulseWidthSet( PWM0_BASE, PWM_OUT_2, width);
	}
	else if( channel == 3)
	{
		PWMPulseWidthSet( PWM0_BASE, PWM_OUT_3, width);
	}
	else if( channel == 4)
	{
		PWMPulseWidthSet( PWM0_BASE, PWM_OUT_4, width);
	}
	else if( channel == 5)
	{
		PWMPulseWidthSet( PWM0_BASE, PWM_OUT_5, width);
	}
}

void PWMDrv::SetDuty(int channel, float duty)
{
	unsigned long ulPeriod = PWMGenPeriodGet(PWM0_BASE, PWM_GEN_2);
	unsigned long width = (unsigned long)(duty/100 * ulPeriod);

	if( channel == 0)
	{
		PWMPulseWidthSet( PWM0_BASE, PWM_OUT_1, width);
	}
	else if( channel == 1)
	{
		PWMPulseWidthSet( PWM0_BASE, PWM_OUT_0, width);
	}
	else if( channel == 2)
	{
		PWMPulseWidthSet( PWM0_BASE, PWM_OUT_2, width);
	}
	else if( channel == 3)
	{
		PWMPulseWidthSet( PWM0_BASE, PWM_OUT_3, width);
	}
	else if( channel == 4)
	{
		PWMPulseWidthSet( PWM0_BASE, PWM_OUT_4, width);
	}
	else if( channel == 5)
	{
		PWMPulseWidthSet( PWM0_BASE, PWM_OUT_5, width);
	}
}
