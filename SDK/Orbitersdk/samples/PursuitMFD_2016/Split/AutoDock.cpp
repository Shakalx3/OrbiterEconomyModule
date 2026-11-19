// PursuitMFD Subfunctions
#define STRICT

#include "..\Main\PursuitMFD.h"


void SubMFD::SetDockAP()
{
	if(inhibate) return;

	if(!oapiIsVessel(htgt)){// || !DockS || !DockT){
		DockAP = false;
		return;
	}

	DockAP = !DockAP;
	if(DockAP){
		Hold = true;
		SetPYRMethod(); // <-- change trim method
		SetTrimMode(T_ALL);
	}
	else{
		Hold = false;
		SetTrimMode(T_OFF);
//		vclass->SetAttitudeRotLevel(NULLVECTOR);
//		vclass->SetAttitudeLinLevel(NULLVECTOR);
		Vth->NullRotLinLevel();
//		Vth->SetRotLevel(ROLL, NULLVECTOR);
//		Vth->SetRotLevel(PITCH, NULLVECTOR);
//		Vth->SetRotLevel(YAW, NULLVECTOR);
	}
}

VECTOR3 SubMFD::DockingTrim()
{
	double time = 10.0;

	VECTOR3 vel = NULLVECTOR;

	if(DockAP && !(DockS && DockT)){ // nullify TgtRvel
		if(!Hold || RadialVel<0.005){ // stop Trim but kill rot
			Hold = false;
			SetTrimMode(T_OFF);
//			vclass->SetAttitudeRotLevel(NULLVECTOR);
//			vclass->SetAttitudeLinLevel(NULLVECTOR);
			Vth->NullRotLinLevel();
			DockAP = false;
		}else TrimUpd(TgtRvel);

		return TgtRvel;
	}

//	if(length(PitchYawRollRate)<1.1*RPM_PITCH_RATE){

		for(int axis=0; axis<3; axis++){
			double maxacc = Vth->MaxForce[(THRUST_TYPE)(axis+3)]/Vth->StructMass;;
			Vth->MaxLinRate.data[axis] = min(maxacc*time,Vth->MaxLinRate.data[axis]);
			double maxvel = Vth->MaxLinRate.data[axis];
			double maxpos = maxvel*time;

			double deltapos = TgtDockRpos.data[axis];
			double tgtvel = deltapos/time;

//			if(length(PitchYawRollRate)<1.1*RPM_PITCH_RATE)
				maxvel = max(maxvel,maxvel*floor(sqrt(fabs(deltapos/(2.0*maxpos))))); //////

			if (fabs(tgtvel)>maxvel || fabs(deltapos)>maxpos)
				tgtvel = maxvel*sign(deltapos);

			vel.data[axis] = tgtvel + TgtDockRvel.data[axis];

//		}
	}

	TrimUpd(vel);

	return vel;
}

// END OF FILE