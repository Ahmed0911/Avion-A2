
#include "SDCardDriver.h"
#include <inc/lm3s9d90.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <driverlib/interrupt.h>
#include <stdio.h>
#include <string.h>

extern SDataFile datafile;

bool SDCardDriver::Init()
{
	FRESULT fresult;
	m_FileValid = false;

	// mount
	fresult = f_mount(0, &m_sFatFs);

	if(fresult != FR_OK)
	{
		return false;
	}

	// find first file (1...1000)
	for(int i=0; i!=1000; i++)
	{
		char filename[100];
		sprintf(filename, "file-%d.bin", i);
		FRESULT res = f_open(&m_sFileObject, filename, FA_CREATE_NEW | FA_WRITE );
		if( res == FR_OK)
		{
			// new file created, OK
			m_FileValid = true;

			datafile.SDCardActive = true;

			return true;
		}
		if( res != FR_EXIST) return false; // unknown error!!!
	}

	return false; // ERROR
}

bool SDCardDriver::WriteData(BYTE* data, int len)
{
	if( !m_FileValid ) return false;

	// write file
	WORD bWr;
	FRESULT res = f_write(&m_sFileObject,data,len,&bWr);
	if( res != FR_OK)
	{
		datafile.SDCardFails++;
		return false;
	}

	datafile.SDCardBytesWritten+=len;

	return true;
}

bool SDCardDriver::Flush()
{
	if( !m_FileValid ) return false;

	f_sync(&m_sFileObject);

	return true;
}

