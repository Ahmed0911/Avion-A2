using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Net;
using System.Net.Sockets;
using System.IO;
using System.Diagnostics;
using System.IO.Ports;

namespace WinEthApp
{
    public partial class FormMain : Form
    {
        MainSystem mainSystem;
        Navigation navigation;
        

        // Filtered Data (RPY)
        public double FilteredRoll;
        public double FilteredPitch;
        public double FilteredYaw;

        public double[] FilteredTuningLastData = new double[10];
        public double[] FilteredTuningData = new double[10];

        // Params read/write
        public int ParamsCommandAckCntExpected = 0;
        public int ParamsCommandAckCntReceived = 0;

        public FormMain()
        {
            InitializeComponent();

            mainSystem = new MainSystem(this);

            navigation = new Navigation(this, mainSystem);
            comboBoxMapSelector.SelectedIndex = comboBoxMapSelector.Items.Count-1; // Load default map         
            radioButtonHopeRF.Checked = true;
               
            // Fill Default Params
            labelParam1.Text = "GyroOffX";
            labelParam2.Text = "GyroOffY";
            labelParam3.Text = "GyroOffZ";
            labelParam4.Text = "MagOffX";
            labelParam5.Text = "MagOffY";
            labelParam6.Text = "MagOffZ";
            labelParam7.Text = "AttOffRoll";
            labelParam8.Text = "AttOffPitch";
            labelParam9.Text = "RollMax";
            labelParam10.Text = "RollKp";
            labelParam11.Text = "RollKi";
            labelParam12.Text = "RollKd";
            labelParam13.Text = "PitchMax";
            labelParam14.Text = "PitchKp";
            labelParam15.Text = "PitchKi";
            labelParam16.Text = "PitchKd";
            labelParam17.Text = "";

            // Serial port stuff
            comboBoxSerialPorts.Items.AddRange(SerialPort.GetPortNames());
            if (comboBoxSerialPorts.Items.Count > 0) comboBoxSerialPorts.SelectedIndex = 0;
        }

        private void buttonConnectSystem_Click(object sender, EventArgs e)
        {
            mainSystem.Connect(textBoxAddressSystem.Text);
            buttonConnectSystem.Enabled = false;
            buttonConnectSystem.Text = "Opened";
        }

        private void timerCommUpdate_Tick(object sender, EventArgs e)
        {
            mainSystem.Update();
            navigation.Update();
            //gateway.Update();

            // update
            // update data            
            if (mainSystem.commMgr.serialPortComm.comm433MHz != null)
            {
                textBoxCommMsgOK.Text = mainSystem.commMgr.serialPortComm.comm433MHz.MsgReceivedOK.ToString();
                textBoxCommCRCErrors.Text = mainSystem.commMgr.serialPortComm.comm433MHz.CrcErrors.ToString();
                textBoxCommHeaderErrors.Text = mainSystem.commMgr.serialPortComm.comm433MHz.HeaderFails.ToString();
                textBoxTimeoutCnt.Text = mainSystem.commMgr.TimeoutCounter.ToString();
            }

            // REMOVE START
            // dummy, REMOVE ME
            /*float altitude = (float)((Math.Cos(T+0.3)+1)*50);
            double N = Math.Cos(T) * 20;
            double E = Math.Sin(T) * 20;
            T += 0.1;
            gimbaledTracker.UpdateTargetPosition(altitude, N, E);
            textBoxGWTrackerTargetN.Text = string.Format("{0:0.00} m", N);
            textBoxGWTrackerTargetE.Text = string.Format("{0:0.00} m", E);
            textBoxGWTrackerTargetZ.Text = string.Format("{0:0.00} m", altitude);*/
            // REMOVE END
        }       

        private void buttonLocAssist_Click(object sender, EventArgs e)
        {
            mainSystem.ExecuteAssistNow();            
        }

