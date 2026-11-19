/**************************************************************************
 * JumpDriveMFD V1.0 - FTL travel in a difficult fashion for Orbiter 2010P1
 *     created: July, 6th 2006
 * last update: May, 20th 2014
 * Copyright (c) 2006-2014 Friedrich Kastner-Masilko, M.Sc.
 *
 * JumpDriveMFD class
 **************************************************************************/

// Standard module defines
#define STRICT
#define ORBITER_MODULE

// MFD sounds
#define ENGAGEWAV		"Sound\\JumpDriveMFD\\engage.wav"
#define CANCELWAV		"Sound\\JumpDriveMFD\\cancel.wav"
#define INIFILE			"Modules\\Plugin\\JumpDriveMFD.ini"

// Plugin defines
#define ENGAGE			0
#define CANCEL			1
#define MODES			10
#define MAXCHARS		255
#define MAXFOV			RAD * 45

// Includes
#include "windows.h"
#include "orbitersdk.h"
#include "mfdsoundsdk40.h"
#include "JumpDrive.h"
#include "JumpDriveMFD.h"
#include <stdio.h>
#include <map>

// Global variables
int g_MFDmode;				// identifier for new MFD mode
int g_Sound;				// MFD mode sound handle
std::map< VESSEL*, JumpDrive* > g_JumpDrives; // static hash table for drive instances
std::map< VESSEL*, std::map< int, JumpDriveMFDData* > > g_JumpDriveMFDs; // static hash table for MFD instances
Environment g_Environment;	// static environment structure
oapi::Brush *myBrush;		// used for drawing

// Configurable settings
double JUMPVEL			= 5E4;
double FLASHOFFSET		= 1E1;
double JUMPPOS			= 7.5E8;
double JUMPSCALE		= 1E1;
double RELOADTIME		= 1E1;
double CAPACITY			= 6E3;
   int CELLS			= 5;
double FLEETRANGE		= 1E5;
 char  g_JumpDriveName[MAXCHARS];

// API interface

//This is the module start callback. After the module loaded, the INI file will be loaded
//(if available) and all global variables are set. It then registers the MFD mode.
DLLCLBK void InitModule (HINSTANCE hDLL)
{	
	strcpy(g_JumpDriveName, "S C O U T");
	char value[MAXCHARS];
	value[0]=0;
	GetPrivateProfileString("Effects", "JumpVelocity", 0, value, MAXCHARS, INIFILE);
	if (value[0]!=0) JUMPVEL=atof(value);
	value[0]=0;
	GetPrivateProfileString("Effects", "FlashOffset", 0, value, MAXCHARS, INIFILE);
	if (value[0]!=0) FLASHOFFSET=atof(value);
	value[0]=0;
	GetPrivateProfileString("Settings", "ClassName", 0, value, MAXCHARS, INIFILE);
	if (value[0]!=0) strcpy(g_JumpDriveName, value);
	value[0]=0;
	GetPrivateProfileString("Settings", "Capacitors", 0, value, MAXCHARS, INIFILE);
	if (value[0]!=0) CELLS=atoi(value);
	value[0]=0;
	GetPrivateProfileString("Settings", "CellCapacity", 0, value, MAXCHARS, INIFILE);
	if (value[0]!=0) CAPACITY=atof(value);
	value[0]=0;
	GetPrivateProfileString("Settings", "CellReloadTime", 0, value, MAXCHARS, INIFILE);
	if (value[0]!=0) RELOADTIME=atof(value);
	value[0]=0;
	GetPrivateProfileString("Settings", "JumpRange", 0, value, MAXCHARS, INIFILE);
	if (value[0]!=0) JUMPPOS=atof(value);
	value[0]=0;
	GetPrivateProfileString("Settings", "JumpRangeScaling", 0, value, MAXCHARS, INIFILE);	
	if (value[0]!=0) JUMPSCALE=atof(value);
	value[0]=0;
	GetPrivateProfileString("Features", "FleetRange", 0, value, MAXCHARS, INIFILE);
	if (value[0]!=0) FLEETRANGE=atof(value);
	myBrush=oapiCreateBrush(0x00008000);
	// Register the new MFD mode with Orbiter
	static char *name = "JumpDrive";
	MFDMODESPEC spec;
	spec.name = name;
	spec.key = OAPI_KEY_J;
	spec.msgproc = JumpDriveMFD::MsgProc;
	g_MFDmode = oapiRegisterMFDMode (spec);
	g_Sound = -2; //-1 is "not connected", so -2 is "not initialized"
}

