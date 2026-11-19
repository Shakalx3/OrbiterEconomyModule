///////////////////////////////////////////////////////////////////////////////
//
//	Sub.cpp
//
///////////////////////////////////////////////////////////////////////////////
#define STRICT

#include "Sub.h"


// ==============================================================
// creator & destructor
SubMFD::SubMFD(char *vname, ThrustDef *vtd, int idx)
{
	mfdindex=idx;

	Initialize(vname, vtd);
}

SubMFD::~SubMFD()
{
//	sprintf(oapiDebugString(),"~SubMFD");
}

// ==============================================================
void SubMFD::Initialize(char *vname, ThrustDef *vtd)
{
	DebugstringTime = oapiGetSysTime();
	debugstring = false;

	vesname	= vname;
	hves	= oapiGetObjectByName(vesname);
	vclass	= oapiGetVesselInterface(hves);

	SimTime = oapiGetSimTime()-TIMESTEP;
	Hold = false;
	AttVel = false;

	Trim = T_OFF;
	TrimGroup = 0x0;

	href = vclass->GetGravityRef(); // GetSurfaceRef();
	oapiGetObjectName(href,refname,NAMEMAX);

	htgt = href;
	strcpy(tgtname,refname);

	Mode = VEL;
	TgtVel = false;

	if(vclass->DockCount()>0) DockS=1;
	DockT=0;

	RefAtt = NULLVECTOR;
	RelAtt = NULLVECTOR;

	Attmethod = YPR;
	ORT = OFF;

	Vel.RefAtt = NULLVECTOR;
	Vel.RelAtt = NULLVECTOR;
	Tgt.RefAtt = NULLVECTOR;
	Tgt.RelAtt = NULLVECTOR;
	Att.RefAtt = NULLVECTOR;
	Att.RelAtt = NULLVECTOR;
	Ent.RefAtt = NULLVECTOR;
	Ent.RelAtt = NULLVECTOR;

	RotMat = _M(0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0);

	Vel.htgt = href;
	strcpy(Vel.tgtname,refname);
	Tgt.htgt = href;
	strcpy(Tgt.tgtname,refname);
	Att.htgt = href;
	strcpy(Att.tgtname,refname);
	Ent.htgt = href;
	strcpy(Ent.tgtname,refname);

	ISSCtrl = false;

	DockAP = false;
	DockXYZ = _V(0.0, 0.0, 500.0);
	TgtDockRpos = NULLVECTOR;
	TgtDockRvel = NULLVECTOR;
	UPosXYZ = NULLVECTOR;
	TPosXYZ = NULLVECTOR;

	NumDock = 0;

	Vth = vtd;

	RotLvl = NULLVECTOR;
	LinLvl = NULLVECTOR;

////// landing//////////////////////////////
	LandPhase = WAIT;
	Pad = 0;
	Lsave.BST = 0.0;
	Lsave.tbrg = 0.0;
	Lsave.vlat = 0.0;
	Lsave.vlon = 0.0;
	Lsave.blon = 0.0;
	Lsave.blat = 0.0;
	Lsave.belv = 0.0;
	Lsave.manloc = false;
	reqdV = 0.0;
	prevclosest = -1.0;
	alignmax = 0.0;

	Lfinal = CLOSE;
	CutoffAlt = 0.20;
//////////////////////// tweaks ///////////////
	StLandAlt = 30.0;
	StLandDis = -25.0;
	StLandVel = 1.0;
	AppDuration = 60.0;
	TbeforeApp = 25.0;
	TbeforeFinal = 15.0;

	forceLandState = false;
	applyBrake = true;
	refreshclosest = false;
	brkctrl = NONE;
////////////////////////
	landing = false;
	for(int i=0; i<Qlngth; i++) quad[i] = 0.0;
	errorBF = 0x0;
	RotaTh = false;
	Hover = true;
	graphstyle = 1;
////////////////////////////////////////////
	AscPhase = WAITING;
	tgtEquInc = 66.0*RAD;
	tgtAlt = 50000.0;
	FlightPath = 0.0;
	Azimuth = 0.0;
	tgtPitch = 0.0;
////////////////////////////////////////////
	TX = false;
////////////////////////////////////////////
	TgtBurnRvel = NULLVECTOR;
	fuelplan = 0.0;
	TgtRvel = NULLVECTOR;
	BurnDate = oapiGetSimMJD()+(200.0/86400);
	countdown = 0.0;
	BurnInput = _V(10.0,0.0,0.0);
	dRVel = _V(10.0,0.0,0.0);
	Beco = 0.0;
	halfBeco = 0.0;
	TRemain = 0.0;
	dtremain = 0.0;
	GO = false;
	stop = false;
	DataInput = true;
	ldRvel = 0.0;
	EjD = 0.0;
	circularize = false;
////////////////////////////////////////////

//	iterPYR = 0;
	iterPYRRate = 0;
	DisplayPYR = NULLVECTOR;
	DisplayPYRRate = NULLVECTOR;
//	for(int i=0; i<ITER; i++) PYRTable[i] = NULLVECTOR;
	for(int i=0; i<ITER; i++) PYRrateTable[i] = NULLVECTOR;
	smoothindic = true;

	RollMode = CTRLROLL;

	gAPstate = 0x0;
	gAPid = 0x0;
	inhibate = false;

	Treshold = 0.0005*RAD;

	PitchYawRoll = NULLVECTOR;
	DeltaPYR = NULLVECTOR;
	PitchYawRollRate = NULLVECTOR;

	qAtt = NULLQUAT;
	qDeltAtt = NULLQUAT;
	qAttRate = NULLQUAT;
//	qAtt_NR = NULLQUAT;
//	qDeltAtt_NR = NULLQUAT;

	ModePage = true;

//	TimeStep(SimTime,oapiGetSimStep(), &gAPstate, &gAPid); // first init
	TimeStep(SimTime,-1.0, &gAPstate, &gAPid); // first init

}

