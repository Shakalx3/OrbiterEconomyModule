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
 *	--change addresses for InputBox to be used in 2010 version
 *
 ***************************************************************************/
#ifndef ORBITERSDK_EXT_CJP_H
#define ORBITERSDK_EXT_CJP_H

#include "orbitersdk.h"

/*
Callback arguments:
void *pointer_to_menu
unsigned int menu_index
char *item_name
void *userPtr
*/
typedef bool (*menuClbk)(void *, unsigned int, char *, void *);

#define MENU_HIDE_SUN 0x00000001


void oapiCelBodyPopup(void *userPtr, char *title, menuClbk clbk, DWORD flags);

void oapiTargetPopup(void *userPtr, char *title, menuClbk clbk, OBJHANDLE refObj, DWORD flags);

//void oapiBaseTargetPopup(void *userPtr, char *title, menuClbk clbk, OBJHANDLE refObj, DWORD flags);

void oapiSetHUDReference(OBJHANDLE refObj);

#endif
