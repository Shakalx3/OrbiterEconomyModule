///////////////////////////////////////////////////////////////////////////////
//
//	ThrustCtrl.cpp
//
///////////////////////////////////////////////////////////////////////////////
#define STRICT

#include "ThrustCtrl.h"


// ==============================================================
// creator & destructor
ThrustDef::ThrustDef(VESSEL *V)
{
	Ship = V;
	DefRotRate = DefaultRate(true);
	MaxRotRate = DefRotRate;
	MaxRotRate_docked = DefRotRate;
	MaxRotRate_undocked = DefRotRate;
	MaxLinRate = DefaultRate(false);

	numdockedship = 0;

	rotlinratio = 1.0;
	AR01ratio = 0.90;

	UpdateData();
}

ThrustDef::~ThrustDef(){}
// ==============================================================
// local functions
VECTOR3 ThrustDef::DefaultRate(bool rot)
{
	if (!rot) return defaultLinRate; // in meter/sec

	VECTOR3 Maxrate = defaultRotRate;

	if(strncmp(Ship->GetClassNameA(), "UCGO\\Vessels\\UCGOArrowFreighter", 31) == 0){
		Maxrate.x = 2.00;
		Maxrate.y = 2.00;
		Maxrate.z = 2.00;
	}
	else if(strncmp(Ship->GetClassNameA(), "XR5Vanguard", 11) == 0){
		Maxrate.x = 0.75;
		Maxrate.y = 0.75;
		Maxrate.z = 1.00;
	}
	else if(strncmp(Ship->GetClassNameA(), "Shuttle", 7) == 0){
		Maxrate.x = 0.75;
		Maxrate.y = 0.75;
		Maxrate.z = 0.75;
	}
	else if(strncmp(Ship->GetClassNameA(), "JamesCook", 9) == 0){
		Maxrate.x = 0.25;
		Maxrate.y = 0.25;
		Maxrate.z = 0.50;
	}
	else if(strncmp(Ship->GetClassNameA(), "TotalImmersion\\Descartes", 24) == 0){
		Ship->SetISP(150000); // little hack
		Maxrate.x = 1.00;
		Maxrate.y = 1.00;
		Maxrate.z = 2.00;
	}
	else if(strncmp(Ship->GetClassNameA(), "DavidHume", 9) == 0){
		Maxrate.x = 1.00;
		Maxrate.y = 1.00;
		Maxrate.z = 2.00;
	}
	else if(strncmp(Ship->GetClassNameA(), "AMSO\\Apollo", 11) == 0){
		Maxrate.x = 0.75;
		Maxrate.y = 0.75;
		Maxrate.z = 0.75;
	}
	else if(strncmp(Ship->GetClassNameA(), "Daedalus", 8) == 0){
		Maxrate.x = 0.15;
		Maxrate.y = 0.15;
		Maxrate.z = 0.50;
	}
	else if(strncmp(Ship->GetClassNameA(), "Voyager\\Voyager", 15) == 0){
		Maxrate.x = 0.50;
		Maxrate.y = 0.50;
		Maxrate.z = 0.50;
	}
/*	else if(strncmp(Ship->GetClassNameA(), "Ariane6JM\\A6HLV", 15) == 0){
		Maxrate.x = 0.15;
		Maxrate.y = 0.15;
		Maxrate.z = 0.50;
	}*/

	return Maxrate*RAD; // in radian/sec
}

THRUST_TYPE AxisToThruster(VESSEL *ship, int Axis, bool uselin=false)
{

	THRUST_TYPE tht = TH_USER;
/*	VECTOR3 cg = {0,0,0};
	VECTOR3 thref = {0,0,0};
	VECTOR3 thdir = {0,0,0};
	VECTOR3 thcross = {0,0,0};
	THRUSTER_HANDLE th = NULL;

	ship->GetSuperstructureCG(cg);
*/
	switch(Axis)
	{
		case PITCH:
		{
/*			if(uselin)
				for (DWORD k=0; k<ship->GetGroupThrusterCount(THGROUP_ATT_PITCHUP); k++)
				{
					tht = TH_PIT;
					th = ship->GetGroupThruster(THGROUP_ATT_PITCHUP, k);
					ship->GetThrusterDir(th, thdir);
					ship->GetThrusterRef(th, thref);
					thcross = crossp(thdir,thref-cg);
					if (thcross.x < 0){ // need to change group
						tht = TH_VRT;
						break;
					}
				}
			else */
				tht = TH_PIT;
			break;
		}

		case YAW:
		{
/*			if(uselin)
				for (DWORD k=0; k<ship->GetGroupThrusterCount (THGROUP_ATT_YAWLEFT); k++)
				{
					tht = TH_YAW;
					th = ship->GetGroupThruster(THGROUP_ATT_YAWLEFT, k);
					ship->GetThrusterDir(th, thdir);
					ship->GetThrusterRef(th, thref);
					thcross = crossp(thdir,thref-cg);
					if (thcross.y < 0){ // need to change group
						tht = TH_LAT;
						break;
					}
				}
			else */
				tht = TH_YAW;
			break;
		}

		case ROLL:
		{
			tht = TH_ROL;
			break;
		}
	}

	return tht;
}

