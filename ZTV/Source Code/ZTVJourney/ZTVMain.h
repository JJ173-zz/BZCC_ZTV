#ifndef ZTVMAIN_H__
#define ZTVMAIN_H__

#include "..\shared\SubMission.h"
#include "ZTVMission.h"

enum AIPType
{
	AIPType0 = 0,
	AIPType1,
	AIPType2,
	AIPType3,
	AIPTypeA,
	AIPTypeL,
	AIPTypeS,
	MAX_AIP_TYPE,
};

class ZTVMain : public SubMission
{
	enum
	{
		MAX_CPU_SCAVS = 15 // Number of CPU Scavs that'll trigger a cleanup
	};

public:
	// Init methods...
	ZTVMain(void);

	// Based on player races and type, set AIPlan
	void SetCPUAIPlan(AIPType Type);
	// Handle Adding a CPU Scav to the game.
	void AddCPUScav(Handle scavHandle);

	// And set up extra race-specific vehicles at pathpoints in the map.
	void SetupExtraVehicles(void);

	// Builds a starting vehicle on the specified team with either ODF1
	// (preferred, if found) or ODF2 as its name, modified by the
	// race. Sets group, if specified. Returns the handle
	Handle BuildStartingVehicle(int aTeam, int aRace, const char* ODF1, const char* ODF2, const char* Where, int aGrp);

	// PlayerEjected event...
	virtual EjectKillRetCodes PlayerEjected(void);
	// PlayerKilled event...
	virtual EjectKillRetCodes PlayerKilled(int KillersHandle);
	// Setup Method..
	virtual void Setup(void);
	// Runs every turn...
	virtual void Execute(bool* pTeamIsSetUp, Handle* pRecyclerHandles);
	// Fires when an object is added to the world...
	virtual void AddObject(Handle h, bool IsStartingVehicles, Handle* m_RecyclerHandles);
	// Fires when an object is removed from the world...
	virtual void DeleteObject(Handle h);
	// Called when setting up the Strategy game
	virtual void DoGenericStrategy(bool* pTeamIsSetUp, Handle* m_RecyclerHandles);

private:
	int m_GameTPS;

	// Store our class for the ZTVMission stuff here.
	ZTVMission ZMission;

	bool
		b_first,
		m_StartDone = false,
		m_SiegeOn = false,
		m_AntiAssault = false,
		m_LateGame = false,
		m_HaveArmory = false,
		m_SetFirstAIP = false,
		m_PastAIP0 = false,
		b_last;

	float
		f_first,
		f = 0.0f,
		f_last;

	Handle
		h_first,
		m_HumanRecycler = 0,
		m_CPURecycler = 0,
		m_CPUScavList[MAX_CPU_SCAVS],
		h_last;

	int
		i_first,
		m_StratTeam = 1,
		m_SiegeCounter = 0,
		m_AssaultCounter = 0,
		m_TimeCount = 0,
		m_CPUTeamNumber = 6,
		m_TurnCounter = 0,
		m_NumHumans = 0,
		m_NumCPUScavs = 0,
		m_TurretAISkill = 0, // ivar17
		m_NonTurretAISkill = 0, // ivar18
		m_CPUTurretAISkill = 0, // ivar21
		m_CPUNonTurretAISkill = 0, // ivar22
		m_HumanForce = 0,
		m_CPUForce = 0,
		m_CPUTeamRace = 0,
		m_HumanTeamRace = 0,
		m_FirstAIPSwitchTime = 0, // ivar26
		m_CPUCommBunkerCount = 0,
		m_LastCPUPlan = 0,
		i_last;
};

#endif