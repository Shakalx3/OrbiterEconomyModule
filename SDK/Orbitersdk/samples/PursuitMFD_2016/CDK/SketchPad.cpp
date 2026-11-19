///////////////////////////////////////////////////////////////////////////////
//
//	SketchPad.cpp
//
///////////////////////////////////////////////////////////////////////////////
#define _CRT_SECURE_NO_DEPRECATE

#include "SketchPad.h"


#pragma warning( disable : 4244)

SKPBOX skpinit(SKPBOX skpbox)
{//fit text to MFD size

	char sizeinit[MAXCOL+1] = "                                     "; // at least MAXCOL char should be present in the title
//	int Wmax = skpbox.skp->GetTextWidth("0",MAXCOL+1);
	int Wmax = skpbox.skp->GetTextWidth(sizeinit,0);
//	int chsize=skpbox.skp->GetCharSize();

	skpbox.dX=int(Wmax/MAXCOL);							//dx=HIWORD(chsize);
	skpbox.dY=int(skpbox.MFDh/MAXLIN);					//dy=LOWORD(chsize);

//sprintf(oapiDebugString(),"HIWORD(height):%i LOWORD(width):%i Wmax:%i", HIWORD(chsize), LOWORD(chsize), Wmax);

	skpbox.oX=int((skpbox.MFDw-Wmax)/2.0);
	if(skpbox.oX<0) skpbox.oX=0;
	skpbox.oY=int((skpbox.MFDh-skpbox.dY*(MAXLIN+1))/2.0);
	if(skpbox.oY<0) skpbox.oY=0;

//sprintf(oapiDebugString(),"skpbox.oX:%i (%i) skpbox.oY:%i (%i)  skpbox.MFDh:%i skpbox.MFDw:%i  Wmax:%i", skpbox.oX, skpbox.dX, skpbox.oY, skpbox.dY, skpbox.MFDh, skpbox.MFDw, Wmax);

	skpbox.dY=skpbox.dX;

	skpbox.align = false;	// flag to align next lines : allow MAXLIN separated or MAXCOL overlaped

	return skpbox;
}

DWORD COLOR(bool setcol, DWORD color, DWORD defcol)
{
	if(setcol) return color;
	return defcol;
}

// copy source to dest, truncated & null-terminated
void strunc(char *dest, char *source, int len)
{
	len = min(len,MAXCOL);
	strncpy(dest, source, len);
	dest[len] = '\0'; // add null-character
}

void skpTxt(SKPBOX sbox, int lin, int col, char *buf, COLORREF color)
{
	int len = min((int)strlen(buf),MAXCOL-col);

	sbox.skp->SetTextColor(color);
	sbox.skp->Text((col-1)*sbox.dX+sbox.oX,(lin-1)*sbox.dY+sbox.oY+sbox.dY*sbox.align/3,buf,len);
}

void skpTxt(SKPBOX sbox, int lin, int col, char *format, double data, COLORREF color)
{
	char string[MAXCOL];
	sprintf(string, format, data);

	char buftrunc[MAXCOL];
	strunc(buftrunc, string, MAXCOL-col);

	char str[MAXCOL] = "";
	strncat(str, buftrunc, strlen(buftrunc));

	skpTxt(sbox,lin,col,str,color);
}

void skpTxt(SKPBOX sbox, int lin, int col, char *buf, char *format, void *data, COLORREF color)
{
	char string[MAXCOL];
	sprintf(string, format, data);

	char buftrunc[MAXCOL];
	strunc(buftrunc, string, MAXCOL-col-strlen(buf));

	char str[MAXCOL] = "";
	strncat(str, buf, strlen(buf));
	strncat(str, buftrunc, strlen(buftrunc));

	skpTxt(sbox,lin,col,str,color);
}

void skpTxt(SKPBOX sbox, int lin, int col, char *buf, char *format, double data, COLORREF color)
{
	char string[MAXCOL];
	sprintf(string, format, data);

	char buftrunc[MAXCOL];
	strunc(buftrunc, string, MAXCOL-col-strlen(buf));

	char str[MAXCOL] = "";
	strncat(str, buf, strlen(buf));
	strncat(str, buftrunc, strlen(buftrunc));

	skpTxt(sbox,lin,col,str,color);
}

