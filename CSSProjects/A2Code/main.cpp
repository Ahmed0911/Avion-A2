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
#include "Drivers/EEPROMDrv.h"
#include "Drivers/CANDrv.h"

#include "Ctrl/Controller.h"
#include "CommData.h"
#include "Parameters.h"
#include "LLConverter.h"
#include "LaunchMgr.h"
#include "CRC32.h"
#include "Comm433MHz.h"

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
//EtherDriver etherDrv;
//HopeRF	hopeRF;
IMU imu;
LaunchMgr launch;
EEPROMDrv eeprom;
CRC32 crc;
Comm433MHz comm433MHz;
CANDrv canDrv;

// System Objects
ControllerModelClass ctrl;
LLConverter llConv;

// GPS Port (serialU2->Internal GPS, serialU5->External GPS on Ext Comm.)
#define serialGPS serialU2
//#define serialGPS serialU5

// Systick
#define SysTickFrequency 400
volatile bool SysTickIntHit = false;

// Buffers
#define COMMBUFFERSIZE 1024
BYTE CommBuffer[COMMBUFFERSIZE];
BYTE HopeRFbuffer[255];

// Global Functions
void InitGPS(void);
void ProcessGPSData(void);
void ProcessCANData(void);
void SendPeriodicDataEth(void);
void ProcessCommand(int cmd, unsigned char* data, int dataSize);
void ReadParamsFromFlash();
void WriteParamsToFlash();
void CopyParams2Str(SParameters& params);
void CopyStr2Params(SParameters& params);

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
float Pressure0 = 101300;

bool PingedSendData = false;

// SD Card stuff
unsigned int SDCardActive;
unsigned int SDCardBytesWritten;
unsigned int SDCardFails;
unsigned int FailedQueues;

// Battery State
float BatteryCurrentA = 0;
double BatteryTotalCharge_mAh = 0;

// Modes:
// 0 - Off
// 1 - Manual
// 2 - Attitude (Roll/Pitch)
// 3 - TBD

// Waypoints + TARGET
float TargetAlt = 30;
float TargetN = 0;
float TargetE = 0;

#define MAXWAYPOINTS 8
float WayAlt[MAXWAYPOINTS];
float WayN[MAXWAYPOINTS];
float WayE[MAXWAYPOINTS];
int WayCnt = 0;
int WayDownloadCnt = 0;
int WayExecute = 0;
float TrajZNEVParams[4];

char ParamsCommandAckCnt = 0;

// OFFSETS
float GyroOffX = 0;
float GyroOffY = 0;
float GyroOffZ = 0;
float MagOffX = 0;
float MagOffY = 0;
float MagOffZ = 0;
float AttOffRoll = 0;
float AttOffPitch = 0;