        private void buttonWpnCommand_Click(object sender, EventArgs e)
        {
            if (sender == buttonWpnArm1)
            {
                if( buttonWpnArm1.Text == "Arm") 
                {
                    buttonWpnArm1.Text = "Dearm";
                    mainSystem.WpnCommand(0, 1);
                }
                else
                {
                    buttonWpnArm1.Text = "Arm";
                    mainSystem.WpnCommand(0, 3);
                }
            }
            if (sender == buttonWpnArm2)
            {
                if( buttonWpnArm2.Text == "Arm") 
                {
                    buttonWpnArm2.Text = "Dearm";
                    mainSystem.WpnCommand(1, 1);
                }
                else
                {
                    buttonWpnArm2.Text = "Arm";
                    mainSystem.WpnCommand(1, 3);
                }
            }
            if( sender == buttonWpnFire1 )
            {
                mainSystem.WpnCommand(0, 2);
            }
            if (sender == buttonWpnFire2)
            {
                mainSystem.WpnCommand(1, 2);
            }
        }  

        // Parameters
        private void buttonReadParams_Click(object sender, EventArgs e)
        {
            mainSystem.ReadParams();            
        }
        private void buttonWriteParams_Click(object sender, EventArgs e)
        {
            // Get data from form
            SParameters p = new SParameters();
            p.GyroOffX = float.Parse(textBoxParam1.Text);
            p.GyroOffY = float.Parse(textBoxParam2.Text);
            p.GyroOffZ = float.Parse(textBoxParam3.Text);
            p.MagOffX = float.Parse(textBoxParam4.Text);
            p.MagOffY = float.Parse(textBoxParam5.Text);
            p.MagOffZ = float.Parse(textBoxParam6.Text);
            p.AttOffRoll = float.Parse(textBoxParam7.Text);
            p.AttOffPitch = float.Parse(textBoxParam8.Text);
            p.RollMax = float.Parse(textBoxParam9.Text);
            p.RollKp = float.Parse(textBoxParam10.Text);
            p.RollKi = float.Parse(textBoxParam11.Text);
            p.RollKd = float.Parse(textBoxParam12.Text);
            p.PitchMax = float.Parse(textBoxParam13.Text);
            p.PitchKp = float.Parse(textBoxParam14.Text);
            p.PitchKi = float.Parse(textBoxParam15.Text);
            p.PitchKd = float.Parse(textBoxParam16.Text);

            // Add CRC
            byte[] data = Comm.GetBytes(p);
            p.CRC32 = Crc32.CalculateCrc32(data, data.Length - sizeof(uint));      

            mainSystem.WriteParams(p);
            ParamsCommandAckCntExpected = ParamsCommandAckCntReceived + 1;
        }

        private void buttonStoreToFlash_Click(object sender, EventArgs e)
        {
            mainSystem.StoreToFlashParams();
            ParamsCommandAckCntExpected = ParamsCommandAckCntReceived + 1;
        }    

        public void DisplayParams(SParameters p)
        {
            textBoxParam1.Text = p.GyroOffX.ToString();
            textBoxParam2.Text = p.GyroOffY.ToString();
            textBoxParam3.Text = p.GyroOffZ.ToString();
            textBoxParam4.Text = p.MagOffX.ToString();
            textBoxParam5.Text = p.MagOffY.ToString();
            textBoxParam6.Text = p.MagOffZ.ToString();
            textBoxParam7.Text = p.AttOffRoll.ToString();
            textBoxParam8.Text = p.AttOffPitch.ToString();
            textBoxParam9.Text = p.RollMax.ToString();
            textBoxParam10.Text = p.RollKp.ToString();
            textBoxParam11.Text = p.RollKi.ToString();
            textBoxParam12.Text = p.RollKd.ToString();
            textBoxParam13.Text = p.PitchMax.ToString();
            textBoxParam14.Text = p.PitchKp.ToString();
            textBoxParam15.Text = p.PitchKi.ToString();
            textBoxParam16.Text = p.PitchKd.ToString();

            UpdateTrackBars(p);
        }        