void skpSca(SKPBOX sbox, int lin, int col, double data, COLORREF color)
{
	char scale[MAXCOL];
	Scale(scale, data);

	char buftrunc[MAXCOL];
	strunc(buftrunc, scale, MAXCOL-col);

	char str[MAXCOL] = "";
	strncat(str, buftrunc, strlen(buftrunc));

	skpTxt(sbox,lin,col,str,color);
}

void skpSca(SKPBOX sbox, int lin, int col, char *buf, double data, COLORREF color)
{
	char scale[MAXCOL];
	Scale(scale, data);

	char buftrunc[MAXCOL];
	strunc(buftrunc, scale, MAXCOL-col-strlen(buf));

	char str[MAXCOL] = "";
	strncat(str, buf, strlen(buf));
	strncat(str, buftrunc, strlen(buftrunc));

	skpTxt(sbox,lin,col,str,color);
}

void DrawLine(SKPBOX sbox, RECT rect, int style, int width, COLORREF color)
{
	oapi::Pen *Lpen;
	Lpen=oapiCreatePen(style,width,color);
	sbox.skp->SetPen(Lpen);

	sbox.skp->Line((rect.left)*sbox.dX+sbox.oX, (rect.top)*sbox.dX+sbox.oY+sbox.dY*sbox.align/3,
		(rect.right)*sbox.dX+sbox.oX, (rect.bottom)*sbox.dX+sbox.oY+sbox.dY*sbox.align/3);

	oapiReleasePen(Lpen);
}

void DrawRect(SKPBOX sbox, RECT rect, int style, int width, bool filled, COLORREF color)
{
	oapi::Pen *Lpen;
	Lpen=oapiCreatePen(style,width,color);
	sbox.skp->SetPen(Lpen);

	oapi::Brush *Rbrush;
	if (filled) Rbrush=oapiCreateBrush(color);
	else Rbrush=oapiCreateBrush(rgb.BLACK);
	sbox.skp->SetBrush(Rbrush);

	sbox.skp->Rectangle((rect.left)*sbox.dX+sbox.oX, (rect.top)*sbox.dX+sbox.oY,
		(rect.right)*sbox.dX+sbox.oX, (rect.bottom)*sbox.dX+sbox.oY);

	oapiReleasePen(Lpen);
	oapiReleaseBrush(Rbrush);
}

