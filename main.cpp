// Convert an image to a TI compatible image
// 

#define _WIN32_IE 0x0400

#include "stdafx.h"
#include <windows.h>
#include <wininet.h>
#include <shlobj.h>
#include <time.h>
#include <stdio.h>
#include <crtdbg.h>

#include "tipicview.h"
#include "D:\WORK\imgsource\4.0\islibs40_vs05\ISource.h"
#include "2passscale.h"
#include "TIPicViewDlg.h"

bool StretchHist;
void MYRGBTo8BitDithered(BYTE *pRGB, BYTE *p8Bit, MYRGBQUAD *pal, double dark);

#define MAXFILES 200000

int pixeloffset;
int heightoffset;
char szFiles[MAXFILES][256];
char *pTmp;
int iCnt, n, ret, errCount;
unsigned int idx1, idx2;
int ScaleMode;
char szFolder[256];
char szBuf[256];
unsigned int iWidth, iHeight;
unsigned int inWidth, inHeight;
unsigned int outWidth, outHeight;
unsigned int finalW, finalH;
int currentx;
int currenty;
int currentw;
int currenth;
int maxerrorcount;
HISSRC hSource, hDest;
HGLOBAL hBuffer, hBuffer2;
unsigned char buf8[256*192];
RGBQUAD winpal[256];
bool fFirstLoad=false;
bool fRand;
bool fVerbose = false;
extern int g_orderSlide;
extern int g_nFilter;
extern int g_nPortraitMode;
extern int g_UsePerLinePalette;
extern int g_UseColorOnly;
extern int g_MatchColors;
extern float_precision g_PercepR, g_PercepG, g_PercepB;
extern float_precision g_LumaEmphasis;
extern float_precision g_Gamma;
extern int g_Perceptual;
extern int g_UsePalette;
extern int g_UseHalfMulticolor;
extern int g_MaxMultiDiff;
extern int g_MaxColDiff;
extern int g_OrderedDither;
extern int g_MapSize;
extern char *cmdFileIn;		// from command line, makes a non-GUI mode
extern char *cmdFileOut;
extern HGLOBAL load_gif(char *filename, unsigned int *iWidth, unsigned int *iHeight);
MYRGBQUAD pal[256];

extern LPVOID pSharedPointer;
extern char szLastFilename[256];	// used to make sure we don't reload our own shared file

int instr(unsigned short *, char*);
bool ScalePic(int nFilter, int nPortraitMode);
void BuildFileList(char *szFolder);
BOOL ResizeRGBBiCubic(BYTE *pImgSrc, UINT32 uSrcWidthPix, UINT32 uSrcHeight, BYTE *pImgDest, UINT32 uDestWidthPix, UINT32 uDestHeight);

// BIG NOTE! WHITE IS INSERTED AS COLOR 0, and TRANSPARENT IS NOT PRESENT
// Furthermore, GREY is color 2! So, White, Black, Grey. 
// This needs to be corrected when outputting the data.
// So, 0=white,and the rest match the TI colors (white should be 15)
MYRGBQUAD palinit16[256] = {
#if 0
	// RacconLad's MSX palette from his flash games
	226,226,226,0,
	0,0,0,0,
	161,161,161,0,
	32,194,32,0,
	97,226,97,0,
	32,32,226,0,
	64,97,226,0,
	161,32,32,0,
	64,194,226,0,
	226,32,32,0,
	226,97,97,0,
	194,194,32,0,
	194,194,129,0,
	32,129,32,0,
	194,64,161,0,	

#elif 0
	// calculated palette from datasheet
	255,255,255,0,
	0,0,0,0,
	204,204,204,0,
	71,183,59,0,
	124,207,111,0,
	93,78,255,0,
	128,114,255,0,
	182,98,71,0,
	93,200,237,0,
	215,107,72,0,
	251,143,108,0,
	195,205,65,0,
	211,218,118,0,
	62,159,47,0,
	182,100,199,0,
#elif 0
	// warmer palette measured from adam by http://junior.apk.net/~drushel/pub/coleco/twwmca/wk961201.html
	// I have reason to think that the TI's palette is even darker. The output stages would
	// change the video slightly, so the raw datasheet values are not necessarily right.
	191,207,175,0,
	0,16,0,0,
	159,160,143,0,
	16,191,0,0,
	64,192,32,0,
	64,63,255,0,
	96,95,255,0,
	160,80,16,0,
	47,192,208,0,
	207,79,15,0,
	224,111,32,0,
	160,175,0,0,
	175,175,31,0,
	16,144,0,0,
	159,79,160,0,
#elif 0
	// Classic99 palette - scaled to fill 0-255 fully	3% difference from unscaled
	255, 255, 255, 0,
	0, 0, 0, 0,
	206, 206, 206, 0,
	33, 206, 66, 0,
	90, 222, 123, 0,
	82, 82, 239, 0,
	123, 115, 255, 0,
	214, 82, 74, 0,
	66, 239, 247, 0,
	255, 82, 82, 0,
	255, 123, 123, 0,
	214, 198, 82, 0,
	231, 206, 132, 0,
	33, 181, 57, 0,
	206, 90, 189, 0,
#else
	// classic99 palette				<-------- this one, normally
	248, 248, 248, 0,
	0, 0, 0, 0,
	200, 200, 200, 0,
	32, 200, 64, 0,
	88, 216, 120, 0,
	80, 80, 232, 0,
	120, 112, 248, 0,
	208, 80, 72, 0,
	64, 232, 240, 0,
	248, 80, 80, 0,
	248, 120, 120, 0,
	208, 192, 80, 0,
	224, 200, 128, 0,
	32, 176, 56, 0,
	200, 88, 184, 0,
#endif
};
// remembers the default palette for the settings file
MYRGBQUAD defaultpalinit16[16];

