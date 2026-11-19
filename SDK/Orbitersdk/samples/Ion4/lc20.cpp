#include "lc20.h"

void lc20::clbkSetClassCaps(FILEHANDLE cfg)
{
	lc20_mesh = oapiLoadMesh("Ion_LVs\\Ion4\\lc20");
	ClearMeshes(true);
	SetMeshVisibilityMode(AddMesh(lc20_mesh, &_V(0, 0, 0)), MESHVIS_ALWAYS);
	SetSize(LC20_SIZE);
	lv_attach = CreateAttachment(false, LV_POS, _V(0, 1, 0), _V(0, 0, 1), "I4P", false);
	SetTouchdownPoints(_V(50, -0.1, 0), _V((cos(120 * RAD) * 50), -0.1, (sin(120 * RAD) * 50)), _V((cos(240 * RAD) * 50), -0.1, (sin(240 * RAD) * 50)));
	SetEmptyMass(0.001);
	DefineAnimations();
	AddParticleStream(&loxvent_s1, _V(-1, 21.67, 16), _V(0, 0, 1), &loxvent_lvl[0]);
	AddParticleStream(&loxvent_s1, _V(1, 21.67, 16), _V(0, 0, 1), &loxvent_lvl[0]);
	AddParticleStream(&loxvent_s2, _V(-1, 60.67, 16), _V(0, 0, 1), &loxvent_lvl[1]);
	AddParticleStream(&loxvent_s2, _V(1, 60.67, 16), _V(0, 0, 1), &loxvent_lvl[1]);
	AddParticleStream(&flamebucket, _V(20, 5, 16), _V(0.993883734673619, 0.110431526074847, 0), &lv_thlvl);
}

void lc20::clbkPostCreation()
{
	if (autosequence == 0) {
		SetAnimation(anim_lst, lst_proc);
	}
	else {
		autosequence = 1;
		AutosequenceTimer();
		SetAutoState();
	}
}

void lc20::clbkPreStep(double simt, double simdt, double mjd)
{
	GetThrust();
	if ((lv_thlvl == 1) & (post_liftoff == false)) {
		DetachChild(lv_attach);
		autosequence = 0;
		post_liftoff = true;
	}
}

void lc20::clbkPostStep(double simt, double simdt, double mjd)
{
	if (autosequence == 1) {
		AutosequenceTimer();
		if ((autosequence_timer <= AUTO_S1_LH2LOAD) & (auto_tanks_full[0] == 0)) {
			DoS1PropLoad(simdt);
		}
		if ((autosequence_timer <= AUTO_S2_LH2LOAD) & (auto_tanks_full[1] == 0)) {
			DoS2PropLoad(simdt);
		}
		if ((autosequence_timer <= AUTO_TOPPING_CLOSEOUT) & (loxvent_lvl[0] > 0)) {
			loxvent_lvl[0] = 0;
			loxvent_lvl[1] = 0;
			if (oapiGetTimeAcceleration() > 10) {
				oapiSetTimeAcceleration(10);
			}
		}
		if ((autosequence_timer <= AUTO_LST_DOWN) & (lst_status == CLOSED)) {
			RevertLST();
			if (oapiGetTimeAcceleration() > 1) {
				oapiSetTimeAcceleration(1);
			}
		}
	}
	MoveLST(simdt);
	if (post_liftoff == true) {
		DoPostLiftoffEffects(simdt);
	}
	//sprintf(oapiDebugString(), "%lf %i %lf", lv_thlvl, (int)post_liftoff, post_liftoff_flamebucket);
}

int lc20::clbkConsumeBufferedKey(DWORD key, bool down, char* kstate)
{
	if (down == true) { // key down events
		if (key == OAPI_KEY_K && !KEYMOD_SHIFT(kstate) && !KEYMOD_CONTROL(kstate))
		{
			RevertLST();
			return 1;
		}
	}
	return 0;
}

void lc20::clbkSaveState(FILEHANDLE scn)
{
	char cbuf[256];

	VESSEL4::clbkSaveState(scn);

	if (autosequence == 0) {
		sprintf(cbuf, "%i %lf", lst_status, lst_proc);
		oapiWriteScenario_string(scn, "LST", cbuf);
	}

	sprintf(cbuf, "%d %d %d %i", lot_hh, lot_mm, lot_ss, autosequence);
	oapiWriteScenario_string(scn, "LAUNCH_TIME", cbuf);
}

void lc20::clbkLoadStateEx(FILEHANDLE scn, void* status)
{
	char* line;
	while (oapiReadScenario_nextline(scn, line))
	{
		if (!_strnicmp(line, "LST", 3)) {
			sscanf(line + 3, "%i %lf", &lst_status, &lst_proc);
		}
		else if (!_strnicmp(line, "LAUNCH_TIME", 11)) {
			sscanf(line + 11, "%d %d %d %i", &lot_hh, &lot_mm, &lot_ss, &autosequence);
		}
		else {
			ParseScenarioLineEx(line, status);
		}
	}
}

