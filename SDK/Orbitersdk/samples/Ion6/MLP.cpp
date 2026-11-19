#include "MLP.h"

void MLP::clbkSetClassCaps(FILEHANDLE cfg)
{
	floodlight_tex = oapiRegisterExhaustTexture("Ion_LVs\\Ion6\\Floodlight");
	mlp_mesh = oapiLoadMesh("Ion_LVs\\Ion6\\MLP");
	ClearMeshes(true);
	SetMeshVisibilityMode(AddMesh(mlp_mesh, &_V(0, 0, 0)), MESHVIS_ALWAYS);
	SetSize(MLP_SIZE);
	lv_attach = CreateAttachment(false, LV_POS, _V(0, 1, 0), _V(-1, 0, 0), "I6P", false);
	SetTouchdownPoints(_V(50, -22.409, 0), _V((cos(120 * RAD) * 50), -22.409, (sin(120 * RAD) * 50)), _V((cos(240 * RAD) * 50), -22.409, (sin(240 * RAD) * 50)));
	SetEmptyMass(0.001);
	DefineAnimations();
	AddParticleStream(&loxvent_s1, _V(-2.924, 17.5, 3.796), _V(0, -1, 0), &loxvent_lvl[0]);
	AddParticleStream(&loxvent_s2, _V(-1.828, 57.795, 4.389), _V(0, -1, 0), &loxvent_lvl[1]);
	AddParticleStream(&loxvent_s3, _V(-1.687, 87.178, 6.027), _V(0, -1, 0), &loxvent_lvl[2]);
	AddParticleStream(&flamebucket, _V(0, -15, 9.15), _V(0, 0.242535625036333, 0.970142500145332), &lv_thlvl[0]);
	AddParticleStream(&flamebucket, _V(0, -15, 9.15), _V(0, 0, -1), &lv_thlvl[1]);

	COLOUR4 col_d = { 1,1,0.9,0 };
	COLOUR4 col_s = { 1,1,0.9,0 };
	COLOUR4 col_a = { 0,0,0,0 };
	for (int i = 0; i < 5; i++) {
		floodlight_beam[i].th = NULL;
		floodlight_beam[i].level = &floodlight_status;
		floodlight_beam[i].lpos = &lights_pos[i];
		floodlight_beam[i].ldir = &lights_dir_inv[i];
		floodlight_beam[i].lsize = 200;
		floodlight_beam[i].wsize = 20;
		floodlight_beam[i].modulate = 0;
		floodlight_beam[i].tex = floodlight_tex;
		AddExhaust(&floodlight_beam[i]);

		floodlight[i] = (SpotLight*)AddSpotLight(lights_pos[i], lights_dir[i], 4000, 2e-4, 0, 2e-4, RAD * 20, RAD * 90, col_d, col_s, col_a);
		floodlight[i]->Activate(false);
	}
}

void MLP::clbkPostCreation()
{
	if (autosequence == 0) {
		SetAnimation(anim_swingarms, swingarms_proc);
	}
	else {
		autosequence = 1;
		AutosequenceTimer();
		SetAutoState();
	}
}

void MLP::clbkPreStep(double simt, double simdt, double mjd)
{
	GetThrust();
	if ((lv_thlvl[0] == 1) & (post_liftoff == false)) {
		DetachChild(lv_attach);
		autosequence = 0;
		post_liftoff = true;
	}
	DoAutoFloodlights();
}

