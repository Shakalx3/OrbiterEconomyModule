/***************************************************************************
 *   Copyright (C) 2008 by C J Plooy                                       *
 *   cornwarecjp@lavabit.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/***************************************************************************
 *	
 * R. 06/2011
 *	--change addresses for InputBox to be used in 2010 D3D7 & D3D9
 *
 ***************************************************************************/
#include <time.h>
#include <windows.h>

#include "orbitersdk.h"

#include "orbitersdk_ext_cjp.h"


//Global variables to store things for the oapiOpenInputBox fallback
void *InputBox_userPtr = NULL;
menuClbk InputBox_clbk = NULL;

bool InputBoxCallback(void *id,char *str,void *usrdata)
{
	if(InputBox_clbk == NULL) return false; //Should never occur anyway

	return InputBox_clbk(NULL, 0, str, InputBox_userPtr);
}


bool isOrbiter2006p1()
{
	//Check for exact orbiter version

	//First check: what does Orbiter say about itself?
	if(oapiGetOrbiterVersion() != 60929) return false;

	//Second check: test the exact memory contents at some location
	//  (this location is selected carefully: it's not just a random choice) --> CelBodyPopup testProcAddr + 0x23
	unsigned char *testMem = (unsigned char *)0x00431073;

	return
		testMem[0] == 0xe8 &&
		testMem[1] == 0x58 &&
		testMem[2] == 0x84 &&
		testMem[3] == 0x06 &&
		testMem[4] == 0x00;
}

bool isOrbiter2010()
{
	//Check for exact orbiter version

	//First check: what does Orbiter say about itself?
	if(oapiGetOrbiterVersion() != 100830) return false;

	//Second check: test the exact memory contents at some location
	//  (this location is selected carefully: it's not just a random choice) --> CelBodyPopup testProcAddr + 0x23
	unsigned char *testMem = (unsigned char *)0x0042be53;

//sprintf(oapiDebugString(),"testMem: %02x %02x %02x %02x %02X %02x %02x %02x",
//	testMem[0],testMem[1],testMem[2],testMem[3],testMem[4],testMem[5],testMem[6],testMem[7]
//);

	return
		testMem[0] == 0xe8 &&
		testMem[1] == 0xf8 &&
		testMem[2] == 0x6d &&
		testMem[3] == 0x07 &&
		testMem[4] == 0x00;
}

bool isOrbiter2010_server()
{
	//Check for exact orbiter version

	//First check: what does Orbiter say about itself?
	if(oapiGetOrbiterVersion() != 100830) return false;

	//Second check: test the exact memory contents at some location
	//  (this location is selected carefully: it's not just a random choice) --> CelBodyPopup testProcAddr + 0x23
	unsigned char *testMem = (unsigned char *)0x0042a1d3;
/*
sprintf(oapiDebugString(),"testMem: %02x %02x %02x %02x %02X",
	testMem[0],testMem[1],testMem[2],testMem[3],testMem[4]
);
*/
	return
		testMem[0] == 0xe8 &&
		testMem[1] == 0x78 &&
		testMem[2] == 0x62 &&
		testMem[3] == 0x07 &&
		testMem[4] == 0x00;
}

bool isOrbiter2016()
{
	//Check for exact orbiter version

	//First check: what does Orbiter say about itself?
	if(oapiGetOrbiterVersion() != 160828) return false;

	//Second check: test the exact memory contents at some location
	//  (this location is selected carefully: it's not just a random choice) --> CelBodyPopup testProcAddr + 0x23
	unsigned char *testMem = (unsigned char *)0x0043d903;
/*
sprintf(oapiDebugString(),"testMem: %02x %02x %02x %02x %02X",
	testMem[0],testMem[1],testMem[2],testMem[3],testMem[4]
);
*/
	return
		testMem[0] == 0xe8 &&
		testMem[1] == 0xd8 &&
		testMem[2] == 0x89 &&
		testMem[3] == 0x09 &&
		testMem[4] == 0x00;
}

