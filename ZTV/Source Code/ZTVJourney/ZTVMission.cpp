/* 
	ZTVMission Logic. 
	Original Code by Zeeder & TimeVirus
	Converted to BZCC by AI_Unit
*/

#include "ZTVMission.h";

ZTVMission::ZTVMission() { }

void ZTVMission::Setup() { }

void ZTVMission::Execute(bool* pTeamIsSetUp, Handle* pRecyclerHandles)
{
	// If TV is alive but he is idle, we should make sure we send him off to patrol.
	if (IsAlive(m_TimeVirus) && m_TVIdle)
	{
		// The part of the mission should declare where we send ZTV/Zeeder.
		if (m_IsPartOne)
			Patrol(m_TimeVirus, "patrol_1", 1);
		else
			Patrol(m_TimeVirus, "patrol_2", 1);

		// Make sure we don't loop
		m_TVIdle = false;
	}

	// If Zeeder is alive but he is idle, send him to follow TV.
	if (IsAlive(m_Zeeder) && m_ZeederIdle)
	{
		// I'm following!
		Follow(m_Zeeder, m_TimeVirus, 1);

		// Make sure we don't loop
		m_ZeederIdle = false;
	}
}