void main(void)
{
	// Enable lazy stacking for interrupt handlers.  This allows floating-point
	FPULazyStackingEnable();

	// Ensure that ext. osc is used!
	SysCtlMOSCConfigSet(SYSCTL_MOSC_HIGHFREQ);

	// set clock
	g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);

	// Read params from flash
	ReadParamsFromFlash();

	// Init
	dbgLed.Init();
	timerLoop.Init();
	crc.Init();
	wpnOut.Init();
	pwmDrv.Init();
	pwmDrv.SetWidthUS(0, 1000); // Set zero PWMs
	pwmDrv.SetWidthUS(1, 1500); // Set midpoint PWMs
	pwmDrv.SetWidthUS(2, 1500); // Set midpoint PWMs
	pwmDrv.SetWidthUS(3, 1500);	// Set midpoint PWMs
	serialU2.Init(UART2_BASE, 9600); // GPS
	serialU3.Init(UART3_BASE, 100000); // SBUS
	serialU5.Init(UART5_BASE, 115200); // Ext. Comm
	sbusRecv.Init();
	adcDrv.Init();
	baroDrv.Init();
	mpu9250Drv.Init();
    //lsm90Drv.Init();
	InitGPS(); // init GPS
	//etherDrv.Init();
	//hopeRF.Init();
	imu.Init();
	launch.Init();
	canDrv.Init();
	ctrl.initialize();



	// Systick
	SysTickPeriodSet(g_ui32SysClock/SysTickFrequency);
	SysTickIntEnable();
	SysTickEnable();

	// Master INT Enable
	IntMasterEnable();

	// FAKE GPS
	//llConv.SetHome(45.80349, 15.88388);

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

		// ADC + Current Calc
		adcDrv.Update();
		BatteryCurrentA = adcDrv.BATTCurrent();
		BatteryTotalCharge_mAh += (1000.0/3600.0 * (double)BatteryCurrentA / SysTickFrequency );

		// Baro
		baroDrv.Update(); // [??? us]

		// IMU1
		mpu9250Drv.Update();
		Acc[0] = -mpu9250Drv.Accel[1];
		Acc[1] = -mpu9250Drv.Accel[0];
		Acc[2] = mpu9250Drv.Accel[2];
		Gyro[0] = mpu9250Drv.Gyro[1] - GyroOffX;
		Gyro[1] = mpu9250Drv.Gyro[0] - GyroOffY;
		Gyro[2] = -mpu9250Drv.Gyro[2] - GyroOffZ;
		Mag[0] = mpu9250Drv.Mag[0] - MagOffX;
		Mag[1] = mpu9250Drv.Mag[1] - MagOffY;
		Mag[2] = mpu9250Drv.Mag[2] - MagOffZ;
		imu.Update(Acc[0], Acc[1], Acc[2], Gyro[0], Gyro[1], Gyro[2], Mag[0], Mag[1], Mag[2]);
		imu.Roll -= AttOffRoll; // Correct IMU offsets
		imu.Pitch -= AttOffPitch;

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

		// Set Pressure0 on 2 sec
		if(MainLoopCounter == (SysTickFrequency * 2)) Pressure0 = baroDrv.PressurePa;

		// process ethernet (RX)
		//etherDrv.Process(1000/SysTickFrequency); // 2.5ms tick

		// CAN
		canDrv.Update();
		ProcessCANData(); // Receive CAN data

        // Read Lora Data
        int dataReceived = serialU5.Read(CommBuffer, COMMBUFFERSIZE);
        comm433MHz.NewRXPacket(CommBuffer, dataReceived); // calls ProcessCommand callback!!!

		// set inputs
		ctrl.rtU.RPY[0] = imu.Roll;
		ctrl.rtU.RPY[1] = imu.Pitch;
		ctrl.rtU.RPY[2] = imu.Yaw;
		ctrl.rtU.dRPY[0] = Gyro[0];
		ctrl.rtU.dRPY[1] = Gyro[1];
		ctrl.rtU.dRPY[2] = Gyro[2];
		ctrl.rtU.PressureAbs = baroDrv.PressurePa;
		ctrl.rtU.Pressure0 = Pressure0;
		ctrl.rtU.Mode = sbusRecv.Channels[4]; // Mode selector
		ctrl.rtU.Thr = sbusRecv.Channels[0];
		ctrl.rtU.Aileron = sbusRecv.Channels[1];
		ctrl.rtU.Elevator = sbusRecv.Channels[2];
		ctrl.rtU.Rudder = sbusRecv.Channels[3];
		ctrl.rtU.FlatVe[0] = gps.VelN / 1000.0f; // [m/s], Northing - X
		ctrl.rtU.FlatVe[1] = gps.VelE / 1000.0f; // [m/s], Easting - Y
		ctrl.rtU.FlatVe[2] = gps.VelD / 1000.0f; // [m/s], Down - Z, -> ignored, gps alt Z vel is not accurate! Probably?!
		ctrl.rtU.FlatXe[0] = XN;
		ctrl.rtU.FlatXe[1] = XE;

