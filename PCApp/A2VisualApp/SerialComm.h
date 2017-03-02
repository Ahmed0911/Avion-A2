#pragma once
#include "Serial.h"
#include "Comm433MHz.h"

typedef void(*NewPacketCallbackType)(char, BYTE*, int);

class CSerialComm
{
public:
	CSerialComm();
	bool Init();
	void Close();
	void ConnectTo(TCHAR* serialPort, NewPacketCallbackType callback);

	void Update();
	void SendData(char type, BYTE* buffer, int length);


private:
	CSerial m_Serial;
	CComm433MHz m_Comm433MHz;
	NewPacketCallbackType NewPacketCallback; // callback 
};