void MLP::clbkPostStep(double simt, double simdt, double mjd)
{
	if (autosequence == 1) {
		AutosequenceTimer();
		if ((autosequence_timer <= AUTO_S1_LH2LOAD) & (auto_tanks_full[0] == 0)) {
			DoS1PropLoad(simdt);
		}
		if ((autosequence_timer <= AUTO_S2_LH2LOAD) & (auto_tanks_full[1] == 0)) {
			DoS2PropLoad(simdt);
		}
		if ((autosequence_timer <= AUTO_S3_LH2LOAD) & (auto_tanks_full[2] == 0)) {
			DoS3PropLoad(simdt);
		}
		if ((autosequence_timer <= AUTO_TOPPING_CLOSEOUT) & (loxvent_lvl[0] > 0)) {
			loxvent_lvl[0] = 0;
			loxvent_lvl[1] = 0;
			loxvent_lvl[2] = 0;
			if (oapiGetTimeAcceleration() > 10) {
				oapiSetTimeAcceleration(10);
			}
		}
		if ((autosequence_timer <= 10) & (timeaccel_rset_armed == true)) {
			if (oapiGetTimeAcceleration() > 1) {
				oapiSetTimeAcceleration(1);
			}
			timeaccel_rset_armed = false;
		}
		if ((autosequence_timer <= AUTO_SWINGARMS) & (swingarms_status == CLOSED)) {
			RevertSwingArms();
		}
	}
	MoveSwingArms(simdt);
	if (post_liftoff == true) {
		DoPostLiftoffEffects(simdt);
	}
	//sprintf(oapiDebugString(), "%lf %i %lf", lv_thlvl, (int)post_liftoff, post_liftoff_flamebucket);
}

int MLP::clbkConsumeBufferedKey(DWORD key, bool down, char* kstate)
{
	if (key == COMM_SWINGARMS) {
		if (swingarms_status == OPEN) {}
		else {
			swingarms_status = OPENING;
		}
		return 1;
	}
	return 0;
}

void MLP::clbkSaveState(FILEHANDLE scn)
{
	char cbuf[256];

	VESSEL4::clbkSaveState(scn);

	if (autosequence == 0) {
		sprintf(cbuf, "%i %lf", swingarms_status, swingarms_proc);
		oapiWriteScenario_string(scn, "SWINGARMS", cbuf);
	}

	sprintf(cbuf, "%d %d %d %i", lot_hh, lot_mm, lot_ss, autosequence);
	oapiWriteScenario_string(scn, "LAUNCH_TIME", cbuf);
}

void MLP::clbkLoadStateEx(FILEHANDLE scn, void* status)
{
	char* line;
	while (oapiReadScenario_nextline(scn, line))
	{
		if (!_strnicmp(line, "SWINGARMS", 9)) {
			sscanf(line + 9, "%i %lf", &swingarms_status, &swingarms_proc);
		}
		else if (!_strnicmp(line, "LAUNCH_TIME", 11)) {
			sscanf(line + 11, "%d %d %d %i", &lot_hh, &lot_mm, &lot_ss, &autosequence);
		}
		else {
			ParseScenarioLineEx(line, status);
		}
	}
}

bool MLP::clbkLoadVC(int id)
{
	SetCameraOffset(CAMERA_POS);
	SetCameraDefaultDirection(CAMERA_DIR);
	return true;
}

bool MLP::clbkLoadGenericCockpit()
{
	return false;
}

void MLP::GetThrust()
{
	OBJHANDLE hVes = GetAttachmentStatus(lv_attach);
	VESSEL4* v;
	if (hVes == NULL) {
		lv_thlvl[0] = 0;
		lv_thlvl[1] = 0;
	}
	else {
		v = (VESSEL4*)oapiGetVesselInterface(hVes);
		lv_thlvl[0] = v->GetThrusterGroupLevel(THGROUP_MAIN);
		lv_thlvl[1] = lv_thlvl[0] * lv_thlvl_factor;
	}
}

void MLP::AutosequenceTimer()
{
	double lot_secs = (3600 * lot_hh) + (60 * lot_mm) + (lot_ss);
	double current_mjd = oapiGetSimMJD();
	double ct_secs_frac = current_mjd - (int)(current_mjd);
	double ct_secs = 86400 * ct_secs_frac;
	autosequence_timer = (lot_secs - ct_secs);
	if (autosequence_timer < 0) {
		autosequence_timer += 86400;
	}
	//sprintf(oapiDebugString(), "%lf %d %d %d", autosequence_timer, lot_hh, lot_mm, lot_ss);
}

