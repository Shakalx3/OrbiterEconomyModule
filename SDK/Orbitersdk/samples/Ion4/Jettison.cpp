#define STRICT
#define ORBITER_MODULE

#include "orbitersdk.h"

class Jettison : public VESSEL4 {
public:
	Jettison(OBJHANDLE hVessel, int flightmodel);
	~Jettison();
	void clbkSetClassCaps(FILEHANDLE cfg);
	void clbkPostStep(double simt, double simdt, double mjd);
	void clbkSaveState(FILEHANDLE scn);
	void clbkLoadStateEx(FILEHANDLE scn, void* vs);
private:
	double dyn_pressure;
	double mach_number;
	OBJHANDLE this_vessel;
	bool TIMER_ENABLED = false;
	bool DYNPRESS_ENABLED = false;
	bool GROUNDCONTACT_ENABLED = false;
	double TIMER = 0;
	double DYNPRESS = 0;
	int timer_from_cfg = 0;
};

Jettison::Jettison(OBJHANDLE hVessel, int flightmodel)
	: VESSEL4(hVessel, flightmodel)
{
}

Jettison::~Jettison()
{
}

void Jettison::clbkSetClassCaps(FILEHANDLE cfg)
{
	TIMER_ENABLED = oapiReadItem_int(cfg, "TIMER", timer_from_cfg);
	DYNPRESS_ENABLED = oapiReadItem_float(cfg, "DYN_PRESS", DYNPRESS);
	oapiReadItem_bool(cfg, "GROUNDCONTACT", GROUNDCONTACT_ENABLED);
	TIMER = timer_from_cfg;
}

void Jettison::clbkPostStep(double simt, double simdt, double mjd)
{
	if (DYNPRESS_ENABLED == true) {
		dyn_pressure = GetDynPressure();
		mach_number = GetMachNumber();
		if ((dyn_pressure > DYNPRESS) & (mach_number > 3)) {
			this_vessel = GetHandle();
			oapiDeleteVessel(this_vessel);
		}
	}
	if (GROUNDCONTACT_ENABLED == true) {
		if (GroundContact() == true) {
			this_vessel = GetHandle();
			oapiDeleteVessel(this_vessel);
		}
	}
	if (TIMER_ENABLED == true) {
		TIMER -= simdt;
		if (TIMER <= 0) {
			this_vessel = GetHandle();
			oapiDeleteVessel(this_vessel);
		}
	}
}

void Jettison::clbkSaveState(FILEHANDLE scn)
{
	char cbuf[256];
	VESSEL4::clbkSaveState(scn);
	if (TIMER_ENABLED == true) {
		sprintf(cbuf, "%lf", TIMER);
		oapiWriteScenario_string(scn, "DELETE_TIMER", cbuf);
	}
}

void Jettison::clbkLoadStateEx(FILEHANDLE scn, void* vs)
{
	char* line;
	while (oapiReadScenario_nextline(scn, line)) {
		if (!_strnicmp(line, "DELETE_TIMER", 12)) {
			sscanf(line + 12, "%lf", &TIMER);
		}
		else {
			ParseScenarioLineEx(line, vs);
		}
	}
}

DLLCLBK VESSEL* ovcInit(OBJHANDLE hvessel, int flightmodel)
{
	return new Jettison(hvessel, flightmodel);
}

DLLCLBK void ovcExit(VESSEL* vessel)
{
	if (vessel) delete (Jettison*)vessel;
}
