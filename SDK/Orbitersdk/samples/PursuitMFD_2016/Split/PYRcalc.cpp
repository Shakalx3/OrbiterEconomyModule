// PursuitMFD Subfunctions
#define STRICT

#include "..\Main\PursuitMFD.h"


void SubMFD::SetPYRMethod()
{
/*
	MATRIX3 Grot;
	vclass->GetRotationMatrix(Grot);

//Rotation matrices
	MATRIX3 Mrel = GetRotMatrixXYZ(RelAtt);
	MATRIX3 Mref = GetRotMatrixXYZ(RefAtt);
	MATRIX3 Mtot = mul(InvertMatrixNorm(Grot),mul(Mref,Mrel));

	VECTOR3 resPYR = ReturnAngles(Mtot);
	VECTOR3 resYPR = ReturnAngles(Mtot);
*/

	QUAT qTest = CalcRotQuaternion();
	VECTOR3 resPYR = RotQuat2Euler(qTest,PYR);
	VECTOR3 resYPR = RotQuat2Euler(qTest,YPR);

	Attmethod = (resPYR.x*resPYR.x+resPYR.z*resPYR.z)<(resYPR.y*resYPR.y+resYPR.z*resYPR.z) ? PYR:YPR;

//sprintf(oapiDebugString(),"attvector:%i  resPYR:%0.3f %0.3f %0.3f  (%0.3f) resYPR:%0.3f %0.3f %0.3f  (%0.3f)", Attmethod, resPYR, resPYR.x*resPYR.x+resPYR.z*resPYR.z, resYPR, resYPR.y*resYPR.y+resYPR.z*resYPR.z);


}

VECTOR3 SubMFD::CalcPitchYawRollAngles()
{
	MATRIX3 Grot;
	vclass->GetRotationMatrix(Grot); // rotation sequence 1,2,3

//Rotation matrices
	MATRIX3 Mref = GetRotMatrixXYZ(RefAtt);
	MATRIX3 Mrel = GetRotMatrixXYZ(RelAtt);
	RotMat = mul(InvertMatrixNorm(Grot),mul(Mref,Mrel));

	return ReturnAngles(RotMat, Attmethod);
}

QUAT SubMFD::CalcRotQuaternion(bool PYR)
{
	VECTOR3 arot;
	vclass->GetGlobalOrientation(arot); // rotation sequence 1,2,3
/*
	if(!PYR){
		arot.z = 0.0;
		RefAtt.z = 0.0;
		RelAtt.z = 0.0;
	}
*/
	QUAT qRot = Cardan2RotQuat(arot);

	QUAT qRef = Cardan2RotQuat(RefAtt);
	QUAT qRel = Cardan2RotQuat(RelAtt);

	return qRel*qRef*inverse(qRot);
}

void SubMFD::CalcVelocity()
{
	if(htgt == href || !oapiIsVessel(htgt)) TgtVel = false;

	if(TgtVel && !ORT){
		VECTOR3 Ovel, Opos;
		oapiGetRelativeVel(htgt,href,&Ovel);
		oapiGetRelativePos(htgt,href,&Opos);
		RefAtt = GetPYR(Ovel, crossp(Opos, Ovel));

		vclass->GetRelativePos(htgt, TgtRpos); // for display
		vclass->GetRelativeVel(htgt, TgtRvel); // for display
	}

	else RefAtt = GetPYR(Rvel, crossp(Rpos, Rvel));


	PitchYawRoll = CalcPitchYawRollAngles();

/*//////////////////////////////////////////////////////////////
	QUAT Qrel = Cardan2UnitQuat(RelAtt);

	QUAT Qref = GetRotQuat(Rvel, crossp(Rpos, Rvel));

	VECTOR3 arot;
	vclass->GetGlobalOrientation(arot);
	QUAT Qrot = Cardan2UnitQuat(arot);

	PitchYawRoll = Quat2Cardan(Qrel*Qref*inverse(Qrot), Attmethod);
//*/////////////////////////////////////////////////////////////
}

