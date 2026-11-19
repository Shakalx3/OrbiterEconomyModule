#define STRICT
#define ORBITER_MODULE

#include "orbitersdk.h"

const DWORD COMM_SWINGARMS = 2401;

const VECTOR3 MESH_POS[5] = { {0,0,-16},{0,0,23.77},{0,0,47.67},{-1.75,0,68.346},{1.75,0,68.346} };
const VECTOR3 CG_SHIFT_S2 = { 0,0,-29 };
const VECTOR3 CG_SHIFT_S3 = { 0,0,-18.67 };
const double ION6_SIZE[3] = { 116,51,22 };
const double ION6_MASS_STG1 = 100516;
const double ION6_MASS_STG2 = 33747;
const double ION6_MASS_STG3 = 10705;
const double ION6_MASS_FAIRINGS = 5100;
const VECTOR3 ION6_PMI[3] = { {300,300,12},{150,150,8},{20,20,6} };
const VECTOR3 ION6_CS[3] = { {1031.9,1044.21,120.28},{432.19,438.31,83.32},{118.15,121.47,41.54} };
const double ION6_STG1_FUELMASS = 1222352.13;
const double ION6_STG2_FUELMASS = 518154.01;
const double ION6_STG3_FUELMASS = 141968.89;
const double ION6_APS_FUELMASS = 750; //Each APS pod has this much fuel
const double ION6_ULLAGE_FUELMASS = 1000;
const VECTOR3 RS68B_POS[7] = { {0,3.875,-38.22},{3.35584844,1.9375,-38.22},{3.35584844,-1.9375,-38.22},{0,-3.875,-38.22},{-3.35584844,-1.9375,-38.22},{-3.35584844,1.9375,-38.22},{0,0,-38.22} };
const double RS68B_THRUST = 3698173.8; //Uprated to 109%
const double RS68B_ISP = 420 * G;
const double RS68B_ISP_SL = 365 * G;
const double RS68B_EXHAUST_OFS = 4.404;
const double RS68B_REL = 10200;
const VECTOR3 J2S_POS[6] = { {0,2.625,8.95},{2.27331669,1.3125,8.95},{2.27331669,-1.3125,8.95},{0,-2.625,8.95},{-2.27331669,-1.3125,8.95},{-2.27331669,1.3125,8.95} };
const double J2S_THRUST = 1138500;
const double J2S_ISP = 436 * G;
const double J2S_EXHAUST_OFS = 2.7274;
const double J2S_REL = 10874;
const VECTOR3 J2X_POS = { 0,0,39.77 };
const double J2X_THRUST = 1310000;
const double J2X_ISP = 448 * G;
const double J2X_EXHAUST_OFS = 4.2141;
const double J2X_REL = 10874;
const VECTOR3 APS_JETS_POS[8] = { {0,4.325,42.045},{0,-4.325,42.045},{0.6875,3.825,42.045},{0.6875,-3.825,42.045},{-0.6875,3.825,42.045},{-0.6875,-3.825,42.045},{0,3.825,41.545},{0,-3.825,41.545} };
const VECTOR3 APS_JETS_DIR[8] = { {0,-1,0},{0,1,0},{-1,0,0},{-1,0,0},{1,0,0},{1,0,0},{0,0,1},{0,0,1} };
const double APS_JETS_THRUST = 5560;
const double APS_JETS_ISP = 320 * G;
const VECTOR3 ULLAGE_POS[4] = { {3.827215,3.827215,10.7},{3.827215,-3.827215,10.7},{-3.827215,-3.827215,10.7},{-3.827215,3.827215,10.7} };
const double ULLAGE_THRUST = 184609;
const double ULLAGE_ISP = 290 * G;
const VECTOR3 ION6_PAYLOAD_POS = {0,0,56.296};
const VECTOR3 ION6_PADMOUNT_POS = {0,0,-38.97};
const double MAX_TVC_ANGLE_STG1 = 5 * RAD;
const double MAX_TVC_ANGLE_STG2 = 3 * RAD;
const double MAX_TVC_ANGLE_STG3 = 3 * RAD;
const VECTOR3 STG1_CAM_POS = { {0,6.75,-5} };
const VECTOR3 STG2_CAM_POS = { {0,5.75,-2} };
const VECTOR3 STG3_CAM_POS = { {0,4.25,3} };

