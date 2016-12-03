/*
 * LSM90DDrv.h
 *
 *  Created on: Jul 3, 2015
 *      Author: Sara
 */

#ifndef LSM90DDRV_H_
#define LSM90DDRV_H_

class LSM90DDrv
{
public:
	bool Init();
	void Update();
	void MotionINTG(void);
	void MotionINTX(void);
	void MotionINTM(void);
private:
	void ReadBytes(unsigned char address, unsigned char* buffer, int count, bool accessXS = false);
	void WriteBytes(unsigned char address, unsigned char* buffer, int count, bool accessXS = false);
	void ClearFIFO();
	unsigned char ReadReg(unsigned char address, bool accessXS = false);
	void WriteReg(unsigned char address, unsigned char value, bool accessXS = false);

public:
	// data
	float Accel[3];
	float Gyro[3];
	float Mag[3];
	float Temperature;

private:
	unsigned int m_INTGCounter;
	unsigned int m_INTXCounter;
	unsigned int m_INTMCounter;
	unsigned char Data[30];
};

#define LSM9DS0_G_ID                       (0b11010100)
#define LSM9DS0_XM_ID                      (0b01001001)

#define LSM9DS0_WHO_AM_I_G		0x0F
#define	LSM9DS0_CTRL_REG1_G		0x20
#define	LSM9DS0_CTRL_REG2_G		0x21
#define	LSM9DS0_CTRL_REG3_G		0x22
#define	LSM9DS0_CTRL_REG4_G		0x23
#define	LSM9DS0_CTRL_REG5_G		0x24
#define	LSM9DS0_OUT_X_L_G		0x28

#define LSM9DS0_WHO_AM_I_XM		0x0F
#define	LSM9DS0_CTRL_REG1_XM	0x20
#define	LSM9DS0_CTRL_REG2_XM	0x21
#define	LSM9DS0_CTRL_REG3_XM	0x22
#define	LSM9DS0_CTRL_REG4_XM	0x23
#define	LSM9DS0_CTRL_REG5_XM	0x24
#define	LSM9DS0_CTRL_REG6_XM	0x25
#define	LSM9DS0_CTRL_REG7_XM	0x26

#define LSM9DS0_GYRO_ODR_BW_190_70	0b01111111
#define LSM9DS0_GYRO_ODR_BW_380_100	0b10111111
#define LSM9DS0_GYRO_ODR_BW_760_100	0b11111111
#define LSM9DS0_GYRO_245DPS		0b00000000
#define LSM9DS0_GYRO_500DPS		0b00010000
#define LSM9DS0_GYRO_2000DPS	0b00100000
#define LSM9DS0_GYRO_I2_DRDY	0x08

#define LSM9DS0_TEMP_OUT_L_XM	0x05
#define LSM9DS0_OUT_X_L_M 		0x08
#define LSM9DS0_CTRL_REG1_XM   	0x20
#define LSM9DS0_CTRL_REG2_XM   	0x21
#define LSM9DS0_CTRL_REG3_XM   	0x22
#define LSM9DS0_CTRL_REG4_XM   	0x23
#define LSM9DS0_CTRL_REG5_XM   	0x24
#define LSM9DS0_CTRL_REG6_XM   	0x25
#define LSM9DS0_CTRL_REG7_XM   	0x26
#define LSM9DS0_OUT_X_L_A 		0x28

#define LSM9DS0_ACC_ODR_50		0x5F
#define LSM9DS0_ACC_ODR_100		0x6F
#define LSM9DS0_ACC_ODR_200		0x7F
#define LSM9DS0_ACC_ODR_400		0x8F
#define LSM9DS0_ACC_ODR_800		0x9F
#define LSM9DS0_ACC_ODR_1200	0xAF
#define LSM9DS0_MAG_ODR_100		0xF4

#define LSM9DS0_ACCEL_2G		0x00
#define LSM9DS0_ACCEL_4G		0x08
#define LSM9DS0_ACCEL_6G		0x10
#define LSM9DS0_ACCEL_8G		0x18
#define LSM9DS0_ACCEL_16G		0x20

#define LSM9DS0_MAG_2GAUSS		0x00
#define LSM9DS0_MAG_4GAUSS		0x20
#define LSM9DS0_MAG_8GAUSS		0x40
#define LSM9DS0_MAG_12GAUSS		0x60

#endif /* LSM90DDRV_H_ */
