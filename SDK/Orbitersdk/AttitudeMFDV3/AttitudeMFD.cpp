//////////////////////////////////////////////////////////////////////////////////////////////
//
//	AttitudeMFD.cpp - Primary source file for the AttitudeMFD class
//
//	Copyright Chris Knestrick, 2003 
//	 
//	For a full description of the copyright, please see the file Copyright.txt
//
// ==================================================
// ORBITER 2006 P1 AND VISUAL STUDIO.NET 2005 (VC++ 7.0)
// ==================================================
//
// NOTE: The static libraries supplied with Orbiter 2005 cause references to VCPP 6 standard 
//       libraries. Thus it is necessary to disable them with Linker command line options:
//			/nodefaultlib:"msvcrt.lib" 
//			/nodefaultlib:"msvcirt.lib"
//
//
// Tim Blaxland <tblaxland@gfloth.com.au>
// Orbiter 2006 P1 - Attitude MFD V3.2
//
// 07-Dec-07 tb		Changes so that the attitude hold mode is disabled upon docking but is
//					able to be re-activated manually after docking provided the docked vessel
//					is not running and inbuilt Orbiter auto-pilot.
// 10-Dec-07 tb		Changes so that the current target is not reset when changing MFD
//					modes.
// 14-Dec-07 tb		Changes to improve stability of steering to target attitude, particularly
//					for large attitude errors.
// 19-Dec-07 tb		Changes to rate calculations to improve fuel efficiency and stability up to
//					1000x time acceleration.
//
// Bugfixes required:
// - RelAttitude changes when changing view in VC
//
//
// Tim Blaxland <tblaxland@gfloth.com.au>
// Orbiter 2006 P1 - Attitude MFD V3.1
//
// 10-Oct-07 tb		The previous version of AttitudeMFD had a lot of oscillation and RCS fuel
//					usage when trying to maintain attitude relative to a rotating vector, ie,
//					the velocity vector (in VELOCITY mode) or the vector to a target (in
//					TARGET_RELATIVE mode - assuming you a moving relative to the target, which
//					the typical case). This has been rectified by hacing the auto-pilot null
//					the rotation of the craft relative to the rotation of the target vector
//					instead of trying to null absolute rotation.
// 10-Oct-07 tb		Modified the auto-pilot so that when trying to attain the target attitude
//					from a large angle away, the auto-pilot will only try to rotate two axes
//					at a time. This stops the auto-pilot "hunting" around the target attitude
//					and wasting fuel. Pitch and yaw are done before roll.
// 29-Oct-07 tb		Modified TARGET_RELATIVE mode so that 0 deg roll is measured against the
//					a vector perpendicular to both the velocity vector and the target vector.
//					If the target is within approx 5deg of the velocity vector, local horizon-
//					tal is used instead.
// 29-Oct-07 tb		Modified TARGET_RELATIVE mode so that a PYR offset can be
//					specified as for other modes. Note this means that in TARGET_RELATIVE mode
//					the "P" key does not control the 'previous target (PVT)' function any longer.
// 02-Nov-07 tb		Improved stability at 10x acceleration.
// 05-Nov-07 tb		The auto-pilot now shuts down if time acceleration exceeds 10x because
//					control becomes too unstable.
// 13-Nov-07 tb		Added controls for Rendezvous Pitch Maneuver and Twice Orbital Rate modes
//					for typical Shuttle/ISS missions.// Robert B. Denny <rdenny@dc3.com>
//
//
// Orbiter 2005 - Attitude MFD V3
//
// 15-Jun-05 rbd	For Visual C++ 7 (VS.NET 2005). Disable many unused Windows.H things.
//					Disable warnings resulting from interface to MFDLib and OrbiterAPI
//					MsgProc(). Disable truncation of precision warning, have many of these
//					also. Add explicit typecasts to eliminate a few other warnings instead of
//					disabling them. Add MFD buttons, reorganize ButtonMenu correspondingly.
//					This greatly enhanced my ability to use it since I am keyboard-challenged!
//					Add version to MFD title. Add help page (button command only, I think we've
//					run out of keyboard codes!!!). Change some opcXxx() functions which are now
//					obsolete to the correct ones for Orbiter 2005.
// 16-Jun-05 rbd	New code for thruster and engine forces and torques, complies with 2005API.
//					Look at AttCtrl.cpp for new thruster firing scaling. For now, disallow and
//					immediately stop attitude hold and/or trimming if a docked vehicle is
//					detected. Finished help (the right way).
// 18-Jun-05 rbd	Slow down the thruster graphic updates to the user's selected MFD Update
//					rate. Chris' way of doing it fast and calling InvalidateDisplay() at
//					each simulation time step (from opcTimeStep(), now opcPostStep()) was 
//					a frame-rate killer. On my 800 MHz system it cut the frame rate from 180
//					to 110 on simple scenes. The thruster pix aren't that important.
//
///////////////////////////////////////////////////////////////////////////////////////////////
#define WIN32_LEAN_AND_MEAN
#define NOSERVICE
#define NOMCX
#define NOIME
#define NOSOUND
#define NOKANJI
#define NOIMAGE
#define NOTAPE
#define STRICT
#define ORBITER_MODULE
#define _CRT_SECURE_NO_DEPRECATE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "Orbitersdk.h"
#include "AttitudeMFD.h"
#include "Help.h"
// This MFD uses Trevor Johns MFDlib to make it a bit easier to put some stuff on the screen (http://members.fortunecity.com/tjohns/).
#include "MFDlib.h"

// =====================
// DISABLE SOME WARNINGS
// =====================
//
// 4311 is common to all MFDs due to API
// 4267 is caused by using strlen() in calls to MFDLib's TextOut()
// 4244 is cause by a bunch of places where truncation happens :-(
//
#pragma warning( disable : 4311 4267 4244)

static VESSEL *Spacecraft = NULL;			// Can't be class data, changed by opcFocusChanged()

static struct {  
	int mode;
	AttitudeMFD *CurrentMFD1, *CurrentMFD2;	// This allows two opens copies at once

	struct {
		// This flag indicates if this struct contains valid state data.  It is set to true
		// after a SaveStatus() or an ReadStatus()
		bool ValidStatus;
		
		REF_MODE RefMode;
		ATT_HOLD_MODE AttHoldMode;

		Attitude RelAttitude;

		// For reference modes OTHER than TARGET_RELATIVE
		Attitude RefAttitude;

		// For TARGET_RELATIVE reference mode
		OBJHANDLE TargetRef;		
		int Index;
		char TargetName[MAX_TARGET_NAME];	
		TRIM_STATUS TrimStatus;
		TRIM_MODE TrimMode;

		COLOR_MODE ColorMode;
	} SavedStatus;

} g_AttitudeMFD;


DLLCLBK void InitModule (HINSTANCE hDLL)
{
	static char *name = "Attitude";
	MFDMODESPEC spec;
	spec.name    = name;
	spec.key     = OAPI_KEY_U;
	spec.msgproc = AttitudeMFD::MsgProc;

	g_AttitudeMFD.mode = oapiRegisterMFDMode (spec);
	g_AttitudeMFD.SavedStatus.ValidStatus = false;
}

DLLCLBK void ExitModule (HINSTANCE hDLL)
{
	oapiUnregisterMFDMode (g_AttitudeMFD.mode);
}

//
// This is called -after- the simulation state has been updated
// to the current time. It updates up to two active instances
// of the MFD. **TODO** I believe this should be an array, 
// allowing for some arbitrary max number of instances...
//
DLLCLBK void opcPostStep(double SimT, double SimDT, double mjd)
{
	if (g_AttitudeMFD.CurrentMFD1 != NULL) {
		g_AttitudeMFD.CurrentMFD1->UpdateState(SimDT);
	} 

	if (g_AttitudeMFD.CurrentMFD2 != NULL) {
		g_AttitudeMFD.CurrentMFD2->UpdateState(SimDT);
	}
}

DLLCLBK void opcFocusChanged(OBJHANDLE n, OBJHANDLE o)
{
	Spacecraft = oapiGetVesselInterface(n);
	InitAttControl(Spacecraft);							// In AttCtrl.cpp
}


