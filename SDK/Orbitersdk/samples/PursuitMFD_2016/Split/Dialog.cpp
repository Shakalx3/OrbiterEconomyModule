// PursuitMFD Subfunctions
#define STRICT

#include "..\Main\PursuitMFD.h"

#include "..\CJP\orbitersdk_ext_cjp.h"


// =============================================================================
// local dialog functions
bool commandMenuResponseOrb(void *menuPtr, unsigned int menuIndex, char *str, void *userPtr)
{
	PursuitMFD *mfd=(PursuitMFD *)userPtr;
/*
	if(strlen(str)==1){
			if(strcmp(str,"c")==0 || strcmp(str,"C")==0){
			mfd->config = true;
			mfd->InvalidateButtons();
			return true;
		}
			if(strcmp(str,"o")==0 || strcmp(str,"O")==0){
			mfd->ConsumeKeyBuffered(OAPI_KEY_O);
			return true;
		}
	}
*/
//	if(strcmp(str,"x")==0 || strcmp(str,"X")==0)
	if(strlen(str)<1)
		str = oapiGetFocusInterface()->GetName(); // set focus

	return mfd->setMFDFocus(oapiGetObjectByName(str));
}

bool commandMenuResponseInput(void *id, char *str, void *userPtr)
{
	PursuitMFD *mfd=(PursuitMFD *)userPtr;

/*
	if(strlen(str)==1){
//			if(strcmp(str,"c")==0 || strcmp(str,"C")==0){
//			mfd->config = true;
//			mfd->InvalidateButtons();
//			return true;
//		}
			if(strcmp(str,"o")==0 || strcmp(str,"O")==0){
			mfd->ConsumeKeyBuffered(OAPI_KEY_O);
			return true;
		}
//			if(strcmp(str,"p")==0 || strcmp(str,"P")==0){
//			mfd->ConsumeKeyBuffered(OAPI_KEY_P);
//			return false;
//		}
	}
*/
//	if(strcmp(str,"x")==0 || strcmp(str,"X")==0)

	if(strlen(str)<1)
		str = oapiGetFocusInterface()->GetName(); // set focus

	return mfd->setMFDFocus(oapiGetObjectByName(str));
}

void PursuitMFD::setCommand(INPUT_MODE style)
{
	switch(style) {
		case USER: // SDK style
			oapiOpenInputBox("Select remote vessel( empty line to focus )",
//			oapiOpenInputBox("Select Remote ( 'x' to focus )",
				commandMenuResponseInput, 0, 20, (void *) this);
			return;

		case ORBI: // OrbitMFD style
			oapiTargetPopup(this, "PursuitMFD: remote vessel ( empty line to focus )",
//			oapiTargetPopup(this, "PursuitMFD: Remote ( 'x' to focus )",
				commandMenuResponseOrb, pV->GetGravityRef(), 0);
			return;
	}
}

// ==============================================================
// Sub dialog functions (must be before their call)
bool refMenuResponseOrb(void *menuPtr, unsigned int menumfdindex, char *name, void *userPtr)
{
	SubMFD *sub=(SubMFD *)userPtr;

	return sub->setReference(name);
}

bool refMenuResponseInput(void *id, char *name, void *userPtr)
{
	SubMFD *sub=(SubMFD *)userPtr;

	return sub->setReference(name);
}

bool tgtMenuResponseOrb(void *menuPtr, unsigned int menumfdindex, char *name, void *userPtr)
{
	SubMFD *sub=(SubMFD *)userPtr;

	return sub->setTarget(name);
}

bool tgtMenuResponseInput(void *id, char *name, void *userPtr)
{
	SubMFD *sub=(SubMFD *)userPtr;

	return sub->setTarget(name);
}

