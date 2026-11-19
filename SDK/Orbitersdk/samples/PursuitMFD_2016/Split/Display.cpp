// PursuitMFD Subfunctions
#define STRICT

#include "..\Main\PursuitMFD.h"

//#include <EnjoLib/IDrawsHUD.hpp>


// ==============================================================
// refresh display  23lines 36columns
void SubMFD::MFDUpd(SKPBOX skpbox)
{

	skpb = skpinit(skpbox);

	if(smoothindic){
		skpTxt(skpb,18,12,"Initializing...",rgb.WHITEBRIGHT);
		skpTxt(skpb,20,18,"%.0f%%",100.0*iterPYRRate/ITER,rgb.WHITEBRIGHT);
		return;
	}

	if(ModePage){
		skpTxt(skpb,10,16,"Welcome");
		skpTxt(skpb,18,14,"Select Mode");
		char builddate[6];
		getBuildDate(builddate);
		skpTxt(skpb,35,18,"PursuitMFD ","%s",builddate,rgb.WHITELIGHT);
		return;
	}

	// for safety, in case of object deletion
	char nm[NAMEMAX];
	oapiGetObjectName(htgt,nm,NAMEMAX);
	if(strcmp(nm,tgtname) != 0) return;
	oapiGetObjectName(href,nm,NAMEMAX);
	if(strcmp(nm,refname) != 0) return;

	char buf[MAXCOL];
	strunc(buf, vesname, 31);
	skpTxt(skpb,1,1,buf,rgb.WHITELIGHT);

	skpTxt(skpb,1,35,"","%02i",(void*)(mfdindex+1),rgb.WHITELIGHT);
	if (hves != oapiGetFocusObject()) skpTxt(skpb,1,1+strlen(buf),"(*)",rgb.WHITELIGHT);

	RECT sep = {0,2,36,2};	DrawLine(skpb,sep,1,1);

	if(ISSCtrl){
		skpTxt(skpb,18,7,"!!! ISS control mode !!!",rgb.REDBRIGHT); //skpTxt(skpb,35,5,"","%s",ISSname,rgb.Brown);
		return;
	}

//	if(Vth->AR01) skpTxt(skpb,35,33,"AR01",rgb.Brown);
//	else skpTxt(skpb,35,29,"not AR01",rgb.Brown);
//	Vth->GetDockingStatus();
//	skpTxt(skpb,33,29,"%0.3f",Vth->StructMass);

	skpb.align = 2;
	if(NumDock) skpTxt(skpb,33,30,"Docked",rgb.WHITELIGHT);
	skpb.align = false;

	skpTxt(skpb,3,1,"Att.Mode :");
	switch(Mode){
		case VEL:	DisplayVelocity(); break;
		case REL:	DisplayTarget(); break;
		case ATT:	DisplayCurrent(); break;
		case DOC:	DisplayDocking(); break;
		case LAN:	DisplayEI(); return;
		case DCT:	DisplayDirectBurn(); break;
		case ASC:	DisplayAscent(); break;
	}
/*
	switch(Attmethod){
		case PYR:	skpTxt(skpb,3,34,"PYR",rgb.Brown); break;
		case PRY:	skpTxt(skpb,3,34,"PRY",rgb.Brown); break;
		case YPR:	skpTxt(skpb,3,34,"YPR",rgb.Brown); break;
		case YRP:	skpTxt(skpb,3,34,"YRP",rgb.Brown); break;
		case RYP:	skpTxt(skpb,3,34,"RYP",rgb.Brown); break;
		case RPY:	skpTxt(skpb,3,34,"RPY",rgb.Brown); break;
	}
*/
	if(Mode!=REL) AttVel=true;
	if(Mode==DOC) AttVel=false;

// Data indicator //////////////////////////////
	DataDisplay();
//	if(Mode==DCT) return;
// Thruster level indicator ////////////////////
	ThrusterDisplay();
// indicators //////////////////////////////////
	IndicDisplay();
}