AttitudeMFD::AttitudeMFD (DWORD w, DWORD h, VESSEL *vessel)
: MFD (w, h, vessel)
{
	// First, for the callback
	if (!g_AttitudeMFD.CurrentMFD1) {
		g_AttitudeMFD.CurrentMFD1 = this;
	} else {
		g_AttitudeMFD.CurrentMFD2 = this;									// We're the second instance of the MFD
	}
	
	//Initialise some variables
	TimeElapsed = 0.0;
	RefMode = USER_ATT;
	AttHoldMode = DISENGAGED;
	RelAttitude = NULL_VECTOR;
	PitchYawRoll = NULL_VECTOR;
	PitchYawRollRate = NULL_VECTOR;
	DelPitchYawRoll = NULL_VECTOR;
	VesselDocked = false;
	VesselDocked_LE = false;
	
	// Initialize for the current vessel now...
	Spacecraft = oapiGetFocusInterface();
	InitAttControl(Spacecraft);							// In AttCtrl.cpp

	// Let's prime the pump :-)  We'll just lie about the time
	UpdateState(UPDATE_INTERVAL + 1.0);		// Add extra second to force update

	SelectClosestTarget(); //Initialise the target for target relative mode

	StartModeAttitude();

	ColorMode = COLOR_OFF;
	HelpPage = HELP_OFF;													// Help mode off
	LINE = h / 28;

	// Store the width and height
	Width = w;
	Height = h;

	// Set the brush for the thruster display
	ThrusterBrush = CreateSolidBrush(RGB(0, 200, 0));
	ThrusterBrushNeg = CreateSolidBrush(RGB(100, 100, 100));

	// For sanity
	CurrentLine = 0;

	// This is a little redundant, since we took the time to write the initial values
	if (g_AttitudeMFD.SavedStatus.ValidStatus) {
		LoadSavedStatus();
	}
}

AttitudeMFD::~AttitudeMFD()
{
	// Which instance are we?
	if (g_AttitudeMFD.CurrentMFD1 == this) {
		g_AttitudeMFD.CurrentMFD1 = NULL;
	} else {
		g_AttitudeMFD.CurrentMFD2 = NULL;	
	}

	// Save our state for future use
	SaveStatus();
}

void AttitudeMFD::SaveStatus()
{
	g_AttitudeMFD.SavedStatus.ValidStatus = true;

	g_AttitudeMFD.SavedStatus.RefMode = RefMode;
	g_AttitudeMFD.SavedStatus.AttHoldMode = AttHoldMode;

	if (RefMode == TARGET_RELATIVE) {
		g_AttitudeMFD.SavedStatus.TargetRef = TargetRef;		
		g_AttitudeMFD.SavedStatus.Index = Index;
		strcpy(g_AttitudeMFD.SavedStatus.TargetName, TargetName);
		g_AttitudeMFD.SavedStatus.TrimStatus = TrimStatus;
		g_AttitudeMFD.SavedStatus.TrimMode = TrimMode;
	} else {
		g_AttitudeMFD.SavedStatus.RefAttitude = RefAttitude;		
		g_AttitudeMFD.SavedStatus.RelAttitude = RelAttitude;		
	}
	
	g_AttitudeMFD.SavedStatus.ColorMode = ColorMode;

}

void AttitudeMFD::LoadSavedStatus()
{
	RefMode = g_AttitudeMFD.SavedStatus.RefMode;
	AttHoldMode = g_AttitudeMFD.SavedStatus.AttHoldMode;

	if (RefMode == TARGET_RELATIVE) {
		TargetRef = g_AttitudeMFD.SavedStatus.TargetRef;		
		Index = g_AttitudeMFD.SavedStatus.Index;
		strcpy(TargetName, g_AttitudeMFD.SavedStatus.TargetName);
		TrimStatus = g_AttitudeMFD.SavedStatus.TrimStatus;
		TrimMode = g_AttitudeMFD.SavedStatus.TrimMode;
	} else {
		RefAttitude = g_AttitudeMFD.SavedStatus.RefAttitude;		
		RelAttitude = g_AttitudeMFD.SavedStatus.RelAttitude;		
	}
	
	ColorMode = g_AttitudeMFD.SavedStatus.ColorMode;
	
	// Force a state update
	UpdateState(UPDATE_INTERVAL + 1.0);		// Add extra second to force update
}

int AttitudeMFD::MsgProc (UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
	case OAPI_MSG_MFD_OPENED:
// warning C4311: 'type cast' : pointer truncation from 'AttitudeMFD *' to 'int' (Common to all MFDs)
		return (int)(new AttitudeMFD (LOWORD(wparam), HIWORD(wparam), (VESSEL*)lparam));
	}
	return 0;
}

void AttitudeMFD::StartModeTargetRelative()
{

	TrimStatus = T_DISENGAGED;
//	SelectClosestTarget();

	CalcTargetRelative();
}


void AttitudeMFD::StartModeVelocity()
{
	CalcVelocity();
}

void AttitudeMFD::StartModeAttitude()
{
	SetRefAttitude();

	CalcAttitude();
}

void AttitudeMFD::StartModeEI()
{
	CalcEI();
}

void AttitudeMFD::SetRefAttitude()
{	
	// Set the current attitude as the reference attitude
	RefAttitude = Status.arot;
}

void AttitudeMFD::ToggleColorMode()
{
	ColorMode = (COLOR_MODE)((ColorMode + 1) % 3);

}

void AttitudeMFD::ToggleAttHoldMode()
{
	if (AttHoldMode != ENGAGED)
	//AttHoldMode = (ATT_HOLD_MODE)((AttHoldMode + 1) % 2);
		AttHoldMode = ENGAGED;
	else
		AttHoldMode = DISENGAGED;
	
	// Stop all engine firings
	Spacecraft->SetAttitudeRotLevel(NULL_VECTOR);
}



void inline AttitudeMFD::SetTrimMode(TRIM_MODE Mode)
{
	if (RefMode != TARGET_RELATIVE) {
		return;
	}

	// If we hit the same key a second time, we'll disable the trim mode
	if ((TrimStatus == T_ENGAGED) && (TrimMode == Mode)) {
		ToggleTrimStatus();
		return;
	} 

	ToggleTrimStatus();
	TrimMode = Mode;
}

void AttitudeMFD::ToggleTrimStatus()
{
	TrimStatus = (TRIM_STATUS)((TrimStatus + 1) % 2);

	// Don't leave the thrusters firing!
	if (TrimStatus == T_DISENGAGED) {
		Spacecraft->SetAttitudeLinLevel(NULL_VECTOR);
	}

}


//
// Update spacecraft state data to be used. This is called from opcPostStep()
// every SimDT seconds -- fast! So most of this is throttled down to interval
// given by UPDATE_INTERVAL.
//
void inline AttitudeMFD::UpdateState(double DT)
{
	TimeElapsed += DT;

	//
	// Update the state of the system - independent of the MFD Update
	// Interval setting in Orbiter because it needs to be fast enough
	// for the autopilot to work right.
	//
	if (TimeElapsed >= UPDATE_INTERVAL)
	{
		Spacecraft->GetStatus(Status);
		oapiGetGlobalPos(Spacecraft->GetHandle(), &GSpacecraftPos);
		oapiGetGlobalVel(Spacecraft->GetHandle(), &GSpacecraftVel);
		Spacecraft->GetShipAirspeedVector(Airspeed);
		AOA = Spacecraft->GetAOA();
		SlipAngle = Spacecraft->GetSlipAngle();
		Mass = Spacecraft->GetMass();

		switch (RefMode) {
		case USER_ATT:
			CalcAttitude();	
			break;
		case VELOCITY:
			CalcVelocity();
			PitchYawRollRate = DelPitchYawRoll / TimeElapsed; //Calc PYR rate of change. Note the vrot of the vessel is not applicable because the reference vector is also rotating.
			break;
		case TARGET_RELATIVE:
			CalcTargetRelative();
			PitchYawRollRate = DelPitchYawRoll / TimeElapsed; //Calc PYR rate of change. Note the vrot of the vessel is not applicable because the reference vector is also rotating.
			break;
		case EI:
			CalcEI();
			break;
		};

		LastTimeElapsed = TimeElapsed;
		TimeElapsed = 0.0;												// Reset state-update throttle
	}
	
	//	==================
	Control();													// The autopilot needs the fastest update
	//	==================
}

VECTOR3 GetPYR(VECTOR3 Pitch, VECTOR3 YawRoll)
{	
	VECTOR3 Res = { 0, 0, 0 };

	// Normalize the vectors
	Pitch = Normalize(Pitch);
	YawRoll = Normalize(YawRoll);
	VECTOR3 H = Normalize(CrossProduct(Pitch, YawRoll));

	Res.data[YAW] = asin(Pitch.x);
//	Res.data[YAW] = -asin(YawRoll.z);

	Res.data[ROLL] = -atan2(H.x, YawRoll.x);
//	Res.data[ROLL] = atan2(YawRoll.y, YawRoll.x);

	Res.data[PITCH] = -atan2(Pitch.y, Pitch.z);
//	Res.data[PITCH] = atan2(H.z, Pitch.z);

	return Res;

}


// This is the same as the previous function, except that we use the transpose matrix
VECTOR3 GetPYR2(VECTOR3 Pitch, VECTOR3 YawRoll)
{	
	VECTOR3 Res = { 0, 0, 0 };

	// Normalize the vectors
	Pitch = Normalize(Pitch);
	YawRoll = Normalize(YawRoll);
	VECTOR3 H = Normalize(CrossProduct(Pitch, YawRoll));

	Res.data[YAW] = -asin(Pitch.x);

	Res.data[ROLL] = atan2(H.x, YawRoll.x);

	Res.data[PITCH] = atan2(Pitch.y, Pitch.z);


	return Res;

}