//This is the module stop callback. The mode will be unregistered and static instances deleted.
//If there is no INI-FILE yet, it will be created from scratch.
DLLCLBK void ExitModule (HINSTANCE hDLL)
{
	// Unregister the custom MFD mode when the module is unloaded
	oapiUnregisterMFDMode (g_MFDmode);
	oapiReleaseBrush(myBrush);

	if (fopen(INIFILE, "r")==0)
	{
		char value[MAXCHARS];
		sprintf(value, "%e", JUMPVEL);
		WritePrivateProfileString("Effects", "JumpVelocity", value, INIFILE);
		sprintf(value, "%e", FLASHOFFSET);
		WritePrivateProfileString("Effects", "FlashOffset", value, INIFILE);
		sprintf(value, g_JumpDriveName);
		WritePrivateProfileString("Settings", "ClassName", value, INIFILE);
		sprintf(value, "%d", CELLS);
		WritePrivateProfileString("Settings", "Capacitors", value, INIFILE);
		sprintf(value, "%e", CAPACITY);
		WritePrivateProfileString("Settings", "CellCapacity", value, INIFILE);
		sprintf(value, "%e", RELOADTIME);
		WritePrivateProfileString("Settings", "CellReloadTime", value, INIFILE);
		sprintf(value, "%e", JUMPPOS);
		WritePrivateProfileString("Settings", "JumpRange", value, INIFILE);
		sprintf(value, "%e", JUMPSCALE);
		WritePrivateProfileString("Settings", "JumpRangeScaling", value, INIFILE);
		sprintf(value, "%e", FLEETRANGE);
		WritePrivateProfileString("Features", "FleetRange", value, INIFILE);
	}
}

//This is the pre-step callback. In every step, all available drive instances will be
//proceeded further on. This way you can have more than one vessel jumping simultaneously.
DLLCLBK void opcPreStep (double SimT,double SimDT,double mjd)
{
	if (g_Sound<-1)
	{
		g_Sound=ConnectMFDToOrbiterSound("{912EE8D3-460B-4907-A3EF-CAD4D3A46E46}"); //GUID to identify JumpDrive mfd mode
		LoadMFDWave(g_Sound, ENGAGE, ENGAGEWAV);
		LoadMFDWave(g_Sound, CANCEL, CANCELWAV);
	}

	for(std::map< VESSEL*, JumpDrive* >::iterator i=g_JumpDrives.begin();i!=g_JumpDrives.end();i++)
		i->second->Process(SimT, SimDT);

	double aperture=0;
	OBJHANDLE cam=oapiCameraTarget();
	if (cam!=NULL && oapiIsVessel(cam))
	{
		VESSEL *vessel=oapiGetVesselInterface(cam);
		if (g_JumpDrives.find(vessel)!=g_JumpDrives.end())
			aperture=g_JumpDrives[vessel]->GetAperture();
	}
	if (aperture>0)
	{
		if (g_Environment.Reset)
		{
			g_Environment.FOV=oapiCameraAperture();
			g_Environment.Reset=false;
		}
		oapiCameraSetAperture(g_Environment.FOV+aperture*(MAXFOV-g_Environment.FOV));
	}
	else if (!g_Environment.Reset)
	{
		//Reseting FOV to environment setting if no jumping vessel has focus
		oapiCameraSetAperture(g_Environment.FOV);
		g_Environment.Reset=true;
	}
}

//This is the opening callback. It sets the environment variables.
DLLCLBK void opcOpenRenderViewport (HWND renderWnd, DWORD width, DWORD height, BOOL fullscreen)
{
	g_Environment.FOV=oapiCameraAperture();
	g_Environment.Reset=true;
}

//This is the closing callback. All drive and MFD instances will be deleted.
DLLCLBK void opcCloseRenderViewport ()
{
	for(std::map< VESSEL*, JumpDrive* >::iterator i=g_JumpDrives.begin();i!=g_JumpDrives.end();i++)
		delete i->second;
	for(std::map< VESSEL*, std::map< int, JumpDriveMFDData* > >::iterator i=g_JumpDriveMFDs.begin();i!=g_JumpDriveMFDs.end();i++)
		for(std::map< int, JumpDriveMFDData* >::iterator j=i->second.begin();j!=i->second.end();j++)
			delete j->second;
	g_JumpDrives.clear();
	g_JumpDriveMFDs.clear();
}

