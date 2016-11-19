#include "stdafx.h"
#include "Serial.h"

#define READ_BUFFER_SIZE 4096

CSerial::CSerial(void)
{
	m_hComm = INVALID_HANDLE_VALUE;
}


CSerial::~CSerial(void)
{
}

bool CSerial::Init(TCHAR* port, int baudrate)
{	
	// Open port
	m_hComm = CreateFile( port,  
						GENERIC_READ | GENERIC_WRITE, 
						0, 
						0, 
						OPEN_EXISTING,
						0,
						0);

	if (m_hComm == INVALID_HANDLE_VALUE) false;

	// set DCB
	DCB dcb;
	FillMemory(&dcb, sizeof(dcb), 0);
	dcb.DCBlength = sizeof(dcb);
	dcb.fBinary = TRUE;
	dcb.BaudRate = baudrate; // 57600 Baud
	dcb.ByteSize = 8; //8 data bits
	dcb.Parity = NOPARITY; //no parity
	dcb.StopBits = ONESTOPBIT; //1 stop

	if (!SetCommState(m_hComm, &dcb))
	{
		return false;
	}

	COMMTIMEOUTS timeouts;
	timeouts.ReadIntervalTimeout = MAXDWORD; 
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 0;

	if (!SetCommTimeouts(m_hComm, &timeouts))
	{
		return false;
	}

	return true;
}

int CSerial::Read(void* buffer)
{
	DWORD rd = 0;	
	if(ReadFile(m_hComm, buffer, READ_BUFFER_SIZE, &rd, NULL) == 0)
    {
		// error
		return -1;
    }

	return rd;
}

bool CSerial::Write(void* buffer, int len)
{
	DWORD wr;

    if(WriteFile(m_hComm, buffer, len, &wr, NULL) == 0)
    {
		// error

		return false;
    }

	if( wr != len ) return false; // buffer full????
    
	return true; // OK
}

void CSerial::Close()
{
	if( m_hComm != INVALID_HANDLE_VALUE )
	{
		CloseHandle(m_hComm);
	}
}

bool CSerial::IsOpen(void)
{
	return (m_hComm != INVALID_HANDLE_VALUE);
}
