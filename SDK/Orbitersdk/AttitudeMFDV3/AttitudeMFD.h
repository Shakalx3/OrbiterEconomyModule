//////////////////////////////////////////////////////////////////////////////////////////////
//
//	AttitudeMFD.h - Header  file for the AttitudeMFD class
//
//	Copyright Chris Knestrick, 2003 
//	 
//	For a full description of the copyright, please see the file Copyright.txt
//
// Robert B. Denny <rdenny@dc3.com>
// Orbiter 2005 - Attitude MFD V3
//
// 15-Jun-05 rbd	For Visual C++ 7 (VS.NET 2005). Removed includes of iomanip.h and
//					fstream.h. Add MFD buttons.
///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef AUTOPILOT_MFD_H
#define AUTOPILOT_MFD_H
#define _CRT_SECURE_NO_DEPRECATE
#include "TargetObj.h"
#include "List.h"
#include "CDK.h"

typedef struct {
	VECTOR3 Pitch;
	VECTOR3 Yaw;
	VECTOR3 Roll;			
} RefPoints;

// The angular rotations from the a specific reference frame for a new reference frame
typedef VECTOR3 Attitude;

enum REF_MODE {USER_ATT = 1, VELOCITY, TARGET_RELATIVE,  EI };						
enum ATT_HOLD_MODE {DISENGAGED, ENGAGED, RPM, TOR}; 
enum COLOR_MODE {COLOR_OFF, COLOR_WHITE, COLOR_RED};

enum TRIM_STATUS {T_ENGAGED, T_DISENGAGED};
enum T_AXIS {TA_LAT, TA_VERT, TA_FA};


// Trim modes:
// T_VERT == Vertical (Numpad 1)
// T_LAT == Lateral (Numpad 2)
// T_FA == Fore/Aft (Numpad 3)
// T_ALL == All Axes (Numpad 5)
// T_VERT_LAT == Vertical AND Lateral (Numpad 7)
// T_LAT_FA == Lateral AND Fore/Aft (Numpad 8)
// T_VERT_FA == Vertical AND Fore/Aft (Numpad 9)
enum TRIM_MODE {T_VERT, T_LAT, T_FA, T_ALL, T_VERT_LAT, T_LAT_FA, T_VERT_FA};

//const int LINE = 15;					// Number of pixels to jump to write the next line
const int NUM_CONTROL_MODES = 3;
const int MAX_TARGET_NAME = 50;
const double UPDATE_INTERVAL = 0.25;	// Time between MFD State recalculation
const int HELP_OFF = 0;					// HelpPage = 0 means help off



// For ease of accessing the target list
#define CurrentTarget (TargetList.GetCurrent()).GetObj()

class AttitudeMFD:public MFD {
public:

	AttitudeMFD(DWORD w, DWORD h, VESSEL *vessel);
	~AttitudeMFD();

	void Update (HDC hdc);
	void UpdateState(double DT);
	
	static int MsgProc (UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam);
	bool ConsumeKeyBuffered(DWORD key);
	bool SelectTarget(char *name);
	bool ConsumeButton (int bt, int event);
	char *ButtonLabel (int bt);
	int ButtonMenu (const MFDBUTTONMENU **menu) const;
	bool SetRelAttitude(char *str, AXIS Axis);
	void ChangeRefMode(REF_MODE Mode);

private:
	
	// MFD Display-related variables
	HDC hDC;	
	DWORD Width, Height;
	int LINE;				// Number of pixels to jump to write the next line
	int CurrentLine;
	
	
	// Common variables
	REF_MODE RefMode; //MFD reference mode
	Attitude RefAttitude; //Reference attitude
	Attitude RelAttitude; //Desired attitude relative to the reference attitude
	VECTOR3 PitchYawRoll; //Current PYR angles relative to RelAttitude
	VECTOR3 DelPitchYawRoll; //Change in PYR between calculation updates
	VECTOR3 PitchYawRollRate; //Rate of change of PYR
	VECTOR3 RotLevel;
	ATT_HOLD_MODE AttHoldMode;
	bool VesselDocked; //Stores is something is docked to us
	bool VesselDocked_LE; //Used to detect the moment of docking

	
	// For TARGET_RELATIVE reference mode
	List<TargetObj> TargetList;	// List of targets, sorted by distance (nearest to farthest)
	OBJHANDLE TargetRef;
	char TargetName[MAX_TARGET_NAME];
	int Index;
	double RadialVel;
	VECTOR3 RelPos, RelVel;
	TRIM_STATUS TrimStatus;
	TRIM_MODE TrimMode;

	// For EI reference mode
	ENTRY_INTERFACE Interface;


	// Basic state data
	//VESSEL *Spacecraft;													// Global so opsFocusChanged can change
	VESSELSTATUS Status;
	VECTOR3 GSpacecraftPos, GSpacecraftVel, Airspeed;
	double AOA, SlipAngle, Mass;

	// Misc variables
	HBRUSH ThrusterBrush, ThrusterBrushNeg;
	COLOR_MODE ColorMode;
	int HelpPage;															// 0 = Not in Help mode
	double TimeElapsed, LastTimeElapsed;									// Time since last MFD state update


	// Functions
	VECTOR3 CalcPitchYawRollAngles();

	void CalcTargetRelative();
	void CalcVelocity();	
	void CalcAttitude();
	void CalcEI();
	void Control();

	void SetTrimLevelVert();
	void SetTrimLevelLat();
	void SetTrimLevelFA();
	void SetTrimThrustLevel(T_AXIS Axis);
	void Trim();

	void DisplayTargetRelative();
	void DisplayVelocity();	
	void DisplayAttitude();
	void DisplayEI();
	void DisplayHelp();

	void StartModeTargetRelative();
	void StartModeVelocity();	
	void StartModeAttitude();
	void StartModeEI();
		
	void ToggleColorMode();
	void ToggleAttHoldMode();
	void SetTrimMode(TRIM_MODE Mode);
	void ToggleTrimStatus();
	void PrintMFDHeading();
	void SetRefAttitude();
	void SelectNextTarget();
	void SelectPrevTarget();
	void SelectClosestTarget();
	void SetTarget();
	void UpdateTargetList();
	void BuildTargetList();
	bool SelectBase();
	void PrintRefMode();
	void PrintAngleRate(const char *Heading, double Angle, double Rate);
	void PrintAngle(const char *Heading, double Angle);
	void PrintRate(const char *Heading, double Rate);
	void PrintRate(const char *Heading, double Rate, bool Trim);
	void PrintPitchThrustLevel(double Level);
	void PrintYawThrustLevel(double Level);
	void PrintRollThrustLevel(double Level);
	void PrintRotThrust();
	void PrintRelVel();

	bool HaveMainEngine();
	bool HaveRetroEngine();
	bool HaveHoverEngine();

	void SaveStatus();
	void LoadSavedStatus();

};

#endif