// ordered dither threshold maps
// 3 and 8 didn't look very good due to color clash
float_precision g_thresholdMap2x2[2][2] = {
	0, 2,
	3, 1
};
float_precision g_thresholdMap4x4[4][4] = {
	 0,	 8,	 2,	10,
	12,	 4,	14,	 6,
	 3,	11,	 1,	 9,
	15,	 7,	13,	 5
};

unsigned char orig8[256*192];
CWnd *pWnd;

// simple wrapper for debug - command line mode is normally quiet unless verbose is set
void debug(char *s, ...) {
	if ((cmdFileIn == NULL) || (fVerbose)) {
		char buf[1024];

		_vsnprintf(buf, 1023, s, (char*)((&s)+1));
		buf[1023]='\0';

		OutputDebugString(buf);
		printf("%s",buf);
	}
}

// read a TI Artist into a 24-bit color buffer :)
// use palinit16[] about (remember only 15 colors there)
// We know the filename must end with .TIA[P|C] or _[P|C]
// TODO: can't load half-multicolor today...
// TODO: when loading scanline palette images, the re-converted image shifts.
// This means we aren't handling the palette right during conversion, probably.
HGLOBAL ReadTIArtist(CString csFile) {
	HGLOBAL hGlob=NULL;
	unsigned char identifier[128];
	unsigned char *pBuf, *pOut, *p8Out;
	bool bHeader=true;
	bool bRLE=false;

	// first, check if it really is a TI Artist file
	// make sure we are trying for the _P file first
	if (csFile.Right(1).MakeUpper() == 'M') {
		csFile=csFile.Left(csFile.GetLength()-1);
		csFile+='P';
	}
	if (csFile.Right(1).MakeUpper() == 'C') {
		csFile=csFile.Left(csFile.GetLength()-1);
		csFile+='P';
	}

	FILE *fp=NULL;
	fopen_s(&fp, csFile, "rb");
	if (NULL == fp) {
		printf("Can't open %s\n", (LPCSTR)csFile);
		return NULL;
	}

	memset(identifier, 0, 128);
	fread(identifier, 1, 128, fp);

	// check header
	if ((identifier[0]==7) && 
		(identifier[1]=='T') && 
		(identifier[2]=='I') && 
		(identifier[3]=='F') && 
		(identifier[4]=='I') && 
		(identifier[5]=='L') && 
		(identifier[6]=='E') && 
		(identifier[7]=='S') &&		// tag
		(identifier[10]==1) &&		// PROGRAM
		(identifier[8]==0) &&		// size in sectors (6144 byte file)
		(identifier[9]==0x18)) {
			goto test2;				// just to indicate okay, we don't read the header again after this
	};

	// v9t9 detection - kind of weak...
	if ((isprint(identifier[0])) && 
		(isprint(identifier[1]) || isspace(identifier[1])) &&
		(isprint(identifier[2]) || isspace(identifier[2])) &&
		(isprint(identifier[3]) || isspace(identifier[3])) &&
		(isprint(identifier[4]) || isspace(identifier[4])) &&
		(isprint(identifier[5]) || isspace(identifier[5])) &&
		(isprint(identifier[6]) || isspace(identifier[6])) &&
		(isprint(identifier[7]) || isspace(identifier[7])) &&
		(isprint(identifier[8]) || isspace(identifier[8])) &&
		(isprint(identifier[9]) || isspace(identifier[9])) &&	// filename
		(identifier[12]==1) &&		// PROGRAM
		(identifier[14]==0) &&		// size in sectors (6144 byte file)
		(identifier[15]==0x18)) {
			goto test2;				// just to indicate okay, we don't read the header again after this
	}

	// not a valid header - hold control to assume it's a raw file
	if (0 == (GetAsyncKeyState(VK_CONTROL) & 0x8000)) {
		printf("No TIFILES or V9T9 header - hold CONTROL to load a RAW file\n");
		fclose(fp);
		return NULL;
	}
	// else, fseek back since the header doesn't exist
	printf("Holding CONTROL overrides lack of header.\n");
	fseek(fp, 0, SEEK_SET);
	bHeader=false;

test2:
	pBuf=(unsigned char*)malloc(6144*3);
	if (NULL == pBuf) {
		printf("Can't allocate temporary buffer\n");
		fclose(fp);
		return NULL;
	}

	// allow for max size of a scanline-palette image
	memset(pBuf, 0, 6144*3);

	if (GetAsyncKeyState(VK_MENU) & 0x8000) {
		printf("TI Artist file - Holding ALT assumes RLE encoding.\n");
		bRLE=true;
		// read as RLE
		int p=0;
		while ((!feof(fp)) && (p < 6144)) {
			int x=fgetc(fp);
			if (x&0x80) {
				int y=fgetc(fp);
				x&=0x7f;
				while ((p<6144)&&((x--)>0)) {
					pBuf[p++]=y;
				}
			} else {
				while ((p<6144)&&((x--)>0)) {
					pBuf[p++]=fgetc(fp);
				}
			}
		}
	} else {
		printf("TI Artist file - hold ALT to assume RLE file\n");
		// just read
		fread(pBuf, 1, 6144, fp);
	}

	fclose(fp);

	csFile=csFile.Left(csFile.GetLength()-1);
	csFile+='C';

	fp=NULL;
	fopen_s(&fp, csFile, "rb");
	if (NULL == fp) {
		printf("Can't open %s (mono file assumed)\n", (LPCSTR)csFile);
		memset(pBuf+6144, 0x1f, 6144);
	} else {
		// just assume the _c is the same as the _p
		if (bHeader) {
			fread(identifier, 1, 128, fp);
		}

		if (bRLE) {
			// read as RLE
			int p=0;
			while ((!feof(fp)) && (p < 6144)) {
				int x=fgetc(fp);
				if (x&0x80) {
					int y=fgetc(fp);
					x&=0x7f;
					while ((p<6144)&&((x--)>0)) {
						pBuf[(p++)+6144]=y;
					}
				} else {
					while ((p<6144)&&((x--)>0)) {
						pBuf[(p++)+6144]=fgetc(fp);
					}
				}
			}
		} else {
			// just read
			fread(pBuf+6144, 1, 6144, fp);
		}

		fclose(fp);
	}

	// check for palette file
	int palSize = 0;
	csFile=csFile.Left(csFile.GetLength()-1);
	csFile+='M';

	fp=NULL;
	fopen_s(&fp, csFile, "rb");
	if (NULL == fp) {
		printf("Can't open %s (non-extended file assumed)\n", (LPCSTR)csFile);
		memset(pBuf+6144*2, 0, 6144);
	} else {
		// just assume the _c is the same as the _p
		if (bHeader) {
			fread(identifier, 1, 128, fp);
		}

//		if (bRLE) {
//			// this is probably a bug, but at the moment palette files
//			// are not compressed, even the big ones! ;)
//		}

		// just read - palette size indicates the type of image
		// 6144 - scanline palette	- 192 lines of 16 palette entries, each entry 16 bits (12 bits padded)
		// 2048 - half multicolor	- staggered multicolor PDT, see docs
		// 32   - paletted bitmap	- 16 palette entries, each 16 bits (12 bits padded)
		palSize = fread(pBuf+6144*2, 1, 6144, fp);

		fclose(fp);
	}

	// not supporting half-multicolor atm, that's a bigger parse)
	if ((palSize != 0) && (palSize != 6144) && (palSize != 32)) {
		printf("Unsupported palette type - ignoring\n");
		palSize = 0;
	}

	// now parse it up to 24 bit
	hGlob = GlobalAlloc(GPTR, 256*192*3);
	if (NULL == hGlob) {
		free(pBuf);
		return NULL;
	}

	pOut=(unsigned char*)hGlob;				// gives us the 24-bit source
	p8Out=buf8;								// and just copy the original into here too (TODO: might not work for palettes...)
	for (int row=0; row<24; row++) {
		for (int line=0; line<8; line++) {
			for (int col=0; col<32; col++) {
				int offset=(row*32+col)*8+line;
				if (palSize == 0) {
					int fg=((*(pBuf+6144+offset))>>4)&0x0f;
					// remap white and grey from TI colors to internal colors
					if (fg == 15) {
						fg=0; 
					} else if (fg == 14) {
						fg=2;
					} else if (fg > 1) {
						fg++;
					}
					int bg=((*(pBuf+6144+offset)))&0x0f;
					// remap white and grey from TI colors to internal colors
					if (bg == 15) {
						bg=0; 
					} else if (bg == 14) {
						bg=2;
					} else if (bg > 1) {
						bg++;
					}
					int pix=*(pBuf+offset);
					// now we have 8 pixels
					int bit=0x80;
					for (int cnt=0; cnt<8; cnt++) {
						if (pix&bit) {
							*(pOut++)=palinit16[fg].rgbRed;
							*(pOut++)=palinit16[fg].rgbGreen;
							*(pOut++)=palinit16[fg].rgbBlue;
							*(p8Out++)=fg;
						} else {
							*(pOut++)=palinit16[bg].rgbRed;
							*(pOut++)=palinit16[bg].rgbGreen;
							*(pOut++)=palinit16[bg].rgbBlue;
							*(p8Out++)=bg;
						}
						bit>>=1;
					}
				} else {
					// almost the same, but with custom colors
					MYRGBQUAD FG;
					MYRGBQUAD BG;

					int fg=((*(pBuf+6144+offset))>>4)&0x0f;
					int bg=((*(pBuf+6144+offset)))&0x0f;
					int rowoff = palSize == 32 ? 0 : ((row*8)+line)*32;

					FG.rgbRed = ((*(pBuf+6144*2+fg*2+rowoff))&0x0f)<<4;
					FG.rgbGreen = ((*(pBuf+6144*2+fg*2+rowoff+1))&0xf0);
					FG.rgbBlue = ((*(pBuf+6144*2+fg*2+rowoff+1))&0x0f)<<4;
					BG.rgbRed = ((*(pBuf+6144*2+bg*2+rowoff))&0x0f)<<4;
					BG.rgbGreen = ((*(pBuf+6144*2+bg*2+rowoff+1))&0xf0);
					BG.rgbBlue = ((*(pBuf+6144*2+bg*2+rowoff+1))&0x0f)<<4;

					int pix=*(pBuf+offset);
					// now we have 8 pixels
					int bit=0x80;
					for (int cnt=0; cnt<8; cnt++) {
						if (pix&bit) {
							*(pOut++)=FG.rgbRed;
							*(pOut++)=FG.rgbGreen;
							*(pOut++)=FG.rgbBlue;
							*(p8Out++)=fg;
						} else {
							*(pOut++)=BG.rgbRed;
							*(pOut++)=BG.rgbGreen;
							*(pOut++)=BG.rgbBlue;
							*(p8Out++)=bg;
						}
						bit>>=1;
					}
				}
			}
		}
	}

	free(pBuf);

	if (palSize > 0) {
		memcpy(buf8, "NOT-TI-PIC", 10);
	}

	return hGlob;
}

