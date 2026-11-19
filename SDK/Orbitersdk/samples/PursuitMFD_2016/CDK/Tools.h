//////////////////////////////////////////////////////////////////////////////////////////////
//
//	Tools.h - some functions & useful math operations
//
//	1st Copyright Chris Knestrick, 2002
//
///////////////////////////////////////////////////////////////////////////////////////////////

#ifndef TOOLS_H
#define TOOLS_H
#define _CRT_SECURE_NO_DEPRECATE

#include "Orbitersdk.h"


// Null vector
const VECTOR3 NULLVECTOR = { 0.0, 0.0, 0.0 };

// Provide ease of access to vectors (e.g. Status.vrot.data[PITCH])
enum AXIS {PITCH, YAW, ROLL};

enum ATTMETHOD {PYR, PRY, YPR, YRP, RYP, RPY};

// return 2^k binary for bits-field mask
// |= to assign
// &  to read
// ^= to remove
inline int BFpow(int bit_loc)
{
// platform dependant
//	return 1<<bit_loc;

// less performance but endian independant
	int bit = 1;
	for(int i=0;i<bit_loc;i++)
		bit *= 2;
	return bit;

};


inline void getBuildDate(char *builddate){
	char const date[12] = __DATE__;
	char result[7];
	char buf[3];
	int month, day, year;
	static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
	sscanf(date, "%s %d 20%d", buf, &day, &year); // also remove century
	month = (strstr(month_names, buf)-month_names)/3+1;
	sprintf(result, "%d%02d%02d", year, month, day);
	for(int i=0; i<7; i++)
		builddate[i] = result[i];
}


// Scale a value before it's displayed.  The buffer must be at least 10 bytes.
inline void ScaleOutput(char *Buffer, double Value)
{
	if (fabs(Value) < 1000) {
		sprintf(Buffer, "%.2f", Value);
	} else if (fabs(Value) < 1000000) {
		sprintf(Buffer, "%.2fk", Value/1000);
	} else if (fabs(Value) < 1495978707) {		// 1 A.U. / 100 
		sprintf(Buffer, "%.2fM", Value/1000000);
	} else {
		sprintf(Buffer, "%.2fAU", Value/149597870700);
	}
}


// cross product with the most orthogonal basis vector
inline VECTOR3 orthogonal (const VECTOR3 &a)
{
	VECTOR3 basis = a.x < a.y ? (a.x < a.z ? _V(1.0,0.0,0.0) : _V(0.0,0.0,1.0)) : (a.y < a.z ? _V(0.0,1.0,0.0) : _V(0.0,0.0,1.0));
	return crossp(a, basis);
}

// determinant of 2D vector
inline double det2D (const VECTOR3 &a, const VECTOR3 &b)
{
	return a.x*b.y - a.y*b.x;
}

inline double sign (const double &a)
{
	return a/fabs(a);
}

////// quaternion ////////////////////////////////////////////////////////////////
typedef union {
	double data[4];                      ///< array data interface
	struct { double q0, q1, q2, q3; };   ///< named data interface
} QUAT;

inline QUAT _Q(double q0, double q1, double q2, double q3)
{
	QUAT quaternion = {q0,q1,q2,q3}; return quaternion;
}

// Null vector
const QUAT NULLQUAT = { 0.0, 0.0, 0.0, 0.0 };

// quaternion operators
inline QUAT operator* (const QUAT &q, const double f)
{
	return _Q(q.q0*f, q.q1*f, q.q2*f, q.q3*f);
}
inline QUAT operator/ (const QUAT &q, const double f)
{
	return _Q(q.q0/f, q.q1/f, q.q2/f, q.q3/f);
}
inline QUAT &operator/= (QUAT &q, const double f)
{
	q.q0 /= f; q.q1 /= f; q.q2 /= f; q.q3 /= f;
	return q;
}
inline QUAT operator* (const QUAT &q, const QUAT &p) // not commutative
{
	return _Q(
				p.q0*q.q0 - p.q1*q.q1 - p.q2*q.q2 - p.q3*q.q3,
				p.q0*q.q1 + p.q1*q.q0 - p.q2*q.q3 + p.q3*q.q2,
				p.q0*q.q2 + p.q1*q.q3 + p.q2*q.q0 - p.q3*q.q1,
				p.q0*q.q3 - p.q1*q.q2 + p.q2*q.q1 + p.q3*q.q0
			);
}
inline QUAT operator/ (const QUAT &q, const QUAT &p) // not commutative
{
	double norm_p = p.q0*p.q0 + p.q1*p.q1 + p.q2*p.q2 + p.q3*p.q3;
	return _Q(
				(p.q0*q.q0 + p.q1*q.q1 + p.q2*q.q2 + p.q3*q.q3)/norm_p,
				(p.q0*q.q1 - p.q1*q.q0 - p.q2*q.q3 + p.q3*q.q2)/norm_p,
				(p.q0*q.q2 + p.q1*q.q3 - p.q2*q.q0 - p.q3*q.q1)/norm_p,
				(p.q0*q.q3 - p.q1*q.q2 + p.q2*q.q1 - p.q3*q.q0)/norm_p
			);
}

// quaternion maths
inline QUAT conj(const QUAT &q) // conjugate
{
	return _Q(q.q0, -q.q1, -q.q2, -q.q3);
}
inline double length(const QUAT &q) // modulus
{
	return sqrt(q.q0*q.q0 + q.q1*q.q1 + q.q2*q.q2 + q.q3*q.q3);
}
inline QUAT unit(const QUAT &q)
{
	return q/length(q);
}
inline double norm(const QUAT &q)
{
	return q.q0*q.q0 + q.q1*q.q1 + q.q2*q.q2 + q.q3*q.q3;
}
inline QUAT inverse(const QUAT &q)
{
	return conj(q)/norm(q);
}
inline void normalise(QUAT &q)
{
	q /= length(q);
}

