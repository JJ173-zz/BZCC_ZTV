#include "ZTVMain.h"
#include "ZTVMission.h"

#include <string.h>
#include "..\Shared\Taunts.h"
#include "..\Shared\TRNAllies.h"
#include "..\Shared\DLLUtils.h"

/* Variables */

// Number of scavs (vehicles) that'll be left around after a cleanup.
const int MIN_CPU_SCAVS_AFTER_CLEANUP = 5;

// AIPType above is offset in this string table
const char* AIPTypeExtensions = "0123als";

// Siege Distance
const float kfSIEGE_DISTANCE = 250.0f;

// Store the CustomAIPNameBase here from pContents
char CustomAIPNameBase[256];

// Make sure we mark whether we have checked SVar3 or not.
bool CheckedSVar3 = false;

// Check if we're in debug state
bool DEBUG = true;

/* Checkers */
static int IsRecyclerODF(Handle h)
{
	if (h == 0)
		return 0;

	char ODFName[64];
	GetObjInfo(h, Get_CFG, ODFName);

	char ObjClass[64];
	GetObjInfo(h, Get_GOClass, ObjClass);

	if (_stricmp(ObjClass, "CLASS_RECYCLERVEHICLE") == 0 || _stricmp(ObjClass, "CLASS_RECYCLER") == 0)
		return ODFName[0];

	return 0;
}

/* Class Init */
ZTVMain::ZTVMain()
{
	// Enable high TPS.
	EnableHighTPS(m_GameTPS);

	// If the user wants random music, we're fine with that.
	AllowRandomTracks(true);

	// Count our number of bools
	b_count = &b_last - &b_first - 1;
	b_array = &b_first + 1;

	if (b_array)
		memset(b_array, 0, b_count * sizeof(bool));

	// Count our number of floats
	f_count = &f_last - &f_first - 1;
	f_array = &f_first + 1;

	if (f_array)
		memset(f_array, 0, f_count * sizeof(float));

	// Count our number of handles
	h_count = &h_last - &h_first - 1;
	h_array = &h_first + 1;

	if (h_array)
		memset(h_array, 0, h_count * sizeof(Handle));

	// Count our number of ints
	i_count = &i_last - &i_first - 1;
	i_array = &i_first + 1;

	if (i_array)
		memset(i_array, 0, i_count * sizeof(int));

	// Set up TRN stuff.
	TRNAllies::SetupTRNAllies(GetMapTRNFilename());
}