// handles invalid arguments to strcpy_s and strcat_s, etc
void app_handler(const wchar_t * expression,
                 const wchar_t * function, 
                 const wchar_t * file, 
                 unsigned int line,
                 uintptr_t pReserved) {
   printf("Warning: path or string too long. Skipping.\n");
}


// Mode 0 - pass in a path, random image from that path
// Mode 1 - reload image - if pFile is NULL, use last from list, else use passed file
// Mode 2 - load specific image and remember it
// 'dark' is a value from 0-16 and is the amount to darken for ordered dither
void maincode(int mode, char *pFile, double dark)
{
	// initialize
	bool fArgsOK=true;
	static bool fHaveFiles=false;
	static bool initialized=false;
	char szFileName[256];
	static int nlastpick=-1;
	int noldlast;
	char szOldFN[256];

	if (!initialized) {
		IS40_Initialize("{887916EA-FAE3-12E2-19C1-8B0FC40F045F}");	// please do not reuse this key in other applications
		srand((unsigned)time(NULL));
		initialized=true;
		pWnd=AfxGetMainWnd();

		// I build this on the assumption that the _s string functions would just
		// truncate and move on on long strings. that's untrue - they assert and fatal.
		// We can override that for now like this:
		_CrtSetReportMode(_CRT_ASSERT, 0);
		_set_invalid_parameter_handler(app_handler);
		// TODO: I really have paths this long. I should fix this.

		// update threshold maps - they're supposed to be fractional
		for (int i1=0; i1<2; i1++) {
			for (int i2=0; i2<2; i2++) {
				g_thresholdMap2x2[i1][i2] /= (float_precision)(4.0);
			}
		}
		for (int i1=0; i1<4; i1++) {
			for (int i2=0; i2<4; i2++) {
				g_thresholdMap4x4[i1][i2] /= (float_precision)(16.0);
			}
		}
	}

	if (mode == 0) {
		// Set defaults
		strcpy_s(szFolder, 256, pFile);
	}

	iWidth=256;
	iHeight=192;

	fRand=false;
	
	maxerrorcount=6;

	if (mode == 0) {
		if ((!fHaveFiles)&&(NULL != pFile)) {
			// get list of files
			iCnt=0;
			BuildFileList(szFolder);

			printf("\n%d files found.\n", iCnt);

			if (0==iCnt) {
				printf("That's an error\n");
				AfxMessageBox("The 'rnd' function currently only works if you have a c:\\pics folder.");
				return;
			}

			fHaveFiles=true;
		}
	}

	// Used to fit the image into the remaining space
	currentx=0;
	currenty=0;
	currentw=iWidth;
	currenth=iHeight;
	hBuffer2=NULL;
	noldlast=nlastpick;
	if (noldlast != -1) {
		strcpy_s(szOldFN, 256, szFiles[nlastpick]);
	}


	bool bIsTIArtist=false;
	{
		int cntdown;

		errCount=0;
		ScaleMode=-1;

		// randomly choose one
	tryagain:
		bIsTIArtist=false;
		hBuffer = NULL;

		if (mode & 0x1000) {
			// this means to generate a solid 12-bit color
			hBuffer = GlobalAlloc(GPTR, 256*192*3);
			if (NULL == hBuffer) {
				printf("Failed to allocate memory for solid image.\n");
				return;
			}
			for (int idx = 0; idx< 256*192*3; idx+=3) {
				*((unsigned char*)hBuffer+idx) = ((mode&0xf00)>>8)|((mode&0xf00)>>4);
				*((unsigned char*)hBuffer+idx+1) = ((mode&0xf0)>>4)|((mode&0xf0));
				*((unsigned char*)hBuffer+idx+2) = ((mode&0xf)<<4)|((mode&0xf));
			}
			n=MAXFILES-1;
			sprintf_s(szFiles[n], "RGB_0x%04X", mode);
			inWidth=256;
			inHeight=192;
		} else if ((1 == mode) || (2 == mode)) {
			if ((pFile == NULL) && (-1 != nlastpick)) {
				n=nlastpick;
			} else {
				if (pFile == NULL) {
					return;
				}
				n=MAXFILES-1;
				strcpy_s(szFiles[n], 256, pFile);
			}
		} else {
			if (nlastpick!=-1) {
				strcpy_s(szFiles[nlastpick],256, "");		// so we don't choose it again
			}
			pixeloffset=0;
			heightoffset=0;
			errCount++;
			if (errCount>6) {
				printf("Too many errors - stopping.\n");
				if (noldlast != -1) {
					strcpy_s(szFiles[noldlast], 256, szOldFN);
					nlastpick=noldlast;
				}
				return;
			}

			n=((rand()<<16)|rand())%iCnt;
			cntdown=iCnt;
			while (strlen(szFiles[n]) == 0) {
				n++;
				if (n>=iCnt) n=0;
				cntdown--;
				if (cntdown == 0) {
					printf("Ran out of images in the list, aborting.\n");
					if (noldlast != -1) {
						strcpy_s(szFiles[noldlast], 256, szOldFN);
						nlastpick=noldlast;
					}
					return;
				}
			}
		}

		printf("Chose #%d: %s\n", n, &szFiles[n][0]);
		strcpy_s(szFileName, 256, szFiles[n]);
		nlastpick=n;

		if (NULL != hBuffer) {
			goto ohJustSkipTheLoad;
		}

		// dump the data into the shared memory, if available (and not from there)
		strncpy(szLastFilename, szFileName, 255);		// save the name locally first so we can compare
		if ((pSharedPointer)&&((mode == 0)||(mode==2))) {
			// we copy the first byte last to ensure it's atomic
			memcpy((char*)pSharedPointer+1, szFileName+1, 255);
			InterlockedExchange((volatile LONG*)pSharedPointer, *((LONG*)szFileName));	// actually writes 32-bits, but guaranteed execution across cores
		}

		// open the file
		hSource=IS40_OpenFileSource(szFileName);
		if (NULL==hSource) {
			printf("Can't open image file.\n");
			return;
		}

		// guess filetype
		ret=IS40_GuessFileType(hSource);
		IS40_Seek(hSource, 0, 0);

		switch (ret)
		{
		case 1: //bmp
			hBuffer=IS40_ReadBMP(hSource, &inWidth, &inHeight, 24, NULL, 0);
			break;

		case 2: //gif
			IS40_CloseSource(hSource);
			hSource=NULL;
			hBuffer=load_gif(szFileName, &inWidth, &inHeight);
			break;

		case 3: //jpg
			hBuffer=IS40_ReadJPG(hSource, &inWidth, &inHeight, 24, 0);
			break;

		case 4: //png
			hBuffer=IS40_ReadPNG(hSource, &inWidth, &inHeight, 24, NULL, 0);
			break;

		case 5: //pcx
			hBuffer=IS40_ReadPCX(hSource, &inWidth, &inHeight, 24, NULL, 0);
			break;

		case 6: //tif
			hBuffer=IS40_ReadTIFF(hSource, &inWidth, &inHeight, 24, NULL, 0, 0);
			break;

		default:
			hBuffer=NULL;
			// is it TI Artist?
			{
				CString csTmp=szFileName;
				if ((csTmp.Right(5).CompareNoCase(".TIAP") == 0) ||
					(csTmp.Right(2).CompareNoCase("_P") == 0) ||
					(csTmp.Right(5).CompareNoCase(".TIAC") == 0) ||
					(csTmp.Right(2).CompareNoCase("_C") == 0) ||
					(csTmp.Right(5).CompareNoCase(".TIAM") == 0) ||
					(csTmp.Right(2).CompareNoCase("_M") == 0)) {
						// might be! try it!
						IS40_CloseSource(hSource);
						hSource=NULL;
						hBuffer=ReadTIArtist(csTmp);
						inWidth=256;
						inHeight=192;
				}
			}

			if (NULL == hBuffer) {
				// not something supported, then
				printf("Unable to indentify file (corrupt?)\n-> %s <-\n", szFileName);
				IS40_CloseSource(hSource);
				return;
			} else {
				// use this to shortcut a bit below
				if (memcmp(buf8, "NOT-TI-PIC", 10)) {
					// buffer gets that string set if there was a palette file,
					// indicating we need to reparse the image.
					bIsTIArtist=true;
				}
			}
		}

		if (NULL != hSource) {
			IS40_CloseSource(hSource);
		}

		if (NULL == hBuffer) {
			printf("Failed reading image file. (%d)\n", IS40_GetLastError());
			GlobalFree(hBuffer);
			if (mode == 1) goto tryagain; else return;
		}

ohJustSkipTheLoad:

		// if needed, convert to greyscale (using the default formula)
		if (g_MatchColors < 4) {
			unsigned char *pBuf = (unsigned char*)hBuffer;
			for (unsigned int idx=0; idx<inWidth*inHeight*3; idx+=3) {
				int r,g,b,x;
				r=pBuf[idx];
				g=pBuf[idx+1];
				b=pBuf[idx+2];
				if (g_Perceptual) {
					x=(int)(((float_precision)r*g_PercepR) + ((float_precision)g*g_PercepG) + ((float_precision)b*g_PercepB) + 0.5);
				} else {
					float_precision y;
					makeY(r, g, b, y);
					x=(int)y;
				}
				if (x>255) x=255;
				if (x<0) x=0;
				pBuf[idx]=x;
				pBuf[idx+1]=x;
				pBuf[idx+2]=x;
			}			
		}

#if 0
		// adapt to lower bitdepth to reduce noise (by throwing away data)
		// try 16 bit (565) for now... always an RGB operation
		// doesn't help much
		{
			unsigned char *pBuf = (unsigned char*)hBuffer;
			for (unsigned int idx=0; idx<inWidth*inHeight*3; idx+=3) {
				int r,g,b;
				r=pBuf[idx];
				g=pBuf[idx+1];
				b=pBuf[idx+2];
				r=r&0xf8;
				g=g&0xfc;
				b=b&0xf8;
				pBuf[idx]=r;
				pBuf[idx+1]=g;
				pBuf[idx+2]=b;
			}
		}
#endif

		// scale the image (this is okay for TI Artist - nop!)
		// Color reduce the image to the TI palette (this part does the most damage ;) )
		memcpy(pal, palinit16, sizeof(pal));
		for (int idx=0; idx<256; idx++) {
			// reorder for the windows draw
			winpal[idx].rgbBlue=pal[idx].rgbBlue;
			winpal[idx].rgbRed=pal[idx].rgbRed;
			winpal[idx].rgbGreen=pal[idx].rgbGreen;
		}

		// cache these values from ScalePic for the filename code
		int origx, origy, origw, origh;
		// no, not very clean code ;) And I started out so well ;)
		origx=currentx;
		origy=currenty;
		origw=currentw;
		origh=currenth;
		ScaleMode=-1;

		if (!ScalePic(g_nFilter, g_nPortraitMode)) {			// from hBuffer to hBuffer2
			// failed due to scale 
			if (mode == 1) goto tryagain; else return;
		}

		GlobalFree(hBuffer);

		// flag that we actually used the current setting
		((CTIPicViewDlg*)pWnd)->MakeConversionModeValid();
	}

	// if TI Artist, it's already also loaded into buf8, so we should be good!
	if (!bIsTIArtist) {
		// don't color shift if it's a work color
		if ((mode&0x1000) == 0) {
			// stretch histogram
			if (StretchHist) {
				printf("Equalize histogram...\n");
				if (!IS40_BrightnessHistogramEqualizeImage((unsigned char*)hBuffer2, iWidth, iHeight, 3, iWidth*3, 32, 224, 0)) {
					printf("Failed to equalize image, code %d\n", IS40_GetLastError());
				}
			}

			// apply gamma
			if ((g_Gamma != 1.0)&&(g_Gamma != 0.0)) {
				// http://www.dfstudios.co.uk/articles/programming/image-programming-algorithms/image-processing-algorithms-part-6-gamma-correction/
				double correct = 1/g_Gamma;
				unsigned char *pWork = (unsigned char*)hBuffer2;
				for (int idx=0; idx<256*192*3; idx++) {
					// every pixel is handled the same, so we just walk through it all at once
					int pix = *(pWork+idx);
					double newpix = pow(((double)pix/255), correct);
					*(pWork+idx) = (unsigned char)(newpix*255);
				}
			}

			// color shift if not using palette
			if ((!g_UsePalette) && (!g_UseHalfMulticolor)) {
				if (g_MaxColDiff > 0) {
					// diff is 0-100% for how far we can shift towards a valid color
					// max distance in color space is SQRT(255^2+255^2+255^2) = 441.673 units (so the 255 is kind of arbitrary)
					const float_precision maxrange = ((255*255)+(255*255)+(255*255))*(((float_precision)g_MaxColDiff/100.0)*((float_precision)g_MaxColDiff/100.0));		// scaled distance squared
					unsigned char *pWork = (unsigned char*)hBuffer2;
					for (int idx=0; idx<256*192; idx++) {
						int r = *(pWork);
						int g = *(pWork+1);
						int b = *(pWork+2);
						float_precision mindist = ((255*255)+(255*255)+(255*255));
						int best = 0;
						for (int i2=0; i2<g_MatchColors; i2++) {
							int rd = r-pal[i2].rgbRed;
							int gd = g-pal[i2].rgbGreen;
							int bd = b-pal[i2].rgbBlue;
							float_precision dist = (rd*rd)+(gd*gd)+(bd*bd);
							if (dist < mindist) {
								mindist = dist;
								best = i2;
							}
						}
						// we have a distance and a best color
						if (mindist <= maxrange) {
							// it's in range, just directly map it
							r = pal[best].rgbRed;
							g = pal[best].rgbGreen;
							b = pal[best].rgbBlue;
						} else {
							// not in range, so calculate a scale to move by
							float_precision scale = maxrange / mindist;	// x% ^2
							scale = sqrt((double)scale);				// x% raw
							float_precision move = (pal[best].rgbRed - r) * scale;
							if (move < 0) {
								r += (int)(move - 0.5);
							} else {
								r += (int)(move + 0.5);
							}
							move = (pal[best].rgbGreen - g) * scale;
							if (move < 0) {
								g += (int)(move - 0.5);
							} else {
								g += (int)(move + 0.5);
							}
							move = (pal[best].rgbBlue - b) * scale;
							if (move < 0) {
								b += (int)(move - 0.5);
							} else {
								b += (int)(move + 0.5);
							}
						}
						*(pWork) = r;
						*(pWork+1) = g;
						*(pWork+2) = b;
						pWork+=3;
					}
				}
			}
		}

		// original after adjustments
		{
			if (NULL != pWnd) {
				if (cmdFileIn == NULL) {
					printf("Drawing source image...\n");
				}
				pWnd->SetWindowText(szFileName);
				CDC *pCDC=pWnd->GetDC();
				if (NULL != pCDC) {
					int dpi = GetDpiForWindow(pWnd->GetSafeHwnd());
					if (!IS40_StretchDrawRGB(*pCDC, (unsigned char*)hBuffer2, iWidth, iHeight, iWidth*3, DPIFIX(XOFFSET), DPIFIX(0), DPIFIX(iWidth*2), DPIFIX(iHeight*2))) {
						printf("ISDrawRGB failed, code %d\n", IS40_GetLastError());
					}
					pWnd->ReleaseDC(pCDC);
				}
			}
		}

		debug("Reducing colors...\n");
		MYRGBTo8BitDithered((unsigned char*)hBuffer2, buf8, pal, dark);
	}

	debug("\n");

	// Note: if we want to save the image in TI format, there is a macro in TIPicViewDlg to do it

	// Draw the image 
	if (NULL != pWnd) {
		// temp hack - don't overdraw if it's using per-line palette (this approach won't work)
		if (!g_UsePerLinePalette) {
			debug("Draw final image...\n");
			CDC *pCDC=pWnd->GetDC();
			if (NULL != pCDC) {
				int dpi = GetDpiForWindow(pWnd->GetSafeHwnd());
				if (!IS40_StretchDraw8Bit(*pCDC, buf8, iWidth, iHeight, iWidth, winpal, DPIFIX(XOFFSET), DPIFIX(0), DPIFIX(iWidth*2), DPIFIX(iHeight*2))) {
					printf("ISDraw8 failed, code %d\n", IS40_GetLastError());
				}
				pWnd->ReleaseDC(pCDC);
			}
			// and while we've got it, if there's an output name
			if (cmdFileOut != NULL) {
				CString csName = cmdFileOut;
				csName+=".BMP";
				HISDEST dst = IS40_OpenFileDest(csName, FALSE);
				if (NULL != dst) {
					IS40_WriteBMP(dst, buf8, iWidth, iHeight, 8, iWidth, 256, winpal, NULL, 0);
					IS40_CloseDest(dst);
				}
			}
		}
	}

	fFirstLoad=true;
	delete[] hBuffer2;
}

