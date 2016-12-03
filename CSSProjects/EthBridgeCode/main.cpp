/*
 * main.c
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/uart.h"
#include "driverlib/fpu.h"

#include "Drivers/DBGLed.h"
#include "Drivers/Timer.h"
#include "Drivers/WpnOutputs.h"
#include "Drivers/PWMDrv.h"
#include "Drivers/SerialDriver.h"
#include "Drivers/SBUSComm.h"
#include "Drivers/ADCDrv.h"
#include "Drivers/BaroDrv.h"
#include "Drivers/MPU9250Drv.h"
#include "Drivers/LSM90DDrv.h"
#include "Drivers/UBloxGPS.h"
#include "Drivers/EtherDriver.h"
#include "Drivers/HopeRF.h"
#include "Drivers/IMU.h"
#include "Drivers/CANDrv.h"

#include "CommData.h"
#include "LLConverter.h"
#include "LaunchMgr.h"

uint32_t g_ui32SysClock;

// Drivers
DBGLed dbgLed;
Timer timerLoop;
WpnOutputs wpnOut;
PWMDrv pwmDrv;
SerialDriver serialU2;
SerialDriver serialU3;
SerialDriver serialU5;
SBUSComm sbusRecv;
ADCDrv adcDrv;
BaroDrv baroDrv;
MPU9250Drv mpu9250Drv;
LSM90DDrv lsm90Drv;
UBloxGPS gps;
EtherDriver etherDrv;
HopeRF	hopeRF;
IMU imu;
LaunchMgr launch;
CANDrv canDrv;

// System Objects
LLConverter llConv;

// GPS Port (serialU2->Internal GPS, serialU5->External GPS on Ext Comm.)
#define serialGPS serialU2
//#define serialGPS serialU5

// Systick
#define SysTickFrequency 100
volatile bool SysTickIntHit = false;

// Buffers
#define COMMBUFFERSIZE 1024
BYTE CommBuffer[COMMBUFFERSIZE];
BYTE HopeRFbuffer[255];

// Global Functions
void InitGPS(void);
void ProcessGPSData(void);
void SendPeriodicDataEth(void);
void ProcessCommand(int cmd, unsigned char* data, int dataSize);
void ProcessCANData();
void SendPeriodicDataCAN();

// Global Data
int MainLoopCounter;
float PerfLoopTimeMS;
float PerfCpuTimeMS;
float PerfCpuTimeMSMAX;
float Acc[3];
float Gyro[3];
float Mag[3];
int HopeRSSI;
int AssistNextChunkToSend = 0;

// HopeData
float HopeRSSIAvg= -80;
float HopeRSSIMax = 0;
float HopeRSSIMin = -200;

// OFFSETS
#define GYROOFFX -0.85F
#define GYROOFFY 1.2F
#define GYROOFFZ -0.85F
#define MAGOFFX 17.6360
#define MAGOFFY 35.9106
#define MAGOFFZ -10.2525
#define ATTOFFROLL 1.5F
#define ATTOFFPITCH 0


// ESC Data
int ESC1OperationMode;
int ESC2OperationMode;
int ESC1Locked;
int ESC2Locked;
int ESC1PositionCNT;
int ESC2PositionCNT;
float ESC1CurrentIq;
float ESC2CurrentIq;

// Tracker Data
int TrackerOpMode = 0;
int TrackerPanRef = 0;
int TrackerTiltRef = 0;

void main(void)
{
	// Enable lazy stacking for interrupt handlers.  This allows floating-point
	FPULazyStackingEnable();

	// Ensure that ext. osc is used!
	SysCtlMOSCConfigSet(SYSCTL_MOSC_HIGHFREQ);

	// set clock
	g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);

	// Init
	dbgLed.Init();
	timerLoop.Init();
	wpnOut.Init();
	pwmDrv.Init();
	serialU2.Init(UART2_BASE, 9600); // GPS
	serialU3.Init(UART3_BASE, 100000); // SBUS
	serialU5.Init(UART5_BASE, 9600); // Ext. Comm, Ext. GPS
	sbusRecv.Init();
	adcDrv.Init();
	baroDrv.Init();
	mpu9250Drv.Init();
	//lsm90Drv.Init();
	InitGPS(); // init GPS
	etherDrv.Init();
	hopeRF.Init();
	imu.Init();
    launch.Init();
    canDrv.Init();

	// Systick
	SysTickPeriodSet(g_ui32SysClock/SysTickFrequency);
	SysTickIntEnable();
	SysTickEnable();

	// Master INT Enable
	IntMasterEnable();

	while(1)
	{
		timerLoop.Start(); // start timer
		MainLoopCounter++;

		/////////////////////////////////
		// INPUTS
		/////////////////////////////////
		// SBUS Data
		int rd = serialU3.Read(CommBuffer, COMMBUFFERSIZE); // read data from SBUS Recv [2500 bytes/second, read at least 3x per second for 1k buffer!!!]
		sbusRecv.NewRXPacket(CommBuffer, rd); // process data

		// ADC
		adcDrv.Update();

		// Baro
		if( (MainLoopCounter % 1) == 0 ) baroDrv.Update(); // [17 us] -> 10ms!!!

		// IMU1
		mpu9250Drv.Update();
		Acc[0] = -mpu9250Drv.Accel[1];
		Acc[1] = -mpu9250Drv.Accel[0];
		Acc[2] = mpu9250Drv.Accel[2];
		Gyro[0] = mpu9250Drv.Gyro[1] - GYROOFFX;
		Gyro[1] = mpu9250Drv.Gyro[0] - GYROOFFY;
		Gyro[2] = -mpu9250Drv.Gyro[2] - GYROOFFZ;
		Mag[0] = mpu9250Drv.Mag[0] - MAGOFFX;
		Mag[1] = mpu9250Drv.Mag[1] - MAGOFFY;
		Mag[2] = mpu9250Drv.Mag[2] - MAGOFFZ;
		imu.Update(Acc[0], Acc[1], Acc[2], Gyro[0], Gyro[1], Gyro[2], Mag[0], Mag[1], Mag[2]); // TODO: CHECK AXES
		// Correct IMU offsets
		imu.Roll -= ATTOFFROLL;
		imu.Pitch -= ATTOFFPITCH;

		// IMU2
		//lsm90Drv.Update();

		// GPS
		rd = serialGPS.Read(CommBuffer, COMMBUFFERSIZE); // read data from GPS
		gps.NewRXPacket(CommBuffer, rd); // process data
		// set home position
		if( gps.NumSV >= 6)
		{
			if( !llConv.IsHomeSet() )
			{
				double lat = gps.Latitude * 1e-7;
				double lon = gps.Longitude * 1e-7;
				llConv.SetHome(lat, lon);
			}
		}
		float XN = 0, XE = 0;
		if( llConv.IsHomeSet()) llConv.ConvertLLToM(gps.Latitude*1e-7, gps.Longitude*1e-7, XN, XE);

		// process ethernet (RX)
		etherDrv.Process(1000/SysTickFrequency); // 10ms tick

		// HopeRF
		HopeRSSI = hopeRF.ReadRSSI();
		HopeRSSIAvg = HopeRSSIAvg*0.995 + HopeRSSI*0.005;
		if( HopeRSSI < HopeRSSIMax  ) HopeRSSIMax = HopeRSSI;
		if( HopeRSSI > HopeRSSIMin  ) HopeRSSIMin = HopeRSSI;

		// Hope RX Stuff
		int hopeReceived = hopeRF.Read(HopeRFbuffer);
		if( hopeReceived > 0)
		{
			// Relay data to eth (type 0x41 - HopeRF Data)
			etherDrv.SendPacket(0x41, (char*)&HopeRFbuffer, hopeReceived);
		}

		// CAN
		canDrv.Update();
		ProcessCANData();

		/////////////////////////////////
		// CTRL STEP
		/////////////////////////////////




		/////////////////////////////////
		// OUTPUTS
		/////////////////////////////////

        // Launch Process
		launch.Update();

		// DBG LED
		if( MainLoopCounter%10 == 0) dbgLed.Toggle();

		// send periodic data (ethernet + hopeRF)
		SendPeriodicDataEth();

		// send periodic CAN Data
		SendPeriodicDataCAN();

		// Get CPU Time
		PerfCpuTimeMS = timerLoop.GetUS()/1000.0f;
		if( PerfCpuTimeMS > PerfCpuTimeMSMAX ) PerfCpuTimeMSMAX = PerfCpuTimeMS;
		// wait next
		while(!SysTickIntHit);
		SysTickIntHit = false;
		// Get total loop time
		PerfLoopTimeMS = timerLoop.GetUS()/1000.0f;
	}
}

// Process CAN Data
#define CANBASEADR 0x500
void ProcessCANData()
{
	int id, len;
	unsigned char msg[8];
	while( canDrv.GetMessage(id, msg, len) == true )
	{
		// process message
		if( id == 0x100 )
		{
			memcpy(&ESC1OperationMode, &msg[0], 4);
			memcpy(&ESC1Locked, &msg[4], 4);
		}
		else if( id == 0x200)
		{
			memcpy(&ESC2OperationMode, &msg[0], 4);
			memcpy(&ESC2Locked, &msg[4], 4);
		}
		else if( id == 0x101) memcpy(&ESC1PositionCNT, &msg[0], 4);
		else if( id == 0x201) memcpy(&ESC2PositionCNT, &msg[0], 4);
		else if( id == 0x102) memcpy(&ESC1CurrentIq, &msg[0], 4);
		else if( id == 0x202) memcpy(&ESC2CurrentIq, &msg[0], 4);
	};
}

void SendPeriodicDataCAN(void)
{
	int enabled = 1;
	unsigned char dataToSend[8];

	if( TrackerOpMode == 0) enabled = 0; // disable ESCs

	// ESC1
	memcpy(&dataToSend[0], &enabled, 4);
	memcpy(&dataToSend[4], &TrackerPanRef, 4);
	canDrv.SendMessage(0x110, dataToSend, 8); // send to ESC1

	// ESC2
	memcpy(&dataToSend[0], &enabled, 4);
	memcpy(&dataToSend[4], &TrackerTiltRef, 4);
	canDrv.SendMessage(0x210, dataToSend, 8); // send to ESC2
}


void SendPeriodicDataEth(void)
{
	// Fill data
	SCommEthData data;
	data.LoopCounter = MainLoopCounter;
	data.ActualMode = 0;//ctrl.Ctrl_Y.ActualMode;
	data.Roll = imu.Roll;
	data.Pitch = imu.Pitch;
	data.Yaw = imu.Yaw;
	data.dRoll = Gyro[0];
	data.dPitch = Gyro[1];
	data.dYaw = Gyro[2];
	data.AccX = Acc[0];
	data.AccY = Acc[1];
	data.AccZ = Acc[2];
	data.MagX = Mag[0];
	data.MagY = Mag[1];
	data.MagZ = Mag[2];

	data.Pressure = baroDrv.PressurePa;
	data.Temperature = baroDrv.TemperatureC;
	data.Altitude = 0; //ctrl.Ctrl_Y.Altitude;
	data.Vertspeed = 0; //ctrl.Ctrl_Y.VertSpeed;
	data.FuelLevel = 0; //ctrl.Ctrl_Y.FuelPercent;
	data.BatteryVoltage = adcDrv.BATTVoltage();
	data.BatteryCurrentA = 0;
	data.BatteryTotalCharge_mAh = 0;
	data.MotorThrusts[0] = 0; //(unsigned char)(100*(ctrl.Ctrl_Y.PWM1-1000)/900);
	data.MotorThrusts[1] = 0; //(unsigned char)(100*(ctrl.Ctrl_Y.PWM2-1000)/900);
	data.MotorThrusts[2] = 0; //(unsigned char)(100*(ctrl.Ctrl_Y.PWM3-1000)/900);
	data.MotorThrusts[3] = 0; //(unsigned char)(100*(ctrl.Ctrl_Y.PWM4-1000)/900);

	// gps
	data.GPSTime = gps.GPSTime;
	data.FixType = gps.FixType;
	data.FixFlags = gps.FixFlags;
	data.NumSV = gps.NumSV;
	data.Longitude = gps.Longitude;
	data.Latitude = gps.Latitude;
	data.HeightMSL = gps.HeightMSL;
	data.HorizontalAccuracy = gps.HorizontalAccuracy;
	data.VerticalAccuracy = gps.VerticalAccuracy;
	data.VelN = gps.VelN;
	data.VelE = gps.VelE;
	data.VelD = gps.VelD;
	data.SpeedAcc = gps.SpeedAcc;
	memcpy(data.SatCNOs, gps.SatCNOs, sizeof(data.SatCNOs));

	// RF Data + Perf
	data.EthReceivedCount = etherDrv.ReceivedFrames;
	data.EthSentCount = etherDrv.SentFrames;
	data.HopeRXFrameCount = hopeRF.ReceivedFrames;
	data.HopeRXRSSI = hopeRF.PacketRSSI;
	data.HopeRSSI = HopeRSSIAvg;
	data.PerfCpuTimeMS = PerfCpuTimeMS;
	data.PerfCpuTimeMSMAX = PerfCpuTimeMSMAX;
	data.PerfLoopTimeMS = PerfLoopTimeMS;

	data.AssistNextChunkToSend = AssistNextChunkToSend;

	// Waypoints
	data.WaypointCnt = 0;
	data.WaypointDownloadCounter = 0;
	double hLat, hLong;
	llConv.GetHome(hLat, hLong);
	data.HomeLatitude = hLat*1e7;
	data.HomeLongitude = hLong*1e7;

	// Launch Mgr
	data.LaunchStatus1 = launch.WpnState[0];
	data.LaunchStatus2 = launch.WpnState[1];

	// Tuning data
	data.TuningData[0] = ESC1OperationMode;
	data.TuningData[1] = ESC2OperationMode;
	data.TuningData[2] = ESC1Locked;
	data.TuningData[3] = ESC2Locked;
	data.TuningData[4] = ESC1PositionCNT;
	data.TuningData[5] = ESC2PositionCNT;
	data.TuningData[6] = ESC1CurrentIq;
	data.TuningData[7] = ESC2CurrentIq;
	data.TuningData[8] = 0;
	data.TuningData[9] = 0;

	// send packet (type 0x20 - data)
	etherDrv.SendPacket(0x20, (char*)&data, sizeof(data));
}

void ProcessCommand(int cmd, unsigned char* data, int dataSize)
{
	switch( cmd )
	{
		case 0x30: // AssistNow
		{
			// send data to GPS
			serialGPS.Write(data, dataSize );
			AssistNextChunkToSend++;
			// TODO: Reset AssistNextChunkToSend somewhere!!
			break;
		}

		case 0x40: // Relay to HopeRF
		{
			// Send to hopeRF
			hopeRF.Write(data, dataSize);
			AssistNextChunkToSend = 0;
			break;
		}

		case 0x90: // Launch codes
		{
			// fill data
			SCommLaunch launchCmd;
			memcpy(&launchCmd, data, dataSize);

			if( launchCmd.Command == 1) launch.Arm(launchCmd.Index, launchCmd.CodeTimer);
			else if( launchCmd.Command == 2  ) launch.Fire(launchCmd.Index, launchCmd.CodeTimer);
			else if( launchCmd.Command == 3  ) launch.Dearm(launchCmd.Index, launchCmd.CodeTimer);

			break;
		}

		case 0xA0: // tracker commands
		{
			// fill data
			SCommTrackerCommands trackerCmd;
			memcpy(&trackerCmd, data, dataSize);

			TrackerOpMode = trackerCmd.Mode;
			TrackerPanRef = trackerCmd.PanRef;
			TrackerTiltRef = trackerCmd.TiltRef;

			break;
		}
	}
}



void InitGPS(void)
{
	SysCtlDelay(g_ui32SysClock); // Wait Ext. GPS to boot

	gps.Init();
	// send GPS init commands
	int toSend = gps.GenerateMsgCFGPrt(CommBuffer, 57600); // set to 57k
	serialGPS.Write(CommBuffer, toSend);
	SysCtlDelay(g_ui32SysClock/10); // 100ms wait, flush
	serialGPS.Init(UART2_BASE, 57600); // open with 57k (115k doesn't work well??? small int FIFO, wrong INT prio?)
	toSend = gps.GenerateMsgCFGRate(CommBuffer, 100); // 100ms rate, 10Hz
	serialGPS.Write(CommBuffer, toSend);
	toSend = gps.GenerateMsgCFGMsg(CommBuffer, 0x01, 0x07, 1); // NAV-PVT
	serialGPS.Write(CommBuffer, toSend);
	toSend = gps.GenerateMsgCFGMsg(CommBuffer, 0x01, 0x35, 1); // NAV-SAT
	serialGPS.Write(CommBuffer, toSend);
	toSend = gps.GenerateMsgNAV5Msg(CommBuffer, 6, 3); // airborne <1g, 2D/3D mode
	//toSend = m_GPS.GenerateMsgNAV5Msg(CommBuffer, 7, 2); // airborne <2g, 3D mode only
	serialGPS.Write(CommBuffer, toSend);

	// check response
	SysCtlDelay(g_ui32SysClock/10); // 100ms wait, wait response
	int rd = serialGPS.Read(CommBuffer, COMMBUFFERSIZE);
	gps.NewRXPacket(CommBuffer, rd);
}

///////////////
// INTERRUPTS
///////////////
extern "C" void UART2IntHandler(void)
{
	serialU2.IntHandler();
}

extern "C" void UART3IntHandler(void)
{
	serialU3.IntHandler();
}

extern "C" void UART5IntHandler(void)
{
	serialU5.IntHandler();
}

extern "C" void IntGPIOA(void)
{
	lsm90Drv.MotionINTG();
	lsm90Drv.MotionINTX();
}

extern "C" void IntGPIOH(void)
{
	lsm90Drv.MotionINTM();
}

extern "C" void IntGPIOK(void)
{
	mpu9250Drv.MotionINT();
}

extern "C" void IntGPION(void)
{
	hopeRF.IntHandler();
}

extern "C" void SysTickIntHandler(void)
{
	SysTickIntHit = true;
}
