// TIPicView.cpp : Defines the class behaviors for the application.
//

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

// define this to enable HAM4 (note that it is not integrated into the menus yet, it overrides multicolor mode)
// It looks great! and fits in RAM! But the F18A can't change palette accurately enough to implement on hardware.
// We can revisit if we get a hardware-based way to trigger the GPU on a scanline (HBLANK start register?)
// It still needs dithering added.
// TODO: we got that reliable start. But can we change colors every 4 pixels?? I don't think so, and it
// looks like crap at 16 pixels.
// TODO: need to retest real chip... didn't I find I could only change once every 8 pixels or so?
// Frankly.. scanline pallette has improved a lot since I did this, and it looks pretty comparable, so
// probably there's no need to pursue this difficult (and unlikely) mode.. ;)
// Retested 1/3/2019 - Scanline palette is now superior is nearly every image, no need to pursue
//#define ALLOWHAM4

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// whether to enable the half-multicolor mode
int g_UseHalfMulticolor = 0;
// whether to do multicolor only
int g_UseMulticolorOnly = 0;
// whether to use color table only
int g_UseColorOnly = 0;
// really nasty - separate 16-color palette per scanline
int g_UsePerLinePalette = 0;
int g_UsePalette = 0;				// single F18A palette over whole image
int g_GreyPalette = 0;				// assume palette is grey scale
int g_StaticColors = 0;				// fixed popular colors in per-scanline palette F18 mode (0-15 - note that 15 allows creation of static palette images)
bool g_bStaticByPopularity = false;	// select static colors by popularity (else use median cut - default is popularity)
									// popularity tends to preserve more detail - but, make it an option anyway
bool g_bDisplayPalette = false;		// draw palette on line
int g_MatchColors = 15;				// number of non-paletted colors to be matched (used for B&W and greyscale)

uchar pal[256][4];	// RGB0
double YUVpal[256][4];	// YCrCb0 - misleadingly called YUV for easier typing
unsigned char scanlinepal[192][16][4];	// RGB0
int pixels[32][8][4];
extern MYRGBQUAD palinit16[256], defaultpalinit16[16];
int cols[4096];				// work space
Point points[256];

// hacky command line interface
wchar_t cmdLineCopy[1024];
wchar_t *cmdFileOut, *cmdFileIn;
#define MAX_OPTIONS 128
wchar_t *cmdOptions[MAX_OPTIONS];
int numOptions;

int nCurrentByte;
int PIXA,PIXB,PIXC,PIXD,PIXE,PIXF;
int g_orderSlide = 0;			// the step (n/17) that is subtracted from the ordered dither pattern to darken
int g_nFilter=4;
int g_nPortraitMode=0;
int g_Perceptual=0;				// enable perceptual RGB color matching instead of YCrCb
int g_AccumulateErrors=1;		// accumulate errors instead of averaging (old method)
int g_MaxMultiDiff=95;			// default percentage that we allow colors to differ luminence by (255=100%)
int g_MaxColDiff=1;				// max color shift allowed (>15 is pretty pointless - percentage of color space)
int g_OrderedDither=0;			// whether to use the built-in ordered dither 1 or 2
int g_MapSize=2;				// whether ordered dither uses 2x2 or 4x4 maps
double g_PercepR=0.30, g_PercepG=0.52, g_PercepB=0.18;
double g_LumaEmphasis = 1.2;		// much like the perceptual shifts, this emphasizes (or de-emphasizes if less than 1) the luma component
double g_Gamma = 1.0;

extern unsigned char buf8[256*192];
extern RGBQUAD winpal[256];
extern CWnd *pWnd;
extern bool StretchHist;
extern int pixeloffset;
extern int heightoffset;
extern int ScaleMode;
extern bool fVerbose;

void ConvertToHalfMulticolor(unsigned char *pOrig, BYTE *pDest, MYRGBQUAD *inpal);
void quantize_new(BYTE* pRGB, BYTE* p8Bit, double dark, int mapsize);
void quantize_new_percept(BYTE* pRGB, BYTE* p8Bit, double dark, int mapsize);
void quantize_new_halfmult(BYTE* pRGB, BYTE* p8Bit, double dark, int mapsize);
void quantize_new_halfmult_percept(BYTE* pRGB, BYTE* p8Bit, double dark, int mapsize);
void quantize_new_ordered(BYTE* pRGB, BYTE* p8Bit, double dark, int mapsize);
void quantize_new_percept_ordered(BYTE* pRGB, BYTE* p8Bit, double dark, int mapsize);
void quantize_new_ordered2(BYTE* pRGB, BYTE* p8Bit, double dark, int mapsize);
void quantize_new_percept_ordered2(BYTE* pRGB, BYTE* p8Bit, double dark, int mapsize);
void quantize_new_halfmult_ordered(BYTE* pRGB, BYTE* p8Bit, double dark, int mapsize);
void quantize_new_halfmult_percept_ordered(BYTE* pRGB, BYTE* p8Bit, double dark, int mapsize);
void quantize_new_halfmult_ordered2(BYTE* pRGB, BYTE* p8Bit, double dark, int mapsize);
void quantize_new_halfmult_percept_ordered2(BYTE* pRGB, BYTE* p8Bit, double dark, int mapsize);

#ifdef ALLOWHAM4
void ConvertToHAM4(unsigned char *pOrig, BYTE *pDest, MYRGBQUAD *inpal);
#endif

/////////////////////////////////////////////////////////////////////////////