/* Events */
void ZTVMain::AddObject(Handle h, bool IsStartingVehicles, Handle* pRecyclerHandles)
{
	// Always grab the ODF name out, if possible. Makes life much easier in here.
	char ODFName[64];
	GetObjInfo(h, Get_CFG, ODFName);

	// Also grab the class name out.
	char ObjClass[64];
	GetObjInfo(h, Get_GOClass, ObjClass);

	// Always get one random # in 0..1, for scion shield randomization
	float fRandomNum = GetRandomFloat(1.0f);

	// Check for a turret
	bool IsTurret = (_stricmp(ObjClass, "CLASS_TURRETTANK") == 0);

	// Start setting the team races and Recycler handles here
	if (m_TurnCounter < 2)
	{
		// monitor what objects get built by the mpstrat dll
		if (GetTeamNum(h) == m_StratTeam)
		{
			int HumanRecyRace = IsRecyclerODF(h);
			int ShellRace = GetVarItemInt("network.session.ivar13");

			if (HumanRecyRace)
			{
				m_HumanTeamRace = HumanRecyRace;
				m_HumanRecycler = h;
			}

			if (ShellRace)
			{
				m_CPUTeamRace = ShellRace;
			}
			else
			{
				Handle h = GetObjectByTeamSlot(m_CPUTeamNumber, DLL_TEAM_SLOT_RECYCLER);
				if (h != 0)
				{
					int cpuRace = IsRecyclerODF(h);
					if (cpuRace != 0)
					{
						m_CPUTeamRace = cpuRace;
					}
				}
				else
				{
					m_CPUTeamRace = 'i';
				}
			}
		}
	}

	if (GetTeamNum(h) != m_CPUTeamNumber)
	{
		// Set AI Unit skills...
		if (IsTurret)
			SetSkill(h, m_TurretAISkill);
		else
			SetSkill(h, m_NonTurretAISkill);

		// Adjust counter if an Assault Unit is added
		if (_stricmp(ObjClass, "CLASS_ASSAULTTANK") == 0 || _stricmp(ObjClass, "CLASS_WALKER") == 0)
		{
			++m_AssaultCounter;
		}
	}
	else if (GetTeamNum(h) == m_CPUTeamNumber)
	{
		int UseTurretSkill = m_CPUTurretAISkill;
		int UseNonTurretSkill = m_CPUNonTurretAISkill;

		if (IsStartingVehicles)
		{
			if (UseTurretSkill > 0)
				UseTurretSkill--;
			if (UseNonTurretSkill > 0)
				UseNonTurretSkill--;
		}

		// Set AI Unit skills...
		if (IsTurret)
			SetSkill(h, UseTurretSkill);
		else
			SetSkill(h, UseNonTurretSkill);

		if (_stricmp(ObjClass, "CLASS_SCAVENGER") == 0 || _stricmp(ObjClass, "CLASS_SCAVENGERH") == 0)
		{
			AddCPUScav(h);
		}
		else if (_stricmp(ObjClass, "CLASS_ARMORY") == 0)
		{
			m_HaveArmory = true;
		}

		if (m_HaveArmory)
		{
			// Handle tank faction weapon upgrades.
			if (_strnicmp(ODFName, "ivtank", 6) == 0)
			{
				GiveWeapon(h, "gspstab_c");
			}
			else if (_strnicmp(ODFName, "fvtank", 6) == 0)
			{
				GiveWeapon(h, "garc_c");
			}

			// Select our shields based on a random number.
			if (_strnicmp(ODFName, "fv", 2) == 0)
			{
				if (fRandomNum < 0.3f)
					GiveWeapon(h, "gshield");
				else if (fRandomNum < 0.6f)
					GiveWeapon(h, "gabsorb");
				else if (fRandomNum < 0.9f)
					GiveWeapon(h, "gdeflect");
			}
		}

		if (_stricmp(ObjClass, "CLASS_COMMBUNKER") == 0 || _stricmp(ObjClass, "CLASS_COMMTOWER") == 0)
			++m_CPUCommBunkerCount;

		// ZTV Custom Assets
		if (_strnicmp(ODFName, "fvtv", 6) == 0)
			ZMission.m_TimeVirus = h;
		else if (_strnicmp(ODFName, "fvzeeder", 6) == 0)
			ZMission.m_Zeeder = h;
	}

	// Handle setting our AIP.
	if (!m_PastAIP0 && m_FirstAIPSwitchTime > 0 && m_TurnCounter > m_FirstAIPSwitchTime)
	{
		m_PastAIP0 = true;

		switch ((m_TurnCounter % 2) % 2)
		{
			default:
			case 0:
				SetCPUAIPlan(AIPType1);
				break;
			case 1:
				SetCPUAIPlan(AIPType3);
				break;
		}
	}
}

void ZTVMain::DeleteObject(Handle h)
{
	char ObjClass[64];
	GetObjInfo(h, Get_GOClass, ObjClass);

	if (GetTeamNum(h) == m_StratTeam)
	{
		if ((_stricmp(ObjClass, "CLASS_ASSAULTTANK") == 0 || _stricmp(ObjClass, "CLASS_WALKER") == 0) && m_AssaultCounter > 0)
		{
			m_AssaultCounter = 0;
		}
	}
	else
	{
		if (_stricmp(ObjClass, "CLASS_COMMBUNKER") == 0 || _stricmp(ObjClass, "CLASS_COMMTOWER") == 0)
			--m_CPUCommBunkerCount;
		else if (_stricmp(ObjClass, "CLASS_ARMORY") == 0)
			m_HaveArmory = false;
	}
}