#if 0
		// fill waypoint buffer
		ctrl.Ctrl_U.Waypoints[0] = TargetAlt;
		ctrl.Ctrl_U.Waypoints[9] = TargetN;
		ctrl.Ctrl_U.Waypoints[18] = TargetE;
		for(int i=0; i!=MAXWAYPOINTS; i++)
		{
			ctrl.Ctrl_U.Waypoints[1+i] = WayAlt[i];
			ctrl.Ctrl_U.Waypoints[10+i] = WayN[i];
			ctrl.Ctrl_U.Waypoints[19+i] = WayE[i];
		}
		ctrl.Ctrl_U.WayCnt = WayCnt;
		ctrl.Ctrl_U.ExecuteWaypoints = WayExecute;
		ctrl.Ctrl_U.TrajZNEVParams[0] = TrajZNEVParams[0];
		ctrl.Ctrl_U.TrajZNEVParams[1] = TrajZNEVParams[1];
		ctrl.Ctrl_U.TrajZNEVParams[2] = TrajZNEVParams[2];
		ctrl.Ctrl_U.TrajZNEVParams[3] = TrajZNEVParams[3];
		WayExecute = 0; // auto reset flag
#endif

		/////////////////////////////////
		// CTRL STEP
		/////////////////////////////////
		ctrl.step();


		/////////////////////////////////
		// OUTPUTS
		/////////////////////////////////
		pwmDrv.SetWidthUS(0, ctrl.rtY.PWM1);
		pwmDrv.SetWidthUS(1, ctrl.rtY.PWM2);
		pwmDrv.SetWidthUS(2, ctrl.rtY.PWM3);
		pwmDrv.SetWidthUS(3, ctrl.rtY.PWM4);

		// Launch Process
		launch.Update();

		// DBG LED
		dbgLed.Set(ctrl.rtY.GreenLED);

		// send periodic data (ethernet + hopeRF)
		SendPeriodicDataEth();



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

