// PursuitMFD Subfunctions
#define STRICT

#include "..\Main\PursuitMFD.h"

#pragma warning( disable : 4800)


// must be share with other MFD
int nves;

void PursuitMFD ::SetCurrentInactive(int mfdidx)
{
	if(current != setShip(oapiGetFocusObject())){

		if(returnShip(current)->globalAP) return;

		RemoveShip(current, true);
		nves--;
	}
}

void PursuitMFD ::SetCurrentHoldOff(int mfdidx)
{
	returnShip(current)->StopHold = true;
}

void PursuitMFD::SwitchPrecision()
{
	SubMFD *sub = returnShip(current)->returnMFD(gMFDid);
	sub->Lfinal = (LTOUCHDOWN)(sub->Lfinal+1);
	if(sub->Lfinal > 2) sub->Lfinal = AWAY;
}

void PursuitMFD::ConfigPage(SKPBOX skpb, char *currentvesname)
{
	skpb = skpinit(skpb);

	skpTxt(skpb,1,1,"PursuitMFD Summary");
	RECT sep = {0,2,36,2};	DrawLine(skpb,sep,1,1);

//	skpTxt(skpb,1,35,"","%02i",(void*)nves,rgb.WHITELIGHT);
	skpTxt(skpb,1,35,"","%02i",(void*)(gMFDid+1),rgb.WHITELIGHT);

// vessel listing////////////////////////////
	RECT sep1 = {0,19,36,19};	DrawLine(skpb,sep1,1,1);
	RECT sep2 = {30,19,30,36};	DrawLine(skpb,sep2,1,1);

	skpb.align = false;

	int offset = 20; // section start, including title
	int endlst = 36; // section end

	skpTxt(skpb,offset,1,"Active vessels:");

	int listoff = offset+1; // list offset
	int listend = (endlst-(endlst-listoff)/3)-listoff; // list end (taking care of alignment shift)

	if(nves<1 || nves>MAXSHIPS) nves = 1; // init
	if(selection>MAXSHIPS) selection = 1; // safety

	if(selection>nves) selection = 1;
	if(selection<1) selection = nves;

	// page listing
	int first = 0;
	for(int p=0;p<MAXSHIPS;p++)
		if(selection>listend){
			first += listend-1;
			if(selection<=first+listend) break;
		}

	int sel = selection+listoff-first;

	nves = 0;

	int curctrl = -1;

	char bufnm[MAXCOL];

	class MFDlnk *ship;

	for(int k=0; k<MAXSHIPS; k++){ // vessel listing

		ship = returnShip(k);
		if(ship == NULL) continue; // vessel not set during closing session

		nves++;

		if(nves<=first) continue; // display next page
		if(nves>first+listend){
			skpTxt(skpb,listoff+1,5,"...",rgb.WHITELIGHT); // next page indicator
//			break; // skip other
			continue; // skip other
		}

		listoff++;
		skpb.align++;

		if(strcmp(ship->vname, currentvesname) == 0){ // show current submfd vessel
			curctrl = k;
			skpTxt(skpb,listoff,3,"*",rgb.WHITEBRIGHT);
//			skpTxt(skpb,listoff,3,"*",COLOR(!(listoff==sel),rgb.WHITELIGHT));
		}

		strunc(bufnm,ship->vname,26);
		if(listoff==sel){ // show current selected vessel
			current = k;
			skpTxt(skpb,listoff,1,"->",rgb.WHITEBRIGHT);
			skpTxt(skpb,listoff,4,bufnm,rgb.WHITEBRIGHT);
		}
		else
			if(first>0 && listoff==offset+2) skpTxt(skpb,listoff,5,"...",rgb.WHITELIGHT); // previous page indicator
			else skpTxt(skpb,listoff,4,bufnm,COLOR(ship->globalAP,rgb.OrangeDark,rgb.WHITELIGHT));

		if(ship->globalAP){
//			skpTxt(skpb,listoff,32,"","%i",(void*)ship[k]->globalAP,rgb.BlueNeon);
			if(ship->globalAP > 128) skpTxt(skpb,listoff,36,"B",rgb.REDBRIGHT);
			if(ship->globalAP > 127) skpTxt(skpb,listoff,32,"H",rgb.BlueNeon);
			if(ship->globalAP != 128) skpTxt(skpb,listoff,34,"T",rgb.RedTomato);
		}

//sprintf(oapiDebugString(),"gMFDid:%i listoff:%i selection:%i sel:%i current:%i",gMFDid,listoff,selection,sel,current);

	}

// current vessel data////////////////////////////
	SubMFD *subcurrent = NULL;
	SubMFD *sub = NULL;

	ship = returnShip(current);

	int s;
	for(s=0; s<MAXMFDS; s++){
		subcurrent = ship->returnMFD(s);
		if(subcurrent != NULL) break;
	}
	if(s==MAXMFDS) return; // safety

	DWORD color = COLOR(current==curctrl,rgb.WHITEBRIGHT,rgb.WHITELIGHT);

	skpb.align=2;
	skpTxt(skpb,3,1,ship->vname,color);
//	sprintf(bufnm,"mass: %.3f kg",subcurrent->Vth->StructMass);
//	skpTxt(skpb,4,1,bufnm,rgb.WHITEBRIGHT);
	sub = ship->returnMFD(gMFDid);

	char buffer[MAXCOL];
	if(sub != NULL){
		char tmp[MAXCOL];
		skpb.align=2;
		switch(sub->Mode){
			case VEL:
				sprintf(buffer,"MFD %02i mode: Velocity",gMFDid+1);
				skpTxt(skpb,7,3,"target: ","%s",sub->tgtname,color);
				break;
			case REL:
				sprintf(buffer,"MFD %02i mode: Relative",gMFDid+1);
				skpTxt(skpb,7,3,"target: ","%s",sub->tgtname,color);
				break;
			case ATT:
				sprintf(buffer,"MFD %02i mode: Attitude",gMFDid+1);
				break;
			case DOC:
				sprintf(buffer,"MFD %02i mode: Docking",gMFDid+1);
				if(sub->DockS<1 || sub->DockT<1 || !oapiIsVessel(sub->htgt))
					skpTxt(skpb,7,3,"target: ","%s",sub->tgtname,color);
				else{
					char buf[MAXCOL];
					strunc(buf,sub->tgtname,20);
					sprintf(tmp,"%s,%02i->%02i",buf,sub->DockS,sub->DockT);
					skpTxt(skpb,7,3,"target: ","%s",tmp,color);
				}
				break;
			case LAN:
				sprintf(buffer,"MFD %02i mode: Landing",gMFDid+1);
				if(sub->Pad<1)
					skpTxt(skpb,7,3,"target: ","%s",sub->tgtname,color);
				else{
					char buf[MAXCOL];
					strunc(buf,sub->tgtname,22);
					sprintf(tmp,"%s - %02i",buf,sub->Pad);
					skpTxt(skpb,7,3,"target: ","%s",tmp,color);
				}
				break;
			case DCT:
				sprintf(buffer,"MFD %02i mode: Direct Burn",gMFDid+1);
				break;
			case ASC:
				sprintf(buffer,"MFD %02i mode: Ascent",gMFDid+1);
				break;
		}
		skpb.align=false;
		skpTxt(skpb,6,3,buffer,color);
	}
	else{
		skpb.align=2;
		sprintf(buffer,"MFD %02i mode: (not set)",gMFDid+1);
		skpTxt(skpb,6,3,buffer,color);
	}

	skpb.align=false;
	skpTxt(skpb,10,2,"max rate:",color);
	sprintf(buffer, "rotation: %.3f %.3f %.3f °/s", subcurrent->Vth->MaxRotRate*DEG);
	skpb.align=2;
	skpTxt(skpb,11,4,buffer,color);
	skpb.align=1;
	sprintf(buffer, "linear:   %.3f %.3f %.3f m/s", subcurrent->Vth->MaxLinRate);
	skpTxt(skpb,13,4,buffer,color);

	skpb.align=2;
	sprintf(buffer, "Landing:");
	skpTxt(skpb,15,2,buffer,color);
	switch(subcurrent->Lfinal){
		case AWAY: sprintf(buffer, "decision:   FAR"); break;
		case CLOSE: sprintf(buffer, "decision:   CLOSE"); break;
		case ABOVE: sprintf(buffer, "decision:   ABOVE"); break;
	}
	skpTxt(skpb,15,11,buffer,color);

	skpb.align=1;
	sprintf(buffer, "cutoff alt: %.1f m", subcurrent->CutoffAlt);
	skpTxt(skpb,17,11,buffer,color);

}

// END OF FILE