// MFD class implementation

// Constructor - Creates the MFD mode and links it with the appropriate drive and MFD
// instances if available. If the drive or MFD instance is not available, it will be
// created.
JumpDriveMFD::JumpDriveMFD (DWORD w, DWORD h, VESSEL *vessel, UINT mfd)
: MFD2 (w, h, vessel)
{
	char key[30];
	DWORD i;

	this->vessel=vessel;
	width=(int)w/35;
	heigth=(int)h/28;

	if (g_JumpDrives.find(vessel)==g_JumpDrives.end())
	{
		JumpDriveData data;
		data.vessel=vessel;
		data.jump=new double[CELLS];
		data.jump[0]=JUMPPOS;
		for(i=1;(int)i<CELLS;i++) data.jump[i]=data.jump[i-1]*JUMPSCALE;
		data.speed=JUMPVEL;
		data.offset=FLASHOFFSET;
		data.loadtime=RELOADTIME;
		data.capacity=CAPACITY;
		data.cells=CELLS;
		Drive=new JumpDrive(data);
		g_JumpDrives[vessel]=Drive;
	}
	else Drive=g_JumpDrives[vessel];

	Data=NULL;
	if (g_JumpDriveMFDs.find(vessel)!=g_JumpDriveMFDs.end())
		if (g_JumpDriveMFDs[vessel].find(mfd)!=g_JumpDriveMFDs[vessel].end())
			Data=g_JumpDriveMFDs[vessel][mfd];
	if (Data==NULL)
	{
		Data=new JumpDriveMFDData;
		g_JumpDriveMFDs[vessel][mfd]=Data;
		DWORD j=oapiGetObjectCount();
		OBJHANDLE obj=vessel->GetHandle();
		for(i=0; i<j; i++) if (oapiGetObjectByIndex(i)==obj) break;
		Data->target=i;
		Data->distance=0;
		Data->mode=0;
		Data->j2cells=1;
	}		
}

// Destructor - empty body.
JumpDriveMFD::~JumpDriveMFD ()
{
	// Add MFD cleanup code here
}

// Return button labels
char *JumpDriveMFD::ButtonLabel (int bt)
{
	// The labels for the two buttons used by our MFD mode
	static char *label[12] = {"JMP", "FUE", "CHG", "TGT", "MOD", "<>", "ALL", "INC", "DEC", "PRV", "NXT", "SET"};
	return (bt < 12 ? label[bt] : 0);
}

// Return button menus
int JumpDriveMFD::ButtonMenu (const MFDBUTTONMENU **menu) const
{
	// The menu descriptions for the two buttons
	static const MFDBUTTONMENU mnu[12] = {
		{"Engage jump", 0, 'J'},
		{"Select fuel index", 0, 'F'},
		{"Charge up cell", 0, 'C'},
		{"Select target", 0, 'T'},
		{"Change display mode", 0, 'M'},
		{"Set to current", 0, 'U'},
		{"Fleet jump command", 0, 'A'},
		{"Increase index", 0, 'I'},
		{"Decrease index", 0, 'D'},
		{"Previous target", 0, 'P'},
		{"Next target", 0, 'N'},
		{"Select distance", 0, 'S'}		
	};
	if (menu) *menu = mnu;	
	return 12; // return the number of buttons used
}


