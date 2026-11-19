///////////////////////////////////////////////////////////////////////////////
//
//			PursuitMFD.cpp - Primary source file for the PursuitMFD class
//
//
//	Attempt to make a prototype of multiMFD, vessel independant
//
//
///////////////////////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN

#define STRICT
#define ORBITER_MODULE
#define _CRT_SECURE_NO_DEPRECATE

#include "PursuitMFD.h"


// =============================================================================
// global values : initialize when Launchpad is open, be careful
static struct global{
	int mode;			// MFD register
	int numMFD;			// number of current MFD
	bool online;		// detect if PursuitMFD called once
	UINT32 BFinit;		// bits-field for updating state
	UINT32 BFstore;		// bits-field for MFD stored
	double NextTime;	// control update rate
	double LastTime;	// check backward time
	int MFDid[MAXMFDS];	// list of MFD ID (useful for externals)
	int MFDext;			// number of external MFD
}global;

static struct data{
	class MFDlnk *ships[MAXSHIPS];	// MFD class for each vessel
	OBJHANDLE current[MAXMFDS];		// current remote
	UINT32 BFvalid;					// bits-field for safe checking
}data;

// =============================================================================
// initialize global values
void PursuitMFD::DefaultValues()
{
	global.numMFD	= 0;
	global.online	= false;
	global.BFinit	= 0x0;
	global.BFstore	= 0x0;
	global.NextTime	= 0.0;
	global.LastTime	= 0.0;
	global.MFDext	= 0;

	for (int k=0;k<MAXMFDS;k++){
		data.current[k] = NULL;
		global.MFDid[k] = k;
	}

	for (int k=0;k<MAXSHIPS;k++){
		RemoveShip(k,false);
	}
	data.BFvalid = 0x0;
}

// =============================================================================
// DLLCLBK global functions : avoid the unwanted calls
DLLCLBK void InitModule (HINSTANCE hDLL) // called when Launchpad is open
{
	PursuitMFD::DefaultValues();

	static char *name ="PursuitMFD";
	MFDMODESPECEX spec;
	spec.name		= name;
	spec.key		= OAPI_KEY_U;
	spec.context	= NULL;
	spec.msgproc	= PursuitMFD::MsgProc;

	global.mode = oapiRegisterMFDMode(spec);
}

DLLCLBK void ExitModule (HINSTANCE hDLL) // called when Launchpad is close
{
	//	sprintf(oapiDebugString(),"ExitModule");
	oapiUnregisterMFDMode(global.mode);
}

DLLCLBK void opcCloseRenderViewport() // called before StoreStatus()
{
// reinit data before closing simulation
// except global.store called after in StoreStatus()
	PursuitMFD::DefaultValues();
}

DLLCLBK void opcPreStep(double SimT, double SimDT, double MJD) // called in every step even whenever close or open, before updating states
{
	global.online = ((global.numMFD > 0) || global.online) ? true:false; // online notification

//sprintf(oapiDebugString(),"Online:%i BFinit:%i BFstore:%i numMFD:%i NextTime:%.2f",global.online,global.BFinit,global.BFstore,global.numMFD,global.NextTime);

//	if (!global.online) return; // MFD class never open --> skip
////////////////////
//	if (SimT < global.NextTime && SimT>global.LastTime) return; // don't update now
////////////////////
//	if(global.BFinit) return; // waiting for a state update
}

DLLCLBK void opcPostStep(double SimT, double SimDT, double MJD) // called in every step even whenever close or open, after updating states
{
	if (!global.online) return; // MFD class never open --> skip

////////////////////
	if (SimT < global.NextTime && SimT>global.LastTime) return; // don't update now
	global.LastTime = SimT; // last processed time
	global.NextTime = SimT + TIMESTEP; // next date for update, independant of TimeAcceleration
////////////////////

//	if(global.BFinit) return; // waiting for a state update (general)

	for(int k=0;k<MAXSHIPS;k++){
		if(global.BFinit & BFpow(k)) return; // waiting for a state update
		if (data.BFvalid & BFpow(k)) // valid vessel
				data.ships[k]->PostStep(SimT,SimDT,k); // for (fast) calculations in background, independant of MFD RefreshRate
	}
}

DLLCLBK void opcDeleteVessel(OBJHANDLE hVessel)
{
	if (!global.online) return; // MFD class never open --> skip

	// delete ship in the list
	int k = PursuitMFD::setShip(hVessel);
	PursuitMFD::RemoveShip(k, true);
}