void SubMFD::CalcRelative()
{
	MATRIX3 mRot; 
	vclass->GetRotationMatrix(mRot);

	// Calculate relative position in ecliptic frame
	oapiGetRelativePos(htgt, hves, &TgtRpos); // ecliptic

	// Check to see if target is too close to velocity vector in ecliptic frame.
//	VECTOR3 H;
//	if ( (length(crossp(unit(-Rvel), unit(TgtRpos)))) < 0.015 ) // Corresponds to approx 5deg.
//		H = crossp(Rvel, TgtRpos); // Use velocity vector as roll reference
//	else H = crossp(Rpos, Rvel); // Use bodyref vertical as roll reference

	RefAtt = GetPYR(TgtRpos, crossp(Rpos, Rvel));

	PitchYawRoll = CalcPitchYawRollAngles();


	// Transform relative position to vessel local frame (for display)
	TgtRpos = tmul(mRot, TgtRpos);

//	if(NoRoll) PitchYawRoll.z = 0.0;

	if(htgt==vclass->GetGravityRef()){
		vclass->GetShipAirspeedVector(TgtRvel); // !! check if it's correct
//		RadialVel = length(TgtRvel);
	}
	else{
		// Calculate relative velocity in vessel local frame
		oapiGetRelativeVel(htgt, hves, &TgtRvel); // ecliptic
		TgtRvel = tmul(mRot, TgtRvel); // vessel local frame (for display)
		// Compute the radial component
//		if(oapiGetObjectType(htgt)==OBJTP_VESSEL)
//			RadialVel = (TgtRpos * TgtRvel) / Mag(TgtRpos);
//		else RadialVel = length(TgtRvel);
	}

	RadialVel = length(TgtRvel);
}