        public unsafe void DisplaySystemData(int ReceivedPacketsCounter, SCommEthData MainSystemData)
        {
            if (radioButtonWifi.Checked || radioButtonWifiHopeRF.Checked)
            {
                // GPS
                textBoxLocTime.Text = MainSystemData.GPSTime.ToString();
                textBoxLocSatNr.Text = MainSystemData.NumSV.ToString();
                textBoxNavLocSatNr.Text = MainSystemData.NumSV.ToString();
                textBoxLocLat.Text = (MainSystemData.Latitude * 1e-7).ToString("0.000000");
                textBoxLocLon.Text = (MainSystemData.Longitude * 1e-7).ToString("0.000000");
                textBoxHomeLat.Text = (MainSystemData.HomeLatitude * 1e-7).ToString("0.000000");
                textBoxHomeLon.Text = (MainSystemData.HomeLongitude * 1e-7).ToString("0.000000");
                textBoxLocVelocityN.Text = (MainSystemData.VelN / 1000.0).ToString("0.00 m/s");
                textBoxLocVelocityE.Text = (MainSystemData.VelE / 1000.0).ToString("0.00 m/s");
                textBoxLocPressure.Text = (MainSystemData.Pressure / 100).ToString("0.00 hPa");
                textBoxLocTemperature.Text = (MainSystemData.Temperature).ToString("0.0 °C");
                textBoxLocAlt.Text = (MainSystemData.Altitude).ToString("0.0 m");
                textBoxLocVertSpeed.Text = (MainSystemData.Vertspeed).ToString("0.0 m/s");
                textBoxLocAssist.Text = MainSystemData.AssistNextChunkToSend.ToString();
                string cnos1 = "";
                string cnos2 = "";
                for (int i = 0; i != 16; i++)
                {
                    cnos1 += string.Format("{0}", MainSystemData.SatCNOs[i]) + Environment.NewLine;
                    cnos2 += string.Format("{0}", MainSystemData.SatCNOs[i + 16]) + Environment.NewLine;
                }
                textBoxSatCNOs1.Text = cnos1;
                textBoxSatCNOs2.Text = cnos2;

                // Attitude
                textBoxRoll.Text = MainSystemData.Roll.ToString("0.0 °");
                textBoxPitch.Text = MainSystemData.Pitch.ToString("0.0 °");
                textBoxYaw.Text = MainSystemData.Yaw.ToString("0.0 °");
                textBoxAccX.Text = MainSystemData.AccX.ToString("0.0 m/s2");
                textBoxAccY.Text = MainSystemData.AccY.ToString("0.0 m/s2");
                textBoxAccZ.Text = MainSystemData.AccZ.ToString("0.0 m/s2");
                textBoxMagX.Text = MainSystemData.MagX.ToString("0.0 uT");
                textBoxMagY.Text = MainSystemData.MagY.ToString("0.0 uT");
                textBoxMagZ.Text = MainSystemData.MagZ.ToString("0.0 uT");

                textBoxdRoll.Text = MainSystemData.dRoll.ToString("0.0 °/s");
                textBoxdPitch.Text = MainSystemData.dPitch.ToString("0.0 °/s");
                textBoxdYaw.Text = MainSystemData.dYaw.ToString("0.0 °/s");

                textBoxdRollFiltered.Text = FilteredRoll.ToString("0.000 °/s");
                textBoxdPitchFiltered.Text = FilteredPitch.ToString("0.000 °/s");
                textBoxdYawFiltered.Text = FilteredYaw.ToString("0.000 °/s");

                // System
                textBoxSystemEthFrames.Text = ReceivedPacketsCounter.ToString();
                textBoxSystemEthSent.Text = MainSystemData.EthSentCount.ToString();
                textBoxSystemEthReceived.Text = MainSystemData.EthReceivedCount.ToString();

                textBoxSystemLoop.Text = MainSystemData.LoopCounter.ToString();
                textBoxSystemLoopTime.Text = MainSystemData.PerfLoopTimeMS.ToString("0.000 ms");
                textBoxSystemCPUTime.Text = MainSystemData.PerfCpuTimeMS.ToString("0.000 ms");
                textBoxSystemCPUTimeMax.Text = MainSystemData.PerfCpuTimeMSMAX.ToString("0.000 ms");
                textBoxSystemBattery.Text = MainSystemData.BatteryVoltage.ToString("0.00 V");
                textBoxSystemBatteryPercent.Text = MainSystemData.FuelLevel.ToString("0.0 ") + "%";
                textBoxNavigationBatteryPercent.Text = MainSystemData.FuelLevel.ToString("0.0 ") + "%";
                textBoxSystemMode.Text = MainSystemData.ActualMode.ToString();
                textBoxNavigationMode.Text = MainSystemData.ActualMode.ToString();
                textBoxSystemT1.Text = MainSystemData.MotorThrusts[0].ToString();
                textBoxSystemT2.Text = MainSystemData.MotorThrusts[1].ToString();
                textBoxSystemT3.Text = MainSystemData.MotorThrusts[2].ToString();
                textBoxSystemT4.Text = MainSystemData.MotorThrusts[3].ToString();
                textBoxSystemBatteryCurrentA.Text = MainSystemData.BatteryCurrentA.ToString("0.0 A");
                textBoxSystemBatteryTotalCharge_mAH.Text = MainSystemData.BatteryTotalCharge_mAh.ToString("0 mAh");

                // HopeRF
                textBoxHopeRXCount.Text = MainSystemData.HopeRXFrameCount.ToString();
                textBoxHopeRSSI.Text = MainSystemData.HopeRSSI.ToString();
                textBoxHopeRXRSSI.Text = MainSystemData.HopeRXRSSI.ToString();

                textBoxWaypointsCnt.Text = MainSystemData.WaypointCnt.ToString();
                textBoxWaypointsDownloadCnt.Text = MainSystemData.WaypointDownloadCounter.ToString();

                // Download map
                textBoxMapDownloadLat.Text = (MainSystemData.Latitude * 1e-7).ToString("0.000000");
                textBoxMapDownloadLon.Text = (MainSystemData.Longitude * 1e-7).ToString("0.000000");

                // Launch
                string[] launchStatus = { "Idle", "Armed", "Firing" };
                textBoxWpnStatus1.Text = launchStatus[MainSystemData.LaunchStatus1];
                textBoxWpnStatus2.Text = launchStatus[MainSystemData.LaunchStatus2];

                // Update Home
                navigation.SetHome(MainSystemData.HomeLongitude * 1e-7, MainSystemData.HomeLatitude * 1e-7);
                // Update current position
                navigation.UpdateCurrentPosition(MainSystemData.Altitude, MainSystemData.Longitude * 1e-7, MainSystemData.Latitude * 1e-7, MainSystemData.ActualMode);
            }               
        }
        public unsafe void DisplayRelayedData(SCommHopeRFDataA2Avion RelayedData)
        {
            if (radioButtonHopeRF.Checked || radioButtonWifiHopeRF.Checked)
            {
                // GPS
                textBoxLocSatNr.Text = RelayedData.NumSV.ToString();
                textBoxNavLocSatNr.Text = RelayedData.NumSV.ToString();
                textBoxLocLat.Text = (RelayedData.Latitude * 1e-7).ToString("0.000000");
                textBoxLocLon.Text = (RelayedData.Longitude * 1e-7).ToString("0.000000");
                textBoxLocVelocityN.Text = (RelayedData.VelN / 1000.0).ToString("0.00 m/s");
                textBoxLocVelocityE.Text = (RelayedData.VelE / 1000.0).ToString("0.00 m/s");
                textBoxLocAlt.Text = (RelayedData.Altitude).ToString("0.0 m");
                textBoxLocVertSpeed.Text = (RelayedData.Vertspeed).ToString("0.0 m/s");

                // Attitude
                textBoxRoll.Text = RelayedData.Roll.ToString("0.0 °");
                textBoxPitch.Text = RelayedData.Pitch.ToString("0.0 °");
                textBoxYaw.Text = RelayedData.Yaw.ToString("0.0 °");
                textBoxdRoll.Text = RelayedData.dRoll.ToString("0.0 °/s");
                textBoxdPitch.Text = RelayedData.dPitch.ToString("0.0 °/s");
                textBoxdYaw.Text = RelayedData.dYaw.ToString("0.0 °/s");

                textBoxdRollFiltered.Text = FilteredRoll.ToString("0.000 °/s");
                textBoxdPitchFiltered.Text = FilteredPitch.ToString("0.000 °/s");
                textBoxdYawFiltered.Text = FilteredYaw.ToString("0.000 °/s");

                // System
                textBoxSystemLoop.Text = RelayedData.LoopCounter.ToString();
                textBoxSystemBattery.Text = RelayedData.BatteryVoltage.ToString("0.00 V");
                textBoxSystemMode.Text = RelayedData.ActualMode.ToString();
                textBoxNavigationMode.Text = RelayedData.ActualMode.ToString();
                textBoxSystemBatteryCurrentA.Text = RelayedData.BatteryCurrentA.ToString("0.0 A");
                textBoxSystemBatteryTotalCharge_mAH.Text = RelayedData.BatteryTotalCharge_mAh.ToString("0 mAh");

                // HopeRF
                textBoxHopeRXCount.Text = RelayedData.HopeRXFrameCount.ToString();
                textBoxHopeRSSI.Text = RelayedData.HopeTXRSSI.ToString();
                textBoxHopeRXRSSI.Text = RelayedData.HopeRXRSSI.ToString();

                // Motors
                textBoxSystemT1.Text = RelayedData.MotorThrusts[0].ToString();
                textBoxSystemT2.Text = RelayedData.MotorThrusts[1].ToString();
                textBoxSystemT3.Text = RelayedData.MotorThrusts[2].ToString();
                textBoxSystemT4.Text = RelayedData.MotorThrusts[3].ToString();

                // Download map
                textBoxMapDownloadLat.Text = (RelayedData.Latitude * 1e-7).ToString("0.000000");
                textBoxMapDownloadLon.Text = (RelayedData.Longitude * 1e-7).ToString("0.000000");

                // Update current position
                navigation.UpdateCurrentPosition(RelayedData.Altitude, RelayedData.Longitude * 1e-7, RelayedData.Latitude * 1e-7, RelayedData.ActualMode);

                // update Params Command Ack
                ParamsCommandAckCntReceived = RelayedData.ParamsCommandAckCnt;
                textBoxParamsStateNum.Text = string.Format("{0}/{1}", ParamsCommandAckCntReceived, ParamsCommandAckCntExpected);
                if (ParamsCommandAckCntReceived == ParamsCommandAckCntExpected)
                {
                    textBoxParamsState.BackColor = Color.Green;
                    textBoxParamsState.Text = "OK";
                }
                else
                {
                    textBoxParamsState.BackColor = Color.Red;
                    textBoxParamsState.Text = "FAIL";
                }
            }
        }
        
