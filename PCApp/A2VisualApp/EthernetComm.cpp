#include "stdafx.h"
#include "EthernetComm.h"
#include "TrajectoryMgr.h"
#include "CommData.h"
#include <Ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib") //Winsock Library

/* Max length of buffer */
#define BUFLEN 2048

/* The ports */
#define TARGET_PORT 12000

CEthernetComm::CEthernetComm()
{
	m_socket = SOCKET_ERROR;
	ReceivedPacketsCounter = 0;
	PingCounter = 0;
}

bool CEthernetComm::Init(int localPort)
{	
	//Initialise winsock
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
	
	//create socket
	if ((m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR) return false;
	
	// setup and bind local
	memset((char *)&m_siLocal, 0, sizeof(m_siLocal));
	m_siLocal.sin_family = AF_INET;
	m_siLocal.sin_port = htons(localPort);
	m_siLocal.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (bind(m_socket, (SOCKADDR *)&m_siLocal, sizeof (m_siLocal)) != S_OK) return false;

	// non blocking mode
	u_long iMode = 1;
	ioctlsocket(m_socket, FIONBIO, &iMode);

	return true;
}

void CEthernetComm::ConnectTo(TCHAR* targetAddress, NewPacketCallbackType callback)
{
	//setup address structure
	memset((char *)&m_siTarget, 0, sizeof(m_siTarget));
	m_siTarget.sin_family = AF_INET;
	m_siTarget.sin_port = htons(TARGET_PORT);
	InetPton(AF_INET, targetAddress, &m_siTarget.sin_addr);

	// set RX callback
	NewPacketCallback = callback;
}

void CEthernetComm::Close()
{
	closesocket(m_socket);
	WSACleanup();
}

void CEthernetComm::Update()
{
	if (m_socket == SOCKET_ERROR ) return; // no socket

	//try to receive some data, this is a blocking call
	char buffer[BUFLEN];
	struct sockaddr_in si_recvSource;
	int si_size = sizeof(si_recvSource);
	
	// Get data from UDP
	int received = recvfrom(m_socket, buffer, BUFLEN, 0, (struct sockaddr *) &si_recvSource, &si_size);
	while (received > 0)
	{
		// process data
		ReceivedPacketsCounter++;
		if (PacketIsValid(buffer))
		{
			// remove header
			char* withoutHeader = &buffer[3];
			int sizeWithoutHeader = received - 3;

			// OK
			NewPacketCallback(buffer[2], (BYTE*)withoutHeader, sizeWithoutHeader);
		}

		// get next?
		received = recvfrom(m_socket, buffer, BUFLEN, 0, (struct sockaddr *) &si_recvSource, &si_size);
	}

	// send PING
	if (++PingCounter > 20)
	{
		PingCounter = 0;
		int port = ntohs(m_siLocal.sin_port);
		unsigned char data[4];
		memcpy(data, &port, 4);
		SendData(0x10, data, 4);
	}
}

void CEthernetComm::SendData(char type, BYTE* buffer, int length)
{
	if (m_socket == SOCKET_ERROR) return; // no socket

	char data[BUFLEN];
	// assemble
	data[0] = 0x42;
	data[1] = 0x24;
	data[2] = type; // Type
	memcpy(&data[3], buffer, length);
	sendto(m_socket, data, length + 3, 0, (struct sockaddr *) &m_siTarget, sizeof(m_siTarget));
}

bool CEthernetComm::PacketIsValid(char* bytes)
{
	bool valid = true;
	if (bytes[0] != 0x42) valid = false;
	if (bytes[1] != 0x24) valid = false;

	return valid;
}
