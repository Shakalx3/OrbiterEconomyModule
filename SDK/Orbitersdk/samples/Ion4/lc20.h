#define STRICT
#define ORBITER_MODULE

#include "orbitersdk.h"

const VECTOR3 CAMERA_POS = { -225,13,27.25 };
const VECTOR3 CAMERA_DIR = { 0.986393923832144,0.164398987305357,0 };
const double LC20_SIZE = 200;
const VECTOR3 LV_POS = { 0,11.5,16 };

const double AUTO_S1_LH2LOAD = 2700;
const double AUTO_S1_LOXLOAD = 1500;
const double AUTO_S2_LH2LOAD = 1170;
const double AUTO_S2_LOXLOAD = 705;
const double AUTO_S1S2_LH2TOP = 600;
const double AUTO_S1S2_LOXTOP = 300;
const double AUTO_TOPPING_CLOSEOUT = 45;
const double AUTO_LST_DOWN = 30;

const double S1_LH2MASS = 26036.15;
const double S1_LOXMASS = 114392.26;
const double S2_LH2MASS = 7060.34;
const double S2_LOXMASS = 31020.27;

class lc20 : public VESSEL4 {
public:
	lc20(OBJHANDLE hVessel, int flightmodel);
	~lc20();
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
	void DoPostLiftoffEffects(double simdt);
	void DefineAnimations();
	enum DoorStatus { CLOSED, OPEN, CLOSING, OPENING } lst_status;
	void ActivateLST(DoorStatus action);
	void RevertLST(void);
	void MoveLST(double simdt);
private:
	MESHHANDLE lc20_mesh;
	ATTACHMENTHANDLE lv_attach;
	double lv_thlvl = 0;
	UINT anim_lst;
	double lst_proc;
	int autosequence = 0;
	double autosequence_timer = 0;
	bool auto_tanks_full[2] = { 0,0 };
	double loxvent_lvl[2] = { 0,0 };
	double post_liftoff_flamebucket = 0;
	bool post_liftoff = false;
	int lot_hh = 0;
	int lot_mm = 0;
	int lot_ss = 0;
	PARTICLESTREAMSPEC loxvent_s1 = {
		0, 3, 5, 5, 0.5, 2, 0.5, 1, PARTICLESTREAMSPEC::DIFFUSE,
		PARTICLESTREAMSPEC::LVL_LIN, 0, 1,
		PARTICLESTREAMSPEC::ATM_FLAT, 1, 1
	};
	PARTICLESTREAMSPEC loxvent_s2 = {
		0, 2, 5, 5, 0.5, 2, 0.5, 1, PARTICLESTREAMSPEC::DIFFUSE,
		PARTICLESTREAMSPEC::LVL_LIN, 0, 1,
		PARTICLESTREAMSPEC::ATM_FLAT, 1, 1
	};
	PARTICLESTREAMSPEC flamebucket = {
		0, 10, 200, 400, 0.25, 2, 30, 2, PARTICLESTREAMSPEC::DIFFUSE,
		PARTICLESTREAMSPEC::LVL_LIN, 0, 1,
		PARTICLESTREAMSPEC::ATM_FLAT, 1, 1
	};
};

lc20::lc20(OBJHANDLE hVessel, int flightmodel)
	: VESSEL4(hVessel, flightmodel)
{
}

lc20::~lc20()
{
}

DLLCLBK VESSEL* ovcInit(OBJHANDLE hvessel, int flightmodel)
{
	return new lc20(hvessel, flightmodel);
}

DLLCLBK void ovcExit(VESSEL* vessel)
{
	if (vessel) delete (lc20*)vessel;
}