VECTOR3 GetPY(VECTOR3 PitchYaw)
{	
	VECTOR3 Res = { 0, 0, 0 };

	Res = GetPitchYawRoll(PitchYaw, NULL_VECTOR);

	// Change the signs to make it consistent
	Res.data[PITCH] = -Res.data[PITCH];

	return Res;

}



// Returns a vector containing the pitch, yaw, and roll angles based upon the desired attitude
// relative to the current reference attitude
VECTOR3 inline AttitudeMFD::CalcPitchYawRollAngles()
{
	RefPoints GlobalPts, LocalPts;
	VECTOR3 PitchUnit = {0, 0, 1.0}, YawRollUnit = {1.0, 0, 0};

	RotateVector(PitchUnit, RelAttitude, PitchUnit);
	RotateVector(YawRollUnit, RelAttitude, YawRollUnit);

	RotateVector(PitchUnit, RefAttitude, GlobalPts.Pitch);
	RotateVector(YawRollUnit, RefAttitude, GlobalPts.Yaw);

	GlobalPts.Pitch = GSpacecraftPos + GlobalPts.Pitch;
	GlobalPts.Yaw = GSpacecraftPos + GlobalPts.Yaw;	

	Spacecraft->Global2Local(GlobalPts.Pitch, LocalPts.Pitch);
	Spacecraft->Global2Local(GlobalPts.Yaw, LocalPts.Yaw);

	return GetPYR(LocalPts.Pitch, LocalPts.Yaw);
}

void inline AttitudeMFD::CalcTargetRelative()
{
	VECTOR3 SpacecraftPos, TargetPos, GTargetPos, GVel;
	VECTOR3 GRelPos, H;

	oapiGetGlobalPos(TargetRef, &GTargetPos); // Get position of target
	Spacecraft->Global2Local(GSpacecraftPos, SpacecraftPos); // Convert to positions to local coordinates
	Spacecraft->Global2Local(GTargetPos, TargetPos);
	RelPos = TargetPos - SpacecraftPos; // Calculate relative position of target in local coordinates
	Spacecraft->GetRelativePos(TargetRef, GRelPos); // Get position of spacecraft relative to target
	GRelPos = -GRelPos; // Reverse vector so that the it points from spacecraft to target

	if ( (Mag(CrossProduct(Normalize(-Status.rvel), Normalize(GRelPos)))) < 0.1 ) { // Check to see if target is too close to velocity vector. Corresponds to approx 5deg.
		H = CrossProduct(Status.rpos, GRelPos); // Use local vectical as roll reference
	}
	else {
		H = CrossProduct(Status.rvel, GRelPos); // Use velocity vector as roll reference
	}

	RefAttitude = GetPYR2(GRelPos, H);
	
	DelPitchYawRoll = PitchYawRoll; //Save current pitch, yaw & roll so we can calc the change

	PitchYawRoll = CalcPitchYawRollAngles();

	DelPitchYawRoll = PitchYawRoll - DelPitchYawRoll; //Calculate the change in PYR

	// Calculate relative velocity
	Spacecraft->GetRelativeVel(TargetRef, GVel);
	Spacecraft->Global2Local((GVel + GSpacecraftPos), RelVel);

	// Compute the radial component
	RadialVel = (RelPos * RelVel) / Mag(RelPos);


}

void inline AttitudeMFD::CalcAttitude()
{
	PitchYawRoll = CalcPitchYawRollAngles();

}

void inline AttitudeMFD::CalcVelocity()
{
	VECTOR3 H;

	H = CrossProduct(Status.rpos, Status.rvel);
	
	RefAttitude = GetPYR2(Status.rvel, H);

	DelPitchYawRoll = PitchYawRoll; //Save current pitch, yaw & roll so we can calc the change
	PitchYawRoll = CalcPitchYawRollAngles();
	DelPitchYawRoll = PitchYawRoll - DelPitchYawRoll; //Calculate the change in PYR
	
}


void inline AttitudeMFD::CalcEI()
{
	VECTOR3 H;
	CalcEntryInterface(Interface);

	if (!Interface.InterfaceDefined) {
		return;
	}

	H = CrossProduct(Interface.Pos, Interface.Vel);

	RefAttitude = GetPYR2(Interface.Vel, H);

	PitchYawRoll = CalcPitchYawRollAngles();
	
}


void inline AttitudeMFD::Control()
{
	int VesselDockedCount; //The number of vessels docked to us
	OBJHANDLE DockedObj;
	VESSEL *DockedVessel;

	if (AttHoldMode!=DISENGAGED)
	{
		//
		// Don't allow AttHold or Trimming if Entry Interface mode
		// and no interface defined.
		//
		if (RefMode == EI && !Interface.InterfaceDefined) {
			if(TrimStatus == T_ENGAGED) ToggleTrimStatus();
			AttHoldMode = DISENGAGED;
			return;
		}

		//
		// Turn off AttHold/Trimming if TimeAccel>10. The auto-pilot is unstable at high time accel.
		//
/*		if (oapiGetTimeAcceleration() > 10) {
			if (TrimStatus == T_ENGAGED) ToggleTrimStatus();
			AttHoldMode = DISENGAGED;
			return;
		}*/

		//
		// Auto-stop AttHold and Trimming if we docked or something docked to us.
		//
		VesselDockedCount = 0;
		for(unsigned int i = 0; i < Spacecraft->DockCount(); i++) { //Determine the number of vessels docked to us
			if((DockedObj=Spacecraft->GetDockStatus(Spacecraft->GetDockHandle(i))) != NULL) {
				VesselDockedCount++;
				DockedVessel = oapiGetVesselInterface(DockedObj);
				for (int j=1;j<=7;j++) {
					if (DockedVessel->GetNavmodeState(j)) { //Disable the auto-pilot if the other vessel is running an Orbiter navmode
						if(TrimStatus == T_ENGAGED) ToggleTrimStatus();
						AttHoldMode = DISENGAGED;
						return;
					}
				}
			}
		}
		if (VesselDockedCount==0) { 
			VesselDocked = false; 
		}
		else {
			if (!VesselDocked) {
				VesselDocked = true;
				VesselDocked_LE = true; //Detects if a vessel just docked or was been docked before
			}
			else {
				VesselDocked_LE = false; //
			}
		}
		if (VesselDocked_LE) { //Disable the auto-pilot if we just docked to something
			if(TrimStatus == T_ENGAGED) ToggleTrimStatus();
			AttHoldMode = DISENGAGED;
			return;
		}

	}

	if (AttHoldMode==ENGAGED)
	{
		// Use different function calls to SetAttitude for the rotating reference vector cases than the static reference vector cases
		if ( (fabs(PitchYawRoll.data[YAW]) + fabs(PitchYawRoll.data[PITCH])) > DEADBAND_MAX ) {
			SetAttitude(0, PitchYawRoll.data[PITCH], PITCH, DB_COARSE, RATE_NULL_HIGH, LastTimeElapsed);
			SetAttitude(0, PitchYawRoll.data[YAW], YAW, DB_COARSE, RATE_NULL_HIGH, LastTimeElapsed);
			SetAttitude(0, 0, ROLL, DB_COARSE, RATE_NULL_HIGH, TimeElapsed);
		}
		else {
			if ((RefMode == VELOCITY)||(RefMode == TARGET_RELATIVE)) {
				SetAttitude(0, PitchYawRoll.data[PITCH], PitchYawRollRate.data[PITCH], PITCH, DB_FINE, RATE_NULL_HIGH, LastTimeElapsed);
				SetAttitude(0, PitchYawRoll.data[YAW], PitchYawRollRate.data[YAW], YAW, DB_FINE, RATE_NULL_HIGH, LastTimeElapsed);
				SetAttitude(0, PitchYawRoll.data[ROLL], PitchYawRollRate.data[ROLL], ROLL, DB_FINE, RATE_NULL_HIGH, LastTimeElapsed);
			}
			else {
				SetAttitude(0, PitchYawRoll.data[PITCH], PITCH, DB_MIN, RATE_NULL_LOW, LastTimeElapsed);
				SetAttitude(0, PitchYawRoll.data[YAW], YAW, DB_MIN, RATE_NULL_LOW, LastTimeElapsed);
				SetAttitude(0, PitchYawRoll.data[ROLL], ROLL, DB_MIN, RATE_NULL_LOW, LastTimeElapsed);
			}
		}
	}

	if (AttHoldMode==RPM)
	{
		SetAttitude(0, 0, Status.vrot.data[PITCH] - RPM_PITCH_RATE, PITCH, DB_MIN, RATE_NULL_LOW, UPDATE_INTERVAL);
		SetAttitude(0, 0, YAW, DB_MIN, RATE_NULL_LOW, UPDATE_INTERVAL);
		SetAttitude(0, 0, ROLL, DB_MIN, RATE_NULL_LOW, UPDATE_INTERVAL);
	}

	if (AttHoldMode==TOR)
	{
		double TOR_Rate;
		ELEMENTS TOR_els;
		ORBITPARAM TOR_op;

		Spacecraft->GetElements(Spacecraft->GetGravityRef(), TOR_els, &TOR_op);
		TOR_Rate = 2*PI/TOR_op.T;

		SetAttitude(0, 0, Status.vrot.data[PITCH] - TOR_Rate, PITCH, DB_MIN, RATE_NULL_LOW, UPDATE_INTERVAL);
		SetAttitude(0, 0, YAW, DB_MIN, RATE_NULL_LOW, UPDATE_INTERVAL);
		SetAttitude(0, 0, ROLL, DB_MIN, RATE_NULL_LOW, UPDATE_INTERVAL);
	}

	if (TrimStatus == T_ENGAGED) {
		Trim();
	}

}