void MLP::SetAutoState()
{
	OBJHANDLE hVes = GetAttachmentStatus(lv_attach);
	VESSEL4* v;
	v = (VESSEL4*)oapiGetVesselInterface(hVes);
	if (hVes == NULL) {}
	else {
		PROPELLANT_HANDLE prop[9];
		prop[0] = v->GetPropellantHandleByIndex(0);
		prop[1] = v->GetPropellantHandleByIndex(1);
		prop[2] = v->GetPropellantHandleByIndex(2);
		prop[3] = v->GetPropellantHandleByIndex(3);
		prop[4] = v->GetPropellantHandleByIndex(4);
		prop[5] = v->GetPropellantHandleByIndex(5);
		prop[6] = v->GetPropellantHandleByIndex(6);
		prop[7] = v->GetPropellantHandleByIndex(7);
		prop[8] = v->GetPropellantHandleByIndex(8);
		v->SetPropellantMass(prop[0], 0);
		v->SetPropellantMass(prop[1], 0);
		v->SetPropellantMass(prop[2], 0);
		v->SetPropellantMass(prop[3], v->GetPropellantMaxMass(prop[3]));
		v->SetPropellantMass(prop[4], v->GetPropellantMaxMass(prop[4]));
		v->SetPropellantMass(prop[5], v->GetPropellantMaxMass(prop[5]));
		v->SetPropellantMass(prop[6], v->GetPropellantMaxMass(prop[6]));
		v->SetPropellantMass(prop[7], v->GetPropellantMaxMass(prop[7]));
		v->SetPropellantMass(prop[8], v->GetPropellantMaxMass(prop[8]));

		if (autosequence_timer >= AUTO_S1_LH2LOAD) {
			swingarms_proc = 0;
			swingarms_status = CLOSED;
		}

		if ((autosequence_timer < AUTO_S1_LH2LOAD) & (autosequence_timer >= AUTO_S123_LOXTOP)) {
			double lh2_lvl = 0;
			double lox_lvl = 0;
			double lh2_mass = 0;
			double lox_mass = 0;
			double max_propmass = 0;
			double new_propmass = 0;
			if (autosequence_timer > AUTO_S123_LH2TOP) {
				lh2_lvl = 1 - ((autosequence_timer - AUTO_S123_LH2TOP) / (AUTO_S1_LH2LOAD - AUTO_S123_LH2TOP));
			}
			else {
				lh2_lvl = 1;
			}
			if (autosequence_timer <= AUTO_S1_LOXLOAD) {
				lox_lvl = 1 - ((autosequence_timer - AUTO_S123_LOXTOP) / (AUTO_S1_LOXLOAD - AUTO_S123_LOXTOP));
				loxvent_lvl[0] = 1 - ((autosequence_timer - AUTO_S123_LOXTOP) / (AUTO_S1_LOXLOAD - AUTO_S123_LOXTOP));
			}
			lh2_mass = S1_LH2MASS * lh2_lvl;
			lox_mass = S1_LOXMASS * lox_lvl;
			max_propmass = v->GetPropellantMaxMass(prop[0]);
			new_propmass = lh2_mass + lox_mass;
			if (new_propmass > max_propmass) {
				new_propmass = max_propmass;
				loxvent_lvl[0] = 1;
			}
			v->SetPropellantMass(prop[0], new_propmass);
			swingarms_proc = 0;
			swingarms_status = CLOSED;
		}

		if ((autosequence_timer < AUTO_S2_LH2LOAD) & (autosequence_timer >= AUTO_S123_LOXTOP)) {
			double lh2_lvl = 0;
			double lox_lvl = 0;
			double lh2_mass = 0;
			double lox_mass = 0;
			double max_propmass = 0;
			double new_propmass = 0;
			if (autosequence_timer > AUTO_S123_LH2TOP) {
				lh2_lvl = 1 - ((autosequence_timer - AUTO_S123_LH2TOP) / (AUTO_S2_LH2LOAD - AUTO_S123_LH2TOP));
			}
			else {
				lh2_lvl = 1;
			}
			if (autosequence_timer <= AUTO_S2_LOXLOAD) {
				lox_lvl = 1 - ((autosequence_timer - AUTO_S123_LOXTOP) / (AUTO_S2_LOXLOAD - AUTO_S123_LOXTOP));
				loxvent_lvl[1] = 1 - ((autosequence_timer - AUTO_S123_LOXTOP) / (AUTO_S2_LOXLOAD - AUTO_S123_LOXTOP));
			}
			lh2_mass = S2_LH2MASS * lh2_lvl;
			lox_mass = S2_LOXMASS * lox_lvl;
			max_propmass = v->GetPropellantMaxMass(prop[1]);
			new_propmass = lh2_mass + lox_mass;
			if (new_propmass > max_propmass) {
				new_propmass = max_propmass;
				loxvent_lvl[1] = 1;
			}
			v->SetPropellantMass(prop[1], new_propmass);
			swingarms_proc = 0;
			swingarms_status = CLOSED;
		}

		if ((autosequence_timer < AUTO_S3_LH2LOAD) & (autosequence_timer >= AUTO_S123_LOXTOP)) {
			double lh2_lvl = 0;
			double lox_lvl = 0;
			double lh2_mass = 0;
			double lox_mass = 0;
			double max_propmass = 0;
			double new_propmass = 0;
			if (autosequence_timer > AUTO_S123_LH2TOP) {
				lh2_lvl = 1 - ((autosequence_timer - AUTO_S123_LH2TOP) / (AUTO_S3_LH2LOAD - AUTO_S123_LH2TOP));
			}
			else {
				lh2_lvl = 1;
			}
			if (autosequence_timer <= AUTO_S3_LOXLOAD) {
				lox_lvl = 1 - ((autosequence_timer - AUTO_S123_LOXTOP) / (AUTO_S3_LOXLOAD - AUTO_S123_LOXTOP));
				loxvent_lvl[2] = 1 - ((autosequence_timer - AUTO_S123_LOXTOP) / (AUTO_S3_LOXLOAD - AUTO_S123_LOXTOP));
			}
			lh2_mass = S3_LH2MASS * lh2_lvl;
			lox_mass = S3_LOXMASS * lox_lvl;
			max_propmass = v->GetPropellantMaxMass(prop[2]);
			new_propmass = lh2_mass + lox_mass;
			if (new_propmass > max_propmass) {
				new_propmass = max_propmass;
				loxvent_lvl[2] = 1;
			}
			v->SetPropellantMass(prop[2], new_propmass);
			swingarms_proc = 0;
			swingarms_status = CLOSED;
		}

		if ((autosequence_timer < AUTO_S123_LOXTOP) & (autosequence_timer >= AUTO_TOPPING_CLOSEOUT)) {
			v->SetPropellantMass(prop[0], v->GetPropellantMaxMass(prop[0]));
			v->SetPropellantMass(prop[1], v->GetPropellantMaxMass(prop[1]));
			v->SetPropellantMass(prop[2], v->GetPropellantMaxMass(prop[2]));
			loxvent_lvl[0] = 1;
			loxvent_lvl[1] = 1;
			loxvent_lvl[2] = 1;
			swingarms_proc = 0;
			swingarms_status = CLOSED;
		}

		if ((autosequence_timer < AUTO_TOPPING_CLOSEOUT) & (autosequence_timer >= AUTO_SWINGARMS)) {
			v->SetPropellantMass(prop[0], v->GetPropellantMaxMass(prop[0]));
			v->SetPropellantMass(prop[1], v->GetPropellantMaxMass(prop[1]));
			v->SetPropellantMass(prop[2], v->GetPropellantMaxMass(prop[2]));
			swingarms_proc = 0;
			swingarms_status = CLOSED;
		}

		if (autosequence_timer > 10) {
			timeaccel_rset_armed = true;
		}

		if (autosequence_timer < AUTO_SWINGARMS) {
			v->SetPropellantMass(prop[0], v->GetPropellantMaxMass(prop[0]));
			v->SetPropellantMass(prop[1], v->GetPropellantMaxMass(prop[1]));
			v->SetPropellantMass(prop[2], v->GetPropellantMaxMass(prop[2]));
			swingarms_proc = 1;
			swingarms_status = OPEN;
		}
		SetAnimation(anim_swingarms, swingarms_proc);
	}
}

