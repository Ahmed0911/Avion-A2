
#ifndef MPU6000DRV_H_
#define MPU6000DRV_H_

class MPU6000Drv
{
public:
	bool Init(void);
	void UpdateData();
	void GetGyro(float& X, float& Y, float& Z );
	void GetAcc(float& X, float& Y, float& Z );
	void GetTemperature(float& Temp);

private:
	unsigned char ReadByte(unsigned char address);
	void WriteByte(unsigned char address, unsigned char data);

	float GyroX;
	float GyroY;
	float GyroZ;
	float AccX;
	float AccY;
	float AccZ;
	float Temperature;
};

#endif /* MPU6000DRV_H_ */
