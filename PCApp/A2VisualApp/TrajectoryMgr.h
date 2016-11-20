#pragma once

#define MAXWAYPOINTS 8

struct SWaypoint
{
	float Altitude;
	double Latitude;
	double Longitude;	
};

class CTrajectoryMgr
{
public:
	CTrajectoryMgr();

	bool AddWaypoint(int index, float altitude, double latitude, double longitude);
	bool DeleteWaypoint();
	int GetWaypoints(SWaypoint* waypoints);
	void SaveWaypoints();
	void LoadWaypoints();

private:
	int m_LastWaypointIndex;
	SWaypoint waypoints[MAXWAYPOINTS];
};