inline VECTOR3 mul(const QUAT &q, const VECTOR3 &v)
{
	QUAT res = q * _Q(0.0, v.x, v.y, v.z) * conj(q);
	return _V(res.q1, res.q2, res.q3);
}

inline QUAT mul(const MATRIX4 &m, const QUAT &q)
{
	return _Q(
				q.q0*m.m11 + q.q1*m.m12 + q.q2*m.m13 + q.q3*m.m14,
				q.q0*m.m21 + q.q1*m.m22 + q.q2*m.m23 + q.q3*m.m24,
				q.q0*m.m31 + q.q1*m.m32 + q.q2*m.m33 + q.q3*m.m34,
				q.q0*m.m41 + q.q1*m.m42 + q.q2*m.m43 + q.q3*m.m44
			);
}

// quaternion rotation matrix
inline MATRIX3 qrot(const QUAT &q) // unit quaternion rotation matrix
{
	QUAT uq = unit(q);
	return _M(
				1.0 - 2.0*(uq.q2*uq.q2 + uq.q3*uq.q3),	2.0*(uq.q1*uq.q2 + uq.q0*uq.q3),		2.0*(uq.q1*uq.q3 - uq.q0*uq.q2),
				2.0*(uq.q1*uq.q2 - uq.q0*uq.q3),		1.0 - 2.0*(uq.q1*uq.q1 + uq.q3*uq.q3),	2.0*(uq.q2*uq.q3 + uq.q0*uq.q1),
				2.0*(uq.q1*uq.q3 + uq.q0*uq.q2),		2.0*(uq.q2*uq.q3 - uq.q0*uq.q1),		1.0 - 2.0*(uq.q1*uq.q1 + uq.q2*uq.q2)
			);
}
inline VECTOR3 qrotv(const QUAT &q, const VECTOR3 &v) // rotate vector by a unit quaternion
{
	return mul(qrot(q),v);
}

// functions
inline QUAT Cardan2RotQuat(const VECTOR3 &cardan) // rotation sequence (1,2,3)
{
	double cx = cos(cardan.x/2.0);
	double sx = sin(cardan.x/2.0);
	double cy = cos(cardan.y/2.0);
	double sy = sin(cardan.y/2.0);
	double cz = cos(cardan.z/2.0);
	double sz = sin(cardan.z/2.0);

	return _Q(
				cx*cy*cz + sx*sy*sz,
				sx*cy*cz - cx*sy*sz,
				cx*sy*cz + sx*cy*sz,
				cx*cy*sz - sx*sy*cz
			);
}

inline VECTOR3 RotQuat2Euler(const QUAT &unit_quaternion, ATTMETHOD attmethod=PYR)
{
	double length_q = length(unit_quaternion);
	double q0 = unit_quaternion.q0/length_q;
	double q1 = unit_quaternion.q1/length_q;
	double q2 = unit_quaternion.q2/length_q;
	double q3 = unit_quaternion.q3/length_q;

	if(attmethod == PYR) // rotation sequence (1,2,3)
		return _V(
					atan2(2.0*(q2*q3 + q0*q1), 1.0 - 2.0*(q1*q1 + q2*q2)),
					-asin(2.0*(q1*q3 - q0*q2)),
					atan2(2.0*(q1*q2 + q0*q3), 1.0 - 2.0*(q2*q2 + q3*q3))
				);

	else if(attmethod == YPR) // rotation sequence (2,1,3)
		return _V(
					asin(2.0*(q2*q3 + q0*q1)),
					atan2(-2.0*(q1*q3 - q0*q2), 1.0 - 2.0*(q1*q1 + q2*q2)),
					atan2(-2.0*(q1*q2 - q0*q3), 1.0 - 2.0*(q1*q1 + q3*q3))
				);

	return NULLVECTOR;
}

inline VECTOR3 RotQuat2AngVel(const QUAT &unit_quaternion, const QUAT &rate_quaternion, bool bodyfixed = true)
{
	double length_q = length(unit_quaternion);
	double q0 = unit_quaternion.q0/length_q;
	double q1 = unit_quaternion.q1/length_q;
	double q2 = unit_quaternion.q2/length_q;
	double q3 = unit_quaternion.q3/length_q;

	MATRIX4 mWglob = _M( // in world coordinates
					-q1*2.0,	 q0*2.0,	-q3*2.0,	 q2*2.0,
					-q2*2.0,	 q3*2.0,	 q0*2.0,	-q1*2.0,
					-q3*2.0,	-q2*2.0,	 q1*2.0,	 q0*2.0,
						0.0,		0.0,		0.0,		0.0
				);

	MATRIX4 mW = _M( // in body fixed coordinates
					-q1*2.0,	 q0*2.0,	 q3*2.0,	-q2*2.0,
					-q2*2.0,	-q3*2.0,	 q0*2.0,	 q1*2.0,
					-q3*2.0,	 q2*2.0,	-q1*2.0,	 q0*2.0,
						0.0,		0.0,		0.0,		0.0
				);

	if(!bodyfixed) mW = mWglob;

	QUAT qPYRrate = mul(mW, rate_quaternion);

	return _V(qPYRrate.q0, qPYRrate.q1, qPYRrate.q2);
}