void MLP::DoS1PropLoad(double simdt) {
	double lh2_this_step = 0;
	double lox_this_step = 0;
	if (autosequence_timer > AUTO_S123_LH2TOP) {
		lh2_this_step = (S1_LH2MASS / (AUTO_S1_LH2LOAD - AUTO_S123_LH2TOP)) * simdt;
	}
	if (autosequence_timer <= AUTO_S1_LOXLOAD) {
		lox_this_step = (S1_LOXMASS / (AUTO_S1_LOXLOAD - AUTO_S123_LOXTOP)) * simdt;
		loxvent_lvl[0] = 1 - ((autosequence_timer - AUTO_S123_LOXTOP) / (AUTO_S1_LOXLOAD - AUTO_S123_LOXTOP));
	}
	double max_prop_mass = 0;
	double new_prop_mass = 0;
	OBJHANDLE hVes = GetAttachmentStatus(lv_attach);
	VESSEL4* v;
	if (hVes == NULL) {}
	else {
		v = (VESSEL4*)oapiGetVesselInterface(hVes);
		PROPELLANT_HANDLE prop = v->GetPropellantHandleByIndex(0);
		max_prop_mass = v->GetPropellantMaxMass(prop);
		new_prop_mass = v->GetPropellantMass(prop) + lh2_this_step + lox_this_step;
		if (new_prop_mass >= max_prop_mass) {
			new_prop_mass = max_prop_mass;
			auto_tanks_full[0] = 1;
			loxvent_lvl[0] = 1;
		}
		v->SetPropellantMass(prop, new_prop_mass);
	}
	//sprintf(oapiDebugString(), "%lf", loxvent_lvl[0]);
	//sprintf(oapiDebugString(), "%lf %lf", new_prop_mass, max_prop_mass);
	//sprintf(oapiDebugString(), "%lf %lf", lh2_this_step, lox_this_step);
}

