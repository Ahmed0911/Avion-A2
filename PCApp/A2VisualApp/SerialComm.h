#pragma once
#include "Serial.h"
#include "Comm433MHz.h"

typedef void(*ReceivedMessageCallbackType)(BYTE, BYTE*, BYTE);

class CSerialComm
{
public:
	CSerialComm();
	bool Init();
	void Close();
	void ConnectTo(TCHAR* serialPort, ReceivedMessageCallbackType callback);
	bool IsOpen();

	void Update();
	void SendData(char type, BYTE* buffer, int length);

public:
	CComm433MHz m_Comm433MHz;

private:
	CSerial m_Serial;	
	ReceivedMessageCallbackType NewPacketCallback; // callback 
};