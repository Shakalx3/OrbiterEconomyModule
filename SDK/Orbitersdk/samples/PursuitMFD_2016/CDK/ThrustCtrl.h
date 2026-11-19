///////////////////////////////////////////////////////////////////////////////
//	ThrustCtrl.h
//
//		Defines thrusters fro the vessel according to superstructure CoG.
//    Controls thrusters to reach specific attitude.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef THRUSTCTRL_H
#define THRUSTCTRL_H
#define _CRT_SECURE_NO_DEPRECATE

#include "Orbitersdk.h"
#include "Tools.h"


enum THRUST_TYPE {
	EN_MAIN, EN_RETRO, EN_HOVER,
	TH_PIT, TH_YAW, TH_ROL,
	TH_LAT, TH_VRT, TH_LNG,
	TH_USER
};

class ThrustDef
{
public:
	ThrustDef(VESSEL *V);
	~ThrustDef();

	void UpdateData();

	double rotlinratio;	// share thrusters power between rotation & linear
	double AR01ratio;	// share thrusters power between pitch & bank

	void GetRotLinLevel(VECTOR3 &currentrotlvl, VECTOR3 &currentlinlvl);
	int SetRotLinLevel(VECTOR3 rotlvl, VECTOR3 linlvl);
	int SetRotLevel(VECTOR3 rotlvl);
	int SetLinLevel(VECTOR3 linlvl);
	void NullRotLinLevel();


	double SetAttitude(AXIS Axis, double TargetAttitude, double CurrentAttitude, double RotRate, double AngleTreshold, bool smoothness=false, double time=10.0);
	double SetRotRate(AXIS Axis, double TgtRate, double Angvel, double RateNull);

	VECTOR3 KillRotLvl(VECTOR3 avel, double Rate_Max = 0.005*RAD); // R_DB_FINE
	double KillRotLvl(AXIS Axis, double angvel, double Rate_Max = 0.005*RAD); // R_DB_FINE

	VECTOR3 DefaultRate(bool rot);

	double MaxForce[9];  // Max force ratings (Newtons) for thrusters in each axis
	double MaxTorque[9]; // Max torque ratings (Newton-meters) for thrusters in each axis
	double StructMass; // superstructure mass
	VECTOR3 StructPMI; // superstructure PMI
	int Docked; // how many vessels dock to the current
	bool AR01; // type of rotation method

	VECTOR3 DefRotRate;
	VECTOR3 MaxRotRate;
	VECTOR3 MaxRotRate_tmp;
	VECTOR3 MaxRotRate_docked;
	VECTOR3 MaxRotRate_undocked;
	VECTOR3 MaxLinRate;

	VECTOR3 GetThrusterGroupForceDir(THGROUP_TYPE thgt);

private:
	int GetDockingStatus();
	int numdockedship;
	VESSEL *Ship;

	bool SetThrusterGroupBF();

	// thrusters map
	THRUSTER_HANDLE hAtth[64];
	UINT16 BFAtth[64];
};

double NullRate(AXIS Axis, double Rate, double RateNull, double DT);

const VECTOR3 defaultRotRate = _V(2.5, 2.5, 5.0); // in degree/sec
const VECTOR3 defaultLinRate = _V(0.5, 0.5, 0.5); // in meter/sec

// fixed RPM rate
const double RPM_PITCH_RATE = 0.75*RAD;

// delta angle used to calculate rate max
const double DB_ANGLE = 10.0 * RAD;

// delta angle when NullRate function is applied
const double DB_ULTRA	= 0.001 * RAD; // 3.6 arcsec
const double DB_MIN		= 0.010 * RAD; //  36 arcsec
const double DB_FINE	= 0.050 * RAD; //   3 arcmin
const double DB_NORMAL	= 0.100 * RAD; //   6 arcmin
const double DB_COARSE	= 0.250 * RAD; //  15 arcmin

// rate min when NullRate function is stopped
const double RATE_NULL_HIGH		= 0.050 * RAD; // Two different "null" rates are used. The larger one is for when we have a rotating target vector
const double RATE_NULL_LOW		= 0.010 * RAD; // and the smaller one is for static target vectors.

// rates
const double RATE_MAX	= 10.0 * RAD;
const double R_DB_MAX	= 2.00 * RAD;

const double RATE_HIGH	= 5.00 * RAD;
const double R_DB_HIGH	= 1.00 * RAD;

const double RATE_MID	= 1.00 * RAD;
const double R_DB_MID	= 0.25 * RAD;

const double RATE_LOW	= 0.25 * RAD;
const double R_DB_LOW	= 0.05 * RAD;

const double RATE_FINE	= 0.01 * RAD;
const double R_DB_FINE	= .005 * RAD;

#endif // def THRUSTCTRL_H

// END OF FILE