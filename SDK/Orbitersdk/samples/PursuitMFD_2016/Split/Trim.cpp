// PursuitMFD Subfunctions
#define STRICT

#include "..\Main\PursuitMFD.h"


void SubMFD::SetTrimMode(TRIMODE TrimMode)
{
	if(!oapiIsVessel(htgt)) TrimMode = T_OFF;

	if(inhibate) return;

	if(DockAP && !(DockS && DockT)){
		if(TrimMode != T_OFF) TrimMode=T_ALL;;
		if(TrimGroup != 0) TrimGroup = 7;;
	}

	if(TrimGroup & BFpow(TrimMode)){
		TrimGroup ^= BFpow(TrimMode);	// already set --> remove from group
		LinLvl.data[TrimMode] = 0.0;
//		if(!Hold) 
			vclass->SetAttitudeLinLevel(TrimMode, 0.0);
//		Vth->SetLinLevel(THRUST_TYPE(TrimMode+6), 0.0);
	}
	else TrimGroup |= BFpow(TrimMode);	// assign new linear trim

	if(TrimMode == T_ALL){
		TrimGroup = 7; // all trim are set: b111
//		if(Trim == T_ALL) TrimGroup = 0; // stop all trim b000
	}

	switch(TrimGroup){
		case 1: TrimMode = T_LAT;		break; // b001
		case 2: TrimMode = T_VERT;		break; // b010
		case 4: TrimMode = T_FA;		break; // b100
		case 3: TrimMode = T_LAT_VERT;	break; // b011
		case 6: TrimMode = T_VERT_FA;	break; // b110
		case 5: TrimMode = T_FA_LAT;	break; // b101
		case 7: TrimMode = T_ALL;		break; // b111
		default:TrimMode = T_OFF;		break; // b000
	}

	if(TrimMode == T_OFF){ // no more linear group active
		TrimGroup = 0;
//		LinLvl = NULLVECTOR;
//		if(!Hold) vclass->SetAttitudeLinLevel(NULLVECTOR);
		Vth->NullRotLinLevel();
//		Vth->SetLinLevel(TH_LAT, 0.0);
//		Vth->SetLinLevel(TH_VRT, 0.0);
//		Vth->SetLinLevel(TH_LNG, 0.0);
	}

	Trim = TrimMode;
}

void SubMFD::TrimUpd(VECTOR3 LinVel)
{
	switch(Trim){
		case T_LAT:
		case T_VERT:
		case T_FA:
			SetTrimAxis(Trim, LinVel);
			break;
		case T_ALL:
			SetTrimAxis(T_LAT, LinVel);
			SetTrimAxis(T_VERT, LinVel);
			SetTrimAxis(T_FA, LinVel);
			break;
		case T_LAT_VERT:
			SetTrimAxis(T_LAT, LinVel);
			SetTrimAxis(T_VERT, LinVel);
			break;
		case T_VERT_FA:
			SetTrimAxis(T_VERT, LinVel);
			SetTrimAxis(T_FA, LinVel);
			break;
		case T_FA_LAT:
			SetTrimAxis(T_FA, LinVel);
			SetTrimAxis(T_LAT, LinVel);
			break;

		case T_OFF: LinLvl = NULLVECTOR;
		default: break;
	}
}

double SubMFD::SetTrimAxis(TRIMODE TrimMode, VECTOR3 tgtlinvel)
{
	if(!oapiIsVessel(htgt)){
		DockAP = false;
		SetTrimMode(T_OFF);
//		LinLvl = NULLVECTOR;
//		if(!Hold) vclass->SetAttitudeLinLevel(NULLVECTOR);
		Vth->NullRotLinLevel();
		return 0.0;
//		Vth->SetLinLevel(TH_LAT, 0.0);
//		Vth->SetLinLevel(TH_VRT, 0.0);
//		Vth->SetLinLevel(TH_LNG, 0.0);
	}

	double lvl = 0.0;

	double stab = 1.0; // Improves stability at higher accel.
	if(oapiGetTimeAcceleration() > 10.0)
		stab *= oapiGetTimeAcceleration()/10.0;

	if(fabs(tgtlinvel.data[TrimMode])>0.0005){
		lvl = tgtlinvel.data[TrimMode] * Vth->StructMass / Vth->MaxForce[TrimMode+6] / TIMESTEP / stab; //TIMESTEP

		if(fabs(lvl)>1.0) lvl /= fabs(lvl); // limitation @ level 1.0 max

//		Vth->SetLinLevel(ThType, lvl);
		LinLvl.data[TrimMode] = lvl;
	}

	return lvl;
}

// END OF FILE