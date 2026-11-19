#include "Ion4.h"

void Ion4::clbkSetClassCaps(FILEHANDLE cfg)
{
	char cbuf[256];
	if (oapiReadItem_string(cfg, "FAIRING", cbuf) == true) {
		sprintf(fairing_mesh_file1, "%s_1", cbuf);
		sprintf(fairing_mesh_file2, "%s_2", cbuf);
		has_custom_fairing = true;
	}
	oapiReadItem_float(cfg, "FAIRING_POS", fairing_pos);
	oapiReadItem_float(cfg, "FAIRING_MASS", custom_fairings_mass);
	if (oapiReadItem_string(cfg, "ADAPTER", adapter_mesh_file) == true) {
		has_custom_adapter = true;
	}
	oapiReadItem_float(cfg, "ADAPTER_MASS", custom_adapter_mass);
	oapiReadItem_vec(cfg, "PAYLOAD_OFS", custom_ofs);
	stg1_exhaust = oapiRegisterExhaustTexture("Ion_LVs\\Ion4\\Exhaust_Stg1");
	stg2_exhaust = oapiRegisterExhaustTexture("Ion_LVs\\Ion4\\Exhaust_Stg2");
	thruster_exhaust = oapiRegisterExhaustTexture("Exhaust2");
	stg1_mesh = oapiLoadMesh("Ion_LVs\\Ion4\\Stage1");
	stg2_mesh = oapiLoadMesh("Ion_LVs\\Ion4\\Stage2");
	inter1_mesh = oapiLoadMesh("Ion_LVs\\Ion4\\Interstage_1");
	inter2_mesh = oapiLoadMesh("Ion_LVs\\Ion4\\Interstage_2");
	fairing1_mesh = oapiLoadMesh(fairing_mesh_file1);
	fairing2_mesh = oapiLoadMesh(fairing_mesh_file2);
	adapter_mesh = oapiLoadMesh("Ion_LVs\\Ion4\\Payload_Adapter");
	ClearMeshes(true);
	SetMeshVisibilityMode(mesh_idx[4] = AddMesh(stg1_mesh, &MESH_POS[0]), MESHVIS_ALWAYS);
	SetMeshVisibilityMode(mesh_idx[0] = AddMesh(stg2_mesh, &MESH_POS[1]), MESHVIS_ALWAYS);
	SetMeshVisibilityMode(mesh_idx[5] = AddMesh(inter1_mesh, &MESH_POS[2]), MESHVIS_ALWAYS);
	SetMeshVisibilityMode(mesh_idx[6] = AddMesh(inter2_mesh, &MESH_POS[3]), MESHVIS_ALWAYS);
	if (has_custom_fairing == false) {
		SetMeshVisibilityMode(mesh_idx[2] = AddMesh(fairing1_mesh, &MESH_POS[4]), MESHVIS_ALWAYS);
		SetMeshVisibilityMode(mesh_idx[3] = AddMesh(fairing2_mesh, &MESH_POS[5]), MESHVIS_ALWAYS);
	}
	else {
		SetMeshVisibilityMode(mesh_idx[2] = AddMesh(fairing1_mesh, &_V(0, 0, 35.47 + fairing_pos)), MESHVIS_ALWAYS);
		SetMeshVisibilityMode(mesh_idx[3] = AddMesh(fairing2_mesh, &_V(0, 0, 35.47 + fairing_pos)), MESHVIS_ALWAYS);
	}
	SetMeshVisibilityMode(mesh_idx[1] = AddMesh(adapter_mesh, &MESH_POS[6]), MESHVIS_ALWAYS);
	if (has_custom_adapter == true) {
		custom_adapter_mesh = oapiLoadMesh(adapter_mesh_file);
		SetMeshVisibilityMode(mesh_idx[7] = AddMesh(custom_adapter_mesh, &_V(0, 0, MESH_POS[6].z + 6.38)), MESHVIS_ALWAYS);
	}
	SetSize(ION4_SIZE[0]);
	payload_attach = CreateAttachment(false, ION4_PAYLOAD_POS + custom_ofs, _V(0, 0, 1), _V(0, 1, 0), "I4", false);
	pad_attach = CreateAttachment(true, ION4_PADMOUNT_POS, _V(0, 0, -1), _V(0, 1, 0), "I4P", false);
	CalcEmptyMass();
	SetPMI(ION4_PMI[0]);
	SetCrossSections(ION4_CS[0]);
	SetRotDrag(_V(0.5, 0.5, 0.01));
	stg1_fuel = CreatePropellantResource(ION4_STG1_FUELMASS);
	stg1_att_fuel = CreatePropellantResource(ION4_STG1_ATT_FUELMASS);
	stg2_fuel = CreatePropellantResource(ION4_STG2_FUELMASS);
	stg2_att_fuel = CreatePropellantResource(ION4_STG2_ATT_FUELMASS);
	rs_68b = CreateThruster(RS68B_POS, _V(0, 0, 1), RS68B_THRUST, stg1_fuel, RS68B_ISP, RS68B_ISP_SL);
	AddExhaust(rs_68b, 50, 3.25, RS68B_EXHAUST_OFS, stg1_exhaust);
	j_2x = CreateThruster(J2X_POS, _V(0, 0, 1), J2X_THRUST, stg2_fuel, J2X_ISP);
	AddExhaust(j_2x, 35, 3.5, stg2_exhaust);
	for (int i = 0; i < 4; i++) {
		stg1_att_jets[i] = CreateThruster(STG1_ATT_JETS_POS[i], STG1_ATT_JETS_DIR[i], STG1_ATT_JETS_THRUST, stg1_att_fuel, STG1_ATT_JETS_ISP);
		AddExhaust(stg1_att_jets[i], 2, 0.2, thruster_exhaust);
	}
	for (int i = 0; i < 8; i++) {
		stg2_att_jets[i] = CreateThruster(STG2_ATT_JETS_POS[i], STG2_ATT_JETS_DIR[i], STG2_ATT_JETS_THRUST, stg2_att_fuel, STG2_ATT_JETS_ISP);
		AddExhaust(stg2_att_jets[i], 1, 0.1, thruster_exhaust);
	}
	CreateThrusterGroup(&rs_68b, 1, THGROUP_MAIN);
	THRUSTER_HANDLE rcs_group[2];
	rcs_group[0] = stg1_att_jets[0];
	rcs_group[1] = stg1_att_jets[2];
	CreateThrusterGroup(rcs_group, 2, THGROUP_ATT_BANKLEFT);
	rcs_group[0] = stg1_att_jets[1];
	rcs_group[1] = stg1_att_jets[3];
	CreateThrusterGroup(rcs_group, 2, THGROUP_ATT_BANKRIGHT);
	SetCameraOffset(STG1_CAM_POS);
	DefineAnimations();
}