// Returns shortest (?) rotation quaternion from origin & final
inline QUAT GetRotationQuaternion(const VECTOR3 &u, const VECTOR3 &v)
{
// normalise the vectors
	VECTOR3 uu = unit(u);
	VECTOR3 vv = unit(v);

	VECTOR3 h = crossp(uu, vv);
	double q0 = dotp(uu, vv) + 1.0;

	if(q0 == 0.0) // anti-parallel vector
		h = orthogonal(uu); // construct an orthogonal vector

	return unit(_Q(q0, h.x, h.y, h.z));
}

// Returns changing frame quaternion
inline QUAT GetRotQuat(const VECTOR3 &pitchyaw, const VECTOR3 &roll)
{
	QUAT qpitchyaw = GetRotationQuaternion(pitchyaw, _V(0.0, 0.0, 1.0));
	VECTOR3 r = mul(inverse(qpitchyaw), _V(1.0, 0.0, 0.0));
	QUAT qroll = GetRotationQuaternion(roll, r);

	return unit(qpitchyaw*qroll);
}


// Returns angle/axis quaternion from unit quaternion
inline QUAT GetAngleAxisQuat(const QUAT &unit_quaternion)
{
	QUAT q = unit(unit_quaternion); // quaternion must be unit

	VECTOR3 vecrot = _V(q.q1, q.q2, q.q3);

	VECTOR3 rotaxis = _V(0.0, 0.0, 1.0);

	double anglerot = 2.0*acos(q.q0); // extract rotation angle
	if(anglerot!=0.0) rotaxis = vecrot/sqrt(1.0-q.q0*q.q0); // extract rotation axis (division by 0 !!!)
	if(anglerot>PI){
		anglerot -= PI2;
		rotaxis = -rotaxis;
	}

//	double anglerot = 2.0*atan2(length(vecrot), q.q0); // extract rotation angle
//	VECTOR3 rotaxis = vecrot/sin(anglerot/2.0); // extract rotation axis

	return _Q(anglerot,rotaxis.x,rotaxis.y,rotaxis.z);
}


// Returns rotation vector from unit quaternion
inline VECTOR3 GetRotationVector(const QUAT &unit_quaternion)
{
	QUAT q = GetAngleAxisQuat(unit_quaternion);

	return _V(q.q1,q.q2,q.q3) * q.q0;
}


// Returns new rotation unit quaternion with no roll
inline QUAT GetNoRollRotQuat(const QUAT &unit_quaternion)
{
	double length_q = length(unit_quaternion);
	double q0 = unit_quaternion.q0/length_q;
	double q1 = unit_quaternion.q1/length_q;
	double q2 = unit_quaternion.q2/length_q;
	double q3 = unit_quaternion.q3/length_q;

	// roots : (unit_quaternion*rot(gamma)).q3 = 0
	double gamma1 = 4.0*atan((q0+sqrt(q0*q0+q3*q3))/q3);
	double gamma2 = 4.0*atan((q0-sqrt(q0*q0+q3*q3))/q3);

	// choose the minimum rotation angle
	double rot1 = 2.0*acos(q0*cos(gamma1/2.0)-q3*sin(gamma1/2.0));
	double rot2 = 2.0*acos(q0*cos(gamma2/2.0)-q3*sin(gamma2/2.0));
	double minangle = rot1<rot2 ? gamma1:gamma2;

	return unit(_Q(cos(minangle/2.0),0.0,0.0,sin(minangle/2.0)) * _Q(q0, q1, q2, q3));
}


////// rotation matrices /////////////////////////////////////////////////////////
inline MATRIX3 InvertMatrixNorm(const MATRIX3 &M)
{
	return _M(
				M.m11, M.m21, M.m31,
				M.m12, M.m22, M.m32,
				M.m13, M.m23, M.m33
			);
}

// Returns the rotation matrix around the X axis
inline MATRIX3 GetRotMatrixX(const double &alpha)
{
	double ca = cos(alpha);
	double sa = sin(alpha);

	return _M(
				1.0,	0.0,	0.0,
				0.0,	 ca,	 sa,
				0.0,	-sa,	 ca
			);
}

// Returns the rotation matrix around the Y axis
inline MATRIX3 GetRotMatrixY(const double &beta)
{
	double cb = cos(beta);
	double sb = sin(beta);

	return _M(
				 cb,	0.0,	-sb,
				0.0,	1.0,	0.0,
				 sb,	0.0,	 cb
			);
}

// Returns the rotation matrix around the Z axis
inline MATRIX3 GetRotMatrixZ(const double &gamma)
{
	double cg = cos(gamma);
	double sg = sin(gamma);

	return _M(
				 cg,	 sg,	0.0,
				-sg,	 cg,	0.0,
				0.0,	0.0,	1.0
			);
}

// Returns the rotation matrix (DCM) given rotation sequence (1,2,3)
inline MATRIX3 GetRotMatrixXYZ(const VECTOR3 &cardan)
{
	double cx = cos(cardan.x);
	double sx = sin(cardan.x);
	double cy = cos(cardan.y);
	double sy = sin(cardan.y);
	double cz = cos(cardan.z);
	double sz = sin(cardan.z);

	return _M(
				cy*cz,				cy*sz,				-sy,
				sx*sy*cz - cx*sz,	sx*sy*sz + cx*cz,	sx*cy,
				cx*sy*cz + sx*sz,	cx*sy*sz - sx*cz,	cx*cy
			);
}

