using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace WinEthApp
{
    public unsafe struct SCommEthData
    {
        public uint LoopCounter;

        // IMU
        public float Roll;     // [deg]
        public float Pitch;    // [deg]
        public float Yaw;      // [deg]
        public float dRoll;    // [deg/s]
        public float dPitch;   // [deg/s]
        public float dYaw;     // [deg/s]

        public float AccX; // [m/s^2]
        public float AccY; // [m/s^2]
        public float AccZ; // [m/s^2]
        public float MagX; // [uT]
        public float MagY; // [uT]
        public float MagZ; // [uT]

        // alt/speed
        public float Pressure; // [Pa]
        public float Temperature;// [°C]
        public float Altitude; // [m]
        public float Vertspeed;// [m/s]        

        // engines
        public fixed byte MotorThrusts[4]; // [0...100%]

        public float FuelLevel; // [0...100%]
        public float BatteryVoltage; // [V]
        public float BatteryCurrentA; // [A]
        public float BatteryTotalCharge_mAh; // [mAh]

        // GPS
        public uint GPSTime;
        public byte NumSV;
        public byte FixType;
        public byte FixFlags; // FIX Flags        
        public byte ActualMode; // Non GPS, but used for 32bit alignment
        public int Longitude; // 1e-7 [deg]
        public int Latitude; // 1e-7 [deg]
        public int HeightMSL; // MSL [mm]
        public uint HorizontalAccuracy; // [mm]
        public uint VerticalAccuracy; // [mm]
        public int VelN; // Speed North [mm/s]
        public int VelE; // Speed East [mm/s]
        public int VelD; // Speed Down [mm/s]
        public uint SpeedAcc; // Speed accuracy [mm/s]

        // Comm Eth
        public uint EthSentCount;
        public uint EthReceivedCount;
        // Comm HopeRF
        public uint HopeRXFrameCount;
        public int HopeRXRSSI;
        public int HopeRSSI;

        // Perf Stuff
        public float PerfLoopTimeMS;
        public float PerfCpuTimeMS;
        public float PerfCpuTimeMSMAX;

        // AssistNow Data
        public int AssistNextChunkToSend;

        // Tuning Data
	    public fixed float TuningData[10];

        // SAT cnos
        public fixed byte SatCNOs[32]; // 32 satellites cno [dB]
        
        // Waypoints
        public int WaypointCnt;
        public int WaypointDownloadCounter;

        public int HomeLongitude; // 1e-7 [deg]
        public int HomeLatitude; // 1e-7 [deg]

        // Launch
	    public short LaunchStatus1;
        public short LaunchStatus2;
    }

    public struct SCommHopeRFData
    {
	    public uint LoopCounter;
	    // IMU
	    public float Roll;     // [deg]
        public float Pitch;    // [deg]
        public float Yaw;      // [deg]

	    // alt/speed
	    public float Altitude; // [m]
	    public float Vertspeed;// [m/s]

	    public float FuelLevel; // [0...100%]
	    public float BatteryVoltage; // [V]

	    // GPS
        public byte NumSV;
        public byte FixType;
        public byte FixFlags; // FIX Flags        
        public byte ActualMode; // Non GPS, but used for 32bit alignment
	    public int Longitude; // 1e-7 [deg]
	    public int Latitude; // 1e-7 [deg]
	    public int VelN; // Speed North [mm/s]
	    public int VelE; // Speed East [mm/s]

	    // Comm HopeRF
	    public uint HopeRXFrameCount;
        public int HopeRXRSSI;
        public int HopeTXRSSI;
    };

    public unsafe struct SCommHopeRFDataA2Avion
    {
        public uint LoopCounter;

        // IMU
        public float Roll;		// [deg]
        public float Pitch;	// [deg]
        public float Yaw;		// [deg]
        public float dRoll;	// [deg/s]
        public float dPitch;	// [deg/s]
        public float dYaw;		// [deg/s]

        // alt/speed
        public float Altitude; // [m]
        public float Vertspeed;// [m/s]

        // engines
        public fixed byte MotorThrusts[4]; // [0...100%]

        // Fuel
        public float BatteryVoltage; // [V]
        public float BatteryCurrentA; // [A]
        public float BatteryTotalCharge_mAh; // [mAh]

        // GPS
        public byte NumSV;
        public byte FixType;
        public byte ParamsCommandAckCnt; // Sync for params R/W        
        public byte ActualMode; // Non GPS, but used for 32bit alignment
        public int Longitude; // 1e-7 [deg]
        public int Latitude; // 1e-7 [deg]
        public int VelN; // Speed North [mm/s]
        public int VelE; // Speed East [mm/s]
        public uint HorizontalAccuracy; // [mm]

        // SDCard
        public uint SDCardBytesWritten;
        public uint SDCardFails;
        public uint FailedQueues;

        // Comm HopeRF
        public uint HopeRXFrameCount;
        public int HopeRXRSSI; // Received RSSI at A2 Plane (on received command)
        public int HopeTXRSSI; // dummy, filled in GCS App

	    // CRC
	    public uint CRC32;
    };

    public struct SWpt
    {
	    public float Altitude;
	    public int Latitude; // 1e-7 [deg]
	    public int Longitude; // 1e-7 [deg]
    };

    public struct SCommWaypoints
    {
	    public uint WaypointCnt;
        [MarshalAs(UnmanagedType.ByValArray, ArraySubType = UnmanagedType.LPStruct, SizeConst = 8)]
	    public SWpt[] waypoints;
    };

    public struct SCommGotoExecute
    {
	    public byte Command;  // 1 - Execute Target, 2 - Start Waypoints, 3 - Orbit, ..., 10 - Abort
        public SWpt TargetWaypoint;
        public float Velocity;
    };

    public struct SParameters
    {
        public float GyroOffX;
        public float GyroOffY;
        public float GyroOffZ;
        public float MagOffX;
        public float MagOffY;
        public float MagOffZ;
        public float AttOffRoll;
        public float AttOffPitch;
        public float RollMax;
        public float RollKp;
        public float RollKi;
        public float RollKd;
        public float PitchMax;
        public float PitchKp;
        public float PitchKi;
        public float PitchKd;

        // CRC
        public uint CRC32;
    };

    struct SCommLaunch
    {
        public byte Command;
        public byte Index;
        public byte _dummy1;
        public byte _dummy2;
        public uint CodeTimer;
    };

    struct SCommTrackerCommands
    {
        public int Mode; // 0 - disabed, 1 - manual (pan/tilt [CNT]), 2 - semi auto, 3 - auto
        public int PanRef;
        public int TiltRef;
    };

    class Comm
    {
        public static byte[] GetBytes(object str)
        {
            int size = Marshal.SizeOf(str);
            byte[] arr = new byte[size];

            IntPtr ptr = Marshal.AllocHGlobal(size);
            Marshal.StructureToPtr(str, ptr, true);
            Marshal.Copy(ptr, arr, 0, size);
            Marshal.FreeHGlobal(ptr);
            return arr;
        }

        public static object FromBytes(byte[] arr, object str)
        {
            int size = Marshal.SizeOf(str);
            IntPtr ptr = Marshal.AllocHGlobal(size);

            Marshal.Copy(arr, 0, ptr, size);

            str = Marshal.PtrToStructure(ptr, str.GetType());
            Marshal.FreeHGlobal(ptr);

            return str;
        }        
    }
}