int Ion4::clbkConsumeBufferedKey(DWORD key, bool down, char* kstate)
{
	if (down == true) { // key down events
		if (key == OAPI_KEY_J && !KEYMOD_SHIFT(kstate) && !KEYMOD_CONTROL(kstate))
		{
			if (fairing_cfg == 1) {
				DetachChild(payload_attach, 0.5);
				CalcEmptyMass();
			}
			else {
				FairingJett();
			}
			return 1;
		}
		if (key == OAPI_KEY_V && !KEYMOD_SHIFT(kstate) && !KEYMOD_CONTROL(kstate))
		{
			if (lot_status != 1 && lot_status != 2)
			{
				if (vehicle_cfg == 0)
				{
					if (ap_status == 0)ap_status = 1;
					else ap_status = 0;
					op_status = 0;
				}
				else if (vehicle_cfg == 1)
				{
					if (op_status == 0)op_status = 1;
					else op_status = 0;
					ap_status = 0;
				}
				apop_status = ap_status + op_status;
			}
			return 1;
		}
		if (key == OAPI_KEY_N && !KEYMOD_SHIFT(kstate) && !KEYMOD_CONTROL(kstate))
		{
			Stage();
			if (ap_status == 1)
			{
				ap_status = 0;
				op_status = 1;
			}
			return 1;
		}
		if (key == OAPI_KEY_L && !KEYMOD_SHIFT(kstate) && !KEYMOD_CONTROL(kstate))
		{
			if (dlgl_status == 0)
			{
				double lat, lng, rds;
				GetEquPos(lng, lat, rds);
				min_inc = lat;
				oapiOpenInputBox("Enter Equ.Inc° (+North -South)  'Final (Dogleg)'", SetINC, 0, 50, 0);
				dlgl_status = 1;
			}
			else if (dlgl_status == 1)
			{
				oapiOpenInputBox("Enter Apogee Alt > 160 (km)", SetOAP, 0, 50, 0);
				dlgl_status = 2;
			}
			else if (dlgl_status == 2)
			{
				oapiOpenInputBox("Enter Perigee Alt > 160 (km)", SetOPE, 0, 50, 0);
				dlgl_status = 0;
			}
			return 1;
		}
	}
	return 0;
}

void Ion4::clbkPostCreation()
{
	CalcEmptyMass();
	if (engine_failure == 1) {
		if (vehicle_cfg == 0) {
			SetThrusterResource(rs_68b, NULL);
		}
		else {
			SetThrusterResource(j_2x, NULL);
		}
	}
	double lat, lng, rds;
	GetEquPos(lng, lat, rds);
	min_inc = lat;
	if (dogleg == false) {
		if (abs(tgt_inc) < abs(lat)) tgt_inc = lat;
		tgt_inc_dogleg = tgt_inc;
	}
	else {
		if (abs(tgt_inc_dogleg) < abs(lat)) tgt_inc_dogleg = lat;
	}
}

void Ion4::clbkPreStep(double simt, double simdt, double mjd)
{
	if (op_status == 1) {
		if (vehicle_cfg > 0) {
			if (GetPropellantMass(stg2_fuel) <= 0) {
				op_status = 0;
				for (int i = 2; i < 8; i++) {
					SetThrusterLevel(stg2_att_jets[i], 0);
				}
				ActivateNavmode(NAVMODE_KILLROT);
			}
			else if (engine_failure == 1) {
				op_status = 0;
				for (int i = 2; i < 8; i++) {
					SetThrusterLevel(stg2_att_jets[i], 0);
				}
				ActivateNavmode(NAVMODE_KILLROT);
			}
		}
	}

	if (lot_status == 1)DoLOT();
	else if (lot_status == 2)DoLOT();
	if (ap_status == 1) DoAscent();
	else if (op_status == 1) DoOrbit();
	apop_status = op_status + ap_status;

	ManualTVC();
}

void Ion4::clbkPostStep(double simt, double simdt, double mjd)
{
	DoEngineFailure(simdt);
}

void Ion4::clbkSaveState(FILEHANDLE scn)
{
	char cbuf[256];

	VESSEL4::clbkSaveState(scn);

	oapiWriteScenario_int(scn, "CFG", vehicle_cfg);

	oapiWriteScenario_int(scn, "FAIRING", fairing_cfg);

	if (payload_mass_override > 0) {
		if (GetAttachmentStatus(payload_attach) == NULL) {}
		else {
			oapiWriteScenario_float(scn, "MASS_OVERRIDE", payload_mass_override);
		}
	}

	sprintf(cbuf, "%0.4f %d %d %d %d %0.1f %0.4f", timer, ap_status, ap_function, op_status, start_sw, ap_tgt, pitch_angle);
	oapiWriteScenario_string(scn, "AUTOPILOT", cbuf);

	sprintf(cbuf, "%0.3f %0.1f %0.1f", tgt_inc * DEG, op_meco_alt / 1000, op_tgt / 1000);
	oapiWriteScenario_string(scn, "TGT_ORBIT", cbuf);

	if (dogleg == true) {
		sprintf(cbuf, "%0.3f", tgt_inc_dogleg * DEG);
		oapiWriteScenario_string(scn, "DOGLEG", cbuf);
	}

	sprintf(cbuf, "%d %d %d %d", lot_hh, lot_mm, lot_ss, lot_status);
	oapiWriteScenario_string(scn, "LAUNCH_TIME", cbuf);

	if (engine_failure == 1) {
		oapiWriteScenario_string(scn, "ENGINE_FAILURE", "");
	}
}

