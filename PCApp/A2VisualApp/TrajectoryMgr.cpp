#include "stdafx.h"
#include "TrajectoryMgr.h"
#include <fstream>

CTrajectoryMgr::CTrajectoryMgr()
{
	m_LastWaypointIndex = -1; // no waypoints
}


bool  CTrajectoryMgr::AddWaypoint(int index, float altitude, double latitude, double longitude)
{
	if (index < 0 || index >(m_LastWaypointIndex + 1) || index >= MAXWAYPOINTS) return false; // invalid index
	SWaypoint wp;
	wp.Altitude = altitude;
	wp.Latitude = latitude;
	wp.Longitude = longitude;
	waypoints[index] = wp;

	if (index > m_LastWaypointIndex) m_LastWaypointIndex = index;

	return true;
}


bool CTrajectoryMgr::DeleteWaypoint()
{
	if (m_LastWaypointIndex < 0) return false;
	m_LastWaypointIndex--;
	
	return true;
}

int CTrajectoryMgr::GetWaypoints(SWaypoint* wps)
{
	memcpy(wps, waypoints, sizeof(waypoints));

	return m_LastWaypointIndex+1;
}

void CTrajectoryMgr::SaveWaypoints()
{
	std::wstring filename = L"trajectory.txt";
	std::ofstream trajFile(filename, std::ios::trunc);

	for (int i = 0; i != m_LastWaypointIndex+1; i++)
	{		
		CHAR buf[500];
		sprintf_s(buf, 500, "%0.2f %0.8f %0.8f", waypoints[i].Altitude, waypoints[i].Latitude, waypoints[i].Longitude);
		trajFile << buf << std::endl;
	}

	trajFile.close();
}

void CTrajectoryMgr::LoadWaypoints()
{
	std::wstring filename = L"trajectory.txt";
	std::ifstream logFile(filename, std::ios::binary);

	int i = 0;
	do
	{
		logFile >> waypoints[i].Altitude >> waypoints[i].Latitude >> waypoints[i].Longitude;
		i++;
	} while ((!logFile.eof()));
	m_LastWaypointIndex = i-2;
}