void DrawLevel(SKPBOX sbox, RECT rect, int style, int width, double lvl, bool filled, COLORREF color)
{
	if(fabs(lvl) < 0.01) return;

	oapi::Pen *Lpen;
	Lpen=oapiCreatePen(style,width,color);
	sbox.skp->SetPen(Lpen);

	oapi::Brush *Rbrush;
	Rbrush=oapiCreateBrush(color);
	if (filled) sbox.skp->SetBrush(Rbrush);

	int x0 = rect.left*sbox.dX;
	int y0 = rect.top*sbox.dX;
	int x1 = rect.right*sbox.dX;
	int y1 = rect.bottom*sbox.dX;

	int V = (y1-y0)/2.0;
	int H = (x1-x0)/2.0;

//sprintf(oapiDebugString(),"(%.3f)  x0:%i x1:%i y0:%i y1:%i  V:%i  H:%i  sbox: h:%i w:%i oX:%i dX:%i oY:%i dY:%i",
//	oapiGetSimTime(), x0, x1, y0, y1, V, H, sbox.MFDh, sbox.MFDw, sbox.oX, sbox.dX, sbox.oY, sbox.dY);

	int origin, final;
	if (H < V){ // vertical
		if(lvl < 0.0){ // invert axis
			final = y0 + V;
			origin = final + lvl * V;
		}else{
			origin = y0 + V;
			final  = origin + lvl * V;
		}
		if(filled) sbox.skp->Rectangle(x0+sbox.oX, origin+sbox.oY, x1+sbox.oX, final+sbox.oY);
		else{
			x1 -= 1;
			// left
			sbox.skp->Line(x0+sbox.oX, origin+sbox.oY, x0+sbox.oX, final+sbox.oY);
			// right
			sbox.skp->Line(x1+sbox.oX, origin+sbox.oY, x1+sbox.oX, final+sbox.oY);
			// origin
			sbox.skp->Line(x0+sbox.oX, origin+sbox.oY, x1+sbox.oX, origin+sbox.oY);
			// final
			sbox.skp->Line(x0+sbox.oX, final-1+sbox.oY,  x1+sbox.oX, final-1+sbox.oY);
		}
	}
	else { // horizontal
		if(lvl < 0.0){ // invert axis
			final = x0 + H;
			origin = final + lvl * H;
		}else{
			origin = x0 + H;
			final  = origin + lvl * H;
		}
		if(filled) sbox.skp->Rectangle(origin+sbox.oX, rect.top*sbox.dX+sbox.oY, final+sbox.oX, rect.bottom*sbox.dX+sbox.oY);
		else{
			y1 -= 1;
			// up
			sbox.skp->Line(origin+sbox.oX, y0+sbox.oY, final+sbox.oX,  y0+sbox.oY);
			// down
			sbox.skp->Line(origin+sbox.oX, y1+sbox.oY, final+sbox.oX,  y1+sbox.oY);
			// origin
			sbox.skp->Line(origin+sbox.oX, y0+sbox.oY, origin+sbox.oX, y1+sbox.oY);
			// final
			sbox.skp->Line(final-1+sbox.oX,  y0+sbox.oY, final-1+sbox.oX,  y1+sbox.oY);
		}
	}

	oapiReleasePen(Lpen);
	oapiReleaseBrush(Rbrush);
}

void DrawArc(SKPBOX sbox, ARC arc, int style, int width, bool filled, COLORREF color)
{
	if(fabs(arc.origin-arc.sweep) < 0.01) return;

	oapi::Pen *Lpen;
	Lpen=oapiCreatePen(style,width,color);
	sbox.skp->SetPen(Lpen);

	oapi::Brush *Rbrush;
	Rbrush=oapiCreateBrush(color);
	if (filled) sbox.skp->SetBrush(Rbrush);

	oapi::IVECTOR2 border[180];

	int npts = 90;
	if (arc.width<1) arc.width=1;

	int x0 = arc.x0*sbox.dX+sbox.oX;
	int y0 = arc.y0*sbox.dX+sbox.oY;
	int rho = arc.rho*sbox.dX;
	int wdth = arc.width*sbox.dX;
	double angle1, angle2;

	for(int k=0; k<npts; k++)
	{
		angle1 = arc.origin+(arc.sweep-arc.origin)*k/(npts-1);
		angle2 = arc.sweep-(arc.sweep-arc.origin)*k/(npts-1);

		border[k].x = x0+rho*cos(angle1);
		border[k].y = y0-rho*sin(angle1);

		if (wdth>0){
			border[k+npts].x = x0+(rho-wdth)*cos(angle2);
			border[k+npts].y = y0-(rho-wdth)*sin(angle2);
		}
	}

//sprintf(oapiDebugString(),"oX:%i dX:%i oY:%i dY:%i   %i  %i",sbox.oX,sbox.dX,sbox.oY,sbox.dY,sbox.MFDw,sbox.MFDh);
//sprintf(oapiDebugString(),"circle:%i %i %i",x0,y0,rho);

	if (filled) sbox.skp->Polygon(border,2*npts);
//	if (arc.width>0) sbox.skp->Polygon(border,2*npts);
	else{
		// origin
		//sbox.skp->Line(border[0].x+sbox.oX, border[0].y+sbox.oY, border[2*npts-1].x+sbox.oX, border[2*npts-1].y+sbox.oY);
		// arc
		sbox.skp->Polyline(border,2*npts);
	}

	oapiReleasePen(Lpen);
	oapiReleaseBrush(Rbrush);
}


// END OF FILE