void Ion4::clbkLoadStateEx(FILEHANDLE scn, void* status)
{
	char* line;
	while (oapiReadScenario_nextline(scn, line))
	{
		if (!_strnicmp(line, "CFG", 3)) {
			sscanf(line + 3, "%i", &vehicle_cfg);
		}
		else if (!_strnicmp(line, "FAIRING", 7)) {
			sscanf(line + 7, "%i", &fairing_cfg);
		}
		else if (!_strnicmp(line, "MASS_OVERRIDE", 13)) {
			sscanf(line + 13, "%lf", &payload_mass_override);
		}
		else if (!_strnicmp(line, "AUTOPILOT", 9)) {
			sscanf(line + 9, "%lf %d %d %d %d %lf %lf", &timer, &ap_status, &ap_function, &op_status, &start_sw, &ap_tgt, &pitch_angle);
		}
		else if (!_strnicmp(line, "TGT_ORBIT", 9)) {
			sscanf(line + 9, "%lf %lf %lf", &tgt_inc, &op_meco_alt, &op_tgt);
			tgt_inc = tgt_inc * RAD;
			op_meco_alt *= 1000;
			op_tgt *= 1000;
		}
		else if (!_strnicmp(line, "DOGLEG", 6)) {
			sscanf(line + 6, "%lf", &tgt_inc_dogleg);
			tgt_inc_dogleg = tgt_inc_dogleg * RAD;
			dogleg = true;
		}
		else if (!_strnicmp(line, "LAUNCH_TIME", 11)) {
			sscanf(line + 11, "%d %d %d %d", &lot_hh, &lot_mm, &lot_ss, &lot_status);
		}
		else if (!_strnicmp(line, "ENGINE_FAILURE", 14)) {
			engine_failure = 1;
		}
		else {
			ParseScenarioLineEx(line, status);
		}
	}

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

	if (vehicle_cfg > 0) {
		DelFirstStage();
	}
	if (fairing_cfg > 0) {
		DelFairing();
	}
}

void Ion4::CalcEmptyMass()
{
	double new_mass = ION4_MASS_STG2;
	OBJHANDLE payload = GetAttachmentStatus(payload_attach);
	if (vehicle_cfg == 0) {
		new_mass += ION4_MASS_STG1;
	}
	if (has_custom_adapter == true) {
		new_mass += custom_adapter_mass;
	}
	if (fairing_cfg == 0) {
		if (has_custom_fairing == false) {
			new_mass += ION4_MASS_FAIRINGS;
		}
		else {
			new_mass += custom_fairings_mass;
		}
	}
	if (payload == NULL) {}
	else {
		if (payload_mass_override > 0) {
			new_mass += payload_mass_override;
		}
		else {
			new_mass += oapiGetMass(payload);
		}
	}
	SetEmptyMass(new_mass);
}

void Ion4::DefineAnimations()
{
	static UINT tvc_group[1] = { 12 };
	static MGROUP_ROTATE tvc_pitch_comp(mesh_idx[4], tvc_group, 1, RS68B_POS, _V(-1, 0, 0), (float)(MAX_TVC_ANGLE * 2));
	static MGROUP_ROTATE tvc_yaw_comp(mesh_idx[4], tvc_group, 1, RS68B_POS, _V(0, 1, 0), (float)(MAX_TVC_ANGLE * 2));
	anim_tvc_pitch = CreateAnimation(0.5);
	AddAnimationComponent(anim_tvc_pitch, 0, 1, &tvc_pitch_comp);
	anim_tvc_yaw = CreateAnimation(0.5);
	AddAnimationComponent(anim_tvc_yaw, 0, 1, &tvc_yaw_comp);
}

void Ion4::DoTVC(double pitch, double yaw)
{
	VECTOR3 new_dir = _V(0, 0, 1);
	new_dir.y = pitch * MAX_TVC_ANGLE;
	new_dir.x = yaw * MAX_TVC_ANGLE;
	normalise(new_dir);
	SetThrusterDir(rs_68b, new_dir);
	SetAnimation(anim_tvc_pitch, (pitch + 1) / 2);
	SetAnimation(anim_tvc_yaw, (yaw + 1) / 2);
}

void Ion4::ManualTVC()
{
	if (vehicle_cfg == 0) {
		double pitch_up = GetManualControlLevel(THGROUP_ATT_PITCHUP, MANCTRL_ROTMODE);
		double pitch_dn = GetManualControlLevel(THGROUP_ATT_PITCHDOWN, MANCTRL_ROTMODE);
		double yaw_left = GetManualControlLevel(THGROUP_ATT_YAWLEFT, MANCTRL_ROTMODE);
		double yaw_right = GetManualControlLevel(THGROUP_ATT_YAWRIGHT, MANCTRL_ROTMODE);
		double pitch = 0;
		double yaw = 0;
		if (pitch_up > 0) {
			pitch = -pitch_up;
		}
		else if (pitch_dn > 0) {
			pitch = pitch_dn;
		}
		if (yaw_left > 0) {
			yaw = yaw_left;
		}
		else if (yaw_right > 0) {
			yaw = -yaw_right;
		}
		if (apop_status == 0) {
			DoTVC(pitch, yaw);
		}
	}
}

void Ion4::Stage()
{
	DelFirstStage();
	SpawnObject("Ion4\\Stg1", "_Stg1", MESH_POS[0] + CG_SHIFT_S2, _V(0, 0, -0.5), _V(oapiRand() / 200, oapiRand() / 200, oapiRand() / 200));
	SpawnObject("Ion4\\IntPanel1", "_IntPanel1", MESH_POS[2] + CG_SHIFT_S2, _V(-2, 0, -0.25), _V(oapiRand() / 10, (oapiRand() / 10) + 0.25, oapiRand() / 10));
	SpawnObject("Ion4\\IntPanel2", "_IntPanel2", MESH_POS[3] + CG_SHIFT_S2, _V(2, 0, -0.25), _V(oapiRand() / 10, (oapiRand() / 10) - 0.25, oapiRand() / 10));
	if (ap_status == 1)
	{
		ap_status = 0;
		op_status = 1;
	}
}