VECTOR3 ThrustDef::GetThrusterGroupForceDir(THGROUP_TYPE thgt)
{
	THRUSTER_HANDLE th;
	VECTOR3 dir= {0.0,0.0,0.0};
	double force = 0.0;

	int num = Ship->GetGroupThrusterCount(thgt);

	for (int i = 0; i < num; i++)
	{
		th = Ship->GetGroupThruster(thgt, i);
		Ship->GetThrusterDir(th, dir);
		Ship->GetThrusterMax(th, force);
		dir += dir*force;
	}
	normalise(dir);

	return dir;
}

// ================
// PUBLIC INTERFACE
// ================

double ThrustDef::SetAttitude(AXIS Axis, double TargetAttitude, double CurrentAttitude, double RotRate, double AngleTreshold, bool smoothness, double time)
{
// TODO : anglerate smoothness

	if(oapiGetTimeAcceleration() > 100.0) return 0.0;

	double DeltaAngle = TargetAttitude - CurrentAttitude;

	if (fabs(DeltaAngle) < fabs(AngleTreshold)) return 0.0;

	double DeltaRate = 0.0;		// The difference between the desired and the actual rate
	double TgtRate = 0.0;		// Depends on magnitude of DeltaAngle
	double RateDeadband = 0.0;	// Depends on the magnitude of Rate
	double TorqueReq = 0.0;		// Torque required to achieve rate in 1 time step (N.B.)
	double Level=0.0,lvl=0.0;	// Required thrust level [0 - 1]

	double maxtorque = MaxTorque[AxisToThruster(Ship,Axis)];	// full thrust torque
	double InMoment = StructMass * StructPMI.data[Axis];		// inertial momentum
	double maxacc = maxtorque / InMoment;

	MaxRotRate_tmp.data[Axis] = min(maxacc*time, MaxRotRate.data[Axis]);

	double maxrat = MaxRotRate_tmp.data[Axis]; // in radian/sec

	TgtRate = DeltaAngle/time;

	if (fabs(TgtRate)>maxrat || fabs(DeltaAngle)>1.0*maxrat/rotlinratio){
		TgtRate = maxrat*sign(DeltaAngle);
	}
	DeltaRate = RotRate - TgtRate;

	double stab = 1.0; // Improves stability at higher accel.
	if(oapiGetTimeAcceleration() > 1.0)
		stab *= oapiGetTimeAcceleration()/10.0;

	TorqueReq = InMoment * DeltaRate/stab;
	lvl = TorqueReq / maxtorque;

	RateDeadband = 0.1 * fabs(TgtRate);

	if(fabs(lvl)>1.0) lvl /= fabs(lvl); // limitation @ level 1.0 max
//	if (fabs(DeltaAngle) >= RateDeadband)
	if (fabs(DeltaRate) >= RateDeadband)
		Level = lvl;

/*////////////
if(Axis==PITCH)
	sprintf(oapiDebugString(),"DeltaAngle:%.3f  TgtRate:%.3f (%.3f)  RotRate:%.3f  DeltaRate:%.3f (%.3f)  TorqueReq:%.3f (%.3f)  Level:%.3f",
			DeltaAngle*DEG,TgtRate*DEG,maxacc*time*DEG,RotRate*DEG,DeltaRate*DEG,(TgtRate-2*RotRate)*DEG,TorqueReq,maxtorque,Level);
//oapiSetPause(true);
//*///////////

	return Level;
}