void SubMFD::DataDisplay()
{
	if(Mode != DCT && Mode != ASC){
		if(AttVel){
	// Set angle indicator /////////////////////////
			skpTxt(skpb,12,1,"Set Pitch:");
			skpTxt(skpb,12,13,"%0+8.3f",RelAtt.data[PITCH]*DEG,COLOR(RelAtt.data[PITCH]<0,rgb.WHITE));
			skpb.align = 1;
			skpTxt(skpb,13,1,"Set Yaw  :");
			skpTxt(skpb,13,13,"%0+8.3f",RelAtt.data[YAW]*DEG,COLOR(RelAtt.data[YAW]<0,rgb.WHITE));
			skpb.align = 2;
			skpTxt(skpb,14,1,"Set Roll :");
			skpTxt(skpb,14,13,"%0+8.3f",RelAtt.data[ROLL]*DEG,COLOR(RelAtt.data[ROLL]<0,rgb.WHITE));
			skpb.align = false;
		}else{
	// Relative velocity ///////////////////////////
			bool trimindic = false;
			if(TrimGroup & BFpow(T_LAT)) trimindic=true;
			skpTxt(skpb,12,1,"Lateral  :",COLOR(trimindic,rgb.RedTomato));
			skpSca(skpb,12,13,TgtRvel.x,COLOR(TgtRvel.x<0,rgb.WHITE));
			skpb.align = 1;
			trimindic = false;
			if(TrimGroup & BFpow(T_VERT)) trimindic=true;
			skpTxt(skpb,13,1,"Vertical :",COLOR(trimindic,rgb.RedTomato));
			skpSca(skpb,13,13,TgtRvel.y,COLOR(TgtRvel.y<0,rgb.WHITE));
			skpb.align = 2;
			trimindic = false;
			if(TrimGroup & BFpow(T_FA)) trimindic=true;
			skpTxt(skpb,14,1,"Fore/Aft :",COLOR(trimindic,rgb.RedTomato));
			skpSca(skpb,14,13,TgtRvel.z,COLOR(TgtRvel.z<0,rgb.WHITE));
			skpb.align = false;
		}
	}else if(Mode == DCT){
		skpb.align = false;
		skpTxt(skpb,11,2,"-dY,dVf :");
		skpSca(skpb,11,13,dRVel.x,COLOR(dRVel.x<0,rgb.WHITE));
		skpb.align = 1;
		skpTxt(skpb,12,2," dZ,dVp :");
		skpSca(skpb,12,13,dRVel.y,COLOR(dRVel.y<0,rgb.WHITE));
		skpb.align = 2;
		skpTxt(skpb,13,2," dX,dVi :");
		skpSca(skpb,13,13,dRVel.z,COLOR(dRVel.z<0,rgb.WHITE));
		skpb.align = false;
	}

// Angle error indicator ///////////////////////
	skpTxt(skpb,17,1,"Pitch:",COLOR(Hold,rgb.BlueNeon));
	skpTxt(skpb,17,13,"%0+8.3f",DisplayPYR.data[PITCH]*DEG,COLOR(DisplayPYR.data[PITCH]<0,rgb.WHITE));
	skpb.align = 1;
	skpTxt(skpb,18,1,"Yaw  :",COLOR(Hold,rgb.BlueNeon));
	skpTxt(skpb,18,13,"%0+8.3f",DisplayPYR.data[YAW]*DEG,COLOR(DisplayPYR.data[YAW]<0,rgb.WHITE));
	skpb.align = 2;
	skpTxt(skpb,19,1,"Roll :",COLOR(Hold,rgb.BlueNeon));
	skpTxt(skpb,19,13,"%0+8.3f",DisplayPYR.data[ROLL]*DEG,COLOR(DisplayPYR.data[ROLL]<0,rgb.WHITE));
	skpb.align = false;
// Angle Rate indicator ////////////////////////
/*	if(Mode==ATT){
		VECTOR3 avel;
		vclass->GetAngularVel(avel);
		skpTxt(skpb,17,25,"%0+8.3f",avel.data[PITCH]*DEG,COLOR(avel.data[PITCH]<0,rgb.WHITE));
		skpb.align = 1;
		skpTxt(skpb,18,25,"%0+8.3f",avel.data[YAW]*DEG,COLOR(avel.data[YAW]<0,rgb.WHITE));
		skpb.align = 2;
		skpTxt(skpb,19,25,"%0+8.3f",avel.data[ROLL]*DEG,COLOR(avel.data[ROLL]<0,rgb.WHITE));
		skpb.align = false;
	}
	else */
		if(Mode!=DOC){ // AttVel
		skpTxt(skpb,17,25,"%0+8.3f",DisplayPYRRate.data[PITCH]*DEG,COLOR(DisplayPYRRate.data[PITCH]<0,rgb.WHITE));
		skpb.align = 1;
		skpTxt(skpb,18,25,"%0+8.3f",DisplayPYRRate.data[YAW]*DEG,COLOR(DisplayPYRRate.data[YAW]<0,rgb.WHITE));
		skpb.align = 2;
		skpTxt(skpb,19,25,"%0+8.3f",DisplayPYRRate.data[ROLL]*DEG,COLOR(DisplayPYRRate.data[ROLL]<0,rgb.WHITE));
		skpb.align = false;
	}

}
void SubMFD::ThrusterDisplay()
{
	RECT sep1 = {0,21,36,21};
	DrawLine(skpb,sep1,1,1);
	RECT sep2 = {26,21,26,36};
	DrawLine(skpb,sep2,1,1);

	VECTOR3 tgtlvlrot=RotLvl;
	VECTOR3 tgtlvllin=LinLvl;
	VECTOR3 lvlrot=NULLVECTOR;
	VECTOR3 lvllin=NULLVECTOR;
	Vth->GetRotLinLevel(lvlrot, lvllin);
	lvlrot /= Vth->rotlinratio;
	lvllin = _V(lvllin.x/Vth->rotlinratio, lvllin.y/Vth->rotlinratio, lvllin.z);
/*
	tgtlvlrot = RotLvl*Vth->rotlinratio;
	if(Vth->AR01) tgtlvlrot = _V(RotLvl.x*Vth->AR01ratio, RotLvl.y, RotLvl.z*(1.0-Vth->AR01ratio));
	tgtlvllin = _V(LinLvl.x*Vth->rotlinratio, LinLvl.y*Vth->rotlinratio, LinLvl.z);
//sprintf(oapiDebugString(),"(%.3f)    tgtlvlrot:%.3f %.3f %.3f  lvlrot:%.3f %.3f %.3f   tgtlvllin:%.3f %.3f %.3f  lvllin:%.3f %.3f %.3f",
//	oapiGetSimTime(), tgtlvlrot, lvlrot, tgtlvllin, lvllin);
	if(lvlrot.x>1.0 || lvlrot.y>1.0 || lvlrot.z>1.0 || lvllin.x>1.0 || lvllin.y>1.0 || lvllin.z>1.0) oapiSetPause(true);
*/


	// Pitch & vertical
	RECT pitch_L = {1,23,3,35};
	RECT pitch_R = {21,23,23,35};
	DrawRect(skpb,pitch_L,1,1,false);
	DrawRect(skpb,pitch_R,1,1,false);
	DrawLevel(skpb,pitch_L,1,1,-lvlrot.x,true);
	DrawLevel(skpb,pitch_R,1,1,lvlrot.x,true);
	DrawLevel(skpb,pitch_L,1,1,lvllin.y,true,rgb.YELLOW);
	DrawLevel(skpb,pitch_R,1,1,lvllin.y,true,rgb.YELLOW);
	DrawLevel(skpb,pitch_L,1,1,-tgtlvlrot.x,false,rgb.BlueNeon);
	DrawLevel(skpb,pitch_R,1,1,tgtlvlrot.x,false,rgb.BlueNeon);
	DrawLevel(skpb,pitch_L,1,1,tgtlvllin.y,false,rgb.RedTomato);
	DrawLevel(skpb,pitch_R,1,1,tgtlvllin.y,false,rgb.RedTomato);
	// yaw & lateral
	RECT yaw_U = {5,22,19,24};
	RECT yaw_D = {5,34,19,36};
	DrawRect(skpb,yaw_U,1,1,false);
	DrawRect(skpb,yaw_D,1,1,false);
	DrawLevel(skpb,yaw_U,1,1,lvlrot.y,true);
	DrawLevel(skpb,yaw_D,1,1,-lvlrot.y,true);
	DrawLevel(skpb,yaw_U,1,1,-lvllin.x,true,rgb.YELLOW);
	DrawLevel(skpb,yaw_D,1,1,-lvllin.x,true,rgb.YELLOW);
	DrawLevel(skpb,yaw_U,1,1,tgtlvlrot.y,false,rgb.BlueNeon);
	DrawLevel(skpb,yaw_D,1,1,-tgtlvlrot.y,false,rgb.BlueNeon);
	DrawLevel(skpb,yaw_U,1,1,-tgtlvllin.x,false,rgb.RedTomato);
	DrawLevel(skpb,yaw_D,1,1,-tgtlvllin.x,false,rgb.RedTomato);
	// Fore/Aft
	RECT fore = {11,25,13,33};
	DrawRect(skpb,fore,1,1,false);
	DrawLevel(skpb,fore,1,1,lvllin.z,true,rgb.YELLOW);
 	DrawLevel(skpb,fore,1,1,tgtlvllin.z,false,rgb.RedTomato);
	// Roll
	ARC roll_L = {18,29,12,2,160*RAD,200*RAD};
	ARC roll_R = {6,29,12,2,20*RAD,-20*RAD};
// body
	roll_L.origin = 180*RAD;
	roll_L.sweep = 160*RAD;
	DrawArc(skpb,roll_L,1,1,false);
	roll_L.origin = 180*RAD;
	roll_L.sweep = 200*RAD;
	DrawArc(skpb,roll_L,1,1,false);
	roll_R.origin = 0*RAD;
	roll_R.sweep = 20*RAD;
	DrawArc(skpb,roll_R,1,1,false);
	roll_R.origin = 0*RAD;
	roll_R.sweep = -20*RAD;
	DrawArc(skpb,roll_R,1,1,false);
//level
	roll_L.origin = 180*RAD;
	roll_L.sweep = roll_L.origin - lvlrot.z*20*RAD;
	DrawArc(skpb,roll_L,1,1,true);
	roll_L.sweep = roll_L.origin - tgtlvlrot.z*20*RAD;
	DrawArc(skpb,roll_L,1,1,false,rgb.BlueNeon);
	roll_R.origin = 0*RAD;
	roll_R.sweep = roll_R.origin - lvlrot.z*20*RAD;
	DrawArc(skpb,roll_R,1,1,true);
	roll_R.sweep = roll_R.origin - tgtlvlrot.z*20*RAD;
	DrawArc(skpb,roll_R,1,1,false,rgb.BlueNeon);
}
void SubMFD::IndicDisplay(){

	if(inhibate){
		int k;
		char buftmp[9];
		for(k=0; k<MAXMFDS;k++) if(gAPid & BFpow(k)) break;
		skpTxt(skpb,27,28,"AP active",rgb.GreenLime);
		sprintf(buftmp,"on mfd %02i",k+1);
		skpTxt(skpb,29,28,buftmp,rgb.GreenLime);
	}
	else{
		if(Hold) skpTxt(skpb,26,33,"HOLD",rgb.BlueNeon);
		skpb.align = 1;
		if(DockAP)
			if(DockS && DockT)
				skpTxt(skpb,27,28,"DOCK",rgb.REDBRIGHT);
			else skpTxt(skpb,27,28,"ZERO",rgb.YELLOW);
		else if(GO) skpTxt(skpb,31,35,"GO",rgb.REDBRIGHT);
		skpb.align = 2;
		if(Trim != T_OFF) skpTxt(skpb,28,33,"TRIM",rgb.RedTomato);
		if(Mode==VEL && TgtVel) skpTxt(skpb,22,30,"FOLLOW",rgb.WHITELIGHT);
	}
	skpb.align = false;

	if(RollMode==KILLROLL) skpTxt(skpb,35,28,"KILL ROLL",rgb.WHITELIGHT);
	else if(RollMode==NOROLL) skpTxt(skpb,35,29,"NO ROLL",rgb.WHITELIGHT);
}