void Ion4::DelFirstStage()
{
	vehicle_cfg = 1;
	engine_failure = 0;
	CalcEmptyMass();
	DelAnimation(anim_tvc_pitch);
	DelAnimation(anim_tvc_yaw);
	DelAttachment(pad_attach);
	DelThruster(rs_68b);
	for (int i = 0; i < 4; i++) {
		DelThruster(stg1_att_jets[i]);
	}
	DelPropellantResource(stg1_fuel);
	DelPropellantResource(stg1_att_fuel);
	CreateThrusterGroup(&j_2x, 1, THGROUP_MAIN);
	SetMeshVisibilityMode(mesh_idx[4], MESHVIS_NEVER);
	SetMeshVisibilityMode(mesh_idx[5], MESHVIS_NEVER);
	SetMeshVisibilityMode(mesh_idx[6], MESHVIS_NEVER);
	THRUSTER_HANDLE rcs_group[2];
	rcs_group[0] = stg2_att_jets[2];
	rcs_group[1] = stg2_att_jets[3];
	CreateThrusterGroup(rcs_group, 2, THGROUP_ATT_PITCHUP);
	rcs_group[0] = stg2_att_jets[4];
	rcs_group[1] = stg2_att_jets[5];
	CreateThrusterGroup(rcs_group, 2, THGROUP_ATT_PITCHDOWN);
	CreateThrusterGroup(&stg2_att_jets[6], 1, THGROUP_ATT_YAWLEFT);
	CreateThrusterGroup(&stg2_att_jets[7], 1, THGROUP_ATT_YAWRIGHT);
	rcs_group[0] = stg2_att_jets[2];
	rcs_group[1] = stg2_att_jets[4];
	CreateThrusterGroup(rcs_group, 2, THGROUP_ATT_BANKLEFT);
	rcs_group[0] = stg2_att_jets[3];
	rcs_group[1] = stg2_att_jets[5];
	CreateThrusterGroup(rcs_group, 2, THGROUP_ATT_BANKRIGHT);
	rcs_group[0] = stg2_att_jets[0];
	rcs_group[1] = stg2_att_jets[1];
	CreateThrusterGroup(rcs_group, 2, THGROUP_ATT_FORWARD);
	ShiftCG(-CG_SHIFT_S2);
	SetSize(ION4_SIZE[1]);
	SetPMI(ION4_PMI[1]);
	SetCrossSections(ION4_CS[1]);
	SetCameraOffset(STG2_CAM_POS);
}

void Ion4::FairingJett()
{
	DelFairing();
	if (has_custom_fairing == false) {
		if (vehicle_cfg == 0) {
			SpawnObject("Ion4\\Fairing1", "_Fairing1", MESH_POS[4], _V(-1, 0, -0.15), _V(oapiRand() / 100, (oapiRand() / 100) + 0.05, oapiRand() / 100));
			SpawnObject("Ion4\\Fairing2", "_Fairing2", MESH_POS[5], _V(1, 0, -0.15), _V(oapiRand() / 100, (oapiRand() / 100) - 0.05, oapiRand() / 100));
		}
		else {
			SpawnObject("Ion4\\Fairing1", "_Fairing1", MESH_POS[4] + CG_SHIFT_S2, _V(-1, 0, -0.15), _V(oapiRand() / 100, (oapiRand() / 100) + 0.05, oapiRand() / 100));
			SpawnObject("Ion4\\Fairing2", "_Fairing2", MESH_POS[5] + CG_SHIFT_S2, _V(1, 0, -0.15), _V(oapiRand() / 100, (oapiRand() / 100) - 0.05, oapiRand() / 100));
		}
	}
	else {
		OBJHANDLE fairing[2];
		VESSEL4* v;
		if (vehicle_cfg == 0) {
			fairing[0] = SpawnObject("Ion4\\Fairing1", "_Fairing1", _V(0, 0, 35.47 + fairing_pos), _V(-1, 0, -0.15), _V(oapiRand() / 100, (oapiRand() / 100) + 0.05, oapiRand() / 100));
			fairing[1] = SpawnObject("Ion4\\Fairing2", "_Fairing2", _V(0, 0, 35.47 + fairing_pos), _V(1, 0, -0.15), _V(oapiRand() / 100, (oapiRand() / 100) - 0.05, oapiRand() / 100));
		}
		else {
			fairing[0] = SpawnObject("Ion4\\Fairing1", "_Fairing1", _V(0, 0, 35.47 + fairing_pos) + CG_SHIFT_S2, _V(-1, 0, -0.15), _V(oapiRand() / 100, (oapiRand() / 100) + 0.05, oapiRand() / 100));
			fairing[1] = SpawnObject("Ion4\\Fairing2", "_Fairing2", _V(0, 0, 35.47 + fairing_pos) + CG_SHIFT_S2, _V(1, 0, -0.15), _V(oapiRand() / 100, (oapiRand() / 100) - 0.05, oapiRand() / 100));
		}
		v = (VESSEL4*)oapiGetVesselInterface(fairing[0]);
		v->ClearMeshes(false);
		v->AddMesh(fairing_mesh_file1);
		v = (VESSEL4*)oapiGetVesselInterface(fairing[1]);
		v->ClearMeshes(false);
		v->AddMesh(fairing_mesh_file2);
	}
}

void Ion4::DelFairing()
{
	fairing_cfg = 1;
	CalcEmptyMass();
	SetMeshVisibilityMode(mesh_idx[2], MESHVIS_NEVER);
	SetMeshVisibilityMode(mesh_idx[3], MESHVIS_NEVER);
}

OBJHANDLE Ion4::SpawnObject(char* classsname, char* ext, VECTOR3 ofs, VECTOR3 spd, VECTOR3 rot)
{
	OBJHANDLE temp;
	VESSELSTATUS vs;
	char name[256];
	GetStatus(vs);
	VECTOR3 rvel;
	vs.vrot += rot;
	Local2Rel(ofs, vs.rpos);
	GlobalRot(spd, rvel);
	vs.rvel += rvel;
	strcpy(name, GetName()); strcat(name, ext);
	temp = oapiCreateVessel(name, classsname, vs);
	return temp;
}

// For some reason, this doesn't work right.  Engines fail WAY to often...
void Ion4::DoEngineFailure(double simdt)
{
	//if (GetThrusterGroupLevel(THGROUP_MAIN) > 0) {
	//	double random_number = oapiRand();
	//	if (vehicle_cfg == 0) {
	//		random_number *= (RS68B_REL / simdt);
	//		random_number /= failure_mutliply;
	//		if (random_number <= 1) {
	//			SetThrusterResource(rs_68b, NULL);
	//			engine_failure = 1;
	//		}
	//	}
	//	else {
	//		random_number *= (J2X_REL / simdt);
	//		random_number /= failure_mutliply;
	//		if (random_number <= 1) {
	//			SetThrusterResource(j_2x, NULL);
	//			engine_failure = 1;
	//		}
	//	}
	//}
}

// WARNING!!!
// I am not really sure how some of the stuff from here down works!
// Beyond this point, all code taken/modified from the Falcon 9R addon by BrianJ.  Used with permission :)
// If it seems to work right, I leave it alone...

