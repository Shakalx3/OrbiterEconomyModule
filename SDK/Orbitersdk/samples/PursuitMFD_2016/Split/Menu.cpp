// PursuitMFD Subfunctions
#define STRICT

#include "..\Main\PursuitMFD.h"

#if NDEBUG
	static bool release = true;
#endif
#if _DEBUG
	static bool release = false;
#endif

// =============================================================================
// generic functions
bool PursuitMFD::ConsumeKeyBuffered(DWORD key)
{

// inhibate some functions when holding or triming
	bool hld = (submfd->Hold || submfd->Trim!=T_OFF || submfd->GO);

	INPUT_MODE style = release ? USER:ORBI;

	switch(key) {

		case OAPI_KEY_TAB: // set remote MFD
			setCommand(style);
			break;


		case OAPI_KEY_H: // set Hold Mode
		case OAPI_KEY_SPACE:
			if(config) SetCurrentHoldOff(gMFDid);
			else submfd->SetHoldMode();
			break;


		case OAPI_KEY_DELETE: // set Trim All
			if(submfd->Mode==REL || submfd->Mode==DOC)
				if(submfd->Trim==T_ALL) submfd->SetTrimMode(T_OFF);
				else submfd->SetTrimMode(T_ALL);
			break;
		case OAPI_KEY_END: // set Trim Lateral
			if(submfd->Mode==REL || submfd->Mode==DOC)
				submfd->SetTrimMode(T_LAT);
			break;
		case OAPI_KEY_NEXT: // set Trim Vertical
			if(submfd->Mode==REL || submfd->Mode==DOC)
				submfd->SetTrimMode(T_VERT);
			break;
		case OAPI_KEY_PRIOR: // set Trim Fore/Aft
			if(submfd->Mode==REL || submfd->Mode==DOC)
				submfd->SetTrimMode(T_FA);
			break;
		case OAPI_KEY_INSERT: // set Trim OFF
			if(submfd->Mode==REL || submfd->Mode==DOC)
				submfd->SetTrimMode(T_OFF);
			break;

		case OAPI_KEY_G: // Docking Autopilot
			if(submfd->Mode==DOC)
				submfd->SetDockAP();
			else if(submfd->Mode==DCT){ // Active Burn
				submfd->GO = !submfd->GO;
			}
			else if(submfd->Mode==LAN) submfd->setGraph();
			break;
		case OAPI_KEY_X: // set distance for Docking Autopilot
			if(submfd->Mode==DOC)
				submfd->setDistanceVector(NULLVECTOR, 0x0);
			else if(submfd->Mode==DCT){ // get data from TransX
				if(hld) return false;
				if(!submfd->TX) submfd->BurnDate = oapiGetSimMJD()+(200.0/86400);
				submfd->TX =!submfd->TX;
			}
			break;

		case OAPI_KEY_P: // set Pitch angle
			if(config){ // set landing precision
				SwitchPrecision();
				break;
			}
			if(submfd->Mode==VEL || submfd->Mode==REL || submfd->Mode==ATT)//			if(submfd->Mode!=DOC && submfd->Mode!=LAN)
				submfd->SetRelAttitude(0.0, PITCH);
			else if(submfd->Mode==LAN)
				submfd->SetPad(-1);
			else if(submfd->Mode==DCT){ // set prograde vel
				if(hld) return false;
				submfd->SetBurnInputVelocity(0.0,0);
			}
			break;
		case OAPI_KEY_Y: // set Yaw angle
			if(submfd->Mode==VEL || submfd->Mode==REL || submfd->Mode==ATT)//			if(submfd->Mode!=DOC && submfd->Mode!=LAN)
				submfd->SetRelAttitude(0.0, YAW);
			else if(submfd->Mode==DCT){ // set plane change vel
				if(hld) return false;
				submfd->SetBurnInputVelocity(0.0,1);
			}
			break;
		case OAPI_KEY_R: // set Roll angle
			if(config){ // set rot or lin rates
				setRate(NULLVECTOR, 0x0);
				break;
			}
			if(submfd->Mode==VEL || submfd->Mode==REL || submfd->Mode==ATT)//			if(submfd->Mode!=DOC && submfd->Mode!=LAN)
				submfd->SetRelAttitude(0.0, ROLL);
			else if(submfd->Mode==LAN){ // burn refresh
				if(hld || submfd->LandPhase) return false; // hold or active burn
				submfd->refreshclosest = true;
			}
			else if(submfd->Mode==DCT){ // set outward vel
				if(hld) return false;
				submfd->SetBurnInputVelocity(0.0,2);
			}
			break;

		case OAPI_KEY_V: // switch between VEl or ATT in relative mode
			if(submfd->Mode==REL || submfd->Mode==DOC)
				submfd->AttVel = !submfd->AttVel;
			break;

		case OAPI_KEY_C: // circularize or orbit insertion
			submfd->circularize = !submfd->circularize;
			break;

		case OAPI_KEY_N: // switch to Dock IDS & HUD
			if(submfd->Mode==DOC)
				submfd->SetDockNav();
			if(submfd->Mode==LAN){ // landing now
				if(!release){
					if(hld || submfd->LandPhase) return false;
					submfd->LandPhase = INITBRK;
					submfd->SetHoldMode();
				}
			}
			break;

		case OAPI_KEY_I: // cheat code to set ISS attitude when docking
			if(submfd->Mode==ASC) submfd->SetTargetInclination(0);
			else submfd->SetISSControl(submfd->ISSCtrl);
			break;

		case OAPI_KEY_O:
			submfd->SetRollMode();
			submfd->waitsmooth();
			break;


//////////// configuration actions
		case OAPI_KEY_BACK: // config page
			config = !config;
			InvalidateButtons();
			break;

		case OAPI_KEY_UP:
			if(config) selection--;
			config = true;
			break;
		case OAPI_KEY_DOWN:
			if(config) selection++;
			config = true;
			break;
		case OAPI_KEY_LEFT:
			if(config) SetCurrentInactive(gMFDid);
			break;
		case OAPI_KEY_RIGHT:
			if(config){
				setMFDFocus(current);
				config = false;
			}
			break;
		case OAPI_KEY_A:
			if(config) submfd->setCutOffAlt(0.0);// setRotRate(NULLVECTOR, 0x0);
			else{
				if(submfd->Mode==LAN) submfd->applyBrake = !submfd->applyBrake;
				else if(submfd->Mode==ASC && !release) submfd->SetTargetAltitude(0);
			}
			break;
//		case OAPI_KEY_Q:
//			if(config) setLinRate(NULLVECTOR, 0x0);
//			break;
		case OAPI_KEY_S:
			if(config){
				SwapMFDdialog(-1);
				InvalidateButtons();
			}
			break;


//////////// actions to inhibate

		case OAPI_KEY_M: // Switch between Mode
		case OAPI_KEY_OEM_102:	// key next to LeftShift
		case OAPI_KEY_SLASH:	// key before RightShift
			if(!hld && !config) // submfd->SetNextMode();
				submfd->ModePage = !submfd->ModePage;
			else config = !config;
			InvalidateButtons();
			break;


		case OAPI_KEY_B: // set body reference or reference attitude
			if(hld) return false;
			if(submfd->Mode==ATT)
				submfd->SetRefAttitude();
			else if(submfd->Mode==LAN)
				submfd->setBase("");
//			else if(submfd->Mode==DCT)
//				submfd->SetBurnTime(0.0);
			else submfd->setRefName(style);
			break;
		case OAPI_KEY_T: // set target		--> input global ATT ??
			if(submfd->Mode==LAN){
				submfd->setManLoc(0.0,0.0);
				break;
			}
			if(hld) return false;
			if(submfd->Mode==VEL || submfd->Mode==REL || submfd->Mode==DOC)//			if(submfd->Mode!=ATT && submfd->Mode!=LAN)
				submfd->setTgtName(style);
			else if(submfd->Mode==DCT)
				submfd->SetTimetoBurn("");
			break;
		case OAPI_KEY_F: // follow target velocity
			if(hld) return false;
			if(submfd->Mode==VEL)
				submfd->TgtVel = !submfd->TgtVel;
			submfd->waitsmooth();
			break;

		case OAPI_KEY_D:
//			if(hld) return false;////////////////////////////////////////////
			if(submfd->Mode==DOC) // set Target Dock
				submfd->SelectDock(-1,true);
			if(submfd->Mode==LAN){ // invert rotation around engines axis
				submfd->RotaTh = !submfd->RotaTh;
				submfd->waitsmooth();
			}
			break;
		case OAPI_KEY_E: // set Engine for Landing
			if(hld) return false;
			submfd->LandPhase = WAIT;
			if(submfd->Mode==LAN)
				submfd->Hover = !submfd->Hover;
			submfd->waitsmooth();
			break;

		case OAPI_KEY_K: // RPM
			if(hld) return false;
			if(submfd->Mode==VEL)
				submfd->SetSubVelMode(RPM);
			break;
		case OAPI_KEY_L:
			if(submfd->Mode==LAN){
				if(submfd->LandPhase==FINAL) return false;
				submfd->setElevation(0.0);
			}
//			else if(hld) return false; // TOR or local dock choice///////////////////////////////////
			if(submfd->Mode==VEL) // TOR
				submfd->SetSubVelMode(TOR);
			else if(submfd->Mode==DOC) // set Dock in use
				submfd->SelectDock(-1,false);
			break;

		case OAPI_KEY_1: 
			if(submfd->ModePage && !submfd->Hold)
				submfd->SetMode(submfd->Mode, VEL);
			break;
		case OAPI_KEY_2:
			if(submfd->ModePage && !submfd->Hold)
				submfd->SetMode(submfd->Mode, REL);
			break;
		case OAPI_KEY_3:
			if(submfd->ModePage && !submfd->Hold)
			submfd->SetMode(submfd->Mode, ATT);
			break;
		case OAPI_KEY_4:
			if(submfd->ModePage && !submfd->Hold)
				submfd->SetMode(submfd->Mode, DOC);
			break;
		case OAPI_KEY_5:
			if(submfd->ModePage && !submfd->Hold)
				submfd->SetMode(submfd->Mode, LAN);
			break;
		case OAPI_KEY_6:
			if(!release){
				if(submfd->ModePage && !submfd->Hold)
					submfd->SetMode(submfd->Mode, DCT);
			}
			break;
		case OAPI_KEY_7:
			if(!release){
				if(submfd->ModePage && !submfd->Hold)
					submfd->SetMode(submfd->Mode, ASC);
			}
			break;

		case OAPI_KEY_RETURN: // enter pressed
			break;

	default:
		InvalidateButtons();
		return false;
	}

	InvalidateButtons();

	return true;
}