void SubMFD::DisplayVelocity()
{
	skpTxt(skpb,3,12,"Velocity",rgb.WHITEBRIGHT);

	if(ORT)
		if(ORT==RPM) skpTxt(skpb,3,21,"- RPM",rgb.WHITEBRIGHT);
		else if (ORT==TOR) skpTxt(skpb,3,21,"- TOR",rgb.WHITEBRIGHT);

	skpb.align = 2;
	if(TgtVel) skpTxt(skpb,5,1,"Ref.Object: ","%s",tgtname);
	else skpTxt(skpb,5,1,"Ref.Frame:  ","%s",refname);

	skpb.align = false;
	if(href!=vclass->GetSurfaceRef() && href!=vclass->GetGravityRef()){
		skpSca(skpb,8,1,"Distance : ",length(Rpos));
		skpb.align = 2;
	}
	else{
		skpSca(skpb,8,1,"Altitude : ",length(Rpos)-oapiGetSize(href));
		if(TgtVel){
			skpSca(skpb,8,23,"Rpos: ",length(TgtRpos));
			skpb.align = 2;
			skpSca(skpb,9,23,"Rvel: ",length(TgtRvel));
		}
		else{ ELEMENTS el;
			vclass->GetElements(href,el,NULL,0,FRAME_ECL);
			skpTxt(skpb,8,23,"Ecc.: ","%.4f",el.e);
			skpb.align = 2;
			skpSca(skpb,9,23,"Spd.:",vclass->GetAirspeed());
		}
	}

	skpSca(skpb,9,1,"Velocity : ",length(Rvel));
	skpb.align = false;
}