void Ion4::DoAscent(void)
{
	if (vehicle_cfg == 1)return;

	CalcAzimuth();
	CalcFlightVector();

	double englev = 1;

	if (ap_function == 0)
	{
		OBJHANDLE hLpad;
		VESSEL4* v;

		if (tu_sw == 0)
		{
			hLpad = GetAttachmentStatus(pad_attach);
			if (!hLpad) tu_sw = 1;
			else
			{
				tu_sw = 2;
				v = (VESSEL4*)oapiGetVesselInterface(hLpad);
			}
		}

		if (tu_sw == 1)
		{
			timer = timer + oapiGetSimStep();
			if (timer > 10)
			{
				englev = 1;
				timer = 10;
			}
			else englev = 0;
		}
		else if (tu_sw == 2)
		{
			timer = timer + oapiGetSimStep();
			if (timer > 10) timer = 10;
			if (timer < 10) englev = (timer - 7) / 3;
			if (englev < 0)englev = 0;
		}
	}
	SetThrusterGroupLevel(THGROUP_MAIN, englev);

	if (GetAltitude(ALTMODE_GROUND) > 70 && ap_function == 0)
	{
		ap_function = 1;
		timer = 0;
	}
	if (ap_function > 0) RollToAz();
	if (ap_function == 1) GoToFlightVector();
	if (ap_function == 2) GoToFlightVector();
	if (ap_function == 3) GoToFlightVector();
	if (ap_function == 4) GoToFlightVector();
	if (ap_function == 5)
	{
		SetThrusterGroupLevel(THGROUP_MAIN, 0);
		ActivateNavmode(NAVMODE_KILLROT);
		ap_function = 0;
		ap_status = 0;
		Stage();
		op_status = 1;
	}
}

void Ion4::DoOrbit()
{
	double engine_start_lvl = 0;
	if (start_sw == 0)
	{
		timer = timer + oapiGetSimStep();
		if (timer < 12) {
			if ((timer >= 2) & (fairing_cfg == 0)) {
				if (GetDynPressure() < 500) {
					FairingJett();
				}
			}
			SetThrusterGroupLevel(THGROUP_ATT_FORWARD, 1);
			if (timer >= 10) {
				engine_start_lvl = (timer - 10) / 2;
			}
			else {
				return;
			}
		}
		else
		{
			start_sw = 1;
			timer = 0;
			SetThrusterGroupLevel(THGROUP_ATT_FORWARD, 0);
			SetThrusterLevel(j_2x, 1);
		}
	}

	DeactivateNavmode(NAVMODE_KILLROT);
	CalcAzimuth();

	double fnl_vel = sqrt(mu * (2 / (op_meco_alt + rt) - 1 / (0.5 * (op_tgt + op_meco_alt + (2 * rt)))));

	double emp_mass = GetEmptyMass();
	double vmass = GetMass();

	OBJHANDLE hearth = GetSurfaceRef();

	VECTOR3 haspdv = _V(0, 0, 0);
	GetGroundspeedVector(FRAME_HORIZON, haspdv);

	VECTOR3 weight = _V(0, 0, 0);
	GetWeightVector(weight);

	//centrifugal acc
	VECTOR3 relvel = _V(0, 0, 0);
	GetRelativeVel(hearth, relvel);
	double orb_vel = length(relvel);
	double orb_grnd_spd = sqrt((orb_vel * orb_vel) - (haspdv.y * haspdv.y));
	double rds = (rt + GetAltitude());
	double ang_vel = orb_grnd_spd / rds;
	double acc_cent_fact = 1;
	double acc_cent = ang_vel * ang_vel * rds * acc_cent_fact;

	double gravt = length(weight);
	double g = -(gravt / vmass) + acc_cent;

	double aa = g / 2;
	double bb = haspdv.y;
	double cc = GetAltitude() - op_meco_alt;

	double max_th = GetThrusterMax(j_2x);
	double max_acc = max_th / vmass;
	double acc_lim = 7 * G;
	double max_eng_lvl = 1;
	if (max_acc > acc_lim) max_eng_lvl = acc_lim / max_acc;
	if (max_eng_lvl < 0.6)max_eng_lvl = 0.6;
	double sinp = sin(GetPitch());
	double cosp = cos(GetPitch());

	double dv_req = fnl_vel - orb_grnd_spd;
	double fraction = exp(dv_req / GetThrusterIsp(j_2x));
	double fuelmass = vmass - (vmass / fraction);

	double dth = oapiGetSimStep() * 0.2;

	double englevel = GetThrusterGroupLevel(THGROUP_MAIN);

	double fuelrate = GetThrusterMax(j_2x) * englevel / GetThrusterIsp(j_2x);
	double dvtime = fuelmass / fuelrate;
	dvtime = dvtime / cosp;

	GetElements(hearth, el, &prm, 0, FRAME_EQU);
	double ap_time = prm.ApT;
	double ap_time_limiter = dvtime / ap_time;
	if (ap_time_limiter > 1)ap_time_limiter = 1;
	else if (ap_time_limiter < 1)ap_time_limiter = 1;
	if (ap_time_limiter < max_eng_lvl) max_eng_lvl = ap_time_limiter;

	double vvel_fnl = 0;

	double acc_req = ((6 * (op_meco_alt - GetAltitude() - (haspdv.y * dvtime))) + (2 * dvtime * haspdv.y)) / (dvtime * dvtime);

	acc_req = acc_req - g;

	double srp = acc_req / (max_acc * englevel);
	if (srp > 1)srp = 1;
	else if (srp < -1)srp = -1;
	double req_pitch = asin(srp);
	double crp = cos(req_pitch);

	VECTOR3 fv2d = _V(0, srp, crp);
	VECTOR3 fv_hrz = _V(0, 0, 0);
	fv_hrz.x = sin(azimuth) * crp;
	fv_hrz.y = srp;
	fv_hrz.z = cos(azimuth) * crp;

	double max_dv = oapiGetSimStep() * max_acc * englevel;
	double req_englevel = (fnl_vel - orb_vel) / max_dv;
	if (req_englevel < 0)
	{
		op_status = 0;
		englevel = 0;
		SetThrusterLevel(j_2x, 0);
		for (int i = 2; i < 8; i++) {
			SetThrusterLevel(stg2_att_jets[i], 0);
		}
		ActivateNavmode(NAVMODE_KILLROT);
		return;
	}
	if (req_englevel > max_eng_lvl)req_englevel = max_eng_lvl;

	if (englevel > req_englevel)
	{
		englevel = englevel - dth;
		if (englevel < req_englevel) englevel = req_englevel;
	}
	else if (englevel < req_englevel)
	{
		englevel = englevel + dth;
		if (englevel > req_englevel) englevel = req_englevel;
	}

	if ((fnl_vel - orb_vel) < 30)
	{
		ActivateNavmode(NAVMODE_KILLROT);

	}
	else
	{
		HorizonInvRot(fv_hrz, flight_vector);
		GoToFlightVector();
	}

	if (start_sw == 0)englevel = engine_start_lvl;
	SetThrusterLevel(j_2x, englevel);
}