void MLP::DoS2PropLoad(double simdt) {
	double lh2_this_step = 0;
	double lox_this_step = 0;
	if (autosequence_timer > AUTO_S123_LH2TOP) {
		lh2_this_step = (S2_LH2MASS / (AUTO_S2_LH2LOAD - AUTO_S123_LH2TOP)) * simdt;
	}
	if (autosequence_timer <= AUTO_S2_LOXLOAD) {
		lox_this_step = (S2_LOXMASS / (AUTO_S2_LOXLOAD - AUTO_S123_LOXTOP)) * simdt;
		loxvent_lvl[1] = 1 - ((autosequence_timer - AUTO_S123_LOXTOP) / (AUTO_S2_LOXLOAD - AUTO_S123_LOXTOP));
	}
	double max_prop_mass = 0;
	double new_prop_mass = 0;
	OBJHANDLE hVes = GetAttachmentStatus(lv_attach);
	VESSEL4* v;
	if (hVes == NULL) {}
	else {
		v = (VESSEL4*)oapiGetVesselInterface(hVes);
		PROPELLANT_HANDLE prop = v->GetPropellantHandleByIndex(1);
		max_prop_mass = v->GetPropellantMaxMass(prop);
		new_prop_mass = v->GetPropellantMass(prop) + lh2_this_step + lox_this_step;
		if (new_prop_mass >= max_prop_mass) {
			new_prop_mass = max_prop_mass;
			auto_tanks_full[1] = 1;
			loxvent_lvl[1] = 1;
		}
		v->SetPropellantMass(prop, new_prop_mass);
	}
	//sprintf(oapiDebugString(), "%lf", loxvent_lvl[1]);
	//sprintf(oapiDebugString(), "%lf %lf", new_prop_mass, max_prop_mass);
	//sprintf(oapiDebugString(), "%lf %lf", lh2_this_step, lox_this_step);
}