void BuildFileList(char *szFolder)
{
	HANDLE hIndex;
	WIN32_FIND_DATA dat;
	char buffer[256];

	strcpy_s(buffer, 256, szFolder);
	strcat_s(buffer, 256, "\\*.*");
	hIndex=FindFirstFile(buffer, &dat);

	while (INVALID_HANDLE_VALUE != hIndex) {
		if (iCnt>MAXFILES-1) {
			FindClose(hIndex);
			return;
		}
		
		strcpy_s(buffer, 256, szFolder);
		strcat_s(buffer, 256, "\\");
		strcat_s(buffer, 256, dat.cFileName);

		if (dat.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (dat.cFileName[0]=='.') goto next;
			BuildFileList(buffer);
			goto next;
		}

		// BMP, GIF, JPG, PNG, PCX, TIF
		// Check last three characters
		if ((0==_stricmp(&buffer[strlen(buffer)-3], "bmp")) ||
			(0==_stricmp(&buffer[strlen(buffer)-4], "tiap")) ||
			(0==_stricmp(&buffer[strlen(buffer)-3], "gif")) ||
			(0==_stricmp(&buffer[strlen(buffer)-3], "jpg")) ||
			(0==_stricmp(&buffer[strlen(buffer)-4], "jpeg")) ||
			(0==_stricmp(&buffer[strlen(buffer)-3], "jpc")) ||
			(0==_stricmp(&buffer[strlen(buffer)-3], "png")) ||
			(0==_stricmp(&buffer[strlen(buffer)-3], "pcx")) ||
			(0==_stricmp(&buffer[strlen(buffer)-4], "tiff")) ||
			(0==_stricmp(&buffer[strlen(buffer)-3], "tif"))) {
				strcpy_s(&szFiles[iCnt++][0], 256, buffer);
		}

next:
		if (false == FindNextFile(hIndex, &dat)) {
			int ret;
			if ((ret=GetLastError()) != ERROR_NO_MORE_FILES) {
				OutputDebugString("Error in findnextfile\n");
			}
			FindClose(hIndex);
			hIndex=INVALID_HANDLE_VALUE;
		}
	}
}

