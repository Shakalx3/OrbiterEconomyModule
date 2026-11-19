////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	PursuitMFD.h - Header  file for the PursuitMFD class
//
////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __PursuitMFD_H
#define __PursuitMFD_H
#define _CRT_SECURE_NO_DEPRECATE

#include "common.h"

#include "Link.h"

#include "Sub.h"

#include "..\CDK\ThrustCtrl.h"


class PursuitMFD:public MFD2 {
public:
	PursuitMFD(DWORD w, DWORD h, VESSEL *vessel, UINT mfd);
	~PursuitMFD();

	static void DefaultValues();

	void StoreStatus() const;
	void RecallStatus();

	void WriteStatus (FILEHANDLE scn) const;
	void ReadStatus (FILEHANDLE scn);

	bool Update(oapi::Sketchpad *skp);

	static int MsgProc (UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam);
	bool ConsumeKeyBuffered (DWORD key);
	bool ConsumeButton (int bt, int event);
	char *ButtonLabel (int bt);
	int ButtonMenu (const MFDBUTTONMENU **menu) const;

	static int setShip(OBJHANDLE hVes); // search for vessel & save it if needed
	static void RemoveShip(int vidx, bool changeMFDfocus);

	virtual bool setMFDFocus(OBJHANDLE newRem);
	virtual bool setMFDFocus(int vidx);

	class MFDlnk *returnShip(int vidx);

	bool setRotRate(VECTOR3 xyz, DWORD set);
	bool setLinRate(VECTOR3 xyz, DWORD set);
	bool setRate(VECTOR3 xyz, DWORD set);

	bool SwapMFDdialog(int tgtidx);

//Configuration////////////////////////
	bool config;

protected:


private:
	class SubMFD *submfd; // set display for each MFD

	int gMFDid;
	int defMFDix(int mfdix); // set MFD ID for each

	void setCommand(INPUT_MODE style);

//Configuration////////////////////////
	int current;
	int selection;
	void SetCurrentInactive(int mfdidx);
	void ConfigPage(SKPBOX skpbox, char *currentvesname);

	void SetCurrentHoldOff(int mfdidx);

	void PursuitMFD::SwitchPrecision();

	bool MoveMFD(int tgtidx, int homidx);

	bool Orb2016;

};

#endif

// END OF FILE