double Ion4::CalcAzimuth()
{
	VESSEL4* v;
	OBJHANDLE hPad = GetAttachmentStatus(pad_attach);
	if (hPad != NULL) v = (VESSEL4*)oapiGetVesselInterface(hPad);
	else v = (VESSEL4*)oapiGetVesselInterface(GetHandle());

	double tgt_inc_thisstep = 0;
	if (vehicle_cfg == 0) {
		tgt_inc_thisstep = tgt_inc_dogleg;
	}
	else {
		tgt_inc_thisstep = tgt_inc;
	}

	double lon, lat, rad;
	v->GetEquPos(lon, lat, rad);
	if (abs(tgt_inc_thisstep) < abs(lat)) {
		tgt_inc_thisstep = abs(lat);
	}
	else {
		tgt_inc_thisstep = tgt_inc_thisstep;
	}

	OBJHANDLE hearth = v->GetSurfaceRef();
	VECTOR3 relvel = _V(0, 0, 0);
	VECTOR3 globpos = _V(0, 0, 0);
	v->GetRelativeVel(hearth, relvel);
	v->GetGlobalPos(globpos);
	VECTOR3 relvel_glob = relvel + globpos;
	VECTOR3 rel_vel_loc = _V(0, 0, 0);
	v->Global2Local(relvel_glob, rel_vel_loc);

	VECTOR3 ov_hrz = _V(0, 0, 0);
	v->HorizonRot(rel_vel_loc, ov_hrz);

	double orb_ground_spd = sqrt((ov_hrz.x * ov_hrz.x) + (ov_hrz.z * ov_hrz.z));

	VECTOR3 orb_ground_vector = _V(ov_hrz.x, 0, ov_hrz.z);

	double tgt_inc_sc = tgt_inc_thisstep;
	if (v->GetAltitude(ALTMODE_GROUND) > 10000)
	{
		if (lat > 0)
		{
			if (ov_hrz.z < -1 && tgt_inc_thisstep > 0) tgt_inc_sc = -tgt_inc_thisstep;
		}
		else if (lat < 0)
		{
			if (ov_hrz.z > 1 && tgt_inc_thisstep < 0) tgt_inc_sc = -tgt_inc_thisstep;
		}
	}
	double final_azimuth = asin(cos(tgt_inc_sc) / cos(lat));
	final_azimuth += (90 * RAD - final_azimuth) * (1 - (tgt_inc_sc / abs(tgt_inc_sc)));
	VECTOR3 final_ground_vector = _V((FinalV() * sin(final_azimuth)), 0, (FinalV() * cos(final_azimuth)));

	VECTOR3 req_ground_vector = final_ground_vector - orb_ground_vector;

	azimuth = atan2(req_ground_vector.x, req_ground_vector.z);

	vazim1 = _V(sin(azimuth), 0, cos(azimuth));

	v->HorizonInvRot(vazim1, azim_local);
	return azimuth;
}

void Ion4::CalcFlightVector()
{
	double initial_pitch = RAD * (77.0 + (6 * ((ap_tgt - 125000) * 0.00001))); //77.5, 5*

	double fuel_res_fctr = 0;
	if (fuel_res_fctr < 0)fuel_res_fctr = 0;
	else if (fuel_res_fctr > 1)fuel_res_fctr = 1;
	fuel_res_fctr = 3.5 * fuel_res_fctr * RAD; // 3

	initial_pitch = initial_pitch + fuel_res_fctr;

	double azmth = CalcAzimuth();
	double azmth_fctr = RAD * (-0.25 * (sin(azmth)));

	initial_pitch = initial_pitch + azmth_fctr;

	if (ap_function == 2)
	{
		VECTOR3 haspdv = _V(0, 1, 0);
		GetGroundspeedVector(FRAME_HORIZON, haspdv);
		double mass = GetMass();
		VECTOR3 thrustvector = _V(0, 0, 0);
		GetThrustVector(thrustvector);
		double trst = length(thrustvector);
		double acc = trst / mass;
		double time_avail = (200 - length(haspdv)) / (acc - 9.8);
		double pitch_rate = ((pitch_angle)-initial_pitch) / time_avail;
		pitch_angle = pitch_angle - (oapiGetSimStep() * pitch_rate);

		if (length(haspdv) > 200)ap_function = 3;
	}
	else if (ap_function == 3)
	{
		VECTOR3 haspdv = _V(0, 1, 0);
		GetGroundspeedVector(FRAME_HORIZON, haspdv);
		double xzlngth = sqrt((haspdv.x * haspdv.x) + (haspdv.z * haspdv.z));
		if (haspdv.y < 0)haspdv.y = 0;
		pitch_angle = atan2(haspdv.y, xzlngth);
		if (GetAltitude() > 30000) ap_function = 4;
	}
	else if (ap_function == 4)
	{
		double ap_dist = 0;
		GetApDist(ap_dist);

		double englev = GetThrusterGroupLevel(THGROUP_MAIN);
		double fuel_remaining = GetPropellantMass(stg1_fuel);
		double isp_curr = 0;
		double th_max = 0;
		if (vehicle_cfg == 0) {
			isp_curr = GetThrusterIsp(rs_68b);
			th_max = GetThrusterMax(rs_68b);
		}
		else {
			isp_curr = GetThrusterIsp(j_2x);
			th_max = GetThrusterMax(j_2x);
		}
		double fuel_rate = englev * th_max / isp_curr;
		double mintime = fuel_remaining / fuel_rate;
		if (mintime < 0)mintime = 0;

		VECTOR3 haspdv = _V(0, 1, 0);
		GetGroundspeedVector(FRAME_HORIZON, haspdv);
		if (haspdv.y < 0)haspdv.y = 0;

		double vmass = GetMass();

		OBJHANDLE hearth = GetSurfaceRef();
		VECTOR3 relvel = _V(0, 0, 0);
		GetRelativeVel(hearth, relvel);
		double orb_vel = length(relvel);
		double orb_grnd_spd = sqrt((orb_vel * orb_vel) - (haspdv.y * haspdv.y));
		double circ = PI2 * (rt + GetAltitude());
		double ang_vel = PI2 * orb_grnd_spd / circ;
		VECTOR3 angmomvect = _V(0, (-1 * ang_vel), 0);
		VECTOR3 velvect = _V(0, 0, orb_grnd_spd);
		VECTOR3 corvect = crossp(angmomvect, velvect);
		double acc_cor = length(corvect);

		double sinp = sin(GetPitch());
		double dv_grnd = isp_curr * log(vmass / (vmass - fuel_remaining));
		dv_grnd = dv_grnd * sinp;

		double final_orb_grnd_spd = orb_grnd_spd + dv_grnd;
		velvect = _V(0, 0, final_orb_grnd_spd);
		VECTOR3 final_corvect = crossp(angmomvect, velvect);
		double final_acc_cor = length(corvect);

		VECTOR3 weight = _V(0, 0, 0);
		GetWeightVector(weight);

		double gravt = length(weight);
		double gg = (gravt / vmass) - final_acc_cor;
		double aa = (mintime * mintime) / (2 * gg);
		double bb = (haspdv.y * mintime / gg) + ((mintime * mintime) / 2);
		double cc = -(ap_tgt - GetAltitude() - (haspdv.y * mintime) - ((haspdv.y * haspdv.y) / (2 * gg)));

		double req_acc_1 = (-bb + (sqrt((bb * bb) - (4 * aa * cc)))) / (2 * aa);
		double req_acc_2 = (-bb - (sqrt((bb * bb) - (4 * aa * cc)))) / (2 * aa);

		double curr_acc = th_max * englev / (vmass - (fuel_remaining * 0.5));

		if (fuel_remaining == 0)ap_function = 5;

		double total_req_acc = req_acc_1 + gg - acc_cor;

		double psc = total_req_acc / curr_acc;
		if (psc > 0.9962)psc = 0.9962;
		else if (psc < -0.9962)psc = -0.9962;
		double pangle = asin(psc);

		if (mintime < 2)pitch_angle = pitch_angle;
		else pitch_angle = pangle;
	}

	vazim2 = _V(vazim1.x, (tan(pitch_angle)), vazim1.z);

	if (ap_function < 2)
	{
		VECTOR3 haspdv = _V(0, 1, 0);
		GetGroundspeedVector(FRAME_HORIZON, haspdv);
		vazim2 = _V(-haspdv.x * 0.5, haspdv.y, -haspdv.z * 0.5);
	}
	HorizonInvRot(vazim2, flight_vector);
}

