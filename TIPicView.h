// TIPicView.h : main header file for the TIPICVIEW application
//

#if !defined(AFX_TIPICVIEW_H__F4725884_9A29_4492_AE12_0E3B106C2738__INCLUDED_)
#define AFX_TIPICVIEW_H__F4725884_9A29_4492_AE12_0E3B106C2738__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

typedef unsigned int UINT32;
typedef unsigned char uchar;
typedef unsigned long ulong;
typedef unsigned long slong;
typedef short sshort;
typedef char schar;

#define XOFFSET 250

#define DPIFIX(xx) MulDiv((xx), dpi, 96)

#pragma pack(push)
#pragma pack(1)
typedef struct tagMYRGBQUAD {
	// mine is RGB instead of BGR
        BYTE    rgbRed;
        BYTE    rgbGreen;
        BYTE    rgbBlue;
        BYTE    rgbReserved;
} MYRGBQUAD;
#pragma pack(pop)

// change between float or double here
// for all that effort, though, float is NOT faster than double
//typedef double double;

// we manually wrap the Win10 function GetDpiForWindow()
UINT WINAPI GetDpiForWindow(_In_ HWND hWnd);

/////////////////////////////////////////////////////////////////////////////
// CTIPicViewApp:
// See TIPicView.cpp for the implementation of this class
//

class CTIPicViewApp : public CWinApp
{
public:
	CTIPicViewApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTIPicViewApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
	void loadSettings();
	void saveSettings();

	//{{AFX_MSG(CTIPicViewApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

void debug(wchar_t *s, ...);

// This macro basically reduces to 4 bits of accuracy, but rounds up and shifts back to the 8-bit RGB range (so high nibble only)
// I think this is causing a lot of my streaking...
// so for now, we are working in true color, which is great, but we are generating truecolor palettes when
// the chip can only do 12-bit color. So we'll get loss of detail and banding that we really should be
// dithering. But when we do this correction in the palette, it sometimes results in different palette selection.
// So.. do we do this after the palette selection but before the dither?
//#define MakeRoundedRGB(x) ((((((int)(x)<256-8)?(x)+8:(x))>>4)<<4)+8)
#define MakeRoundedRGB(x) (x)
#define MakeTrulyRoundedRGB(x) ((((((int)(x)<256-8)?(x)+8:(x))>>4)<<4)+8)

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

double yuvdist(double r1, double g1, double b1, double r2, double g2, double b2);
double yuvpaldist(double r1, double g1, double b1, int nCol);
void makeYUV(double r, double g, double b, double &y, double &u, double &v);
void makeY(double r, double g, double b, double &y);
void buildlookup();

#endif // !defined(AFX_TIPICVIEW_H__F4725884_9A29_4492_AE12_0E3B106C2738__INCLUDED_)