void SubMFD::SetMode(ATTMODE currentmode, ATTMODE nextmode)
{

	switch(currentmode){
		case VEL:
			Vel.RefAtt = RefAtt;
			Vel.RelAtt = RelAtt;
			Vel.htgt = htgt;
			strcpy(Vel.tgtname,tgtname);
			break;
		case REL:
		case DOC:
			Tgt.RefAtt = RefAtt;
			Tgt.RelAtt = RelAtt;
			Tgt.htgt = htgt;
			strcpy(Tgt.tgtname,tgtname);
			break;
		case ATT:
			Att.RefAtt = RefAtt;
			Att.RelAtt = RelAtt;
			Att.htgt = htgt;
			strcpy(Att.tgtname,tgtname);
			break;
		case LAN:
			Ent.RefAtt = RefAtt;
			Ent.RelAtt = RelAtt;
			Ent.htgt = htgt;
			strcpy(Ent.tgtname,tgtname);
			break;
		case DCT:
		case ASC:
			break;
	}

	switch(nextmode){
		case VEL:
			RollMode = KILLROLL;
			RefAtt = Vel.RefAtt;
			RelAtt = Vel.RelAtt;
			htgt = Vel.htgt;
			strcpy(tgtname,Vel.tgtname);
			break;
		case REL:
			RollMode = KILLROLL;
		case DOC:
			RefAtt = Tgt.RefAtt;
			RelAtt = Tgt.RelAtt;
			htgt = Tgt.htgt;
			strcpy(tgtname,Tgt.tgtname);
			break;
		case ATT:
			RollMode = KILLROLL;
			RefAtt = Att.RefAtt;
			RelAtt = Att.RelAtt;
			htgt = Att.htgt;
			strcpy(tgtname,Att.tgtname);
			break;
		case LAN:
			RollMode = CTRLROLL;
			RefAtt = Ent.RefAtt;
			RelAtt = Ent.RelAtt;
			htgt = Ent.htgt;
			strcpy(tgtname,Ent.tgtname);
			break;
		case DCT:
			RollMode = KILLROLL;
			break;
		case ASC:
			RollMode = CTRLROLL;
			break;
	}

	Mode = nextmode;

	SetPYRMethod();

	ModePage = false;
}