bool baseMenuResponseInput(void *id, char *name, void *userPtr)
{
	SubMFD *sub=(SubMFD *)userPtr;

	return sub->setBase(name,false);
}
bool cbSelectPad(void *id, char *str, void *data)
{
	SubMFD *sub=(SubMFD *)data;

	int nd= atoi(str);
	if(nd == 0) sscanf (str, "P%i", &nd);

	return sub->SetPad(nd,false);
}
bool locMenuResponseInput(void *id, char *str, void *data)
{
	SubMFD *sub=(SubMFD *)data;

	int format = 0;
	double lon=0.0, lat=0.0;
	int length = strlen(str);
	int kpos = -1;

	kpos = strcspn(str,"eE");
	if(kpos != length){
		lon = atof(str+1+kpos);
		format = 1;
	}
	kpos = strcspn(str,"wW");
	if(kpos != length){
		lon = -atof(str+1+kpos);
		format = 2;
	}

	kpos = strcspn(str,"nN");
	if(kpos != length){
		lat = atof(str+1+kpos);
		format = 3;
	}
	kpos = strcspn(str,"sS");
	if(kpos != length){
		lat = -atof(str+1+kpos);
		format = 4;
	}

	if(format<1){
		sscanf(str, "%lf %lf", &lon, &lat);
		if(lon>180.0) lon -= 180.0;
	}

	return (sub->setManLoc(lon,lat,false));
}
bool elevMenuResponseInput(void *id, char *str, void *data)
{
	// This allows a space to represent a decimal point
	for (int i = 0; i < (int)strlen(str); i++) {
		if(str[i] == ' ') {
			str[i] = '.';
			break;
		}
	}
	return (((SubMFD *)data)->setElevation(atof(str), false));
}

bool cutoffMenuResponseInput(void *id, char *str, void *data)
{
	// This allows a space to represent a decimal point
	for (int i = 0; i < (int)strlen(str); i++) {
		if(str[i] == ' ') {
			str[i] = '.';
			break;
		}
	}
	return (((SubMFD *)data)->setCutOffAlt(atof(str), false));
}

bool cbSetRelAttPitch(void *id, char *str, void *data)
{
	// This allows a space to represent a decimal point
	for (int i = 0; i < (int)strlen(str); i++) {
		if(str[i] == 'P' || str[i] == 'Y' || str[i] == 'R') str[i] = '0';
		if(str[i] == ' ') {
			str[i] = '.';
			break;
		}
	}
	return (((SubMFD *)data)->SetRelAttitude(atof(str), PITCH, false));
}
bool cbSetRelAttYaw(void *id, char *str, void *data)
{
	// This allows a space to represent a decimal point
	for (int i = 0; i < (int)strlen(str); i++) {
		if(str[i] == 'P' || str[i] == 'Y' || str[i] == 'R') str[i] = '0';
		if(str[i] == ' ') {
			str[i] = '.';
			break;
		}
	}
	return (((SubMFD *)data)->SetRelAttitude(atof(str), YAW, false));
}
bool cbSetRelAttRoll(void *id, char *str, void *data)
{
	// This allows a space to represent a decimal point
	for (int i = 0; i < (int)strlen(str); i++) {
		if(str[i] == 'P' || str[i] == 'Y' || str[i] == 'R') str[i] = '0';
		if(str[i] == ' ') {
			str[i] = '.';
			break;
		}
	}
	return (((SubMFD *)data)->SetRelAttitude(atof(str), ROLL, false));
}

bool cbSetdVf(void *id, char *str, void *data)
{
	// This allows a space to represent a decimal point
	for (int i = 0; i < (int)strlen(str); i++) {
//		if(str[i] == 'P' || str[i] == 'Y' || str[i] == 'R') str[i] = '0';
		if(str[i] == ' ') {
			str[i] = '.';
			break;
		}
	}
	return (((SubMFD *)data)->SetBurnInputVelocity(atof(str), 0, false));
}
bool cbSetdVp(void *id, char *str, void *data)
{
	// This allows a space to represent a decimal point
	for (int i = 0; i < (int)strlen(str); i++) {
//		if(str[i] == 'P' || str[i] == 'Y' || str[i] == 'R') str[i] = '0';
		if(str[i] == ' ') {
			str[i] = '.';
			break;
		}
	}
	return (((SubMFD *)data)->SetBurnInputVelocity(atof(str), 1, false));
}
bool cbSetdVi(void *id, char *str, void *data)
{
	// This allows a space to represent a decimal point
	for (int i = 0; i < (int)strlen(str); i++) {
//		if(str[i] == 'P' || str[i] == 'Y' || str[i] == 'R') str[i] = '0';
		if(str[i] == ' ') {
			str[i] = '.';
			break;
		}
	}
	return (((SubMFD *)data)->SetBurnInputVelocity(atof(str), 2, false));
}