// Returns the angle rates matrix given rotation sequence (1,2,3)
inline MATRIX3 GetAngRatesMatrix(const VECTOR3 &cardan)
{
	return _M(
				 0.0,			-cardan.z,		 cardan.y,
				 cardan.z,		 0.0,			-cardan.x,
				-cardan.y,		 cardan.x,		 0.0
			);
}

// Returns rotations
inline VECTOR3 GetPYR(const VECTOR3 &pitchyaw, const VECTOR3 &roll, ATTMETHOD attmethod=PYR)
{
	// normalise the vectors
	VECTOR3 py = unit(pitchyaw);
	VECTOR3 r = unit(roll);

	if(attmethod == PYR) // rotation sequence (1,2,3)
		return _V(
					atan2(py.y, py.z),
					-asin(py.x),
					atan2(crossp(py,r).x, r.x)
				);

	else if(attmethod == YPR) // rotation sequence (2,1,3)
		return _V(
					asin(py.y),
					atan2(-py.x, py.z),
					atan2(crossp(py,r).x, r.x)
				);

	return NULLVECTOR;
}

inline VECTOR3 ReturnAngles(const MATRIX3 &mrot, ATTMETHOD attmethod=PYR)
{
	if(attmethod == PYR) // rotation sequence (1,2,3)
		return _V(
					atan2(mrot.m23, mrot.m33),
					-asin(mrot.m13),
					atan2(mrot.m12, mrot.m11)
				);

	else if(attmethod == YPR) // rotation sequence (2,1,3)
		return _V(
					asin(mrot.m23),
					atan2(-mrot.m13, mrot.m33),
					atan2(-mrot.m21, mrot.m22)
				);

	return NULLVECTOR;
}

// Returns pitch & yaw angles (roll=0.0) from Dir vector giving rotation sequence
inline VECTOR3 GetPYdir(const VECTOR3 &dir, ATTMETHOD attmethod=PYR)
{
	// normalise the vector
	VECTOR3 d = unit(dir);

	if(attmethod == PYR) // rotation sequence (1,2,0)
		return _V(
					atan2(d.y, d.z),
					-asin(d.x),
					0.0
				);

	else if(attmethod == YPR) // rotation sequence (2,1,0)
		return _V(
					asin(d.y),
					atan2(-d.x, d.z),
					0.0
				);

	return NULLVECTOR;
}

inline VECTOR3 vGetDockOri(const VECTOR3 &dir, const VECTOR3 &rot)
{
	VECTOR3 d = unit(dir);
	VECTOR3 r = unit(rot);

	VECTOR3 res = GetPYdir(d);

	VECTOR3 A = tmul(GetRotMatrixXYZ(res),r);
	res.data[ROLL] = atan2(A.x, A.y);

	return res;
}

inline MATRIX3 mGetDockOri(const VECTOR3 &dir, const VECTOR3 &rot)
{
	return GetRotMatrixXYZ(vGetDockOri(dir, rot));
}


//////////////////////////////////////////////////////////////////////////////////
// return total mass & PMI of coupled vessel
inline void GetStructMoment(VESSEL *vessel, VESSEL *parent, double &vmass, VECTOR3 &vmoment)
{
	VECTOR3 pmives = NULLVECTOR;
	vessel->GetPMI(pmives);

	VECTOR3 Vpts[6] = {NULLVECTOR, NULLVECTOR, NULLVECTOR, NULLVECTOR, NULLVECTOR, NULLVECTOR};
	Vpts[0].x = 0.5*sqrt(fabs(-pmives.x+pmives.y+pmives.z));
	Vpts[0].y = 0.0;
	Vpts[0].z = 0.0;

	Vpts[1].x = -0.5*sqrt(fabs(-pmives.x+pmives.y+pmives.z));
	Vpts[1].y = 0.0;
	Vpts[1].z = 0.0;

	Vpts[2].x = 0.0;
	Vpts[2].y = 0.5*sqrt(fabs(pmives.x-pmives.y+pmives.z));
	Vpts[2].z = 0.0;

	Vpts[3].x = 0.0;
	Vpts[3].y = -0.5*sqrt(fabs(pmives.x-pmives.y+pmives.z));
	Vpts[3].z = 0.0;

	Vpts[4].x = 0.0;
	Vpts[4].y = 0.0;
	Vpts[4].z = 0.5*sqrt(fabs(pmives.x+pmives.y-pmives.z));

	Vpts[5].x = 0.0;
	Vpts[5].y = 0.0;
	Vpts[5].z = -0.5*sqrt(fabs(pmives.x+pmives.y-pmives.z));


//	VECTOR3 Gpts[6] = {{0.0,0.0,0.0}, {0.0,0.0,0.0}, {0.0,0.0,0.0}, {0.0,0.0,0.0}, {0.0,0.0,0.0}, {0.0,0.0,0.0}};
	VECTOR3 Gpts = NULLVECTOR;
	VECTOR3 NewVesPMI = NULLVECTOR;
	for(int i=0; i<6; i++)
	{
		vessel->Local2Global(Vpts[i],Gpts);
		parent->Global2Local(Gpts,Vpts[i]);

		NewVesPMI.x += Vpts[i].y*Vpts[i].y+Vpts[i].z*Vpts[i].z;
		NewVesPMI.y += Vpts[i].x*Vpts[i].x+Vpts[i].z*Vpts[i].z;
		NewVesPMI.z += Vpts[i].y*Vpts[i].y+Vpts[i].x*Vpts[i].x;
	}

	vmass	= vessel->GetMass();
	vmoment	= NewVesPMI * vmass;

	return;
}