void Ion4::GoToFlightVector()
{
	DeactivateNavmode(NAVMODE_KILLROT);

	double angoa = atan2(-flight_vector.y, flight_vector.z);
	double slipage = atan2(flight_vector.x, flight_vector.z);

	if (angoa > PI)angoa = (PI * 2) - angoa;
	else if (angoa < -PI)angoa = (PI * 2) + angoa;

	if (slipage > PI)slipage = (PI * 2) - slipage;
	else if (slipage < -PI)slipage = (PI * 2) + slipage;

	double mass = GetMass();

	double max_thrust = RS68B_THRUST * (sin(MAX_TVC_ANGLE));
	double max_torque = max_thrust * 29;
	double inertia = 140 * mass;

	if (vehicle_cfg == 1)
	{
		max_thrust = GetThrusterMax(stg2_att_jets[0]);
		max_torque = 2 * max_thrust * 8;
		inertia = 15 * mass;
	}

	VECTOR3 avel = _V(0, 0, 0);
	GetAngularVel(avel);
	double max_acc = max_torque / inertia;
	double max_drot = max_acc * oapiGetSimStep();

	double reqxv = 0.9 * (sqrt(abs(angoa * max_acc * 2)));
	if (reqxv > vrot_lim)reqxv = vrot_lim;
	if (angoa > 0)reqxv = -reqxv;
	double req_drotx = reqxv - avel.x;
	double thx_lvl = req_drotx / max_drot;
	if ((angoa * DEG) < 1 && (angoa * DEG) > -1)
	{
		reqxv = 1 * -angoa;
		double acc_reqx = (reqxv - avel.x) / 0.1;
		thx_lvl = acc_reqx / max_acc;
	}

	double reqyv = 0.9 * (sqrt(abs(slipage * max_acc * 2)));
	if (reqyv > vrot_lim)reqyv = vrot_lim;
	if (slipage > 0)reqyv = -reqyv;
	double req_droty = reqyv - avel.y;
	double thy_lvl = req_droty / max_drot;
	if ((slipage * DEG) < 1 && (slipage * DEG) > -1)
	{
		reqyv = 1 * -slipage;
		double acc_reqy = (reqyv - avel.y) / 0.1;
		thy_lvl = acc_reqy / max_acc;
	}

	if (thx_lvl > 1)thx_lvl = 1;
	else if (thx_lvl < -1)thx_lvl = -1;

	if (thy_lvl > 1)thy_lvl = 1;
	else if (thy_lvl < -1)thy_lvl = -1;

	if (vehicle_cfg == 0) {
		DoTVC(-thx_lvl, thy_lvl);
	}
	else {
		if (thx_lvl > 0)
		{
			SetThrusterGroupLevel(THGROUP_ATT_PITCHUP, thx_lvl);
			SetThrusterGroupLevel(THGROUP_ATT_PITCHDOWN, 0);
		}
		else if (thx_lvl < 0)
		{
			SetThrusterGroupLevel(THGROUP_ATT_PITCHDOWN, -thx_lvl);
			SetThrusterGroupLevel(THGROUP_ATT_PITCHUP, 0);
		}
		else
		{
			SetThrusterGroupLevel(THGROUP_ATT_PITCHUP, 0);
			SetThrusterGroupLevel(THGROUP_ATT_PITCHDOWN, 0);
		}

		if (thy_lvl > 0)
		{
			SetThrusterGroupLevel(THGROUP_ATT_YAWLEFT, thy_lvl);
			SetThrusterGroupLevel(THGROUP_ATT_YAWRIGHT, 0);
		}
		else if (thy_lvl < 0)
		{
			SetThrusterGroupLevel(THGROUP_ATT_YAWRIGHT, -thy_lvl);
			SetThrusterGroupLevel(THGROUP_ATT_YAWLEFT, 0);
		}
		else
		{
			SetThrusterGroupLevel(THGROUP_ATT_YAWRIGHT, 0);
			SetThrusterGroupLevel(THGROUP_ATT_YAWLEFT, 0);
		}
	}
}