bool cbSetAlt(void *id, char *str, void *data)
{
	// This allows a space to represent a decimal point
	for (int i = 0; i < (int)strlen(str); i++) {
		if(str[i] == ' ') {
			str[i] = '.';
			break;
		}
	}
	return (((SubMFD *)data)->SetTargetAltitude(atof(str), false));
}
bool cbSetInc(void *id, char *str, void *data)
{
	// This allows a space to represent a decimal point
	for (int i = 0; i < (int)strlen(str); i++) {
		if(str[i] == ' ') {
			str[i] = '.';
			break;
		}
	}
	return (((SubMFD *)data)->SetTargetInclination(atof(str), false));
}

bool cbSetT2B(void *id, char *str, void *data)
{
	return (((SubMFD *)data)->SetTimetoBurn(str, false));
}

bool cbSelectDockS(void *id, char *str, void *data)
{
	int nd= atoi(str);
	if(nd == 0) sscanf (str, "U%i", &nd);
	return (((SubMFD *)data)->SelectDock(nd,false));
}
bool cbSelectDockT(void *id, char *str, void *data)
{
	int nd= atoi(str);
	if(nd == 0) sscanf (str, "D%i", &nd);
	return (((SubMFD *)data)->SelectDock(nd,true));
}

bool cbSetDistance(void *id, char *str, void *data)
{
	DWORD set = 0x08; // BFpow(3)

	VECTOR3 XYZ = NULLVECTOR;
	int length = strlen(str);
	int kpos;

	kpos = strcspn(str,"uU");
	if(kpos < 2) set |= BFpow(4); // set current values
	if(kpos < 1) set = 0x1F; // null answer : set current values

	kpos = strcspn(str,"Xx");
	if(kpos != length){
		XYZ.data[0] = atof(str+1+kpos);
		set |= BFpow(0);
	}
	kpos = strcspn(str,"Yy");
	if(kpos != length){
		XYZ.data[1] = atof(str+1+kpos);
		set |= BFpow(1);
	}
	kpos = strcspn(str,"Zz");
	if(kpos != length){
		XYZ.data[2] = atof(str+1+kpos);
		set |= BFpow(2);
	}

	if(set == 0x08){ // numerical answer
		sscanf(str, "%lf %lf %lf", &XYZ.x, &XYZ.y, &XYZ.z); // z distance first
		set = 0x07; // set the 3 terms
	}

	return (((SubMFD *)data)->setDistanceVector(XYZ, set));
}