// Repaint the MFD - contains display and navcomp routines
bool JumpDriveMFD::Update (oapi::Sketchpad *skp)
{
	// Label strings
	static char *label[12] =	{"Drive class","Jump range","Capacity","Charge rate","Fuel source", "Fuel mass",
								 "Capacitors","Target name", "Distance set", "Display mode", "Mode value", "Miscellaneous"};
	// Label positions
	static unsigned char labelpos[12] = {2,6,8,10,15,17,24,34,50,42,46,52};
	// Mode labels
	static char *modeLabel[MODES] = {
		"Target size",
    	"Target mass",
    	"Target distance",
		"Course offset",
    	"Course deviation",		
    	"Deviation needed",
    	"Deviation offset",
    	"Relative velocity",
    	"Velocity elevation",
		"X-Y(pitch)alignment"
	};
	
	char buffer[80];
	int l=0;
	PROPELLANT_HANDLE propellant;

	this->skp=skp;
	
	//Get information from drive instance
	int cells=Drive->GetFilledCells();
	double jump=Drive->GetJumpRange(cells);

	//Descriptions (normal, light green)
	SetWriteStyle(0);
	//Jump drive class
	sprintf(buffer, g_JumpDriveName);
	l=strlen(buffer);
	skp->Text((1+(37-l)/2)*width, (labelpos[0]*heigth) >> 1, buffer, l);
	//Jump drive range
	sprintf(buffer, "%5.2em(%5.3fAU)", jump, jump/AU);
	l=strlen(buffer);
	skp->Text((1+(37-l)/2)*width, (labelpos[1]*heigth) >> 1, buffer, l);
	//Jump drive capacity
	sprintf(buffer, "%4.1fkg x %d", CAPACITY, CELLS);
	l=strlen(buffer);
	skp->Text((1+(37-l)/2)*width, (labelpos[2]*heigth) >> 1, buffer, l);
	//Jump drive charge rate
	sprintf(buffer, "%4.1fkg/s(%4.1fs)", CAPACITY/RELOADTIME, RELOADTIME);
	l=strlen(buffer);
	skp->Text((1+(37-l)/2)*width, (labelpos[3]*heigth) >> 1, buffer, l);
	//Navigation computer mode setting
	l=strlen(modeLabel[Data->mode]);
	skp->Text((1+(37-l)/2)*width, (labelpos[9]*heigth) >> 1, modeLabel[Data->mode], l);
	
	//Settings (normal, white)
	SetWriteStyle(0,2);
	//Fuel source info
	l=Drive->GetFuelSource(); 
	propellant=vessel->GetPropellantHandleByIndex(l);
	if (propellant!=NULL) sprintf(buffer, "%d of %d", l+1, vessel->GetPropellantCount());
	else sprintf(buffer, "OFF");
	l=strlen(buffer);
	skp->Text((1+(37-l)/2)*width, (labelpos[4]*heigth) >> 1, buffer, l);
	if (propellant!=NULL) sprintf(buffer, "%4.1fkg", vessel->GetPropellantMass(propellant));
	else sprintf(buffer, "n/a");
	l=strlen(buffer);
	skp->Text((1+(37-l)/2)*width, (labelpos[5]*heigth) >> 1, buffer, l);
	//Target information
	if (Data->target>=oapiGetObjectCount()) Data->target=oapiGetObjectCount()-1;
	OBJHANDLE obj=oapiGetObjectByIndex(Data->target);
	oapiGetObjectName(obj, buffer, 80);
	l=strlen(buffer);
	skp->Text((1+(37-l)/2)*width, (labelpos[7]*heigth) >> 1, buffer, l);
	//Navigation computer mode value
	VECTOR3 v1, v2, v3;
	double d1, d2;
	switch(Data->mode)
	{
	case 0: sprintf(buffer, "%7.6em", oapiGetSize(obj));
		break;
	case 1: sprintf(buffer, "%7.6ekg", oapiGetMass(obj));
		break;
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:// Trigonometrics:
		   //
		   //        +
		   //       / \
		   //     /gamma\                      j1^2+distance^2-j2^2
		   //    j1      j2         cos(beta)=(--------------------)
		   //  /beta   alpha\                      2*j1*distance
		   // +-- distance --+
		   //
		   // -1<=cos(beta)<=1      j1=j*s^x j2=j*s^y
		   //
		   //                             j1+distance
		   //                          ln(-----------)
		   //                                  j
		   //                       y<=---------------
		   //                               ln(s)
		   //
		   //
		   //                             |j1-distance|
		   //                          ln(-------------)
		   //                                  j
		   //                       y>=-----------------
		   //                                ln(s)
		   //

		oapiGetRelativePos(obj, vessel->GetHandle(), &v1);
		d1=length(v1);
		if (Data->setToCurrent)
		{
			Data->setToCurrent=false;
			Data->distance=d1;
		}
		if (Data->mode==2)
		{
			sprintf(buffer, "%7.6em", d1);
			break;
		}
		vessel->GlobalRot(_V(0,0,1), v2);
		if (d1!=0.0) v1/=d1;
		d1=dist(v1, v2);
		if (Data->mode==3)
		{
			sprintf(buffer, "%7.6em", d1*jump);
			break;
		}
		d1=360*asin(d1/2)/PI;
		if (Data->mode==4)
		{
			sprintf(buffer, "%9.6f°", d1);
			break;
		}		
		d2=Drive->GetJumpRange(Data->j2cells-1);
		if (jump!=0.0 && Data->distance!=0.0) d2=(jump*jump + Data->distance*Data->distance - d2*d2)/2/jump/Data->distance;
		else
		{
			sprintf(buffer, "n/a");
			break;
		}
		if (d2<-1 || d2>1)
		{
			d2=log(JUMPSCALE);
			d1=jump-Data->distance;
			if (d1<0) d1=-d1;
			d1=log(d1/JUMPPOS)/d2;
			unsigned long l=(unsigned long)d1;
			if (d1>l) l++;
			unsigned long k=(unsigned long)(log((jump+Data->distance)/JUMPPOS)/d2);
			if (k<l || l<0 || k>=(unsigned long)CELLS) sprintf(buffer, "n/a");
			else sprintf(buffer, "2nd jump:%d-%d cells", l+1, k+1);
			break;
		}
		d2=180*acos(d2)/PI;
		if (Data->mode==5)
		{
			sprintf(buffer, "%9.6f°", d2);
			break;
		}
		sprintf(buffer, "%9.6f°", d2-d1);
		break;		
	case 7:
	case 8:
		oapiGetRelativeVel(vessel->GetHandle(), obj, &v1);
		d1=length(v1);
		if (Data->mode==7)
		{
			sprintf(buffer, "%7.6em/s", d1);
			break;
		}
		oapiGetRelativePos(obj, vessel->GetHandle(), &v2);
		if ((d2=length(v2))!=0.0) v2/=d2;
		if (d1!=0.0) v1/=d1;
		d1=(360*asin(dist(v1,v2)/2)/PI)-90;
		sprintf(buffer, "%9.6f°", d1);
		break;
	case 9:
		oapiGetRelativeVel(vessel->GetHandle(), obj, &v1);
		oapiGetRelativePos(obj, vessel->GetHandle(), &v2);
		v1=crossp(v1, v2);
		if ((d1=length(v1))!=0.0) v1/=d1;
		vessel->GlobalRot(_V(0,0,1), v3);
		v3=crossp(v3, v2);
		if ((d1=length(v3))!=0.0) v3/=d1;		
		d1=(360*asin(dist(v1,v3)/2)/PI);
		vessel->GlobalRot(_V(0,1,0), v3);
		v3=crossp(v3, v2);
		if ((d2=length(v3))!=0.0) v3/=d2;		
		d2=(360*asin(dist(v1,v3)/2)/PI);		
		sprintf(buffer, "%4.1f° (%4.1f°)", d2, d1);
		break;
	default: sprintf(buffer, "n/a");
		break;
	}	
	l=strlen(buffer);
	skp->Text((1+(37-l)/2)*width, (labelpos[10]*heigth) >> 1, buffer, l);
	//Distance parameter
	sprintf(buffer, "%7.6em", Data->distance);
	l=strlen(buffer);
	skp->Text((1+(37-l)/2)*width, (labelpos[8]*heigth) >> 1, buffer, l);
	//Additional navigation parameter
	sprintf(buffer, "2nd jump with %d cells", Data->j2cells);
	l=strlen(buffer);
	skp->Text((1+(37-l)/2)*width, (labelpos[11]*heigth) >> 1, buffer, l);

	//Capacitor status (small font, light green)
	SetWriteStyle(1);
	skp->SetBrush(myBrush);
	int i,j,k,s,w;
	s=7*width;
	w=25*width;
	j=Drive->GetFuelCells();
	if (Drive->Jumping())
	{
		l=(int)(Drive->GetFuel()*100);	
		skp->Rectangle(			  s, (((labelpos[6]-2)*heigth) >> 1)+2,
						w*l/100+s+1, (((labelpos[6]+4)*heigth) >> 1)+2);
		if (l==0) sprintf(buffer, "00%%");
		else if (l==100) sprintf(buffer, "GO");
		else sprintf(buffer, "%d%%", l);
		l=strlen(buffer);
		SetWriteStyle(1,1);
		skp->Text((1+(37-l)/2)*width, (labelpos[6]*heigth) >> 1, buffer, l);
		skp->SetBrush(NULL);
		SetWriteStyle(1);
		skp->Rectangle(		s, (((labelpos[6]-2)*heigth) >> 1)+2,
						w+s+1, (((labelpos[6]+4)*heigth) >> 1)+2);
	}
	else
	{
		for(i=0;i<cells;i++)
			skp->Rectangle(		  w*i/j+s, (((labelpos[6]-2)*heigth) >> 1)+2,
							w*(i+1)/j+s+1, (((labelpos[6]+4)*heigth) >> 1)+2);	
		l=(int)(Drive->GetFuel()*100);	
		skp->Rectangle(				w*i/j+s, (((labelpos[6]-2)*heigth) >> 1)+2,
						w*i/j+w*l/100/j+s+1, (((labelpos[6]+4)*heigth) >> 1)+2);
		if (l==0) sprintf(buffer, "00%%");
		else if (l==100) sprintf(buffer, "GO");
		else sprintf(buffer, "%d%%", l);
		l=strlen(buffer);
		k=w*i/j+w/j/2+s-(l/2)*(width-2);
		if (k<s+2) k=s+2;
		SetWriteStyle(1,1);
		skp->Text(k, (labelpos[6]*heigth) >> 1, buffer, l);
		skp->SetBrush(NULL);
		SetWriteStyle(1);
		for(;i<j;i++)
			skp->Rectangle(		  w*i/j+s, (((labelpos[6]-2)*heigth) >> 1)+2,
							w*(i+1)/j+s+1, (((labelpos[6]+4)*heigth) >> 1)+2);
	}
	//MFD mode labels (small font, dark green)
	skp->SetTextColor(0x00008000);
	for (l=0; l<12; l++) skp->Text(width, (labelpos[l]*heigth) >> 1, label[l], strlen(label[l]));

	return true;
}

