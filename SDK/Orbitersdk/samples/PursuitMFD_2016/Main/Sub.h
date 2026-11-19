///////////////////////////////////////////////////////////////////////////////
//	Sub.h
//
//    Controls each MFD calculation & display.
//
///////////////////////////////////////////////////////////////////////////////
#define _CRT_SECURE_NO_DEPRECATE

#include "common.h"

#include "..\CDK\ThrustCtrl.h"


class SubMFD
{
public:
	SubMFD(char *vname, ThrustDef *vtd, int idx);
	~SubMFD();

	int mfdindex;

	double DT;

	void TimeStep(double SimT, double SimDT, DWORD *APstate, DWORD *APid);
	void MFDUpd(SKPBOX skpbox);

	OBJHANDLE hves;
	char *vesname;

	OBJHANDLE href;
	char refname[NAMEMAX];
	void setRefName(INPUT_MODE style);
	bool setReference(char *name);
	OBJHANDLE htgt;
	char tgtname[NAMEMAX];
	void setTgtName(INPUT_MODE style);
	bool setTarget(char *name);

	int NoSpherGrav;

//landing///////////////////////////////////
	int Pad;
	bool SetPad(int nd, bool input = true);
	bool setBase(char *name, bool input = true);
	bool setManLoc(double lon, double lat, bool input = true);
	bool setElevation(double elev, bool input = true);
	void setGraph();
	int graphstyle;
	bool setCutOffAlt(double coalt, bool input = true);

	bool refreshclosest;

	LTOUCHDOWN Lfinal;
	bool forceLandState;
	bool applyBrake;
	double CutoffAlt;
////////////////////////////////////////////  !!!!!! no boolean declaration before LPHASE declaration WEIRD !!!!!!!!!!
	LPHASE LandPhase;
	Ldata Lsave;
	static const int Qlngth = 32;
	double quad[Qlngth];
	bool RotaTh;
	bool Hover;
////////////////////////////////////////////
	APHASE AscPhase;
	bool SetTargetAltitude(double alt, bool input = true);
	bool SetTargetInclination(double inc, bool input = true);
	double tgtEquInc;
	double tgtAlt;
	double tgtPitch;
////////////////////////////////////////////
	bool TX;
////////////////////////////////////////////
	bool SetBurnInputVelocity(double vel, int Axis, bool input = true);
	bool SetTimetoBurn(char *str, bool input = true);
	double BurnDate;
	double countdown;
	VECTOR3 BurnInput;
	VECTOR3 dRVel;
	VECTOR3 TgtBurnRvel;
	bool GO;
	bool DataInput;
	double ldRvel;
	double EjD;
	bool circularize;
////////////////////////////////////////////


	// HUD Hookers
//	void DrawHUD(int mode, const HUDPAINTSPEC *hps, oapi::Sketchpad * skp);
//	bool ShouldDrawHUD() const;
//	const char* GetModuleName() const;

	ROLLMODE RollMode;
	void SetRollMode();

	ATTMODE Mode;
	void SetMode(ATTMODE currentmode, ATTMODE nextmode);
	bool ModePage;

	bool Hold;
	void SetHoldMode();
	void StopHoldAndTrim();

	TRIMODE Trim;
	void SetTrimMode(TRIMODE TrimMode);

	void SetRefAttitude();
	bool SetRelAttitude(double deg, AXIS Axis, bool input = true);

	int DockS, DockT;
	bool SelectDock(int nd, bool target);

	VECTOR3 RefAtt; //Reference attitude
	VECTOR3 RelAtt; //Desired attitude relative to the reference attitude

	bool AttVel;
	bool TgtVel;

	bool ISSCtrl;

	bool DockAP;
	bool setDistanceVector(VECTOR3 xyz, DWORD set);
	void SetDockAP();
	VECTOR3 DockXYZ;

	void SetDockNav();

	bool SetISSControl(int confirm);