bool isOrbiter2016_server()
{
	//Check for exact orbiter version

	//First check: what does Orbiter say about itself?
	if(oapiGetOrbiterVersion() != 160828) return false;

	//Second check: test the exact memory contents at some location
	//  (this location is selected carefully: it's not just a random choice) --> CelBodyPopup testProcAddr + 0x23
	unsigned char *testMem = (unsigned char *)0x004356d3;
/*
sprintf(oapiDebugString(),"testMem: %02x %02x %02x %02x %02X",
	testMem[0],testMem[1],testMem[2],testMem[3],testMem[4]
);
*/
	return
		testMem[0] == 0xe8 &&
		testMem[1] == 0x88 &&
		testMem[2] == 0x69 &&
		testMem[3] == 0x08 &&
		testMem[4] == 0x00;
}

void searchcode()
{
	unsigned char *testMem = (unsigned char *)0x0042a690;
	int hex=0x0; int n=0;

/*
	for(hex=0x00400000; hex<0x00600000; hex+=0x00000001){
		testMem = (unsigned char *)hex;
		n++;
		if(
//////////////////////////////////////////////////// reference
/*	//	--> Orbiter_NG_D3D9 : ref 00 4b 61 10
			testMem[0] == 0x4f &&
			testMem[1] == 0x72 &&
			testMem[2] == 0x62 &&
			testMem[3] == 0x69 &&
			testMem[4] == 0x74 &&
			testMem[5] == 0x20 &&
			testMem[6] == 0x4d &&
			testMem[7] == 0x46 &&
			testMem[8] == 0x44 &&
			testMem[9] == 0x3a &&
			testMem[10] == 0x20 &&
			testMem[11] == 0x52 &&
			testMem[12] == 0x65 &&
			testMem[13] == 0x66
*/
/*	//	--> Orbiter_NG_D3D9 : ref 00 43 88 5b + 5 => E8 4b 19 ff ff
			testMem[0] == 0x68 &&
			testMem[1] == 0x10 &&
			testMem[2] == 0x61 &&
			testMem[3] == 0x4b &&
			testMem[4] == 0x00
*/

//////////////////////////////////////////////////// target
/*	//	--> Orbiter_NG_D3D9 : tgt 00 4b 60 fc
			testMem[0] == 0x4f &&
			testMem[1] == 0x72 &&
			testMem[2] == 0x62 &&
			testMem[3] == 0x69 &&
			testMem[4] == 0x74 &&
			testMem[5] == 0x20 &&
			testMem[6] == 0x4d &&
			testMem[7] == 0x46 &&
			testMem[8] == 0x44 &&
			testMem[9] == 0x3a &&
			testMem[10] == 0x20 &&
			testMem[11] == 0x54 &&
			testMem[12] == 0x61 &&
			testMem[13] == 0x72
*/

/*	//	--> Orbiter_NG_D3D9 : tgt 00 43 88 79 + 5 => E8 0b 1e ff ff
			testMem[0] == 0x68 &&
			testMem[1] == 0xfc &&
			testMem[2] == 0x60 &&
			testMem[3] == 0x4b &&
			testMem[4] == 0x00
*/
/*
		)	break;
	}
*/
	sprintf(oapiDebugString(),"testMem:%08x (hex:%08x) --> %02x %02x %02x %02x %02X  (n:%i)",
		testMem, hex, testMem[0],testMem[1],testMem[2],testMem[3],testMem[4],n
	);
}


void oapiCelBodyPopup(void *userPtr, char *title, menuClbk clbk, DWORD flags)
{
// search string "Orbit MFD: Reference" then goto push & get the call following

/*
sprintf(oapiDebugString(),"version: %i   2006P1:%i   2010:%i   2010_server:%i   2016:%i   2016_server:%i",
	oapiGetOrbiterVersion(),isOrbiter2006p1(),isOrbiter2010(),isOrbiter2010_server(),isOrbiter2016(),isOrbiter2016_server()
);
*/
	DWORD testProcAddr = 0x00431050;//Orbiter2006P1

	if(isOrbiter2010())
		testProcAddr = 0x0042be30;//Orbiter2010

	if(isOrbiter2010_server())
		testProcAddr = 0x0042a1b0;//Orbiter2010_server

	if(isOrbiter2016())
		testProcAddr = 0x0043d8e0;//Orbiter2016

	if(isOrbiter2016_server())
		testProcAddr = 0x004356b0;//Orbiter2016_server


	//Check for exact orbiter version,
	//and provide alternative if not
	if(!isOrbiter2006p1() && !isOrbiter2010() && !isOrbiter2010_server() && !isOrbiter2016() && !isOrbiter2016_server())
	{
		InputBox_userPtr = userPtr;
		InputBox_clbk    = clbk;
		oapiOpenInputBox(title, InputBoxCallback, "", 20, NULL);
		return;
	}

	/*
	In the actual function 0x00431050, ecx (the this pointer)
	points to an InstrUser object. However, it is not
	dereferenced, so we can put any user-defined pointer
	in it.
	*/
	__asm {
		mov ecx, [userPtr];
		push [flags];
		push [clbk];
		push [title];
		call testProcAddr;
	};
}