DLLCLBK void opcFocusChanged(OBJHANDLE newfocus, OBJHANDLE oldfocus) // also called when open simulation
{
// first initialisation before opening render
	if (oldfocus == NULL) 
		PursuitMFD::DefaultValues();

	if (!global.online) return; // MFD never open --> skip

	global.BFstore = 0x0;

//	for(int k=0;k<global.numMFD;k++)// check open MFDs only
//		global.BFinit |= BFpow(k);	// re-init MFDs

	global.BFinit = 0x0;// check every MFDs

}

// =============================================================================
// creation & deletion functions
PursuitMFD::PursuitMFD (DWORD w, DWORD h, VESSEL *vessel, UINT mfd)
: MFD2 (w, h, vessel)
{
	gMFDid = defMFDix(mfd); // assign ID to mfd

	global.numMFD++; // another MFD open

	if(!setMFDFocus(data.current[gMFDid]) || !(global.BFstore & BFpow(gMFDid)))
		setMFDFocus(vessel->GetHandle()); // load remote stored or set focus

	if (global.BFinit & BFpow(gMFDid))
		global.BFinit ^= BFpow(gMFDid); // end of init

///////////////////////
	config = false;

	Orb2016 = (oapiGetOrbiterVersion() > 150000);
}

void PursuitMFD::StoreStatus() const // also called when simulation closing or changing focus
{
	if (!(global.BFinit & BFpow(gMFDid)))	// skip if changing focus
		global.BFstore |= BFpow(gMFDid);	// register MFD in bits-field
	
	global.numMFD--; // another MFD destroy
}

void PursuitMFD::RecallStatus() // also called in MFD first creation except open render
{
	if (global.BFstore & BFpow(gMFDid))	// check if already store
		global.BFstore ^= BFpow(gMFDid);// unregister MFD in bits-field
}

PursuitMFD::~PursuitMFD() // call when power off or change module
{

}
// =============================================================================
// MFD functions
int PursuitMFD::defMFDix(int mfdix) // return useful external MFD index
{
	int ID = mfdix;

	if (mfdix>=MAXMFD || mfdix<=-MAXMFD){ // external MFD
		for (ID=10; ID<MAXMFDS; ID++)
			if (mfdix == global.MFDid[ID]) break; // open MFD already store

		if (ID>=MAXMFDS){ // no MFD stored found -> assign new ID
			if(global.MFDext>= MAXMFDS) global.MFDext = 0; // no more than 32 MFDs
			ID = global.MFDext + MAXMFD;
			global.MFDid[ID] = mfdix;
			global.MFDext++;
		}
	}

	return ID;
}

int PursuitMFD::setShip(OBJHANDLE hVes) // register ship to list 
{
	if(!oapiIsVessel(hVes)) return -1;

	int k;
	char name[NAMEMAX];

//check if its useful vessel...
	//if (!hVes->GetEnableFocus()) return NULL;//for example

//ok, a good one, lets check if it was already saved...
	oapiGetObjectName(hVes,name,NAMEMAX);

	for (k=0;k<MAXSHIPS;k++){
		if (!(data.BFvalid & BFpow(k))) continue; // safety
		if (strcmp(data.ships[k]->vname,name)==0) return k; // already saved
	}

// lets save at the right place...
	for (k=0;k<MAXSHIPS;k++){
		if (!(data.BFvalid & BFpow(k))){	// found a free slot
			data.ships[k] = MFDlnk::linkShip(name,k); // create link
			data.BFvalid |= BFpow(k);		// register to list
			return k;
		}
	}

	return -1; // no more free slot or vessel not found
}

void PursuitMFD::RemoveShip(int vidx, bool changeMFDfocus) // Remove ship to list 
{
	if(changeMFDfocus)
		for(int c=0;c<MAXMFDS;c++)
			if (data.current[c] == oapiGetObjectByName(data.ships[vidx]->vname)) // deleted vessel is in use
				global.BFinit |= BFpow(c); // set to focus


	data.ships[vidx]-> unlinkShip(vidx);
	data.ships[vidx] = NULL;
	if(data.BFvalid & BFpow(vidx)) data.BFvalid ^= BFpow(vidx);
}

bool PursuitMFD::setMFDFocus(OBJHANDLE newRem)
{
	if(!oapiIsVessel(newRem)) return false; // invalid object

	int vidx = setShip(newRem); // link ship /////
	if (vidx == -1) return false;

	if (data.BFvalid & BFpow(vidx)){ // check ship BFvalidity
		submfd = NULL;
		int subidx = gMFDid; // link each submfd to its gMFDid
//		if (newRem != oapiGetFocusObject()) subidx = 0; // remote have only one mfd active
		submfd = data.ships[vidx]->linkMFD(vidx, data.ships[vidx]->vname, subidx); // create pointer to display
		data.current[gMFDid] = newRem; // store status
		return true;
	}

	return false;
}

