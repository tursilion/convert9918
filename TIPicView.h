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
// this structure is assumed in quantize_new where it's accessed as unsigned chars, be careful
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
// Try the Sometimes99er method of doubling up the nibble - no rounding (much better)
// Both macros do the same thing.
#define MakeRoundedRGB(x)      (((x)&0xf0)|((x>>4)&0xf))
#define MakeTrulyRoundedRGB(x) (((x)&0xf0)|((x>>4)&0xf))

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

double yuvdist(double r1, double g1, double b1, double r2, double g2, double b2);
double yuvpaldist(double r1, double g1, double b1, int nCol);
void makeYUV(double r, double g, double b, double &y, double &u, double &v);
void makeY(double r, double g, double b, double &y);
void buildlookup();

#endif // !defined(AFX_TIPICVIEW_H__F4725884_9A29_4492_AE12_0E3B106C2738__INCLUDED_)

