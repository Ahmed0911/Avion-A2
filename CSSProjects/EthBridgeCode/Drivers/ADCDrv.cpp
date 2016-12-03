/*
 * ADCDrv.cpp
 *
 *  Created on: Nov 7, 2014
 *      Author: User
 */

#include "ADCDrv.h"
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_gpio.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/adc.h"
#include "driverlib/sysctl.h"


void ADCDrv::Init()
{
	// ADC
	// PK0 - AIN16 - Current
	// PK1 - AIN17 - Voltage
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
	GPIOPinTypeADC(GPIO_PORTK_BASE, GPIO_PIN_0 | GPIO_PIN_1); // PK0 - AIN16, PK1 - AIN17
	ADCReferenceSet(ADC0_BASE, ADC_REF_EXT_3V);

	// configure sequencers
	ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH17); // PK1 - AIN17 - Voltage
	ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_CH16); // PK0 - AIN16 - Current
	ADCSequenceStepConfigure(ADC0_BASE, 0, 2, ADC_CTL_TS | ADC_CTL_IE | ADC_CTL_END); // TEMP
	ADCSequenceEnable(ADC0_BASE, 0);

	ADCHardwareOversampleConfigure(ADC0_BASE, 64);

	ADCIntClear(ADC0_BASE, 0);
	ADCProcessorTrigger(ADC0_BASE, 0); // first conversion
}

unsigned int ADCDrv::GetValue(int channel)
{
	return m_Values[channel];
}


void ADCDrv::Update()
{
	if( ADCIntStatus(ADC0_BASE, 0, false ) == true ) // data available
	{
		// new data available
		unsigned int vals[8];
		int rd = ADCSequenceDataGet(ADC0_BASE, 0, vals);
		if( rd == 3 )
		{
			memcpy(m_Values, vals, sizeof(m_Values));
		}

		ADCProcessorTrigger(ADC0_BASE, 0); // restart conversion
	}
}

float ADCDrv::CPUTemperature()
{
	float Temperature = 147.5f - ((75.0f * 3.0f * m_Values[ADCTEMP]) / 4096.0f);

	return Temperature;
}

float ADCDrv::BATTVoltage()
{
	float voltage = (3.0f * m_Values[ADCBATTVOLT] / 4096.0f) * 11.0f; // [V]

	return voltage;
}

float ADCDrv::BATTCurrent()
{
	float voltage = (3.0f * m_Values[ADCBATTCURRENT] / 4096.0f) * 2.0f; // [V]
	float current = (voltage-2.5f)/0.066f; // sensor dependant!!!  (ACS712-30A: 66mV/A)

	return current;
}