/* Main Functions */
void ZTVMain::Setup()
{
	// Don't autogroup units
	SetAutoGroupUnits(false);

	// Get Turret AI Skill
	m_TurretAISkill = GetVarItemInt("network.session.ivar17");
	m_TurretAISkill = clamp(m_TurretAISkill, 0, 3);

	// Get Non Turret AI Skill
	m_NonTurretAISkill = GetVarItemInt("network.session.ivar18");
	m_NonTurretAISkill = clamp(m_NonTurretAISkill, 0, 3);

	// Get CPU Turret AI Skill
	m_CPUTurretAISkill = GetVarItemInt("network.session.ivar21");
	m_CPUTurretAISkill = clamp(m_CPUTurretAISkill, 0, 3);

	// Get CPU Non Turret AI Skill
	m_CPUNonTurretAISkill = GetVarItemInt("network.session.ivar22");
	m_CPUNonTurretAISkill = clamp(m_CPUNonTurretAISkill, 0, 3);

	// Get Human Force
	m_HumanForce = GetVarItemInt("network.session.ivar23");
	m_HumanForce = clamp(m_HumanForce, 0, 3);

	// Get CPU Force
	m_CPUForce = GetVarItemInt("network.session.ivar24");
	m_CPUForce = clamp(m_CPUForce, 0, 3);

	// Get AIP Switch time
	m_FirstAIPSwitchTime = GetVarItemInt("network.session.ivar26");

	// If there's no set time, default to 3 mins.
	if (m_FirstAIPSwitchTime == 0)
		m_FirstAIPSwitchTime = 180;

	// Make sure we convert to sim ticks for accurate times.
	if (m_FirstAIPSwitchTime > 0)
		m_FirstAIPSwitchTime *= m_GameTPS;

	// How many players we got? 
	m_NumHumans = CountPlayers();

	// Debug..
	if (DEBUG)
	{
		Ally(m_StratTeam, m_CPUTeamNumber);
		Ally(m_CPUTeamNumber, m_StratTeam);
	}

	// Run ZTV Setup...
	ZMission.Setup();
}

void ZTVMain::Execute(bool* pTeamIsSetUp, Handle* pRecyclerHandles)
{
	// Increase the turn counter every....well turn.
	m_TurnCounter++;

	// Call the generic strategy method to set up the mission.
	DoGenericStrategy(pTeamIsSetUp, pRecyclerHandles);

	// Call on our mission logic here
	ZMission.Execute(pTeamIsSetUp, pRecyclerHandles);

	// Any humans not on the right team? Make life hell for them.
	if (m_TurnCounter % (10 * m_GameTPS) == 0)
	{
		for (int i = 0; i < MAX_TEAMS; i++)
		{
			Handle h = GetPlayerHandle(i);

			if (h && WhichTeamGroup(i) != 0)
			{
				Damage(h, (1 << 28) + 1);
				AddToMessagesBox("MPI is limited to 5 humans! No joining the CPU team!");
			}
		}
	}
}

