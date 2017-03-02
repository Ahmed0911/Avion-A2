#include "stdafx.h"
#include "SerialComm.h"
#include "TrajectoryMgr.h"
#include "CommData.h"

CSerialComm::CSerialComm()
{

}

bool CSerialComm::Init()
{
	return true;
}

void CSerialComm::ConnectTo(TCHAR* serialPort, NewPacketCallbackType callback)
{
	// close if already opened
	if (m_Serial.IsOpen()) m_Serial.Close();

	// open serial comm
	m_Serial.Init(serialPort, 115200);

	// set RX callback
	NewPacketCallback = callback;
}

void CSerialComm::Close()
{
	m_Serial.Close();
}

void CSerialComm::Update()
{
	if (!m_Serial.IsOpen()) return; // port not open

	// Get data from Serial Port
	BYTE packet[4096]; // max buffer size defined in CSerial
	int dataLen = m_Serial.Read(packet);

	// Process Command
	m_Comm433MHz.NewRXPacket(packet, dataLen, NewPacketCallback);
}

void CSerialComm::SendData(char type, BYTE* buffer, int length)
{
	if (!m_Serial.IsOpen()) return; // port not open

	// Assemble full TX message (with Header and CRC)
	BYTE outputPacket[500];
	int sizeToSend = m_Comm433MHz.GenerateTXPacket(type, buffer, length, outputPacket);

	// Send Message
	m_Serial.Write(outputPacket, sizeToSend);
}