// MFD message parser - returns a new MFD mode instance
int JumpDriveMFD::MsgProc (UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam)
{	
	switch (msg) {
	case OAPI_MSG_MFD_OPENED:		
		return (int)(new JumpDriveMFD (LOWORD(wparam), HIWORD(wparam), (VESSEL*)lparam, mfd));
	}
	return 0;
}

// Callback for target selection dialog
bool TargetSelected (void *id, char *str, void *usrdata)
{
	JumpDriveMFD *mfd=(JumpDriveMFD *)usrdata;
	if (strlen(str)==0) return false;
	OBJHANDLE obj=oapiGetObjectByName(str);
	if (obj==NULL) return false;
	DWORD i, j=oapiGetObjectCount();
	for(i=0; i<j; i++) if (oapiGetObjectByIndex(i)==obj) break;
	mfd->Data->target=i;
	mfd->InvalidateDisplay();
	return true;
}

// Callback for fuel source selection dialog
bool FuelSelected (void *id, char *str, void *usrdata)
{
	JumpDriveMFD *mfd=(JumpDriveMFD *)usrdata;
	if (strlen(str)==0) return false;
	int index=atoi(str);
	if (index<0) return false;
	mfd->Drive->SetFuelSource(index-1);
	return true;	
}

// Callback for parameter dialog
bool ParameterSet (void *id, char *str, void *usrdata)
{
	int i, j;
	JumpDriveMFD *mfd=(JumpDriveMFD *)usrdata;
	if ((j=strlen(str))==0) return false;	
	for (i=0;i<j;i++)
		if (str[i]==',')
		{
			j=atoi(str+i+1);
			if (j>0 && j<=CELLS) mfd->Data->j2cells=j;
			else return false;
			str[i]=0;
			break;
		}
	if (strlen(str)>0)
	{
		double distance=atof(str);
		if (distance<0) return false;
		mfd->Data->distance=distance;
	}
	mfd->InvalidateDisplay();
	return true;
}