bool cbSetRotRate(void *id, char *str, void *data)
{
	DWORD set = 0x08; // BFpow(3)

	VECTOR3 XYZ = NULLVECTOR;
	int length = strlen(str);
	int kpos;

	kpos = strcspn(str,"aAqQ");
	if(kpos < 1){
		if(kpos!=length) set = 0x17; // only alphabetic : set default values
		else set = 0x1F; // null answer: set minimum values
	}
	else{
		kpos = strcspn(str,"pP");
		if(kpos != length){
			XYZ.data[0] = atof(str+1+kpos);
			set |= BFpow(0);
		}
		kpos = strcspn(str,"Yy");
		if(kpos != length){
			XYZ.data[1] = atof(str+1+kpos);
			set |= BFpow(1);
		}
		kpos = strcspn(str,"Rr");
		if(kpos != length){
			XYZ.data[2] = atof(str+1+kpos);
			set |= BFpow(2);
		}

		if(set == 0x08){ // numerical answer
			sscanf(str, "%lf %lf %lf", &XYZ.x, &XYZ.y, &XYZ.z);
			set = 0x07; // set the 3 terms
		}
	}

//sprintf(oapiDebugString(),"XYZ: %.3f %.3f %.3f   set:%i",
//	XYZ, set);
	XYZ = _V(fabs(XYZ.x),fabs(XYZ.y),fabs(XYZ.z));

	return (((PursuitMFD *)data)->setRotRate(XYZ, set));

}
bool cbSetLinRate(void *id, char *str, void *data)
{
	DWORD set = 0x08; // BFpow(3)

	VECTOR3 XYZ = NULLVECTOR;
	int length = strlen(str);
	int kpos;

	kpos = strcspn(str,"aAqQ");
	if(kpos < 1){
		if(kpos!=length) set = 0x17; // only alphabetic : set default values
		else set = 0x1F; // null answer: set minimum values
	}
	else{
		kpos = strcspn(str,"xX");
		if(kpos != length){
			XYZ.data[0] = atof(str+1+kpos);
			set |= BFpow(0);
		}
		kpos = strcspn(str,"Yy");
		if(kpos != length){
			XYZ.data[1] = atof(str+1+kpos);
			set |= BFpow(1);
		}
		kpos = strcspn(str,"zZ");
		if(kpos != length){
			XYZ.data[2] = atof(str+1+kpos);
			set |= BFpow(2);
		}

		if(set == 0x08){ // numerical answer
			sscanf(str, "%lf %lf %lf", &XYZ.x, &XYZ.y, &XYZ.z);
			set = 0x07; // set the 3 terms
		}
	}

	return (((PursuitMFD *)data)->setLinRate(XYZ, set));

}

bool cbSetRate(void *id, char *str, void *data)
{
	char *split1 = strtok(str," ");	// get the first part of the string
	char *split2 = strtok(NULL,"\n");		// get the last part of the string

	if(strlen(split1) < 2 && strncmp(split1, "r", 1) == 0)
		cbSetRotRate(id, split2, data);
	else if(strlen(split1) < 4 && strncmp(split1, "rot", 3) == 0)
		cbSetRotRate(id, split2, data);
	else if(strlen(split1) < 2 && strncmp(split1, "l", 1) == 0)
		cbSetLinRate(id, split2, data);
	else if(strlen(split1) < 4 && strncmp(split1, "lin", 3) == 0)
		cbSetLinRate(id, split2, data);
	else return false;

	return true;
}

bool cbconfirmISS(void *id, char *str, void *data)
{
	bool yes = true;// = atoi(str);
//	if(strlen(str)>0) yes = true;
	return (((SubMFD *)data)->SetISSControl(yes));
}

bool cbswapMFD(void *id, char *str, void *data)
{
	int idx= atoi(str);
	if(idx == 0) sscanf (str, "S%i", &idx);
	if(idx<1 || idx>MAXMFDS) return false;
	return (((PursuitMFD *)data)->SwapMFDdialog(idx-1));
}
// ==============================================================
// calls to dialog functions & answers
void SubMFD::setRefName(INPUT_MODE style)
{
	switch(style) {
		case USER: // SDK style
			oapiOpenInputBox("PursuitMFD: Reference", refMenuResponseInput, 0, 20, (void *) this);
			return;

		case ORBI: // OrbitMFD style
			oapiCelBodyPopup(this, "PursuitMFD: Reference", refMenuResponseOrb, 0);
			return;
	}
}

