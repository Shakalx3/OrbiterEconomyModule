///////////////////////////////////////////////////////////////////////////////
//	SketchPad.h
//
//		Library of graphical functions for MFD display
//
///////////////////////////////////////////////////////////////////////////////
#ifndef SKETCHPAD_H
#define SKETCHPAD_H
#define _CRT_SECURE_NO_DEPRECATE

#include "Orbitersdk.h"

static const int MAXLIN = 23; // num max of lines
static const int MAXCOL = 37; // num max of columns (including '\0' termination)

typedef struct {
	oapi::Sketchpad *skp;
	int MFDw;
	int MFDh;
	int dX;
	int dY;
	int oX;
	int oY;
	int align;
} SKPBOX;

typedef struct {
	int x0;
	int y0;
	int rho;
	int width;
	double origin;
	double sweep;
} ARC;

SKPBOX skpinit(SKPBOX skpbox);

static const DWORD StdColor = RGB(0xFF, 0xFF, 0x00); // YELLOWBRIGHT

DWORD COLOR(bool setcol, DWORD color, DWORD defcol = StdColor);

void strunc(char *dest, char *source, int len = MAXCOL);
//void strunc2(char *dest, char *source, int len = MAXCOL);
//char* strunc(char *source, int len = MAXCOL);

void skpTxt(SKPBOX sbox, int lin, int col, char *buf, COLORREF color = StdColor);
void skpTxt(SKPBOX sbox, int lin, int col, char *format, double data, COLORREF color = StdColor);
void skpTxt(SKPBOX sbox, int lin, int col, char *buf, char *format, void *data, COLORREF color = StdColor);
void skpTxt(SKPBOX sbox, int lin, int col, char *buf, char *format, double data, COLORREF color = StdColor);

void skpSca(SKPBOX sbox, int lin, int col, double data, COLORREF color = StdColor);
void skpSca(SKPBOX sbox, int lin, int col, char *buf, double data, COLORREF color = StdColor);

void DrawLine(SKPBOX sbox, RECT rect, int style, int width, COLORREF color = StdColor);
void DrawRect(SKPBOX sbox, RECT rect, int style, int width, bool filled, COLORREF color = StdColor);
void DrawArc(SKPBOX sbox, ARC arc, int style, int width, bool filled, COLORREF color = StdColor);

void DrawLevel(SKPBOX sbox, RECT rect, int style, int width, double lvl, bool filled, COLORREF color = StdColor);

// Scale a value before it's displayed. Needs 7 chars. Distance is in meters.
inline void Scale(char *buf, double data)
{
	char *unit = "AU";
	double coef = AU;
	int width = 6;
	int precision = 1;

	if (fabs(data) < 1e03) {
		unit = "";
		coef = 1;
	}
	else if (fabs(data) < 1e06) {
		unit = "k";
		coef = 1e03;
	}
	else if (fabs(data) < 1e09) {
		unit = "M";
		coef = 1e06;
	}
	else if (fabs(data) < AU) {
		unit = "G";
		coef = 1e09;
	}

	if (fabs(data/coef) < 1)
		precision = 3;
	else if (fabs(data/coef) < 10)
		precision = 3;
	else if (fabs(data/coef) < 100)
		precision = 2;
	else if (fabs(data/coef) < 1000)
		precision = 1;
/*
	if (fabs(data) > AU) {
		width = 5;
		precision--;
	}
*/
	sprintf(buf, "%*.*f", width, precision, data/coef);

	strcat(buf, unit);
}

// standard color names
static struct rgb {
//	DWORD STD;
	static const DWORD BLACK			= RGB(0x00, 0x00, 0x00);
	static const DWORD BLUELIGHT		= RGB(0x00, 0x00, 0x80);
	static const DWORD BLUE				= RGB(0x00, 0x00, 0xC0);
	static const DWORD BLUEBRIGHT		= RGB(0x00, 0x00, 0xFF);
	static const DWORD BlueNeon			= RGB(0x4D, 0x4D, 0xFF);
	static const DWORD BlueSky			= RGB(0x00, 0xBF, 0xFF);
	static const DWORD Brown			= RGB(0xA5, 0x2A, 0x2A);
	static const DWORD BrownLight		= RGB(0x7F, 0xFF, 0xD4); // Aquamarine
	static const DWORD CYAN				= RGB(0x00, 0xC0, 0xC0);
	static const DWORD CYANBRIGHT		= RGB(0x00, 0xFF, 0xFF);
	static const DWORD CYANLIGHT		= RGB(0x00, 0x80, 0x80);
	static const DWORD GREEN			= RGB(0x00, 0xC0, 0x00);
	static const DWORD GREENBRIGHT		= RGB(0x00, 0xFF, 0x00);
	static const DWORD GreenForest		= RGB(0x23, 0x8E, 0x23);
	static const DWORD GREENLIGHT		= RGB(0x00, 0x80, 0x00);
	static const DWORD GreenLime		= RGB(0x32, 0xCD, 0x32);
	static const DWORD MAGENTA			= RGB(0xC0, 0x00, 0xC0);
	static const DWORD MAGENTABRIGHT	= RGB(0xFF, 0x00, 0xFF);
	static const DWORD MAGENTALIGHT		= RGB(0x80, 0x00, 0x80);
	static const DWORD OrangeDark		= RGB(0xFF, 0x8C, 0x00);
	static const DWORD OrangeLight		= RGB(0xE9, 0x96, 0x7A); // DarkSalmon
	static const DWORD PinkDeep			= RGB(0xFF, 0x14, 0x93);
	static const DWORD PinkLight		= RGB(0xFF, 0x69, 0xB4); // HotPink
	static const DWORD RED				= RGB(0xC0, 0x00, 0x00);
	static const DWORD REDBRIGHT		= RGB(0xFF, 0x00, 0x00);
	static const DWORD REDLIGHT			= RGB(0x80, 0x00, 0x00);
	static const DWORD RedTomato		= RGB(0xFF, 0x63, 0x47);
	static const DWORD Turquoise		= RGB(0x40, 0xE0, 0xD0);
	static const DWORD Violet			= RGB(0x99, 0x32, 0xCD); // DarkOrchid
	static const DWORD WHITE			= RGB(0xC0, 0xC0, 0xC0);
	static const DWORD WHITEBRIGHT		= RGB(0xFF, 0xFF, 0xFF);
	static const DWORD WHITELIGHT		= RGB(0x80, 0x80, 0x80);
	static const DWORD YELLOW			= RGB(0xC0, 0xC0, 0x00);
	static const DWORD YELLOWBRIGHT		= RGB(0xFF, 0xFF, 0x00);
	static const DWORD YELLOWLIGHT		= RGB(0x80, 0x80, 0x00);
}rgb;


#endif

// END OF FILE