void SubMFD::DisplayTarget()
{
	if(Mode==REL){
		if(htgt!=vclass->GetGravityRef())
			skpTxt(skpb,3,12,"Relative",rgb.WHITEBRIGHT);
		else skpTxt(skpb,3,12,"Surface Relative",rgb.WHITEBRIGHT);
	}

	skpb.align = 2;
	skpTxt(skpb,5,1,"Target: ","%s",tgtname);
	skpb.align = false;

	skpSca(skpb,8,1,"Distance : ",length(TgtRpos));
	skpb.align = 2;
	if(htgt!=vclass->GetGravityRef())
		skpSca(skpb,9,1,"Rad.Vel. : ",RadialVel);
	else skpSca(skpb,9,1,"Hor.Vel. : ",RadialVel);
	skpb.align = false;

	if(oapiIsVessel(htgt)){
		skpTxt(skpb,12,24,"rX:");
		skpSca(skpb,12,29,TgtRpos.x,COLOR(TgtRpos.x<0,rgb.WHITE));
		skpb.align = 1;
		skpTxt(skpb,13,24,"rY:");
		skpSca(skpb,13,29,TgtRpos.y,COLOR(TgtRpos.y<0,rgb.WHITE));
		skpb.align = 2;
		skpTxt(skpb,14,24,"rZ:");
		skpSca(skpb,14,29,TgtRpos.z,COLOR(TgtRpos.z<0,rgb.WHITE));
		skpb.align = false;
	}

	if(Mode == REL){
		skpb.align = 2;
		skpTxt(skpb, 22, 28, "loc frame", rgb.WHITELIGHT);
		skpb.align = false;
	}
}

void SubMFD::DisplayDocking()
{
	DisplayTarget();
	skpTxt(skpb,3,12,"Docking Relative",rgb.WHITEBRIGHT);

	if(oapiGetObjectType(htgt)==OBJTP_VESSEL){
		if(DockS && DockT){
			skpTxt(skpb,8,24,"Local Dock:","%02i",(void*)DockS);
			skpb.align = 2;
			skpTxt(skpb,9,23,"Target Dock:","%02i",(void*)DockT);
			skpTxt(skpb, 22, 28, "tgt frame", rgb.WHITELIGHT);
		}
		else if(RadialVel<0.005){
//			skpTxt(skpb,8,23,"TgtRVel < 0.05");
			skpb.align = 2;
			skpTxt(skpb,9,23,"Rvel");
		}
		else if(!DockS){
			skpTxt(skpb,8,24,"local not set");
			skpb.align = 2;
			skpTxt(skpb,9,23,"Rvel Alignment");
		}
		else if(!DockT){
			if(DockS) skpTxt(skpb,8,24,"Local Dock:","%02i",(void*)DockS);
			else skpTxt(skpb,8,23,"no active dock");
			skpb.align = 2;
			skpTxt(skpb,9,23,"Rvel Alignment");
		}

		if(!(DockS && DockT)) skpTxt(skpb, 22, 28, "loc frame", rgb.WHITELIGHT);

		skpb.align = false;
		skpTxt(skpb,17,24,"pX:");
		skpSca(skpb,17,29,DockXYZ.x,COLOR(DockXYZ.x<0,rgb.WHITE));
		skpb.align = 1;
		skpTxt(skpb,18,24,"pY:");
		skpSca(skpb,18,29,DockXYZ.y,COLOR(DockXYZ.y<0,rgb.WHITE));
		skpb.align = 2;
		skpTxt(skpb,19,24,"pZ:");
		skpSca(skpb,19,29,DockXYZ.z,COLOR(DockXYZ.z<0,rgb.WHITE));
	}
	else{
		skpTxt(skpb,8,23,"Invalid Target",rgb.REDBRIGHT);
//		skpb.align = 2;
//		skpTxt(skpb,9,23,"(Rel Vel mode)");
	}
	skpb.align = false;
}

void SubMFD::DisplayCurrent()
{
	skpTxt(skpb,3,12,"Set Attitude",rgb.WHITEBRIGHT);

	char buf[MAXCOL];
	skpb.align = 2;
	sprintf(buf,"%0+7.2f %0+7.2f %0+7.2f",RefAtt*DEG);
	skpTxt(skpb,6,1,"Ref Att:  ","%s",buf);
	skpb.align = false;
	sprintf(buf,"%0+7.2f %0+7.2f %0+7.2f",CurAtt*DEG);
	skpTxt(skpb,9,1,"Cur Att:  ","%s",buf);
}

