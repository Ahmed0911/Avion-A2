/*
 * LSM90DDrv.cpp
 *
 *  Created on: Jul 3, 2015
 *      Author: Sara
 */

#include "LSM90DDrv.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/fpu.h"
#include "driverlib/ssi.h"

extern uint32_t g_ui32SysClock;

#define SELECT_G()  GPIOPinWrite(GPIO_PORTH_BASE, GPIO_PIN_2, 0 )
#define DESELECT_G()  GPIOPinWrite(GPIO_PORTH_BASE, GPIO_PIN_2, GPIO_PIN_2 )
#define SELECT_XM()  GPIOPinWrite(GPIO_PORTH_BASE, GPIO_PIN_1, 0 )
#define DESELECT_XM()  GPIOPinWrite(GPIO_PORTH_BASE, GPIO_PIN_1, GPIO_PIN_1 )

// SPI
// ---------
// 10MHz max
// Freescale SPI
// SPO = 1,SPH = 1 -> FRF_MOTO_MODE_3
// SSI2
// PINS:
// PD3 - CLK
// PD1 - MOSI
// PD0 - MISO
// PH1 - CS_XM
// PH2 - CS_G
// PA0 - INT_G
// PA1 - INT_X
// PH3 - INT_M
bool LSM90DDrv::Init()
{
	m_INTGCounter = 0;
	m_INTXCounter = 0;
	m_INTMCounter = 0;

	// Enable
	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI2);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	// SPI Pins
	GPIOPinConfigure(GPIO_PD3_SSI2CLK);
	GPIOPinConfigure(GPIO_PD1_SSI2XDAT0);
	GPIOPinConfigure(GPIO_PD0_SSI2XDAT1);
	GPIOPinTypeSSI(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3);

	// Chip Select Pins - CS_G, CS_XM
	GPIOPinWrite(GPIO_PORTH_BASE, GPIO_PIN_1, GPIO_PIN_1 ); // default to 1
	GPIOPinWrite(GPIO_PORTH_BASE, GPIO_PIN_2, GPIO_PIN_2 ); // default to 1
	GPIOPinTypeGPIOOutput(GPIO_PORTH_BASE, GPIO_PIN_1);
	GPIOPinTypeGPIOOutput(GPIO_PORTH_BASE, GPIO_PIN_2);
	// INT Pins
	GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_0);
	GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_1);
	GPIOPinTypeGPIOInput(GPIO_PORTH_BASE, GPIO_PIN_3);
	// Set AS INT Source!!!
	GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_0);
	GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_1);
	GPIOIntEnable(GPIO_PORTH_BASE, GPIO_PIN_3);
	GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_0, GPIO_FALLING_EDGE);
	GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_1, GPIO_FALLING_EDGE);
	GPIOIntTypeSet(GPIO_PORTH_BASE, GPIO_PIN_3, GPIO_FALLING_EDGE);
	IntEnable(INT_GPIOA);
	IntEnable(INT_GPIOH);

	// Configure
	SSIConfigSetExpClk(SSI2_BASE, g_ui32SysClock, SSI_FRF_MOTO_MODE_3, SSI_MODE_MASTER, 10000000, 8);
	// Enable
	SSIEnable(SSI2_BASE);

	// Clear fifo
	ClearFIFO();

	// check IDs
	unsigned char ID = ReadReg(LSM9DS0_WHO_AM_I_G, false);
	if( ID != LSM9DS0_G_ID) return false;

	ID = ReadReg(LSM9DS0_WHO_AM_I_XM, true);
	if( ID != LSM9DS0_XM_ID) return false;

	// GYRO
	WriteReg(LSM9DS0_CTRL_REG1_G, LSM9DS0_GYRO_ODR_BW_190_70 ); // Set Gyro ODR
	WriteReg(LSM9DS0_CTRL_REG4_G, LSM9DS0_GYRO_245DPS); // Set Gyro Range
	//WriteReg(LSM9DS0_CTRL_REG3_G, LSM9DS0_GYRO_I2_DRDY); // Enable Gyro INT
	// TODO: NE RADI INT, NESTO S PINOM NE VALJA!!!

	// ACCEL+MAG
	WriteReg(LSM9DS0_CTRL_REG1_XM, LSM9DS0_ACC_ODR_100, true ); // Set Accel ODR
	WriteReg(LSM9DS0_CTRL_REG2_XM, LSM9DS0_ACCEL_2G, true ); // Set Accel Range
	WriteReg(LSM9DS0_CTRL_REG5_XM, LSM9DS0_MAG_ODR_100, true ); // Set Mag ODR + Enable Temp Sensor
	WriteReg(LSM9DS0_CTRL_REG6_XM, LSM9DS0_MAG_2GAUSS, true ); // Set Mag Range
	WriteReg(LSM9DS0_CTRL_REG7_XM, 0x00, true); // Continuos Mag
	//WriteReg(LSM9DS0_CTRL_REG3_XM, 0x04, true ); // Set Acc INT on INT1
	//WriteReg(LSM9DS0_CTRL_REG4_XM, 0x04, true ); // Set Mag INT on INT2


	return true;
}

