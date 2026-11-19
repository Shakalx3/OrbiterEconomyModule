// PursuitMFD Subfunctions
#define STRICT

#include "..\Main\PursuitMFD.h"

#if NDEBUG
	static double timefactor = 1;
#endif
#if _DEBUG
	static double timefactor = 10;
#endif

////////////////////////////////////////////////////////////////////////////////////////////
VECTOR3 CalcJSC(double p0, double pf, double v0, double vf, double a0, double af, double t) // return jerk, snap & crackle coeffs to satisfy a minimum jerk trajectory
{
	
	double Cj = (-1.0/(t*t*t))*((6.0*(p0-pf)/(t*t))+(3.0*(v0+vf)/t)+((a0-af)/2.0));
	double Bj = ((1.0/(2*t*t))*(((v0-vf)/t)+((a0+af)/2.0)))-(5.0*t*Cj/2.0);
	double Aj = ((af-a0)/(6.0*t))-(2.0*t*Bj)-(10.0*t*t*Cj/3.0);

	return _V(Aj, Bj, Cj);

}

bool a0Root(double p0, double pf, double v0, double vf, double &a0) // find root of 3rd degree equation
{
	double A = 4.0*pow(p0,2.0);
	double B = -4.0*pow(v0,2.0)*p0;
	double C = pow(v0,4.0) + 24.0*v0*(-v0*p0+2.0*v0*pf+vf*p0);
	double D = -12.0* (pow(v0,4.0) + pow(v0,3.0)*vf);
	double p03 = ((C/A)-(B*B/(3*A*A)))/3.0;
	double q05 = (((2.0*B*B*B)/(27*A*A*A))+(D/A)-((B*C)/(3.0*A*A)))/2.0;
	double det = pow(q05,2.0)+pow(p03,3.0);
	if(det < 0.0) return false;
	double sdet = sqrt(det);
	double X1 = -q05+sdet;
	double X2 = -q05-sdet;
	double X = ((fabs(X1)/X1)*pow(fabs(X1),1.0/3.0))+((fabs(X2)/X2)*pow(fabs(X2),1.0/3.0)); // c don't perform negative root

	a0 = -pow(X-(B/(3.0*A)),2.0);

	return true;
}

double SubMFD::PredictTraj(double bradius, double gravB, double t, double &max_acc, double &req_dv) // compute trajectory to predict fuel use & warning messages
{
	double vpf = 0.0;
	double yf = 0.0;
	double vyf = 0.0;
	if(LandPhase < INITAPP){
		double factor = 1.25;
		double pitch = 20*RAD;
		if(Vth->AR01){
//			pitch = 20*RAD;
//			factor = 1.25;
			AppDuration = 180.0;
		}
		vpf = factor*gravB*sin(pitch)*AppDuration;
		yf = 2.0* 0.5*factor*gravB*(1-cos(pitch))*AppDuration*AppDuration;
		vyf = 2.0* factor*-gravB*(1-cos(pitch))*AppDuration;
	}

	double minalt = yf;
//	double maxacc = 0.0;

	double dVtot = 0.0;

	int iter = 200;
	double step = (quad[7]-t)/iter;
	double thvp, thy, thap, thay, centrip, accy, accmax;//, thvy;
	double t2, t3;

	double pitch=0.0;

	// calculate trajectory minimum
	for(int st=0; st<iter; st++){
		t2 = t*t; t3 = t*t2;
		thy  = quad[10] + quad[11]*t + quad[3]*t3 + quad[4]*t2*t2 + quad[5]*t2*t3;
		minalt = min(minalt, thy);

		if(minalt < 0.90*yf){
			errorBF |= BFpow(7); //errmsg = "WARNING:trajectory too low";
			if(minalt < 0.0) errorBF |= BFpow(6); // errmsg = "DANGER:collision trajectory!!!";
			else if(errorBF & BFpow(6)) errorBF ^= BFpow(6);
		}
		else if(errorBF & BFpow(7)) errorBF ^= BFpow(7);

		thvp = quad[9] + quad[6]*t + 3.0*quad[0]*t2 + 4.0*quad[1]*t3 + 5.0*quad[2]*t2*t2;
//		thvy = quad[11] + 3.0*quad[3]*t2 + 4.0*quad[4]*t3 + 5.0*quad[5]*t2*t2;

		centrip = thvp*thvp/(bradius+thy); // centripetal force

		thap = quad[6] + 6.0*quad[0]*t + 12.0*quad[1]*t2 + 20.0*quad[2]*t3;
		thay = 6.0*quad[3]*t + 12.0*quad[4]*t2 + 20.0*quad[5]*t3;

		accy = thay + gravB - centrip; // taking account of centripetal force in y-axis acceleration
		accmax = hypot(thap, accy);

		pitch = min(pitch,asin(accy/accmax));
//		if(st==195) pitch = acos(accy/accmax);

		max_acc = max(max_acc, accmax); // save max acceleration needed

		t += step;

		dVtot += accmax*step; // acceleration integration to compute velocity change during braking
	}
//	req_dv = dVtot + hypot(vpf, vyf) + gravB*(TbeforeApp+AppDuration+TbeforeFinal) + gravB*90.0; // approximation of total velocity change
	if(LandPhase < INITAPP) req_dv = dVtot + hypot(vpf, vyf) + gravB*AppDuration + gravB*90.0; // approximation of total velocity change
	else if(LandPhase < INITFIN) req_dv = dVtot + gravB*90.0; // approximation of total velocity change

//	req_dv = 100.0 * (1.0+floor(req_dv/100.0));

	// around data @ 2.5% above result
//	double inc1 = floor(req_dv*0.025);
//	double inc2 = floor(req_dv/inc1);
//	req_dv = (inc2+1)*inc1;

	return pitch;
}