double ThrustDef::SetRotRate(AXIS Axis, double TgtRate, double Angvel, double RateNull)
{
	if(oapiGetTimeAcceleration() > 100.0) return 0.0;

	if (fabs(Angvel-TgtRate) < RateNull)// Already nulled?
		return 0.0;						// (yes)

	double stab = 1.0; // Improves stability at higher accel.
	if ( oapiGetTimeAcceleration() > 10.0 )
		stab *= oapiGetTimeAcceleration()/10.0;
	double TorqueReq = StructMass * StructPMI.data[Axis] * (Angvel-TgtRate);
	double Level = TorqueReq/MaxTorque[AxisToThruster(Ship,Axis)]/1.0/stab; // 1sec derivation @ normal time & 10x
	if(fabs(Level)>1.0) Level /= fabs(Level); // limitation @ level 1.0 max

	return Level;
}

VECTOR3 ThrustDef::KillRotLvl(VECTOR3 avel, double Rate_Max){
	VECTOR3 lvl = NULLVECTOR;
	if(length(avel) == 0) Ship->GetAngularVel(avel);
	lvl.data[PITCH]	= SetRotRate(PITCH, 0.0, avel.data[PITCH], Rate_Max);
	lvl.data[YAW]	= SetRotRate(YAW, 0.0, avel.data[YAW], Rate_Max);
	lvl.data[ROLL]	= SetRotRate(ROLL, 0.0, avel.data[ROLL], Rate_Max);
	return lvl;
}

double ThrustDef::KillRotLvl(AXIS Axis, double angvel, double Rate_Max){
	VECTOR3 lvl = NULLVECTOR;
	if(angvel == 0.0){
		VECTOR3 avel = NULLVECTOR;
		Ship->GetAngularVel(avel);
		angvel = avel.data[Axis];
	}
	lvl.data[Axis] = SetRotRate(Axis, 0.0, angvel, Rate_Max);
	return lvl.data[Axis];
}

void ThrustDef::UpdateData()
{
	SetThrusterGroupBF();
	Docked = GetDockingStatus();

	double time = 8.0;// 2.5; // 10.0
	double maxtorque,InMoment,maxacc,maxrat;

	for(int a=0;a<3;a++){
///////
DefRotRate = DefaultRate(true); // bug : pitch DefRotRate not always initialized !!!
///////
		maxtorque = MaxTorque[AxisToThruster(Ship,a)];	// full thrust torque
		InMoment = StructMass * StructPMI.data[a];		// inertial momentum
		maxacc = maxtorque / InMoment;
		maxrat = maxacc*time;
		MaxRotRate.data[a] = min(min(maxrat,DefRotRate.data[a]),MaxRotRate.data[a]);
	}

	if(Docked) MaxRotRate_docked = MaxRotRate;
	else MaxRotRate_undocked = MaxRotRate;

//sprintf(oapiDebugString(),"MaxRotRate_docked:%.3f %.3f %.3f   MaxRotRate_undocked:%.3f %.3f %.3f   MaxRotRate:%.3f %.3f %.3f   maxacc:%.3f   maxrat:%.3f",
//	MaxRotRate_docked*DEG,MaxRotRate_undocked*DEG,MaxRotRate*DEG, maxacc, maxrat*DEG);

}

// return the number of docked vessel & the list if so
int ThrustDef::GetDockingStatus()
//int ThrustDef::GetDockingStatus(double &mass, VECTOR3 &PMI)
{
	int numdocked = 0;

	double totalmass = 0.0;
	VECTOR3 totalmoment = {0.0,0.0,0.0};
	double vmass = 0.0;
	VECTOR3 vmoment = {0.0,0.0,0.0};
	GetStructMoment(Ship, Ship, totalmass, totalmoment);

	OBJHANDLE hother = NULL;

	for (UINT k=0; k<Ship->DockCount(); k++) {
		if(!Ship->DockingStatus(k)) continue;
		numdocked++;
		hother = Ship->GetDockStatus(Ship->GetDockHandle(k));
		GetStructMoment(oapiGetVesselInterface(hother), Ship, vmass, vmoment);
		totalmoment += vmoment;
		totalmass += vmass;
	}

//	GetStructMoment(Ship, Ship, totalmass, totalmoment);
	StructPMI = totalmoment / totalmass;
	StructMass = totalmass;

//sprintf(oapiDebugString(),"totalmoment:%0.3f %0.3f %0.3f  totalmass:%0.3f  StructPMI:%0.3f %0.3f %0.3f", totalmoment, totalmass, StructPMI);

	if(numdocked > numdockedship) // docking event
		MaxRotRate = MaxRotRate_docked;
	else if(numdocked < numdockedship) // undocking event
		MaxRotRate = MaxRotRate_undocked;

	numdockedship = numdocked;

	return numdocked;
}

