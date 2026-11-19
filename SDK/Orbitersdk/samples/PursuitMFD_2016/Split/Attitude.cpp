// PursuitMFD Subfunctions
#define STRICT

#include "..\Main\PursuitMFD.h"


VECTOR3 SubMFD::HoldAttitude(double DT)
{
	VECTOR3 lvl = NULLVECTOR;

	VECTOR3 CurrentPYR = PitchYawRoll;
	VECTOR3 PYRrate = PitchYawRollRate;
	QUAT qAA = _Q(0.0, 0.0, 0.0, 1.0);

	if(RollMode!=CTRLROLL){
		QUAT qAtt_NR =GetNoRollRotQuat(qAtt);
		CurrentPYR = RotQuat2Euler(qAtt_NR, Attmethod);
		qAA = GetAngleAxisQuat(qAtt_NR);
	}
	else{
		CurrentPYR = PitchYawRoll;
		qAA = GetAngleAxisQuat(qAtt);
	}

	VECTOR3 avel;
	vclass->GetAngularVel(avel);


	if(!ORT && length(_V(PitchYawRoll.data[PITCH], PitchYawRoll.data[YAW], 0.0)) > DB_ANGLE) { // 10.0*RAD    Mode!=LAN && 

		if(Mode==LAN){ // no roll control for big change
			QUAT qAtt_NR =GetNoRollRotQuat(qAtt);
			CurrentPYR = RotQuat2Euler(qAtt_NR, Attmethod);
			qAA = GetAngleAxisQuat(qAtt_NR);
		}

		VECTOR3 maxrot = NULLVECTOR;
		for(int axis=0; axis<3; axis++){ // need to be tweak ?
//			maxrot.data[axis] = min(45.0*RAD, Vth->MaxRotRate.data[axis]*10.0*(0+fabs(qAA.data[axis+1])));
			maxrot.data[axis] = min(90.0*RAD, Vth->MaxRotRate.data[axis]*10.0);
		}
		CurrentPYR = _V(qAA.q1*maxrot.data[PITCH], qAA.q2*maxrot.data[YAW], qAA.q3*maxrot.data[ROLL]);

//sprintf(oapiDebugString(),"qAA:%+.3f %+.3f %+.3f %+.3f  CurrentPYR:%+.3f %+.3f %+.3f  PYRrate:%+.3f %+.3f %+.3f  maxrot:%+.3f %+.3f %+.3f",
//	qAA.q0*DEG, qAA.q1, qAA.q2, qAA.q3, CurrentPYR*DEG, PYRrate*DEG, maxrot*DEG);

// TODO : anglerate smoothness
		lvl.data[PITCH] = Vth->SetAttitude(PITCH, 0.0, CurrentPYR.data[PITCH], PYRrate.data[PITCH], R_DB_LOW, true);
		lvl.data[YAW]   = Vth->SetAttitude(YAW,   0.0, CurrentPYR.data[YAW],   PYRrate.data[YAW],   R_DB_LOW, true);
		if(RollMode==CTRLROLL) lvl.data[ROLL] = Vth->SetAttitude(ROLL,  0.0, CurrentPYR.data[ROLL],  PYRrate.data[ROLL],  R_DB_LOW, true);
		else if(RollMode==KILLROLL || Mode==LAN) lvl.data[ROLL] = Vth->KillRotLvl(ROLL, PYRrate.data[ROLL]);
	}

	else{ // small angles : use Euler angles instead of quaternions
		switch(Mode){
			case VEL:
				if(ORT){
					RollMode = CTRLROLL;
//					Attmethod = RYP;
					double pitchrate;
					if(ORT==RPM) pitchrate = RPM_PITCH_RATE;
					else if(ORT==TOR){
						ELEMENTS TOR_els; ORBITPARAM TOR_op;
						vclass->GetElements(vclass->GetGravityRef(), TOR_els, &TOR_op);
						pitchrate = 4.0*PI/TOR_op.T;
					}
					if (fabs(PitchYawRoll.data[YAW]) < DB_ANGLE){
						lvl.data[PITCH] = Vth->SetRotRate(PITCH, pitchrate, avel.data[PITCH], R_DB_FINE);
						lvl.data[ROLL]  = Vth->SetAttitude(ROLL, 0.0, CurrentPYR.data[ROLL], avel.data[ROLL], Treshold);
					}
					lvl.data[YAW] = Vth->SetAttitude(YAW, 0.0, CurrentPYR.data[YAW], avel.data[YAW], Treshold);
				}
				else{
					lvl.data[PITCH] = Vth->SetAttitude(PITCH, 0.0, CurrentPYR.data[PITCH], PYRrate.data[PITCH], Treshold);
					lvl.data[YAW]   = Vth->SetAttitude(YAW,   0.0, CurrentPYR.data[YAW],   PYRrate.data[YAW],   Treshold);
					if(RollMode==CTRLROLL) lvl.data[ROLL] = Vth->SetAttitude(ROLL, 0.0, CurrentPYR.data[ROLL], PYRrate.data[ROLL], Treshold);
					else if(RollMode==KILLROLL) lvl.data[ROLL] = Vth->KillRotLvl(ROLL, PYRrate.data[ROLL]);
				}
				break;

			case REL:
			case ATT:
				lvl.data[PITCH] = Vth->SetAttitude(PITCH, 0.0, CurrentPYR.data[PITCH], PYRrate.data[PITCH], Treshold);
				lvl.data[YAW]   = Vth->SetAttitude(YAW,   0.0, CurrentPYR.data[YAW],   PYRrate.data[YAW],   Treshold);
				if(RollMode==CTRLROLL) lvl.data[ROLL] = Vth->SetAttitude(ROLL, 0.0, CurrentPYR.data[ROLL], PYRrate.data[ROLL], Treshold);
				else if(RollMode==KILLROLL) lvl.data[ROLL] = Vth->KillRotLvl(ROLL, PYRrate.data[ROLL]);
				break;

			case DOC:
				if(DockS && DockT){ // docking
					lvl.data[PITCH] = Vth->SetAttitude(PITCH, 0.0, CurrentPYR.data[PITCH], PYRrate.data[PITCH], Treshold);
					lvl.data[YAW]   = Vth->SetAttitude(YAW,   0.0, CurrentPYR.data[YAW],   PYRrate.data[YAW],   Treshold);
					if(RollMode==CTRLROLL) lvl.data[ROLL] = Vth->SetAttitude(ROLL, 0.0, CurrentPYR.data[ROLL], PYRrate.data[ROLL], Treshold);
					else if(RollMode==KILLROLL) lvl.data[ROLL] = Vth->KillRotLvl(ROLL, PYRrate.data[ROLL]);
				}else{ // alignement
					if(CurrentPYR.data[PITCH] == 0.0) lvl.data[PITCH] = Vth->KillRotLvl(PITCH, PYRrate.data[PITCH]);
					else lvl.data[PITCH] = Vth->SetAttitude(PITCH, 0.0, CurrentPYR.data[PITCH], PYRrate.data[PITCH], R_DB_FINE);
					if(CurrentPYR.data[YAW] == 0.0) lvl.data[YAW] = Vth->KillRotLvl(YAW, PYRrate.data[YAW]);
					else lvl.data[YAW]   = Vth->SetAttitude(YAW,   0.0, CurrentPYR.data[YAW],   PYRrate.data[YAW],   R_DB_FINE);
					if(RollMode==KILLROLL) lvl.data[ROLL] = Vth->KillRotLvl(ROLL, PYRrate.data[ROLL]);
				}
				break;

			case LAN:
				lvl.data[PITCH] = Vth->SetAttitude(PITCH, 0.0, CurrentPYR.data[PITCH], PYRrate.data[PITCH], R_DB_FINE);
				lvl.data[YAW]   = Vth->SetAttitude(YAW,   0.0, CurrentPYR.data[YAW],   PYRrate.data[YAW],   R_DB_FINE);
				lvl.data[ROLL]  = Vth->SetAttitude(ROLL,  0.0, CurrentPYR.data[ROLL],  PYRrate.data[ROLL],  R_DB_FINE);
				break;

			case DCT:
				lvl.data[PITCH] = Vth->SetAttitude(PITCH, 0.0, CurrentPYR.data[PITCH], PYRrate.data[PITCH], R_DB_FINE);
				lvl.data[YAW]   = Vth->SetAttitude(YAW,   0.0, CurrentPYR.data[YAW],   PYRrate.data[YAW],   R_DB_FINE);
				if(RollMode==CTRLROLL) lvl.data[ROLL] = Vth->SetAttitude(ROLL, 0.0, CurrentPYR.data[ROLL], PYRrate.data[ROLL], R_DB_FINE);
				else if(RollMode==KILLROLL) lvl.data[ROLL] = Vth->KillRotLvl(ROLL, PYRrate.data[ROLL]);
				break;

			case ASC:
				lvl.data[PITCH] = Vth->SetAttitude(PITCH, 0.0, CurrentPYR.data[PITCH], PYRrate.data[PITCH], R_DB_FINE, false, 5.0);
				lvl.data[YAW]   = Vth->SetAttitude(YAW,   0.0, CurrentPYR.data[YAW],   PYRrate.data[YAW],   R_DB_FINE, false, 5.0);
				lvl.data[ROLL]  = Vth->SetAttitude(ROLL,  0.0, CurrentPYR.data[ROLL],  PYRrate.data[ROLL],  R_DB_FINE, false, 5.0);
				break;
		}

	}

//	return NULLVECTOR;
	return lvl;

}

// END OF FILE