void SubMFD::StopHoldAndTrim()
{
	Hold = false;
	Trim = T_OFF;
	TrimGroup = 0;
	GO = false;

	Vth->NullRotLinLevel();
}

void SubMFD::SetHoldMode()
{
	if(inhibate) return;

	if(Hold){
//		RotLvl = NULLVECTOR;
//		if(Trim==T_OFF) vclass->SetAttitudeRotLevel(NULLVECTOR);
		Vth->NullRotLinLevel();
//		Vth->SetRotLevel(ROLL, NULLVECTOR);
//		Vth->SetRotLevel(PITCH, NULLVECTOR);
//		Vth->SetRotLevel(YAW, NULLVECTOR);
	}
	else SetPYRMethod();

// landing case restrictions
	if(Mode==LAN){
		if(Hold){
			if(errorBF & BFpow(12)) errorBF ^= BFpow(12);
			else if(LandPhase>BRAKE && LandPhase<LANDED){ // keep gravity compensation when leaving AP in critical area
//				if(errorBF & BFpow(2)){ // Critical landing area
					errorBF |= BFpow(12); // errmsg = "zeroing vertical velocity";
					return;
//				}
			}
		}
//		if(Hover) vclass->SetThrusterGroupLevel(THGROUP_HOVER, 0.0);
//		else vclass->SetThrusterGroupLevel(THGROUP_MAIN, 0.0);
	}

	Hold = !Hold;
}

void SubMFD::SetSubVelMode(VELMODE vmd)
{
	if(vmd == OFF || vmd == ORT) ORT = OFF;
	else ORT = vmd;
}

void SubMFD::setGraph(){
	graphstyle += 2;
	if(graphstyle>3) graphstyle = 1;
}

void SubMFD::SetRollMode()
{
	switch(RollMode){
		case CTRLROLL:
			RollMode = KILLROLL;
			break;
		case KILLROLL:
			RollMode = NOROLL;
			break;
		case NOROLL:
			RollMode = CTRLROLL;
			break;
	}
}