inline void Ecl2Equ(const OBJHANDLE href, VECTOR3 &rpos, VECTOR3 &rvel, bool fixed=false, double time=0.0) // from ScnEditor
{
	if(oapiGetObjectType(href)!=OBJTP_PLANET) return;

	MATRIX3 rot;

	if(time==0.0 || fixed){	// current time rotation matrix & time independent

		if(fixed) oapiGetPlanetObliquityMatrix(href, &rot);
		else oapiGetRotationMatrix(href, &rot);

		rpos = tmul(rot, rpos);
		rvel = tmul(rot, rvel);

		if(fixed) return;

		double T = oapiGetPlanetPeriod(href);
		double r = hypot(rpos.x,rpos.z);
		double phi = atan2 (rpos.z, rpos.x);
		double v = r*PI2/T;
		rvel.x += v*sin(phi);
		rvel.z -= v*cos(phi);

	}else{

		oapiGetPlanetObliquityMatrix(href, &rot);
		double bangvel = PI2/oapiGetPlanetPeriod(href);
		double bcurrent_rot = oapiGetPlanetCurrentRotation(href);
		rot = mul(rot, GetRotMatrixY(bcurrent_rot+bangvel*time));

		rpos = tmul(rot, rpos);
		rvel = tmul(rot, rvel);

		double r = hypot(rpos.x,rpos.z);
		double phi = atan2 (rpos.z, rpos.x);
		double v = r*bangvel;
		rvel.x += v*sin(phi);
		rvel.z -= v*cos(phi);

	}

}

inline void Equ2Ecl(const OBJHANDLE href, VECTOR3 &rpos, VECTOR3 &rvel, bool fixed=false, double time=0.0) // from ScnEditor
{
	if(oapiGetObjectType(href)!=OBJTP_PLANET) return;

	MATRIX3 rot;

	if(time==0.0 || fixed){	// current time rotation matrix & time independent

		if(!fixed){
			double T = oapiGetPlanetPeriod(href);
			double r = hypot(rpos.x,rpos.z);
			double phi = atan2 (rpos.z, rpos.x);
			double v = r*PI2/T;
			rvel.x -= v*sin(phi);
			rvel.z += v*cos(phi);
			oapiGetRotationMatrix(href, &rot);
		}
		else oapiGetPlanetObliquityMatrix(href, &rot);

		rpos = mul(rot, rpos);
		rvel = mul(rot, rvel);

	}
	else{

		double bangvel = PI2/oapiGetPlanetPeriod(href);
		double r = hypot(rpos.x,rpos.z);
		double phi = atan2 (rpos.z, rpos.x);
		double v = r*bangvel;
		rvel.x -= v*sin(phi);
		rvel.z += v*cos(phi);

		oapiGetPlanetObliquityMatrix(href, &rot);
		double bcurrent_rot = oapiGetPlanetCurrentRotation(href);
		rot = mul(rot, GetRotMatrixY(bcurrent_rot+bangvel*time));

		rpos = mul(rot, rpos);
		rvel = mul(rot, rvel);

	}

}

inline void Cart2Polar(VECTOR3 &pos, VECTOR3 &vel) // from ScnEditor
{
	// position in polar coordinates
	double r      = sqrt  (pos.x*pos.x + pos.y*pos.y + pos.z*pos.z);
	double lng    = atan2 (pos.z, pos.x);
	double lat    = asin  (pos.y/r);
	// derivatives in polar coordinates
	double drdt   = (vel.x*pos.x + vel.y*pos.y + vel.z*pos.z) / r;
	double dlngdt = (vel.z*pos.x - pos.z*vel.x) / (pos.x*pos.x + pos.z*pos.z);
	double dlatdt = (vel.y*r - pos.y*drdt) / (r*sqrt(r*r - pos.y*pos.y));
	pos.data[0] = lng;
	pos.data[1] = lat;
	pos.data[2] = r;
	vel.data[0] = dlngdt;
	vel.data[1] = dlatdt;
	vel.data[2] = drdt;
}

inline void Polar2Cart(VECTOR3 &pos, VECTOR3 &vel) // from ScnEditor
{
	double lng = pos.data[0], clng = cos(lng), slng = sin(lng);
	double lat = pos.data[1], clat = cos(lat), slat = sin(lat);
	double r   = pos.data[2];
	// position in cartesian coordinates
	double x   = r * cos(lat) * cos(lng);
	double z   = r * cos(lat) * sin(lng);
	double y   = r * sin(lat);
	// derivatives in cartesian coordinates
	double dxdt = vel.data[2]*clat*clng - vel.data[0]*r*clat*slng - vel.data[1]*r*slat*clng;
	double dzdt = vel.data[2]*clat*slng + vel.data[0]*r*clat*clng - vel.data[1]*r*slat*slng;
	double dydt = vel.data[2]*slat + vel.data[1]*r*clat;
	pos.data[0] = x;
	pos.data[1] = y;
	pos.data[2] = z;
	vel.data[0] = dxdt;
	vel.data[1] = dydt;
	vel.data[2] = dzdt;
}

inline VECTOR3 Position2Horizon(const VECTOR3 rpos, VECTOR3 vec)
{
	MATRIX3 rot = GetRotMatrixXYZ(GetPYR(rpos, crossp(rpos, _V(0.0,1.0,0.0))));
	vec = tmul(rot, vec);
	return _V(vec.x, vec.z, -vec.y);
}