// Scales from hBuffer to hBuffer2
bool ScalePic(int nFilter, int nPortraitMode)
{
#define X_AXIS 1
#define Y_AXIS 2
	
	float_precision x1,y1,x_scale,y_scale;
	int r;
	unsigned int thisx, thisy;
	HGLOBAL tmpBuffer;
	
	debug("Image:  %d x %d\n",inWidth, inHeight);
	debug("Output: %d x %d\n",currentw, currenth);
	
	x1=(float_precision)(inWidth);
	y1=(float_precision)(inHeight);
	
	x_scale=((float_precision)(currentw))/x1;
	y_scale=((float_precision)(currenth))/y1;

	debug("Scale:  %f x %f\n",x_scale,y_scale);
	
	if (ScaleMode == -1) {
		ScaleMode=Y_AXIS;
	
		if (y1*x_scale > (float_precision)(currenth)) ScaleMode=Y_AXIS;
		if (x1*y_scale > (float_precision)(currentw)) ScaleMode=X_AXIS;
		debug("Decided scale (1=X, 2=Y): %d\n",ScaleMode);
	} else {
		debug("Using scale (1=X, 2=Y): %d\n",ScaleMode);
	}
	
	// "portrait mode" is used for both X and Y axis as a fill mode now
	switch (nPortraitMode) {
		default:
		case 0:		// full image
			// no change
			break;

		case 1:		// top/left
		case 2:		// middle
		case 3:		// bottom/right
			// use other axis instead, and we'll crop it after we scale
			debug("Scaling for fill mode (opposite scale used).\n");
			if (ScaleMode == Y_AXIS) {
				ScaleMode=X_AXIS;
			} else {
				ScaleMode=Y_AXIS;
			}
			break;
	}

	if (ScaleMode==Y_AXIS) {
		x1*=y_scale;
		y1*=y_scale;
	} else {
		x1*=x_scale;
		y1*=x_scale;
	}
	
	x1+=0.5;
	y1+=0.5;
	finalW=(int)(x1);
	finalH=(int)(y1);

	debug("Output size: %d x %d\n", finalW, finalH);

	switch (nFilter) {
		case 0 :
			{
				C2PassScale <CBoxFilter> ScaleEngine;
				tmpBuffer=ScaleEngine.AllocAndScale((unsigned char*)hBuffer, inWidth, inHeight, finalW, finalH);
			}
			break;

		case 1 :
			{
				C2PassScale <CGaussianFilter> ScaleEngine;
				tmpBuffer=ScaleEngine.AllocAndScale((unsigned char*)hBuffer, inWidth, inHeight, finalW, finalH);
			}
			break;

		case 2 :
			{
				C2PassScale <CHammingFilter> ScaleEngine;
				tmpBuffer=ScaleEngine.AllocAndScale((unsigned char*)hBuffer, inWidth, inHeight, finalW, finalH);
			}
			break;

		case 3 :
			{
				C2PassScale <CBlackmanFilter> ScaleEngine;
				tmpBuffer=ScaleEngine.AllocAndScale((unsigned char*)hBuffer, inWidth, inHeight, finalW, finalH);
			}
			break;

		default:
		case 4 :
			{
				C2PassScale <CBilinearFilter> ScaleEngine;
				tmpBuffer=ScaleEngine.AllocAndScale((unsigned char*)hBuffer, inWidth, inHeight, finalW, finalH);
			}
			break;
	}

	// apply portrait cropping if needed, max Y is 192 pixels
	if (finalH > 192) {
		int y=0;

		debug("Cropping...\n");

		switch (nPortraitMode) {
			default:
			case 0:		// full
				debug("This could be a problem - image scaled too large!\n");
				// no change
				break;

			case 1:		// top
				// just tweak the size
				break;

			case 2:		// middle
				// move the middle to the top
				y=(finalH-192)/2;
				break;

			case 3:		// bottom
				// move the bottom to the top
				y=(finalH-192);
				break;
		}
		if (heightoffset != 0) {
			debug("Vertical nudge %d pixels...\n", heightoffset);

			y+=heightoffset;
			while (y<0) { 
				y++;
				heightoffset++;
			}
			while ((unsigned)y+191 >= finalH) {
				y--;
				heightoffset--;
			}
		}
		finalH=192;
		if (y > 0) {
			memmove(tmpBuffer, (unsigned char*)tmpBuffer+y*finalW*3, 192*finalW*3);
		}
	}
	// apply landscape cropping if needed, max X is 256 pixels
	if (finalW > 256) {
		int x;
		unsigned char *p1,*p2;

		debug("Cropping...\n");

		switch (nPortraitMode) {
			default:
			case 0:		// full
				debug("This could be a problem - image scaled too large!\n");
				// no change
				x=-1;
				break;

			case 1:		// top
				// just copy the beginning rows
				x=0;
				break;

			case 2:		// middle
				// move the middle 
				x=(finalW-256)/2;
				break;

			case 3:		// bottom
				// move the right edge
				x=(finalW-256);
				break;
		}
		if (x >= 0) {
			// we need to move each row manually!
			p1=(unsigned char*)tmpBuffer;
			p2=p1+x*3;
			for (unsigned int y=0; y<finalH; y++) {
				memmove(p1, p2, 256*3);
				p2+=finalW*3;
				p1+=256*3;
			}
		}
		finalW=256;
	}

	if (NULL == hBuffer2) {
		hBuffer2=new BYTE[iWidth * iHeight * 3];
		ZeroMemory(hBuffer2, iWidth * iHeight * 3);
	}

	// calculate the exact position. If this is the last image (remaining space will be too small),
	// then we will center this one in the remaining space
	thisx=(currentw-finalW)/2;
	thisy=(currenth-finalH)/2;

	currenth-=finalH;
	currentw-=finalW;
	currentx+=finalW;
	currenty+=finalH;

	r=IS40_OverlayImage((unsigned char*)hBuffer2, iWidth, iHeight, 3, iWidth*3, (unsigned char*)tmpBuffer, finalW, finalH, finalW*3, thisx, thisy, 1.0, 0, 0, 0);
	if (false==r) {
		printf("Overlay failed.\n");
		return false;
	}
	delete tmpBuffer;

	// handle shifting
	if (pixeloffset != 0) {
		printf("Nudging %d pixels...\n", pixeloffset);
		if (pixeloffset > 0) {
			memmove(hBuffer2, (char*)hBuffer2+pixeloffset*3, (256*192-pixeloffset)*3);
		} else {
			memmove((char*)hBuffer2-pixeloffset*3, hBuffer2, (256*192+pixeloffset)*3);
		}
	}

	return true;
}

int instr(unsigned short *s1, char *s2)
{
	while (*s1)
	{
		if (*s1 != *s2) {
			s1++;
		} else {
			break;
		}
	}

	if (0 == *s1) {
		return 0;
	}

	while (*s2)
	{
		if (*(s1++) != *(s2++)) {
			return 0;
		}
	}

	return 1;
}