// register thrusters within their groups (32 thrusters max in each group)
// also registering force & torque of each group
bool ThrustDef::SetThrusterGroupBF()
{
	int num;
	double force[15];
	double torque[15];
	THRUSTER_HANDLE th;
	THGROUP_TYPE thgt;
	VECTOR3 cg= NULLVECTOR;
	VECTOR3 R = NULLVECTOR;

	for(int n=0; n<64; n++){
		BFAtth[n] = 0x0;
		hAtth[n] = NULL;
	}

	Ship->GetSuperstructureCG(cg);

	UINT64 BFth[12]; // bit-fields for thrusters assignment in each group
	for(int b=0; b<12; b++) BFth[b] = 0x0;

	int k=0; int c=0;
	for (DWORD group = 0; group < 15; group++) // scan all groups THGROUP_TYPE
	{
		force[group] = 0.0;
		torque[group] = 0.0;
		thgt = (THGROUP_TYPE)group;
		num = Ship->GetGroupThrusterCount(thgt);

		if(group>2) BFth[group-3] = 0x0;

		if(num == 0){ // safety
			MaxForce[group] = 0.0;
			MaxTorque[group] = 0.0;
		}

		for (int i = 0; i < num; i++) // scan all thrusters of group
		{
			th = Ship->GetGroupThruster(thgt, i);

			force[group] += Ship->GetThrusterMax(th);

			if(group>2){ // only rot & linear thrusters
				Ship->GetThrusterRef(th, R);
				torque[group] += Ship->GetThrusterMax(th) * length(R-cg);
/*VECTOR3 F = NULLVECTOR;
VECTOR3 T = NULLVECTOR;
Ship->GetThrusterMoment(th, F, T);
if(i==0 && group==3) sprintf(oapiDebugString(),"torque:%.3f  R:%.3f %.3f %.3f  cg:%.3f %.3f %.3f  F:%.3f %.3f %.3f  T:%.3f %.3f %.3f", torque[group], R, cg, F, T);
*/
//torque[group] /=8.5;

				for(c=0; c<k; c++) if(th==hAtth[c]) break; // break if known thruster
				if(c==k) k++; // new thruster
				hAtth[c] = th;
				BFAtth[c] |= BFpow(group-3); // assign group to each thruster
				BFth[group-3] |= BFpow(c); // assign thruster to each group
			}
			else{ // only power engine
				MaxForce[group] = force[group];
				MaxTorque[group] = 0.0;
			}
		}
	}

	for(int g = 3; g < 9; g++){ // only rot & linear thrusters
		MaxForce[g] = (force[2*g-3]+force[2*g-2])/2.0;
		MaxTorque[g] = (torque[2*g-3]+torque[2*g-2])/2.0;
	}

	// share thrusters power between rotation & linear
	for(int g = 3; g < 8; g++){ // only rot & linear thrusters
		MaxForce[g] *= rotlinratio;
		MaxTorque[g] *= rotlinratio;
	}

// check if pitch thrusters are coupled with bank thrusters --> AR01 style
	AR01 = (BFth[0] & BFth[4])!=0; // pitch up & roll left

	if(AR01){// share thrusters power between pitch & bank
		AR01ratio = 1.0/(((MaxTorque[3]/StructPMI.data[0])/(MaxTorque[5]/StructPMI.data[2]))+1.0);
		if(AR01ratio>0.99) AR01ratio = 0.90;
		if(AR01ratio<0.50) AR01ratio = 0.50;
		// pitch thrusters
		MaxForce[3] *= AR01ratio; MaxTorque[3] *= AR01ratio;
		// bank thrusters
		MaxForce[5] *= 1.0-AR01ratio; MaxTorque[5] *= 1.0-AR01ratio;
	}
/*
	sprintf(oapiDebugString(),"(%i) %.3f  %.3f  %.3f  %.3f  %.3f  %.3f  %.3f  %.3f  %.3f",
		AR01,MaxForce[0],MaxForce[1],MaxForce[2],MaxForce[3],
		MaxForce[4],MaxForce[5],MaxForce[6],MaxForce[7],MaxForce[8]);
*/
	return (k < 64); // num total attitude thrusters
}

