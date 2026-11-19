/**************************************************************************
 * JumpDriveMFD V1.0 - FTL travel in a difficult fashion for Orbiter 2010P1
 *     created: July, 6th 2006
 * last update: May, 20th 2014
 * Copyright (c) 2006-2014 Friedrich Kastner-Masilko, M.Sc.
 *
 * JumpDrive class header
 **************************************************************************/

#if !defined(AFX_JUMPDRIVE_H__ABA580C1_0C57_11DB_BD29_0002442A51E7__INCLUDED_)
#define AFX_JUMPDRIVE_H__ABA580C1_0C57_11DB_BD29_0002442A51E7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//Data for drive construction
struct JumpDriveData
{
	//Link to vessel instance
	VESSEL *vessel;
	//Available cells
	int cells;
	//Jump speed used
	double speed;
	//Jump length
	double *jump;
	//Explosion effect view point offset
	double offset;
	//Time to load one cell
	double loadtime;
	//Capacity of one cell
	double capacity;	
};

//The JumpDrive class
class JumpDrive  
{
public:
	//Constructor
	JumpDrive(JumpDriveData data);
	//Destructor
	~JumpDrive();
	//Processing jump drive progress - should be called periodically
	void Process (double SimT,double SimDT);
	//Starts a jump procedure
	bool Start ();	
	//Starts a loading procedure
	bool Load ();	
	//Sets the current fuel source index
	void SetFuelSource(int index);	
	//Returns the current fuel source index
	int GetFuelSource();
	//Returns amount of available fuel cells
	int GetFuelCells();
	//Returns amount of filled fuel cells
	int GetFilledCells();
	//Returns internal fuel level (0-1)
	double GetFuel();
	//Returns current jump range in meters
	double GetJumpRange(int cells);
	//Engages a fleet command to the current drive's owner with the commanding
	//vessel in parameter 1 and the command range in meters in parameter 2
	void FleetCommand(VESSEL *vessel, double treshold);
	//Returns wether or not the vessel is currently jumping
	bool Jumping();
	//Returns the current aperture setting according to jump state
	double GetAperture();
private:
	//Returns wether or not a jump is possible
	bool isJumpPossible();	
	//Delete all docked vessels from the parent recursively
	void deleteVessels(VESSEL *vessel, OBJHANDLE parent);
	//The linked vessel
	VESSEL *vessel;
	//The fake vessel for hyperspace entry
	VESSEL *hyperspaceIn;
	//The fake vessel for hyperspace leaving
	VESSEL *hyperspaceOut;
	//The linked vessel object
	OBJHANDLE object;
	//The object handle of the fake entry vessel
	OBJHANDLE hyperobjectIn;
	//The object handle of the fake leaving vessel
	OBJHANDLE hyperobjectOut;
	//The reference Gbody of the vessel
	OBJHANDLE reference;
	//Jump acceleration vector
	VECTOR3 addvec;
	//Memory for conserving relative velocity vector
	VECTOR3 oldRVEL;
	//Jump target position
	VECTOR3	target;
	//Position of beacon for explosion effect
	VECTOR3 beaconpos;
	//Color of beacon for explosion effect
	VECTOR3 beaconcol;
	//Current selected fuel source index
	int fuelSource;
	//Sound handle
	int sound;
	//Amount of available cells
	int cells;
	//Amount of filled cells
	int filledCells;
	//Drive state
	unsigned char state;
	//Time stamp of jump start
	double engaged;
	//Acceleration speed
	double speed;
	//Jump length
	double *jump;
	//Explosion effect view point offset
	double offset;
	//Time to load one cell
	double loadtime;
	//Capacity of one cell
	double capacity;	
	//Internal fuel level
	double fuel;
	//Current aperture
	double aperture;
	//Beacon for explosion effect
	BEACONLIGHTSPEC beacon;
};

#endif // !defined(AFX_JUMPDRIVE_H__ABA580C1_0C57_11DB_BD29_0002442A51E7__INCLUDED_)