void inline AttitudeMFD::SetTrimLevelVert()
{
	double Level = 0.0;

	// First, we'll calculate the thrust level assuming that we'll
	// be using the linear thruster
	Level = -(Mass * RelVel.data[TA_VERT]) / GetMaxForce(TH_VRT);
	
	// We need to decided if we really want to use linear thrusters, or if
	// it's better to use one of the engines
	if (HaveHoverEngine() && Level > 3.0) {
			Level = -(Mass * RelVel.data[TA_VERT]) / GetMaxForce(EN_HOVER);
			NormalizeThrustLevel(Level);
			Spacecraft->SetEngineLevel(ENGINE_HOVER, Level);
	} else {
		NormalizeThrustLevel(Level);
		Spacecraft->SetAttitudeLinLevel((int)TA_VERT, Level);
	}

}

void inline AttitudeMFD::SetTrimLevelLat()
{
	double Level = 0.0;

	// First, we'll calculate the thrust level assuming that we'll
	// be using the linear thruster
	Level = -(Mass * RelVel.data[TA_LAT]) / GetMaxForce(TH_LAT);
	
	NormalizeThrustLevel(Level);
	Spacecraft->SetAttitudeLinLevel((int)TA_LAT, Level);
	
}


void inline AttitudeMFD::SetTrimLevelFA()
{
	double Level = 0.0;

	// First, we'll calculate the thrust level assuming that we'll
	// be using the linear thruster
	Level = -(Mass * RelVel.data[TA_FA]) / GetMaxForce(TH_LNG);

	if (HaveMainEngine() && Level > 3.0) {
		Level = -(Mass * RelVel.data[TA_FA]) / GetMaxForce(EN_MAIN);
		NormalizeThrustLevel(Level);
		Spacecraft->SetEngineLevel(ENGINE_MAIN, Level);
	} else if (HaveRetroEngine() &&Level < -3.0) {
		Level = (Mass * RelVel.data[TA_FA]) / GetMaxForce(EN_RETRO);
		NormalizeThrustLevel(Level);
		Spacecraft->SetEngineLevel(ENGINE_RETRO, Level);
	} else {
		NormalizeThrustLevel(Level);
		Spacecraft->SetAttitudeLinLevel((int)TA_FA, Level);
	}
}

void inline AttitudeMFD::SetTrimThrustLevel(T_AXIS Axis)
{
	const double DEADBAND = 0.001;

	if (fabs(RelVel.data[Axis]) > DEADBAND) {	
		switch (Axis) {
		case TA_VERT:
			SetTrimLevelVert();
			TrimStatus = T_ENGAGED;
			break;
		case TA_LAT:
			SetTrimLevelLat();
			TrimStatus = T_ENGAGED;
			break;
		case TA_FA:
			SetTrimLevelFA();
			TrimStatus = T_ENGAGED;
			break;
		default:
			break;
		}	
	}
}


// This function assumes that trim is engaged
void inline AttitudeMFD::Trim()
{
	
	// "Clear the board"
	Spacecraft->SetAttitudeLinLevel(NULL_VECTOR);

	// This will be reset by SetTrimThrustLevel() if trim is used
	TrimStatus = T_DISENGAGED;			

	if (TrimMode == T_VERT || 
		TrimMode == T_ALL || 
		TrimMode == T_VERT_LAT ||
		TrimMode == T_VERT_FA) {
	
		// Clear hover engine thrust
		Spacecraft->SetEngineLevel(ENGINE_HOVER, 0.0);

		SetTrimThrustLevel(TA_VERT);
	}

	 if (TrimMode == T_LAT ||
		 TrimMode == T_ALL ||
		 TrimMode == T_VERT_LAT ||
		 TrimMode == T_LAT_FA) {
		SetTrimThrustLevel(TA_LAT);
	 }

	 if (TrimMode == T_FA || 
		 TrimMode == T_ALL ||
		 TrimMode == T_LAT_FA ||
		 TrimMode == T_VERT_FA) {

		 // Clear main/retro thrust
		Spacecraft->SetEngineLevel(ENGINE_MAIN, 0.0);
		Spacecraft->SetEngineLevel(ENGINE_RETRO, 0.0);

		SetTrimThrustLevel(TA_FA);
	}

}

// ==================
// MAIN UPDATE METHOD
// ==================

void AttitudeMFD::Update(HDC hdc)
{

	// These are used only by PrintXxxThrustLevel() and thus are display-only
	// items. Earlier versions of this MFD updated the thruster graphics really
	// fast, and the result was major frame-rate killing. The graphics aren't
	// really too useful, so we let the user control them via the Orbiter
	// MFD Update Rate which controls how often this is called. No more calling
	// of InvalidateDisplay() from the high speed UpdateState() method!
	//
	RotLevel.data[PITCH] = Spacecraft->GetThrusterGroupLevel(THGROUP_ATT_PITCHUP) - 
		Spacecraft->GetThrusterGroupLevel(THGROUP_ATT_PITCHDOWN);
	RotLevel.data[YAW] = Spacecraft->GetThrusterGroupLevel(THGROUP_ATT_YAWRIGHT) - 
		Spacecraft->GetThrusterGroupLevel(THGROUP_ATT_YAWLEFT);
	RotLevel.data[ROLL] = Spacecraft->GetThrusterGroupLevel(THGROUP_ATT_BANKRIGHT) - 
		Spacecraft->GetThrusterGroupLevel(THGROUP_ATT_BANKLEFT);

	CurrentLine = 0;
	hDC = hdc;

	PrintMFDHeading();
	CurrentLine += LINE;

	if(HelpPage != HELP_OFF)												// If help is active
		DisplayHelp();														// Display current help page
	else
	{
		switch (RefMode) {
		case USER_ATT:			DisplayAttitude(); break;
		case VELOCITY:			DisplayVelocity(); break;
		case TARGET_RELATIVE:	DisplayTargetRelative(); break;
		case EI:				DisplayEI(); break;
		}
	}
	
}


void AttitudeMFD::DisplayTargetRelative()
{
	char Buffer[100];

	PrintRefMode();

	sprintf(Buffer, "Target: %s", TargetName);
//(typ.) warning C4267: 'argument' : conversion from 'size_t' to 'int', possible loss of data
	TextOut(hDC, 5, CurrentLine, Buffer, strlen(Buffer));
	CurrentLine += LINE;

	// Print distance
	sprintf(Buffer, "Distance:", 8);
	TextOut(hDC, 5, CurrentLine, Buffer, strlen(Buffer));
	ScaleOutput(Buffer, Mag(RelPos));
	TextOut(hDC, 100, CurrentLine, Buffer, strlen(Buffer));
	CurrentLine += LINE;
	sprintf(Buffer, "Rad Vel:", 8);
	TextOut(hDC, 5, CurrentLine, Buffer, strlen(Buffer));
	ScaleOutput(Buffer, RadialVel);
	TextOut(hDC, 100, CurrentLine, Buffer, strlen(Buffer));
	CurrentLine += 2 * LINE;

	PrintRelVel();
	CurrentLine += 2 * LINE;

	PrintAngleRate("Pitch:", PitchYawRoll.data[PITCH], PitchYawRollRate.data[PITCH]);
	PrintAngleRate("Yaw:", PitchYawRoll.data[YAW], PitchYawRollRate.data[YAW]);
	PrintAngleRate("Roll:", PitchYawRoll.data[ROLL], PitchYawRollRate.data[ROLL]);

	CurrentLine += 2 * LINE;

	PrintRotThrust();

}