bool SubMFD::setReference(char *name)
{
	OBJHANDLE newRef=NULL;

//	if(strlen(name)==0) newRef=vclass->GetGravityRef();
//	else 
		newRef=oapiGetGbodyByName(name);

	if(newRef==NULL) return false; // Reject non-existent objects

	// Reject non-celestial body objects:
	int objType=oapiGetObjectType(newRef);
	if(objType!=OBJTP_STAR && objType!=OBJTP_PLANET) return false;

	// Assign reference:
	href=newRef;
	oapiGetObjectName(newRef,name,strlen(name));
	strcpy(refname,name);

	waitsmooth();

	return true; // accept selection
}

void SubMFD::setTgtName(INPUT_MODE style)
{
	switch(style) {
		case USER: // SDK style
			oapiOpenInputBox("PursuitMFD: Target", tgtMenuResponseInput, 0, 20, (void *) this);
			return;

		case ORBI: // OrbitMFD style
			if(href!=NULL) oapiTargetPopup(this, "PursuitMFD: Target", tgtMenuResponseOrb, href, 0);
			return;
	}
}

bool SubMFD::setTarget(char *name)
{
	OBJHANDLE newTgt = oapiGetObjectByName(name);

//	if(newTgt==NULL) newTgt = oapiGetBaseByName(href, name);

	if(newTgt==NULL || newTgt==hves) return false; // Reject non-existent objects or focus vessel

	if(oapiIsVessel(newTgt) && Mode==VEL) TgtVel = true;

	// Assign target:
	htgt=newTgt;
	oapiGetObjectName(htgt,name,strlen(name));
	strcpy(tgtname,name);

//	if(oapiGetObjectType(htgt)==OBJTP_VESSEL)
//		if(oapiGetVesselInterface(htgt)->DockCount()>0) DockT=1;
	DockT=0;
	if(Mode == DOC && oapiIsVessel(htgt)) SetDockNav();

	waitsmooth();

	return true; // accept selection
}

///////////////////////////////////////////////////////////////////////
bool SubMFD::setBase(char *name, bool input)
{

	if(input){
		oapiOpenInputBox("PursuitMFD: Target Base (Name or Index) or landed Vessel (Name)", baseMenuResponseInput, 0, 20, (void *) this);
		return false;
	}

	OBJHANDLE newTgt = NULL;

	int nd = atoi(name);
	if(nd == 0){ // name entered
		newTgt = oapiGetBaseByName(href, name);
		if(newTgt == NULL) newTgt = oapiGetObjectByName(name); // not a base so switch to vessel search
	}
	else newTgt = oapiGetBaseByIndex(href, nd-1); // find base by index

	if(newTgt==NULL) return false; // Reject non-existent objects

	// Assign target:
	Lsave.manloc = false;
	htgt=newTgt;
	oapiGetObjectName(htgt,name,NAMEMAX);
	strcpy(tgtname,name);


	LandPhase = WAIT;

	for(int i=0; i<Qlngth; i++) quad[i] = 0.0;

	Pad = 0;

	errorBF = 0x0;

	if(oapiGetObjectType(htgt) == OBJTP_VESSEL){
		VESSEL * V = oapiGetVesselInterface(htgt);
		V->GetEquPos(Lsave.blon, Lsave.blat, Lsave.belv);
		Lsave.belv -= oapiGetSize(href) + V->GetCOG_elev();
	}
	else if(oapiGetObjectType(htgt) == OBJTP_SURFBASE){
		oapiGetBaseEquPos(htgt, &Lsave.blon, &Lsave.blat, &Lsave.belv);
//		Lsave.belv -= oapiGetSize(href); // prior to orbiter Beta 2015
		Lsave.belv = oapiSurfaceElevation(href, Lsave.blon, Lsave.blat); // Orbiter 2016
	}
	
	waitsmooth();

	return true; // accept selection
}
bool SubMFD::SetPad(int nd, bool input)
{

	if(input){
		if(oapiGetObjectType(htgt) == OBJTP_SURFBASE){
			int num = oapiGetBasePadCount(htgt);
			if(num > 0){
				char buf[36];
				sprintf(buf,"Target Pad (01 - %02i), 0 to remove", num);
				oapiOpenInputBox(buf, cbSelectPad, 0, 20, (void *)this);
			}
		}
		return false;
	}

	if(oapiGetObjectType(htgt) == OBJTP_VESSEL){
		Pad = 0;
		return true;
	}

	int num = oapiGetBasePadCount(htgt);

	if(num == 0) return false;

	if(nd>num) nd=num;
	if(nd<0) nd=0;

	Pad = nd;

	oapiGetBasePadEquPos(htgt, Pad-1, &Lsave.blon, &Lsave.blat);
//	Lsave.belv = 0.0; // prior to orbiter Beta 2015
	Lsave.belv = oapiSurfaceElevation(href, Lsave.blon, Lsave.blat); // Orbiter 2016

	waitsmooth();

	return true;
}
bool SubMFD::setManLoc(double lon, double lat, bool input){

	if(input){
		oapiOpenInputBox("PursuitMFD: Target manual location in degrees (lon,lat)", locMenuResponseInput, 0, 20, (void *) this);
		return false;
	}

	Lsave.manloc = true;
	Pad = -1;
	Lsave.blon = lon*RAD;
	Lsave.blat = lat*RAD;
//	Lsave.belv = 0.0; // prior to orbiter Beta 2015
	Lsave.belv = oapiSurfaceElevation(href, Lsave.blon, Lsave.blat); // Orbiter 2016

	waitsmooth();

	return true;
}
bool SubMFD::setElevation(double elev, bool input)
{
	if(input){
		oapiOpenInputBox("PursuitMFD: Set elevation above target (in meters)", elevMenuResponseInput, 0, 20, (void *) this);
		return false;
	}

	Lsave.belv = elev + oapiSurfaceElevation(href, Lsave.blon, Lsave.blat); // Orbiter 2016

	waitsmooth();

	return true;
}