bool PursuitMFD::ConsumeButton (int bt, int event)
{

	// skip other events
	if (!(event & PANEL_MOUSE_LBDOWN) || (bt >= 12) || (bt == 99)) return false;

//== Page 1 ==
	static const DWORD VelKey[12] = // VELOCITY
	{
		OAPI_KEY_M,OAPI_KEY_B,OAPI_KEY_P,OAPI_KEY_Y,OAPI_KEY_R,OAPI_KEY_H,
		OAPI_KEY_O,OAPI_KEY_T,99,OAPI_KEY_F,OAPI_KEY_K,OAPI_KEY_L,
	};

//== Page 2 ==
	static const DWORD RelKey[12] = // RELATIVE
	{
		OAPI_KEY_M,OAPI_KEY_V,OAPI_KEY_P,OAPI_KEY_Y,OAPI_KEY_R,OAPI_KEY_H,
		OAPI_KEY_O,OAPI_KEY_T,OAPI_KEY_DELETE,OAPI_KEY_END,OAPI_KEY_NEXT,OAPI_KEY_PRIOR,
	};

//== Page 3 ==
	static const DWORD AttKey[12] = // USER_ATT
	{
		OAPI_KEY_M,OAPI_KEY_B,OAPI_KEY_P,OAPI_KEY_Y,OAPI_KEY_R,OAPI_KEY_H,
		OAPI_KEY_O,99,99,99,99,99,
	};

//== Page 4 ==
	static const DWORD DocKey[12] = // DOCKING
	{
		OAPI_KEY_M,OAPI_KEY_L,OAPI_KEY_D,OAPI_KEY_X,OAPI_KEY_G,OAPI_KEY_H,
		OAPI_KEY_O,OAPI_KEY_T,OAPI_KEY_DELETE,OAPI_KEY_END,OAPI_KEY_NEXT,OAPI_KEY_PRIOR,
	};

//== Page 5 ==
	static const DWORD LanKey[12] = // Landing
	{
		OAPI_KEY_M,OAPI_KEY_G,OAPI_KEY_E,OAPI_KEY_R,OAPI_KEY_A,OAPI_KEY_H,
		OAPI_KEY_TAB,OAPI_KEY_T,OAPI_KEY_B,OAPI_KEY_P,OAPI_KEY_L,OAPI_KEY_D,
	};

//== Page 6 ==
	static const DWORD DctKey[12] = // Direct Burn
	{
		OAPI_KEY_M,OAPI_KEY_P,OAPI_KEY_Y,OAPI_KEY_R,99,OAPI_KEY_H,
		OAPI_KEY_TAB,OAPI_KEY_T,OAPI_KEY_X,99,OAPI_KEY_C,OAPI_KEY_G,
	};

//== Page 7 ==
	static const DWORD AscKey[12] = // Ascent
	{
		OAPI_KEY_M,99,OAPI_KEY_A,99,99,OAPI_KEY_H,
		OAPI_KEY_TAB,99,OAPI_KEY_I,99,99,99,
	};

//== Page Summary ==
	static const DWORD ConKey[12] =
	{
		OAPI_KEY_M,OAPI_KEY_S,99,OAPI_KEY_UP,OAPI_KEY_DOWN,OAPI_KEY_RIGHT,
		OAPI_KEY_TAB,OAPI_KEY_R,OAPI_KEY_P,OAPI_KEY_A,OAPI_KEY_H,OAPI_KEY_LEFT,
	};

//== Mode Selection Page == 
	static const DWORD ModKey[12] =
	{
		OAPI_KEY_M,OAPI_KEY_1,OAPI_KEY_2,OAPI_KEY_3,99,OAPI_KEY_BACK,
		OAPI_KEY_TAB,99,OAPI_KEY_4,OAPI_KEY_5,OAPI_KEY_6,OAPI_KEY_7,
	};

	if(config) return ConsumeKeyBuffered(ConKey[bt]);

	if(submfd->ModePage) return ConsumeKeyBuffered(ModKey[bt]);

	switch(submfd->Mode){
		case VEL: return ConsumeKeyBuffered(VelKey[bt]);
		case REL: return ConsumeKeyBuffered(RelKey[bt]);
		case ATT: return ConsumeKeyBuffered(AttKey[bt]);
		case DOC: return ConsumeKeyBuffered(DocKey[bt]);
		case LAN: return ConsumeKeyBuffered(LanKey[bt]);
		case DCT: if(!release) return ConsumeKeyBuffered(DctKey[bt]);
		case ASC: if(!release) return ConsumeKeyBuffered(AscKey[bt]);
	}
	return false;

}