//
const double mu = 398600439968871.2;
const double rt = 6371010;
double min_inc = 0;
double tgt_inc = 0;
double tgt_inc_dogleg = 0;
bool dogleg = false;
double op_tgt = 200000;
double op_meco_alt = 180000;
double ap_tgt = 180000;
//

class Ion6 : public VESSEL4 {
public:
	Ion6(OBJHANDLE hVessel, int flightmodel);
	~Ion6();
	void clbkSetClassCaps(FILEHANDLE cfg);
	void clbkPostCreation();
	void clbkPreStep(double simt, double simdt, double mjd);
	void clbkPostStep(double simt, double simdt, double mjd);
	int clbkConsumeBufferedKey(DWORD key, bool down, char* kstate);
	void clbkLoadStateEx(FILEHANDLE scn, void* status);
	void clbkSaveState(FILEHANDLE scn);
	void CalcEmptyMass();
	void DefineAnimations();
	void DoStg1TVC(double pitch, double yaw, double roll);
	void DoStg2TVC(double pitch, double yaw, double roll);
	void DoStg3TVC(double pitch, double yaw);
	void ManualTVC();
	void Stage(int stage);
	void DelStage(int stage);
	void FairingJett();
	void DelFairing();
	OBJHANDLE SpawnObject(char* classsname, char* ext, VECTOR3 ofs, VECTOR3 spd, VECTOR3 rot);
	void DoEngineFailure(double simdt);

	void DoAscent(void);
	void DoOrbit(void);
	double CalcAzimuth();
	void CalcFlightVector();
	void GoToFlightVector();
	double FinalV();
	double GetOS();
	void DoLOT();
private:
	SURFHANDLE stg1_exhaust;
	SURFHANDLE stg23_exhaust;
	SURFHANDLE aps_exhaust;
	SURFHANDLE ullage_exhaust;
	MESHHANDLE stg1_mesh;
	MESHHANDLE stg2_mesh;
	MESHHANDLE stg3_mesh;
	MESHHANDLE fairing1_mesh;
	MESHHANDLE fairing2_mesh;
	MESHHANDLE custom_adapter_mesh;
	UINT mesh_idx[6]; //0-stg3, 1-stg2, 2-fair1, 3-fair2, 4-stg1, 5-custom_adapter
	ATTACHMENTHANDLE payload_attach;
	ATTACHMENTHANDLE pad_attach;
	PROPELLANT_HANDLE stg1_fuel;
	PROPELLANT_HANDLE stg2_fuel;
	PROPELLANT_HANDLE stg3_fuel;
	PROPELLANT_HANDLE ullage_fuel[4];
	PROPELLANT_HANDLE aps_1_fuel; //+Y APS Module
	PROPELLANT_HANDLE aps_2_fuel; //-Y APS Module
	THRUSTER_HANDLE rs_68b[7];
	THRUSTER_HANDLE j_2s[6];
	THRUSTER_HANDLE j_2x;
	THRUSTER_HANDLE aps_1_jets[6]; //+Y APS Module
	THRUSTER_HANDLE aps_2_jets[6]; //-Y APS Module
	THRUSTER_HANDLE ullage_motors[4];
	UINT anim_stg1_tvc_pitch;
	UINT anim_stg1_tvc_yaw;
	UINT anim_stg1_tvc_roll;
	UINT anim_stg2_tvc_pitch;
	UINT anim_stg2_tvc_yaw;
	UINT anim_stg2_tvc_roll;
	UINT anim_stg3_tvc_pitch;
	UINT anim_stg3_tvc_yaw;
	double payload_mass_override = 0;
	int vehicle_cfg = 0;
	int fairing_cfg = 0;
	double failure_mutliply = 1;
	bool swingarms_armed = false;
	int engine_failure = 0;
	char fairing_mesh_file1[256] = "Ion_LVs\\Ion6\\Fairing_1";
	char fairing_mesh_file2[256] = "Ion_LVs\\Ion6\\Fairing_2";
	bool has_custom_fairing = false;
	double fairing_pos = 0;
	double custom_fairings_mass = 5000;
	char adapter_mesh_file[256] = "";
	bool has_custom_adapter = false;
	double custom_adapter_mass = 1500;
	VECTOR3 custom_ofs = _V(0, 0, 0);