void ThrustDef::NullRotLinLevel()
{
//	Ship->SetAttitudeRotLevel(NULLVECTOR);
//	Ship->SetAttitudeLinLevel(NULLVECTOR);
	for(int t=3; t<15; t++) Ship->SetThrusterGroupLevel(THGROUP_TYPE(t),0.0);
}

int ThrustDef::SetRotLinLevel(VECTOR3 rotlvl, VECTOR3 linlvl)
{
	rotlvl *= rotlinratio;
	linlvl = _V(linlvl.x*rotlinratio, linlvl.y*rotlinratio, linlvl.z);
	if(AR01) rotlvl = _V(rotlvl.x*AR01ratio, rotlvl.y, rotlvl.z*(1.0-AR01ratio));

	THRUSTER_HANDLE th = NULL;
	double lvl = 0.0;

	int t;
	for(t=0; t<64; t++){
		th = hAtth[t];
		if(th==NULL) break;
		lvl = 0.0;
		for(WORD group=0; group<12; group++){
			switch(BFAtth[t] & BFpow(group)){

				case 1: // THGROUP_ATT_PITCHUP
					if(rotlvl.x>0.0)
						lvl += rotlvl.x;
					break;
				case 2: // THGROUP_ATT_PITCHDOWN
					if(rotlvl.x<0.0)
						lvl -= rotlvl.x;
					break;
				case 4: // THGROUP_ATT_YAWLEFT
					if(rotlvl.y>0.0)
						lvl += rotlvl.y;
					break;
				case 8: // THGROUP_ATT_YAWRIGHT
					if(rotlvl.y<0.0)
						lvl -= rotlvl.y;
					break;
				case 16: // THGROUP_ATT_BANKLEFT
					if(rotlvl.z<0.0)
						lvl -= rotlvl.z;
					break;
				case 32: // THGROUP_ATT_BANKRIGHT
					if(rotlvl.z>0.0)
						lvl += rotlvl.z;
					break;
				case 64: // THGROUP_ATT_RIGHT
					if(linlvl.x>0.0)
						lvl += linlvl.x;
					break;
				case 128: // THGROUP_ATT_LEFT
					if(linlvl.x<0.0)
						lvl -= linlvl.x;
					break;
				case 256: // THGROUP_ATT_UP
					if(linlvl.y>0.0)
						lvl += linlvl.y;
					break;
				case 512: // THGROUP_ATT_DOWN
					if(linlvl.y<0.0)
						lvl -= linlvl.y;
					break;
				case 1024: // THGROUP_ATT_FORWARD
					if(linlvl.z>0.0)
						lvl += linlvl.z;
					break;
				case 2048: // THGROUP_ATT_BACK
					if(linlvl.z<0.0)
						lvl -= linlvl.z;
					break;

				default: break; // no relevant
			}
		}
		if(fabs(lvl)>1.0) lvl /= fabs(lvl);
		Ship->SetThrusterLevel(th, lvl);
	}
/*
sprintf(oapiDebugString(),"(%.3f)  t:%i   %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d    rotlvl:%.3f %.3f %.3f    linlvl:%.3f %.3f %.3f   rotlinratio:%.3f", oapiGetSimTime(), t,
	BFAtth[0],BFAtth[1],BFAtth[2],BFAtth[3],BFAtth[4],BFAtth[5],BFAtth[6],BFAtth[7],BFAtth[8],
	BFAtth[9],BFAtth[10],BFAtth[11],BFAtth[12],BFAtth[13],BFAtth[14],BFAtth[15],
	rotlvl, linlvl, rotlinratio);
*/
	return t;
}

