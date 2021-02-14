#ifndef ZTVMISSION_H__
#define ZTVMISSION_H__

#include "..\shared\SubMission.h"

class ZTVMission : SubMission
{
public:
	// Init methods...
	ZTVMission(void);

	// Setup Method..
	virtual void Setup(void);
	// Execution method that can be called upon from ZTVMain.
	virtual void Execute(bool* pTeamIsSetUp, Handle* pRecyclerHandles);
	// Based on player races and type, set AIPlan
	void ExecuteMissionCode(void);

	// Zeeder's handle that can be populated from ZTVMain.
	Handle m_Zeeder;
	// TV's handle that can be populated from ZTVMain.
	Handle m_TimeVirus;
private:
	int
		m_MissionState = 0;

	bool
		m_TVIdle,
		m_ZeederIdle,
		m_IsPartOne,
		m_IsPartTwo;

};

#endif