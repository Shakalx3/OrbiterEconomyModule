#define STRICT
#define ORBITER_MODULE

#include "orbitersdk.h"

//fairings 1500kg
//int panels 800kg

const VECTOR3 MESH_POS[7] = { {0,0,0},{0,0,29.94},{-1.055,0,17.515},{1.055,0,17.515},{-1.25,0,42.658},{1.25,0,42.658},{0,0,29.94} };
const VECTOR3 CG_SHIFT_S2 = { 0,0,-29 };
const double ION4_SIZE[2] = { 40,12 };
const double ION4_MASS_STG1 = 24500;
const double ION4_MASS_STG2 = 4400;
const double ION4_MASS_FAIRINGS = 1500;
const VECTOR3 ION4_PMI[2] = { {140,140,2},{15,15,1.5} };
const VECTOR3 ION4_CS[2] = { {309.81,306.92,19.66},{65.08,63.85,11.35} };
const double ION4_STG1_FUELMASS = 140428.41;
const double ION4_STG1_ATT_FUELMASS = 500;
const double ION4_STG2_FUELMASS = 38080.61;
const double ION4_STG2_ATT_FUELMASS = 100;
const VECTOR3 RS68B_POS = { 0,0,-25.17 };
const double RS68B_THRUST = 3392820;
const double RS68B_ISP = 420 * G;
const double RS68B_ISP_SL = 365 * G;
const double RS68B_EXHAUST_OFS = 4.404;
const double RS68B_REL = 10200;
const VECTOR3 J2X_POS = { 0,0,14.921 };
const double J2X_THRUST = 1310000;
const double J2X_ISP = 448 * G;
const double J2X_REL = 10874;
const VECTOR3 STG1_ATT_JETS_POS[4] = { {-1.45,1.2,-23.17},{1.45,1.2,-23.17},{1.45,-1.2,-23.17},{-1.45,-1.2,-23.17} };
const VECTOR3 STG1_ATT_JETS_DIR[4] = { {0,-1,0},{0,-1,0},{0,1,0},{0,1,0} };
const double STG1_ATT_JETS_THRUST = 15569;
const double STG1_ATT_JETS_ISP = 300 * G;
const VECTOR3 STG2_ATT_JETS_POS[8] = { {-2.005,0,20.74},{2.005,0,20.74},{-2.005,0.4,21.015},{2.005,0.4,21.015},{2.005,-0.4,21.015},{-2.005,-0.4,21.015},{-2.152,0,21.015},{2.152,0,21.015} };
const VECTOR3 STG2_ATT_JETS_DIR[8] = { {0,0,1},{0,0,1},{0,-1,0},{0,-1,0},{0,1,0},{0,1,0},{1,0,0},{-1,0,0} };
const double STG2_ATT_JETS_THRUST = 1557;
const double STG2_ATT_JETS_ISP = 320 * G;
const VECTOR3 ION4_PAYLOAD_POS = {0,0,36.32};
const VECTOR3 ION4_PADMOUNT_POS = {0,0,-25.67};
const double MAX_TVC_ANGLE = 3 * RAD;
const VECTOR3 STG1_CAM_POS = { {0,2.5,13.5} };
const VECTOR3 STG2_CAM_POS = { {0,2.25,0.5} };

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

class Ion4 : public VESSEL4 {
public:
	Ion4(OBJHANDLE hVessel, int flightmodel);
	~Ion4();
	void clbkSetClassCaps(FILEHANDLE cfg);
	void clbkPostCreation();
	void clbkPreStep(double simt, double simdt, double mjd);
	void clbkPostStep(double simt, double simdt, double mjd);
	int clbkConsumeBufferedKey(DWORD key, bool down, char* kstate);
	void clbkLoadStateEx(FILEHANDLE scn, void* status);
	void clbkSaveState(FILEHANDLE scn);
	void CalcEmptyMass();
	void DefineAnimations();
	void DoTVC(double pitch, double yaw);
	void ManualTVC();
	void Stage();
	void DelFirstStage();
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
	void RollToAz();
	void DoLOT();
private:
	SURFHANDLE stg1_exhaust;
	SURFHANDLE stg2_exhaust;
	SURFHANDLE thruster_exhaust;
	MESHHANDLE stg1_mesh;
	MESHHANDLE stg2_mesh;
	MESHHANDLE inter1_mesh;
	MESHHANDLE inter2_mesh;
	MESHHANDLE fairing1_mesh;
	MESHHANDLE fairing2_mesh;
	MESHHANDLE adapter_mesh;
	MESHHANDLE custom_adapter_mesh;
	UINT mesh_idx[8]; //0-stg2, 1-adapter, 2-fair1, 3-fair2, 4-stg1, 5-int1, 6-int2, 7-custom_adapter
	ATTACHMENTHANDLE payload_attach;
	ATTACHMENTHANDLE pad_attach;
	PROPELLANT_HANDLE stg1_fuel;
	PROPELLANT_HANDLE stg1_att_fuel;
	PROPELLANT_HANDLE stg2_fuel;
	PROPELLANT_HANDLE stg2_att_fuel;
	THRUSTER_HANDLE rs_68b;
	THRUSTER_HANDLE j_2x;
	THRUSTER_HANDLE stg1_att_jets[4];
	THRUSTER_HANDLE stg2_att_jets[8];
	UINT anim_tvc_pitch;
	UINT anim_tvc_yaw;
	double payload_mass_override = 0;
	int vehicle_cfg = 0;
	int fairing_cfg = 0;
	double failure_mutliply = 1;
	int engine_failure = 0;
	char fairing_mesh_file1[256] = "Ion_LVs\\Ion4\\Fairing_1";
	char fairing_mesh_file2[256] = "Ion_LVs\\Ion4\\Fairing_2";
	bool has_custom_fairing = false;
	double fairing_pos = 0;
	double custom_fairings_mass = 1500;
	char adapter_mesh_file[256] = "";
	bool has_custom_adapter = false;
	double custom_adapter_mass = 250;
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
	int lot_status = 0;
	NOTEHANDLE hNote;
	int lot_hh = 0;
	int lot_mm = 0;
	int lot_ss = 0;
	double lot_timer = 0;
	int dlgl_status = 0;
};

Ion4::Ion4(OBJHANDLE hVessel, int flightmodel)
	: VESSEL4(hVessel, flightmodel)
{
}

Ion4::~Ion4()
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
	return new Ion4(hvessel, flightmodel);
}

DLLCLBK void ovcExit(VESSEL* vessel)
{
	if (vessel) delete (Ion4*)vessel;
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