char *PursuitMFD::ButtonLabel (int bt)
{
//	static char *label[] = {"TGT","ALT","I +", "I -","Ad+","Ad-","MOD","DEF","HUD","SND","COR","AP"};
//	return (bt < 12 ? label[bt] : 0);

//== Page 1 == 
	static char *Vel[12] = // VELOCITY classic
	{
		"MOD","REF","PIT","YAW","ROL","HLD",
		"NOR","TGT","   ","FOL","RPM","TOR",
	};
	static char *VelHold[12] = // VELOCITY hold
	{
		"CFG","---","PIT","YAW","ROL","HLD",
		"NOR","---","   ","---","---","---",
	};
	static char *VelInhib[12] = // VELOCITY inhibate
	{
		"MOD","REF","PIT","YAW","ROL","---",
		"NOR","TGT","   ","FOL","---","---",
	};

//== Page 2 == 
	static char *Rel[12] = // RELATIVE classic
	{
		"MOD","V/A","PIT","YAW","ROL","HLD",
		"NOR","TGT","TRA","TRL","TRV","TFA",
	};
	static char *RelHold[12] = // RELATIVE hold
	{
		"CFG","V/A","PIT","YAW","ROL","HLD",
		"NOR","---","TRA","TRL","TRV","TFA",
	};
	static char *RelInhib[12] = // RELATIVE inhibate
	{
		"MOD","V/A","PIT","YAW","ROL","---",
		"NOR","TGT","---","---","---","---",
	};

//== Page 3 == 
	static char *Att[12] = // USER_ATT classic
	{
		"MOD","REF","PIT","YAW","ROL","HLD",
		"NOR","   ","   ","   ","   ","   ",
	};
	static char *AttHold[12] = // USER_ATT hold
	{
		"CFG","---","PIT","YAW","ROL","HLD",
		"NOR","   ","   ","   ","   ","   ",
	};
	static char *AttInhib[12] = // USER_ATT inhibate
	{
		"MOD","REF","PIT","YAW","ROL","---",
		"NOR","   ","   ","   ","   ","   ",
	};

//== Page 4 == 
	static char *Doc[12] = // DOCKING classic
	{
		"MOD","LDK","TDK","DIS","APD","HLD",
		"NOR","TGT","TRA","TRL","TRV","TFA",
	};
	static char *DocHold[12] = // DOCKING hold
	{
		"CFG","---","---","DIS","APD","HLD",
		"NOR","---","TRA","TRL","TRV","TFA",
	};
	static char *DocInhib[12] = // DOCKING inhibate
	{
		"MOD","LDK","TDK","DIS","---","---",
		"NOR","TGT","---","---","---","---",
	};

//== Page 5 == 
	static char *Lan[12] = // Landing classic
	{
		"MOD","GPH","ENG","RFH","BRK","HLD",
		"REM","MAN","TGT","PAD","ELV","INV",
	};
	static char *LanHold[12] = // Landing hold
	{
		"CFG","GPH","---","---","BRK","HLD",
		"REM","MAN","---","PAD","ELV","INV",
	};
	static char *LanInhib[12] = // Landing inhibate
	{
		"MOD","GPH","ENG","RFH","BRK","---",
		"REM","MAN","TGT","PAD","ELV","INV",
	};
	static char *LanActive[12] = // Landing active
	{
		"MOD","GPH","ENG","---","BRK","HLD",
		"REM","MAN","TGT","PAD","---","INV",
	};

	static char *Lan_release[12] = // Landing classic
	{
		"MOD","GPH","ENG","RFH","BRK","HLD",
		"REM","MAN","TGT","PAD","ELV","INV",
	};
	static char *LanHold_release[12] = // Landing hold
	{
		"CFG","GPH","---","   ","BRK","HLD",
		"REM","MAN","---","PAD","ELV","INV",
	};
	static char *LanInhib_release[12] = // Landing inhibate
	{
		"MOD","GPH","ENG","RFH","BRK","---",
		"REM","MAN","TGT","PAD","ELV","INV",
	};
	static char *LanActive_release[12] = // Landing active
	{
		"MOD","GPH","ENG","   ","BRK","HLD",
		"REM","MAN","TGT","PAD","---","INV",
	};

//== Page 6 == 
	static char *Dct[12] = // Direct Burn classic
	{
		"MOD","dVf","dVp","dVi","   ","HLD",
		"REM","Tej","GTx","   ","CIR","GO",
	};
	static char *DctHold[12] = // Direct Burn hold
	{
		"CFG","---","---","---","   ","HLD",
		"REM","---","---","   ","---","GO",
	};
	static char *DctInhib[12] = // Direct Burn inhibate
	{
		"MOD","dVf","dVp","dVi","   ","---",
		"REM","Tej","GTx","   ","CIR","---",
	};
	static char *DctCirc[12] = // Direct Burn classic
	{
		"MOD","---","dVp","---","   ","HLD",
		"REM","Tej","GTx","   ","CIR","GO",
	};

//== Page 7 == 
	static char *Asc[12] = // Ascent classic
	{
		"MOD","   ","Alt","   ","   ","HLD",
		"REM","   ","Inc","   ","   ","   ",
	};
	static char *AscHold[12] = // Ascent hold
	{
		"CFG","   ","---","   ","   ","HLD",
		"REM","   ","---","   ","   ","   ",
	};
	static char *AscInhib[12] = // Ascent inhibate
	{
		"MOD","   ","Alt","   ","   ","---",
		"REM","   ","Inc","   ","   ","   ",
	};

//== Page Summary == 
	static char *Config[12] =
	{
		"BCK","SWP","   ","PRV","NXT","SET",
		"REM","RAT","PCN","ALT","STP","DEL",
	};

//== Mode Selection Page == 
	static char *Mode[12] =
	{
		"BCK","VEL","REL","ATT","   ","CFG",
		"REM","   ","DOC","LAN","DCT","ASC",
	};
	static char *Mode_release[12] =
	{
		"BCK","VEL","REL","ATT","   ","CFG",
		"REM","   ","DOC","LAN","   ","   ",
	};

	if(config) return (bt < 12 ? Config[bt] : 0);

	if(submfd->ModePage)
		if(release) return (bt < 12 ? Mode_release[bt] : 0);
		else return (bt < 12 ? Mode[bt] : 0);

	bool hld = (submfd->Hold || submfd->Trim!=T_OFF || submfd->GO);

	switch(submfd->Mode){
		case VEL:
			if(hld)
				return (bt < 12 ? VelHold[bt] : 0);
			if(submfd->inhibate)
				return (bt < 12 ? VelInhib[bt] : 0);
			return (bt < 12 ? Vel[bt] : 0);
		case REL:
			if(hld)
				return (bt < 12 ? RelHold[bt] : 0);
			if(submfd->inhibate)
				return (bt < 12 ? RelInhib[bt] : 0);
			return (bt < 12 ? Rel[bt] : 0);
		case ATT:
			if(hld)
				return (bt < 12 ? AttHold[bt] : 0);
			if(submfd->inhibate)
				return (bt < 12 ? AttInhib[bt] : 0);
			return (bt < 12 ? Att[bt] : 0);
		case DOC:
			if(hld)
				return (bt < 12 ? DocHold[bt] : 0);
			if(submfd->inhibate)
				return (bt < 12 ? DocInhib[bt] : 0);
			return (bt < 12 ? Doc[bt] : 0);
		case LAN:
			if(release){
				if(hld)
					return (bt < 12 ? LanHold_release[bt] : 0);
				if(submfd->inhibate)
					return (bt < 12 ? LanInhib_release[bt] : 0);
				if(submfd->LandPhase)
					return (bt < 12 ? LanActive_release[bt] : 0);
				return (bt < 12 ? Lan_release[bt] : 0);
			}
			else{
				if(hld)
					return (bt < 12 ? LanHold[bt] : 0);
				if(submfd->inhibate)
					return (bt < 12 ? LanInhib[bt] : 0);
				if(submfd->LandPhase)
					return (bt < 12 ? LanActive[bt] : 0);
				return (bt < 12 ? Lan[bt] : 0);
			}
		case DCT:
			if(!release){
				if(hld)
					return (bt < 12 ? DctHold[bt] : 0);
				if(submfd->inhibate)
					return (bt < 12 ? DctInhib[bt] : 0);
				if(submfd->circularize)
					return (bt < 12 ? DctCirc[bt] : 0);
				return (bt < 12 ? Dct[bt] : 0);
			}
		case ASC:
			if(!release){
				if(hld)
					return (bt < 12 ? AscHold[bt] : 0);
				if(submfd->inhibate)
					return (bt < 12 ? AscInhib[bt] : 0);
				return (bt < 12 ? Asc[bt] : 0);
			}
	}
	return (bt < 12 ? Config[bt] : 0);
}