inline MATRIX3 StateVectors2Matrix(const VECTOR3 rpos, const VECTOR3 rvel)
{
	VECTOR3 pro_axis = rvel/length(rvel);
	VECTOR3 pla_axis = -crossp(rpos,rvel)/length(crossp(rpos,rvel));
	VECTOR3 out_axis = crossp(pro_axis,pla_axis);

	return _M(pro_axis.x, pla_axis.x, out_axis.x,
				pro_axis.y, pla_axis.y, out_axis.y,
					pro_axis.z, pla_axis.z, out_axis.z);
}

inline void StateVectors2LonLat(const OBJHANDLE href, VECTOR3 &rpos, VECTOR3 &rvel, double time=0.0)
{
	if(oapiGetObjectType(href)!=OBJTP_PLANET) return;

	Ecl2Equ(href, rpos, rvel, false, time);
	Cart2Polar(rpos, rvel);
}

inline void LonLat2StateVectors(const OBJHANDLE href, VECTOR3 &pos, VECTOR3 &vel, double time=0.0)
{
	if(oapiGetObjectType(href)!=OBJTP_PLANET) return;

	Polar2Cart(pos, vel);
	Equ2Ecl(href, pos, vel, false, time);
}
/*
inline VECTOR3 LonLat2Rpos(const OBJHANDLE href, const VECTOR3 &geo)
{
	if(oapiGetObjectType(href)!=OBJTP_PLANET) return NULLVECTOR;

	double r   = geo.data[1];
	double lng = geo.data[0], clng = cos(lng), slng = sin(lng);
	double lat = geo.data[2], clat = cos(lat), slat = sin(lat);
	// position in cartesian coordinates
	double x   = r * cos(lat) * cos(lng);
	double z   = r * cos(lat) * sin(lng);
	double y   = r * sin(lat);

	// rotating equatorial to ecliptic
	MATRIX3 rot;
	oapiGetRotationMatrix(href, &rot);

	return mul(rot, _V(x, y, z));
}
*/
/*
inline VECTOR3 GetHorizonGroundSpeed(const OBJHANDLE href, VECTOR3 rpos, VECTOR3 rvel, const double &lat, const double &lon, const double &bradius)
{
	if(oapiGetObjectType(href)!=OBJTP_PLANET) return NULLVECTOR;

	VECTOR3 GS = _V(-sin(lon),0.0,cos(lon))
						*cos(lat); // equator fixed
							*(2.0*PI*bradius/oapiGetPlanetPeriod(href))

	Ecl2Equ(href, rpos, rvel, false); // equator fixed

	Cart2PolarHvel(rpos, rvel);

	return rvel-GS;
}
*/
inline VECTOR3 GetHorizonGroundSpeed(const OBJHANDLE href, VECTOR3 rpos, VECTOR3 rvel, double time=0.0)
{
	if(oapiGetObjectType(href)!=OBJTP_PLANET) return NULLVECTOR;

	Ecl2Equ(href, rpos, rvel, false, time); // equator rotating
	Cart2Polar(rpos, rvel);
	return _V(rvel.data[0]*rpos.data[2]*cos(rpos.data[1]), rvel.data[1]*rpos.data[2], rvel.data[2]); // convert anglerate to velocity
}

/*////////////////////////////////////////////////////////////////////////////////
  Copyright 2003-2005 Chris Knestrick

  Code to solve the prediction problem - given the position and velocity
  vectors at some initial time, what will the position and velocity vectors
  be at specified time in the future. The method implemented here is
  described in Chapter 4 of "Fundementals of Astrodynamics" by Bate, Mueller,
  and White.
// LazyD combined two functions into one
inline void CalcCAndS(const double &Z, double &C, double &S)
{
	if (Z > 0) {
		C = ((1 - cos(sqrt(Z))) / Z);
		S = ((sqrt(Z) - sin(sqrt(Z))) / sqrt(pow(Z, 3)));
	} else if (Z < 0) {
		C = ((1 - cosh(sqrt(-Z))) / Z);
		S = ((sinh(sqrt(-Z)) - sqrt(-Z)) / sqrt(pow(-Z, 3)));
	} else {
		C = 0.5; // probably doesn't matter, but this is correct - LazyD
		S = 1.0/6.0; // probably doesn't matter, but this is correct - LazyD
	}
}

// Iterative method to calculate new X and Z values for the specified time of flight
inline void CalcXandZ(double &X, double &Z, const VECTOR3 Pos, const VECTOR3 Vel,
							 const double a, const double Time, const double SqrtMu)
{
	const double MAX_ITERS = 16;
	const double EPSILON = 0.000000001;

	double C, S, T, dTdX, DeltaTime, r = length(Pos), IterNum = 0;

	// These don't change over the iterations
	double RVMu = dotp(Pos,Vel) / SqrtMu;		// Dot product of position and velocity divided
											// by the squareroot of Mu
	double OneRA = (1 - (r / a));			// One minus Pos over the semi-major axis

	CalcCAndS(Z, C, S);
	T = ((RVMu * pow(X, 2) * C) +  (OneRA * pow(X, 3) * S) + (r * X)) / SqrtMu;

	DeltaTime = Time - T;

	// Iterate while the result isn't within tolerances
	while (fabs(DeltaTime) > EPSILON && IterNum++ < MAX_ITERS) {
		dTdX = ((pow(X, 2) * C) + (RVMu * X * (1 - Z * S)) + (r * (1 - Z * C))) / SqrtMu;

		X = X + (DeltaTime / dTdX);
		Z=(X*X)/a;

		CalcCAndS(Z, C, S);
		T = ((RVMu * pow(X, 2) * C) +  (OneRA * pow(X, 3) * S) + (r * X)) / SqrtMu;

		DeltaTime = Time - T;

	}


}

// Given the specified position and velocity vectors for a given orbit, retuns the position
// and velocity vectors after a specified time
inline void PredictPosVel(const VECTOR3 &Pos, const VECTOR3 &Vel, const double Mu, const double Time, VECTOR3 &NewPos, VECTOR3 &NewVel)
{
	double SqrtMu = sqrt(Mu);
	double v = sqrt(Vel.x*Vel.x+Vel.y*Vel.y+Vel.z*Vel.z);
	double r = sqrt(Pos.x*Pos.x+Pos.y*Pos.y+Pos.z*Pos.z);
	double e = (v*v)/2.0-Mu/r;
	double a = -Mu/(2.0*e);

	// Variables for computation
	double X = (SqrtMu * Time) / a;	// Initial guesses for X
	double Z = (X*X)/a;				// and Z
	double C, S;					// C(Z) and S(Z)
	double F, FDot, G, GDot;

	// Calculate the X and Z for the specified time of flight
	CalcXandZ(X, Z, Pos, Vel, a, Time, SqrtMu);

	// Calculate C(Z) and S(Z)
	CalcCAndS(Z, C, S);

	// Calculate the new position and velocity vectors
	F = 1.0 - (((X*X )/ r) * C);
	G = Time - (((X*X*X) / SqrtMu) * S);
	NewPos = (Pos * F) + (Vel * G);

	FDot = (SqrtMu / (r * length(NewPos))) * X * (Z * S - 1.0);
	GDot = 1.0 - (((X*X) / length(NewPos)) * C);
	NewVel = (Pos * FDot) + (Vel * GDot);
}
*/
//*////////////////////////////////////////////////////////////////////////////////