// Handling shortcut keys
bool JumpDriveMFD::ConsumeKeyBuffered(DWORD key)
{	
	static char *fuelSelect = "Enter fuel source index (0=off):";
	static char *targetSelect = "Enter target name:";
	static char *parameterSet = "Enter Distance[m](,Cell count):";
	int fuelSource;
	bool result=true;
	switch(key)
	{
	case OAPI_KEY_J://Jump
		if (Drive->Start()) PlayMFDWave(g_Sound,ENGAGE,NOLOOP,255);
		else PlayMFDWave(g_Sound,CANCEL,NOLOOP,255);
		break;
	case OAPI_KEY_C://Charge
		if (Drive->Load()) PlayMFDWave(g_Sound,ENGAGE,NOLOOP,255);
		else PlayMFDWave(g_Sound,CANCEL,NOLOOP,255);
		break;
	case OAPI_KEY_I://Increase fuel source index
		PlayMFDWave(g_Sound,ENGAGE,NOLOOP,255);
		fuelSource=Drive->GetFuelSource();
		if ((DWORD)++fuelSource>vessel->GetPropellantCount()) fuelSource=0;
		Drive->SetFuelSource(fuelSource);
		break;
	case OAPI_KEY_D://Decrease fuel source index
		PlayMFDWave(g_Sound,ENGAGE,NOLOOP,255);
		fuelSource=Drive->GetFuelSource();
		if (--fuelSource<0) fuelSource=vessel->GetPropellantCount();
		Drive->SetFuelSource(fuelSource);
		break;
	case OAPI_KEY_T://Open target selection dialog
		PlayMFDWave(g_Sound,ENGAGE,NOLOOP,255);
		oapiOpenInputBox (targetSelect, TargetSelected, 0, 20, this);
		break;
	case OAPI_KEY_P://Select previous target
		PlayMFDWave(g_Sound,ENGAGE,NOLOOP,255);
		if (Data->target==0) Data->target=oapiGetObjectCount()-1;
		else Data->target--;
		break;
	case OAPI_KEY_N://Select next target
		PlayMFDWave(g_Sound,ENGAGE,NOLOOP,255);
		if (++(Data->target)>=oapiGetObjectCount()) Data->target=0;		
		break;
	case OAPI_KEY_M://Switch navcomp mode
		PlayMFDWave(g_Sound,ENGAGE,NOLOOP,255);		
		if (++(Data->mode)>=MODES) Data->mode=0;
		break;
	case OAPI_KEY_S://Open parameter dialog
		PlayMFDWave(g_Sound,ENGAGE,NOLOOP,255);
		oapiOpenInputBox (parameterSet, ParameterSet, 0, 20, this);
		break;
	case OAPI_KEY_U://Set distance parameter to current target's distance
		PlayMFDWave(g_Sound,ENGAGE,NOLOOP,255);
		Data->setToCurrent=true;
		Data->mode=2;
		break;
	case OAPI_KEY_F://Open fuel source selection dialog
		PlayMFDWave(g_Sound,ENGAGE,NOLOOP,255);
		oapiOpenInputBox (fuelSelect, FuelSelected, 0, 20, this);
		break;
	case OAPI_KEY_A://Fleet command - jump all vessels into same direction
		if (Drive->Start())
		{
			PlayMFDWave(g_Sound,ENGAGE,NOLOOP,255);
			for(std::map< VESSEL*, JumpDrive* >::iterator i=g_JumpDrives.begin();i!=g_JumpDrives.end();i++)
				i->second->FleetCommand(vessel, FLEETRANGE);
		}
		else PlayMFDWave(g_Sound,CANCEL,NOLOOP,255);
		break;
	default:
		result=false;
	}
	if (!result) return false;
	InvalidateDisplay();
	return true;
}

