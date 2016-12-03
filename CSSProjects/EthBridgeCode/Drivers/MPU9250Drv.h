/*
 * MPU9250Drv.h
 *
 *  Created on: Jul 1, 2015
 *      Author: Sara
 */

#ifndef MPU9250DRV_H_
#define MPU9250DRV_H_

class MPU9250Drv
{
public:
	bool Init();
	void Update();
	void MotionINT(void);
private:
	void ReadBytes(unsigned char address, unsigned char* buffer, int count);
	void WriteBytes(unsigned char address, unsigned char* buffer, int count);
	void ClearFIFO();

public:
	// data
	float Accel[3];
	float Gyro[3];
	float Mag[3];
	float Temperature;

private:
	unsigned int m_MPU9150INTCounter;
	unsigned char Data[30];
};

// DEFINES
#define MPU9150_O_PWR_MGMT_1    			0x6B        // Power management 1 register
#define MPU9150_PWR_MGMT_1_DEVICE_RESET		0x80        // Device reset
#define MPU9150_PWR_MGMT_1_SLEEP            0x40        // Enter sleep mode
#define MPU9150_PWR_MGMT_1_CLKSEL_XG        0x01        // PLL with X-axis gyro reference

#define MPU9150_O_USER_CTRL     			0x6A        // User control register
#define MPU9150_USER_CTRL_I2C_MST_EN        0x20        // I2C master mode enable

#define MPU9150_O_SMPLRT_DIV    			0x19        // Sample rate divider register
#define MPU9150_O_CONFIG        			0x1A        // Configuration register
#define MPU9150_CONFIG_DLPF_CFG_460_250     0x00        // 460 Hz accelerometer bandwidth, 250 Hz gyro bandwidth
#define MPU9150_CONFIG_DLPF_CFG_184_184     0x01        // 184 Hz accelerometer bandwidth, 184 Hz gyro bandwidth
#define MPU9150_CONFIG_DLPF_CFG_92_92       0x02        // 92 Hz accelerometer bandwidth,  92 Hz gyro bandwidth
#define MPU9150_CONFIG_DLPF_CFG_41_41       0x03        // 41 Hz accelerometer bandwidth,  41 Hz gyro bandwidth
#define MPU9150_CONFIG_DLPF_CFG_20_20       0x04        // 20 Hz accelerometer bandwidth,  20 Hz gyro bandwidth
#define MPU9150_CONFIG_DLPF_CFG_10_10       0x05        // 10 Hz accelerometer bandwidth,  10 Hz gyro bandwidth
#define MPU9150_CONFIG_DLPF_CFG_5_5         0x06        // 5 Hz accelerometer bandwidth,   5 Hz gyro bandwidth

#define MPU9150_O_GYRO_CONFIG   			0x1B        // Gyro configuration register
#define MPU9150_GYRO_CONFIG_FS_SEL_250      0x00        // Gyro full-scale range +/- 250 degrees/sec
#define MPU9150_GYRO_CONFIG_FS_SEL_500      0x08        // Gyro full-scale range +/- 500 degrees/sec
#define MPU9150_GYRO_CONFIG_FS_SEL_1000     0x10        // Gyro full-scale range +/- 1000 degrees/sec
#define MPU9150_GYRO_CONFIG_FS_SEL_2000     0x18        // Gyro full-scale range +/- 2000 degrees/sec

#define MPU9150_O_ACCEL_CONFIG  			0x1C        // Accelerometer configuration register
#define MPU9150_ACCEL_CONFIG_AFS_SEL_2G     0x00        // Accelerometer full-scale range 2 g
#define MPU9150_ACCEL_CONFIG_AFS_SEL_4G     0x08        // Accelerometer full-scale range 4 g
#define MPU9150_ACCEL_CONFIG_AFS_SEL_8G     0x10        // Accelerometer full-scale range 8 g
#define MPU9150_ACCEL_CONFIG_AFS_SEL_16G    0x18        // Accelerometer full-scale range 16 g

#define MPU9150_O_ACCEL_CONFIG2  			0x1D        // Accelerometer configuration register2

#define MPU9150_O_I2C_MST_CTRL				0x24        // I2C master control register
#define MPU9150_I2C_MST_CTRL_I2C_MST_CLK_400	0x0D	// 400 kHz I2C master clock
#define MPU9150_I2C_MST_CTRL_WAIT_FOR_ES	0x40        // Wait for external sensor data
#define MPU9150_I2C_SLV0_ADDR_RW            0x80        // Read/not write
#define MPU9150_I2C_SLV0_CTRL_EN            0x80        // Enable slave

#define MPU9150_O_I2C_SLV4_ADDR 			0x31        // I2C slave 4 address register
#define MPU9150_I2C_SLV4_CTRL_EN            0x80        // Enable slave

#define MPU9150_O_INT_PIN_CFG   			0x37        // INT pin configuration register
#define MPU9150_INT_PIN_CFG_INT_LEVEL       0x80        // INT pin active low
#define MPU9150_INT_PIN_CFG_INT_OPEN        0x40        // INT pin open-drain
#define MPU9150_INT_PIN_CFG_LATCH_INT_EN    0x20        // Latch INT pin output
#define MPU9150_INT_PIN_CFG_INT_RD_CLEAR    0x10        // Interrupt clear on any read

#define MPU9150_O_INT_ENABLE   				0x38        // Interrupt enable register
#define MPU9150_INT_ENABLE_DATA_RDY_EN      0x01        // Data ready interrupt enable

#define MPU9150_O_INT_STATUS    			0x3A        // Interrupt status register

#define MPU9150_O_ACCEL_XOUT_H  			0x3B        // X-axis acceleration data MSB register

#define MPU9150_O_I2C_MST_DELAY_CTRL        0x67        // I2C master delay control register
#define MPU9150_I2C_MST_DELAY_CTRL_I2C_SLV4_DLY_EN  0x10	// I2C slave 4 delay enable
#define MPU9150_I2C_MST_DELAY_CTRL_I2C_SLV0_DLY_EN	0x01	// I2C slave 0 delay enable

#define AK8975_O_ST1            			0x02        // Status 1 register
#define AK8975_O_CNTL           			0x0A        // Control register
#define AK8975_CNTL_MODE_SINGLE 			0x01        // Single measurement mode

#endif /* MPU9250DRV_H_ */