	int ap_status = 0;
	int op_status = 0;
	int apop_status = 0;
	int ap_function = 0;
	int tu_sw = 0;
	double timer = 0;
	int start_sw = 0;
	ELEMENTS el;
	ORBITPARAM prm;
	double azimuth = 0;
	VECTOR3 flight_vector = _V(0, 1, 0);
	VECTOR3 vazim1 = _V(0, 1, 0);
	VECTOR3 azim_local = _V(0, -1, 0);
	double pitch_angle = 90 * RAD;
	VECTOR3 vazim2 = _V(0, 1, 0);
	double vrot_lim = 5 * RAD;
	double vrot_lim_roll = 15 * RAD;
	int lot_status = 0;
	NOTEHANDLE hNote;
	int lot_hh = 0;
	int lot_mm = 0;
	int lot_ss = 0;
	double lot_timer = 0;
	int dlgl_status = 0;
};

Ion6::Ion6(OBJHANDLE hVessel, int flightmodel)
	: VESSEL4(hVessel, flightmodel)
{
}

Ion6::~Ion6()
{
}

DLLCLBK void InitModule(HINSTANCE hModule)
{

}

DLLCLBK void ExitModule(HINSTANCE hModule)
{

}

DLLCLBK VESSEL* ovcInit(OBJHANDLE hvessel, int flightmodel)
{
	return new Ion6(hvessel, flightmodel);
}

DLLCLBK void ovcExit(VESSEL* vessel)
{
	if (vessel) delete (Ion6*)vessel;
}

bool SetINC(void* id, char* str, void* data)
{
	float new_inc;
	float new_inc_dogleg = -99;
	sscanf(str, "%f %f", &new_inc, &new_inc_dogleg);
	if (new_inc_dogleg == -99) {
		dogleg = false;
	}
	else {
		dogleg = true;
	}

	if (dogleg == false) {
		if (abs(new_inc) < abs(min_inc * DEG))return false;
		else
		{
			tgt_inc = new_inc * RAD;
			tgt_inc_dogleg = tgt_inc;
			return true;
		}
	}
	else {
		if (abs(new_inc_dogleg) < abs(min_inc * DEG))return false;
		else
		{
			tgt_inc_dogleg = new_inc_dogleg * RAD;
			tgt_inc = new_inc * RAD;
			return true;
		}
	}
}

bool SetOPE(void* id, char* str, void* data)
{
	float new_ope;
	sscanf(str, "%f", &new_ope);
	if (new_ope < 160)return false;
	else
	{
		if (new_ope > op_tgt / 1000)
		{
			op_meco_alt = op_tgt;
			op_tgt = new_ope * 1000;
		}
		else op_meco_alt = new_ope * 1000;
		if (((op_meco_alt * 0.001) - 200) > 0)
		{
			ap_tgt = 1000 * ((5.05 * (sqrt((op_meco_alt * 0.001) - 200))) + 200);
		}
		else if (((op_meco_alt * 0.001) - 200) < 0)
		{
			ap_tgt = op_meco_alt;
		}
		else ap_tgt = 200000;

		if (ap_tgt > 450000)ap_tgt = 450000;
		if (ap_tgt < 100000)ap_tgt = 100000;
		return true;
	}
}

bool SetOAP(void* id, char* str, void* data)
{
	float new_oap;
	sscanf(str, "%f", &new_oap);
	if (new_oap < 160)return false;
	else
	{
		if (new_oap < op_meco_alt / 1000)
		{
			op_tgt = op_meco_alt;
			op_meco_alt = new_oap * 1000;
		}
		else op_tgt = new_oap * 1000;
		if (((op_meco_alt * 0.001) - 200) > 0)
		{
			ap_tgt = 1000 * ((5.05 * (sqrt((op_meco_alt * 0.001) - 200))) + 200);
		}
		else if (((op_meco_alt * 0.001) - 200) < 0)
		{
			ap_tgt = op_meco_alt;
		}
		else ap_tgt = 200000;

		if (ap_tgt > 450000)ap_tgt = 450000;
		if (ap_tgt < 100000)ap_tgt = 100000;
		return true;
	}
}