// Handling button presses by forwarding shortcuts
bool JumpDriveMFD::ConsumeButton(int bt, int event)
{	
	if (event & PANEL_MOUSE_LBDOWN)
	{
		switch(bt)
		{
		case 0:
			return ConsumeKeyBuffered(OAPI_KEY_J);
		case 1:
			return ConsumeKeyBuffered(OAPI_KEY_F);
		case 2:
			return ConsumeKeyBuffered(OAPI_KEY_C);
		case 3:
			return ConsumeKeyBuffered(OAPI_KEY_T);
		case 4:
			return ConsumeKeyBuffered(OAPI_KEY_M);
		case 5:
			return ConsumeKeyBuffered(OAPI_KEY_U);
		case 6:
			return ConsumeKeyBuffered(OAPI_KEY_A);
		case 7:
			return ConsumeKeyBuffered(OAPI_KEY_I);
		case 8:
			return ConsumeKeyBuffered(OAPI_KEY_D);
		case 9:
			return ConsumeKeyBuffered(OAPI_KEY_P);
		case 10:
			return ConsumeKeyBuffered(OAPI_KEY_N);
		case 11:
			return ConsumeKeyBuffered(OAPI_KEY_S);
		}
	}	
	return false;
}

void JumpDriveMFD::SetWriteStyle(int font, int color, int intensity, int style)
{
	static DWORD colors[4] = {0x0000FF00, 0x0000FFFF, 0x00FFFFFF, 0x000000FF};
	skp->SetFont(GetDefaultFont(font));
	skp->SetTextColor(colors[color]);
	skp->SetPen(GetDefaultPen(color, intensity, style));
}