// ==============================================================
// refresh calculation
void SubMFD::TimeStep(double SimT, double SimDT, DWORD *APstate, DWORD *APid)
{
	if(DebugstringTime < oapiGetSysTime() && debugstring){
		sprintf(oapiDebugString(),"");
		DebugstringTime = oapiGetSysTime();
		debugstring = false;
	}

	char nm[NAMEMAX];
	oapiGetObjectName(htgt,nm,NAMEMAX);
	if(strcmp(nm,tgtname) != 0)	// for safety, in case of object deletion
		if(Lsave.manloc || (errorBF & 3586)){
			htgt = href;
			strcpy(tgtname,refname);
		}
		else Initialize(vesname, Vth);
	oapiGetObjectName(href,nm,NAMEMAX);
	if(strcmp(nm,refname) != 0)
		Initialize(vesname, Vth);


	NoSpherGrav = vclass->NonsphericalGravityEnabled();

	gAPstate = *APstate;
	gAPid = *APid;
	inhibate = (gAPstate && !(gAPid & BFpow(mfdindex))); // other MFD takes control

	if(!Hold && Trim == T_OFF) DockAP = false;

	if(ISSCtrl){
		Hold = false;
		SetTrimMode(T_OFF);
		Vth->NullRotLinLevel();
		DockAP = false;
		ISSControl();
		ISSCtrl = false;
	}

	DT = SimT-SimTime;

	SimTime = SimT;

	vclass->GetGlobalOrientation(CurAtt);
	vclass->GetGlobalPos(GPos);
	vclass->GetRelativePos(href,Rpos);
	vclass->GetRelativeVel(href,Rvel);


	if(!Hold) SetPYRMethod();

	switch(Mode){
		case VEL:	CalcVelocity(); break;
		case REL:	CalcRelative(); break;
		case ATT:	PitchYawRoll = CalcPitchYawRollAngles(); break;
		case DOC:	CalcDockRelative(DT); break;
		case LAN:	Landing(DT); break;
		case DCT:	DirectBurn(); break;
		case ASC:	Ascent(); break;
	}

//////////////////////////////////////////
	qDeltAtt = qAtt;
	qAtt = CalcRotQuaternion();
	qAttRate = _Q(qAtt.q0-qDeltAtt.q0, qAtt.q1-qDeltAtt.q1, qAtt.q2-qDeltAtt.q2, qAtt.q3-qDeltAtt.q3)/DT;

	PitchYawRoll = RotQuat2Euler(qAtt, Attmethod);
	PitchYawRollRate = SmoothingPYRRate(RotQuat2AngVel(qAtt, qAttRate));

	// for display
	DeltaPYR = DisplayPYR;
	DisplayPYR = PitchYawRoll;

	//Calculate the change in PYR for display : high rate value (>PI/sec) are not considered
	DeltaPYR = DisplayPYR - DeltaPYR;
	if(DeltaPYR.data[PITCH] < -PI) DeltaPYR.data[PITCH] += PI2;
	else if(DeltaPYR.data[PITCH] > PI) DeltaPYR.data[PITCH] -= PI2;
	if(DeltaPYR.data[YAW] < -PI) DeltaPYR.data[YAW] += PI2;
	else if(DeltaPYR.data[YAW] > PI) DeltaPYR.data[YAW] -= PI2;
	if(DeltaPYR.data[ROLL] < -PI) DeltaPYR.data[ROLL] += PI2;
	else if(DeltaPYR.data[ROLL] > PI) DeltaPYR.data[ROLL] -= PI2;
	DisplayPYRRate = DeltaPYR/DT;
//////////////////////////////////////////

	int dockevent = Vth->Docked;
	if(dockevent > NumDock){ // detect docking event
		Hold = false;
		SetTrimMode(T_OFF);
		Vth->NullRotLinLevel();
		DockAP = false;
	}
//	if(dockevent < NumDock){ // detect undocking event
//	}
	NumDock = dockevent;


	if(oapiGetSysTime()<TIMESTEP || smoothindic) return; // first call or complete PYR change

	if(vclass->GetFlightStatus() & BFpow(0)) StopHoldAndTrim(); //stop AP if landed

	Vth->rotlinratio = 1.0;
	if(Hold && Trim!=T_OFF) Vth->rotlinratio = 0.5;

	RotLvl = NULLVECTOR;
	if(Hold) RotLvl = HoldAttitude(DT);

	LinLvl = NULLVECTOR;
	if(Mode==DOC) DockingTrim();
	else TrimUpd(TgtRvel);


	if(Hold || Trim!=T_OFF) Vth->SetRotLinLevel(RotLvl, LinLvl);
//	else if(Hold) Vth->SetRotLevel(RotLvl);
//	else if(Trim!=T_OFF) Vth->SetLinLevel(LinLvl);


//sprintf(oapiDebugString(),"(%.3f)   RotLvl:%.3f %.3f %.3f    LinLvl:%.3f %.3f %.3f   rotlinratio:%.3f", oapiGetSimTime(), RotLvl, LinLvl, Vth->rotlinratio);

/****************************************************************************************

sprintf(oapiDebugString(),"version: %i",oapiGetOrbiterVersion());

/****************************************************************************************/
}

