
#ifndef CANDRIVER_H_
#define CANDRIVER_H_

class CANDriver
{
public:
	void Init();
	bool SendMessage(int msgID, unsigned char data[8], int len);
	//void SetRXMessage(int objectIdx, int msgID, int len);

private:
	void SendMessage(int objectIdx, int msgID, unsigned char data[8], int len);
	//bool ReadMessage(int objectIdx, unsigned char data[8]);
};

#endif /* CANDRIVER_H_ */