void LSM90DDrv::Update()
{
	// make local copy
	IntMasterDisable();
	unsigned char data[30];
	memcpy(data, Data, 30);
	IntMasterEnable();

	// Test READ
	unsigned int value = 0;
	ReadBytes(LSM9DS0_OUT_X_L_G, data, 6, false);
	value = (data[1] << 8) + data[0];
	Gyro[0] = (short)value * 0.00875f; // [°/s]
	value = (data[3] << 8) + data[2];
	Gyro[1] = (short)value * 0.00875f; // [°/s]
	value = (data[5] << 8) + data[4];
	Gyro[2] = (short)value * 0.00875f; // [°/s]


	ReadBytes(LSM9DS0_OUT_X_L_A, data, 6, true);
	value = (data[1] << 8) + data[0];
	Accel[0] = (short)value * 0.061f * 0.00981f; // [g]
	value = (data[3] << 8) + data[2];
	Accel[1] = (short)value * 0.061f * 0.00981f; // [g]
	value = (data[5] << 8) + data[4];
	Accel[2] = (short)value * 0.061f * 0.00981f; // [g]

	ReadBytes(LSM9DS0_OUT_X_L_M, data, 6, true);
	value = (data[1] << 8) + data[0];
	Mag[0] = (short)value * 0.008f; // [uT]
	value = (data[3] << 8) + data[2];
	Mag[1] = (short)value * 0.008f; // [uT]
	value = (data[5] << 8) + data[4];
	Mag[2] = (short)value * 0.008f; // [uT]

	ReadBytes(LSM9DS0_TEMP_OUT_L_XM, data, 2, true);
	value = (data[1] << 8) + data[0];
	Temperature = (short)value * 0.125f + 21; // [°C]
}

void LSM90DDrv::MotionINTG(void)
{
	unsigned int ulStatus = GPIOIntStatus(GPIO_PORTA_BASE, true);

	if(ulStatus & GPIO_PIN_0)
	{
		GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_0); // clear INT_G

		//ReadBytes(LSM9DS0_OUT_X_L_G, Data, 8, false);
		m_INTGCounter++;
	}
}
void LSM90DDrv::MotionINTX(void)
{
	unsigned int ulStatus = GPIOIntStatus(GPIO_PORTA_BASE, true);

	if(ulStatus & GPIO_PIN_1)
	{
		GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_1); // clear INT_X

		//ReadBytes(LSM9DS0_OUT_X_L_A, Data, 6, true);
		m_INTXCounter++;
	}
}
void LSM90DDrv::MotionINTM(void)
{
	unsigned int ulStatus = GPIOIntStatus(GPIO_PORTH_BASE, true);

	if(ulStatus & GPIO_PIN_3)
	{
		GPIOIntClear(GPIO_PORTH_BASE, GPIO_PIN_3); // clear INT_M

		//ReadBytes(LSM9DS0_OUT_X_L_M, Data, 6, true);
		m_INTMCounter++;
	}
}

void LSM90DDrv::ClearFIFO()
{
	// Clear FIFO
	uint32_t data;
	while(SSIDataGetNonBlocking(SSI2_BASE, &data));
}

// FIFO is 8 words wide!
void LSM90DDrv::ReadBytes(unsigned char address, unsigned char* buffer, int count, bool accessXS)
{
	uint32_t data;
	if( accessXS ) SELECT_XM();
	else SELECT_G();

	SSIDataPut(SSI2_BASE, address + 0x80 + 0x40); // 0x80 - READ FLAG, 0x40 - Auto Addr. Increment
	while(SSIBusy(SSI2_BASE));
	SSIDataGet(SSI2_BASE, &data); // dummy

	for(int i=0; i!=count; i++)
	{
		SSIDataPut(SSI2_BASE, 0x00); // READ
		while(SSIBusy(SSI2_BASE));
		SSIDataGet(SSI2_BASE, &data);
		buffer[i] = (data&0x00FF);
	}

	if( accessXS ) DESELECT_XM();
	else DESELECT_G();
}

void LSM90DDrv::WriteBytes(unsigned char address, unsigned char* buffer, int count, bool accessXS)
{
	uint32_t data;
	if( accessXS ) SELECT_XM();
	else SELECT_G();

	SSIDataPut(SSI2_BASE, address + 0x40); // 0x40 - Auto Addr. Increment
	while(SSIBusy(SSI2_BASE));
	SSIDataGet(SSI2_BASE, &data); // dummy

	for(int i=0; i!=count; i++)
	{
		SSIDataPut(SSI2_BASE, buffer[i]); // WRITE
		while(SSIBusy(SSI2_BASE));
		SSIDataGet(SSI2_BASE, &data);
	}

	if( accessXS ) DESELECT_XM();
	else DESELECT_G();
}

// helpers
unsigned char LSM90DDrv::ReadReg(unsigned char address, bool accessXS)
{
	unsigned char data = 0;
	ReadBytes(address, &data, 1, accessXS);

	return data;
}

void LSM90DDrv::WriteReg(unsigned char address, unsigned char value, bool accessXS)
{
	WriteBytes(address, &value, 1, accessXS);
}
