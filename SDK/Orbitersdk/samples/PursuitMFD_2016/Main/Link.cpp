///////////////////////////////////////////////////////////////////////////////
//
//	Link.cpp
//
///////////////////////////////////////////////////////////////////////////////
#define STRICT

#include "Link.h"


class MFDlnk* ships[MAXSHIPS] = {NULL};

class ThrustDef* VThDef[MAXSHIPS] = {NULL};


// ==============================================================
// creator & destructor
MFDlnk::MFDlnk(char *vesselname, int vidx)
{
	strcpy(vname,vesselname);

	OBJHANDLE hvessel = oapiGetObjectByName(vesselname);
	VESSEL *vesselclass = oapiGetVesselInterface(hvessel);

	VThDef[vidx] = new ThrustDef(vesselclass);

////// MaxRate//////////////////////////////
//	VThDef[vidx]->MaxRotRate = VThDef[vidx]->DefaultRate(true)*RAD;
//	VThDef[vidx]->MaxLinRate = VThDef[vidx]->DefaultRate(false);
////////////////////////////////////////////

	StopHold = false;
	globalAP = 0x0;
	mfdAP = 0x0;

	MFDListCreation(vidx);
}

MFDlnk::~MFDlnk()
{
//	sprintf(oapiDebugString(),"~MFDlnk");
	for(int d=0; d<MAXSHIPS; d++){
		delete MFDxx[d];
		delete ships[d];
		delete VThDef[d];
	}
}

// ==============================================================
// TimeStep function
void MFDlnk::PostStep(double SimT, double SimDT, int vidx)
{
	VThDef[vidx]->UpdateData(); // update thrusters once for all mfd

	DWORD tmpAP = 0x0;
	DWORD tmpID = 0x0;

	for (int k=0;k<MAXMFDS;k++)
		if (MFDxx[k] != NULL){

			if(StopHold){
//				MFDxx[k]->Hold = false;
//				MFDxx[k]->Trim = T_OFF;
//				MFDxx[k]->GO = false;
				MFDxx[k]->StopHoldAndTrim();
			}

			// check AP state in each mfd
			if(MFDxx[k]->Hold){tmpAP |= BFpow(7); tmpID |= BFpow(k);}
			if(MFDxx[k]->Trim != T_OFF){tmpAP |= BFpow(MFDxx[k]->Trim); tmpID |= BFpow(k);}
//////////////////
			if(MFDxx[k]->GO){tmpAP |= BFpow(8); tmpID |= BFpow(k);}

			// update mfd calc
			MFDxx[k]->TimeStep(SimT,SimDT,&globalAP,&mfdAP);
		}

	globalAP = tmpAP; // set value
	mfdAP = tmpID; // set value

	StopHold = false;
}

// ==============================================================
// Linking functions
void MFDlnk::MFDListCreation(int vidx)
{
	for (int k=0;k<MAXMFDS;k++)
		MFDxx[k]=NULL; // No views yet

	ships[vidx]=this; // keep mfd data in memory

}

class MFDlnk *MFDlnk::linkShip(char *vesselname, int vidx)
{
	class MFDlnk *vptr = ships[vidx];

	if (vptr==NULL)
		vptr = new MFDlnk(vesselname,vidx);

	return vptr;
}

void MFDlnk::unlinkShip(int vidx)
{
	ships[vidx] = NULL;
	VThDef[vidx] = NULL;
}

class SubMFD *MFDlnk::linkMFD(int vidx, char *vname, int idx)
{
	if (idx<0 || idx>=MAXMFDS) idx=0;

	if (MFDxx[idx] == NULL)
		MFDxx[idx] = new SubMFD(vname, VThDef[vidx], idx);

	return MFDxx[idx];
}

class SubMFD *MFDlnk::returnMFD(int idx)
{
	return MFDxx[idx];
}

 bool MFDlnk::SwapMFD(int tgtidx, int homidx)
{
	if(MFDxx[homidx] == NULL) return false;
	if(MFDxx[tgtidx] == NULL) return false;

	SubMFD *subtmp = MFDxx[tgtidx];

	MFDxx[tgtidx] = MFDxx[homidx];
	MFDxx[tgtidx]->mfdindex = tgtidx;

	MFDxx[homidx] = subtmp;
	MFDxx[homidx]->mfdindex = homidx;

	return true;
}

// END OF FILE