//////////////////////////////////////////////////////////////////////////////////////////////
//
//	AttCtrl.h - Header file for spacecraft control 
//
//	Copyright Chris Knestrick, 2002
//
//	For a full description of the copyright, please see the file Copyright.txt
//
// 15-Jun-05 rbd	For Orbiter 2005 and Visual C++ 7 (VS.NET 2005), renamed this H
//					file to AttCtrl.h to avoid conflict with standard control.h file.
//					Add void return type to NormalizeThrustLevel() to avoid warning.
// 16-Jun-05 rbd	New call for control focus changes to init forces/torques/PMI
///////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CONTROL_H
#define CONTROL_H
#define _CRT_SECURE_NO_DEPRECATE
#include <math.h>
#include "Orbitermath.h"

// The deadband for attitude adjustments.  These are just the "standard" deadband limits - a user
// can pass any deadband value to SetAttitude().  For example, a 5 degree deadband: 
// SetAttitude(TargetYaw, CurrentYaw, YAW, DEADBAND(Radians(5)))
typedef double DEADBAND;

// CCK

const DEADBAND DB_MIN = Radians(0.01),
			   DB_FINE = Radians(0.05), 
			   DB_NORMAL = Radians(0.5), 
			   DB_COARSE = Radians(1.0);
const double RATE_MAX = Radians(5.0);
const double DEADBAND_MAX = Radians(10.0);
const double RATE_HIGH = Radians(1.0);
const double DEADBAND_HIGH = Radians(3.0);
const double RATE_MID = Radians(0.3);
const double DEADBAND_MID = Radians(1.0);
const double RATE_LOW = Radians(0.1);
const double DEADBAND_LOW = Radians(0.15);
const double RATE_FINE = Radians(0.01);
const double RATE_NULL_HIGH = Radians(0.005); // Two different "null" rates are used. The larger one is for when we have a rotating target vector
const double RATE_NULL_LOW = Radians(0.0001); // and the smaller one is for static target vectors.
const double RPM_PITCH_RATE = Radians(0.75);
const double RATE_SLOPE = 5.0;

//CCK End

// Provide ease of access to vectors (e.g. Status.vrot.data[PITCH])
enum AXIS {PITCH, YAW, ROLL};

// Prototypes
void InitAttControl(VESSEL* V);
bool SetAttitude(double TargetAttitude, double CurrentAttitude, double RotRate, 
				 AXIS Axis, DEADBAND DeadbandLow, double RateNull, double DT);
bool SetAttitude(double TargetAttitude, double CurrentAttitude, AXIS Axis, DEADBAND DeadbandLow, double RateNull, double DT);
bool NullRate(AXIS Axis, double Rate, double RateNull, double DT);

static inline void NormalizeThrustLevel(double& Level)
{
	if (Level > 1.0) {
		Level = 1.0;
	} else if (Level < -1.0) {
		Level = -1.0;
	}
}

#endif