	DWORD gAPstate;
	DWORD gAPid;
	bool inhibate;

	ThrustDef *Vth;

	VELMODE ORT;
	void SetSubVelMode(VELMODE vmd);

	ATTMETHOD Attmethod;

	double Treshold;

	bool waitsmooth(int iteroffset = 0);

private:

//////////////
	bool debugstring;
	double DebugstringTime;

//////////////
static const int ITER = 5;
//	int iterPYR;
	int iterPYRRate;
//	VECTOR3 PYRTable[ITER];
//	VECTOR3 SmoothingPYR(VECTOR3 PYR);
	VECTOR3 DisplayPYR;
	VECTOR3 DisplayPYRRate;
	VECTOR3 PYRrateTable[ITER];
	VECTOR3 SmoothingPYRRate(VECTOR3 PYRrate);

	bool smoothindic;

	VECTOR3 GetAxisAngleVector();
/////////////

//landing///////////////////////////////////
	void Landing(double DT);
	bool landing;
	DWORD errorBF;
	double StLandAlt;
	double StLandDis;
	double StLandVel;
	double AppDuration;
	double TbeforeApp;
	double TbeforeFinal;
	double reqdV;
	double PredictTraj(double bradius, double gravB, double t, double &max_acc, double &req_dv);
	double BurnCalc(double gravB, VECTOR3 pos, VECTOR3 vel);
	double prevclosest;
	double alignmax;
	LATTINDIC brkctrl;
////////////////////////////////////////////
	void Ascent();
	double FlightPath;
	double Azimuth;
////////////////////////////////////////////
	void DirectBurn();
	double Beco;
	double halfBeco;
	double TRemain;
	double dtremain;
	double fuelplan;
	bool stop;
////////////////////////////////////////////

	VESSEL *vclass;

	double SimTime;

	void Initialize(char *vname, ThrustDef *vtd);

	SKPBOX skpb;
	void DisplayVelocity();
	void DisplayTarget();
	void DisplayDocking();
	void DisplayCurrent();
	void DisplayEI();
	void DataDisplay();
	void ThrusterDisplay();
	void IndicDisplay();
	void DisplayDirectBurn();
	void DisplayAscent();

	VECTOR3 CurAtt;				//Current attitude
	VECTOR3 PitchYawRoll;		//Current PYR angles relative to RelAtt
	VECTOR3 DeltaPYR;	//Change in PYR between calculation updates
	VECTOR3 PitchYawRollRate;	//Rate of change of PYR

	QUAT qAtt;
	QUAT qDeltAtt;
	QUAT qAttRate;
//	QUAT qAtt_NR;
//	QUAT qDeltAtt_NR;

	VECTOR3 GPos;
	VECTOR3 Rpos;
	VECTOR3 Rvel;
	VECTOR3 TgtRpos;
	VECTOR3 TgtRvel;
	double RadialVel;
	VECTOR3 TgtDockRpos;
	VECTOR3 TgtDockRvel;

	AttVar Vel;
	AttVar Tgt;
	AttVar Att;
	AttVar Ent;
//	AttVar Lol;

	MATRIX3 RotMat;

	void SetPYRMethod();

	VECTOR3 CalcPitchYawRollAngles();
//	VECTOR3 CalcPitchYawAngles();
	QUAT CalcRotQuaternion(bool PYR = true);
	void CalcVelocity();
	void CalcRelative();
	void CalcDockRelative(double dt);

	DWORD TrimGroup;
	void TrimUpd(VECTOR3 LinVel);
	double SetTrimAxis(TRIMODE TrimMode, VECTOR3 TgtLinVel);

	VECTOR3 HoldAttitude(double dt);

	VECTOR3 RotLvl;
	VECTOR3 LinLvl;

	int NumDock;

	VECTOR3 DockingTrim();
	VECTOR3 UPosXYZ;
	VECTOR3 TPosXYZ;

	void ISSControl();

};

// END OF FILE