void AttitudeMFD::DisplayVelocity()
{
	char Buffer[100];

	PrintRefMode();
	
	// Print velocity
	TextOut(hDC, 5, CurrentLine, "Velocity:", 9);	
	ScaleOutput(Buffer, Mag(Status.rvel));

	TextOut(hDC, 100, CurrentLine, Buffer, strlen(Buffer));
	CurrentLine += 2 * LINE;

	PrintAngle("Set Pitch", RelAttitude.data[PITCH]);
	PrintAngle("Set Yaw ", RelAttitude.data[YAW]);
	PrintAngle("Set Roll", RelAttitude.data[ROLL]);
	CurrentLine += 2 * LINE;

	PrintAngleRate("Pitch:", PitchYawRoll.data[PITCH], PitchYawRollRate.data[PITCH]);
	PrintAngleRate("Yaw:", PitchYawRoll.data[YAW], PitchYawRollRate.data[YAW]);
	PrintAngleRate("Roll:", PitchYawRoll.data[ROLL], PitchYawRollRate.data[ROLL]);

	CurrentLine += 2 * LINE;

	PrintRotThrust();

}

void AttitudeMFD::DisplayAttitude()
{
	char Buffer[100];
	PrintRefMode();

	sprintf(Buffer, "Reference Att: %.1f %.1f %.1f", 
				DEG * RefAttitude.x, DEG * RefAttitude.y, DEG * RefAttitude.z );
	TextOut(hDC, 5, CurrentLine, Buffer, strlen(Buffer));
	CurrentLine += LINE;	

	sprintf(Buffer, "Current Att:   %.1f %.1f %.1f", 
			DEG * Status.arot.x, DEG * Status.arot.y, DEG * Status.arot.z );
	TextOut(hDC, 5, CurrentLine, Buffer, strlen(Buffer));
	CurrentLine += 2 * LINE;

	PrintAngle("Set Pitch", RelAttitude.data[PITCH]);
	PrintAngle("Set Yaw ", RelAttitude.data[YAW]);
	PrintAngle("Set Roll", RelAttitude.data[ROLL]);
	CurrentLine += 2 * LINE;


	PrintAngleRate("Pitch:", PitchYawRoll.data[PITCH], Status.vrot.x);
	PrintAngleRate("Yaw:", PitchYawRoll.data[YAW], Status.vrot.y);
	PrintAngleRate("Roll:", PitchYawRoll.data[ROLL], Status.vrot.z);

	CurrentLine += 2 * LINE;

	PrintRotThrust();

}


void AttitudeMFD::DisplayEI()
{
	char Buffer[100];

	PrintRefMode();

	if (Interface.InterfaceDefined) {
		sprintf(Buffer, "Entry Angle:   %.2f", Degree(Interface.Angle));
		TextOut(hDC, 5, CurrentLine, Buffer, strlen(Buffer));
		CurrentLine += LINE;

		sprintf(Buffer, "Time To Go:    %.f", Interface.TimeToGo);
		TextOut(hDC, 5, CurrentLine, Buffer, strlen(Buffer));
		CurrentLine += 2 * LINE;
	} else {
		CurrentLine += LINE;
		sprintf(Buffer, "Entry Interface Undefined");
		TextOut(hDC, 5, CurrentLine, Buffer, strlen(Buffer));
		CurrentLine += 2 * LINE;
			
		return;
	}


	PrintAngle("Set Pitch", RelAttitude.data[PITCH]);
	PrintAngle("Set Yaw ", RelAttitude.data[YAW]);
	PrintAngle("Set Roll", RelAttitude.data[ROLL]);
	CurrentLine += 2 * LINE;


	PrintAngleRate("Pitch:", PitchYawRoll.data[PITCH], Status.vrot.x);
	PrintAngleRate("Yaw:", PitchYawRoll.data[YAW], Status.vrot.y);
	PrintAngleRate("Roll:", PitchYawRoll.data[ROLL], Status.vrot.z);

	CurrentLine += 2 * LINE;

	PrintRotThrust();


}

//
// Help pages can be up to 35 char wide and 25 lines
// on the "normal" MFD displays.
//
void AttitudeMFD::DisplayHelp()
{
	if(HelpPage > MAX_HELP_PAGE) return;		// Prevent crash on programmer error;
	int i1 = (HelpPage - 1) * 25;				// Index of 1st line forthis page

	for(int i = 0; i < 25; i++)
	{
		const char *cp = HelpLines[i1 + i];
		if(cp == 0) break;						// No more help lines in this page
		COLORREF prevClr = BLACK;				// [sentinel]
		switch(*cp)								// Special 1st chars
		{
		case '^': prevClr = SetMFDColor(hDC, BRIGHTWHITE); cp += 1; break;
		case '%': prevClr = SetMFDColor(hDC, BRIGHTCYAN); cp += 1; break;
		}
		TextOut(hDC, 5, CurrentLine, cp, strlen(cp));
		if(prevClr != BLACK) SetMFDColor(hDC, prevClr);
		CurrentLine += LINE;
	}
}

void AttitudeMFD::ChangeRefMode(REF_MODE Mode)
{	
	Spacecraft->SetAttitudeRotLevel(NULL_VECTOR);
	AttHoldMode = DISENGAGED;

	RefMode = Mode;

	switch (Mode) {
	case USER_ATT:
		StartModeAttitude();
		break;
	case TARGET_RELATIVE:
		StartModeTargetRelative();
		break;
	case VELOCITY:
		StartModeVelocity();
		break;
	case EI:
		StartModeEI();
		break;
	}

}

bool AttitudeMFD::ConsumeKeyBuffered(DWORD key)
{
	bool cbSelectTarget(void *id, char *str, void *data);
	bool cbSetMode(void *id, char *str, void *data);
	bool cbSetRelAttPitch(void *id, char *str, void *data);
	bool cbSetRelAttYaw(void *id, char *str, void *data);
	bool cbSetRelAttRoll(void *id, char *str, void *data);

	//
	// Trap any key to advance help pages, when done, will
	// return to normal mode.
	//
	if(HelpPage != HELP_OFF)
	{
		HelpPage += 1;
		if(HelpPage > MAX_HELP_PAGE) HelpPage = HELP_OFF;		// No more help, return to normal mode
		return true;											// Return,we handled this key
	}

	switch(key) {
	// Mode keys - the M key only needs to be used as a backup, when the SHIFT + number
	// is in use by a vessel, such as the shuttle's RMS
	case OAPI_KEY_M:
		oapiOpenInputBox("Select Mode", cbSetMode, 0, 20, (void *)this);
	case OAPI_KEY_1:
		ChangeRefMode(USER_ATT);
		return true;
	case OAPI_KEY_2:
		ChangeRefMode(VELOCITY);
		return true;
	case OAPI_KEY_3:
		ChangeRefMode(TARGET_RELATIVE);
		return true;
	case OAPI_KEY_4:
		ChangeRefMode(EI);
		return true;

	case OAPI_KEY_P:
		//if (RefMode != TARGET_RELATIVE) {
			oapiOpenInputBox("Select Pitch", cbSetRelAttPitch, 0, 20, (void *)this);
		//} else {
		//	SelectPrevTarget();
		//}
		return true;
	case OAPI_KEY_Y:
		//if (RefMode != TARGET_RELATIVE) {
			oapiOpenInputBox("Select Yaw", cbSetRelAttYaw, 0, 20, (void *)this);
		//}
		return true;
	case OAPI_KEY_R:
		//if (RefMode != TARGET_RELATIVE) {
			oapiOpenInputBox("Select Roll", cbSetRelAttRoll, 0, 20, (void *)this);
		//}
		return true;

	// Target relative keys
	case OAPI_KEY_T:
		if (RefMode == TARGET_RELATIVE) {
			oapiOpenInputBox("Select Target", cbSelectTarget, 0, 20, (void *)this);
		}
		return true;														// No target selected!
	case OAPI_KEY_N:
		if (RefMode == TARGET_RELATIVE) {
			SelectNextTarget();
		}
		return true;
	case OAPI_KEY_SPACE:
		if (RefMode == TARGET_RELATIVE) {
			SelectClosestTarget();
		}
		return true;
	case OAPI_KEY_B:
		if (RefMode == TARGET_RELATIVE) {
			SelectBase();
		}
		return true;

	case OAPI_KEY_H:
		ToggleAttHoldMode();
		return true;
	case OAPI_KEY_C:
		ToggleColorMode();
		return true;
	case OAPI_KEY_PERIOD:
		if (RefMode == USER_ATT) {
			SetRefAttitude();
		}
		return true;

	// Trim functions
	case OAPI_KEY_NUMPAD5:
		SetTrimMode(T_ALL);
		return true;
	case OAPI_KEY_NUMPAD1:
		SetTrimMode(T_VERT);
		return true;
	case OAPI_KEY_NUMPAD2:
		SetTrimMode(T_LAT);
		return true;
	case OAPI_KEY_NUMPAD3:
		SetTrimMode(T_FA);
		return true;
	case OAPI_KEY_NUMPAD7:
		SetTrimMode(T_VERT_LAT);
		return true;
	case OAPI_KEY_NUMPAD8:
		SetTrimMode(T_LAT_FA);
		return true;
	case OAPI_KEY_NUMPAD9:
		SetTrimMode(T_VERT_FA);
		return true;

	// Help
	case OAPI_KEY_NUMPAD0:
		HelpPage += 1;
		if(HelpPage > MAX_HELP_PAGE) HelpPage = HELP_OFF;		// No more help, return to normal mode
		return true;											// Return,we handled this key

	default:
		return false;
	}

}

