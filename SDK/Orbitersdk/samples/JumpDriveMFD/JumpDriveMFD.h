/**************************************************************************
 * JumpDriveMFD V1.0 - FTL travel in a difficult fashion for Orbiter 2010P1
 *     created: July, 6th 2006
 * last update: May, 20th 2014
 * Copyright (c) 2006-2014 Friedrich Kastner-Masilko, M.Sc.
 *
 * JumpDriveMFD class header
 **************************************************************************/
#ifndef __JUMPDRIVEMFD_H
#define __JUMPDRIVEMFD_H

//The variables for the FOV effect are stored here
struct Environment
{
	//The currently used (unaffected) FOV
	double FOV;
	//Flag for reseting the FOV
	bool Reset;
};

//Each MFD instance has it's own data memory for the JumpDrive mode.
//Even if you switch to another mode, the settings will be saved.
struct JumpDriveMFDData
{
	//target index
	DWORD target;
	//set distance for Z2 calculation
	double distance;
	//set navcomp mode
	int mode;
	//set information about second FLJ's filled cells
	int j2cells;
	//internal flag
	bool setToCurrent;
};

//The JumpDrive MFD mode class.
class JumpDriveMFD: public MFD2 {
public:
	//Default implementations
	JumpDriveMFD (DWORD w, DWORD h, VESSEL *vessel, UINT mfd);
	~JumpDriveMFD ();
	char *ButtonLabel (int bt);
	int ButtonMenu (const MFDBUTTONMENU **menu) const;
	bool ConsumeKeyBuffered (DWORD key);
	bool ConsumeButton (int bt, int event);
	bool Update (oapi::Sketchpad *skp);
	static int MsgProc (UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam);
	//The drive instance the mode is linked to
	JumpDrive *Drive;
	//The data instance the mode is linked to
	JumpDriveMFDData *Data;
private:
	//The vessel the mode is linked to
	VESSEL *vessel;
	//Width and height of the MFD for text positions
	int width;
	int heigth;
	oapi::Sketchpad *skp;
	void SetWriteStyle(int font=0, int color=0, int intensity=0, int style=1);
};

#endif // !__JUMPDRIVEMFD_H