int PursuitMFD::ButtonMenu (const MFDBUTTONMENU **menu) const
{
//== Page 1 == VELOCITY
	static const MFDBUTTONMENU Velmnu[12] = {
		{"Mode/Summary",0,'M'},{"Select Reference",0,'B'},{"Set Pitch Angle",0,'P'},
		{"Set Yaw Angle",0,'Y'},{"Set Roll Angle",0,'R'},{"Hold Mode","Spacebar",'H'},
		{"Roll Mode",0,'O'},{"Select Target",0,'T'},{"   ",0,'\0'},
		{"Follow Vessel",0,'F'},{"Rdzv Pitch Manvr",0,'K'},{"Twice Orb Rate",0,'L'},
	};

//== Page 2 == RELATIVE
	static const MFDBUTTONMENU Relmnu[12] = {
		{"Mode/Summary",0,'M'},{"Vel/Att Mode",0,'V'},{"Set Pitch Angle",0,'P'},
		{"Set Yaw Angle",0,'Y'},{"Set Roll Angle",0,'R'},{"Hold Mode","Spacebar",'H'},
		{"Roll Mode",0,'O'},{"Select Target",0,'T'},{"Trim All Axis","Delete",'\0'},
		{"Trim Lateral","End",'\0'},{"Trim Vertical","PgDown",'\0'},{"Trim Fore/Aft","PgUp",'\0'},
	};

//== Page 3 == DOCKING
	static const MFDBUTTONMENU Docmnu[12] = {
		{"Mode/Summary",0,'M'},{"Set Local Dock",0,'L'},{"Set Target Dock",0,'D'},
		{"Set Distance","Vector XYZ",'X'},{"Docking Auto","keep Alignement",'G'},{"Hold Mode","Spacebar",'H'},
		{"Roll Mode",0,'O'},{"Select Target",0,'T'},{"Trim All Axis","Delete",'\0'},
		{"Trim Lateral","End",'\0'},{"Trim Vertical","PgDown",'\0'},{"Trim Fore/Aft","PgUp",'\0'},
	};

//== Page 4 == USER_ATT
	static const MFDBUTTONMENU Attmnu[12] = {
		{"Mode/Summary",0,'M'},{"Set Global Ref",0,'B'},{"Set Pitch Angle",0,'P'},
		{"Set Yaw Angle",0,'Y'},{"Set Roll Angle",0,'R'},{"Hold Mode","Spacebar",'H'},
		{"Roll Mode",0,'O'},{"   ",0,'\0'},{"   ",0,'\0'},
		{"   ",0,'\0'},{"   ",0,'\0'},{"   ",0,'\0'},
	};

//== Page 5 == Landing
	static const MFDBUTTONMENU Lanmnu[12] = {
		{"Mode/Summary",0,'M'},{"Graph mode",0,'G'},{"Engine choice",0,'E'},
		{"Calc refresh",0,'R'},{"Set Brakes",0,'A'},{"Hold Mode","Spacebar",'H'},
		{"Remote","Shift + Tab",'\0'},{"Set Manual Location",0,'T'},{"Select Target",0,'B'},
		{"Pad Number",0,'P'},{"Set new elevation",0,'L'},{"Invert Rot Dir",0,'D'},
	};
	static const MFDBUTTONMENU Lanmnu_release[12] = {
		{"Mode/Summary",0,'M'},{"Graph mode",0,'G'},{"Engine choice",0,'E'},
		{"Calc refresh",0,'R'},{"Set Brakes",0,'A'},{"Hold Mode","Spacebar",'H'},
		{"Remote","Shift + Tab",'\0'},{"Set Manual Location",0,'T'},{"Select Target",0,'B'},
		{"Pad Number",0,'P'},{"Set new elevation",0,'L'},{"Invert Rot Dir",0,'D'},
	};

//== Page 6 == Direct Burn
	static const MFDBUTTONMENU Dctmnu[12] = {
		{"Mode/Summary",0,'M'},{"Set Prograde Vel",0,'P'},{"Set plane Change Vel",0,'Y'},
		{"Set Outward Vel",0,'R'},{"   ",0,'\0'},{"Hold Mode","Spacebar",'H'},
		{"Remote","Shift + Tab",'\0'},{"Time of Burn",0,'T'},{"Get TransX data",0,'X'},
		{"   ",0,'\0'},{"Circularize/Orbit insert",0,'C'},{"Active Burn",0,'G'},
	};

//== Page 7 == Ascent
	static const MFDBUTTONMENU Ascmnu[12] = {
		{"Mode/Summary",0,'M'},{"   ",0,'\0'},{"Set Altitude",0,'A'},
		{"   ",0,'\0'},{"   ",0,'\0'},{"Hold Mode","Spacebar",'H'},
		{"Remote","Shift + Tab",'\0'},{"   ",0,'\0'},{"Set Inclination",0,'I'},
		{"   ",0,'\0'},{"   ",0,'\0'},{"   ",0,'\0'},
	};

//== Page Summary ==
	static const MFDBUTTONMENU Conmnu[12] = {
		{"Go Back","Shift + Back",'M'},{"Swap MFD",0,'S'},{"   ",0,'\0'},
		{"Previous","Shift + Up",'\0'},{"Next","Shift + Down",'\0'},{"Select","Shift + Right",'\0'},
		{"Remote","Shift + Tab",'\0'},{"Change Rate",0,'R'},{"Landing Precision",0,'P'},
		{"CutOff Altitude",0,'A'},{"Stop Hold & Trim",0,'H'},{"Release","Shift + Left",'\0'},
	};

//== Mode Selection Page == 
	static const MFDBUTTONMENU Modmnu[12] = {
		{"Go Back",0,'M'},{"Velocity",0,'1'},{"Relative",0,'2'},
		{"Absolute",0,'3'},{"   ",0,'\0'},{"Summary","Shift + Back",'\0'},
		{"Remote","Shift + Tab",'\0'},{"   ",0,'\0'},{"Docking",0,'4'},
		{"Landing",0,'5'},{"Direct Burn","beta",'6'},{"Ascent","beta",'7'},
	};
	static const MFDBUTTONMENU Modmnu_release[12] = {
		{"Go Back",0,'M'},{"Velocity",0,'1'},{"Relative",0,'2'},
		{"Absolute",0,'3'},{"   ",0,'\0'},{"Summary","Shift + Back",'\0'},
		{"Remote","Shift + Tab",'\0'},{"   ",0,'\0'},{"Docking",0,'4'},
		{"Landing",0,'5'},{"   ",0,'\0'},{"   ",0,'\0'},
	};

	if (menu){
		if(config) {*menu = Conmnu; return 12;}

		if(submfd->ModePage)
			if(release){*menu = Modmnu_release; return 12;}
			else{*menu = Modmnu; return 12;}

		switch(submfd->Mode){
			case VEL: *menu = Velmnu; return 12;
			case REL: *menu = Relmnu; return 12;
			case ATT: *menu = Attmnu; return 12;
			case DOC: *menu = Docmnu; return 12;
			case LAN: if(release){*menu = Lanmnu_release; return 12;}
					  else{*menu = Lanmnu; return 12;}
			case DCT: if(!release) *menu = Dctmnu; return 12;
			case ASC: if(!release) *menu = Ascmnu; return 12;
		}
	}
	return 12;
}

int PursuitMFD::MsgProc (UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
	case OAPI_MSG_MFD_OPENED:
		return (int)(new PursuitMFD (LOWORD(wparam), HIWORD(wparam), (VESSEL*)lparam, mfd));
	}
	return 0;
}

// END OF FILE