void SubMFD::DisplayEI()
{

skpb.align = 1;
skpTxt(skpb,35,32,"%0.2f",DT,rgb.Brown); //sprintf(oapiDebugString(),"%0.5f",DT);

// Infos ////////////////////////////////////////
	skpb.align = false;
	DWORD colst = rgb.WHITEBRIGHT;
	skpTxt(skpb,3,12,"Landing AP",colst);
	if(!Hold && LandPhase!=LDBRAKE) skpTxt(skpb,3,23,"OFF",colst);
	else{
		skpTxt(skpb,3,24,"ON",rgb.BlueNeon);
		colst = rgb.OrangeLight;
	}

	if(applyBrake) skpTxt(skpb,32,1,"BRK ON",rgb.WHITEBRIGHT);
	else skpTxt(skpb,32,1,"NO BRK",rgb.WHITEBRIGHT);

	skpb.align = 1;
	if(Lsave.manloc){
		skpTxt(skpb,5,1,"Manual Location:");
			char buf[MAXCOL];
			sprintf(buf,"%0+6.3f %0+6.3f",Lsave.blon*DEG, Lsave.blat*DEG);
			skpTxt(skpb,5,18,buf,rgb.WHITEBRIGHT);
	}
	else if(oapiGetObjectType(htgt) != OBJTP_SURFBASE && oapiGetObjectType(htgt) != OBJTP_VESSEL){
		skpTxt(skpb,5,1,"Ref.Body: ","%s",refname);
	}
	else{
		skpTxt(skpb,5,1,"Target:");
		if(Pad > 0){
			char buf[MAXCOL];
			strunc(buf, tgtname, 22);
			sprintf(buf,"%s - %02i",buf,Pad);
			skpTxt(skpb,5,9,buf,rgb.WHITEBRIGHT);
		}
		else skpTxt(skpb,5,9,tgtname,rgb.WHITEBRIGHT);
	}
	skpb.align = false;

	double ttb = (oapiGetSimMJD()-Lsave.BST)*86400; // time to burn

// error messages
	char *errormsg;
	errormsg = "no error";
	if(errorBF & BFpow(8))  errormsg = "begin brake:95% max thrust";
	if(errorBF & BFpow(15)) errormsg = "WARNING:trajectory too steep";
	if(errorBF & BFpow(7))  errormsg = "WARNING:trajectory too low";
	if(errorBF & BFpow(2))  errormsg = "critical landing phase";
	if(errorBF & BFpow(1))  errormsg = "no base on gravity ref";
	if(errorBF & BFpow(10)) errormsg = "target vessel not valid";
	if(errorBF & BFpow(11)) errormsg = "target not valid";
	if(errorBF & BFpow(5))  errormsg = "bt negative";
	if(errorBF & BFpow(4))  errormsg = "det negative";
	if(errorBF & BFpow(9))  errormsg = "target vessel not landed";
	if(errorBF & BFpow(16)) errormsg = "eccentricity too high";
	if(errorBF & BFpow(12)) errormsg = "stopping descent";
	if(errorBF & BFpow(3))  errormsg = "not enough thrust";
	if(errorBF & BFpow(14)) errormsg = "no thrust available";
	if(errorBF & BFpow(6))  errormsg = "DANGER:collision trajectory";
	if(errorBF & BFpow(13)) errormsg = "alignment not valid";

	if(LandPhase==LDBRAKE) errormsg = "Touchdown:applying full brakes";
	if(LandPhase==LANDED) errormsg = "Landing complete";

	if(errorBF & 93746){ // error 1,4,5,9,10,11,13,14,16
		skpb.align = 1;
		skpTxt(skpb,22,9,errormsg, rgb.REDBRIGHT);
	}
	else skpTxt(skpb,35,1,errormsg, rgb.WHITELIGHT);

	if(errorBF & BFpow(0)){
		errormsg = "Other MFD AP on";
		skpTxt(skpb,35,21,errormsg, rgb.GreenLime);
	}
//////////////

	skpb.align = false;
	if(errorBF & BFpow(12)) skpTxt(skpb,3,28,"ABORTED",colst);
	else{
		switch(LandPhase){
			case WAIT:
				skpTxt(skpb,3,28,"WAIT",colst);
				if(Hold)
					switch(brkctrl){
						case PROGRADE:  skpTxt(skpb,3,34,"PRO",rgb.Brown); break;
						case FORWARD:   skpTxt(skpb,3,34,"FOR",rgb.Brown); break;
						case BRAKEPREP: skpTxt(skpb,3,34,"BRK",rgb.Brown); break;
				}
				if(!(errorBF & 24624)){ // error 4,5,13
					skpTxt(skpb,14,4,"Burn start time:");
					skpSca(skpb,14,20,-ttb);
//					skpSca(skpb,14,28,0.95*quad[21]-quad[20], rgb.WHITELIGHT);
					skpSca(skpb,14,28,quad[31], rgb.WHITELIGHT); // time to next closest passage
					skpTxt(skpb,16,9,"yzdV:", rgb.WHITEBRIGHT);
					skpSca(skpb,16,14,reqdV, rgb.WHITEBRIGHT);
					skpTxt(skpb,16,23,"Fuel:", rgb.WHITEBRIGHT);
					skpSca(skpb,16,28,fuelplan, rgb.WHITEBRIGHT);
					skpb.align = 1;
					skpTxt(skpb,30,10,"areq:",rgb.RedTomato);
					skpSca(skpb,30,14,quad[18],rgb.RedTomato);
					skpTxt(skpb,30,23,"trig:",rgb.RedTomato);
					skpSca(skpb,30,27,quad[19],rgb.RedTomato);
				}
				else{
					skpTxt(skpb,14,6,"Next closest passage:", rgb.WHITELIGHT);
					skpSca(skpb,14,27,quad[31], rgb.WHITELIGHT);
				}
//				skpb.align = 2;
//				skpTxt(skpb,3,33,"PGR",rgb.OrangeLight);
////////
				skpb.align = 2;
				skpTxt(skpb,12,1,"(max :        )",rgb.WHITELIGHT);
				skpSca(skpb,12,8,alignmax,rgb.WHITELIGHT);
////////
				skpb.align = false;
				break;
			case BRAKE:
				skpTxt(skpb,3,28,"BRAKE",colst);
				if(!(errorBF & 24624)){ // error 4,5,13
					skpTxt(skpb,14,4,"Brake end time:");
					skpSca(skpb,14,19,quad[7]-TbeforeApp-(oapiGetSimMJD()-Lsave.BST)*86400);
				}
				break;
			case APPROACH:
				skpTxt(skpb,3,28,"APPROACH",colst);
				if(!(errorBF & 24624)){ // error 4,5,13
					skpTxt(skpb,14,4,"Approach end time:");
					skpSca(skpb,14,22,quad[7]-TbeforeFinal-(oapiGetSimMJD()-Lsave.BST)*86400);
				}
				break;
			case FINAL:
				skpTxt(skpb,3,28,"FINAL",colst);
				skpTxt(skpb,14,4,"Landing phase");
				break;
			case LDBRAKE:
				skpTxt(skpb,3,27,"TOUCHDOWN",colst);
				skpTxt(skpb,14,4,"Waiting full stop.");
				break;
			case LANDED:
				skpTxt(skpb,3,27,"LANDED",rgb.WHITEBRIGHT);
				skpTxt(skpb,14,4,"Landed.");
				break;
			case UNABLE: skpTxt(skpb,18,8,"! unable to find a solution !",rgb.REDBRIGHT);// return;
		}

		if(LandPhase>WAIT && LandPhase<FINAL && !(errorBF & 48)){
			skpTxt(skpb,16,9,"yzdV:", rgb.WHITEBRIGHT);
			skpSca(skpb,16,14,reqdV, rgb.WHITEBRIGHT);
			skpTxt(skpb,16,23,"Fuel:", rgb.WHITEBRIGHT);
			skpSca(skpb,16,28,fuelplan, rgb.WHITEBRIGHT);
			skpb.align = 1;
			skpTxt(skpb,30,10,"errY:",rgb.RedTomato);
			skpSca(skpb,30,15,quad[23],rgb.RedTomato);
			skpTxt(skpb,30,23,"errZ:",rgb.RedTomato);
			skpSca(skpb,30,28,quad[22],rgb.RedTomato);
			skpb.align = false;
		}
	}

	skpTxt(skpb,8,1,"GrdD :");
	skpSca(skpb,8,8,quad[12]);
	skpTxt(skpb,8,19,"velZ:");
	skpSca(skpb,8,25,quad[13]);
	skpb.align = 2;
	skpTxt(skpb,9,1,"HgtD :");
	skpSca(skpb,9,8,quad[14]);
	skpTxt(skpb,9,19,"velY:");
	skpSca(skpb,9,25,quad[15]);
	skpb.align = 1;
	skpTxt(skpb,11,1,"Algn :");
	skpSca(skpb,11,8,quad[16]);
	skpTxt(skpb,11,19,"velX:");
	skpSca(skpb,11,25,quad[17]);

	skpSca(skpb,28,9,Lsave.belv,rgb.WHITELIGHT);

	skpb.align = 2;
	if(RotaTh) skpTxt(skpb,33,30,"InvMode", rgb.WHITELIGHT);

	skpb.align = false;
	double pitch = vclass->GetPitch();
	double bank = vclass->GetBank();
	if(fabs(bank)>PI05) pitch = PI-pitch;
	skpTxt(skpb,32,10,"cp:",rgb.GREENLIGHT);
	skpTxt(skpb,32,13,"%0+8.3f",pitch*DEG,rgb.GREENLIGHT);
	skpTxt(skpb,32,23,"cr:",rgb.GREENLIGHT);
	skpTxt(skpb,32,26,"%0+8.3f",bank*DEG,rgb.GREENLIGHT);


// Engine ///////////////////////////////////////
	THGROUP_TYPE thgt = THGROUP_MAIN;
	if(Hover){
		skpTxt(skpb,17,1,"Hover",rgb.WHITEBRIGHT);
		thgt = THGROUP_HOVER;
	}
	else skpTxt(skpb,17,1,"Main",rgb.WHITEBRIGHT);

	RECT eng = {1,18,4,29};
	RECT lvl = {1,18,4,40};
	DrawRect(skpb,eng,1,1,false, rgb.WHITEBRIGHT);
	double thrpct = vclass->GetThrusterGroupLevel(thgt);
	DrawLevel(skpb,lvl,1,1,-thrpct,true,rgb.WHITEBRIGHT);
	skpTxt(skpb,30,1,"%.3f",thrpct,rgb.WHITEBRIGHT);


// Graphs ///////////////////////////////////////
	int originX = 7;
	int wdt = 28;
	int originY = 29;
	int hgt = 12;
	int xa = originX*skpb.dX+skpb.oX;
	int xb = (originX+wdt)*skpb.dX+skpb.oX;
	int ya = originY*skpb.dX+skpb.oY;
	int yb = (originY-hgt)*skpb.dX+skpb.oY;
	int width = wdt*skpb.dX;
	int height = hgt*skpb.dX;
	double ratio = (double)wdt/(double)(hgt); // must be not integer

// axis
	oapi::Pen *LpenAxis;
	LpenAxis=oapiCreatePen(1,2,StdColor);
	skpb.skp->SetPen(LpenAxis);

	RECT Xaxis = {originX,originY,originX+wdt,originY};
	DrawLine(skpb,Xaxis,1,1);
	RECT Yaxis = {originX,originY,originX,originY-hgt};
	DrawLine(skpb,Yaxis,1,1);

	oapiReleasePen(LpenAxis);

	skpTxt(skpb,17,7,"y");
	skpb.align = 2;
	skpTxt(skpb,29,35,"z");
	skpb.align = false;


	if(errorBF & 93746) return; // error 1,4,5,9,10,11,13,14,16 --> skip graph


// data
	double Ax = quad[0];
	double Bx = quad[1];
	double Cx = quad[2];
	double x0 = quad[8];
	double vx0 = quad[9];
	double a0 = quad[6];

	double Ay = quad[3];
	double By = quad[4];
	double Cy = quad[5];
	double y0 = quad[10];
	double vy0 = quad[11];

	double bdur = quad[7]; // burn duration

	double xcur = quad[12];		// live
	double ycur = quad[14];		// live

// set graph min & max
	switch(LandPhase){
		case WAIT:
			quad[24] = 0.0; // time offset
		case BRAKE:
		case APPROACH:
			if(!Hold) break;
			if((ycur<0.25*y0*quad[25] || xcur>(quad[27]+(quad[25]*y0*ratio)))){ // change scale events
				quad[24] = ttb;		// time offset
				double xoff = x0 + vx0*ttb + 0.5*a0*ttb*ttb + Ax*ttb*ttb*ttb + Bx*ttb*ttb*ttb*ttb + Cx*ttb*ttb*ttb*ttb*ttb;
				double yoff = y0 + vy0*ttb + Ay*ttb*ttb*ttb + By*ttb*ttb*ttb*ttb + Cy*ttb*ttb*ttb*ttb*ttb;
				quad[25] = yoff/y0;	// ratio y
				quad[26] = xoff/x0;	// ratio x
				quad[27] = xoff;	// xmin
			}
			break;
		case FINAL:
			graphstyle = 3;
		case LANDED:
			break;
	}

	double offsT  = quad[24]; // begin offset time 
	double yratio = quad[25];
	double xratio = quad[26];
	double xmin = quad[27]; // 
//	double ymin = Lsave.belv; // target altitude (no need)
	double ymax = yratio*y0;
	double xmax = 0.0;
	if(graphstyle<2) xmax = xmin+(ymax*ratio); // same scale

//sprintf(oapiDebugString(),"fixed  offsT:%.3f  bdur:%.3f  yratio:%.3f  xratio:%.3f  xmin:%.3f  ymax:%.3f  xmax:%.3f",offsT,bdur,yratio,xratio,xmin,ymax,xmax);

// axis scales
	double yrespix = ymax/height;		// y-axis pixel resolution
	double xrespix = fabs(xmin)/width;	// x-axis pixel resolution
	if(graphstyle<2) xrespix = yrespix; // same scale

// draw scales
	int resy = 50000;
	double dis = yrespix*height;
	dis *= ratio;
	for(int m=50; m<100000; m *= 10){
		if(dis<m)			{ resy = (int)(m/10); break;}
		else if(dis<2*m)	{ resy = (int)(2*m/10); break;}
		else if(dis<5*m)	{ resy = (int)(5*m/10); break;}
	}

	int resx = resy;
	while(width*xrespix/resx>10.0){resx*=2; resy=resx;}

	int xoffset = xa - (int)(xmin/xrespix); // position pixel x=0
	int rx=0, ry=0;

	if(graphstyle==0 || graphstyle==2){
		oapi::Pen *LpenSca;
		LpenSca=oapiCreatePen(1,1,rgb.WHITELIGHT);
		skpb.skp->SetPen(LpenSca);

		for(int r=resx; r<fabs(xmin); r+=resx){
			rx++;
			int xspt = xoffset - (int)(rx*resx/xrespix) - 1;
			if(xspt>xa && xspt<xb){
				skpb.skp->Line(xspt, ya-1, xspt, ya-(int)(width/50));
			}
		}
		for(int r=resy; r<ymax; r+=resy){
			ry++;
			int yspt = ya - (int)(ry*resy/yrespix) - 1;
			if(yspt>yb && yspt<ya){
				skpb.skp->Line(xa+1, yspt, xa+(int)(width/50), yspt);
			}
		}

		oapiReleasePen(LpenSca);
	}


	else if(graphstyle==1 || graphstyle==3){
		oapi::Pen *LpenScalines;
		LpenScalines=oapiCreatePen(2,1,rgb.WHITELIGHT);
		skpb.skp->SetPen(LpenScalines);

		for(int r=resx; r<fabs(xmin); r+=resx){
			rx++;
			int xspt = xoffset - (int)(rx*resx/xrespix) - 1;
			if(xspt>xa && xspt<xb){
				skpb.skp->Line(xspt, ya-1, xspt, yb+1);
			}
		}
		for(int r=resy; r<ymax; r+=resy){
			ry++;
			int yspt = ya - (int)(ry*resy/yrespix) - 1;
			if(yspt>yb && yspt<ya){
				skpb.skp->Line(xa+1, yspt, min(xb+1,xoffset), yspt);
			}
		}

		oapiReleasePen(LpenScalines);
	}


// draw trajectory
	int x, y, n;
	double t = offsT;
	double posx = xmin;
	double posy = ymax;
	double vx = vx0;
	for(n=0; n<=width; n++){
		posx = x0 + vx0*t + 0.5*a0*t*t + Ax*t*t*t + Bx*t*t*t*t + Cx*t*t*t*t*t;
		if(posx>xmax || posx>0.0) break; // break prematurely
		posy = y0 + vy0*t + Ay*t*t*t + By*t*t*t*t + Cy*t*t*t*t*t;
		x = (int)(xb - (((xmax-posx)/(xmax-xmin))*width));
		y = (int)(ya - ((posy/ymax)*height));
		skpb.skp->Pixel(x,y,rgb.WHITE);
		// adapt next timestep to fit trajectory to x-scale
		vx = vx0+a0*t+3.0*Ax*t*t+4.0*Bx*t*t*t+5.0*Cx*t*t*t*t;
		t += xrespix/vx;
	}

// draw current position
	if(LandPhase!=UNABLE && LandPhase!=WAIT){
		x = (int)(xb - (((xmax-xcur)/(xmax-xmin))*width));
		y = (int)(ya - ((ycur/ymax)*height));

		oapi::Pen *LpenPos;
		LpenPos=oapiCreatePen(2,1,rgb.BlueNeon);
		skpb.skp->SetPen(LpenPos);

		skpb.skp->Line(x, y, x, ya);
		skpb.skp->Line(xa, y, x, y);

		oapi::Brush *RbrushPos;
		RbrushPos=oapiCreateBrush(rgb.YELLOWBRIGHT);
		skpb.skp->SetBrush(RbrushPos);

		skpb.skp->Ellipse(x-3, y-3, x+3, y+3);

		oapiReleaseBrush(RbrushPos);

		oapiReleasePen(LpenPos);
	}

// draw target				///// moved due to d3d9 R1 (20160830) bug: CTD when Pixel(int x, int y, DWORD col) is called after this block
	oapi::Pen *LpenTgt;
	LpenTgt=oapiCreatePen(2,1,rgb.REDLIGHT);
	skpb.skp->SetPen(LpenTgt);

	skpb.skp->Line(xoffset, ya-1, xoffset, yb+1);

	oapiReleasePen(LpenTgt);

}