void ZTVMain::DoGenericStrategy(bool* pTeamIsSetUp, Handle* pRecyclerHandles)
{
	// Increase the TimeCount...
	m_TimeCount++;

	// Handle the starting methods...
	if (!m_StartDone && m_HumanTeamRace)
	{
		SetupExtraVehicles();

		m_CPURecycler = GetObjectByTeamSlot(m_CPUTeamNumber, DLL_TEAM_SLOT_RECYCLER);

		if (m_CPURecycler == 0)
		{
			char startRecyODF[64];
			const char* pContents = DLLUtils::GetCheckedNetworkSvar(12, NETLIST_Recyclers);

			if (pContents != NULL && pContents[0] != '\0')
				strncpy_s(startRecyODF, pContents, sizeof(startRecyODF) - 1);
			else
				strcpy_s(startRecyODF, "*vrecycpu");

			startRecyODF[0] = (char)m_CPUTeamRace;

			if (!DoesODFExist(startRecyODF))
			{
				strcpy_s(startRecyODF, "*vrecycpu");
			}

			m_CPURecycler = BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, startRecyODF, "*vrecy", "RecyclerEnemy", 1);
		}

		pRecyclerHandles[m_CPUTeamNumber] = m_CPURecycler;
		pTeamIsSetUp[m_CPUTeamNumber] = true;

		BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*vturr", "*vturr", "turretEnemy1", 0);
		BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*vturr", "*vturr", "turretEnemy2", 0);

		if (m_CPUForce > 0)
		{
			BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*bspir", "*vturr", "gtow2", 0);
			BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*bspir", "*vturr", "gtow3", 0);
			BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*vsent", "*vscout", "SentryEnemy1", 0);
			BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*vsent", "*vscout", "SentryEnemy2", 0);
			BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*vtank", "*vtank", "TankEnemy1", 0);
			BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*vtank", "*vtank", "TankEnemy2", 0);

			if (m_CPUForce > 1)
			{
				BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*bspir", "*vturr", "gtow4", 0);
				BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*bspir", "*vturr", "gtow5", 0);
				BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*vtank", "*vtank", "TankEnemy3", 0);
				BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*vsent", "*vscout", "SentryEnemy3", 0);
			}
		}

		BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*vscav", "*vscav", "ScavengerEnemy", 0);

		if (!m_PastAIP0)
		{
			SetCPUAIPlan(AIPType0);
		}

		SetScrap(m_CPUTeamNumber, 60);
		SetScrap(1, 40);

		m_StartDone = true;
	}

	// Evaluate this every 10 seconds
	if (m_TimeCount % (10 * m_GameTPS) == 0)
	{
		m_NumHumans = CountPlayers();
	}

	// Avoid divide by zero if we set compforce to 3.
	int CompForceSkew = m_CPUForce * m_GameTPS;

	if (CompForceSkew >= (3 * m_GameTPS))
		CompForceSkew = (3 * m_GameTPS) - 2;

	// Handle scrap cheats.
	if ((m_TimeCount % ((3 * m_GameTPS) - CompForceSkew)) == 0) 
		AddScrap(m_CPUTeamNumber, 1);
	if (m_TimeCount % (m_GameTPS - m_NumHumans + 1) == 0)
		AddScrap(m_CPUTeamNumber, 1);
	if ((m_HumanForce) && (m_TimeCount % ((10 * m_GameTPS) - (m_HumanForce * 20)) == 0))
		AddScrap(m_StratTeam, 1);

	// Handle Siege Logic
	Handle closestEnemy = 0;
	float closestEnemyDist = 1e30f; // big distance!

	// Find nearest enemy to m_HumanRecycler/factory (in that order)
	Handle RecyH = GetObjectByTeamSlot(m_CPUTeamNumber, DLL_TEAM_SLOT_RECYCLER);

	if (RecyH)
	{
		// Ignore scavs, pilots, and any craft > kfSIEGE_DISTANCE away.
		closestEnemy = GetNearestEnemy(RecyH, true, true, kfSIEGE_DISTANCE);
	}
	else
	{
		Handle FactoryH = GetObjectByTeamSlot(m_CPUTeamNumber, DLL_TEAM_SLOT_FACTORY);

		if (FactoryH)
		{
			// Ignore scavs, pilots, and any craft > kfSIEGE_DISTANCE away.
			closestEnemy = GetNearestEnemy(FactoryH, true, true, kfSIEGE_DISTANCE);
		}
	}

	if (closestEnemy != 0)
	{
		closestEnemyDist = GetDistance(closestEnemy, RecyH);
	}

	// Run some checks to make sure we are using siege correctly.
	if (!m_SiegeOn)
	{
		if (closestEnemy)
		{
			m_SiegeCounter++;
		}
		else
		{
			m_SiegeCounter = 0;
		}

		const int kiSIEGE_TIME = 45 * m_GameTPS; // 45 seconds

		if (m_SiegeCounter > kiSIEGE_TIME)
		{
			m_SiegeOn = true;
			SetCPUAIPlan(AIPTypeS);
		}
	}
	else if (closestEnemy == 0)
	{
		SetCPUAIPlan(AIPTypeL);

		if (m_LastCPUPlan == AIPTypeL)
		{
			m_SiegeOn = false;
			m_SiegeCounter = 0;
		}
	}

	// Handle a scenario when human players have too many assault units.
	if (!m_LateGame && !m_SiegeOn && !m_AntiAssault && m_AssaultCounter > 2)
	{
		m_AntiAssault = true;
		SetCPUAIPlan(AIPTypeA);
	}
	else if (m_AntiAssault && m_AssaultCounter < 3)
	{
		m_AntiAssault = false;
		SetCPUAIPlan(AIPTypeL);
	}
}