void MLP::DoS3PropLoad(double simdt) {
	double lh2_this_step = 0;
	double lox_this_step = 0;
	if (autosequence_timer > AUTO_S123_LH2TOP) {
		lh2_this_step = (S3_LH2MASS / (AUTO_S3_LH2LOAD - AUTO_S123_LH2TOP)) * simdt;
	}
	if (autosequence_timer <= AUTO_S3_LOXLOAD) {
		lox_this_step = (S3_LOXMASS / (AUTO_S3_LOXLOAD - AUTO_S123_LOXTOP)) * simdt;
		loxvent_lvl[2] = 1 - ((autosequence_timer - AUTO_S123_LOXTOP) / (AUTO_S3_LOXLOAD - AUTO_S123_LOXTOP));
	}
	double max_prop_mass = 0;
	double new_prop_mass = 0;
	OBJHANDLE hVes = GetAttachmentStatus(lv_attach);
	VESSEL4* v;
	if (hVes == NULL) {}
	else {
		v = (VESSEL4*)oapiGetVesselInterface(hVes);
		PROPELLANT_HANDLE prop = v->GetPropellantHandleByIndex(2);
		max_prop_mass = v->GetPropellantMaxMass(prop);
		new_prop_mass = v->GetPropellantMass(prop) + lh2_this_step + lox_this_step;
		if (new_prop_mass >= max_prop_mass) {
			new_prop_mass = max_prop_mass;
			auto_tanks_full[2] = 1;
			loxvent_lvl[2] = 1;
		}
		v->SetPropellantMass(prop, new_prop_mass);
	}
	//sprintf(oapiDebugString(), "%lf", loxvent_lvl[2]);
	//sprintf(oapiDebugString(), "%lf %lf", new_prop_mass, max_prop_mass);
	//sprintf(oapiDebugString(), "%lf %lf", lh2_this_step, lox_this_step);
}

void MLP::DoPostLiftoffEffects(double simdt)
{
	if (post_liftoff_flamebucket <= 15) {
		lv_thlvl[0] = ((post_liftoff_flamebucket - 15) * -1) / 15;
		lv_thlvl[1] = lv_thlvl[0] * lv_thlvl_factor;
		post_liftoff_flamebucket += simdt;
	}
	else {
		lv_thlvl[0] = 0;
		lv_thlvl[1] = 0;
		post_liftoff = false;
	}
}

