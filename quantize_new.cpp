// this file generates all the various combinations via quantize_common.h
// the hope is to squeeze out a few cycles by having fewer decisions and in some
// cases, fewer operations to perform.
//
// each new option added adds a lot of complexity HERE, however, and in the code
// that selects which function to call. But at least there is only one function
// to MAINTAIN.

#include "stdafx.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>
#include <math.h>
#include <process.h>
#include "D:\WORK\imgsource\4.0\islibs40_vs17_unicode\ISource.h"
#include "D:\WORK\imgsource\2.1\src\ISLib\isarray.h"
#include <windows.h>
#include <list>
#include <cmath>
#include "median_cut.h"

#include "TIPicView.h"
#include "TIPicViewDlg.h"

extern int g_MaxMultiDiff;			// default percentage that we allow colors to differ luminence by (255=100%)
extern double g_PercepR, g_PercepG, g_PercepB;
extern int g_UseColorOnly;
extern int g_UsePerLinePalette;
extern int g_MatchColors;				// number of non-paletted colors to be matched (used for B&W and greyscale)
extern int g_AccumulateErrors;			// accumulate errors instead of averaging (old method)
extern int g_UsePerLinePalette;
extern int g_UsePalette;				// single F18A palette over whole image
extern int g_StaticColors;				// fixed popular colors in per-scanline palette F18 mode (0-15 - note that 15 allows creation of static palette images)
extern bool g_bStaticByPopularity;		// select static colors by popularity (else use median cut - default is popularity)
extern int cols[4096];					// work space
extern unsigned char pal[256][4];		// RGB0 - in TIPicView
extern double YUVpal[256][4];			// YCrCb0 - misleadingly called YUV for easier typing
extern Point points[256];
extern RGBQUAD winpal[256];
extern unsigned char scanlinepal[192][16][4];	// RGB0
extern int PIXA,PIXB,PIXC,PIXD,PIXE,PIXF;
extern CWnd *pWnd;
extern bool g_bDisplayPalette;			// draw palette on line
extern int g_OrderedDither;
extern int g_MapSize;
extern double g_LumaEmphasis;
extern double g_Gamma;

// returns the distance squared on the YUV scale between two RGB entries
double yuvdist(double r1, double g1, double b1, double r2, double g2, double b2) {
	double y1,cr1,cb1;
	double y2,cr2,cb2;
	double t1,t2,t3;

	// make YCrCb
	makeYUV(r1, g1, b1, y1, cr1, cb1);
	makeYUV(r2, g2, b2, y2, cr2, cb2);

	// gets diffs
	t1=(y1-y2)*g_LumaEmphasis;
	t2=cr1-cr2;
	t3=cb1-cb2;
	
	return (t1*t1)+(t2*t2)+(t3*t3);
}
// returns the distance squared against yuvpal
// this is called from the inner loop and is pretty critical
double yuvpaldist(double r, double g, double b, int nCol) {
	double y,cr,cb;
	double t1,t2,t3;

	// make YCrCb
	makeYUV(r, g, b, y, cr, cb);

	// gets diffs - y (brightness) is exaggerated to reduce noise (particularly with grey and white)
	t1=(y-YUVpal[nCol][0])*g_LumaEmphasis;
	t2=cr-YUVpal[nCol][1];
	t3=cb-YUVpal[nCol][2];

	return (t1*t1)+(t2*t2)+(t3*t3);
}

// does a YUV conversion from in (RGB) to out (YUV)
// CHANGES out!
void makeYUV(double r, double g, double b, double &y, double &u, double &v) {
	// NOTE: I'm only using this for distance tests, so I do not add the offsets. Y should be +16, and the others +128, for 0-255 ranges.
	// also, due to error values, we often get out of range inputs, so don't assume :)

	// TODO: this function is really expensive. 9 multiplies takes around 10 seconds.. when 
	// I tested with 5 multiples instead, it dropped to 7 seconds. (a dumb test with
	// no multiplies took 8 seconds, so wtf? ;) )

	y = 0.299*r + 0.587*g + 0.114*b;
	u = 0.500*r - 0.419*g - 0.081*b;
	v =-0.169*r - 0.331*g + 0.500*b;

}