void ZTVMain::SetupExtraVehicles(void)
{
	// Grab all paths on the map that relate to extra vehicles/towers.
	int pathCount;
	char** pathNames;
	GetAiPaths(pathCount, pathNames);

	// Grab each race letter for both teams
	char cCPUTeamRace = (char)m_CPUTeamRace;
	char cHumanTeamRace = (char)m_HumanTeamRace;
	size_t len;

	// Nothing in this list now.
	m_NumCPUScavs = 0;

	for (int i = 0; i < pathCount; ++i)
	{
		char* Label = pathNames[i];

		if (strncmp(Label, "mpi", 3) == 0)
		{
			const int MaxODFLen = 64;
			char ODF1[MaxODFLen];
			char ODF2[MaxODFLen];

			memset(ODF1, 0, sizeof(ODF1));
			memset(ODF2, 0, sizeof(ODF2));

			char* pUnderscore = strchr(Label, '_');

			if (pUnderscore == NULL)
				continue;

			char* pUnderscore2 = strchr(pUnderscore + 1, '_');

			if (pUnderscore2 == NULL)
			{
				strcpy_s(ODF1, pUnderscore + 1);
			}
			else
			{
				size_t len = (pUnderscore2 - pUnderscore) - 1;

				if (len > (sizeof(ODF2) - 1))
					len = (sizeof(ODF2) - 1);

				strncpy_s(ODF1, pUnderscore + 1, len);
				strcpy_s(ODF2, pUnderscore2 + 1);
			}

			len = strlen(ODF1);
			if (len > 0 && ODF1[len - 1] == '_')
				ODF1[len - 1] = '\0';

			len = strlen(ODF2);
			if (len > 0 && ODF2[len - 1] == '_')
				ODF2[len - 1] = '\0';

			if (strncmp(Label, "mpic", 4) == 0)
			{
				if (ODF1[0] == cCPUTeamRace)
					BuildObject(ODF1, m_CPUTeamNumber, Label);
				else if (ODF2[0] == cCPUTeamRace)
					BuildObject(ODF2, m_CPUTeamNumber, Label);
			}
			else if (strncmp(Label, "mpiC", 4) == 0)
			{
				if (ODF1[0])
				{
					ODF1[0] = cCPUTeamRace;
					BuildObject(ODF1, m_CPUTeamNumber, Label);
				}
				else if (ODF2[0])
				{
					ODF2[0] = cCPUTeamRace;
					BuildObject(ODF2, m_CPUTeamNumber, Label);
				}
			}
			else if (strncmp(Label, "mpih", 4) == 0)
			{
				Handle h = 0;
				if (ODF1[0] == cHumanTeamRace)
				{
					h = BuildObject(ODF1, m_StratTeam, Label);
					SetBestGroup(h);
				}
				else if (ODF2[0] == cHumanTeamRace)
				{
					h = BuildObject(ODF2, m_StratTeam, Label);
					SetBestGroup(h);
				}
			}
			else if (strncmp(Label, "mpiH", 4) == 0)
			{
				if (ODF1[0])
				{
					ODF1[0] = cHumanTeamRace;
					SetBestGroup(BuildObject(ODF1, m_StratTeam, Label));
				}
				else if (ODF2[0])
				{
					ODF2[0] = cHumanTeamRace;
					SetBestGroup(BuildObject(ODF2, m_StratTeam, Label));
				}
			}
		}
	}
}

Handle ZTVMain::BuildStartingVehicle(int aTeam, int aRace, const char* ODF1, const char* ODF2, const char* Where, int aGrp)
{
	char TempODF[64];

	strcpy_s(TempODF, ODF1);

	TempODF[0] = (char)aRace;

	if (!DoesODFExist(TempODF))
	{
		strcpy_s(TempODF, ODF2);
		TempODF[0] = (char)aRace;
	}

	Handle h = BuildObject(TempODF, aTeam, Where);

	if (h != 0 && aGrp >= 0)
		SetGroup(h, aGrp);

	return h;
}