// we manually wrap the Win10 function GetDpiForWindow()
// if it's not available, we just return the default of 96
UINT WINAPI GetDpiForWindow(_In_ HWND hWnd) {
	static UINT (WINAPI *getDpi)(_In_ HWND) = NULL;
	static bool didSearch = false;

	if (!didSearch) {
		didSearch = true;
		HMODULE hMod = LoadLibrary(_T("user32.dll"));
		if (NULL == hMod) {
			printf("Failed to load user32.dll (what? really??)\n");
		} else {
			getDpi = (UINT (WINAPI *)(_In_ HWND))GetProcAddress(hMod, "GetDpiForWindow");
			if (NULL == getDpi) {
				printf("Failed to find GetDpiForWindow, defaulting to 96dpi\n");
			} else {
				if (fVerbose) {
					printf("Got GetDpiForWindow, should be able to auto-scale.\n");
				}
			}
		}
	}

	if (NULL == getDpi) {
		return 96;
	} else {
		return getDpi(hWnd);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CTIPicViewApp

BEGIN_MESSAGE_MAP(CTIPicViewApp, CWinApp)
	//{{AFX_MSG_MAP(CTIPicViewApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTIPicViewApp construction

CTIPicViewApp::CTIPicViewApp()
{
	// add construction code here,
	// Place all significant initialization in InitInstance
}

#define INIFILE _T("convert9918.ini")
wchar_t INIFILEPATH[1024];

// helpers
void writeint(int n, wchar_t *name) {
	wchar_t buf[32];
	_swprintf(buf, _T("%d"), n);
	WritePrivateProfileString(_T("Convert9918"), name, buf, INIFILEPATH);
}

void writefloat(double n, wchar_t *name) {
	wchar_t buf[64];
	int x = (int)(n*1000);
	_swprintf(buf, _T("%d"), x);
	WritePrivateProfileString(_T("Convert9918"), name, buf, INIFILEPATH);
}

void writebool(bool n, wchar_t *name) {
	if (n) {
		WritePrivateProfileString(_T("Convert9918"), name, _T("1"), INIFILEPATH);
	} else {
		WritePrivateProfileString(_T("Convert9918"), name, _T("0"), INIFILEPATH);
	}
}

void writequad(MYRGBQUAD n, wchar_t *name, wchar_t *key) {
	wchar_t buf[64];
	_swprintf(buf, _T("%d,%d,%d,%d"), n.rgbRed, n.rgbGreen, n.rgbBlue, n.rgbReserved);
	WritePrivateProfileString(key, name, buf, INIFILEPATH);
}

void profileString(wchar_t *group, wchar_t *key, wchar_t *defval, wchar_t *outbuf, int outsize, wchar_t *path) {
	// wraps GetPrivateProfileString and also checks the command line for easy override
	// note that the command line ignores group, so no duplicate key names are allowed
	// in the INI, groups there are really just for human readability.
	// note that regardless of command line or not, debug is only emitted in this function
	// when fVerbose is set.

	wchar_t workbuf[128];
	bool bFound = false;

	// apply default value
	wcsncpy(outbuf, defval, outsize);
	outbuf[outsize-1]='\0';

	// check command line for key
	_snwprintf(workbuf, 128, _T("/%s="), key);
	workbuf[127]=_T('\0');

	for (int idx=0; idx<numOptions; idx++) {
		if (0 == wcsncmp(cmdOptions[idx], workbuf, wcslen(workbuf))) {
			// copy out just the data part - we know where the equals sign is!
			wcsncpy(outbuf, &cmdOptions[idx][wcslen(workbuf)], outsize);
			outbuf[outsize-1]=_T('\0');
			if (fVerbose) {
				debug(_T("%s/%s = %s (cmdline)\n"), group, key, outbuf);
			}
			bFound = true;
			break;
		}
	}

	if (!bFound) {
		// usual case, read from the INI instead
		GetPrivateProfileString(group, key, defval, outbuf, outsize, path);
		if (fVerbose) {
			debug(_T("%s/%s = %s\n"), group, key, outbuf);
		}
	}
}

int profileInt(wchar_t *group, wchar_t *key, int defval, wchar_t *path) {
	// return value as int
	wchar_t buf[64], bufout[64];
	swprintf(buf, 64, _T("%d"), defval);

	profileString(group, key, buf, bufout, 64, path);
	return _wtoi(bufout);
}

void readint(int &n, wchar_t *key) {
	int x;
	x = profileInt(_T("Convert9918"), key, -9941, INIFILEPATH);
	if (x != -9941) n=x;
}

void readfloat(double &n, wchar_t *key) {
	int x;
	x = profileInt(_T("Convert9918"), key, -9941, INIFILEPATH);
	if (x != -9941) {
		n = (double)x / 1000.0;
	}
}

void readbool(bool &n, wchar_t *key) {
	int x;
	x = profileInt(_T("Convert9918"), key, -9941, INIFILEPATH);
	if (x != -9941) {
		if (x) {
			n = true;
		} else {
			n = false;
		}
	}
}

void readquad(MYRGBQUAD &n, wchar_t *key, wchar_t *group) {
	wchar_t buf[64];
	int r,g,b,a;

	profileString(group, key, _T(""), buf, 64, INIFILEPATH);
	if (buf[0] == '\0') return;
	if (4 != swscanf(buf, _T("%d,%d,%d,%d"), &r, &g, &b, &a)) return;
	n.rgbRed=r&0xff;
	n.rgbGreen=g&0xff;
	n.rgbBlue=b&0xff;
	n.rgbReserved=a&0xff;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CTIPicViewApp object

CTIPicViewApp theApp;

// settings - just load/save off the globals
void CTIPicViewApp::loadSettings() {
	if (0 == GetCurrentDirectory(sizeof(INIFILEPATH), INIFILEPATH)) {
		wcscpy(INIFILEPATH, _T(".\\") INIFILE);
	} else {
		wcscat(INIFILEPATH, _T("\\"));
		wcscat(INIFILEPATH, INIFILE);
	}

	readint(PIXA,               _T("PIXA"));
	readint(PIXB,               _T("PIXB"));
	readint(PIXC,               _T("PIXC"));
	readint(PIXD,               _T("PIXD"));
	readint(PIXE,               _T("PIXE"));
	readint(PIXF,               _T("PIXF"));
	readint(g_orderSlide,       _T("OrderSlide"));
	readint(g_nFilter,          _T("Filter"));
	readint(g_nPortraitMode,    _T("PortraitMode"));
	readint(g_Perceptual,       _T("Perceptual"));
	readint(g_AccumulateErrors, _T("AccumulateErrors"));
	readint(g_MaxMultiDiff,     _T("MaxMultiDiff"));
	readint(g_MaxColDiff,       _T("MaxColDiff"));
	readint(g_OrderedDither,    _T("OrderedDither"));
	readint(g_MapSize,          _T("MapSize"));
	if ((g_MapSize!=2)&&(g_MapSize!=4)) g_MapSize=2;
	readfloat(g_PercepR,        _T("PerceptR"));
	readfloat(g_PercepG,        _T("PerceptG"));
	readfloat(g_PercepB,        _T("PerceptB"));
	readfloat(g_LumaEmphasis,   _T("LumaEmphasis"));
	readfloat(g_Gamma,          _T("Gamma"));
	readbool(StretchHist,       _T("StretchHistogram"));
	readint(pixeloffset,        _T("PixelOffset"));
	readint(heightoffset,       _T("HeightOffset"));
	readint(ScaleMode,          _T("ScaleMode"));
	readquad(palinit16[0],      _T("white"),    _T("palette"));
	readquad(palinit16[1],      _T("black"),    _T("palette"));
	readquad(palinit16[2],      _T("grey"),     _T("palette"));
	readquad(palinit16[3],      _T("medGreen"), _T("palette"));
	readquad(palinit16[4],      _T("ltGreen"),  _T("palette"));
	readquad(palinit16[5],      _T("dkBlue"),   _T("palette"));
	readquad(palinit16[6],      _T("ltBlue"),   _T("palette"));
	readquad(palinit16[7],      _T("dkRed"),    _T("palette"));
	readquad(palinit16[8],      _T("cyan"),     _T("palette"));
	readquad(palinit16[9],      _T("medRed"),   _T("palette"));
	readquad(palinit16[10],     _T("ltRed"),    _T("palette"));
	readquad(palinit16[11],     _T("dkYellow"), _T("palette"));
	readquad(palinit16[12],     _T("ltYellow"), _T("palette"));
	readquad(palinit16[13],     _T("dkGreen"),  _T("palette"));
	readquad(palinit16[14],     _T("magenta"),  _T("palette"));
}

void CTIPicViewApp::saveSettings() {
	SetCurrentDirectory(INIFILEPATH);

	writeint(PIXA,                  _T("PIXA"));
	writeint(PIXB,                  _T("PIXB"));
	writeint(PIXC,                  _T("PIXC"));
	writeint(PIXD,                  _T("PIXD"));
	writeint(PIXE,                  _T("PIXE"));
	writeint(PIXF,                  _T("PIXF"));
	writeint(g_orderSlide,          _T("OrderSlide"));
	writeint(g_nFilter,             _T("Filter"));
	writeint(g_nPortraitMode,       _T("PortraitMode"));
	writeint(g_Perceptual,          _T("Perceptual"));
	writeint(g_AccumulateErrors,    _T("AccumulateErrors"));
	writeint(g_MaxMultiDiff,        _T("MaxMultiDiff"));
	writeint(g_MaxColDiff,          _T("MaxColDiff"));
	writeint(g_OrderedDither,       _T("OrderedDither"));
	writeint(g_MapSize,             _T("MapSize"));
	writefloat(g_PercepR,           _T("PerceptR"));
	writefloat(g_PercepG,           _T("PerceptG"));
	writefloat(g_PercepB,           _T("PerceptB"));
	writefloat(g_LumaEmphasis,      _T("LumaEmphasis"));
	writefloat(g_Gamma,             _T("Gamma"));
	writebool(StretchHist,          _T("StretchHistogram"));
	writeint(pixeloffset,           _T("PixelOffset"));
	writeint(heightoffset,          _T("HeightOffset"));
	writeint(ScaleMode,             _T("ScaleMode"));
	writequad(palinit16[0],         _T("white"),        _T("palette"));
    writequad(palinit16[1],         _T("black"),        _T("palette"));
	writequad(palinit16[2],         _T("grey"),         _T("palette"));
	writequad(palinit16[3],         _T("medGreen"),     _T("palette"));
	writequad(palinit16[4],         _T("ltGreen"),      _T("palette"));
	writequad(palinit16[5],         _T("dkBlue"),       _T("palette"));
	writequad(palinit16[6],         _T("ltBlue"),       _T("palette"));
	writequad(palinit16[7],         _T("dkRed"),        _T("palette"));
	writequad(palinit16[8],         _T("cyan"),         _T("palette"));
	writequad(palinit16[9],         _T("medRed"),       _T("palette"));
	writequad(palinit16[10],        _T("ltRed"),        _T("palette"));
	writequad(palinit16[11],        _T("dkYellow"),     _T("palette"));
	writequad(palinit16[12],        _T("ltYellow"),     _T("palette"));
	writequad(palinit16[13],        _T("dkGreen"),      _T("palette"));
	writequad(palinit16[14],        _T("magenta"),      _T("palette"));
	// no duplicate key names are allowed in this app due to command line overrides
	writequad(defaultpalinit16[0],  _T("def_white"),    _T("default_palette"));
	writequad(defaultpalinit16[1],  _T("def_black"),    _T("default_palette"));
	writequad(defaultpalinit16[2],  _T("def_grey"),     _T("default_palette"));
	writequad(defaultpalinit16[3],  _T("def_medGreen"), _T("default_palette"));
	writequad(defaultpalinit16[4],  _T("def_ltGreen"),  _T("default_palette"));
	writequad(defaultpalinit16[5],  _T("def_dkBlue"),   _T("default_palette"));
	writequad(defaultpalinit16[6],  _T("def_ltBlue"),   _T("default_palette"));
	writequad(defaultpalinit16[7],  _T("def_dkRed"),    _T("default_palette"));
	writequad(defaultpalinit16[8],  _T("def_cyan"),     _T("default_palette"));
	writequad(defaultpalinit16[9],  _T("def_medRed"),   _T("default_palette"));
	writequad(defaultpalinit16[10], _T("def_ltRed"),    _T("default_palette"));
	writequad(defaultpalinit16[11], _T("def_dkYellow"), _T("default_palette"));
	writequad(defaultpalinit16[12], _T("def_ltYellow"), _T("default_palette"));
	writequad(defaultpalinit16[13], _T("def_dkGreen"),  _T("default_palette"));
	writequad(defaultpalinit16[14], _T("def_magenta"),  _T("default_palette"));

}

void GetConsole() {
	static bool gotIt = false;	// only once per app

	if (gotIt) return;
	gotIt=true;

	if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
		// create a console for debugging
		AllocConsole();
	}

#if 1
    freopen("CONOUT$", "w", stdout);
#else
	int hCrt, i;
	FILE *hf;
	hCrt = _open_osfhandle((long) GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
	hf = _fdopen( hCrt, "w" );
	*stdout = *hf;
	i = setvbuf( stdout, NULL, _IONBF, 0 ); 
#endif
}

/////////////////////////////////////////////////////////////////////////////
// CTIPicViewApp initialization
BOOL CTIPicViewApp::InitInstance()
{
	// command line - most behaviour is determined by cmdFileXxx
	// cmdOptions just stores all the /Option=x string pointers
	// cmdXXX vars are just pointers into cmdLineCopy, which has
	// spaces replaced with NULs.
	cmdFileOut = NULL;
	cmdFileIn = NULL;
	memset(cmdOptions, 0, sizeof(cmdOptions));
	numOptions = 0;

	// really shouldn't parse this ourselves, but it's late and I'm tired.
	// and now I build upon that evil! Muhaha! It's not even late this time!
	int tokenStart = -1;
	int idx=0;
	bool quote=false;
	if (m_lpCmdLine[0] != '\0') {
		wcsncpy(cmdLineCopy, m_lpCmdLine, sizeof(cmdLineCopy));
		cmdLineCopy[sizeof(cmdLineCopy)-1] = _T('\0');
		if (wcslen(cmdLineCopy) < sizeof(cmdLineCopy)-2) {
			// pad with a space so we parse the last option
			wcscat(cmdLineCopy, _T(" "));
		}

		while (cmdLineCopy[idx] != _T('\0')) {
			// handle escaped characters and quoted strings
			if (cmdLineCopy[idx] == _T('\\')) {
				idx++;
				if (cmdLineCopy[idx] != _T('\0')) idx++;
				continue;
			}
			if (cmdLineCopy[idx] == _T('\"')) {
				quote=!quote;
				++idx;
				continue;
			}
			if ((cmdLineCopy[idx] == _T(' ')) && (!quote)) {
				cmdLineCopy[idx] = _T('\0');
				if (tokenStart != -1) {
					// we had a string, what was it?
					if (cmdLineCopy[tokenStart] == _T('/')) {
						// as an aside, check immediates here
						if (0 == wcscmp(_T("/verbose"), &cmdLineCopy[tokenStart])) {
							GetConsole();	// open the console now
							fVerbose = true;
							debug(_T("Verbose mode on"));
						} else if (0 == wcscmp(_T("/?"), &cmdLineCopy[tokenStart])) {
							printf(" /verbose = enable verbose output\n");
							printf(" Any other option from the INI may be specified as \"/option=value\" - see INI.\n");
							ExitProcess(0);
						} else if (numOptions < MAX_OPTIONS) {
							// now save off the option in the array
							cmdOptions[numOptions++] = &cmdLineCopy[tokenStart];
						} else {
							printf("** Error, more than %d options can not be read.\n", MAX_OPTIONS);
							ExitProcess(-10);
						}
					} else {
						// should be one of the filenames - in then out
						if (NULL == cmdFileIn) {
							cmdFileIn = &cmdLineCopy[tokenStart];
						} else if (NULL == cmdFileOut) {
							cmdFileOut = &cmdLineCopy[tokenStart];
						} else {
							printf("** Error - too many filenames on command line.\n");
							ExitProcess(-10);
						}
					}
					tokenStart = -1;
				}
			}
			if ((cmdLineCopy[idx]!=_T('\0'))&&(!isspace(cmdLineCopy[idx]))&&(tokenStart == -1)) {
				tokenStart = idx;
			}
			++idx;
		}
		// printf can handle NULL, so this is okay
		printf("File In: %S\nFileOut: %S\n", cmdFileIn, cmdFileOut);
	}

	// we might already have it depending on the command line
	GetConsole();

	// bring up the dialog
	CTIPicViewDlg dlg;
	m_pMainWnd = &dlg;

	// backup palinit16 before we load settings
	memcpy(defaultpalinit16, palinit16, sizeof(MYRGBQUAD)*16);

	// load settings (IF there is an INI)
	loadSettings();

	// don't care about the return code
	dlg.DoModal();

	// save settings (unless we had command line parameters)
	if (numOptions == 0) {
		saveSettings();
	} else {
		if (fVerbose) {
			debug(_T("Skipping saving settings due to command line parameters."));
		}
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}


// pRGB - input image - 256x192x24-bit image
// p8Bit - output image - 256x192x8-bit image, we will provide palette
// pal - palette to use (15-color TI palette, color 0 (transparent) not included, so 0-14 are valid
// many global flags override
void MYRGBTo8BitDithered(BYTE *pRGB, BYTE *p8Bit, MYRGBQUAD *inpal, double darken)
{
	// the difference in splitting up the functions seems negligable on my pc
	if ((g_UseHalfMulticolor)||(g_UseMulticolorOnly)) {
#ifdef ALLOWHAM4
		// HACK - TRY 'HAM4' mode
		g_UsePerLinePalette=true;		// avoid overdraw of final image
		ConvertToHAM4(pRGB, p8Bit, inpal);
		return;
#endif

		// updates p8Bit, also updates the palette as needed
		debug(_T("Applying half-multicolor search...\n"));
		ConvertToHalfMulticolor(pRGB, p8Bit, inpal);	// this is fast enough that we don't need to break it up
		if (g_UseMulticolorOnly) {
			return;
		}
	} else {
		// prepare palette (this will be overridden later if needed)
		for (int i=0; i<15; i++) {
			pal[i][0]=inpal[i].rgbRed;
			pal[i][1]=inpal[i].rgbGreen;
			pal[i][2]=inpal[i].rgbBlue;
			makeYUV(pal[i][0], pal[i][1], pal[i][2], YUVpal[i][0], YUVpal[i][1], YUVpal[i][2]);
		}
	}

	// this function handles it all
	AfxGetMainWnd()->EnableWindow(FALSE);
	if (g_UseHalfMulticolor) {
		if (g_Perceptual) {
			if (g_OrderedDither == 2) {
				quantize_new_halfmult_percept_ordered2(pRGB, p8Bit, darken, g_MapSize);
			} else if (g_OrderedDither) {
				quantize_new_halfmult_percept_ordered(pRGB, p8Bit, darken, g_MapSize);
			} else {
				quantize_new_halfmult_percept(pRGB, p8Bit, darken, g_MapSize);
			}
		} else {
			if (g_OrderedDither == 2) {
				quantize_new_halfmult_ordered2(pRGB, p8Bit, darken, g_MapSize);
			} else if (g_OrderedDither) {
				quantize_new_halfmult_ordered(pRGB, p8Bit, darken, g_MapSize);
			} else {
				quantize_new_halfmult(pRGB, p8Bit, darken, g_MapSize);
			}
		}
	} else {
		if (g_Perceptual) {
			if (g_OrderedDither == 2) {
				quantize_new_percept_ordered2(pRGB, p8Bit, darken, g_MapSize);
			} else if (g_OrderedDither) {
				quantize_new_percept_ordered(pRGB, p8Bit, darken, g_MapSize);
			} else {
				quantize_new_percept(pRGB, p8Bit, darken, g_MapSize);
			}
		} else {
			if (g_OrderedDither == 2) {
				quantize_new_ordered2(pRGB, p8Bit, darken, g_MapSize);
			} else if (g_OrderedDither) {
				quantize_new_ordered(pRGB, p8Bit, darken, g_MapSize);
			} else {
				quantize_new(pRGB, p8Bit, darken, g_MapSize);
			}
		}
	}
	AfxGetMainWnd()->EnableWindow(TRUE);
}

// must be 256x192!
void ConvertToHalfMulticolor(unsigned char *pOrig, BYTE *pDest, MYRGBQUAD *inpal) {
	// first, fix the palette. Cut all shades in half, and then create all possible shades (256 total)
	// we know that color 15 is invalid here, but we'll set it up anyway
	uchar tmppal[16][4];
	int Divisor=2;	// 2 for half multicolor, 1 for full multicolor
	int nRange=15;	// 15 for half- or normal multicolor, 256 for flickered multicolor

	for (UINT32 i=0;i<16;i++)
	{
		pal[i][0]=inpal[i].rgbRed;
		pal[i][1]=inpal[i].rgbGreen;
		pal[i][2]=inpal[i].rgbBlue;
		pal[i][3]=0;

		makeYUV(pal[i][0], pal[i][1], pal[i][2], YUVpal[i][0], YUVpal[i][1], YUVpal[i][2]);
	}

	if (g_UseHalfMulticolor) {
		// tweak palette - this assumes bitmap is up for 1/2 frames, and multicolor for 1/2 frames (or two multicolor frames)
		for (int idx=0; idx<16; idx++) {
			for (int j=0; j<4; j++) {
				tmppal[idx][j]=pal[idx][j];		// save off the full colors
			}
		}
		for (int i=0; i<16; i++) {
			for (int j=0; j<16; j++) {
				for (int k=0; k<4; k++) {
					pal[i*16+j][k] = (uchar)(((int)tmppal[i][k]+(int)tmppal[j][k])/2);	// mix them all into the palette
				}
				makeYUV(pal[i*16+j][0], pal[i*16+j][1], pal[i*16+j][2], YUVpal[i*16+j][0], YUVpal[i*16+j][1], YUVpal[i*16+j][2]);
			}
		}
		if (g_UseMulticolorOnly) {
			// flickered multicolor is both switches on
			nRange = 256;
			Divisor=1;
		}
	} else {
		Divisor=1;
	}
	// copy into winpal
	for (int idx=0; idx<256; idx++) {
		winpal[idx].rgbBlue = pal[idx][2];
		winpal[idx].rgbGreen = pal[idx][1];
		winpal[idx].rgbRed = pal[idx][0];
	}

	// we'll only choose the main color here, and that should give the bitmap size the best odds of finding
	// good matches. So our color here gets stored in the low nibble
	// Multicolor requires us to look at 4x4 pixel blocks at a time
	for (int y=0; y<192; y+=4) {
		for (int x=0; x<256; x+=4) {
			int nBestColor = -1;
			int nBestError = 0x7fffffff;

			// now calculate errors for each of the colors
			for (int col = 0; col<nRange; col++) {						// only search the main 15 colors, but at half the brightness
				unsigned char *pCol = &pal[col+(Divisor-1)*16][0];		// palette entry for this color (needed because we need the black mix, not white)
				int nThisErr = 0;										// (that is, half-multicolor evaluates against the true half-brightness colors)

				if (col>15) {
					// skip color 15 of each block, only 0-14 is valid
					if (col%16 == 15) continue;
					if (col/16 == 15) break;	// well, this one means we're done

					// also skip color matches that exceed the flicker palette difference
					uchar *pPal1=pal[(col%16<<4)|(col%16)];		// (color+itself is solid version)
					uchar *pPal2=pal[(col/16)<<4|(col/16)];
					double lum1, lum2, diff;
					makeY(pPal1[0], pPal1[1], pPal1[2], lum1);
					makeY(pPal2[0], pPal2[1], pPal2[2], lum2);
					if (lum1 > lum2) {
						diff = lum1 - lum2;
					} else {
						diff = lum2 - lum1;
					}
					// diff should be from 0-256
					if ((diff/256.0) > (double)g_MaxMultiDiff/100.0) continue;
				}

				for (int nSubY = 0; nSubY < 4; nSubY++) {
					for (int nSubX = 0; nSubX < 4; nSubX++) {
						unsigned char *pBase24 = pOrig + ((y+nSubY)*256+x+nSubX)*3;		// pixel of 24-bit image

						if (g_Perceptual) {
							int t = pCol[0]-(pBase24[0]/Divisor);	// match 1/2 the brightness
							nThisErr+=(int)((t*t)*(g_PercepR*100));
							t = pCol[1]-(pBase24[1]/Divisor);
							nThisErr+=(int)((t*t)*(g_PercepG*100));
							t = pCol[2]-(pBase24[2]/Divisor);
							nThisErr+=(int)((t*t)*(g_PercepB*100));
						} else {
							double r1,g1,b1;
							double r2,g2,b2;

							// get RGB
							r1=pCol[0];
							g1=pCol[1];
							b1=pCol[2];

							// get RGB
							r2=(pBase24[0]/Divisor);
							g2=(pBase24[1]/Divisor);
							b2=(pBase24[2]/Divisor);

							nThisErr += (int)yuvdist(r1,g1,b1,r2,g2,b2);
						}
					}
				}

				if (nThisErr < nBestError) {
					nBestError = nThisErr;
					nBestColor = col;
				}
			}

			// search done for this block, apply the best match
			for (int nSubY = 0; nSubY < 4; nSubY++) {
				for (int nSubX = 0; nSubX < 4; nSubX++) {
					unsigned char *pBase8 = pDest + (y+nSubY)*256+x+nSubX;	// pixel of 8-bit image
					*pBase8=nBestColor;	
				}
			}
		}

		// and draw it (we'll just draw the 4 whole rows)
		if (NULL != pWnd) {
			CDC *pCDC=pWnd->GetDC();
			if (NULL != pCDC) {
				int dpi = GetDpiForWindow(pWnd->GetSafeHwnd());
				IS40_StretchDraw8Bit(*pCDC, buf8+(y*256), 256, 4, 256, winpal, DPIFIX(XOFFSET), DPIFIX(y*2), DPIFIX(256*2), DPIFIX(8));
				pWnd->ReleaseDC(pCDC);
			}
		}
	}
}

#ifdef ALLOWHAM4

// 'pal' must be configured already for 4 colors
// pBase24 points to the 24-bit pixel to test
int HamPixMatch(unsigned char *pBase24) {
	int nBestError = 0x7fffffff;
	int nBestColor = 0;

	// now calculate errors for each of the colors
	for (int col = 0; col<4; col++) {							// only 4 colors to search
		unsigned char *pCol = &pal[col][0];						// palette entry for this color
		int nThisErr = 0;

		if (g_Perceptual) {
			int t = pCol[0]-pBase24[0];
			nThisErr+=(int)((t*t)*(g_PercepR*100));
			t = pCol[1]-pBase24[1];
			nThisErr+=(int)((t*t)*(g_PercepG*100));
			t = pCol[2]-pBase24[2];
			nThisErr+=(int)((t*t)*(g_PercepB*100));
		} else {
			int r,g,b;
			double y1,cr1,cb1;
			double y2,cr2,cb2;

			// get RGB
			r=pCol[0];
			g=pCol[1];
			b=pCol[2];

			// make YCrCb
			makeYUV(r, g, b, y1, cr1, cb1);

			// get RGB
			r=pBase24[0];
			g=pBase24[1];
			b=pBase24[2];

			// make YCrCb
			makeYUV(r, g, b, y2, cr2, cb2);

			// gets diffs
			nThisErr += (int)((y1-y2)*(y1-y2) + (cr1-cr2)*(cr1-cr2) + (cb1-cb2)*(cb1-cb2) + 0.5);
		}

		if (nThisErr < nBestError) {
			nBestError = nThisErr;
			nBestColor = col;
		}
	}

	return nBestColor;
}


// must be 256x192!
void ConvertToHAM4(unsigned char *pOrig, BYTE *pDest, MYRGBQUAD *inpal) {
	// this mode attempts a mode similar to Amiga's HAM. We set up the 4-color bitmap mode on
	// the F18A, and then every 4 pixels, we change one color gun in one color. The goal here
	// is to see if that can produce nice images anyway.
	//
	// The idea here is that the 4-color bitmap mode takes 12k (same as regular bitmap mode,
	// but with no pixel limitations and no need for a SDT). One byte of control for every
	// 4 pixels takes another 6k, which completely fills the 18k of the F18A. But, if the results
	// are worth it, we can strip a scanline or two to get the space we need for the GPU code.
	//
	
	// So, for now, just attempt a conversion and draw the result, don't worry about how we
	// will export the data for the TI.

	// TODO: no dithering either, as yet.
	int row,col;

	// create a 4-color palette
	int nFixedColors = 4;
	if (nFixedColors > 0) {
		// TODO: proably no point selecting the first 4 colors by the whole image,
		// we should do it just by the first 4 pixels (whole image does seem to work okay though...)
		if (g_bStaticByPopularity) {
			debug(_T("Preserving %d top colors (popularity)\n"), nFixedColors);

			int nColCount = 0;

			// if we are doing a per-line palette, fix the 4 most popular colors to reduce horizontal lines
			// rather than 'first' color, we should probably save 2-4 colors by popularity and
			// only change the rest. 
			// I tried keeping the 4 most DISTINCT colors per line, that was worse than this approach.
			memset(cols, 0, sizeof(cols));
			for (row=0; row<192; row++) {
				for (col=0; col<256; col++) {
					// address of byte in the RGB image (3 bytes per pixel)
					BYTE *pInLine = pOrig + (row*256*3) + (col*3);
					// make a 12-bit color, rounding colors up
					int idx= ((MakeRoundedRGB(*pInLine)&0xf0)<<4) |
								((MakeRoundedRGB(*(pInLine+1)))&0xf0) |
								((MakeRoundedRGB(*(pInLine+2))&0xf0)>>4);
					cols[idx]++;
				}
			}

			// now we've counted them, choose the top x
			int top[16], topcnt[16];
			for (int idx=0; idx<16; idx++) {
				top[idx]=0;
				topcnt[idx]=0;
			}
			for (int idx=0; idx<4096; idx++) {
				if (cols[idx] > 0) nColCount++;		// just some stats since we're here

				for (int idx2=0; idx2<16; idx2++) {
					if (cols[idx] > topcnt[idx2]) {
						top[idx2]=idx;
						topcnt[idx2]=cols[idx];
						break;
					}
				}
			}
			debug(_T("%d relevant colors detected (%d%%).\n"), nColCount, nColCount*100/4096);

			// should have (up to) 15 colors sorted now, grab the top x (popularity)
			for (int idx=0; idx<nFixedColors; idx++) {
				pal[idx+1][0]=(((top[idx]&0xf00)>>8)<<4)+8;		// +8 to center the color in the middle of the range (0x?0 - 0x?F)
				pal[idx+1][1]=(((top[idx]&0xf0)>>4)<<4)+8;
				pal[idx+1][2]=(((top[idx]&0xf))<<4)+8;
				makeYUV(pal[idx+1][0], pal[idx+1][1], pal[idx+1][2], YUVpal[idx+1][0], YUVpal[idx+1][1], YUVpal[idx+1][2]);
			}
		} else {
			debug(_T("Preserving %d top colors (median cut)\n"), nFixedColors);
			// need to make a work copy of the image
			// this uses median cut over the entire image down to the number of static colors
			unsigned char *pWork = (unsigned char*)malloc(256*192*3);
			memcpy(pWork, pOrig, 256*192*3);
			// to force 4 bit color guns in medianCut, reduce the image to 4 bits
			// otherwise we get duplicate colors. Since it's a work buffer, we don't
			// need to shift it back up, but we do need to shift up the colors
			unsigned char *pTmp = pWork;
			for (int idx=0; idx<256*192*3; idx++) {
				*(pTmp++)>>=4;
			}
			std::list<Point> newpal = medianCut((Point*)pWork, 256*192, nFixedColors);
			free(pWork);
			// copy the returned palette back as fixed colors
			std::list<Point>::iterator iter;
			int idx = 0;
			for (iter = newpal.begin() ; iter != newpal.end(); iter++) {
				pal[idx][0]=MakeRoundedRGB(iter->x[0]<<4);
				pal[idx][1]=MakeRoundedRGB(iter->x[1]<<4);
				pal[idx][2]=MakeRoundedRGB(iter->x[2]<<4);
				makeYUV(pal[idx][0], pal[idx][1], pal[idx][2], YUVpal[idx][0], YUVpal[idx][1], YUVpal[idx][2]);
				idx++;
			}
		}

        // TODO: this will try just the first four colors, since it will change a lot anyway
        debug(_T("Psyche! Taking the FIRST %d colors...\n"), nFixedColors);

        for (int idx=0; idx<nFixedColors; ++idx) {
            BYTE *pInLine = pOrig + (idx*3);
            pal[idx+1][0]=(MakeRoundedRGB(*pInLine)&0xf0);
            pal[idx+1][1]=(MakeRoundedRGB(*(pInLine+1)))&0xf0;
            pal[idx+1][2]=(MakeRoundedRGB(*(pInLine+2)))&0xf0;
            makeYUV(pal[idx+1][0], pal[idx+1][1], pal[idx+1][2], YUVpal[idx+1][0], YUVpal[idx+1][1], YUVpal[idx+1][2]);
        }
	}

	// now we run through each scanline, 4 pixels at a time, every four pixels we have to change a color
	for (int y=0; y<192; y++) {
		debug(_T("Scanning line %d\r"), y);
		for (int x=0; x<256; x+=4) {
			// copy into winpal
			for (int idx=0; idx<4; idx++) {
				winpal[idx].rgbBlue = pal[idx][2];
				winpal[idx].rgbGreen = pal[idx][1];
				winpal[idx].rgbRed = pal[idx][0];
			}

			for (int subx=0; subx<4; subx++) {
				// not much of a search needed, it's literally just best match per pixel
				int nBestColor = HamPixMatch(pOrig + (y*256+x+subx)*3);	// pixel of 24-bit image

				// search done for this pixel, apply the best match
				*(buf8+(y*256)+x+subx)=nBestColor;

				// TODO: we could speed this up by saving each pixel in the 256 color winpal, and
				// thus still draw a full 8-bit line at a time
				// it's slower, but we draw it too, so it's saved off
				if (NULL != pWnd) {
					CDC *pCDC=pWnd->GetDC();
					if (NULL != pCDC) {
						int dpi = GetDpiForWindow(pWnd->GetSafeHwnd());
						IS40_StretchDraw8Bit(*pCDC, buf8+(y*256)+x+subx, 1, 1, 256, winpal, DPIFIX(XOFFSET+(x+subx)*2), DPIFIX(y*2), DPIFIX(2), DPIFIX(2));
						pWnd->ReleaseDC(pCDC);
					}
				}
			}

			// search done for this block of four - select a new color
			// Basically... this is where the brute force search happens.
			// The control word has 256 possibilities, and then we need to try a best match
			// against the next 4 pixels. Whichever word gets the smallest error on those four
			// pixels is the one we want to use.
			// naive, but it should tell me if this concept has any merit
			uchar palback[256][4];						// only 4 colors needed
			double yuvback[256][4];

			memcpy(palback, pal, sizeof(palback));		// backup the palette
			memcpy(yuvback, YUVpal, sizeof(yuvback));

			// calculate a best-match error for the next four pixels
			double nBestErr = 0xffffffff;
			int nBestCtrl = 0;

			for (int ctrlword = 0; ctrlword < 256; ctrlword++) {
				// restore palette
				memcpy(pal, palback, sizeof(palback));	// always use palback's size!
				memcpy(YUVpal, yuvback, sizeof(yuvback));

				// parse the control word: xxccdddd
				//	xx - which bits? (0 = all, 1 = red, 2 = green, 3 = blue)
				//  cc - which color? (0-3)
				//  dddd - data for the color gun(s)
				int cc = (ctrlword&0x30)>>4;
				int gun = ((ctrlword&0x0f)<<4) + 8;
				switch (ctrlword&0xc0) {
				case 0x00:	// grey
					pal[cc][0] = gun;
					pal[cc][1] = gun;
					pal[cc][2] = gun;
					break;

				case 0x40:	// red
					pal[cc][0] = gun;
					break;

				case 0x80:	// green
					pal[cc][1] = gun;
					break;

				case 0xc0:	// blue
					pal[cc][2] = gun;
					break;
				}
				// recalculate YUV palette entry
				makeYUV(pal[cc][0], pal[cc][1], pal[cc][2], YUVpal[cc][0], YUVpal[cc][1], YUVpal[cc][2]);

				// calculate position
				int nextx=x+4;
				int nexty=y;
				if (nextx>=256) { nextx=0; ++nexty; }
				if ( nexty < 192) {
					double nCurDistance = 0.0;

					for (int subx=0; subx<4; subx++) {
						uchar *pData = pOrig + (nexty*256+nextx+subx)*3;
						int nCol = HamPixMatch(pData);

						double r,g,b;
						double y,cr,cb;
						double t1, t2, t3;

						// get RGB
						r=pData[0];
						g=pData[1];
						b=pData[2];
						// make YCrCb
						makeYUV(r, g, b, y, cr, cb);
						// gets diffs
						t1=y-YUVpal[nCol][0];
						t2=cr-YUVpal[nCol][1];
						t3=cb-YUVpal[nCol][2];
						nCurDistance+=(t1*t1)+(t2*t2)+(t3*t3);
					}
					if (nCurDistance < nBestErr) {
						nBestErr = nCurDistance;
						nBestCtrl = ctrlword;
					}
				}

			}

			// done the search, apply the best one
			// restore palette
			memcpy(pal, palback, sizeof(palback));	// always use palback's size!
			memcpy(YUVpal, yuvback, sizeof(yuvback));

#if 1
			// parse the control word: xxccdddd
			//	xx - which bits? (0 = all, 1 = red, 2 = green, 3 = blue)
			//  cc - which color? (0-3)
			//  dddd - data for the color gun(s)
			int cc = (nBestCtrl&0x30)>>4;
			int gun = ((nBestCtrl&0x0f)<<4) + 8;
			switch (nBestCtrl&0xc0) {
			case 0x00:	// grey
				pal[cc][0] = gun;
				pal[cc][1] = gun;
				pal[cc][2] = gun;
				break;

			case 0x40:	// red
				pal[cc][0] = gun;
				break;

			case 0x80:	// green
				pal[cc][1] = gun;
				break;

			case 0xc0:	// blue
				pal[cc][2] = gun;
				break;
			}
			// recalculate YUV palette entry
			makeYUV(pal[cc][0], pal[cc][1], pal[cc][2], YUVpal[cc][0], YUVpal[cc][1], YUVpal[cc][2]);
#endif

		}	// next block of 4 x
	}	// next y
	debug(_T("\n"));
}

#endif // ALLOWHAM4