void oapiTargetPopup(void *userPtr, char *title, menuClbk clbk, OBJHANDLE refObj, DWORD flags)
{
// search string "Orbit MFD: Target" then goto push & get the call following

///////////////////////////
//	searchcode();
///////////////////////////

	DWORD testProcAddr = 0x00431190;//Orbiter2006P1

	if(isOrbiter2010())
		testProcAddr = 0x0042c310;//Orbiter2010

	if(isOrbiter2010_server())
		testProcAddr = 0x0042a690;//Orbiter2010_server

	if(isOrbiter2016())
		testProcAddr = 0x0043dea0;//Orbiter2016

	if(isOrbiter2016_server())
		testProcAddr = 0x00435bf0;//Orbiter2016_server


	//Check for exact orbiter version,
	//and provide alternative if not
	if(!isOrbiter2006p1() && !isOrbiter2010() && !isOrbiter2010_server() && !isOrbiter2016() && !isOrbiter2016_server())
	{
		InputBox_userPtr = userPtr;
		InputBox_clbk    = clbk;
		oapiOpenInputBox(title, InputBoxCallback, "", 20, NULL);
		return;
	}

	/*
	In the actual function 0x00431190, ecx (the this pointer)
	points to an InstrUser object. However, it is not
	dereferenced, so we can put any user-defined pointer
	in it.
	*/
	__asm
	{
		mov ecx, [userPtr];
		push [flags];
		push [refObj];
		push [clbk];
		push [title];
		call testProcAddr;
	};
}

/*
void oapiBaseTargetPopup(void *userPtr, char *title, menuClbk clbk, OBJHANDLE refObj, DWORD flags)
{
	DWORD testProcAddr = 0x00431190;//Orbiter2006P1

	if(isOrbiter2010())
		testProcAddr = 0x004a2c50;//Orbiter2010

	//Check for exact orbiter version,
	//and provide alternative if not
	if(!isOrbiter2006p1() && !isOrbiter2010())
	{
		InputBox_userPtr = userPtr;
		InputBox_clbk    = clbk;
		oapiOpenInputBox(title, InputBoxCallback, "", 20, NULL);
		return;
	}

	/*
	In the actual function 0x00431190, ecx (the this pointer)
	points to an InstrUser object. However, it is not
	dereferenced, so we can put any user-defined pointer
	in it.
	*/
/*	__asm
	{
		mov ecx, [userPtr];
		push [title];
		push [title];
		push [refObj];
		push [clbk];
		push [clbk];
		push [title];
		call testProcAddr;
	};
}
*/
void oapiSetHUDReference(OBJHANDLE refObj)
{
	//Check for exact orbiter version,
	//and ignore the call if not
	if(!isOrbiter2006p1()) return;

	DWORD procAddr1 = 0x0044D2A0;
	DWORD procAddr2 = 0x004519C0;

	void *globalObject = *((void **)0x4F4994);

	//Call a method of globalObject with argument value 1
	__asm
	{
		push        1;
		mov         ecx, globalObject;
		call        procAddr1;
	};

	void *hudObject = *(void **)((char *)globalObject + 0x198);
	if(hudObject == NULL) return;

	//Call a virtual method of hudObject
	void *vtable = *(void **)hudObject;
	DWORD returnValue;
	__asm
	{
		mov         edx,vtable;
		mov         ecx,hudObject;
		call        dword ptr [edx+4];
		mov         returnValue, eax;
	};
	if(returnValue != 1) return;

	//Set the HUD reference with a method of hudObject
	__asm
	{
		mov         eax,refObj;
		mov         ecx,hudObject;
		push        eax;
		call        procAddr2;
	};
}