const int NUM_CMNDS = 30;
bool AttitudeMFD::ConsumeButton (int bt, int event)
{  
	static const DWORD btkey[NUM_CMNDS] = 
	{
//		== Page 1 ==
		OAPI_KEY_M,OAPI_KEY_P,OAPI_KEY_Y,OAPI_KEY_R,OAPI_KEY_PERIOD,OAPI_KEY_H,
		OAPI_KEY_T,OAPI_KEY_SPACE,OAPI_KEY_NUMPAD5,OAPI_KEY_NUMPAD1,OAPI_KEY_NUMPAD2,OAPI_KEY_NUMPAD3,
//		== Page 2 ==
		OAPI_KEY_N,99,OAPI_KEY_B,99,99,99,
		OAPI_KEY_NUMPAD7,OAPI_KEY_NUMPAD8,OAPI_KEY_NUMPAD9,99,99,99,
//		== Page 3 ==
		OAPI_KEY_1,OAPI_KEY_2,OAPI_KEY_3,OAPI_KEY_4,OAPI_KEY_NUMPAD0,OAPI_KEY_C
	};

	if (!(event & PANEL_MOUSE_LBDOWN) || (bt >= NUM_CMNDS) || (bt == 99)) return false;

	//
	// If help is already active, any button cycles
	// through the help pages.
	//
	if(HelpPage != HELP_OFF)
	{
		HelpPage += 1;
		if(HelpPage > MAX_HELP_PAGE) HelpPage = HELP_OFF;		// No more help, return to normal mode
		return true;											// Return,we handled this key
	}

	// The MOD (mode) button toggles between reference modes, instead of
	// requiring the user to enter a cryptic numeric code.
	//
	if(btkey[bt] == OAPI_KEY_M)
	{
		switch(RefMode)
		{
		case USER_ATT:			ChangeRefMode(VELOCITY); break;
		case VELOCITY:			ChangeRefMode(TARGET_RELATIVE); break;
		case TARGET_RELATIVE:	ChangeRefMode(EI); break;
		case EI:				ChangeRefMode(USER_ATT); break;
		default:				ChangeRefMode(USER_ATT); break;
		}
		return true;												// Return, we processed the key!
	}

	// This is required because there is no keyboard shortcut for PVT button.
	if(bt==13) {
		SelectPrevTarget();
		return true;
	}

	// RPM button
	if(bt==22) {
		if (AttHoldMode != RPM)
			AttHoldMode = RPM;
		else
			AttHoldMode = DISENGAGED;
		
		// Stop all engine firings
		Spacecraft->SetAttitudeRotLevel(NULL_VECTOR);
		return true;
	}

	// TOR button
	if(bt==23) {
		if (AttHoldMode != TOR)
			AttHoldMode = TOR;
		else
			AttHoldMode = DISENGAGED;
		
		// Stop all engine firings
		Spacecraft->SetAttitudeRotLevel(NULL_VECTOR);
		return true;
	}

	//
	// All other buttons map to the corresponding keyboard key
	//
	return ConsumeKeyBuffered(btkey[bt]);

	return false;
}


char *AttitudeMFD::ButtonLabel (int bt)
{
	char *label[NUM_CMNDS] = 
	{
//		== Page 1 ==
		"MOD","PCH","YAW","ROL","REF","HLD",
		"TGT","CLT","TRA","TRV","TRL","TRF",
//		== Page 2 ==
		"NXT","PVT","BAS",   "",   "",   "",
		"TVL","TLF","TVF",   "","RPM","TOR",
//		== Page 3 ==
		"ATT","VEL","TGT","ENT","HLP","CLR"
	};

	return (bt < NUM_CMNDS ? label[bt] : 0);
}

int AttitudeMFD::ButtonMenu (const MFDBUTTONMENU **menu) const
{
	static const MFDBUTTONMENU mnu[NUM_CMNDS] = {

//		== Page 1 ==
		{"Select Mode", 0, 'M'},
		{"Set Pitch", 0, 'P'},
		{"Set Yaw", 0, 'Y'},
		{"Set Roll", 0, 'R'},
		{"Set Reference", "Attitude", '.'},
		{"Attitude Hold", 0, 'H'},

		{"Select Target", "(Target Mode)", 'T'},
		{"Closest Target", "(Target Mode)", ' '},
		{"Trim All", "(Numpad)", '5'},
		{"Trim Vertical", "(Numpad)", '1'},
		{"Trim Lateral", "(Numpad)", '2'},
		{"Trim F/A", "(Numpad)", '3'},

//		== Page 2==
		{"Next Target", "(Target Mode)", 'N'},
		{"Prev Target", "(Target Mode)", '\0'},
		{"Select Base", "(Target Mode)", 'B'},
		{"", "", '\0'},
		{"", "", '\0'},
		{"", "", '\0'},

		{"Trim Vert, Lat", "(Numpad)", '7'},
		{"Trim Lat, F/A", "(Numpad)", '8'},
		{"Trim Vert, F/A", "(Numpad)", '9'},
		{"", "", '\0'},
		{"Rdzv Pitch Manvr", 0, '\0'},
		{"Twice Orb Rate", 0, '\0'},

//		== Page 3 ==
		{"User Att", "Mode", '1'},
		{"Velocity", "Mode", '2'},
		{"Target Rel", "Mode", '3'},
		{"Entry Interface", "Mode", '4'},
		{"MFD Help", "(Numpad)", '0'},
		{"Toggle Color Mode", 0, 'C'}

	};
	if (menu) *menu = mnu;
	return NUM_CMNDS;
}

bool AttitudeMFD::SelectTarget(char *Name)
{
	OBJHANDLE Obj = oapiGetObjectByName(Name);

	if (Obj) {
		UpdateTargetList();
		
		for (unsigned int i = 0; (i < oapiGetObjectCount()) && (Obj != CurrentTarget); TargetList.Next())
			;
		SetTarget();

		return true;
	}

	return false;
}


bool AttitudeMFD::SelectBase()
{
	VESSELSTATUS Status;

	oapiGetFocusInterface()->GetStatus(Status);

	if (Status.base) {
		TargetRef = Status.base;
		oapiGetObjectName(TargetRef, TargetName, 20);

		Index = 0;

		return true;
	}

	return false;
}


void AttitudeMFD::SelectNextTarget()
{
	UpdateTargetList();
	TargetList.Next();

	// We don't want to select ourselves
	if (CurrentTarget == oapiGetFocusObject()) {
		TargetList.Next();		
	}
	
	SetTarget();
}

void AttitudeMFD::SelectPrevTarget()
{
	UpdateTargetList();
	TargetList.Prev();

	// We don't want to select ourselves
	if (CurrentTarget == oapiGetFocusObject()) {
		TargetList.Prev();		
	}

	SetTarget();
}

void AttitudeMFD::SelectClosestTarget()
{
	UpdateTargetList();
	TargetList.ResetCurrent();
	TargetList.Next();

	SetTarget();
}

// Sets the target to the current object in the target list
void AttitudeMFD::SetTarget()
{
	// Set the TargetRef and target name for the MFD
	TargetRef = CurrentTarget;
	oapiGetObjectName(TargetRef, TargetName, MAX_TARGET_NAME); 
}

// In the default case (no new objects and the existing ones are sorted) this should quickly
// fall through.
void inline AttitudeMFD::UpdateTargetList()
{
	// See if any new objects have been added to the universe.  This assumes that
	// once an object is created, it never is deleted.  This will break if an object is both
	// added and deleted, resulting in the same total number of objects.  However, I think
	// that this should work in practice.
	if (oapiGetObjectCount() != (unsigned int)TargetList.Length()) {
		BuildTargetList();
	} else {
		TargetList.Sort();
	}
}

void AttitudeMFD::BuildTargetList()
{
	OBJHANDLE Obj;
	TargetObj Target;

	// Clear existing list
	TargetList.DeleteList();

	for (unsigned int i = 0; i < oapiGetObjectCount(); i++) {
		Obj = oapiGetObjectByIndex(i);
		Target.SetObj(Obj);
		TargetList.Insert(Target);
	}

	// Sort the new list
	TargetList.Sort();
}

void inline AttitudeMFD::PrintMFDHeading()
{
	const char *cp = "Attitude MFD V3.2";
	COLORREF prevClr = SetMFDColor(hDC, WHITE);
	TextOut(hDC, 5, CurrentLine, cp, strlen(cp));
	SetMFDColor(hDC, prevClr);
	SetTextColor(hDC, RGB(0, 255, 0));
	CurrentLine += LINE;
}