typedef struct {
	double a;          ///< semi-major axis [m]
	double e;          ///< eccentricity
	double i;          ///< inclination [rad]
	double theta;      ///< longitude of ascending node [rad]
	double Omega;      ///< argument of periapsis [rad]
	double L;          ///< mean longitude at epoch
	double MnA;        ///< mean anomaly  [rad]
	double nu;         ///< true anomaly  [rad]
	double EcA;        ///< Eccentric anomaly  [rad]
	double t0;         ///< time to periapsis [s]
	double T;          ///< period [s]
} OSCELEMS;

inline double GetEccentricAnomaly(const double ecc, const double MeanA, int iter = 10)
{
	double Es;
	double EccA = MeanA;
	if(ecc > 1.0) EccA = PI;
	double eps = 0.000000000000001;
	int k=0;
	for(k=0; k<iter; k++){
		Es = (EccA - (MeanA + ecc*sin(EccA)))/(1.0 - ecc*cos(EccA));
		EccA = EccA - Es;
		if(fabs(Es) < eps) break;
	}
	return EccA;
}
// section 27.1 form orbiter.pdf
inline double ElementsfromStateVectors(const OBJHANDLE href, const VECTOR3 &pos, const VECTOR3 &vel, OSCELEMS &osc, int frame = FRAME_ECL)
{
	if(oapiGetObjectType(href)!=OBJTP_PLANET) return 0.0;

	double bmass = oapiGetMass(href);
	double mu = GGRAV*bmass;

	VECTOR3 R = pos;
	VECTOR3 V = vel;

	if(frame==FRAME_EQU) Ecl2Equ(href, R, V, true);

	// right-handed
	R = _V(R.x, R.z, R.y);
	V = _V(V.x, V.z, V.y);
	double r = length(R);
	double v = length(V);

	VECTOR3 H = crossp(R, V); // specific angular momentum
	double h = length(H);

	VECTOR3 N = _V(-H.y, H.x, 0.0); // node line vector
	double n = length(N);

	// eccentricity
	VECTOR3 E = (R*(v*v-mu/r)-V*dotp(R,V))/mu;	// eccentricity vector
	osc.e = length(E); // eccentricity

	// inclination
	osc.i = acos(H.z/h); // inclination

	// longitude of ascending node (RAAN)
	osc.theta = acos(N.x/n);
	if(N.y < 0.0) osc.theta = PI2 - osc.theta;
	if(osc.i == 0.0) osc.theta = 0.0;

	// argument of periapsis
	if(osc.e != 0.0){
		osc.Omega = acos(dotp(N,E)/(n*osc.e));
		if(osc.i == 0.0) osc.Omega = acos(E.x/osc.e);
		if(E.z < 0.0) osc.Omega = PI2 - osc.Omega;
	}
	else osc.Omega = 0.0;

	// true anomaly
	osc.nu = 0.0;
	if(osc.i == 0.0){
		osc.nu = acos(R.x/r);
		if(V.x > 0.0) osc.nu = PI2 - osc.nu;
	}
	else{
		if(osc.e != 0.0){
			osc.nu = acos(dotp(E,R)/(osc.e*r));
			if(dotp(R,V) < 0.0) osc.nu = PI2 - osc.nu;
		}
		else{
			osc.nu = acos(dotp(N,R)/(n*r));
			if(dotp(N,V) > 0.0) osc.nu = PI2 - osc.nu;
		}
	}

	// semimajor axis
	osc.a = -mu/(2*(v*v/2-mu/r));

	// orbit period
	osc.T = PI2* sqrt(osc.a*osc.a*osc.a/mu);

	double E0 = 2.0*atan(tan(osc.nu/2.0)*sqrt((1.0-osc.e)/(1.0+osc.e)));

	osc.t0 = (E0-osc.e*sin(E0))*osc.T/PI2;

	osc.MnA = osc.t0*PI2/osc.T;

	osc.EcA = GetEccentricAnomaly(osc.e, osc.MnA);

	if(osc.e!=1.0)
		osc.nu = 2.0*atan2(sqrt(1.0+osc.e)*sin(osc.EcA/2.0), sqrt(1.0-osc.e)*cos(osc.EcA/2.0));

//sprintf(oapiDebugString(), "SMa:%.3f  ecc:%.6f  inc:%.3f  LAN:%.3f  AgP:%.3f  MnA:%.3f  TrA:%.3f  EcA:%.3f  t0:%.3f  T:%.3f",
//	osc.a, osc.e, osc.i*DEG, osc.theta*DEG, osc.Omega*DEG, osc.MnA*DEG, osc.nu*DEG, osc.EcA*DEG, osc.t0, osc.T);

	return h;
}