bool PursuitMFD::setMFDFocus(int vidx)
{
	if (data.BFvalid & BFpow(vidx)){ // check ship BFvalidity
		submfd = NULL;
		int subidx = gMFDid; // link each submfd to its gMFDid
//		if (newRem != oapiGetFocusObject()) subidx = 0; // remote have only one mfd active
		submfd = data.ships[vidx]->linkMFD(vidx, data.ships[vidx]->vname, subidx); // create pointer to display
		data.current[gMFDid] = oapiGetObjectByName(data.ships[vidx]->vname); // store status
		return true;
	}

	return false;
}

bool PursuitMFD::MoveMFD(int tgtidx, int homidx)
{
	if(tgtidx==homidx) return false;

	data.ships[current]->linkMFD(current, data.ships[current]->vname, homidx);
	data.ships[current]->linkMFD(current, data.ships[current]->vname, tgtidx);

	global.BFinit |= BFpow(homidx);
	global.BFinit |= BFpow(tgtidx);

	return data.ships[current]->SwapMFD(tgtidx,homidx);
}

class MFDlnk *PursuitMFD::returnShip(int vidx)
{
	return data.ships[vidx];
}


// =============================================================================
// Orbiter callback routine for screen updates only when MFD open
// depends on refresh rate setting
bool PursuitMFD::Update(oapi::Sketchpad *skp) // refresh rate set in Launchpad parameters
{
//sprintf(oapiDebugString(),"%i", oapiGetOrbiterVersion());

	if (!global.online) return true;

	if (global.BFinit & BFpow(gMFDid)){		// state update notification
		setMFDFocus(oapiGetFocusObject());	// state update
		global.BFinit ^= BFpow(gMFDid);		// unregister MFD in bits-field
//		return true;//////////////
	}

	if(!oapiIsVessel(submfd->hves)) return true; // safety

//display refresh for each instance////////////////
	SKPBOX skpbox;
	skpbox.skp  = skp;
	skpbox.MFDw = W;
	skpbox.MFDh = H;

	if (config) ConfigPage(skpbox, submfd->vesname);
	else if (submfd!=NULL) submfd->MFDUpd(skpbox);
//end display//////////////////////////////////////

	if(strncmp(oapiGetFocusInterface()->GetClassNameA(), "UCGO\\Vessels\\UCGOArrowFreighter", 31) != 0)
		InvalidateButtons();

	return true;
}