        // Send data to Wifi/HopeRF
        public void SendData(byte type, byte[] buffer)
        {
            if( radioButtonWifi.Checked || radioButtonWifiHopeRF.Checked)
            {
                mainSystem.SendData(type, buffer);
            }
            if (radioButtonHopeRF.Checked || radioButtonWifiHopeRF.Checked)
            {
                mainSystem.QueueSerial(type, buffer);
            }
        }

        private void pictureBoxMap_Click(object sender, EventArgs e)
        {
            Point point = pictureBoxMap.PointToClient(Cursor.Position);
            navigation.OnMouseClick(point);            
        }

        private void buttonNavDownloadDelete_Click(object sender, EventArgs e)
        {
            navigation.DeleteWaypoint();
        }

        private void buttonNavDownloadWaypoints_Click(object sender, EventArgs e)
        {
            navigation.DownloadWaypoints();
        }

        private void buttonNavigationGoHome_Click(object sender, EventArgs e)
        {
            navigation.GoHome();
        }

        private void buttonNavExecuteWaypoints_Click(object sender, EventArgs e)
        {
            float Velocity;
            if (float.TryParse(comboBoxVelocity.Text, out Velocity) == false)
            {
                Velocity = 3; //default
            }

            navigation.ExecuteWaypoints(Velocity);
        }