bool SubMFD::setCutOffAlt(double coalt, bool input)
{
	if(input){
		oapiOpenInputBox("PursuitMFD: Set engine cut off altitude (in meters)", cutoffMenuResponseInput, 0, 20, (void *) this);
		return false;
	}

	CutoffAlt = coalt;

	waitsmooth();

	return true;
}
///////////////////////////////////////////////////////////////////////
bool SubMFD::SetRelAttitude(double deg, AXIS Axis, bool input)
{
	if(input){
		switch(Axis){
			case PITCH:
				oapiOpenInputBox("Select Pitch", cbSetRelAttPitch, 0, 20, (void *)this);
			return true;
			case YAW:
				oapiOpenInputBox("Select Yaw", cbSetRelAttYaw, 0, 20, (void *)this);
			return true;
			case ROLL:
				oapiOpenInputBox("Select Roll", cbSetRelAttRoll, 0, 20, (void *)this);
			return true;
		}
	}

	AttVel = true;

	RelAtt.data[Axis] = deg*RAD;

	waitsmooth();

	return true;
}
///////////////////////////////////////////////////////////////////////
bool SubMFD::SetBurnInputVelocity(double vel, int Axis, bool input)
{
	if(BurnDate<oapiGetSimMJD()-(7/86400)) return false;

	if(input){
		switch(Axis){
			case 0:
				oapiOpenInputBox("Select Prograde Velocity", cbSetdVf, 0, 20, (void *)this);
			return true;
			case 1:
				oapiOpenInputBox("Select Plane Change Velocity", cbSetdVp, 0, 20, (void *)this);
			return true;
			case 2:
				oapiOpenInputBox("Select Outward Velocity", cbSetdVi, 0, 20, (void *)this);
			return true;
			default: return false;
		}
	}

	BurnInput.data[Axis] = vel;
	DataInput = true;


	waitsmooth();

	return true;
}
bool SubMFD::SetTimetoBurn(char *str, bool input)
{
	if(input){
		oapiOpenInputBox("Set Time to Burn (M xxx.xx for MJD format)", cbSetT2B, 0, 20, (void *)this);
		return true;
	}

	bool MJD = false;
	// This allows a space to represent a decimal point
	for (int i = 0; i < (int)strlen(str); i++) {
		if(str[i] == 'M'){
			str[i] = '0';
			MJD = true;
		}
		if(str[i] == ' ') {
			str[i] = '.';
			break;
		}
	}

	if(MJD) BurnDate = atof(str);
	else BurnDate = (atof(str)/86400)+oapiGetSimMJD();

	waitsmooth();

	return true;
}
///////////////////////////////////////////////////////////////////////
void SubMFD::SetDockNav()
{
	if(vclass != oapiGetFocusInterface()) return;

	if(vclass->GetNavCount() < 1) return;

	VESSEL *tgt = oapiGetVesselInterface(htgt);
	NAVHANDLE tgtIDS = NULL;
		
	if(DockS && DockT)
		tgtIDS = tgt->GetIDS(tgt->GetDockHandle(DockT-1));

	if(tgtIDS==NULL){
		tgtIDS = tgt->GetTransponder();
/*		tgt->EnableTransponder(true);

		tgt->EnableIDS(tgt->GetDockHandle(DockT-1),true);
		tgtIDS = tgt->GetIDS(tgt->GetDockHandle(DockT-1));
//		tgt->EnableIDS(tgt->GetDockHandle(DockT-1),false);
*/
	}

	if(tgtIDS!=NULL){
		vclass->SetNavChannel(0, oapiGetNavChannel(tgtIDS));
		HUDPARAM hud; hud.HUDdocking.NavIdx = 0;
		oapiSetHUDMode(HUD_DOCKING, &hud);
	}
}
bool SubMFD::SelectDock(int nd, bool target)
{
	if(!oapiIsVessel(htgt)) return false;

	VESSEL *spacecraft;
	if(target) spacecraft = oapiGetVesselInterface(htgt);
	else spacecraft = vclass;

	int num = spacecraft->DockCount();

	switch(num){
		case 0: return false;
//		case 1:
//			if(target) DockT = 1;
//			else DockS = 1;
//			SetDockNav();
//			return true;
		default:
			if (nd<0){
				char buf[37];
				if(target){
					sprintf(buf,"Target Dock (01 - %02i), 0 to remove", num);
					oapiOpenInputBox(buf, cbSelectDockT, 0, 20, (void *)this);
				}
				else{
					sprintf(buf,"Active Dock (01 - %02i), 0 to remove", num);
					oapiOpenInputBox(buf, cbSelectDockS, 0, 20, (void *)this);
				}
				return true;
			}
	}

	if(nd>num) nd=num;
	if(nd<0) nd=0;

	if(target) DockT = nd;
	else DockS = nd;

	SetDockNav();

	waitsmooth();

	return true;
}
///////////////////////////////////////////////////////////////////////
bool SubMFD::setDistanceVector(VECTOR3 xyz, DWORD set)
{
	if(!set){
//		char buf[54] = "set vector XYZ from target dock (format: 0.0 0.0 0.0)";
		char buf[88] = "'X0.0' and/or 'Y0.0' and/or 'Z0.0' or '+x.x +x.x +x.x' , empty or 'U' to set to current";
		oapiOpenInputBox(buf, cbSetDistance, 0, 20, (void *)this);
		return false;
	}
	else{
		for(int i=0; i<3; i++){
			if(set & BFpow(i)){
				if(set & BFpow(4)) xyz.data[i] = TgtRpos.data[i]; // current values
				DockXYZ.data[i] = xyz.data[i];
			}
		}
	}
	return true;
}
///////////////////////////////////////////////////////////////////////
bool PursuitMFD::setRate(VECTOR3 xyz, DWORD set)
{
	if(!set){
		char buf[85] = "Maximum rates (format: set 'r' for rotation or 'l' for linear,  then  x.x  x.x  x.x)";
		oapiOpenInputBox(buf, cbSetRate, 0, 20, (void *)this);
		return false;
	}
	return true;
}

