
#ifndef ETHERDRIVER_H_
#define ETHERDRIVER_H_

#include "utils/lwiplib.h"

#define ETHPORT 12000

class EtherDriver
{
public:
	void Init();
	void Process(int ms);
	void DataReceived(pbuf *p, ip_addr *addr, u16_t port);
	bool SendPacket(char type, char* txBuff,  int size);

public:
	int ReceivedFrames;
	int SentFrames;

private:
	udp_pcb* m_udpPcb;
	void SendPacket(char type, char* txBuff, int size, ip_addr *destination, u16_t destinationPort);
	bool IsPacketValid(pbuf* p);

	// destination copy
	ip_addr DestinationAddr;
	u16_t DestinationPort;
};

#endif /* ETHERDRIVER_H_ */