void SubMFD::DisplayDirectBurn()
{

	if(circularize) skpTxt(skpb,3,12,"Orbit Insert",rgb.WHITEBRIGHT);
	else{
		skpTxt(skpb,3,12,"Direct Burn",rgb.WHITEBRIGHT);
		if(TX) skpTxt(skpb,3,24,"(TX sync)",rgb.WHITELIGHT);
	}
	skpb.align = 2;
	double BurnTime = (BurnDate-oapiGetSimMJD())*86400.0;
	double countdown = BurnTime-halfBeco;
	skpSca(skpb,5,1,"TEj:",BurnTime);
	skpSca(skpb,5,23,"TBurn:",countdown,COLOR(countdown<7.0,rgb.REDBRIGHT));
//	skpSca(skpb,13,24,"Fuel:",fuelplan,COLOR((countdown<7.0 && TRemain>0.0),rgb.BlueSky));

	skpb.align = false;
	skpTxt(skpb,7,1,"MJD: ","%.7f",BurnDate);
	skpSca(skpb,7,24,"TRem:",TRemain,COLOR((countdown<7.0 && TRemain>0.0),rgb.BlueSky));

	if(TX && fabs(EjD) > 0.0){
		skpb.align = 2;
		skpTxt(skpb,8,1,"EjD: ","%.2f",EjD,rgb.WHITELIGHT);
	}

	skpb.align = false;
//	skpSca(skpb,11,24,"Ttot:",Beco,COLOR((countdown<7.0 && TRemain>0.0),rgb.BlueSky));
	skpSca(skpb,11,23,"dVRem:",length(dRVel),COLOR((countdown<7.0 && TRemain>0.0),rgb.BlueSky));

	skpb.align = 1;
	skpSca(skpb,12,24,"Fuel:",fuelplan,COLOR((countdown<7.0 && TRemain>0.0),rgb.BlueSky));

	skpb.align = 2;
	skpTxt(skpb,13,30,"%05.1f",100*fuelplan/vclass->GetFuelMass(),COLOR((countdown<7.0 && TRemain>0.0),rgb.BlueSky));
	skpTxt(skpb,13,35,"%",COLOR((countdown<7.0 && TRemain>0.0),rgb.BlueSky));

//sprintf(oapiDebugString(),"GetFuelMass: %.3f",vclass->GetFuelMass());

/*
	skpb.align = false;
	skpTxt(skpb,11,2,"-dY,dVf :");
	skpSca(skpb,11,13,dRvel.x,COLOR(dRvel.x<0,rgb.WHITE));
	skpb.align = 1;
	skpTxt(skpb,12,2," dZ,dVp :");
	skpSca(skpb,12,13,dRvel.y,COLOR(dRvel.y<0,rgb.WHITE));
	skpb.align = 2;
	skpTxt(skpb,13,2," dX,dVi :");
	skpSca(skpb,13,13,dRvel.z,COLOR(dRvel.z<0,rgb.WHITE));
	skpb.align = false;

	skpb.align = false;
	skpTxt(skpb,24,3,"dVf :");
	skpSca(skpb,24,8,dRpos.x,COLOR(dRpos.x<0,rgb.WHITE));
	skpb.align = 1;
	skpTxt(skpb,25,3,"dVp :");
	skpSca(skpb,25,8,dRpos.y,COLOR(dRpos.y<0,rgb.WHITE));
	skpb.align = 2;
	skpTxt(skpb,26,3,"dVi :");
	skpSca(skpb,26,8,dRpos.z,COLOR(dRpos.z<0,rgb.WHITE));
	skpb.align = false;
*/

}