bool lc20::clbkLoadVC(int id)
{
	SetCameraOffset(CAMERA_POS);
	SetCameraDefaultDirection(CAMERA_DIR);
	return true;
}

bool lc20::clbkLoadGenericCockpit()
{
	return false;
}

void lc20::GetThrust()
{
	OBJHANDLE hVes = GetAttachmentStatus(lv_attach);
	VESSEL4* v;
	if (hVes == NULL) {
		lv_thlvl = 0;
	}
	else {
		v = (VESSEL4*)oapiGetVesselInterface(hVes);
		lv_thlvl = v->GetThrusterGroupLevel(THGROUP_MAIN);
	}
}

void lc20::AutosequenceTimer()
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

void lc20::SetAutoState()
{
	OBJHANDLE hVes = GetAttachmentStatus(lv_attach);
	VESSEL4* v;
	v = (VESSEL4*)oapiGetVesselInterface(hVes);
	if (hVes == NULL) {}
	else {
		PROPELLANT_HANDLE prop[4];
		prop[0] = v->GetPropellantHandleByIndex(0);
		prop[1] = v->GetPropellantHandleByIndex(1);
		prop[2] = v->GetPropellantHandleByIndex(2);
		prop[3] = v->GetPropellantHandleByIndex(3);
		v->SetPropellantMass(prop[1], v->GetPropellantMaxMass(prop[1]));
		v->SetPropellantMass(prop[3], v->GetPropellantMaxMass(prop[3]));

		if (autosequence_timer >= AUTO_S1_LH2LOAD) {
			v->SetPropellantMass(prop[0], 0);
			v->SetPropellantMass(prop[2], 0);
			lst_proc = 0;
			lst_status = CLOSED;
		}

		if ((autosequence_timer < AUTO_S1_LH2LOAD) & (autosequence_timer >= AUTO_S1S2_LOXTOP)) {
			double lh2_lvl = 0;
			double lox_lvl = 0;
			double lh2_mass = 0;
			double lox_mass = 0;
			double max_propmass = 0;
			double new_propmass = 0;
			if (autosequence_timer > AUTO_S1S2_LH2TOP) {
				lh2_lvl = 1 - ((autosequence_timer - AUTO_S1S2_LH2TOP) / (AUTO_S1_LH2LOAD - AUTO_S1S2_LH2TOP));
			}
			else {
				lh2_lvl = 1;
			}
			if (autosequence_timer <= AUTO_S1_LOXLOAD) {
				lox_lvl = 1 - ((autosequence_timer - AUTO_S1S2_LOXTOP) / (AUTO_S1_LOXLOAD - AUTO_S1S2_LOXTOP));
				loxvent_lvl[0] = 1 - ((autosequence_timer - AUTO_S1S2_LOXTOP) / (AUTO_S1_LOXLOAD - AUTO_S1S2_LOXTOP));
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
			lst_proc = 0;
			lst_status = CLOSED;
		}

		if ((autosequence_timer < AUTO_S2_LH2LOAD) & (autosequence_timer >= AUTO_S1S2_LOXTOP)) {
			double lh2_lvl = 0;
			double lox_lvl = 0;
			double lh2_mass = 0;
			double lox_mass = 0;
			double max_propmass = 0;
			double new_propmass = 0;
			if (autosequence_timer > AUTO_S1S2_LH2TOP) {
				lh2_lvl = 1 - ((autosequence_timer - AUTO_S1S2_LH2TOP) / (AUTO_S2_LH2LOAD - AUTO_S1S2_LH2TOP));
			}
			else {
				lh2_lvl = 1;
			}
			if (autosequence_timer <= AUTO_S2_LOXLOAD) {
				lox_lvl = 1 - ((autosequence_timer - AUTO_S1S2_LOXTOP) / (AUTO_S2_LOXLOAD - AUTO_S1S2_LOXTOP));
				loxvent_lvl[1] = 1 - ((autosequence_timer - AUTO_S1S2_LOXTOP) / (AUTO_S2_LOXLOAD - AUTO_S1S2_LOXTOP));
			}
			lh2_mass = S2_LH2MASS * lh2_lvl;
			lox_mass = S2_LOXMASS * lox_lvl;
			max_propmass = v->GetPropellantMaxMass(prop[2]);
			new_propmass = lh2_mass + lox_mass;
			if (new_propmass > max_propmass) {
				new_propmass = max_propmass;
				loxvent_lvl[1] = 1;
			}
			v->SetPropellantMass(prop[2], new_propmass);
			lst_proc = 0;
			lst_status = CLOSED;
		}

		if ((autosequence_timer < AUTO_S1S2_LOXTOP) & (autosequence_timer >= AUTO_TOPPING_CLOSEOUT)) {
			v->SetPropellantMass(prop[0], v->GetPropellantMaxMass(prop[0]));
			v->SetPropellantMass(prop[2], v->GetPropellantMaxMass(prop[2]));
			loxvent_lvl[0] = 1;
			loxvent_lvl[1] = 1;
			lst_proc = 0;
			lst_status = CLOSED;
		}

		if ((autosequence_timer < AUTO_TOPPING_CLOSEOUT) & (autosequence_timer >= AUTO_LST_DOWN)) {
			v->SetPropellantMass(prop[0], v->GetPropellantMaxMass(prop[0]));
			v->SetPropellantMass(prop[2], v->GetPropellantMaxMass(prop[2]));
			lst_proc = 0;
			lst_status = CLOSED;
		}

		if ((autosequence_timer < AUTO_LST_DOWN) & (autosequence_timer >= AUTO_LST_DOWN - 20)) {
			v->SetPropellantMass(prop[0], v->GetPropellantMaxMass(prop[0]));
			v->SetPropellantMass(prop[2], v->GetPropellantMaxMass(prop[2]));
			lst_proc = 1 - ((autosequence_timer - (AUTO_LST_DOWN - 20)) / (AUTO_LST_DOWN - (AUTO_LST_DOWN - 20)));
			if (lst_proc < 1) {
				lst_status = OPENING;
			}
			else {
				lst_proc = 1;
				lst_status = OPEN;
			}
		}

		if (autosequence_timer < AUTO_LST_DOWN - 20) {
			v->SetPropellantMass(prop[0], v->GetPropellantMaxMass(prop[0]));
			v->SetPropellantMass(prop[2], v->GetPropellantMaxMass(prop[2]));
			lst_proc = 1;
			lst_status = OPEN;
		}

		SetAnimation(anim_lst, lst_proc);
	}
}

void lc20::DoS1PropLoad(double simdt) {
	double lh2_this_step = 0;
	double lox_this_step = 0;
	if (autosequence_timer > AUTO_S1S2_LH2TOP) {
		lh2_this_step = (S1_LH2MASS / (AUTO_S1_LH2LOAD - AUTO_S1S2_LH2TOP)) * simdt;
	}
	if (autosequence_timer <= AUTO_S1_LOXLOAD) {
		lox_this_step = (S1_LOXMASS / (AUTO_S1_LOXLOAD - AUTO_S1S2_LOXTOP)) * simdt;
		loxvent_lvl[0] = 1 - ((autosequence_timer - AUTO_S1S2_LOXTOP) / (AUTO_S1_LOXLOAD - AUTO_S1S2_LOXTOP));
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

void lc20::DoS2PropLoad(double simdt) {
	double lh2_this_step = 0;
	double lox_this_step = 0;
	if (autosequence_timer > AUTO_S1S2_LH2TOP) {
		lh2_this_step = (S2_LH2MASS / (AUTO_S2_LH2LOAD - AUTO_S1S2_LH2TOP)) * simdt;
	}
	if (autosequence_timer <= AUTO_S2_LOXLOAD) {
		lox_this_step = (S2_LOXMASS / (AUTO_S2_LOXLOAD - AUTO_S1S2_LOXTOP)) * simdt;
		loxvent_lvl[1] = 1 - ((autosequence_timer - AUTO_S1S2_LOXTOP) / (AUTO_S2_LOXLOAD - AUTO_S1S2_LOXTOP));
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
			auto_tanks_full[1] = 1;
			loxvent_lvl[1] = 1;
		}
		v->SetPropellantMass(prop, new_prop_mass);
	}
	//sprintf(oapiDebugString(), "%lf", loxvent_lvl[1]);
	//sprintf(oapiDebugString(), "%lf %lf", new_prop_mass, max_prop_mass);
	//sprintf(oapiDebugString(), "%lf %lf", lh2_this_step, lox_this_step);
}

void lc20::DoPostLiftoffEffects(double simdt)
{
	if (post_liftoff_flamebucket <= 5) {
		lv_thlvl = ((post_liftoff_flamebucket - 5) * -1) / 5;
		post_liftoff_flamebucket += simdt;
	}
	else {
		lv_thlvl = 0;
		post_liftoff = false;
	}
}

void lc20::DefineAnimations()
{
	lst_proc = 0.0;
	lst_status = CLOSED;
	static UINT lst_groups[2] = { 19,20 };
	static MGROUP_ROTATE lst_comp(0, lst_groups, 2, _V(0, 7.75, 12.1), _V(-1, 0, 0), (float)(90 * RAD));
	anim_lst = CreateAnimation(0);
	AddAnimationComponent(anim_lst, 0, 1, &lst_comp);
}

void lc20::ActivateLST(DoorStatus action)
{
	lst_status = action;
}

void lc20::RevertLST(void)
{
	ActivateLST((lst_status == CLOSED || lst_status == CLOSING) ?
		OPENING : CLOSING);
}

void lc20::MoveLST(double simdt)
{
	if (lst_status >= CLOSING) {
		double da = simdt * 0.05;
		if (lst_status == CLOSING) {
			if (lst_proc > 0.0) lst_proc = max(0.0, lst_proc - da);
			else                lst_status = CLOSED;
		}
		else {
			if (lst_proc < 1.0) lst_proc = min(1.0, lst_proc + da);
			else                lst_status = OPEN;
		}
		SetAnimation(anim_lst, lst_proc);
	}
}