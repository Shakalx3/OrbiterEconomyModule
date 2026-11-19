///////////////////////////////////////////////////////////////////////////////
//	common.h
//
//		Some constants & definitions
//
///////////////////////////////////////////////////////////////////////////////
#ifndef COMMON_H
#define COMMON_H
#define _CRT_SECURE_NO_DEPRECATE

#include "Orbitersdk.h"
#include "..\CDK\Tools.h"

#include "..\CDK\SketchPad.h"


//#define TIMESTEP	0.01 // 100Hz
#define TIMESTEP	0.1 // 10Hz

#define MAXMFDS		32
#define MAXSHIPS	32
#define NAMEMAX		99

//sprintf(oapiDebugString(),"%s", vclass->GetClassNameA());

enum INPUT_MODE {USER, ORBI};

#define NBMODE	7
enum ATTMODE {VEL, REL, ATT, DOC, LAN, DCT, ASC};

enum TRIMODE {T_LAT, T_VERT, T_FA, T_ALL, T_LAT_VERT, T_VERT_FA, T_FA_LAT, T_OFF};

enum LPHASE {WAIT, BRAKE, INITAPP, APPROACH, INITFIN, FINAL, LANDED, UNABLE, INITBRK, LDBRAKE};

enum LTOUCHDOWN {AWAY, CLOSE, ABOVE};

enum LATTINDIC {NONE, PROGRADE, FORWARD, BRAKEPREP};

enum APHASE {WAITING, HOVER, ASCENT, INITCIRC, CIRC};

enum VELMODE {OFF, RPM, TOR};

enum ROLLMODE {CTRLROLL, KILLROLL, NOROLL};

struct AttVar{
	VECTOR3 RefAtt; //Reference attitude
	VECTOR3 RelAtt; //Desired attitude relative to the reference attitude
	OBJHANDLE htgt;
	char tgtname[NAMEMAX];
};

struct Ldata{
	double BST;		// BurnStartTime
	double tbrg;	// target bearing
	double vlat;	// vessel latitude
	double vlon;	// vessel longitude
	double blat;	// target latitude
	double blon;	// target longitude
	double belv;	// target elevation
	int manloc;		// switch between methods
}; // Ascent data defined in WAIT case

#endif

// END OF FILE