int ThrustDef::SetRotLevel(VECTOR3 rotlvl)
{
	if(AR01) rotlvl = _V(rotlvl.x*AR01ratio, rotlvl.y, rotlvl.z*(1.0-AR01ratio));

	THRUSTER_HANDLE th = NULL;
	double lvl = 0.0;

	int t;
	for(t=0; t<64; t++){
		th = hAtth[t];
		if(th==NULL) break;
		lvl = 0.0;
		for(WORD group=0; group<6; group++){
			switch(BFAtth[t] & BFpow(group)){

				case 1: // THGROUP_ATT_PITCHUP
					if(rotlvl.x>0.0)
						lvl += rotlvl.x;
					break;
				case 2: // THGROUP_ATT_PITCHDOWN
					if(rotlvl.x<0.0)
						lvl -= rotlvl.x;
					break;
				case 4: // THGROUP_ATT_YAWLEFT
					if(rotlvl.y>0.0)
						lvl += rotlvl.y;
					break;
				case 8: // THGROUP_ATT_YAWRIGHT
					if(rotlvl.y<0.0)
						lvl -= rotlvl.y;
					break;
				case 16: // THGROUP_ATT_BANKLEFT
					if(rotlvl.z<0.0)
						lvl -= rotlvl.z;
					break;
				case 32: // THGROUP_ATT_BANKRIGHT
					if(rotlvl.z>0.0)
						lvl += rotlvl.z;
					break;

				default: break; // no relevant
			}
		}
		if(fabs(lvl)>1.0) lvl /= fabs(lvl);
		Ship->SetThrusterLevel_SingleStep(th, lvl);
	}

/*
sprintf(oapiDebugString(),"(%.3f)  t:%i   %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d    rotlvl:%.3f %.3f %.3f   rotlinratio:%.3f", oapiGetSimTime(), t,
	BFAtth[0],BFAtth[1],BFAtth[2],BFAtth[3],BFAtth[4],BFAtth[5],BFAtth[6],BFAtth[7],BFAtth[8],
	BFAtth[9],BFAtth[10],BFAtth[11],BFAtth[12],BFAtth[13],BFAtth[14],BFAtth[15],
	rotlvl, rotlinratio);
*/
	return t;
}

int ThrustDef::SetLinLevel(VECTOR3 linlvl)
{
	THRUSTER_HANDLE th = NULL;
	double lvl = 0.0;

	int t;
	for(t=0; t<64; t++){
		th = hAtth[t];
		if(th==NULL) break;
		lvl = 0.0;
		for(WORD group=6; group<12; group++){
			switch(BFAtth[t] & BFpow(group)){

				case 64: // THGROUP_ATT_RIGHT
					if(linlvl.x>0.0)
						lvl += linlvl.x;
					break;
				case 128: // THGROUP_ATT_LEFT
					if(linlvl.x<0.0)
						lvl -= linlvl.x;
					break;
				case 256: // THGROUP_ATT_UP
					if(linlvl.y>0.0)
						lvl += linlvl.y;
					break;
				case 512: // THGROUP_ATT_DOWN
					if(linlvl.y<0.0)
						lvl -= linlvl.y;
					break;
				case 1024: // THGROUP_ATT_FORWARD
					if(linlvl.z>0.0)
						lvl += linlvl.z;
					break;
				case 2048: // THGROUP_ATT_BACK
					if(linlvl.z<0.0)
						lvl -= linlvl.z;
					break;

				default: break; // no relevant
			}
		}
		if(fabs(lvl)>1.0) lvl /= fabs(lvl);
		Ship->SetThrusterLevel(th, lvl);
	}
/*
sprintf(oapiDebugString(),"(%.3f)  t:%i   %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d    linlvl:%.3f %.3f %.3f", oapiGetSimTime(), t,
	BFAtth[0],BFAtth[1],BFAtth[2],BFAtth[3],BFAtth[4],BFAtth[5],BFAtth[6],BFAtth[7],BFAtth[8],
	BFAtth[9],BFAtth[10],BFAtth[11],BFAtth[12],BFAtth[13],BFAtth[14],BFAtth[15],
	linlvl);
*/
	return t;
}

void ThrustDef::GetRotLinLevel(VECTOR3 &currentrotlvl, VECTOR3 &currentlinlvl)
{
	Ship->GetAttitudeRotLevel(currentrotlvl);
	Ship->GetAttitudeLinLevel(currentlinlvl);
/*
	if(fabs(currentrotlvl.x)>1.0) currentrotlvl.x /= fabs(currentrotlvl.x);
	if(fabs(currentrotlvl.y)>1.0) currentrotlvl.y /= fabs(currentrotlvl.y);
	if(fabs(currentrotlvl.z)>1.0) currentrotlvl.z /= fabs(currentrotlvl.z);
	if(fabs(currentlinlvl.x)>1.0) currentlinlvl.x /= fabs(currentlinlvl.x);
	if(fabs(currentlinlvl.y)>1.0) currentlinlvl.y /= fabs(currentlinlvl.y);
	if(fabs(currentlinlvl.z)>1.0) currentlinlvl.z /= fabs(currentlinlvl.z);
*/
}

// END OF FILE