void MLP::DefineAnimations()
{
	swingarms_proc = 0.0;
	swingarms_status = CLOSED;
	static UINT swingarms_groups_1[3] = { 62,63,64 };
	static UINT swingarms_groups_2[3] = { 66,67,68 };
	static UINT swingarms_groups_3[3] = { 70,71,72 };
	static UINT swingarms_groups_4[3] = { 74,75,76 };
	static UINT swingarms_groups_5[3] = { 78,79,80 };
	static UINT swingarms_groups_6[3] = { 82,83,84 };
	static MGROUP_ROTATE swingarms_comp_1(0, swingarms_groups_1, 3, _V(-8.25, 0, -5.8125), _V(0, 1, 0), (float)(61 * RAD));
	static MGROUP_ROTATE swingarms_comp_2(0, swingarms_groups_2, 3, _V(-8.25, 0, -5.8125), _V(0, 1, 0), (float)(61 * RAD));
	static MGROUP_ROTATE swingarms_comp_3(0, swingarms_groups_3, 3, _V(-8.25, 0, -5.8125), _V(0, 1, 0), (float)(61 * RAD));
	static MGROUP_ROTATE swingarms_comp_4(0, swingarms_groups_4, 3, _V(-8.25, 0, -5.8125), _V(0, 1, 0), (float)(61 * RAD));
	static MGROUP_ROTATE swingarms_comp_5(0, swingarms_groups_5, 3, _V(-8.25, 0, -5.8125), _V(0, 1, 0), (float)(61 * RAD));
	static MGROUP_ROTATE swingarms_comp_6(0, swingarms_groups_6, 3, _V(-8.25, 0, -5.8125), _V(0, 1, 0), (float)(61 * RAD));
	anim_swingarms = CreateAnimation(0);
	AddAnimationComponent(anim_swingarms, 0, 0.375, &swingarms_comp_1);
	AddAnimationComponent(anim_swingarms, 0.625, 1, &swingarms_comp_2);
	AddAnimationComponent(anim_swingarms, 0, 0.375, &swingarms_comp_3);
	AddAnimationComponent(anim_swingarms, 0.625, 1, &swingarms_comp_4);
	AddAnimationComponent(anim_swingarms, 0, 0.375, &swingarms_comp_5);
	AddAnimationComponent(anim_swingarms, 0.625, 1, &swingarms_comp_6);
}

void MLP::ActivateSwingArms(DoorStatus action)
{
	swingarms_status = action;
}

void MLP::RevertSwingArms(void)
{
	ActivateSwingArms((swingarms_status == CLOSED || swingarms_status == CLOSING) ?
		OPENING : CLOSING);
}

void MLP::MoveSwingArms(double simdt)
{
	if (swingarms_status >= CLOSING) {
		double da = simdt * 0.125;
		if (swingarms_status == CLOSING) {
			if (swingarms_proc > 0.0) swingarms_proc = max(0.0, swingarms_proc - da);
			else                swingarms_status = CLOSED;
		}
		else {
			if (swingarms_proc < 1.0) swingarms_proc = min(1.0, swingarms_proc + da);
			else                swingarms_status = OPEN;
		}
		SetAnimation(anim_swingarms, swingarms_proc);
	}
}

void MLP::DoAutoFloodlights()
{
	double sun_angle = GetSunAngle();
	if (sun_angle > on_off_sunangle) {
		if (floodlight_status == 1) {
			floodlight_status = 0;
			for (int i = 0; i < 5; i++) {
				floodlight[i]->Activate(false);
			}
		}
	}
	else {
		if (floodlight_status == 0) {
			floodlight_status = 1;
			for (int i = 0; i < 5; i++) {
				floodlight[i]->Activate(true);
			}
		}
	}
}

double MLP::GetSunAngle()
{
	VECTOR3 rsun;
	VECTOR3 rsunloc;
	OBJHANDLE hSun = oapiGetObjectByIndex(0);
	oapiGetGlobalPos(hSun, &rsun);
	Global2Local(rsun, rsunloc);

	double sun_azimuth = atan2(-rsunloc.z, rsunloc.x); // current angle of earth on ZY plane relative to z axis
	sun_azimuth = sun_azimuth - PI05 - PI;
	if (sun_azimuth < 0) sun_azimuth = sun_azimuth + (2 * PI); // sanity check - angle always positive

	double sunangle;
	sunangle = atan2(-((rsunloc.z * cos(sun_azimuth)) + ((rsunloc.x * sin(sun_azimuth)))), rsunloc.y); // current angle of earth on X(YZ) plane relative to X axis
	sunangle = sunangle - PI05 - PI;
	if (sunangle < 0) sunangle = sunangle + (2 * PI); // sanity check - angle always positive

	if (sunangle * DEG > 180) {
		sunangle = 0;
	}
	else if (sunangle * DEG > 90) {
		sunangle -= 90 * RAD;
	}

	//sprintf(oapiDebugString(), "sunangle %lf", sunangle * DEG);

	return sunangle * DEG;
}