double Ion4::FinalV()
{
	double tgtapo = 200000;
	double tgtperi = 2000000;
	double abside = tgtapo;
	if (tgtapo <= 260000) {
		abside = tgtapo;
	}
	else {
		abside = tgtperi;
	}
	if (op_status == 1) return sqrt(mu * (2 / (op_meco_alt + rt) - 1 / (0.5 * (op_tgt + op_meco_alt + (2 * rt)))));
	else return sqrt(mu * (2 / (abside + rt) - 1 / (0.5 * (tgtapo + tgtperi + 2 * rt))));
}

double Ion4::GetOS() {
	OBJHANDLE hearth = GetSurfaceRef();
	VECTOR3 relvel;
	GetRelativeVel(hearth, relvel);
	double os = length(relvel);
	return os;
}

void Ion4::RollToAz()
{
	DeactivateNavmode(NAVMODE_KILLROT);
	double roll_angle = atan2(azim_local.x, -azim_local.y);
	if (roll_angle > PI) roll_angle = roll_angle - PI2;
	else if (roll_angle < -PI) roll_angle = roll_angle + PI2;

	double mass = GetMass();

	double max_rcs = GetThrusterMax(stg1_att_jets[0]);
	double max_torque = 2 * max_rcs * 1.5;
	double inertia = 2 * mass;

	if (vehicle_cfg == 1)
	{
		max_rcs = GetThrusterMax(stg2_att_jets[0]);
		max_torque = 2 * max_rcs * 2;
		inertia = 1.5 * mass;
	}
	VECTOR3 avel = _V(0, 0, 0);
	GetAngularVel(avel);

	double max_acc = max_torque / inertia;

	double max_drot = max_acc * oapiGetSimStep();

	double reqzv = 0.9 * (sqrt(abs(roll_angle * max_acc * 2)));
	if (roll_angle > 0)reqzv = -reqzv;

	double req_drot = reqzv - avel.z;

	double thz_lvl = req_drot / max_drot;

	if ((roll_angle * DEG) < 1 && (roll_angle * DEG) > -1)
	{
		reqzv = 1 * -roll_angle;
		double acc_reqz = (reqzv - avel.z) / 0.1;
		thz_lvl = acc_reqz / max_acc;
	}

	if (thz_lvl > 1)thz_lvl = 1;
	else if (thz_lvl < -1)thz_lvl = -1;

	if (thz_lvl > 0)
	{
		SetThrusterGroupLevel(THGROUP_ATT_BANKRIGHT, thz_lvl);
		SetThrusterGroupLevel(THGROUP_ATT_BANKLEFT, 0);
	}
	else if (thz_lvl < 0)
	{
		SetThrusterGroupLevel(THGROUP_ATT_BANKLEFT, -thz_lvl);
		SetThrusterGroupLevel(THGROUP_ATT_BANKRIGHT, 0);
	}
	else
	{
		SetThrusterGroupLevel(THGROUP_ATT_BANKRIGHT, 0);
		SetThrusterGroupLevel(THGROUP_ATT_BANKLEFT, 0);
	}

	if (ap_function == 1)
	{
		if (roll_angle > -0.05 && roll_angle < 0.05)
		{
			VECTOR3 aspdv = _V(0, 1, 0);
			GetHorizonAirspeedVector(aspdv);
			double azimuth = CalcAzimuth();
			double az_factor = 30 * (1 - sin(azimuth));
			if (aspdv.y > (55)) //+az_factor
			{
				vrot_lim = 2 * RAD;
				ap_function = 2;
			}
		}
	}
}

void Ion4::DoLOT(void)
{
	oapiDelAnnotation(hNote);
	double lot_secs = (3600 * lot_hh) + (60 * lot_mm) + (lot_ss);
	double current_mjd = oapiGetSimMJD();
	double ct_secs_frac = current_mjd - (int)(current_mjd);
	double ct_secs = 86400 * ct_secs_frac;

	int ct_secs_int = (int)(ct_secs);

	int lot_secs_int = (int)(lot_secs);
	if (lot_secs_int < ct_secs_int)lot_secs_int = lot_secs_int + 86400;

	int t_minus = (lot_secs_int - ct_secs_int);

	int t_minus_hh = (int)(t_minus / 3600);
	int t_minus_mm = (int)((t_minus - (t_minus_hh * 3600)) / 60);
	int t_minus_ss = (int)(t_minus - (t_minus_hh * 3600) - (t_minus_mm * 60));

	char cbuf_hh[2];
	char cbuf_mm[2];
	char cbuf_ss[2];
	char cbuf[256];

	if (abs(t_minus_hh) < 10)sprintf(cbuf_hh, "0%d", t_minus_hh);
	else sprintf(cbuf_hh, "%d", t_minus_hh);

	if (abs(t_minus_mm) < 10)sprintf(cbuf_mm, "0%d", t_minus_mm);
	else sprintf(cbuf_mm, "%d", t_minus_mm);

	if (abs(t_minus_ss) < 10)sprintf(cbuf_ss, "0%d", t_minus_ss);
	else sprintf(cbuf_ss, "%d", t_minus_ss);

	sprintf(cbuf, "T - %s:%s:%s", cbuf_hh, cbuf_mm, cbuf_ss);

	VECTOR3 col = _V(1, 0.9, 0);
	hNote = oapiCreateAnnotation(true, 3, col);
	oapiAnnotationSetPos(hNote, 0.36, 0.89, 0.8, 0.99);
	oapiAnnotationSetText(hNote, cbuf);

	if (lot_status == 1)
	{
		if ((lot_secs_int - ct_secs_int) < 0) return;
		else if ((lot_secs_int - ct_secs_int) < 11)
		{
			{
				if (vehicle_cfg == 0)
				{
					if (ap_status == 0)ap_status = 1;
					else ap_status = 0;
					op_status = 0;
				}
				else if (vehicle_cfg == 1)
				{
					if (op_status == 0)op_status = 1;
					else op_status = 0;
					ap_status = 0;
				}
				apop_status = ap_status + op_status;
			}
			lot_status = 2;
		}
	}
	if (lot_status == 2)
	{
		lot_timer = lot_timer + oapiGetSimStep();
		if (lot_timer > 10)
		{
			sprintf(cbuf, "T - 00:00:00");
			oapiAnnotationSetText(hNote, cbuf);
		}
		if (lot_timer > 11)
		{
			lot_status = 3;
			lot_timer = 0;
			oapiDelAnnotation(hNote);
			return;
		}
	}
}