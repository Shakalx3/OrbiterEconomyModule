/**************************************************************************
 * JumpDriveMFD V1.0 - FTL travel in a difficult fashion for Orbiter 2010P1
 *     created: July, 6th 2006
 * last update: May, 20th 2014
 * Copyright (c) 2006-2014 Friedrich Kastner-Masilko, M.Sc.
 *
 * JumpDrive class header
 **************************************************************************/

// Trigonimetric definitions
#define TAN70			2.747477419
#define RAD90			1.570796327
#define RAD70			1.221730476

// Time definitions
#define ACCTIME			3.6
#define FLYTIME			2.1
#define JMPTIME			1.6
#define DECTIME			7.3

// Drive states
#define S_IDLE			0
#define S_LOAD			1
#define S_ACCELERATE	2
#define S_CRUISE		3
#define S_JUMP			4
#define S_DECELERATE	5

// Drive sounds
#define LOADWAV			"Sound\\JumpDriveMFD\\load.wav"
#define WOOSHOUTWAV		"Sound\\JumpDriveMFD\\woosh1.wav"
#define BANGWAV			"Sound\\JumpDriveMFD\\bang.wav"
#define WOOSHINWAV		"Sound\\JumpDriveMFD\\woosh2.wav"
#define INIFILE			"Modules\\Plugin\\JumpDriveMFD.ini"

// Plugin defines
#define LOAD			50
#define WOOSHOUT		51
#define BANG			52
#define WOOSHIN			53

// Includes
#include "windows.h"
#include "orbitersdk.h"
#include "orbitersoundsdk40.h"
#include "JumpDrive.h"
#include <stdio.h>

// Constructor - Creates a drive with the given data
JumpDrive::JumpDrive(JumpDriveData data)
{
	vessel=data.vessel;
	sound=ConnectToOrbiterSoundDLL(vessel->GetHandle());
	RequestLoadVesselWave(sound,	LOAD,		LOADWAV,		DEFAULT);
	RequestLoadVesselWave(sound,	WOOSHOUT,	WOOSHOUTWAV,	DEFAULT);
	RequestLoadVesselWave(sound,	BANG,		BANGWAV,		DEFAULT);
	RequestLoadVesselWave(sound,	WOOSHIN,	WOOSHINWAV,		DEFAULT);	
	loadtime=data.loadtime;
	capacity=data.capacity;
	offset=data.offset;
	jump=data.jump;
	speed=data.speed;
	cells=data.cells;
	filledCells=0;
	object=vessel->GetHandle();
	addvec=_V(0,1,0);	
	engaged=0;
	oldRVEL=_V(0,0,0);	
	target=_V(0,0,0);	
	state=S_IDLE;
	beacon.shape=BEACONSHAPE_STAR;
	beacon.pos=&beaconpos;
	beacon.col=&beaconcol;
	beacon.size=1.0;
	beacon.falloff=1;
	beacon.period=0;
	beacon.duration=0;
	beacon.tofs=0;
	beacon.active=true;
	beaconcol=_V(1,1,1);		
	reference=oapiGetGbodyByIndex(0);
	fuel=0;
	fuelSource=0;
	aperture=0;
}

// Destructor - removes the internal jump range table
JumpDrive::~JumpDrive()
{
	delete [] jump;
}

//Sets the current fuel source index
void JumpDrive::SetFuelSource(int index)
{	
	fuelSource=index;
}

//Returns the current fuel source index	
int JumpDrive::GetFuelSource()
{
	return fuelSource;
}

//Returns amount of available fuel cells
int JumpDrive::GetFuelCells()
{
	return cells;
}

//Returns amount of filled fuel cells
int JumpDrive::GetFilledCells()
{
	return filledCells;
}

//Returns internal fuel level (0-1)
double JumpDrive::GetFuel()
{
	return fuel;
}

//Returns current jump range in meters
double JumpDrive::GetJumpRange(int cells)
{
	return jump[cells];
}

//Starts a jump procedure if jump is possible.
bool JumpDrive::Start()
{
	if (isJumpPossible())
	{
		VECTOR3 vec;		
		vessel->GlobalRot(_V(0,0,speed), addvec);
		vessel->GlobalRot(_V(0,0,jump[filledCells]), target);
		engaged=oapiGetSimTime();
		vessel->GetRelativeVel(reference, oldRVEL);
		vessel->GetRelativePos(reference, vec);
		target+=vec;		
		state=S_ACCELERATE;		
		PlayVesselWave(sound,WOOSHOUT,NOLOOP,255);
		fuel=0.99;
		filledCells=0;
		return true;
	}
	return false;
}

//Starts a loading procedure
bool JumpDrive::Load()
{
	PROPELLANT_HANDLE propellant=vessel->GetPropellantHandleByIndex(fuelSource);
	if (propellant!=NULL)		
	{
		double mass=vessel->GetPropellantMass(propellant);
		if (mass>0.0 && (fuel<=0.0 || (fuel>=1.0 && filledCells+1<cells)))
		{
			if (fuel>=1.0) filledCells++;
			fuel=0.0;
			state=S_LOAD;			
			return true;
		}
	}
	return false;
}

//Engages a fleet command to the current drive's owner with the commanding
//vessel in parameter 1 and the command range in meters in parameter 2
void JumpDrive::FleetCommand(VESSEL *vessel, double treshold)
{
	if (isJumpPossible())
	{
		VECTOR3 d;
		oapiGetRelativePos (this->object, vessel->GetHandle(), &d);
		if (length(d)<treshold)
		{
			vessel->GetGlobalOrientation(d);
			this->vessel->SetGlobalOrientation(d);
			Start();
		}
	}
}

