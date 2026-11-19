///////////////////////////////////////////////////////////////////////////////
//	Link.h
//
//		Interface between Primary source file & sub files.
//    It creates a specific control & a set of MFD for every vessel called.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef __MFDlnk_H
#define __MFDlnk_H
#define _CRT_SECURE_NO_DEPRECATE

#include "PursuitMFD.h"


class MFDlnk
{
public:
	void PostStep(double SimT, double SimDT, int vidx);

	char vname[NAMEMAX];

	static class MFDlnk *linkShip(char *vesselname, int vidx); // Gets ship pointer , creating new if required
	void unlinkShip(int vidx);

	class SubMFD *linkMFD(int vidx, char *vname, int idx);
	class SubMFD *returnMFD(int idx);

	bool SwapMFD(int tgtidx, int homidx);

	bool StopHold;

	DWORD globalAP;
	DWORD mfdAP;

private:
	MFDlnk(char *vesselname, int vidx);
	~MFDlnk();

	class SubMFD *MFDxx[MAXMFDS];

	void MFDListCreation(int vidx);
};

#endif

// END OF FILE