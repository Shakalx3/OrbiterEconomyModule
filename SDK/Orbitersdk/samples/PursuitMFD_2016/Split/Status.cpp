// PursuitMFD Subfunctions
#define STRICT

#include "..\Main\PursuitMFD.h"


void PursuitMFD::ReadStatus(FILEHANDLE scn)
{
	char *line = NULL;

	OBJHANDLE hRem = NULL;

	while(oapiReadScenario_nextline(scn, line))
	{
		unsigned int len = strlen(line);

		int idx;
		if(len > 8 && strncmp(line, "MFDIndex ", 9) == 0)
			sscanf(line+9, "%d", &idx);

		// Tell to each MFD which data is assigned
		if(idx!=gMFDid) break;

		submfd->ModePage = false;

		if(len > 7 && strncmp(line, "REMOTE ", 7) == 0){
			char remote[NAMEMAX];
			sscanf(line+7, "%s", &remote);
			hRem = oapiGetObjectByName(remote);
			setMFDFocus(hRem);
		}

		if(len > 8 && strncmp(line, "REFMODE ", 8) == 0){
			if(strncmp(line+8, "VELOCITY", 8) == 0)	submfd->Mode = VEL;
			if(strncmp(line+8, "RELATIVE", 8) == 0)	submfd->Mode = REL;
			if(strncmp(line+8, "GLOBAL", 6) == 0)	submfd->Mode = ATT;
			if(strncmp(line+8, "DOCKING", 7) == 0)	submfd->Mode = DOC;
			if(strncmp(line+8, "ENTRY", 5) == 0)	submfd->Mode = LAN;
			if(strncmp(line+8, "BURNDATA", 8) == 0)	submfd->Mode = DCT;
			if(strncmp(line+8, "ASCENT", 4) == 0)	submfd->Mode = ASC;
		}
		
		if(len > 8 && strncmp(line, "BODYREF ", 8) == 0){
			char refname[NAMEMAX];
			sscanf(line+8, "%s", &refname);
			submfd->setReference(refname);
			submfd->setTarget(refname);
		}

		if(len > 10 && strncmp(line, "TARGETREF ", 10) == 0){
			char tgtname[NAMEMAX];
			sscanf(line+10, "%s", &tgtname);
			submfd->setTarget(tgtname);
			if(submfd->Mode == VEL) submfd->TgtVel = true;
		}

		if(len > 11 && strncmp(line, "DOCKINGREF ", 11) == 0){
			char *split = strtok(line+11,",");	// get the first part of the string
			int S,T;
			sscanf(split, "%d:%d", &S, &T);
			split = strtok(NULL,"\n");			// get the last part of the string
			submfd->setTarget(split);
			submfd->SelectDock(S,false);
			submfd->SelectDock(T,true);
		}

		if(len > 8 && strncmp(line, "PADBASE ", 8) == 0){
			char *split = strtok(line+8,",");	// get the first part of the string
			int P;
			sscanf(split, "%d", &P);
			split = strtok(NULL,"\n");			// get the last part of the string
			if(P==-2){} // XPDR (not implemented yet)
			else if(P==-1){ // manual location
				submfd->Lsave.manloc = true;
				submfd->Pad = -1;
				double lon=0.0, lat=0.0;
				sscanf(split, "%lf %lf", &lon, &lat);
				submfd->setManLoc(lon,lat,false);
			}
			else{
				submfd->setBase(split, false);
				submfd->SetPad(P,false);
			}
		}

		if(len > 10 && strncmp(line, "elevation ", 10) == 0){
			sscanf(line+10, "%lf", &submfd->Lsave.belv);
		}

		if(len > 11 && strncmp(line, "graphstyle ", 11) == 0){
			int r=0; sscanf(line+11, "%d", &submfd->graphstyle);
		}

		if(len > 6 && strncmp(line, "Porig ", 6) == 0){
			int l, r, h;
			sscanf(line+6, "%lf %lf %lf %lf %d %d %d",
				&submfd->Lsave.vlon, &submfd->Lsave.vlat, &submfd->Lsave.tbrg,
				&submfd->Lsave.BST, &l, &r, &h);
			submfd->Lsave.vlon *= RAD;
			submfd->Lsave.vlat *= RAD;
			submfd->Lsave.tbrg *= RAD;
//			if(l==FINAL) l=INITFIN; // forcing final
			submfd->LandPhase = (LPHASE)l;
			submfd->RotaTh = r!=0;
			submfd->Hover = h!=0;
		}

		if(len > 6 && strncmp(line, "Coefs ", 6) == 0){
			sscanf(line+6, "%Le %Le %Le %Le %Le %Le",
				&submfd->quad[0], &submfd->quad[1], &submfd->quad[2], &submfd->quad[3], &submfd->quad[4], &submfd->quad[5]);
		}

		if(len > 6 && strncmp(line, "Const ", 6) == 0){
			sscanf(line+6, "%lf %lf %lf %lf %lf %lf",
				&submfd->quad[6], &submfd->quad[7], &submfd->quad[8], &submfd->quad[9], &submfd->quad[10], &submfd->quad[11]);
		}

		if(len > 6 && strncmp(line, "Displ ", 6) == 0){
			sscanf(line+6, "%lf %lf %lf %lf %lf %lf %lf",
				&submfd->quad[14], &submfd->quad[22], &submfd->quad[23], &submfd->quad[24], &submfd->quad[25], &submfd->quad[26], &submfd->quad[27]);
		}

		if(len > 7 && strncmp(line, "RefAtt ", 7) == 0){
			sscanf(line+7, "%lf %lf %lf", &submfd->RefAtt.data[PITCH], &submfd->RefAtt.data[YAW], &submfd->RefAtt.data[ROLL]);
			submfd->RefAtt *= RAD;
		}

		if(len > 7 && strncmp(line, "RelAtt ", 7) == 0){
			sscanf(line+7, "%lf %lf %lf", &submfd->RelAtt.data[PITCH], &submfd->RelAtt.data[YAW], &submfd->RelAtt.data[ROLL]);
			submfd->RelAtt *= RAD;
		}

		if(len > 9 && strncmp(line, "activeAP ", 9) == 0){
			int H=0, T=7, D=0, A=0, O=0, R=0;
			sscanf(line+9, "%d %d %d %d %d %d", &H, &T, &D, &A, &O, &R);
			if(H) submfd->SetHoldMode();
			if(T) submfd->SetTrimMode((TRIMODE)T);
			if(D) submfd->SetDockAP();
			if(A) submfd->Attmethod = (ATTMETHOD)A;
			if(O) submfd->SetSubVelMode((VELMODE)O);
			submfd->RollMode = (ROLLMODE)R;
		}

		if(len > 8 && strncmp(line, "DockXYZ ", 8) == 0)
			sscanf(line+8, "%lf %lf %lf", &submfd->DockXYZ.x, &submfd->DockXYZ.y, &submfd->DockXYZ.z);

		if(len > 6 && strncmp(line, "Rates ", 6) == 0){
			sscanf(line+6, "%lf %lf %lf %lf %lf %lf",
				&submfd->Vth->MaxRotRate.x, &submfd->Vth->MaxRotRate.y, &submfd->Vth->MaxRotRate.z,
				&submfd->Vth->MaxLinRate.x, &submfd->Vth->MaxLinRate.y, &submfd->Vth->MaxLinRate.z);
			submfd->Vth->MaxRotRate *= RAD;
		}
// not used anymore
		if(len > 9 && strncmp(line, "BurnData ", 9) == 0){
			sscanf(line+9, "%lf %lf %lf %lf %lf %lf %lf %d",
				&submfd->BurnDate,
				&submfd->BurnInput.x, &submfd->BurnInput.y, &submfd->BurnInput.z,
				&submfd->RefAtt.x, &submfd->RefAtt.y, &submfd->RefAtt.z,
				&submfd->GO);
			submfd->RefAtt *= RAD;
			submfd->DataInput = true;
		}

		if(len > 9 && strncmp(line, "BurnData1 ", 10) == 0){
			sscanf(line+9, "%lf %lf %lf %lf %d %d",
				&submfd->BurnDate,
				&submfd->BurnInput.x, &submfd->BurnInput.y, &submfd->BurnInput.z,
				&submfd->GO, &submfd->circularize);
		}
/*
		if(len > 9 && strncmp(line, "BurnData2 ", 10) == 0){
			sscanf(line+9, "%lf %lf %lf %lf %lf %lf %lf %lf %lf",
				&submfd->CurrEjRpos.x, &submfd->CurrEjRpos.y, &submfd->CurrEjRpos.z,
				&submfd->CurrEjRvel.x, &submfd->CurrEjRvel.y, &submfd->CurrEjRvel.z,
				&submfd->NewRvel.x, &submfd->NewRvel.y, &submfd->NewRvel.z);
		}
*/
		if(len > 9 && strncmp(line, "BurnData3 ", 10) == 0){
			sscanf(line+9, "%lf %lf %lf %lf %lf %lf",
				&submfd->TgtBurnRvel.x, &submfd->TgtBurnRvel.y, &submfd->TgtBurnRvel.z,
				&submfd->ldRvel);
		}
		 
		if(len > 8 && strncmp(line, "AscData ", 8) == 0){
			int P=0;
			sscanf(line+8, "%d %lf %lf", &P, &submfd->tgtAlt, &submfd->tgtEquInc);
			if(P) submfd->AscPhase = (APHASE)P;
			submfd->tgtEquInc *= RAD;
		}

		if(len > 8 && strncmp(line, "Landing ", 8) == 0){
			submfd->Lfinal = CLOSE;
			char lprm[6];
			sscanf(line+8, "%s", &lprm);
			if(strncmp(lprm, "FAR", 3) == 0) submfd->Lfinal = AWAY;
			else if(strncmp(lprm, "ABOVE", 5) == 0) submfd->Lfinal = ABOVE;
		}

		if(len > 10 && strncmp(line, "CutoffAlt ", 10) == 0){
			sscanf(line+10, "%lf", &submfd->CutoffAlt);
		}

		if(len > 10 && strncmp(line, "FORCELAND ", 10) == 0){
			sscanf(line+10, "%d", &submfd->forceLandState);
		}

		if(len > 10 && strncmp(line, "LANDBRAKE ", 10) == 0){
			sscanf(line+10, "%d", &submfd->applyBrake);
		}

	}

	// set current vessel as last MFDfocus if Hold set or set focus vessel
	if(submfd->Hold) setMFDFocus(hRem);
	else{
		VESSEL *ves = oapiGetFocusInterface();
		setMFDFocus(ves->GetHandle());
	}
}

// END OF FILE