void AttitudeMFD::PrintRefMode()
{
	char Buffer[100], Buffer2[10];
		
	sprintf(Buffer, "Ref Mode:");
	TextOut(hDC, 5, CurrentLine, Buffer, strlen(Buffer));
	
	switch (RefMode) {
	case USER_ATT:
		sprintf(Buffer, "Attitude");	
		break;
	case VELOCITY:
		sprintf(Buffer, "%s", "Velocity");
		break;
	case TARGET_RELATIVE:
		sprintf(Buffer, "%s", "Target Relative");
		break;
	case EI:
		sprintf(Buffer, "%s", "Entry Interface");
		break;
	};
	

	if (AttHoldMode == ENGAGED) {
		sprintf(Buffer2, "%s", " (Hold)");
		strcat(Buffer, Buffer2);
	}
	if (AttHoldMode == RPM) {
		sprintf(Buffer2, "%s", " (RPM)");
		strcat(Buffer, Buffer2);
	}
	if (AttHoldMode == TOR) {
		sprintf(Buffer2, "%s", " (TOR)");
		strcat(Buffer, Buffer2);
	}

	SetTextColor(hDC, RGB(255, 255, 255));
	TextOut(hDC, 90, CurrentLine, Buffer, strlen(Buffer));
	SetTextColor(hDC, RGB(0, 255, 0));

	CurrentLine += LINE;
}

void AttitudeMFD::PrintAngleRate(const char *Heading, double Angle, double Rate)
{
	const int TAB = 100;
	char Buffer[10];	

	TextOut(hDC, 5, CurrentLine, Heading, strlen(Heading));
	
	sprintf(Buffer, "%.3f", DEG * Angle);

	// Adjust for negative number
	if (Angle < 0) {
		if (ColorMode == COLOR_WHITE) {
			sprintf(Buffer, "%.3f", DEG * Angle);
			SetTextColor(hDC, RGB(255, 255, 255));
			TextOut(hDC, TAB, CurrentLine, Buffer, strlen(Buffer));
			SetTextColor(hDC, RGB(0, 255, 0));
		} else if (ColorMode == COLOR_RED) {
			sprintf(Buffer, "%.3f", DEG * Angle);
			SetTextColor(hDC, RGB(255, 0, 0));
			TextOut(hDC, TAB, CurrentLine, Buffer, strlen(Buffer));
			SetTextColor(hDC, RGB(0, 255, 0));
		} else {
			sprintf(Buffer, "%.3f", DEG * Angle);
			TextOut(hDC, TAB, CurrentLine, Buffer, strlen(Buffer));
		} 
	} else {
		sprintf(Buffer, "+%.3f", DEG * Angle);
		TextOut(hDC, TAB, CurrentLine, Buffer, strlen(Buffer));
	}

	// Adjust for negative number
	if (Rate < 0) {
		if (ColorMode == COLOR_WHITE) {
			sprintf(Buffer, "%.3f", DEG * Rate);
			SetTextColor(hDC, RGB(255, 255, 255));
			TextOut(hDC, 2 * TAB, CurrentLine, Buffer, strlen(Buffer));
			SetTextColor(hDC, RGB(0, 255, 0));
		} else if (ColorMode == COLOR_RED) {
			sprintf(Buffer, "%.3f", DEG * Rate);
			SetTextColor(hDC, RGB(255, 0, 0));
			TextOut(hDC, 2 * TAB, CurrentLine, Buffer, strlen(Buffer));
			SetTextColor(hDC, RGB(0, 255, 0));
		} else {
			sprintf(Buffer, "%.3f", DEG * Rate);
			TextOut(hDC, 2 * TAB, CurrentLine, Buffer, strlen(Buffer));
		} 
	} else {
		sprintf(Buffer, "+%.3f", DEG * Rate);
		TextOut(hDC, 2 * TAB, CurrentLine, Buffer, strlen(Buffer));
	}

	CurrentLine += LINE;
}

void AttitudeMFD::PrintAngle(const char *Heading, double Angle)
{
	const int TAB = 100;
	char Buffer[10];	

	TextOut(hDC, 5, CurrentLine, Heading, strlen(Heading));
	
	sprintf(Buffer, "%.2f", Angle);

	// Adjust for negative number
	if (Angle < 0) {
		if (ColorMode == COLOR_WHITE) {
			sprintf(Buffer, "%.2f", DEG * Angle);
			SetTextColor(hDC, RGB(255, 255, 255));
			TextOut(hDC, TAB, CurrentLine, Buffer, strlen(Buffer));
			SetTextColor(hDC, RGB(0, 255, 0));
		} else if (ColorMode == COLOR_RED) {
			sprintf(Buffer, "%.2f", DEG * Angle);
			SetTextColor(hDC, RGB(255, 0, 0));
			TextOut(hDC, TAB, CurrentLine, Buffer, strlen(Buffer));
			SetTextColor(hDC, RGB(0, 255, 0));
		} else {
			sprintf(Buffer, "%.2f", DEG * Angle);
			TextOut(hDC, TAB, CurrentLine, Buffer, strlen(Buffer));
		} 
	} else {
		sprintf(Buffer, "+%.2f", DEG * Angle);
		TextOut(hDC, TAB, CurrentLine, Buffer, strlen(Buffer));
	}

	CurrentLine += LINE;
}

void AttitudeMFD::PrintRate(const char *Heading, double Rate)
{
	const int TAB = 100;
	char Buffer[25];

	TextOut(hDC, 5, CurrentLine, Heading, strlen(Heading));
	
	ScaleOutput(Buffer, Rate);

	// Adjust for negative number
	if (Rate < 0) {
		ScaleOutput(Buffer, Rate);

		if (ColorMode == COLOR_WHITE) {
			//sprintf(Buffer, "%.2f", DEG * Rate);
			SetTextColor(hDC, RGB(255, 255, 255));
			TextOut(hDC, TAB, CurrentLine, Buffer, strlen(Buffer));
			SetTextColor(hDC, RGB(0, 255, 0));
		} else if (ColorMode == COLOR_RED) {
			//sprintf(Buffer, "%.2f", DEG * Rate);
			SetTextColor(hDC, RGB(255, 0, 0));
			TextOut(hDC, TAB, CurrentLine, Buffer, strlen(Buffer));
			SetTextColor(hDC, RGB(0, 255, 0));
		} else {
			//sprintf(Buffer, "%.2f", DEG * Rate);
			TextOut(hDC, TAB, CurrentLine, Buffer, strlen(Buffer));
		} 
	} else {
		// Add + sign
		Buffer[0] = '+';
		ScaleOutput(&Buffer[1], Rate);
		TextOut(hDC, TAB, CurrentLine, Buffer, strlen(Buffer));
	}

	CurrentLine += LINE;
}



void AttitudeMFD::PrintRate(const char *Heading, double Rate, bool Trim)
{
	const int TAB = 100;
	char Buffer[25];

	// If this axis is being trimmed, the heading is in white
	if (Trim) {
		SetTextColor(hDC, RGB(255, 255, 255));
	}
	
	TextOut(hDC, 5, CurrentLine, Heading, strlen(Heading));
	
	// Reset the text color
	SetTextColor(hDC, RGB(0, 255, 0));
	
	ScaleOutput(Buffer, Rate);

	// Adjust for negative number
	if (Rate < 0) {
		ScaleOutput(Buffer, Rate);

		if (ColorMode == COLOR_WHITE) {
			//sprintf(Buffer, "%.2f", DEG * Rate);
			SetTextColor(hDC, RGB(255, 255, 255));
			TextOut(hDC, TAB, CurrentLine, Buffer, strlen(Buffer));
			SetTextColor(hDC, RGB(0, 255, 0));
		} else if (ColorMode == COLOR_RED) {
			//sprintf(Buffer, "%.2f", DEG * Rate);
			SetTextColor(hDC, RGB(255, 0, 0));
			TextOut(hDC, TAB, CurrentLine, Buffer, strlen(Buffer));
			SetTextColor(hDC, RGB(0, 255, 0));
		} else {
			//sprintf(Buffer, "%.2f", DEG * Rate);
			TextOut(hDC, TAB, CurrentLine, Buffer, strlen(Buffer));
		} 
	} else {
		// Add + sign
		Buffer[0] = '+';
		ScaleOutput(&Buffer[1], Rate);
		TextOut(hDC, TAB, CurrentLine, Buffer, strlen(Buffer));
	}

	CurrentLine += LINE;
}

// Note: This is not part of class AttitudeMFD
void PrintRect(HDC hDC, int StartX, int StartY, int Width, int Height)
{
	MoveToEx(hDC, StartX, StartY, NULL);

	LineTo(hDC, StartX + Width, StartY);
	LineTo(hDC, StartX + Width, StartY + Height);
	LineTo(hDC, StartX, StartY + Height);
	LineTo(hDC, StartX, StartY);
}