bool SubMFD::waitsmooth(int iteroffset){

	SetPYRMethod();

//	for(int i=0; i<ITER; i++) PYRTable[i] = NULLVECTOR;
	for(int i=0; i<ITER; i++) PYRrateTable[i] = NULLVECTOR;
//	iterPYR = iteroffset;
	iterPYRRate = iteroffset;
	if(Hold) smoothindic = true;
	return smoothindic;
}
/*
VECTOR3 SubMFD::SmoothingPYR(VECTOR3 PYR){

	if(PYR.data[PITCH] < 0.0) PYR.data[PITCH] += PI2;
	if(PYR.data[YAW] < 0.0)   PYR.data[YAW]   += PI2;
	if(PYR.data[ROLL] < 0.0)  PYR.data[ROLL]  += PI2;

	if(iterPYR>=ITER){ iterPYR=0; smoothindic=false;}
	PYRTable[iterPYR] = PYR;
	VECTOR3 temp = NULLVECTOR;
	for(int i=0; i<ITER; i++){
		temp.data[PITCH] += PYRTable[i].data[PITCH];
		temp.data[YAW]   += PYRTable[i].data[YAW];
		temp.data[ROLL]  += PYRTable[i].data[ROLL];
	}
	VECTOR3 med = temp/ITER;

//	VECTOR3 med1 = med;
//	VECTOR3 tmp = temp;

	double tresholdfactor = 2.5;
	// data possibly out of range
	if(fabs(PYR.data[PITCH]) > tresholdfactor*fabs(med.data[PITCH])){
		temp.data[PITCH] -= PYR.data[PITCH];
		med.data[PITCH] = temp.data[PITCH]/(ITER-1);
	}
	if(fabs(PYR.data[YAW]) > tresholdfactor*fabs(med.data[YAW])){
		temp.data[YAW] -= PYR.data[YAW];
		med.data[YAW] = temp.data[YAW]/(ITER-1);
	}
	if(fabs(PYR.data[ROLL]) > tresholdfactor*fabs(med.data[ROLL])){
		temp.data[ROLL] -= PYR.data[ROLL];
		med.data[ROLL] = temp.data[ROLL]/(ITER-1);
	}

//sprintf(oapiDebugString(),"iterPYR: %i  PYR: %.3f %.3f %.3f  med1: %.3f %.3f %.3f  med: %.3f %.3f %.3f  tmp: %.3f %.3f %.3f  temp: %.3f %.3f %.3f",iterPYR, PYR*DEG, med1*DEG, med*DEG, tmp*DEG, temp*DEG);
//oapiSetPause(smoothindic);

//sprintf(oapiDebugString(),"iterPYR: %i  PYR: %.3f %.3f %.3f  med1: %.3f %.3f %.3f  med: %.3f %.3f %.3f  tmp: %.3f %.3f %.3f  temp: %.3f %.3f %.3f",iterPYR, PYR*DEG, med1*DEG, med*DEG, tmp*DEG, temp*DEG);


	iterPYR++;

	if(med.data[PITCH] > PI) med.data[PITCH] -= PI2;
	if(med.data[YAW] > PI)   med.data[YAW]   -= PI2;
	if(med.data[ROLL] > PI)  med.data[ROLL]  -= PI2;

	return med;
}
*/
VECTOR3 SubMFD::SmoothingPYRRate(VECTOR3 PYRrate){

	if(iterPYRRate>=ITER){ iterPYRRate=0; smoothindic=false;}

	// only active when holding attitude & not in Landing Mode or disable
	if(!Hold || Mode==LAN || ITER<2){
		iterPYRRate++;
		return PYRrate;
	}

	PYRrateTable[iterPYRRate] = PYRrate;
	VECTOR3 temp = NULLVECTOR;
	for(int i=0; i<ITER; i++){
		temp.data[PITCH] += PYRrateTable[i].data[PITCH];
		temp.data[YAW]   += PYRrateTable[i].data[YAW];
		temp.data[ROLL]  += PYRrateTable[i].data[ROLL];
	}

	VECTOR3 med = temp/ITER;

	double tresholdfactor = 2.5;
	// data possibly out of range
	if(fabs(PYRrate.data[PITCH]) > tresholdfactor*fabs(med.data[PITCH])){
		temp.data[PITCH] -= PYRrate.data[PITCH];
		med.data[PITCH] = temp.data[PITCH]/(ITER-1);
	}
	if(fabs(PYRrate.data[YAW]) > tresholdfactor*fabs(med.data[YAW])){
		temp.data[YAW] -= PYRrate.data[YAW];
		med.data[YAW] = temp.data[YAW]/(ITER-1);
	}
	if(fabs(PYRrate.data[ROLL]) > tresholdfactor*fabs(med.data[ROLL])){
		temp.data[ROLL] -= PYRrate.data[ROLL];
		med.data[ROLL] = temp.data[ROLL]/(ITER-1);
	}

	iterPYRRate++;

	return med;
}

// END OF FILE