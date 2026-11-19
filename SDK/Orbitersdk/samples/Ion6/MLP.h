#define STRICT
#define ORBITER_MODULE

#include "orbitersdk.h"

const DWORD COMM_SWINGARMS = 2401;

const VECTOR3 CAMERA_POS = { 0,127.1,-6.5 };
//const VECTOR3 CAMERA_DIR = { 0,-0.928476690885259,0.371390676354104 };
const VECTOR3 CAMERA_DIR = { 0,0,1 };
const double MLP_SIZE = 134;
const VECTOR3 LV_POS = { 0,4.375,9.15 };

const double AUTO_S1_LH2LOAD = 7346; // 7,500 US Gallons per min
const double AUTO_S1_LOXLOAD = 4911; // 3,000 US Gallons per min
const double AUTO_S2_LH2LOAD = 4893; // 5,000 US Gallons per min
const double AUTO_S2_LOXLOAD = 2931; // 2,000 US Gallons per min
const double AUTO_S3_LH2LOAD = 2441; // 3,200 US Gallons per min
const double AUTO_S3_LOXLOAD = 1235; // 1,300 US Gallons per min
const double AUTO_S123_LH2TOP = 600;
const double AUTO_S123_LOXTOP = 300;
const double AUTO_TOPPING_CLOSEOUT = 45;
const double AUTO_SWINGARMS = 5;

const double S1_LH2MASS = 226613.33;
const double S1_LOXMASS = 995738.8;
const double S2_LH2MASS = 96135.63;
const double S2_LOXMASS = 422018.38;
const double S3_LH2MASS = 26387.16;
const double S3_LOXMASS = 115581.73;

class MLP : public VESSEL4 {
public:
	MLP(OBJHANDLE hVessel, int flightmodel);
	~MLP();
	void clbkSetClassCaps(FILEHANDLE cfg);
	void clbkPostCreation();
	void clbkPreStep(double simt, double simdt, double mjd);
	void clbkPostStep(double simt, double simdt, double mjd);
	int clbkConsumeBufferedKey(DWORD key, bool down, char* kstate);
	void clbkLoadStateEx(FILEHANDLE scn, void* status);
	void clbkSaveState(FILEHANDLE scn);
	bool clbkLoadVC(int id);
	bool clbkLoadGenericCockpit();
	void GetThrust();
	void AutosequenceTimer();
	void SetAutoState();
	void DoS1PropLoad(double simdt);
	void DoS2PropLoad(double simdt);
	void DoS3PropLoad(double simdt);
	void DoPostLiftoffEffects(double simdt);
	void DefineAnimations();
	enum DoorStatus { CLOSED, OPEN, CLOSING, OPENING } swingarms_status;
	void ActivateSwingArms(DoorStatus action);
	void RevertSwingArms(void);
	void MoveSwingArms(double simdt);
	void DoAutoFloodlights();
	double GetSunAngle();
private:
	MESHHANDLE mlp_mesh;
	ATTACHMENTHANDLE lv_attach;
	double lv_thlvl[2] = { 0,0 };
	double lv_thlvl_factor = 0.25;
	UINT anim_swingarms;
	double swingarms_proc;
	int autosequence = 0;
	double autosequence_timer = 0;
	bool timeaccel_rset_armed = false;
	bool auto_tanks_full[3] = { 0,0,0 };
	double loxvent_lvl[3] = { 0,0,0 };
	double post_liftoff_flamebucket = 0;
	bool post_liftoff = false;
	int lot_hh = 0;
	int lot_mm = 0;
	int lot_ss = 0;
	PARTICLESTREAMSPEC loxvent_s1 = {
		0, 1, 20, 1, 0.5, 2, 1, 1, PARTICLESTREAMSPEC::DIFFUSE,
		PARTICLESTREAMSPEC::LVL_LIN, 0, 1,
		PARTICLESTREAMSPEC::ATM_FLAT, 1, 1
	};
	PARTICLESTREAMSPEC loxvent_s2 = {
		0, 1, 20, 1, 0.5, 2, 1, 1, PARTICLESTREAMSPEC::DIFFUSE,
		PARTICLESTREAMSPEC::LVL_LIN, 0, 1,
		PARTICLESTREAMSPEC::ATM_FLAT, 1, 1
	};
	PARTICLESTREAMSPEC loxvent_s3 = {
		0, 1, 20, 1, 0.5, 2, 1, 1, PARTICLESTREAMSPEC::DIFFUSE,
		PARTICLESTREAMSPEC::LVL_LIN, 0, 1,
		PARTICLESTREAMSPEC::ATM_FLAT, 1, 1
	};
	PARTICLESTREAMSPEC flamebucket = {
		0, 15, 500, 500, 0.25, 2, 30, 2, PARTICLESTREAMSPEC::DIFFUSE,
		PARTICLESTREAMSPEC::LVL_LIN, 0, 1,
		PARTICLESTREAMSPEC::ATM_FLAT, 1, 1
	};
	VECTOR3 lights_pos[5] = { {-41,9,58},{24,9,73},{30,9,-54},{-41,9,-44},{-43,5,7} };
	VECTOR3 lights_dir[5] = {
		{0.454519477672044,0.707106781186547,-0.541675220419702},
		{-0.293557838477687,0.573576436351046,-0.764744314873372},
		{-0.25,0.866025403784439,0.433012701892219},
		{0.340999180031249,0.866025403784439,0.365676850809585},
		{0.754406506735489,0.642787609686539,-0.133022221559489}
	};
	VECTOR3 lights_dir_inv[5] = {
		-lights_dir[0],
		-lights_dir[1],
		-lights_dir[2],
		-lights_dir[3],
		-lights_dir[4]
	};
	SURFHANDLE floodlight_tex;
	EXHAUSTSPEC floodlight_beam[5];
	double floodlight_status = 0;
	SpotLight* floodlight[5];
	double on_off_sunangle = 2;
};

MLP::MLP(OBJHANDLE hVessel, int flightmodel)
	: VESSEL4(hVessel, flightmodel)
{
}

MLP::~MLP()
{
}

DLLCLBK VESSEL* ovcInit(OBJHANDLE hvessel, int flightmodel)
{
	return new MLP(hvessel, flightmodel);
}

DLLCLBK void ovcExit(VESSEL* vessel)
{
	if (vessel) delete (MLP*)vessel;
}