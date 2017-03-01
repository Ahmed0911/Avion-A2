using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace WinEthApp
{
    // MAP Download: http://dev.virtualearth.net/REST/v1/Imagery/Map/Aerial/45.787469,15.889805/16?mapSize=1000,800&key=ArKPcICqyZdj1wm2mt8QIgEtCOC3sZ-2DX_x62KKjpb-DiqT_77sQe0FiEhVOAnJ
    struct SWaypoint
    {
	    public float Altitude;
	    public double Latitude;
	    public double Longitude;	
    };
    class Navigation
    {
        private FormMain formMain;
        private MainSystem mainSystem;

        public LLConverter llConv = new LLConverter();

        private const int MAXWAYPOINTS = 8;

        // Map
        Image mapBitmap;
        private double MetersPerPix;
        private double ZoomLevel;
        private double CenterLatitude;
        private double CenterLongitude;
        private double HomeLatitude;
        private double HomeLongitude;
        private bool HomeSet = false;
        private Size MapImageSizePix;
        private int CircleSize = 0;

        private Font fontWaypoints;
        private Font fontHome;

        // Current position
        SWaypoint CurrentPosition;
        
        // Waypoints
        List<SWaypoint> Waypoints = new List<SWaypoint>();
        SWaypoint TargetWaypoint;
        private int ActiveWaypoint = -1;

        // Double buffering
        BufferedGraphicsContext DBcurrentContext;
        BufferedGraphics DBmyBuffer;

        public Navigation(FormMain formMain, MainSystem mainSystem)
        {
            this.formMain = formMain;
            this.mainSystem = mainSystem;

            // Create DB objects
            DBcurrentContext = BufferedGraphicsManager.Current;
            DBmyBuffer = DBcurrentContext.Allocate(formMain.pictureBoxMap.CreateGraphics(), formMain.pictureBoxMap.DisplayRectangle);
            fontWaypoints = new Font("Arial", 14, FontStyle.Regular);
            fontHome = new Font("Arial", 24, FontStyle.Regular);

            formMain.comboBoxMapDownloadZoom.SelectedIndex = 3;
            EnumerateMaps("Map");

            formMain.comboBoxVelocity.SelectedIndex = 4;

            // TEST
            SetHome(15.88388, 45.80349); // REMOVE ME!!!
            UpdateCurrentPosition(21.4f, 15.88288, 45.80449, 2); // REMOVE ME!!!
        }

        private void EnumerateMaps(string mapFolder)
        {
            formMain.comboBoxMapSelector.Items.Clear();
            string[] maps = Directory.EnumerateFiles(mapFolder, "*-X.jpg").ToArray();
            formMain.comboBoxMapSelector.Items.AddRange(maps);
        }

        public void DownloadMap(double latitude, double longitude, int zoom)
        {
            Point imgSize = new Point(800, 600);
            string downloadString = string.Format("http://dev.virtualearth.net/REST/v1/Imagery/Map/Aerial/{0},{1}/{2}?mapSize={3},{4}&key=ArKPcICqyZdj1wm2mt8QIgEtCOC3sZ-2DX_x62KKjpb-DiqT_77sQe0FiEhVOAnJ", latitude, longitude, zoom, imgSize.X, imgSize.Y);

            WebClient wb = new WebClient();
            string outputName = string.Format("Map\\map-{0}-{1}-{2}-X.jpg", latitude, longitude, zoom);
            wb.DownloadFile(downloadString, outputName);

            EnumerateMaps("Map");
            formMain.comboBoxMapSelector.SelectedItem = outputName;
        }

        public void LoadMap(string mapFilname)
        {
            mapBitmap = Bitmap.FromFile(mapFilname);            

            // get data
            string[] toks = mapFilname.Split('-');
            CenterLatitude = double.Parse(toks[1]);
            CenterLongitude = double.Parse(toks[2]);
            ZoomLevel = double.Parse(toks[3]);           

            MapImageSizePix = mapBitmap.Size;
            MetersPerPix = 156543.04 * Math.Cos(CenterLatitude / 180 * Math.PI) / (Math.Pow(2, ZoomLevel));

            FillWaypointList(0);
        }

        public void Update()
        {
            DrawMap();

            // update flightplan distance/data
            double speed = 5;
            double distance = CalculateFlightPlanDistanceMeters();
            double time = distance/speed/60;
            formMain.textBoxWaypointsDistance.Text = string.Format("{0:0.0} m, {1:0.0} min", distance, time);
        }

        private double CalculateFlightPlanDistanceMeters()
        {
            double distance = 0;
            if ( Waypoints.Count > 1)
            {
                for(int i=0; i!= Waypoints.Count-1; i++)
                {
                    double distanceXY = LLDistanceM(Waypoints[i].Longitude, Waypoints[i].Latitude, Waypoints[i + 1].Longitude, Waypoints[i + 1].Latitude);
                    double distanceAlt = Math.Abs(Waypoints[i].Altitude - Waypoints[i + 1].Altitude);
                    double totalDistance = Math.Sqrt(distanceXY * distanceXY + distanceAlt * distanceAlt);
                    distance = distance + totalDistance;
                }
            }

                return distance;
        }

        public void SaveWaypoints()
        {
            StreamWriter sw = File.CreateText("waypoints.txt");
            foreach(SWaypoint w in Waypoints)
            {
                sw.WriteLine("{0} {1} {2}", w.Altitude, w.Latitude, w.Longitude);
            }
            sw.Close();
        }

        public void LoadWaypoints()
        {
            Waypoints.Clear();
            try
            {
                StreamReader sr = File.OpenText("waypoints.txt");
                while(!sr.EndOfStream)
                {
                    string s = sr.ReadLine();                   
                    string[] t = s.Split(' ');
                    SWaypoint w = new SWaypoint();
                    w.Altitude = float.Parse(t[0]);
                    w.Latitude = double.Parse(t[1]);
                    w.Longitude = double.Parse(t[2]);

                    Waypoints.Add(w);
                }
                sr.Close();
                FillWaypointList(0);
            }
            catch (Exception)
            { }
        }

        private bool AddWaypoint(float altitude, double latitude, double longitude)
        {
            if (formMain.listViewWaypoints.SelectedIndices.Count == 0) return false;
            SWaypoint wp;
	        wp.Altitude = altitude;
	        wp.Latitude = latitude;
	        wp.Longitude = longitude;

            int index = formMain.listViewWaypoints.SelectedIndices[0];

            if (index < Waypoints.Count) Waypoints[index] = wp;
            else if (index == Waypoints.Count && Waypoints.Count < MAXWAYPOINTS)
            {
                Waypoints.Add(wp);
                index++;
            }

            FillWaypointList(index);

	        return true;
        }


        public bool DeleteWaypoint()
        {
            if (formMain.listViewWaypoints.SelectedIndices.Count == 0) return false;
            int index = formMain.listViewWaypoints.SelectedIndices[0];

            if (index < Waypoints.Count)
            {             
                Waypoints.RemoveAt(index);
            }

            FillWaypointList(index);

	        return true;
        }

        private void FillWaypointList(int selectIndex)
        {
            formMain.listViewWaypoints.Items.Clear();
            for(int i=0; i!=Waypoints.Count; i++)
            {
                System.Windows.Forms.ListViewItem lvi = new System.Windows.Forms.ListViewItem((i+1).ToString());
                float N, E;
                llConv.ConvertLLToM(Waypoints[i].Latitude, Waypoints[i].Longitude, out N, out E);
                lvi.SubItems.Add(N.ToString("0.0"));
                lvi.SubItems.Add(E.ToString("0.0"));
                lvi.SubItems.Add(Waypoints[i].Altitude.ToString("0.0"));
                formMain.listViewWaypoints.Items.Add(lvi);     
            }
            formMain.listViewWaypoints.Items.Add(new System.Windows.Forms.ListViewItem(new string[] { "New", "", "", "" }));
            formMain.listViewWaypoints.Items[selectIndex].Selected = true;
        }

        public void DrawMap()
        {
            // clear backbuffer
            DBmyBuffer.Graphics.Clear(Color.Black);

            if (mapBitmap == null) return;

            // draw map
            DBmyBuffer.Graphics.DrawImageUnscaled(mapBitmap, formMain.pictureBoxMap.DisplayRectangle);

	        // draw location
	        float mapX, mapY;
	        ConvertLocationLL2Pix(CurrentPosition.Longitude, CurrentPosition.Latitude, out mapX, out mapY);
	        // clip to map!
	        if (mapX < 0) mapX = 0;
	        if (mapY < 0) mapY = 0;
            if (mapX > MapImageSizePix.Width) mapX = (float)MapImageSizePix.Width;
            if (mapY > MapImageSizePix.Height) mapY = (float)MapImageSizePix.Height;
            DBmyBuffer.Graphics.FillEllipse(new SolidBrush(Color.White), mapX-5, mapY-5, 10, 10);
            DBmyBuffer.Graphics.DrawEllipse(new Pen(Color.Red, 3), mapX-5-CircleSize/2, mapY-5 - CircleSize/2, 10 + CircleSize, 10 + CircleSize );
	        if (++CircleSize > 30) CircleSize = 0;
            StringFormat strF = new StringFormat();
            strF.Alignment = StringAlignment.Center;
            DBmyBuffer.Graphics.DrawString(string.Format("{0:0.0}", CurrentPosition.Altitude), fontWaypoints, Brushes.BlanchedAlmond, mapX - 1, mapY + 20, strF);

	        // draw home
	        if (HomeSet)
	        {
		        ConvertLocationLL2Pix(HomeLongitude, HomeLatitude, out mapX, out mapY);
                DBmyBuffer.Graphics.DrawEllipse(new Pen(Color.White, 3), mapX - 16, mapY - 16, 32, 32);
                DBmyBuffer.Graphics.DrawString("H", fontHome, Brushes.White, mapX-16, mapY - 17 );
		        // Draw Target Circles
		        float DistanceM = 250;
                float areaSize = DistanceM / (float)MetersPerPix;
                DBmyBuffer.Graphics.DrawEllipse(new Pen(Color.Yellow, 2), mapX - areaSize, mapY - areaSize, areaSize*2, areaSize*2);
	        }
            
	       
		    // immediate mode
		    if (TargetWaypoint.Latitude != 0)
		    {
			    ConvertLocationLL2Pix(TargetWaypoint.Longitude, TargetWaypoint.Latitude, out mapX, out mapY);
                DBmyBuffer.Graphics.DrawEllipse(new Pen(Color.White, 3), mapX-16, mapY-16, 32, 32 );
                DBmyBuffer.Graphics.DrawLine(new Pen(Color.White, 1), mapX - 20, mapY - 20, mapX + 20, mapY + 20);
                DBmyBuffer.Graphics.DrawLine(new Pen(Color.White, 1), mapX + 20, mapY - 20, mapX - 20, mapY + 20);

                StringFormat stringFormat = new StringFormat();
                stringFormat.Alignment = StringAlignment.Center;
                DBmyBuffer.Graphics.DrawString(string.Format("{0}", TargetWaypoint.Altitude), fontWaypoints, Brushes.White, mapX - 1, mapY + 20, stringFormat);
            }
	    
		    // draw trajectory
    	    float lastWpX = 0, lastWpY = 0;
            for (int i = 0; i != Waypoints.Count; i++)
		    {
			    ConvertLocationLL2Pix(Waypoints[i].Longitude, Waypoints[i].Latitude, out mapX, out mapY);
                DBmyBuffer.Graphics.DrawEllipse(new Pen(Color.GreenYellow, 3), mapX-16, mapY-16, 32, 32 );
                DBmyBuffer.Graphics.DrawString(string.Format("{0}", i + 1), fontWaypoints, Brushes.Yellow, mapX - 8, mapY - 10);
                StringFormat stringFormat = new StringFormat();
                stringFormat.Alignment = StringAlignment.Center;
                DBmyBuffer.Graphics.DrawString(string.Format("{0}", Waypoints[i].Altitude), fontWaypoints, Brushes.GreenYellow, mapX - 1, mapY + 20, stringFormat);
			  
			    // draw selected waypoint
                if (i == ActiveWaypoint)
			    {
                    DBmyBuffer.Graphics.DrawEllipse(new Pen(Color.White, 3), mapX-20, mapY-20, 40, 40 );	
			    }

			    // draw line
                if (i > 0)
                {
                    DBmyBuffer.Graphics.DrawLine(new Pen(Color.Blue, 1), lastWpX, lastWpY, mapX, mapY);
                }
			    lastWpX = mapX;
			    lastWpY = mapY;
		    }
            // render to screen            
            DBmyBuffer.Render();
        }

        public void OnMouseClick(Point point)
        {
            float DPI = 1.0F; // TODO!!!
            float mapX = point.X * DPI; 
            float mapY = point.Y * DPI;

            double lon, lat;
            ConvertLocationPix2LL(out lon, out lat, mapX, mapY);

            float Altitude;
            if (float.TryParse(formMain.comboBoxAltitude.Text, out Altitude) == false)
            {
                Altitude = 10; //default
            }

            if (formMain.radioButtonWaypoints.Checked )
            {
                AddWaypoint(Altitude, lat, lon);
            }
            else if (formMain.radioButtonGoto.Checked)
            {
                TargetWaypoint.Altitude = Altitude;
                TargetWaypoint.Latitude = lat;
                TargetWaypoint.Longitude = lon;
                // send target
                Goto(TargetWaypoint);
            }
        }

        public void DownloadWaypoints()
        {
            SCommWaypoints wayPoints = new SCommWaypoints();
            wayPoints.waypoints = new SWpt[8];

            wayPoints.WaypointCnt = (uint)Waypoints.Count;
            for (int i = 0; i != Waypoints.Count; i++)
            {
                wayPoints.waypoints[i].Altitude = Waypoints[i].Altitude;
                wayPoints.waypoints[i].Longitude = (int)(Waypoints[i].Longitude * 1e7);
                wayPoints.waypoints[i].Latitude = (int)(Waypoints[i].Latitude * 1e7);
            }

            // Send
            byte[] toSend = Comm.GetBytes(wayPoints);
            formMain.SendData(0x81, toSend);
        }
        
        public void ExecuteWaypoints(float velocity)
        {
            SCommGotoExecute executeWaypointsCmd = new SCommGotoExecute();
            executeWaypointsCmd.Command = 0x02; // execute trajectory
            executeWaypointsCmd.Velocity = velocity;

            // Send
            byte[] toSend = Comm.GetBytes(executeWaypointsCmd);
            formMain.SendData(0x80, toSend);
        }

        public void AbortWaypoints()
        {
            SCommGotoExecute executeWaypointsCmd = new SCommGotoExecute();
            executeWaypointsCmd.Command = 10; // abort trajectory

            // Send
            byte[] toSend = Comm.GetBytes(executeWaypointsCmd);
            formMain.SendData(0x80, toSend);
        }

        public void GoHome()
        {
            float Altitude;
            if (float.TryParse(formMain.comboBoxAltitude.Text, out Altitude) == false)
            {
                Altitude = 10; //default
            }

            TargetWaypoint.Altitude = Altitude;
            TargetWaypoint.Latitude = HomeLatitude;
            TargetWaypoint.Longitude = HomeLongitude;
            // send target
            Goto(TargetWaypoint);        
        }

        private void Goto(SWaypoint targetWaypoint)
        {
            SCommGotoExecute gotoExecuteCmd = new SCommGotoExecute();
            gotoExecuteCmd.Command = 0x01; // execute GOTO
            gotoExecuteCmd.TargetWaypoint.Altitude = targetWaypoint.Altitude;
            gotoExecuteCmd.TargetWaypoint.Longitude = (int)(targetWaypoint.Longitude * 1e7);
            gotoExecuteCmd.TargetWaypoint.Latitude = (int)(targetWaypoint.Latitude * 1e7);

            // Send
            byte[] toSend = Comm.GetBytes(gotoExecuteCmd);
            formMain.SendData(0x80, toSend);
        }

        public void ExecuteOrbit(SWaypoint targetCenter, float velocity)
        {
            SCommGotoExecute executeOrbitCmd = new SCommGotoExecute();
            executeOrbitCmd.Command = 0x03; // execute orbit
            executeOrbitCmd.TargetWaypoint.Altitude = targetCenter.Altitude;
            executeOrbitCmd.TargetWaypoint.Longitude = (int)(targetCenter.Longitude * 1e7);
            executeOrbitCmd.TargetWaypoint.Latitude = (int)(targetCenter.Latitude * 1e7);
            executeOrbitCmd.Velocity = velocity;

            // Send
            byte[] toSend = Comm.GetBytes(executeOrbitCmd);
            formMain.SendData(0x80, toSend);
        }

        public void UpdateCurrentPosition(float altitude, double longitude, double latitude, int actualMode)
        {
            CurrentPosition.Altitude = altitude;
            CurrentPosition.Longitude = longitude;
            CurrentPosition.Latitude = latitude;

            if (actualMode >= 0 && actualMode <= 5) ActiveWaypoint = -1;
            else ActiveWaypoint = actualMode - 11;
        }

        // Helpers
        public void ConvertLocationLL2Pix(double longitude, double latitude, out float mapX, out float mapY)
        {
	        double deltaX = longitude - CenterLongitude;
	        double pixelX = (deltaX / 360) * 256 * Math.Pow(2, ZoomLevel);
	        mapX = (float)(pixelX + MapImageSizePix.Width/2);

	        double sinLatitudeCenter = Math.Sin(CenterLatitude * Math.PI / 180);
	        double pixelYCenter = (0.5 - Math.Log((1 + sinLatitudeCenter) / (1 - sinLatitudeCenter)) / (4 * Math.PI)) * 256 * Math.Pow(2, ZoomLevel); // center pix
	        double sinLatitude = Math.Sin(latitude * Math.PI / 180);
	        double pixelY = (0.5 - Math.Log((1 + sinLatitude) / (1 - sinLatitude)) / (4 * Math.PI)) * 256 * Math.Pow(2, ZoomLevel);
	        mapY = (float)(pixelY - pixelYCenter + MapImageSizePix.Height/2);
        }

        public void ConvertLocationPix2LL(out double longitude, out double latitude, float mapX, float mapY)
        {
	        double pixelX = mapX - MapImageSizePix.Width / 2;
	        double deltaX = pixelX * 360 / (256 * Math.Pow(2, ZoomLevel));
	        longitude = deltaX + CenterLongitude;

	        double sinLatitudeCenter = Math.Sin(CenterLatitude * Math.PI / 180);
	        double pixelYCenter = (0.5 - Math.Log((1 + sinLatitudeCenter) / (1 - sinLatitudeCenter)) / (4 * Math.PI)) * 256 * Math.Pow(2, ZoomLevel); // center pix
	        double pixelY = mapY + pixelYCenter - MapImageSizePix.Height / 2;
	        double deltaY = 0.5 - pixelY / (256 * Math.Pow(2, ZoomLevel));
	        latitude = 90 - 360 * Math.Atan(Math.Exp(-deltaY * 2 * Math.PI)) / Math.PI;
        }

        public void SetHome(double longitude, double latitude)
        {
	        HomeLongitude = longitude;
	        HomeLatitude = latitude;
	        HomeSet = true;

            llConv.SetHome(HomeLatitude, HomeLongitude);
        }

        public bool HaveHome()
        {
	        return HomeSet;
        }

        private double DistanceFromHomeMeters(double longitude, double latitude)
        {
	        double distance = 0; // no home set
	        if (HomeSet)
	        {
		        distance = LLDistanceM(longitude, latitude, HomeLongitude, HomeLatitude);
	        }
	
	        return distance;
        }

        private double LLDistanceM(double longitude1, double latitude1, double longitude2, double latitude2)
        {
	        double R = 6371000; // [m]
            double dLat = (latitude2 - latitude1) * Math.PI / 180;
            double dLon = (longitude2 - longitude1) * Math.PI / 180;
            double lat1 = latitude1 * Math.PI / 180;
            double lat2 = latitude2 * Math.PI / 180;

            double a = Math.Sin(dLat / 2) * Math.Sin(dLat / 2) + Math.Sin(dLon / 2) * Math.Sin(dLon / 2) * Math.Cos(lat1) * Math.Cos(lat2);
            double c = 2 * Math.Atan2(Math.Sqrt(a), Math.Sqrt(1 - a));
	        double distance = R * c;

	        return distance;
        }        
    }
}
