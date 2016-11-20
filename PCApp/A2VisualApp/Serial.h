#pragma once

class CSerial
{
public:
	CSerial(void);
	~CSerial(void);

	bool Init(TCHAR* port, int baudrate);
	void Close();

	int Read(void* buffer);
	bool Write(void* buffer, int len);
	bool IsOpen(void);

private:
	HANDLE m_hComm;
};