inline double StateVectorsfromElements(const OSCELEMS &osc, const double mu, const double h, VECTOR3 &rpos, VECTOR3 &rvel)
{
//	double r = osc.sma*(1.0-osc.e*cos(osc.EcA));
	double r = h*h/(mu*(1.0+osc.e*cos(osc.EcA)));
	double mu_h = mu/h;

	double ci = cos(osc.i);
	double si = sin(osc.i);
	double ct = cos(osc.theta);
	double st = sin(osc.theta);
	double co = cos(osc.Omega);
	double so = sin(osc.Omega);

	double cno = cos(osc.nu+osc.Omega);
	double sno = sin(osc.nu+osc.Omega);

// left-handed
	rpos = _V(
				cno*ct - sno*st*ci,
				sno*si,
				cno*st + sno*ct*ci
				) * r;
	rvel = _V(
				osc.e*(co*st*ci-so*ct) - sno*ct - cno*st*ci,
				(cno+osc.e*co)*si,
				osc.e*(co*ct*ci-so*st) - sno*st + cno*ct*ci
				) * mu_h;

//sprintf(oapiDebugString(), "rpos:%.3f %.3f %.3f  rvel:%.3f %.3f %.3f  r:%.3f", rpos, rvel, r);

	return r;
}

inline void PredictPosVel(const OBJHANDLE href, ELEMENTS &ovElems, ORBITPARAM &ovParams, const double Time, VECTOR3 &newRpos, VECTOR3 &newRvel, int Nonspherical=1)
{

	if(oapiGetObjectType(href)!=OBJTP_PLANET) return;

	double bmass = oapiGetMass(href);
	double mu = GGRAV*bmass;

	double J2 = oapiGetPlanetJCoeff(href, 0);
	double Re = oapiGetSize(href);
	double n = sqrt(mu / (ovElems.a * ovElems.a * ovElems.a)); // mean motion
	double p = ovElems.a * (1.0 - ovElems.e * ovElems.e); // semi-latus rectum
	double K = (Re/p) * (Re/p);
	double dtheta = -1.5 * n * J2 * K * cos(ovElems.i);
	double dOmega = 0.75 * n * J2 * K * (5 * cos(ovElems.i) * cos(ovElems.i) - 1.0);
	double dMna = n * (1.0 + 0.75 * sqrt(1.0 - ovElems.e * ovElems.e) * J2 * K * (3.0 * cos(ovElems.i) * cos(ovElems.i) - 1.0));

//sprintf(oapiDebugString(), "dtheta:%.3e  dOmega:%.3e  dMna:%.3e", dtheta*DEG, dOmega*DEG, dMna*DEG);

	double h = sqrt(mu*ovParams.PeD*(1.0+ovElems.e));

	OSCELEMS osc = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};

	// static-dependent elements (without perturbations)
	osc.a = ovElems.a;
	osc.e = ovElems.e;
	osc.i = ovElems.i;
	osc.L = ovElems.L;
	osc.T = ovParams.T;

	// time-dependent elements
	osc.t0 = (ovParams.EcA-ovElems.e*sin(ovParams.EcA))*ovParams.T/PI2;
	osc.theta = ovElems.theta + dtheta * Time * Nonspherical;
	osc.Omega = (ovElems.omegab-ovElems.theta) + dOmega * Time * Nonspherical;
//	osc.MnA = ovParams.MnA + Time * PI2 / ovParams.T;
	osc.MnA = ovParams.MnA + Time * (dMna * Nonspherical + ((Nonspherical==0)?1.0:0.0) * PI2 / ovParams.T);

	osc.EcA = GetEccentricAnomaly(ovElems.e, osc.MnA);
	if(osc.e!=1.0)
		osc.nu = 2.0*atan(tan(osc.EcA/2.0)*sqrt((1.0+ovElems.e)/(1.0-ovElems.e)));
	else osc.nu = osc.MnA;

//sprintf(oapiDebugString(), "SMa:%.3f  ecc:%.6f  inc:%.3f  LAN:%.3f (%.3f)  AgP:%.3f (%.3f)  MnA:%.3f  TrA:%.3f (%.3f)  EcA:%.3f  t0:%.3f  T:%.3f",
//	osc.a, osc.e, osc.i*DEG, osc.theta*DEG, ovElems.theta*DEG, osc.Omega*DEG, (ovElems.omegab-ovElems.theta)*DEG, osc.MnA*DEG, osc.nu*DEG, ovParams.TrA*DEG, osc.EcA*DEG, osc.t0, osc.T);

	double r = StateVectorsfromElements(osc, mu, h, newRpos, newRvel);

	// fixed equatorial to ecliptic
	MATRIX3 rot;
	oapiGetPlanetObliquityMatrix(href, &rot);
	newRpos = mul(rot, newRpos);
	newRvel = mul(rot, newRvel);

}

#endif

// END OF FILE