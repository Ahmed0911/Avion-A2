
#include "MPU6000Drv.h"
#include <inc/lm3s9d90.h>
#include <inc/hw_ints.h>
#include <inc/hw_ssi.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <driverlib/ssi.h>
#include <driverlib/gpio.h>
#include <driverlib/sysctl.h>
#include <stdio.h>
#include <string.h>

bool MPU6000Drv::Init()
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);

	GPIOPinConfigure(GPIO_PE0_SSI1CLK);
	GPIOPinConfigure(GPIO_PE1_SSI1FSS);
	GPIOPinConfigure(GPIO_PH6_SSI1RX);
	GPIOPinConfigure(GPIO_PE3_SSI1TX);

	GPIOPinTypeSSI(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3);
	GPIOPinTypeSSI(GPIO_PORTH_BASE, GPIO_PIN_6);

	SSIConfigSetExpClk(SSI1_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_3, SSI_MODE_MASTER, 1000000, 16); //16 bit, 1MHz

	SSIEnable(SSI1_BASE);

	// clear RX FIFO
	unsigned long data;
	while(SSIDataGetNonBlocking(SSI1_BASE, &data)) {}

	// check ID
	unsigned char ID = ReadByte(0x75); //WHOAMI (must return 0x68)
	if( ID != 0x68 ) return false;

	// Enable CLOCKS (GyroX)
	WriteByte(0x6B, 0x01);

	//WriteByte(0x1A, 0x00); // 256Hz Bandwidth
	WriteByte(0x1A, 0x02); // 100Hz Bandwidth
	//WriteByte(0x1A, 0x04); // 20Hz Bandwidth

	return true;
}

void MPU6000Drv::UpdateData()
{
	unsigned long data = 0;

	//ACC
	data = ReadByte(0x3B) << 8;
	data += ReadByte(0x3C);
	AccX = (short)data / 16384.0f * 9.81f;
	data = ReadByte(0x3D) << 8;
	data += ReadByte(0x3E);
	AccY = (short)data / 16384.0f * 9.81f;
	data = ReadByte(0x3F) << 8;
	data += ReadByte(0x40);
	AccZ = (short)data / 16384.0f * 9.81f;
	// GYRO
	data = ReadByte(0x43) << 8;
	data += ReadByte(0x44);
	GyroX = (short)data / 131.0f / 180.0f * 3.14159f;
	data = ReadByte(0x45) << 8;
	data += ReadByte(0x46);
	GyroY = (short)data / 131.0f / 180.0f * 3.14159f;
	data = ReadByte(0x47) << 8;
	data += ReadByte(0x48);
	GyroZ = (short)data / 131.0f / 180.0f * 3.14159f;
	// TEMP
	data = ReadByte(0x41) << 8;
	data += ReadByte(0x42);
	Temperature = (((short)data) + 12421.0f)/340.0f; // y_num = 340*X_temp - 12421
}

void MPU6000Drv::GetGyro(float& X, float& Y, float& Z )
{
	X = GyroX;
	Y = GyroY;
	Z = GyroZ;
}

void MPU6000Drv::GetAcc(float& X, float& Y, float& Z )
{
	X = AccX;
	Y = AccY;
	Z = AccZ;
}

void MPU6000Drv::GetTemperature(float& Temp)
{
	Temp = Temperature;
}

unsigned char MPU6000Drv::ReadByte(unsigned char address)
{
	unsigned long dataToWrite = (address << 8) + 0x8000; // READ FLAG

	SSIDataPut(SSI1_BASE, dataToWrite);
	while(SSIBusy(SSI1_BASE)) { }

	unsigned long dataToRead;
	SSIDataGet(SSI1_BASE, &dataToRead);

	return (dataToRead&0x00FF); // remove higher bits
}

void MPU6000Drv::WriteByte(unsigned char address, unsigned char data)
{
	unsigned long dataToWrite = (address << 8) + data; // WRITE

	SSIDataPut(SSI1_BASE, dataToWrite);
	while(SSIBusy(SSI1_BASE)) { }

	unsigned long dataToRead;
	SSIDataGet(SSI1_BASE, &dataToRead); // clear FIFO
}