// Note: This is not part of class AttitudeMFD
void PrintArc(HDC hDC, int X, int Y, int Radius, int InnerRadius, double InitAngle, double SweepAngle)
{

// (typ.) warning C4244: 'argument' : conversion from 'xxx' to 'yyy', possible loss of data
	MoveToEx(hDC, X + (Radius * cos(Radians(InitAngle))), Y - (Radius * sin(Radians(InitAngle))), NULL);
	AngleArc(hDC, X, Y, Radius, InitAngle, SweepAngle);
	LineTo(hDC, X + (InnerRadius * cos(Radians(InitAngle + SweepAngle))), Y - (InnerRadius * sin(Radians(InitAngle + SweepAngle)))); 
	AngleArc(hDC, X, Y, InnerRadius, (InitAngle + SweepAngle), -SweepAngle);
	LineTo(hDC, X + (Radius * cos(Radians(InitAngle))), Y - (Radius * sin(Radians(InitAngle))));
}

void AttitudeMFD::PrintPitchThrustLevel(double Level)
{
	HBRUSH OldBrush; 

	const int DISPLAY_START_X = 25, DISPLAY_START_Y = CurrentLine, 
				DISPLAY_WIDTH = LINE, DISPLAY_HEIGHT = 90, 
				DISPLAY_MID_X = DISPLAY_START_X + (DISPLAY_WIDTH/2),
				DISPLAY_MID_Y = DISPLAY_START_Y + (DISPLAY_HEIGHT/2);


	// Print the overlaying display box
	PrintRect(hDC, DISPLAY_START_X, DISPLAY_START_Y, DISPLAY_WIDTH, DISPLAY_HEIGHT);

	// The color of the brush depends on if the thrust level is positive or negative
	if (Level >= 0) {
		OldBrush = (HBRUSH)SelectObject(hDC, ThrusterBrush);
	} else {
		OldBrush = (HBRUSH)SelectObject(hDC, ThrusterBrushNeg);		
	}
	
	BeginPath(hDC);
	PrintRect(hDC, DISPLAY_START_X, DISPLAY_MID_Y, DISPLAY_WIDTH, -((DISPLAY_HEIGHT/2) * Level)); 
	EndPath(hDC);
	StrokeAndFillPath(hDC);


	// Return to the original brush
	SelectObject(hDC, OldBrush);


}


void AttitudeMFD::PrintYawThrustLevel(double Level)
{
	HBRUSH OldBrush; 
	const int DISPLAY_START_X = 70, DISPLAY_START_Y = CurrentLine + (45 - (LINE / 2)), 
				DISPLAY_WIDTH = 90, DISPLAY_HEIGHT = LINE, 
				DISPLAY_MID_X = DISPLAY_START_X + (DISPLAY_WIDTH/2),
				DISPLAY_MID_Y = DISPLAY_START_Y + (DISPLAY_HEIGHT/2);


	// Print the overlaying display box
	PrintRect(hDC, DISPLAY_START_X, DISPLAY_START_Y, DISPLAY_WIDTH, DISPLAY_HEIGHT);

	// The color of the brush depends on if the thrust level is positive or negative
	if (Level >= 0) {
		OldBrush = (HBRUSH)SelectObject(hDC, ThrusterBrush);
	} else {
		OldBrush = (HBRUSH)SelectObject(hDC, ThrusterBrushNeg);		
	}
	
	BeginPath(hDC);
	PrintRect(hDC, DISPLAY_MID_X, DISPLAY_START_Y, ((DISPLAY_WIDTH/2) * Level), DISPLAY_HEIGHT); 
	EndPath(hDC);
	StrokeAndFillPath(hDC);


	// Return to the original brush
	SelectObject(hDC, OldBrush);


}

void AttitudeMFD::PrintRollThrustLevel(double Level)
{
	HBRUSH OldBrush; 

	const int DISPLAY_START_X = 225, DISPLAY_START_Y = CurrentLine + 70, 
				DISPLAY_WIDTH = LINE, DISPLAY_HEIGHT = 90, 
				DISPLAY_MID_X = DISPLAY_START_X + (DISPLAY_WIDTH/2),
				DISPLAY_MID_Y = DISPLAY_START_Y + (DISPLAY_HEIGHT/2);


	PrintArc(hDC, DISPLAY_START_X, DISPLAY_START_Y, 45, 45 - LINE, 179.0, -180.0);

	// The color of the brush depends on if the thrust level is positive or negative
	if (Level >= 0) {
		OldBrush = (HBRUSH)SelectObject(hDC, ThrusterBrush);
	} else {
		OldBrush = (HBRUSH)SelectObject(hDC, ThrusterBrushNeg);		
	}
	
	BeginPath(hDC);
	PrintArc(hDC, DISPLAY_START_X, DISPLAY_START_Y, 45, 45 - LINE, 90.0, -(90.0 * Level)); 
	EndPath(hDC);
	StrokeAndFillPath(hDC);


	// Return to the original brush
	SelectObject(hDC, OldBrush);


}

void AttitudeMFD::PrintRotThrust()
{
	char Buffer[100];
	int START_LINE = LINE * 17;
	
	if (CurrentLine < START_LINE) {
		CurrentLine = START_LINE;
	}

	MoveToEx(hDC, 0, CurrentLine, NULL);
	LineTo(hDC, Width, CurrentLine);
	MoveToEx(hDC, 60, CurrentLine, NULL);
	LineTo(hDC, 60, Height);
	MoveToEx(hDC, 170, CurrentLine, NULL);
	LineTo(hDC, 170, Height);

	CurrentLine += LINE / 4;

	SetTextColor(hDC, RGB(255, 255, 255));

	sprintf(Buffer, "%s", "Pitch");
	TextOut(hDC, 10, CurrentLine, Buffer, strlen(Buffer));
	sprintf(Buffer, "%s", "Yaw");
	TextOut(hDC, 105, CurrentLine, Buffer, strlen(Buffer));
	sprintf(Buffer, "%s", "Roll");
	TextOut(hDC, 205, CurrentLine, Buffer, strlen(Buffer));
	SetTextColor(hDC, RGB(0, 255, 0));

	CurrentLine += 1.5 * LINE;
	MoveToEx(hDC, 0, CurrentLine, NULL);
	LineTo(hDC, Width, CurrentLine);
	CurrentLine += LINE;

	PrintPitchThrustLevel(RotLevel.data[PITCH]);
	PrintYawThrustLevel(RotLevel.data[YAW]);
	PrintRollThrustLevel(RotLevel.data[ROLL]);

}

void AttitudeMFD::PrintRelVel()
{
	bool TrimVert = false,
			TrimLat = false,
			TrimFA = false;

	if (TrimStatus == T_ENGAGED) {
		if (TrimMode == T_VERT || 
			TrimMode == T_ALL || 
			TrimMode == T_VERT_LAT ||
			TrimMode == T_VERT_FA) {
			TrimVert = true;
		}

		 if (TrimMode == T_LAT ||
			 TrimMode == T_ALL ||
			 TrimMode == T_VERT_LAT ||
			 TrimMode == T_LAT_FA) {
			TrimLat = true;
		 }

		if (TrimMode == T_FA || 
			 TrimMode == T_ALL ||
			 TrimMode == T_LAT_FA ||
			TrimMode == T_VERT_FA) {
			TrimFA = true;
		}
	}

	PrintRate("Vertical", RelVel.y, TrimVert);
	PrintRate("Lateral", RelVel.x, TrimLat);
	PrintRate("Fore/Aft", RelVel.z, TrimFA);
}

bool inline AttitudeMFD::HaveMainEngine()
{
	return GetMaxForce(EN_MAIN) > 0;
}

bool inline AttitudeMFD::HaveRetroEngine()
{
	return GetMaxForce(EN_RETRO) > 0;
}

bool inline AttitudeMFD::HaveHoverEngine()
{
	return GetMaxForce(EN_HOVER) > 0;
}

bool cbSelectTarget(void *id, char *str, void *data)
{
	return (((AttitudeMFD *)data)->SelectTarget(str));
}

bool cbSetRelAttPitch(void *id, char *str, void *data)
{
	return (((AttitudeMFD *)data)->SetRelAttitude(str, PITCH));
}

bool cbSetRelAttYaw(void *id, char *str, void *data)
{
	return (((AttitudeMFD *)data)->SetRelAttitude(str, YAW));
}

bool cbSetRelAttRoll(void *id, char *str, void *data)
{
	return (((AttitudeMFD *)data)->SetRelAttitude(str, ROLL));
}

bool cbSetMode(void *id, char *str, void *data)
{
	REF_MODE Mode = (REF_MODE)atoi(str);

	if (Mode >= USER_ATT && Mode <= EI) {
		((AttitudeMFD *)data)->ChangeRefMode(Mode);
		return true;
	}

	return false;
}

bool AttitudeMFD::SetRelAttitude(char *str, AXIS Axis)
{
	// This allows a space to represent a decimal point
	for (int i = 0; i < (int)strlen(str); i++) {
		if (str[i] == ' ') {
			str[i] = '.';
			break;
		}
	}
	
	RelAttitude.data[Axis] = Radians(atof(str));

	return true;
}