bool PursuitMFD::setRotRate(VECTOR3 xyz, DWORD set)
{
	if(!set){
		char buf[50] = "Maximum rotation rates   (format:  x.x  x.x  x.x)";
		oapiOpenInputBox(buf, cbSetRotRate, 0, 20, (void *)this);
		return false;
	}
	else{
		MFDlnk *ship = this->returnShip(this->current);
		SubMFD * subcurrent = NULL;
		int s;
		for(s=0; s<MAXMFDS; s++){ // check one good MFD
			subcurrent = ship->returnMFD(s);
			if(subcurrent != NULL) break;
		}
		if(s==MAXMFDS) return true; // safety

		if(set & BFpow(4)) xyz = subcurrent->Vth->DefaultRate(true)*DEG; // default values
		for(int i=0; i<3; i++){
			if(set & BFpow(i)){
				if(set & BFpow(3)) xyz.data[i] = min(xyz.data[i],subcurrent->Vth->MaxRotRate.data[i]*DEG); // minimum values
				subcurrent->Vth->MaxRotRate.data[i] = xyz.data[i]*RAD;
			}
		}

//sprintf(oapiDebugString(),"maxrotrate: %.3f %.3f %.3f | xyz: %.3f %.3f %.3f   test:%i",
//	subcurrent->Vth->MaxRotRate*DEG, xyz, set & BFpow(4));

	}

	return true;
}
bool PursuitMFD::setLinRate(VECTOR3 xyz, DWORD set)
{
	if(!set){
		char buf[48] = "Maximum linear rates   (format:  x.x  x.x  x.x)";
		oapiOpenInputBox(buf, cbSetLinRate, 0, 20, (void *)this);
		return false;
	}
	else{
		MFDlnk *ship = this->returnShip(this->current);
		SubMFD * subcurrent = NULL;
		int s;
		for(s=0; s<MAXMFDS; s++){ // check one good MFD
			subcurrent = ship->returnMFD(s);
			if(subcurrent != NULL) break;
		}
		if(s==MAXMFDS) return true; // safety

		if(set & BFpow(4)) xyz = subcurrent->Vth->DefaultRate(false); // default values
		for(int i=0; i<3; i++){
			if(set & BFpow(i)){
				if(set & BFpow(3)) xyz.data[i] = min(xyz.data[i],subcurrent->Vth->MaxLinRate.data[i]); // minimum values
				subcurrent->Vth->MaxLinRate.data[i] = xyz.data[i];
			}
		}
	}
	return true;
}
///////////////////////////////////////////////////////////////////////
bool SubMFD::SetISSControl(int confirm)
{
	if(!confirm) oapiOpenInputBox("Set Z-axis along velocity vector? (press Enter or Escape)", cbconfirmISS, 0, 20, (void *)this);
	else ISSCtrl = !ISSCtrl;
	return ISSCtrl;
}
///////////////////////////////////////////////////////////////////////
bool SubMFD::SetTargetAltitude(double alt, bool input)
{
	if(input){
		oapiOpenInputBox("Set Target Altitude", cbSetAlt, 0, 20, (void *)this);
		return true;
	}

	tgtAlt = alt;

	waitsmooth();

	return true;
}
bool SubMFD::SetTargetInclination(double inc, bool input)
{
	if(input){
		oapiOpenInputBox("Set Target Equatorial Inclination", cbSetInc, 0, 20, (void *)this);
		return true;
	}

	tgtEquInc = inc*RAD;

	waitsmooth();

	return true;
}
///////////////////////////////////////////////////////////////////////
bool PursuitMFD::SwapMFDdialog(int tgtidx)
{
	if(tgtidx<0){
		char buf[38];
		sprintf(buf,"Swap MFD %02i with another MFD (max:%02i)",gMFDid+1,MAXMFDS);
		oapiOpenInputBox(buf, cbswapMFD, 0, 20, (void *)this);
		return false;
	}
	else return MoveMFD(tgtidx,gMFDid);
}

// END OF FILE