// does a Y-only conversion from in (RGB) to out (Y)
// CHANGES out!
void makeY(double r, double g, double b, double &y) {
	y = 0.299*r + 0.587*g + 0.114*b;
}


// standard mode
#undef quantize_common
#define quantize_common quantize_new
#undef QUANTIZE_FLICKER
#undef QUANTIZE_PERCEPTUAL
#undef QUANTIZE_ORDERED
#define ERROR_DISTRIBUTION
#include "quantize_common.h"

// standard mode with perceptual color matching
#undef quantize_common
#define quantize_common quantize_new_percept
#undef QUANTIZE_FLICKER
#define QUANTIZE_PERCEPTUAL
#undef QUANTIZE_ORDERED
#define ERROR_DISTRIBUTION
#include "quantize_common.h"

// standard mode ordered dither
#undef quantize_common
#define quantize_common quantize_new_ordered
#undef QUANTIZE_FLICKER
#undef QUANTIZE_PERCEPTUAL
#define QUANTIZE_ORDERED
#undef ERROR_DISTRIBUTION
#include "quantize_common.h"

// standard mode with perceptual color matching ordered dither
#undef quantize_common
#define quantize_common quantize_new_percept_ordered
#undef QUANTIZE_FLICKER
#define QUANTIZE_PERCEPTUAL
#define QUANTIZE_ORDERED
#undef ERROR_DISTRIBUTION
#include "quantize_common.h"

// standard mode ordered dither with error dist
#undef quantize_common
#define quantize_common quantize_new_ordered2
#undef QUANTIZE_FLICKER
#undef QUANTIZE_PERCEPTUAL
#define QUANTIZE_ORDERED
#define ERROR_DISTRIBUTION
#include "quantize_common.h"

// standard mode with perceptual color matching ordered dither with error dist
#undef quantize_common
#define quantize_common quantize_new_percept_ordered2
#undef QUANTIZE_FLICKER
#define QUANTIZE_PERCEPTUAL
#define QUANTIZE_ORDERED
#define ERROR_DISTRIBUTION
#include "quantize_common.h"

// half-multicolor mode
#undef quantize_common
#define quantize_common quantize_new_halfmult
#define QUANTIZE_FLICKER
#undef QUANTIZE_PERCEPTUAL
#undef QUANTIZE_ORDERED
#define ERROR_DISTRIBUTION
#include "quantize_common.h"

// half-multicolor mode with perceptual color matching
#undef quantize_common
#define quantize_common quantize_new_halfmult_percept
#define QUANTIZE_FLICKER
#define QUANTIZE_PERCEPTUAL
#undef QUANTIZE_ORDERED
#define ERROR_DISTRIBUTION
#include "quantize_common.h"

// half-multicolor mode ordered dither
#undef quantize_common
#define quantize_common quantize_new_halfmult_ordered
#define QUANTIZE_FLICKER
#undef QUANTIZE_PERCEPTUAL
#define QUANTIZE_ORDERED
#undef ERROR_DISTRIBUTION
#include "quantize_common.h"

// half-multicolor mode with perceptual color matching ordered dither
#undef quantize_common
#define quantize_common quantize_new_halfmult_percept_ordered
#define QUANTIZE_FLICKER
#define QUANTIZE_PERCEPTUAL
#define QUANTIZE_ORDERED
#undef ERROR_DISTRIBUTION
#include "quantize_common.h"

// half-multicolor mode ordered dither with error dist
#undef quantize_common
#define quantize_common quantize_new_halfmult_ordered2
#define QUANTIZE_FLICKER
#undef QUANTIZE_PERCEPTUAL
#define QUANTIZE_ORDERED
#define ERROR_DISTRIBUTION
#include "quantize_common.h"

// half-multicolor mode with perceptual color matching ordered dither with error dist
#undef quantize_common
#define quantize_common quantize_new_halfmult_percept_ordered2
#define QUANTIZE_FLICKER
#define QUANTIZE_PERCEPTUAL
#define QUANTIZE_ORDERED
#define ERROR_DISTRIBUTION
#include "quantize_common.h"