void SendPeriodicDataEth(void)
{
    if( (MainLoopCounter % 2) == 0 )
    {
        // Send data to Logger
        // Fill data
        SCommEthData data;
        data.LoopCounter = MainLoopCounter;
        data.ActualMode = ctrl.rtY.ActualMode;
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
        data.Altitude = ctrl.rtY.Altitude;
        data.Vertspeed = ctrl.rtY.VertSpeed;
        data.FuelLevel = 0;
        data.BatteryVoltage = adcDrv.BATTVoltage();
        data.BatteryCurrentA = BatteryCurrentA;
        data.BatteryTotalCharge_mAh = BatteryTotalCharge_mAh;
        data.MotorThrusts[0] = (unsigned char)(100*(ctrl.rtY.PWM1-1000)/1000);
        data.MotorThrusts[1] = (unsigned char)(100*(ctrl.rtY.PWM2-1000)/1000);
        data.MotorThrusts[2] = (unsigned char)(100*(ctrl.rtY.PWM3-1000)/1000);
        data.MotorThrusts[3] = (unsigned char)(100*(ctrl.rtY.PWM4-1000)/1000);

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
        data.EthReceivedCount = 0;//etherDrv.ReceivedFrames;
        data.EthSentCount = 0;//etherDrv.SentFrames;
        data.HopeRXFrameCount = comm433MHz.MsgReceivedOK;
        data.HopeRXRSSI = comm433MHz.HeaderFails; // fail counter, use as RSSI?
        data.HopeRSSI = HopeRSSI;
        data.PerfCpuTimeMS = PerfCpuTimeMS;
        data.PerfCpuTimeMSMAX = PerfCpuTimeMSMAX;
        data.PerfLoopTimeMS = PerfLoopTimeMS;
        data.AssistNextChunkToSend = AssistNextChunkToSend;

        // Waypoints
        data.WaypointCnt = WayCnt;
        data.WaypointDownloadCounter = WayDownloadCnt;
        double hLat, hLong;
        llConv.GetHome(hLat, hLong);
        data.HomeLatitude = hLat*1e7;
        data.HomeLongitude = hLong*1e7;

        // Launch Mgr
        data.LaunchStatus1 = launch.WpnState[0];
        data.LaunchStatus2 = launch.WpnState[1];

        // Tuning data
        data.TuningData[0] = sbusRecv.Channels[0]; // thr
        data.TuningData[1] = sbusRecv.Channels[1]; // aileron
        data.TuningData[2] = sbusRecv.Channels[2]; // elevation
        data.TuningData[3] = sbusRecv.Channels[3]; // rudder
        data.TuningData[4] = sbusRecv.Channels[4]; // mode
        data.TuningData[5] = sbusRecv.Channels[5]; // ch5
        data.TuningData[6] = sbusRecv.Channels[6]; // ch6
        data.TuningData[7] = sbusRecv.SystemByte; // systemByte
        data.TuningData[8] = 0;
        data.TuningData[9] = 0;

        // send packet (type 0x20 - data)
        //etherDrv.SendPacket(0x20, (char*)&data, sizeof(data));

        // TODO: Send all this crap to CAN
        for(int i=0; i!=30; i++)
        {
            // pack and go
            BYTE* ptr = (BYTE*)&data;
            canDrv.SendMessage(0x100 + i, &(ptr[i*8]), 8); // status
        }
    }


	// Send to Lora
	//if( MainLoopCounter%4000 == 0 || PingedSendData) // 400hz/4000 = 0.1hz, every 10s OR ping
	if( PingedSendData )
	{
		SCommHopeRFDataA2Avion dataRF;
		dataRF.LoopCounter = MainLoopCounter;
		dataRF.ActualMode = ctrl.rtY.ActualMode;
		dataRF.Roll = imu.Roll;
		dataRF.Pitch = imu.Pitch;
		dataRF.Yaw = imu.Yaw;
		dataRF.dRoll = Gyro[0];
		dataRF.dPitch = Gyro[1];
		dataRF.dYaw = Gyro[2];
		dataRF.Altitude = ctrl.rtY.Altitude;
		dataRF.Vertspeed = ctrl.rtY.VertSpeed;
		dataRF.MotorThrusts[0] = (unsigned char)(100*(ctrl.rtY.PWM1-1000)/1000);
		dataRF.MotorThrusts[1] = (unsigned char)(100*(ctrl.rtY.PWM2-1000)/1000);
		dataRF.MotorThrusts[2] = (unsigned char)(100*(ctrl.rtY.PWM3-1000)/1000);
		dataRF.MotorThrusts[3] = (unsigned char)(100*(ctrl.rtY.PWM4-1000)/1000);

		dataRF.BatteryVoltage = adcDrv.BATTVoltage();
		dataRF.BatteryCurrentA = BatteryCurrentA;
		dataRF.BatteryTotalCharge_mAh = BatteryTotalCharge_mAh;

		dataRF.FixType = gps.FixType;
		dataRF.ParamsCommandAckCnt = ParamsCommandAckCnt;
		dataRF.NumSV = gps.NumSV;
		dataRF.Longitude = gps.Longitude;
		dataRF.Latitude = gps.Latitude;
		dataRF.VelN = gps.VelN;
		dataRF.VelE = gps.VelE;
		dataRF.HorizontalAccuracy = gps.HorizontalAccuracy;
		dataRF.SDCardBytesWritten = SDCardBytesWritten;
		dataRF.SDCardFails = SDCardFails;
		dataRF.FailedQueues = FailedQueues;

		dataRF.HopeRXFrameCount = comm433MHz.MsgReceivedOK;
		dataRF.HopeRXRSSI = comm433MHz.HeaderFails; // fail counter, use as RSSI?
		dataRF.HopeTXRSSI = 0; // will be filled on GW station

		// fill CRC code (not needed, auto generated in GenerateTXPacket()!!!!)
		dataRF.CRC32 = crc.CalculateCRC32((BYTE*)&dataRF, sizeof(dataRF)-sizeof(dataRF.CRC32));

		// Send to LORA
        int bytesToSend = comm433MHz.GenerateTXPacket(0x20, (BYTE*)&dataRF, sizeof(dataRF), CommBuffer);
        serialU5.Write(CommBuffer, bytesToSend);
        //serialU5.Write(CommBuffer, 59);
        //serialU5.Write(CommBuffer, 58);

        PingedSendData = false;
	}
}

// Process CAN Data
void ProcessCANData()
{
    int id, len;
    unsigned char msg[8];
    while( canDrv.GetMessage(id, msg, len) == true )
    {
        // process message
        if( id == (0x200))
        {
            // card active and bytes written
            memcpy(&SDCardActive, &msg[0], 4);
            memcpy(&SDCardBytesWritten, &msg[4], 4);
        }
        if( id == (0x201))
        {
            // card fails, failed queues
            memcpy(&SDCardFails, &msg[0], 4);
            memcpy(&FailedQueues, &msg[4], 4);
        }
    };
}