void SubMFD::DisplayAscent()
{

	skpTxt(skpb,3,12,"Ascent",rgb.WHITEBRIGHT);

	char buf[MAXCOL];

	switch (AscPhase){
		case WAITING:	skpTxt(skpb,3,19,"- WAIT",rgb.WHITEBRIGHT); break;
		case HOVER:		skpTxt(skpb,3,19,"- HOVER",rgb.WHITEBRIGHT);break;
		case ASCENT:	skpTxt(skpb,3,19,"- ASCENT",rgb.WHITEBRIGHT);break;
		case INITCIRC:	skpTxt(skpb,3,19,"- WAIT CIRC",rgb.WHITEBRIGHT);break;
		case CIRC:		skpTxt(skpb,3,19,"- CIRC",rgb.WHITEBRIGHT);break;
	}


//	char buf[MAXCOL];
/*
	skpb.align = 2;
	sprintf(buf,"%.3f %.3f %.3f",Rpos/1000);
	skpTxt(skpb,6,1,"rpos: ","%s",buf);
	skpb.align = false;
	sprintf(buf,"%.3f %.3f %.3f",Rvel);
	skpTxt(skpb,9,1,"rvel: ","%s",buf);
*/
/*
	skpb.align = 2;
	sprintf(buf,"%.3f %.3f %.3f",nextRpos/1000);
	skpTxt(skpb,6,1,"npos: ","%s",buf);
	skpb.align = false;
	sprintf(buf,"%.3f %.3f %.3f",nextRvel);
	skpTxt(skpb,9,1,"nvel: ","%s",buf);
*/
/*
	skpb.align = 2;
	sprintf(buf,"%.3f %.3f %.3f",BurnVector/1000);
	skpTxt(skpb,18,1,"vec: ","%s",buf);
	skpb.align = false;
	sprintf(buf,"%.3f %.3f %.3f",BurnInput);
	skpTxt(skpb,21,1,"vel: ","%s",buf);
*/


	skpb.align = 2;
	sprintf(buf,"%0+7.2f",Azimuth*DEG);
	skpTxt(skpb,11,1,"TgtAzimuth: ","%s",buf);

	skpSca(skpb,6,1,"TgtAltitude:",tgtAlt);

	skpb.align = 2;
	skpTxt(skpb,14,1,"tgtPitch :  ","%0+6.3f",tgtPitch*DEG);

	skpb.align = false;
//	sprintf(buf,"%0+7.2f %0+7.2f %0+7.2f",CurAtt*DEG);
//	skpTxt(skpb,13,1,"CurAtt:  ","%s",buf);
	skpTxt(skpb,13,1,"FlightPath: ","%0+6.3f",FlightPath*DEG);

	skpTxt(skpb,8,1,"TgtEquInc:  ","%0+7.3f",tgtEquInc*DEG);

// Thruster level indicator ////////////////////
//	ThrusterDisplay();
// indicators //////////////////////////////////
//	IndicDisplay();

}

// END OF FILE