/* CPU Functions */
void ZTVMain::AddCPUScav(Handle scavHandle)
{
	if (m_NumCPUScavs < MAX_CPU_SCAVS)
		m_CPUScavList[m_NumCPUScavs++] = scavHandle;

	// If our lists aren't full, then we're done from this function.
	if (m_NumCPUScavs < MAX_CPU_SCAVS)
		return;

	// Spice things up.
	DoTaunt(TAUNTS_Random);

	// If we get here, it's cleanup time. First pass - cull CPU scav
	// list of any dead ones, ones that changed teams, or ones that
	// aren't scavs anymore.
	int newScavListCount = 0;
	int i = 0;

	Handle newScavList[MAX_CPU_SCAVS];
	memset(newScavList, 0, sizeof(newScavList));

	for (i = 0; i < m_NumCPUScavs; i++)
	{
		// Choose whether we keep the scav or not
		bool keepIt = false;

		Handle checkScav = m_CPUScavList[i];

		if (IsAround(checkScav) && (GetTeamNum(checkScav) == m_CPUTeamNumber))
		{
			char ObjClass[64];
			GetObjInfo(checkScav, Get_GOClass, ObjClass);
			keepIt = (_stricmp(ObjClass, "CLASS_SCAVENGER") == 0) || (_stricmp(ObjClass, "CLASS_SCAVENGERH") == 0);
		}

		// Always keep the scavHandle passed in to this function.
		keepIt |= (checkScav == scavHandle);

		if (keepIt)
		{
			newScavList[newScavListCount++] = checkScav;
		}
	}

	// Copy culled list back into master list.
	m_NumCPUScavs = newScavListCount;
	memcpy(m_CPUScavList, newScavList, sizeof(m_CPUScavList));

	while (m_NumCPUScavs > MIN_CPU_SCAVS_AFTER_CLEANUP)
	{
		i = m_NumCPUScavs - 1;

		while ((i > 0) && (m_CPUScavList[i] == scavHandle))
			--i;

		if (m_CPUScavList[i] == scavHandle)
			break;

		SetNoScrapFlagByHandle(m_CPUScavList[i]);
		SelfDamage(m_CPUScavList[i], 1e28f);

		for (; i < m_NumCPUScavs; ++i)
			m_CPUScavList[i] = m_CPUScavList[i + 1];

		--m_NumCPUScavs;

		m_CPUScavList[m_NumCPUScavs] = 0;
	}
}

void ZTVMain::SetCPUAIPlan(AIPType Type)
{
	// Check for custom AIP Name...
	if (!CheckedSVar3)
	{
		const char* pContents = DLLUtils::GetCheckedNetworkSvar(3, NETLIST_AIPs);

		if (pContents == NULL || pContents[0] == '\0')
		{
			strcpy_s(CustomAIPNameBase, "stock13_");
		}
		else
		{
			strcpy_s(CustomAIPNameBase, pContents);
		}

		CheckedSVar3 = true;
	}

	// Set the default AIP if Type exceeds the limits.
	if (Type < 0 || Type >= MAX_AIP_TYPE)
		Type = AIPType3;

	// Prevent AIP stalling if rushed too early...
	if ((Type > AIPType3 && m_CPUCommBunkerCount == 0) && m_LastCPUPlan <= AIPType3)
		return;

	// Set the AIP file here...
	char AIPFile[256];
	sprintf_s(AIPFile, "%s%c%c%c.aip", CustomAIPNameBase, (char)m_CPUTeamRace, (char)m_HumanTeamRace, AIPTypeExtensions[Type]);
	SetPlan(AIPFile, m_CPUTeamNumber);

	// Store the last type.
	m_LastCPUPlan = Type;

	// Spicey...
	if (m_SetFirstAIP)
		DoTaunt(TAUNTS_Random);

	// Make sure this is true...
	m_SetFirstAIP = true;
}

/* SubMission */
EjectKillRetCodes ZTVMain::PlayerEjected(void)
{
	return DoEjectPilot;
}

EjectKillRetCodes ZTVMain::PlayerKilled(int)
{
	return DoEjectPilot;
}

SubMission* BuildSubMission(void)
{
	return new ZTVMain();
}