void SubMFD::CalcDockRelative(double dt)
{
	if(!oapiIsVessel(htgt)){ // not a vessel, use relative orientation
		RelAtt = Tgt.RelAtt;
		CalcRelative();
		return;
	}

	VESSEL *tclass = oapiGetVesselInterface(htgt);

	if(DockS && DockT){ // the 2 docks are set --> docking attitude

		VECTOR3 DSpos,DSdir,DSrot,DTpos,DTdir,DTrot;
		vclass->GetDockParams(vclass->GetDockHandle(DockS-1), DSpos, DSdir, DSrot);
		tclass->GetDockParams(tclass->GetDockHandle(DockT-1), DTpos, DTdir, DTrot);

		// rotation matrix target dock frame
		MATRIX3 T = mGetDockOri(DTdir, DTrot);
		MATRIX3 mRotT;
		tclass->GetRotationMatrix(mRotT);
		MATRIX3 mRef = mul(mRotT, T); // reference attitude in global frame

		// inverse rotation matrix vessel dock frame
		MATRIX3 L = mGetDockOri(-DSdir, DSrot); // reverse dock direction --> relative attitude in local frame
//		MATRIX3 mRel = mul(InvertMatrixNorm(mRef),L);

		MATRIX3 mRotL;
		vclass->GetRotationMatrix(mRotL);

		RotMat = mul(InvertMatrixNorm(mRotL), mul(mRef,L));

		PitchYawRoll = ReturnAngles(RotMat);

RefAtt = ReturnAngles(mRef);
RelAtt = ReturnAngles(L);
/*
sprintf(oapiDebugString(),"RotT:%+.3f %+.3f %+.3f  RotL:%+.3f %+.3f %+.3f  RefAtt:%+.3f %+.3f %+.3f  RelAtt:%+.3f %+.3f %+.3f  PitchYawRoll:%+.3f %+.3f %+.3f",
	ReturnAngles(mRotT)*DEG, ReturnAngles(mRotL)*DEG, ReturnAngles(mRef)*DEG, ReturnAngles(L)*DEG, ReturnAngles(RotMat)*DEG);
*/

		// get target dock position in our local frame
		VECTOR3 TgtPt = DTpos + mul(T, DockXYZ + mul(InvertMatrixNorm(L), -DSpos)); // add offset point & local dock position (target frame)
		tclass->Local2Global(TgtPt, TgtDockRpos); // set target point in global frame
//		VECTOR3 TgtPt = mul(T, DockXYZ); // add offset point (target frame)
//		tclass->Local2Global(TgtPt + DTpos, TgtDockRpos); // set target point in global frame
		vclass->Global2Local(TgtDockRpos, TgtDockRpos); // then set target point in our local frame
//		TgtDockRpos -= DSpos; // substract our dock position (local frame)

		// get target point relative velocity
		VECTOR3 tgtavel = NULLVECTOR;
		tclass->GetAngularVel(tgtavel);
		MATRIX3 tgtvelrot = GetAngRatesMatrix(tgtavel);
		vclass->GetRelativeVel(htgt, TgtDockRvel); // global frame
		VECTOR3 tgtrotcor = mul(tgtvelrot, TgtPt); // target frame
		tgtrotcor = mul(mRotT, tgtrotcor); // global frame
		TgtDockRvel = tmul(mRotL, -(TgtDockRvel+tgtrotcor)); // invert & local frame

		// display data : calc our dock distance from target dock
		vclass->Local2Global(DSpos, TgtRpos); // set our dock position in global frame
		tclass->Global2Local(TgtRpos, TgtRpos); // then set it in the target local frame
		TgtRpos = tmul(T, TgtRpos - DTpos); // substract the target dock position & apply the target dock rotation

		TgtRvel = (TgtRpos-TPosXYZ)/dt;
		TPosXYZ = TgtRpos;
		RadialVel = length(TgtRvel);

	}
	else{ // chase target attitude

		MATRIX3 mRot; 
		vclass->GetRotationMatrix(mRot);

		// get target relative velocity
		VECTOR3 targetrvel;
		oapiGetRelativeVel(htgt, hves, &targetrvel); // ecliptic frame
		TgtRvel = tmul(mRot, targetrvel); // vessel local frame

		RadialVel = length(TgtRvel);


		// get target relative position
		oapiGetRelativePos(htgt, hves, &TgtRpos); // ecliptic frame
		TgtRpos = tmul(mRot, TgtRpos); // vessel local
		TgtDockRpos = TgtRpos - DockXYZ; // substract offset point


		// set target rvel (ecliptic frame) as reference
		if(RadialVel<0.005) PitchYawRoll = NULLVECTOR; // don't keep attitude control if radial velocity is too small
		else{
			RefAtt = GetPYR(targetrvel, _V(0.0,0.0,1.0));
			RelAtt = NULLVECTOR; // no angles
			PitchYawRoll = CalcPitchYawRollAngles();
			RollMode = KILLROLL; // PitchYawRoll.z = 0.0;
		}
	}
}

void SubMFD::SetRefAttitude()
{	
// Set the current global/ecliptic attitude as reference
	vclass->GetGlobalOrientation(RefAtt);
}

void SubMFD::ISSControl()
{
	ELEMENTS el; ORBITPARAM prm;
	vclass->GetElements(href,el,&prm,0,FRAME_ECL);
	VECTOR3 avel = NULLVECTOR;
	avel.x = -2*PI/prm.T;

	// "UCGO\\Stations\\StationISS"
	if(strncmp(vclass->GetClassNameA(), "ProjectAlpha_ISS", 16) == 0){
		Rvel = -Rvel;
		avel.x = -avel.x;
	}

	// set new orientation
	VECTOR3 NewArot = GetPYR(Rvel, crossp(Rpos, Rvel));
	vclass->SetGlobalOrientation(NewArot);
	vclass->SetAngularVel(avel);
}

// END OF FILE