        private void buttonNavAbortWaypoints_Click(object sender, EventArgs e)
        {
            navigation.AbortWaypoints();
        }

        private void buttonMapDownload_Click(object sender, EventArgs e)
        {
            try
            {                
                double latitude = double.Parse(textBoxMapDownloadLat.Text);
                double longitude = double.Parse(textBoxMapDownloadLon.Text);
                int zoom = int.Parse(comboBoxMapDownloadZoom.SelectedItem.ToString().Split(':')[0]);

                navigation.DownloadMap(latitude, longitude, zoom);

            }
            catch(Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }        

        private void comboBoxMapSelector_SelectedIndexChanged(object sender, EventArgs e)
        {
            navigation.LoadMap(comboBoxMapSelector.SelectedItem.ToString());
        }      

        private void buttonNavigationWaypointsSave_Click(object sender, EventArgs e)
        {
            if (sender == buttonNavigationWaypointsLoad) navigation.LoadWaypoints();
            else navigation.SaveWaypoints();
        }

        private void trackBarParams_Scroll(object sender, EventArgs e)
        {
            // recalculate slider param values
            double RollKp = trackBarParamsRollKp.Value / 20.0 * 0.1;
            double RollKi = trackBarParamsRollKi.Value / 20.0 * 0.02;
            double RollKd = trackBarParamsRollKd.Value / 20.0 * 0.02;

            double PitchKp = trackBarParamsPitchKp.Value / 20.0 * 0.1;
            double PitchKi = trackBarParamsPitchKi.Value / 20.0 * 0.02;
            double PitchKd = trackBarParamsPitchKd.Value / 20.0 * 0.02;

            textBoxParam10.Text = RollKp.ToString();
            textBoxParam11.Text = RollKi.ToString();
            textBoxParam12.Text = RollKd.ToString();
            textBoxParam14.Text = PitchKp.ToString();
            textBoxParam15.Text = PitchKi.ToString();
            textBoxParam16.Text = PitchKd.ToString();
        }

        private void UpdateTrackBars(SParameters p)
        {
            int RollKp = (int)(p.RollKp / 0.1 * 20);
            int RollKi = (int)(p.RollKi / 0.02 * 20);
            int RollKd = (int)(p.RollKd / 0.02 * 20);

            int PitchKp = (int)(p.PitchKp / 0.1 * 20);
            int PitchKi = (int)(p.PitchKi / 0.02 * 20);
            int PitchKd = (int)(p.PitchKd / 0.02 * 20);

            trackBarParamsRollKp.Value = RollKp;
            trackBarParamsRollKi.Value = RollKi;
            trackBarParamsRollKd.Value = RollKd;

            trackBarParamsPitchKp.Value = PitchKp;
            trackBarParamsPitchKi.Value = PitchKi;
            trackBarParamsPitchKd.Value = PitchKd;   
        }

        private void buttonSerialOpen_Click(object sender, EventArgs e)
        {
            mainSystem.ConnectToSerial((string)comboBoxSerialPorts.SelectedItem);
            buttonSerialOpen.Enabled = false;
        }
    }
}
