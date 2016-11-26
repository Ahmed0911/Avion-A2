#pragma once

#include <winsock2.h>

typedef void(*NewPacketCallbackType)(char, BYTE*, int);

class CEthernetComm
{
public:
	CEthernetComm();
	bool Init(int localPort);
	void Close();
	void ConnectTo(char* targetAddress, NewPacketCallbackType callback);
	
	void Update();
	void SendData(char type, BYTE* buffer, int length);

public:
	int ReceivedPacketsCounter;

private:
	bool PacketIsValid(char* bytes);
	

private:
	WSADATA wsa;
	struct sockaddr_in m_siLocal;
	struct sockaddr_in m_siTarget;
	int m_socket;

	int PingCounter;
	NewPacketCallbackType NewPacketCallback; // callback 
};