// =============================================================================
// WriteStatus function moved here for vessel listing
void PursuitMFD::WriteStatus(FILEHANDLE scn) const
{
	char str[150];
	char idx[2];

	sprintf(idx, "%i", gMFDid);
	oapiWriteScenario_string(scn, "MFDIndex", idx);

	for(int k=0; k<32; k++){ // vessel listing

		if(data.ships[k] == NULL) continue; // vessel not set during closing session

		SubMFD *tmpsub = data.ships[k]->returnMFD(gMFDid); // check current submfd
		if(tmpsub == NULL) continue; // skip

		oapiWriteScenario_string(scn, " REMOTE", tmpsub->vesname);

		switch(tmpsub->Mode){
			case VEL: oapiWriteScenario_string(scn, "  REFMODE", "VELOCITY");
				oapiWriteScenario_string(scn, "   BODYREF", tmpsub->refname);
				if(tmpsub->TgtVel) oapiWriteScenario_string(scn, "   TARGETREF", tmpsub->tgtname);
				break;

			case REL: oapiWriteScenario_string(scn, "  REFMODE", "RELATIVE");
				oapiWriteScenario_string(scn, "   TARGETREF", tmpsub->tgtname);
				break;

			case ATT: oapiWriteScenario_string(scn, "  REFMODE","GLOBAL");
//				sprintf(str, "%0+8.3f %0+8.3f %0+8.3f", tmpsub->RefAtt*DEG);
//				oapiWriteScenario_string(scn, "   RefAtt", str);
				break;

			case DOC: oapiWriteScenario_string(scn, "  REFMODE", "DOCKING");
				sprintf(str, "%i:%i,%s", tmpsub->DockS, tmpsub->DockT, tmpsub->tgtname);
				oapiWriteScenario_string(scn, "   DOCKINGREF", str);
				break;

			case LAN: oapiWriteScenario_string(scn, "  REFMODE", "ENTRY");
				if(tmpsub->Pad==-1)
					sprintf(str, "%i,%0+11.6f %0+11.6f", tmpsub->Pad, tmpsub->Lsave.blon*DEG, tmpsub->Lsave.blat*DEG);
				else sprintf(str, "%i,%s", tmpsub->Pad, tmpsub->tgtname);
				oapiWriteScenario_string(scn, "   PADBASE", str);

				sprintf(str, "%0.1f", tmpsub->Lsave.belv);
				oapiWriteScenario_string(scn, "   elevation", str);

				sprintf(str, "%i", tmpsub->graphstyle);
				oapiWriteScenario_string(scn, "   graphstyle", str);

				sprintf(str, "%0+8.6f %0+8.6f %0+8.6f %.6f %i %i %i",
					tmpsub->Lsave.vlon*DEG, tmpsub->Lsave.vlat*DEG, tmpsub->Lsave.tbrg*DEG,
					tmpsub->Lsave.BST, tmpsub->LandPhase, tmpsub->RotaTh, tmpsub->Hover);
				oapiWriteScenario_string(scn, "   Porig", str);

				sprintf(str, "%.6e %.6e %.6e %.6e %.6e %.6e",
					tmpsub->quad[0], tmpsub->quad[1], tmpsub->quad[2], tmpsub->quad[3], tmpsub->quad[4], tmpsub->quad[5]);
				oapiWriteScenario_string(scn, "   Coefs", str);

				sprintf(str, "%.3f %.3f %.3f %.3f %.3f %.3f",
					tmpsub->quad[6], tmpsub->quad[7], tmpsub->quad[8], tmpsub->quad[9], tmpsub->quad[10], tmpsub->quad[11]);
				oapiWriteScenario_string(scn, "   Const", str);

				sprintf(str, "%.3f %.3f %.3f %.3f %.3f %.3f %.3f",
					tmpsub->quad[14], tmpsub->quad[22], tmpsub->quad[23], tmpsub->quad[24], tmpsub->quad[25], submfd->quad[26], submfd->quad[27]);
				oapiWriteScenario_string(scn, "   Displ", str);
				break;

			case DCT: oapiWriteScenario_string(scn, "  REFMODE","BURNDATA");
				if(tmpsub->TX){
					tmpsub->Hold = false;
					tmpsub->GO = false;
				}
				sprintf(str, "%.10f %.3f %.3f %.3f %d %d",
					tmpsub->BurnDate, tmpsub->BurnInput, tmpsub->GO, tmpsub->circularize);
				oapiWriteScenario_string(scn, "   BurnData1", str);
//				sprintf(str, "%.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f",
//					tmpsub->CurrEjRpos, tmpsub->CurrEjRvel, tmpsub->TgtBurnRvel);
//				oapiWriteScenario_string(scn, "   BurnData2", str);
				sprintf(str, "%.3f %.3f %.3f %.3f",
					tmpsub->TgtBurnRvel, tmpsub->ldRvel);
				oapiWriteScenario_string(scn, "   BurnData3", str);
				break;

			case ASC: oapiWriteScenario_string(scn, "  REFMODE","ASCENT");
				sprintf(str, "%i %.3f %.3f", tmpsub->AscPhase, tmpsub->tgtAlt, tmpsub->tgtEquInc*DEG);
				oapiWriteScenario_string(scn, "   AscData", str);
				break;
		}

		sprintf(str, "%0+8.6f %0+8.6f %0+8.6f", tmpsub->RefAtt*DEG);
		oapiWriteScenario_string(scn, "   RefAtt", str);

		sprintf(str, "%0+8.6f %0+8.6f %0+8.6f", tmpsub->RelAtt*DEG);
		oapiWriteScenario_string(scn, "   RelAtt", str);

		sprintf(str, "%i %i %i %i %i %i", tmpsub->Hold, tmpsub->Trim, tmpsub->DockAP, tmpsub->Attmethod, tmpsub->ORT, tmpsub->RollMode);
		oapiWriteScenario_string(scn, "   activeAP", str);

		sprintf(str, "%.3f %.3f %.3f", tmpsub->DockXYZ);
		oapiWriteScenario_string(scn, "   DockXYZ", str);

		sprintf(str, "%.3f %.3f %.3f %.3f %.3f %.3f", tmpsub->Vth->MaxRotRate*DEG, tmpsub->Vth->MaxLinRate);
		oapiWriteScenario_string(scn, "   Rates", str);

		switch(tmpsub->Lfinal){
			case AWAY: oapiWriteScenario_string(scn, "   Landing", "FAR"); break;
			case CLOSE: oapiWriteScenario_string(scn, "   Landing", "CLOSE"); break;
			case ABOVE: oapiWriteScenario_string(scn, "   Landing", "ABOVE"); break;
		}
		sprintf(str, "%.2f", tmpsub->CutoffAlt);
		oapiWriteScenario_string(scn, "   CutoffAlt", str);

		sprintf(str, "%i", tmpsub->forceLandState);
		oapiWriteScenario_string(scn, "   FORCELAND", str);

		sprintf(str, "%i", tmpsub->applyBrake);
		oapiWriteScenario_string(scn, "   LANDBRAKE", str);

	}
}

// =============================================================================
// END OF FILE