////////////////////////////////////////////////////////////////////////////////////////////
void SubMFD::Landing(double DT)
{

/////////// safety
	RollMode = CTRLROLL;

	href = vclass->GetGravityRef();

	if(htgt == NULL){
		htgt = href;
		LandPhase = UNABLE; errorBF |= BFpow(11);// errmsg = "Target not valid"
	}
	else if(errorBF & BFpow(11)) errorBF ^= BFpow(11);

	if(!Lsave.manloc){
		if(oapiGetObjectType(htgt) != OBJTP_SURFBASE){
			if(oapiGetObjectType(htgt) == OBJTP_VESSEL){
				VESSEL * V = oapiGetVesselInterface(htgt);
				if(!(V->GetFlightStatus() & BFpow(0)) && (oapiGetOrbiterVersion()>150000)){ // Orbiter2015 and superior only (problem with previous release)
					htgt = href;
					LandPhase = UNABLE; errorBF |= BFpow(9);// errmsg = "Target vessel not landed."
				}
				else if(errorBF & BFpow(9)) errorBF ^= BFpow(9);
				if(V->GetGravityRef() != href){
					htgt = href;
					LandPhase = UNABLE; errorBF |= BFpow(10);// errmsg = "Target vessel not valid."
				}
				else if(errorBF & BFpow(10)) errorBF ^= BFpow(10);
			}
			else if(Lsave.BST==0.0){// init
				strcpy(tgtname,"1");
				if(!setBase(tgtname, false)){
					htgt = href;
					LandPhase = UNABLE; errorBF |= BFpow(1);// errmsg = "no base on gravity ref."
				}
				else if(errorBF & BFpow(1)) errorBF ^= BFpow(1);
			}
		}
		oapiGetObjectName(htgt,tgtname,NAMEMAX); // always update
	}
	else{
		htgt = href;
		if(errorBF & BFpow(1)) errorBF ^= BFpow(1);
		if(errorBF & BFpow(9)) errorBF ^= BFpow(9);
		if(errorBF & BFpow(10)) errorBF ^= BFpow(10);
		if(LandPhase == UNABLE) LandPhase = WAIT;
	}

	if(inhibate) errorBF |= BFpow(0); // errmsg = "Other MFD AP on";
	else if(errorBF & BFpow(0)) errorBF ^= BFpow(0);



/////////// engine update
	THGROUP_TYPE thgt = THGROUP_MAIN;

	if(Hold && Vth->MaxForce[THGROUP_HOVER] > 0.0)
		vclass->SetThrusterGroupLevel(THGROUP_HOVER, 0.0); // inhibate hover
	if(Hover || Vth->MaxForce[THGROUP_MAIN] == 0.0){
		Hover = true;
		thgt = THGROUP_HOVER;
		if(Hold && Vth->MaxForce[THGROUP_MAIN] > 0.0)
			vclass->SetThrusterGroupLevel(THGROUP_MAIN, 0.0); // inhibate main
	}

	double vmass = Vth->StructMass;
	double vthrust = Vth->MaxForce[thgt];
	double MAcc = vthrust/vmass;
	double vISP = 1.0;
	if(vclass->GetGroupThruster(thgt,0)) vISP = vclass->GetThrusterIsp(vclass->GetGroupThruster(thgt,0)); // check if thrustergroup is defined


	if(vthrust<=0.0){ LandPhase = WAIT; Hold = false; errorBF |= BFpow(14); return;} // errmsg = "no thrust available"; return;}
	else if(errorBF & BFpow(14)) errorBF ^= BFpow(14);



/////////// state update
	double bradius = oapiGetSize(href) + Lsave.belv; //////// elevation

	double bmass = oapiGetMass(href);
	double Mu = GGRAV*bmass;

	double FPvel = length(Rvel)*sin(acos(dotp(unit(Rpos),unit(Rvel))));
	double centrip = (FPvel*FPvel)/(length(Rpos));
	double dgF = Mu/(length2(Rpos)) - centrip;

	double gravB = Mu/(bradius*bradius);

	//equatorial elements
	ELEMENTS ovElems; ORBITPARAM ovParams;
	vclass->GetElements(NULL, ovElems, &ovParams, 0, FRAME_EQU);

	if(ovElems.e >= 1.0){ // eccentricity not valid
		LandPhase = UNABLE; errorBF |= BFpow(16);// errmsg = "eccentricity too high"
	}
	else if(errorBF & BFpow(16)) errorBF ^= BFpow(16);

	// target bearing
	double vlon, vlat, vrad;
	vclass->GetEquPos(vlon, vlat, vrad);

	double blon = Lsave.blon;
	double blat = Lsave.blat;

	double dlat = vlat-blat;
	double dlon = vlon-blon;
	double cc = oapiOrthodome(vlon, vlat, blon, blat);
	double bdis = bradius*cc;
	double xx0 = -sin(dlon) * cos(blat);
	double yy0 = cos(vlat)*sin(blat) - sin(vlat)*cos(blat)*cos(dlon);
	double tbrg = atan2(xx0, yy0);
	if(tbrg < 0.0) tbrg = PI2-tbrg;
	if(tbrg > PI2) tbrg = 4.0*PI-tbrg;
	
	// targeting vectors
	if(LandPhase == WAIT || LandPhase == INITBRK || LandPhase == INITAPP || LandPhase == INITFIN){
		Lsave.tbrg = tbrg;
		Lsave.vlon = vlon;
		Lsave.vlat = vlat;
	}

	MATRIX3 Mbody;
	oapiGetRotationMatrix(href, &Mbody);

	VECTOR3 origin = NULLVECTOR;
	oapiEquToLocal(href, Lsave.vlon, Lsave.vlat, bradius, &origin);
	origin = mul(Mbody, origin); // global

	VECTOR3 target = NULLVECTOR;
	oapiEquToLocal(href, blon, blat, bradius, &target);
	target = mul(Mbody, target); // global

	VECTOR3 up = unit(Rpos);
	VECTOR3 right = unit(crossp(origin, target));
	VECTOR3 forward = unit(crossp(up, -right)); // left-handed


	// ground speed relative to target point
	VECTOR3 hvel = GetHorizonGroundSpeed(href, Rpos, Rvel); // target ref

	// currrent bearing
	double hv = hypot(hvel.data[0], hvel.data[1]);
	double cbrg = atan2(hvel.data[0], hvel.data[1]);
	if(hvel.data[0] < 0.0)  cbrg += PI2;
	if(cbrg > PI2) cbrg -= PI2;

	// position
	VECTOR3 pos;
	VECTOR3 vel;

	double xxp = -bdis*sin(tbrg);
	double yyp = -bdis*cos(tbrg);
	pos.x = sin(Lsave.tbrg)*xxp+cos(Lsave.tbrg)*yyp;

	pos.y = length(Rpos)-bradius-vclass->GetCOG_elev();

//	pos.z = pos.x*tan(tbrg-cbrg);
	if(LandPhase>APPROACH && LandPhase<UNABLE)
		pos.z = cos(Lsave.tbrg)*xxp-sin(Lsave.tbrg)*yyp;
	else pos.z = cos(cbrg)*xxp-sin(cbrg)*yyp;

	double xxv = hv*sin(cbrg);
	double yyv = hv*cos(cbrg);

//	vel.x = (pos.x-quad[12])/DT; //	
	vel.x = sin(tbrg)*xxv+cos(tbrg)*yyv;

//	vel.y = (pos.y-quad[14])/DT; //	
	vel.y = hvel.data[2];

//	if(LandPhase==FINAL)
//		vel.z = -(-cos(tbrg)*xxv+sin(tbrg)*yyv);
//	else 
		vel.z = (pos.z-quad[16])/DT; // for display only

	bool closest = false;

	VECTOR3 predictpolar = _V(blon, blat, bradius);

//// find closest passage to landed target /////
	if(LandPhase == WAIT){// || LandPhase == BRAKE){//	if(false){

		VECTOR3 predictpos, predictvel;
		VECTOR3 basepos = _V(blon, blat, bradius);
		VECTOR3 basevel = NULLVECTOR;
		LonLat2StateVectors(href, basepos, basevel);
		double dot = dotp(Rvel-basevel, Rpos-basepos);
		double currentdot = dot;
		double currentgeodot = hvel.data[0]*xx0 + hvel.data[1]*yy0;

		double period = ovParams.T;
		double N = 16.0;
		double t_start = 0.0;
		double t_end = 2.0*period;

		double pertr = (int)(prevclosest/period);
		double stp = period*pertr;

		if(!refreshclosest){
			if (prevclosest > 0.0){ // closest already found
				if(errorBF & BFpow(13)){ // alignement not valid
					t_start = prevclosest + 0.5*period;
					t_end = t_start + 2.5*period;
				}
				else{ // search around previous solution
					t_start = stp+0.75*(prevclosest-stp);
					t_end = stp+1.25*(prevclosest-stp);
					if(fabs(t_end-t_start)<N){ // if too close to period start
						t_start = max(t_start-0.25*period,0.0);
						t_end += 0.25*period;
					}
				}
			}
		}
		refreshclosest = false;

		double stepmin = TIMESTEP/10.0;
		double dt = max(stepmin, fabs(t_end-t_start)/N);
		double t = t_start + dt;

		bool ahead = (dot<0.0);

		int iter = 0;
		int itermax = 32;
		while(t<t_end && iter<itermax){

			// ship state vector at time
			PredictPosVel(href, ovElems, ovParams, t, predictpos, predictvel, NoSpherGrav);

			// base state vector at time
			basepos = _V(blon, blat, bradius);
			basevel = NULLVECTOR;
			LonLat2StateVectors(href, basepos, basevel, t);

			dot = dotp(predictvel-basevel, predictpos-basepos);

			if(dot<0.0){ // target ahead
				ahead = true;
				t_start = t;
				dt = fabs(t_end-t_start)/4.0;
				if(dt < stepmin) break;
				else t += dt;
			}
			else{
				if(ahead){ // target behind
					t_end = t;
					dt = fabs(t_end-t_start)/2.0;
					if(dt < stepmin) break;
					else t = t_start + dt;
				}
				else t += dt; // search dot<0.0 further
			}

			iter++;
		}

		if(iter<itermax){ // closest passage found

			predictpolar = predictpos;
			VECTOR3 predictpolarvel = predictvel;
			StateVectors2LonLat(href, predictpolar, predictpolarvel, t);
			VECTOR3 grdvel = GetHorizonGroundSpeed(href, predictpos, predictvel, t);

			pos.x = -oapiOrthodome(vlon, vlat, predictpolar.data[0], predictpolar.data[1]) * bradius;
			if(currentgeodot<0.0) pos.x = -PI2*bradius - pos.x; // next passage
//			pos.x -= PI2*bradius*pertr; // if solution is over period
			pos.y = length(predictpos)-bradius-vclass->GetCOG_elev(); // predictpolar.data[2]
			pos.z = oapiOrthodome(predictpolar.data[0], predictpolar.data[1], blon, blat) * bradius;
			vel.x = fabs(grdvel.data[0]);
			vel.y = grdvel.data[2]; //predictpolarvel.data[2];
//			vel.z = (pos.z-quad[16])/DT; // for display only
//			vel.z = 0.0;
			vel.z = abs(0.0001*(pos.z-quad[16])/DT); // for display only

			closest = true;
			quad[31] = t; // time to closest passage
		}
	}

	// live data for display
	quad[12] = pos.x;
	quad[13] = vel.x;
	quad[14] = pos.y;
	quad[15] = vel.y;
	quad[16] = pos.z;
	quad[17] = vel.z;

	if(fabs(pos.x)<1000.0) errorBF |= BFpow(2); // errmsg = "Target < 1000.0m";
	else if(errorBF & BFpow(2)) errorBF ^= BFpow(2);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	VECTOR3 acc = NULLVECTOR;

////////////
	if(Hold && quad[31]>ovParams.T) Hold = false; // unless infinite loop when optimize burntime
////////////


	switch(LandPhase){
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case WAIT:			//	waiting fot ignition
	{
		if(oapiGetTimeAcceleration() > 100*timefactor){
				oapiSetTimeAcceleration(100.0*timefactor);
				sprintf(oapiDebugString(),"PursuitMFD: %s is waiting for landing brake --> limiting time acceleration", vesname);
				debugstring = true; DebugstringTime = oapiGetSysTime() + 3.0;
		}


		if(Hold) vclass->SetThrusterGroupLevel(thgt, 0.0);

		double amax = min(MAcc, gravB*2.5); // no fuel consumption, worst case...
		double trigger = amax*min(amax/(2.0*gravB), 0.75); // in case of low thrust

		double s;
		if(cc < 1.0*RAD)
			s = length(Rpos-target); // linear approximation
		else s = length(Rpos)*cc;

		double v = length(Rvel);
		double areq = (v*v)/(2.0*s);

		double si = (v*v)/(2.0*trigger);
		double BST = (s-si)/v; // linear approximation of start date

		double bt = 0.0;

		double t = quad[31], ds = 0.0;
		int i = 0;
		VECTOR3 burnpos, burnvel, polarburnpos, polarburnvel;
		prevclosest = -1.0; // reinit previous closest
		if(closest){ // closest passage found --> calc BurnStartTime with areq=trigger (s=si)
///////////////////////////////////////
//predictpolar = _V(blon, blat, bradius);
///////////////////////////////////////
			while(i<32){
				// ship lon/lat & groundvel at time
				PredictPosVel(href, ovElems, ovParams, t, burnpos, burnvel, NoSpherGrav);
				VECTOR3 burngrdvel = GetHorizonGroundSpeed(href, burnpos, burnvel, t);
				polarburnpos = burnpos; polarburnvel = burnvel;
				StateVectors2LonLat(href, polarburnpos, polarburnvel, t);
				s = oapiOrthodome(polarburnpos.data[0], polarburnpos.data[1], predictpolar.data[0], predictpolar.data[1]) * bradius;
				v = sqrt(burngrdvel.data[0]*burngrdvel.data[0]+burngrdvel.data[1]*burngrdvel.data[1]);
				areq = (v*v)/(2.0*s);
				si = (v*v)/(2.0*trigger);
				ds = (s-si)/v;
				t += ds;
				i++;
				if(fabs(s-si)<v/10.0) break;
			}

			alignmax = 0.5*si;

			if(i<32){ // minimum s=si successful
				BST = t;
				prevclosest = quad[31]; // in case of alignement not valid
				if(fabs(pos.z)>alignmax){ LandPhase = WAIT; Hold = false; errorBF |= BFpow(13); return;} // errmsg = "alignment not valid"; return;}
				else if(errorBF & BFpow(13)) errorBF ^= BFpow(13);
			}
			else closest = false;

		}
		else if(errorBF & BFpow(13)) errorBF ^= BFpow(13);
/*
sprintf(oapiDebugString(),"(%i) BST:%.3f (%.3f)  s:%.3f  si:%.3f  v:%.3f  t:%.3f  ds:%.3f  areq:%.3f  trigger:%.3f  i:%i  polarburnpos:%.3f %.3f  predictpolar:%.3f %.3f",
	closest, BST, quad[31], s, si, v, t, ds, areq, trigger, i, polarburnpos.data[0]*DEG, polarburnpos.data[1]*DEG, predictpolar.data[0]*DEG, predictpolar.data[1]*DEG);
oapiSetPause(true);
*/
		if(errorBF & BFpow(15) && BST>0.0){ //errormsg = "WARNING:trajectory too steep"
			BST = (Lsave.BST-oapiGetSimMJD())*86400.0;
		}

		//sprintf(oapiDebugString(),"BST:%.3f  s:%.3f  si:%.f  v:%.3f  trigger:%.3f  amax:%.3f  MAcc:%.3f  gravB:%.3f", BST, s, si, v, trigger, amax, MAcc, gravB);

////////////////////////////// final point & velocity targeted
		double factor = 1.25;
		double pitch = 20*RAD;
		if(Vth->AR01){
			pitch = 20*RAD;
			factor = 1.25;
			AppDuration = 180.0;
		}
		double pf = 0.5*factor*-gravB*sin(pitch)*AppDuration*AppDuration;
		double vpf = factor*gravB*sin(pitch)*AppDuration;
		double yf = 2.0* 0.5*factor*gravB*(1.0-cos(pitch))*AppDuration*AppDuration;
		double vyf = 2.0* factor*-gravB*(1.0-cos(pitch))*AppDuration;
/*
sprintf(oapiDebugString(),"pf:%.3f (%.3f)  vpf:%.3f (%.3f)  yf:%.3f (%.3f)  vyf:%.3f (%.3f)",
	pf, 0.5*1.25*-gravB*sin(20*RAD)*60.0*60.0, vpf, 1.25*gravB*sin(20*RAD)*60.0,
	yf, 2.0* 0.5*1.25*gravB*(1-cos(20*RAD))*60.0*60.0, vyf, 2.0* 1.25*-gravB*(1-cos(20*RAD))*60.0);
*/
///////////////////////////////////////////////////////////////////////////////
		for(int burncorr=0; burncorr<20; burncorr++){ // try to find a better start time

//			if(!closest){ // closest passage not found --> calc pos & vel

				VECTOR3 Brpos=NULLVECTOR, Brvel=NULLVECTOR;
				// calculate position & velocity @ burn start
				PredictPosVel(href, ovElems, ovParams, BST, Brpos, Brvel, NoSpherGrav);

				VECTOR3 equpos = Brpos;
				VECTOR3 equvel = Brvel;
				StateVectors2LonLat(href, equpos, equvel, BST);
				double burnlon = equpos.data[0];
				double burnlat = equpos.data[1];
				double burnrad = equpos.data[2];

				dlat = burnlat-blat;
				dlon = burnlon-blon;
				cc = oapiOrthodome(burnlon, burnlat, blon, blat);
				bdis = bradius*cc;
				xx0 = -sin(dlon) * cos(blat);
				yy0 = cos(burnlat)*sin(blat) - sin(burnlat)*cos(blat)*cos(dlon);
				tbrg = atan2(xx0, yy0);
				if(tbrg < 0.0) tbrg = PI2-tbrg;
				if(tbrg > PI2) tbrg = 4.0*PI-tbrg;

				xxp = -bdis*sin(tbrg);
				yyp = -bdis*cos(tbrg);
				pos.x = sin(tbrg)*xxp+cos(tbrg)*yyp;
				pos.y = length(Brpos)-bradius-vclass->GetCOG_elev();

				hvel = GetHorizonGroundSpeed(href, Brpos, Brvel); // target ref

				hv = hypot(hvel.data[0], hvel.data[1]);
				cbrg = atan2(hvel.data[0], hvel.data[1]);
				if(hvel.data[0] < 0.0)  cbrg += PI2;
				if(cbrg > PI2) cbrg -= PI2;

				xxv = hv*sin(cbrg);
				yyv = hv*cos(cbrg);
				vel.x = sin(tbrg)*xxv+cos(tbrg)*yyv;
				vel.y = hvel.data[2];
//			}
	////////////////////////////////////////////////
			double a0 = 0.0;
			if(!a0Root(pos.x, pf, vel.x, vpf, a0)) {LandPhase = WAIT; Hold = false; errorBF |= BFpow(4); return;} // errmsg = "det negative"; return;}
			else if(errorBF & BFpow(4)) errorBF ^= BFpow(4);

			a0 = max(-MAcc, a0); //////////////

			bt = (vel.x/(2*sqrt(-a0)))-(pos.x/vel.x);
			if(bt < 0.0){ LandPhase = WAIT; Hold = false; errorBF |= BFpow(5); return;} // errmsg = "bt negative"; return;}
			else if(errorBF & BFpow(5)) errorBF ^= BFpow(5);

//	sprintf(oapiDebugString(),"BST:%.3f  a0:%.3f (%.3f)  bt:%.9f  posx:%.3f  tbrg:%.3f  cbrg:%.3f  hvel:%.3f %.3f",
//		BST,a0,MAcc,bt,pos.x,tbrg*DEG,cbrg*DEG,hvel.data[0],hvel.data[2]);

	////////////////////////////////////////////////
			VECTOR3 coefp = CalcJSC(pos.x, pf, vel.x, vpf, a0, 0.0, bt);
			VECTOR3 coefy = CalcJSC(pos.y, yf, vel.y, vyf, 0.0, 0.0, bt);

			quad[0] = coefp.x;
			quad[1] = coefp.y;
			quad[2] = coefp.z;
			quad[3] = coefy.x;
			quad[4] = coefy.y;
			quad[5] = coefy.z;
			quad[6] = a0; // initial decceleration
			quad[7] = bt; // estimated brake duration
			quad[8] = pos.x;
			quad[9] = vel.x;
			quad[10] = pos.y;
			quad[11] = vel.y;

			// live data for display
	//		quad[12] = pos.x;
	//		quad[13] = vel.x;
	//		quad[14] = pos.y;
	//		quad[15] = vel.y;
	//		quad[16] = pos.z;
	//		quad[17] = vel.z;

			double maxacc = 0.0;
			double limit = PredictTraj(bradius, gravB, 0.0, maxacc, reqdV);

			fuelplan = vmass*(1.0-exp(-reqdV/vISP));

			// live waiting data
			quad[18] = areq;
			quad[19] = trigger;
			quad[20] = maxacc;
			quad[21] = MAcc;

			// other data for display
			quad[22] = 0.0;		// apcorr
			quad[23] = 0.0;		// aycorr
			quad[24] = 0.0;		// time offset
			quad[25] = 1.0;		// ratio y
			quad[26] = 1.0;		// ratio x
			quad[27] = pos.x;	// xgraphmin

//	sprintf(oapiDebugString(),"(burncorr:%i) limit:%.3f BST:%.3f  s:%.3f  si:%.f  v:%.3f  trigger:%.3f  amax:%.3f  MAcc:%.3f  gravB:%.3f",
//		burncorr, limit*DEG, BST-burncorr*burncorr*10.0*fabs(limit), s, si, v, trigger, amax, MAcc, gravB);

			if(fabs(limit)<-1.0){ // errormsg = "WARNING:trajectory too steep"
				LandPhase = WAIT;
				errorBF |= BFpow(15);
				BST -= burncorr*burncorr*10.0*fabs(limit);
			}
			else{
				if(errorBF & BFpow(15)) errorBF ^= BFpow(15);
				break;
			}

		}
///////////////////////////////////////////////////////////////////////////////

		if(!Hold && BST<0.0) Lsave.BST = oapiGetSimMJD() + (BST/86400.0); // we missed last burn opportunity, recalc

//		if(areq < trigger && BST>0.0){ // waiting...
		bool wait = false;
		if(closest){
			if(BST>0.0) wait = true;
		}
		else{
			if(areq<trigger && BST>0.0) wait = true;
		}

		if(wait){

			Lsave.BST = oapiGetSimMJD() + (BST/86400.0);

			RelAtt = NULLVECTOR;
			brkctrl = NONE;
			if(Hold && (BST<300.0 || quad[20]>0.90*MAcc)){ // begin alignement
				brkctrl = FORWARD;
				if(BST<180.0 || quad[20]>0.925*MAcc){ // begin attitude control
					brkctrl = BRAKEPREP;
					// rotation matrix of engine direction
					VECTOR3 dir = Vth->GetThrusterGroupForceDir(thgt);
					MATRIX3 mRoll = _M( 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
					if(RotaTh) mRoll = _M(-1.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 1.0);
					MATRIX3 mThrustOri = mul(GetRotMatrixXYZ(GetPYdir(dir)), mRoll);
					// 180deg pitch rotation wrt thruster orientation
					MATRIX3 Mrel = mul(_M( 1.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, -1.0), InvertMatrixNorm(mThrustOri));
					RelAtt = ReturnAngles(Mrel);
				}
				// target orientation in global frame
				RefAtt = GetPYR(forward, right);


/////////////////////////
				PitchYawRoll = CalcPitchYawRollAngles();
			}
			else{ // prograde
				brkctrl = PROGRADE;
				RefAtt = GetPYR(Rvel, crossp(Rpos, Rvel));
				PitchYawRoll = CalcPitchYawRollAngles();
			}

//sprintf(oapiDebugString(),"wait:%i Lsave.BST:%.3f BST:%.3f", wait, Lsave.BST, BST);

//sprintf(oapiDebugString(),"RefAtt:%.3f %.3f %.3f  RelAtt:%.3f %.3f %.3f   PitchYawRoll:%.3f %.3f %.3f  %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f  %.3f %.3f",
//	RefAtt*DEG, RelAtt*DEG, PitchYawRoll*DEG, GetRotMatrixXYZ(_V(180.0*RAD,0.0,0.0)),quad[20],0.925*MAcc);

			return;
		}

		if(!Hold) return;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case INITBRK:
	{
		Lsave.BST = oapiGetSimMJD(); // time of ignition
		LandPhase = BRAKE;
	}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case BRAKE:
	{
		if(oapiGetTimeAcceleration() > 10.0*timefactor){
				oapiSetTimeAcceleration(10.0*timefactor);
				sprintf(oapiDebugString(),"PursuitMFD: %s is in active landing phase --> limiting time acceleration", vesname);
				debugstring = true; DebugstringTime = oapiGetSysTime() + 3.0;
		}
	}
	break;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case INITAPP:		//	targeting for approach phase
	{
		// for display
		quad[24] = 0.0;		// time offset
		quad[25] = 1.0;		// ratio y
		quad[26] = 1.0;		// ratio x
		quad[27] = pos.x;	// xgraphmin

		double pf = StLandDis;//-25.0
		double yf = StLandAlt;//30.0;
		double vpf = StLandVel;//1.0
		double vyf = 0.0;
		double a0 = 0.0;

		if(!a0Root(pos.x, pf, vel.x, vpf, a0)) {LandPhase = WAIT; Hold = false; errorBF |= BFpow(4); return;} //errmsg = "det negative"; return;}
		else if(errorBF & BFpow(4)) errorBF ^= BFpow(4);

		double bt = (vel.x/(2*sqrt(-a0)))-(pos.x/vel.x);

		VECTOR3 coefp = CalcJSC(pos.x, pf, vel.x, vpf, a0, 0.0, bt);
		VECTOR3 coefy = CalcJSC(pos.y, yf, vel.y, vyf, 0.0, 0.0, bt);

		quad[0] = coefp.x;
		quad[1] = coefp.y;
		quad[2] = coefp.z;
		quad[3] = coefy.x;
		quad[4] = coefy.y;
		quad[5] = coefy.z;

		quad[6] = a0;
		quad[7] = bt;

		quad[8] = pos.x;
		quad[9] = vel.x;
		quad[10] = pos.y;
		quad[11] = vel.y;

		Lsave.BST = oapiGetSimMJD();

		LandPhase = APPROACH;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case APPROACH:
	{
		if(oapiGetTimeAcceleration() > 10.0*timefactor){
				oapiSetTimeAcceleration(10.0*timefactor);
				sprintf(oapiDebugString(),"PursuitMFD: %s is in active landing phase --> limiting time acceleration", vesname);
				debugstring = true; DebugstringTime = oapiGetSysTime() + 3.0;
		}
	}
	break;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case INITFIN:		//	targeting for landing phase
	{
		// for display
		quad[24] = 0.0; // time offset
		quad[25] = pos.y/quad[10]; // ratio y
		quad[26] = pos.x/quad[8]; // ratio x
		quad[27] = pos.x; // xgraphmin

////////// error compensation
		StLandAlt = pos.y;
//		StLandDis = pos.x;

		quad[9] = vel.x;

		LandPhase = FINAL;

//////////////loging
	quad[22] = 0.0;
	quad[23] = 0.0;
////////////////////
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case FINAL:
	{
		if(oapiGetTimeAcceleration() > 1.0*timefactor){
				oapiSetTimeAcceleration(1.0*timefactor);
				sprintf(oapiDebugString(),"PursuitMFD: %s is in final landing phase --> switching off time acceleration", vesname);
				debugstring = true; DebugstringTime = oapiGetSysTime() + 3.0;
		}

		// for display
		quad[12] = pos.x;
		quad[13] = vel.x;
		quad[14] = pos.y;
		quad[15] = vel.y;
		quad[16] = pos.z;
		quad[17] = vel.z;


		double tgtvel = -(pos.x-0.0)/10.0; //10.0
		double deltavel = tgtvel - 2.0*fabs(vel.x);  //// 2.0 factor to increase delta velocity and pitch change
//		if(tgtvel>quad[9]) acc.x = deltavel/20.0; //quad[28]/2.0;
//		else 
			acc.x = deltavel/10.0; // deceleration
		acc.x = min(acc.x, 0.0); // only deceleration // 0.0 (almost)

//sprintf(oapiDebugString(),"accx:%.3f (%.3f)  tgtvel:%.3f (%.3f)  deltavel:%.3f  velx:%.3f",acc.x,quad[28],tgtvel,quad[9],deltavel,vel.x);

		double drate = (StLandAlt-pos.y)/10.0; // 10.0
		double delrat = min(0.0,drate)-vel.y; // don't go up
		acc.y = gravB+(delrat/3.0); // 3.0

		double ttf = 10.0 + fabs(pos.x)/(10.0*vel.x); // time left
		double vz = cos(cbrg)*xxp-sin(cbrg)*yyp; // pseudo vel.z
//		double vz = cos(tbrg)*xxv-sin(tbrg)*yyv; // pseudo vel.z
		double ttfz = max(ttf/1.0, 10.0); // ttf/2.0
		acc.z = -2.0*((pos.z/(ttfz*ttfz))+(vz/ttfz));
//		acc.z *= -sign(pos.x); // sign inversion as we have passed the target


		VECTOR3 plim = NULLVECTOR;
		switch(Lfinal){
		case AWAY: plim = _V(15.0, 10.0, 1.00);
			break;
		case CLOSE: plim = _V(5.0, 5.0, 0.60);
			break;
		case ABOVE: plim = _V(1.0, 2.0, 0.50);
			break;
		}
		if(fabs(pos.x)<plim.x && fabs(pos.z)<plim.y && fabs(hv)<plim.z) landing = true; // vertical landing trigger

		if(landing) { // vertical descent
			drate = max(-pos.y/10.0, -3.0);
			delrat = drate-vel.y;
			acc.y = max(gravB+(delrat/3.0), gravB/2.0);

			// limit bank
//			double banklim = sin(10.0*RAD); // 2.0*RAD
//			if(fabs(acc.z/length(acc)) > banklim) acc.z = banklim*length(acc);
		}


		if(pos.y <= CutoffAlt || vclass->GroundContact()) {
			StopHoldAndTrim();

			if(forceLandState){
				if(vclass->GroundContact()){ // force Landed state
					if(!(vclass->GetFlightStatus() & BFpow(0)) && delrat<1.0){ // we can still crash...
						///////from scenario editor
						VESSELSTATUS2 vs;
						memset( &vs, 0, sizeof(vs));
						vs.version = 2;
						vs.rbody = href;
						vclass->GetStatusEx(&vs);

						vs.status = 1; // landed (beware: vs.arot & vs.vrot.x must be set correctly)

						// arot is no more global, the ref change to the body
						MATRIX3 mBod, mVes;
						oapiGetRotationMatrix(href, &mBod);
						vclass->GetRotationMatrix(mVes);
						MATRIX3 mRot = mul(InvertMatrixNorm(mBod),mVes);
						vs.arot.x = atan2(mRot.m23, mRot.m33);
						vs.arot.y = -asin(mRot.m13);
						vs.arot.z = atan2(mRot.m12, mRot.m11);
						vs.vrot.x = vclass->GetCOG_elev();

						vs.surf_lng = vlon;
						vs.surf_lat = vlat;
						double heading = 0.0;
						oapiGetHeading(hves, &heading);
						vs.surf_hdg = heading;
						vclass->DefSetStateEx(&vs);
					}
				}
			}

			if(applyBrake){
				if(vclass->GroundContact()){ // force Landed state
					if(!(vclass->GetFlightStatus() & BFpow(0)) && delrat<1.0){ // we can still crash...
						LandPhase = LDBRAKE;
					}
				}
			}
			else LandPhase = LANDED;

		}
		else break;

	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case LDBRAKE:
	{

		StopHoldAndTrim();

		quad[12] = pos.x;
		quad[13] = vel.x;
		quad[14] = pos.y;
		quad[15] = vel.y;
		quad[16] = pos.z;
		quad[17] = vel.z;

		if(!applyBrake){
			LandPhase = LANDED;
			return;
		}

		vclass->SetWheelbrakeLevel(1.0, 0, true);

		debugstring = true; DebugstringTime = oapiGetSysTime() + 1.0;

		if(vclass->GetFlightStatus() & BFpow(0)) // switch to landed only if vessel is inactive
			LandPhase = LANDED;

		return;

	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case LANDED:
	{
		vclass->SetWheelbrakeLevel(0.0, 0, true);

		quad[12] = pos.x;
		quad[13] = vel.x;
		quad[14] = pos.y;
		quad[15] = vel.y;
		quad[16] = pos.z;
		quad[17] = vel.z;

		return;

	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	case UNABLE:
	{
		if(inhibate) return;

		StopHoldAndTrim();

	}
	return;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	}

// attitude & engine AP
	if(LandPhase == BRAKE || LandPhase == APPROACH || LandPhase == FINAL){

		if(LandPhase == BRAKE || LandPhase == APPROACH){
			double bt = (oapiGetSimMJD()-Lsave.BST)*86400; // current burn time
			double ttf = quad[7]-bt; // time to final
			double ttfcorr = 10.0 + ttf/10.0; // error compensation time with little offset

			double maxacc = 0.0;
			PredictTraj(bradius, gravB, bt, maxacc, reqdV); // calculate reqdV
//			PredictFuel(bradius, gravB, bt);

			fuelplan = vmass*(1.0-exp(-reqdV/vISP));

			if(LandPhase==BRAKE && Hold)
//				if(ttf<15.0 || fabs(RelAtt.data[PITCH])<2.0*RAD){ LandPhase = INITAPP; return;}
				if(ttf<TbeforeApp || (vel.y>0.0 && ttf<2.0*TbeforeApp)){
					LandPhase = INITAPP;
//					return;
				}
			if(LandPhase==APPROACH && Hold)
				if(ttf<TbeforeFinal || (vel.y>0.0 && ttf<2.0*TbeforeFinal)){
					LandPhase = INITFIN;
//					return;
				}

			double Ap = quad[0];
			double Bp = quad[1];
			double Cp = quad[2];
			double Ay = quad[3];
			double By = quad[4];
			double Cy = quad[5];
			double a0p = quad[6];

			double p0 = quad[8];
			double vp0 = quad[9];
			double y0 = quad[10];
			double vy0 = quad[11];

			double bt2 = bt*bt;
			double bt3 = bt*bt2;
			double thp	= pos.x - (p0 + vp0*bt + 0.5*a0p*bt2 + Ap*bt3 + Bp*bt2*bt2 + Cp*bt2*bt3);
			double thvp	= vel.x - (vp0 + a0p*bt + 3.0*Ap*bt2 + 4.0*Bp*bt3 + 5.0*Cp*bt2*bt2);
			double thy	= pos.y - (y0 + vy0*bt + Ay*bt3 + By*bt2*bt2 + Cy*bt2*bt3);
			double thvy	= vel.y - (vy0 + 3.0*Ay*bt2 + 4.0*By*bt3 + 5.0*Cy*bt2*bt2);

			double apcorr = 2.0*((thp/(ttfcorr*ttfcorr))+(thvp/ttfcorr));
			double aycorr = 2.0*((thy/(ttfcorr*ttfcorr))+(thvy/ttfcorr));


			acc.x = a0p + 6.0*Ap*bt + 12.0*Bp*bt2 + 20.0*Cp*bt3 - apcorr;
			acc.y = 0.0 + 6.0*Ay*bt + 12.0*By*bt2 + 20.0*Cy*bt3 + dgF - aycorr;
			double vz = cos(tbrg)*xxv-sin(tbrg)*yyv; // pseudo vel.z
			double ttfcorrz = max(ttfcorr/1.0, 10.0);
			acc.z = -1.0*((pos.z/(ttfcorrz*ttfcorrz))+(vz/ttfcorrz)); // 2.0
			acc.z *= cos(abs(acc.x)/length(acc));
//			vz = abs(vz) > abs(vel.z) ? vz:vel.z;
//			vz = vel.z;
//			double ttfcorrz = max(ttfcorr/2.0, 10.0);
//			acc.z = -2.0*((pos.z/(ttfcorrz*ttfcorrz))+(vz/ttfcorrz));
//			double pz = vz*ttfcorr*2.0;//*tz;
//			acc.z = -2.0*(((pos.z*(1+pz/pos.z))/(ttfcorr/2.0*ttfcorr/2.0))+(vz/ttfcorr/2.0));
//			acc.z = -2.0*((pos.z/(ttfcorr/1.0*ttfcorr/1.0))+(vz/ttfcorr/1.0));
//			acc.z = -(vel.z-(pos.z/(ttfcorr/2.0)))/(ttfcorr/2.0);

//sprintf(oapiDebugString(),"acc:%0.3f %0.3f %0.3f  ttfcorr:%0.3f  ttfcorrz:%0.3f vz:%0.3f  vel.z:%0.3f", acc, ttfcorr, ttfcorrz, vz, vel.z);

			// live data for display
			quad[12] = pos.x;
			quad[13] = vel.x;
			quad[14] = pos.y;
			quad[15] = vel.y;
			quad[16] = pos.z;
			quad[17] = vel.z;
			quad[22] = apcorr;
			quad[23] = aycorr;

		}

		if(inhibate) return;

		if(LandPhase>INITAPP && LandPhase<INITBRK && (errorBF & BFpow(12))){ // keep gravity compensation when aborting AP Approach & Final
			acc.x = -vel.x/1.414; // zeroing forward velocity
			acc.y = gravB - vel.y/1.0; //min(0.0,vel.y/10.0); // don't go down
			acc.z = -vel.z/1.414; // zeroing lateral velocity
//			if(vel.y>0.0) Hold = false;
		}

////////////////////////////////////////
		VECTOR3 drag = NULLVECTOR;
		VECTOR3 lift = NULLVECTOR;
		vclass->GetDragVector(drag);
		vclass->GetLiftVector(lift);
		acc -= (drag + lift) / vmass;
//	sprintf(oapiDebugString(),"acc:%.3f %.3f %.3f (%.3f)   drag:%.3f %.3f %.3f (%.3f %.3f %.3f)   lift:%.3f %.3f %.3f (%.3f %.3f %.3f)",acc,length(acc),drag/vmass,drag,lift/vmass,lift);
////////////////////////////////////////

		if(LandPhase!=FINAL){
			quad[28] = acc.x;
			quad[29] = acc.y;
			quad[30] = acc.z;
		}

		VECTOR3 vec = unit(acc);
		// rearranged to fit local frame (x right, y up, z forward)
		vec = _V(vec.z,vec.y,vec.x);


/////orientation with any engines direction
		RefAtt = GetPYR(forward, right);
		// rotation matrix of engine direction
		VECTOR3 dir = Vth->GetThrusterGroupForceDir(thgt);
		MATRIX3 mRoll = _M( 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
		if(RotaTh) mRoll = _M(-1.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 1.0);
		MATRIX3 mThrustOri = mul(GetRotMatrixXYZ(GetPYdir(dir)), mRoll);
		// acceleration direction wrt thruster orientation
		MATRIX3 Mrel = mul(GetRotMatrixXYZ(GetPYdir(vec)), InvertMatrixNorm(mThrustOri));
		RelAtt = ReturnAngles(Mrel);

		PitchYawRoll = CalcPitchYawRollAngles();


		double thrpct = min(length(acc)/MAcc, 1.0);

		if(LandPhase == FINAL)
			thrpct = min(acc.y/MAcc, 1.0);
		else 
			thrpct *= max(0.0, cos(fabs(PitchYawRoll.data[PITCH])));

		thrpct = max(0.0,thrpct);
		thrpct = min(thrpct,1.0);
		if(Hold) vclass->SetThrusterGroupLevel(thgt, thrpct);

	}

}

// END OF FILE