//Returns wether or not the vessel is currently jumping
bool JumpDrive::Jumping()
{
	return state>S_LOAD;
}

//Returns wether or not a jump is possible
bool JumpDrive::isJumpPossible()
{
	if (fuel<1.0) return false;
	return (vessel->GetFlightStatus() & 0x02)==0;/*Alternative implementation:
	int i=0;
	DOCKHANDLE dock;
	while(true)
	{
		dock=vessel->GetDockHandle(i++);
		if (dock==NULL) return true;
		if (vessel->GetDockStatus(dock)!=NULL) return false;
	}*/
}

//Delete all docked vessels from the parent recursively
void JumpDrive::deleteVessels(VESSEL *vessel, OBJHANDLE parent)
{
	int i=0;
	DOCKHANDLE dock;
	OBJHANDLE dockHandle;
	VESSEL *child;	
	while(true)
	{
		dock=vessel->GetDockHandle(i++);
		if (dock==NULL) break;
		if ((dockHandle=vessel->GetDockStatus(dock))!=NULL)
		{
			if (dockHandle!=parent)
			{
				deleteVessels((child=new VESSEL(dockHandle, 1)), vessel->GetHandle());
				delete child;
				oapiDeleteVessel(dockHandle);
			}
		}
	}	
}

//Processing jump drive progress - should be called periodically
void JumpDrive::Process(double SimT,double SimDT)
{	
	double diff;
	aperture=0.0;
	//Loading a cell
	if (state==S_LOAD)
	{
		PROPELLANT_HANDLE propellant=vessel->GetPropellantHandleByIndex(fuelSource);
		if (propellant!=NULL)		
		{
			double mass=vessel->GetPropellantMass(propellant);
			if (mass>0)
			{
				diff=SimDT/loadtime;
				if (diff>(1.0-fuel))
				{
					diff=1.0-fuel;					
					state=S_IDLE;					
				}						
				if (diff>mass/capacity) diff=mass/capacity;
				fuel+=diff;
				mass-=diff*capacity;
				vessel->SetPropellantMass(propellant, mass);
				if (state!=S_LOAD) StopVesselWave(sound, LOAD);
				else PlayVesselWave(sound,LOAD,LOOP,255);
			}
			else StopVesselWave(sound, LOAD);
		}
		else StopVesselWave(sound, LOAD);
	}
	if (state>S_LOAD)
	{
		diff=SimT-engaged;
		VESSELSTATUS2 status;
		status.version=2;
		status.flag=0;		
		aperture=1.0;
		switch(state)
		{
		//Accelerating to jump speed
		case S_ACCELERATE:
			if (diff>ACCTIME)
			{				
				engaged=SimT;				
				state=S_CRUISE;				
			}
			else
			{	
				vessel->GetStatusEx(&status);
				aperture=diff/ACCTIME;
				status.rvel+=addvec*SimDT*aperture;				
				vessel->DefSetStateEx(&status);
				fuel=0.99-aperture*0.2;
			}
			break;
		//Cruise at jump speed
		case S_CRUISE:
			if (diff>FLYTIME)
			{
				vessel->GetStatusEx(&status);
				hyperobjectIn=oapiCreateVesselEx("Hyperspace","Hyperspace",&status);
				vessel->GetRelativeVel(reference, status.rvel);
				status.rpos=target;
				status.rbody=reference;				
				vessel->DefSetStateEx(&status);
				PlayVesselWave(sound,BANG,NOLOOP,255);
				engaged=SimT;				
				state=S_JUMP;				
				hyperobjectOut=oapiCreateVesselEx("Hyperspace","Hyperspace",&status);
				hyperspaceIn=oapiGetVesselInterface(hyperobjectIn);
				hyperspaceOut=oapiGetVesselInterface(hyperobjectOut);
				hyperspaceIn->SetVisibilityLimit(1e-12);
				hyperspaceOut->SetVisibilityLimit(1e-12);
				vessel->GetCameraOffset(beaconpos);	
				beaconpos.z+=offset;
				hyperspaceIn->AddBeacon(&beacon);
				hyperspaceOut->AddBeacon(&beacon);
				//Delete all docked vessels (hyperspace physics are dangerous, you know...)
				deleteVessels(vessel, NULL);
			}
			else
			{
				
			}
			break;
		//Explosion effect
		case S_JUMP:
			if (diff>JMPTIME)
			{
				PlayVesselWave(sound,WOOSHIN,NOLOOP,255);
				engaged=SimT;				
				state=S_DECELERATE;
				oapiDeleteVessel(hyperobjectIn);
				oapiDeleteVessel(hyperobjectOut);
				fuel=0.2;
			}
			else
			{
				if (diff>0.3) diff=0;
				else diff/=0.3;
				if (diff!=0) diff=1/diff/diff;				
				beacon.size=diff*10;				
			}
			break;
		//Decelerating from jump speed
		case S_DECELERATE:
			if (diff>DECTIME)
			{
				vessel->GetStatusEx(&status);
				vessel->GetRelativeVel(reference, status.rvel);
				vessel->GetRelativePos(reference, status.rpos);
				status.rbody=reference;
				status.rvel=oldRVEL;
				vessel->DefSetStateEx(&status);
				aperture=0.0;
				state=S_IDLE;				
				fuel=0;
			}
			else
			{
				vessel->GetStatusEx(&status);
				aperture=1.0-(diff/DECTIME);
				status.rvel-=addvec*SimDT*aperture*0.5;
				vessel->DefSetStateEx(&status);
				fuel=aperture*0.2;
			}
			break;
		}
	}
}

double JumpDrive::GetAperture() {return aperture;}