// Process commands received from Ethernet and HopeRF
void ProcessCommand(int cmd, unsigned char* data, int dataSize)
{
	switch( cmd )
	{
	    case 0x10: // PING
        {
            // Schedule data transfer
            PingedSendData = true;
            break;
        }

	    case 0x11: // Logger Ping
        {
            SPingLoggerData pingLogger;
            if( dataSize == sizeof(pingLogger))
            {
                memcpy(&pingLogger, data, sizeof(pingLogger));
                SDCardActive = pingLogger.SDCardActive;
                SDCardBytesWritten = pingLogger.SDCardBytesWritten;
                SDCardFails = pingLogger.SDCardFails;
                FailedQueues = pingLogger.FailedQueues;
            }
            break;
        }

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
			//hopeRF.Write(data, dataSize);
			AssistNextChunkToSend = 0;
			break;
		}

		case 0x60: // new parameters received from GCS
		{
			// fill data
			SParameters params;
			if( dataSize == sizeof(params))
			{
				memcpy(&params, data, sizeof(params));

				// check CRC
				unsigned int crcSum = crc.CalculateCRC32((BYTE*)&params, sizeof(params)-sizeof(params.CRC32));
				//if( crcSum == params.CRC32) // Ignore CRC (LORA has its own CRC check
				{
					CopyStr2Params(params);
					// ACK
					ParamsCommandAckCnt++;
				}

				// Send ACK to LORA
				BYTE ackCode = 0x60;
                int bytesToSend = comm433MHz.GenerateTXPacket(0xA0, (BYTE*)&ackCode, 1, CommBuffer);
                serialU5.Write(CommBuffer, bytesToSend);
			}
			break;
		}

		case 0x61: // request params, send response
		{
			SParameters params;
			CopyParams2Str(params);

			// calc CRC
			params.CRC32 = crc.CalculateCRC32((BYTE*)&params, sizeof(params)-sizeof(params.CRC32)); // fill CRC code

			// send packet (type 0x62 - parameters)
			//etherDrv.SendPacket(0x62, (char*)&params, sizeof(params));

			// send to RF
			//hopeRF.Write((BYTE*)&params, sizeof(params));

			// Send to LORA
            int bytesToSend = comm433MHz.GenerateTXPacket(0x62, (BYTE*)&params, sizeof(params), CommBuffer);
            serialU5.Write(CommBuffer, bytesToSend);
            // Send ACK to LORA
            BYTE ackCode = 0x61;
            int bytesToSend2 = comm433MHz.GenerateTXPacket(0xA0, (BYTE*)&ackCode, 1, CommBuffer);
            serialU5.Write(CommBuffer, bytesToSend2);
			break;
		}

		case 0x63: // Store params to flash
		{
			if( dataSize == 1)
			{
				WriteParamsToFlash();
				// ACK
				ParamsCommandAckCnt++;

				// Send ACK to LORA
                BYTE ackCode = 0x63;
                int bytesToSend = comm433MHz.GenerateTXPacket(0xA0, (BYTE*)&ackCode, 1, CommBuffer);
                serialU5.Write(CommBuffer, bytesToSend);
			}
			break;
		}

		case 0x80: // Goto/Execute waypoints
		{
			// fill data
			SCommGotoExecute gotoExecuteCmd;
			if( dataSize == sizeof(gotoExecuteCmd))
			{
				memcpy(&gotoExecuteCmd, data, dataSize);

				if( gotoExecuteCmd.Command == 0x01 )
				{
					// goto target
					if( llConv.IsHomeSet())
					{
						float XN = 0;
						float XE = 0;
						llConv.ConvertLLToM(gotoExecuteCmd.TargetWaypoint.Latitude*1e-7, gotoExecuteCmd.TargetWaypoint.Longitude*1e-7, XN, XE);
						TargetAlt = gotoExecuteCmd.TargetWaypoint.Altitude;
						TargetN = XN;
						TargetE = XE;
					}
				}
				if( gotoExecuteCmd.Command == 0x02 )
				{
					// start waypoints
					TrajZNEVParams[3] = gotoExecuteCmd.Velocity; // waypoint velocity
					WayExecute = 1;
				}
				if( gotoExecuteCmd.Command == 0x03 )
				{
					// start orbit mode
					if( llConv.IsHomeSet())
					{
						float XN = 0;
						float XE = 0;
						llConv.ConvertLLToM(gotoExecuteCmd.TargetWaypoint.Latitude*1e-7, gotoExecuteCmd.TargetWaypoint.Longitude*1e-7, XN, XE);
						TrajZNEVParams[0] = gotoExecuteCmd.TargetWaypoint.Altitude;
						TrajZNEVParams[1] = XN;
						TrajZNEVParams[2] = XE;
						TrajZNEVParams[3] = gotoExecuteCmd.Velocity;
						WayExecute = 2;
					}
				}
				if( gotoExecuteCmd.Command == 10 )
				{
					// Abort waypoints
					WayExecute = 10;
				}
			}
			break;
		}

		case 0x81: // download waypoints
		{
			// fill data
			SCommWaypoints waypointDownloadCmd;
			memcpy(&waypointDownloadCmd, data, dataSize);

			if( llConv.IsHomeSet())
			{
				for(int i=0; i!= waypointDownloadCmd.WaypointCnt; i++)
				{
					float XN = 0;
					float XE = 0;
					llConv.ConvertLLToM(waypointDownloadCmd.waypoints[i].Latitude*1e-7, waypointDownloadCmd.waypoints[i].Longitude*1e-7, XN, XE);
					WayAlt[i] = waypointDownloadCmd.waypoints[i].Altitude;
					WayN[i] = XN;
					WayE[i] = XE;
				}
				WayCnt = waypointDownloadCmd.WaypointCnt;
				WayDownloadCnt++;
			}
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

			// Send ACK to LORA
            BYTE ackCode = 0x90;
            int bytesToSend = comm433MHz.GenerateTXPacket(0xA0, (BYTE*)&ackCode, 1, CommBuffer);
            serialU5.Write(CommBuffer, bytesToSend);

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
	serialGPS.Init(UART2_BASE, 57600); // open with 57k (115k doesn't work well??? small int FIFO, wrong INT prio?)'
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



///////////////////
// PARAMS/FLASH
///////////////////
void CopyParams2Str(SParameters& params)
{
	params.GyroOffX = GyroOffX;
	params.GyroOffY = GyroOffY;
	params.GyroOffZ = GyroOffZ;
	params.MagOffX = MagOffX;
	params.MagOffY = MagOffY;
	params.MagOffZ = MagOffZ;
	params.AttOffRoll = AttOffRoll;
	params.AttOffPitch = AttOffPitch;
	params.RollMax = RollMax;
	params.RollKp = RollKp;
	params.RollKi = RollKi;
	params.RollKd = RollKd;
	params.PitchMax = PitchMax;
	params.PitchKp = PitchKp;
	params.PitchKi = PitchKi;
	params.PitchKd = PitchKd;
}

void CopyStr2Params(SParameters& params)
{
	GyroOffX = params.GyroOffX;
	GyroOffY = params.GyroOffY;
	GyroOffZ = params.GyroOffZ;
	MagOffX = params.MagOffX;
	MagOffY = params.MagOffY;
	MagOffZ = params.MagOffZ;
	AttOffRoll = params.AttOffRoll;
	AttOffPitch = params.AttOffPitch;
	RollMax = params.RollMax;
	RollKp = params.RollKp;
	RollKi = params.RollKi;
	RollKd = params.RollKd;
	PitchMax = params.PitchMax;
	PitchKp = params.PitchKp;
	PitchKi = params.PitchKi;
	PitchKd = params.PitchKd;

}


void ReadParamsFromFlash()
{
	SParameters params;
	if( eeprom.ReadParamsFromFlash(&params, sizeof(params)) )
	{
		// copy params to vars
		CopyStr2Params(params);
	}
}

void WriteParamsToFlash()
{
	SParameters params;

	// fill eeprom structure
	CopyParams2Str(params);

	eeprom.WriteParamsFromFlash(&params, sizeof(params));
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
	//hopeRF.IntHandler();
}

extern "C" void SysTickIntHandler(void)
{
	SysTickIntHit = true;
}
