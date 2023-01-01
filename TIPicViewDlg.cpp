// TIPicViewDlg.cpp : implementation file
//

#include "stdafx.h"
#include <Windows.h>
#include "TIPicView.h"
#include "TIPicViewDlg.h"
#include "D:\WORK\imgsource\4.0\islibs40_vs17_unicode\ISource.h"
#include "xbtest.h"
#include "ClipboardRead.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern bool StretchHist;
static bool fInSlideMode=false;
extern int PIXA,PIXB,PIXC,PIXD,PIXE,PIXF;
extern int g_orderSlide;
extern int g_ctrlList;
extern int g_nFilter;
extern int g_PowerPaint;
extern int g_nPortraitMode;
extern int pixeloffset, heightoffset;
extern int g_Perceptual;
extern int g_UseColorOnly;
extern int g_UseHalfMulticolor;
extern int g_UseMulticolorOnly;
extern int g_AccumulateErrors;
extern int g_MaxMultiDiff;
extern int g_MaxColDiff;
extern int g_UsePerLinePalette;
extern int g_UsePalette;
extern int g_GreyPalette;
extern bool g_bDisplayPalette;
extern int g_StaticColors;
extern int g_MatchColors;
extern bool g_bStaticByPopularity;
extern int g_OrderedDither;
extern int g_MapSize;
extern unsigned char scanlinepal[192][16][4];
extern double g_PercepR, g_PercepG, g_PercepB;
extern double g_LumaEmphasis;
extern double g_Gamma;
extern wchar_t *cmdFileIn, *cmdFileOut;
extern unsigned char buf8[256*192];
extern RGBQUAD winpal[256];
extern bool fVerbose;

int nLastValidConversionMode = 0;
int nOutputPSize=6144, nOutputCSize=6144, nOutputMSize=6144;

#if SIZE_OF_XBTEST > 32768
#error XB TEST Is too large!
#endif
unsigned char XBWorkBuf[32768];

// the colors are resorted to improve performance in some cases - this macro changes
// internal color indexes back to TI color indexes
// Currently: 0 - white (15)
//            1 - black (1 - no change)
//            2 - grey (14)
//           >2 - subtract 1
#define REMAP_COLOR(x) ((x)==0 ? 15 : ((x)==2) ? 14 : ((x)>2) ? (x)-1 : (x) )

enum {
	MODE_BITMAP,				// bitmap
	MODE_GREY_BITMAP,			// greyscale bitmap
	MODE_MONO_BITMAP,			// black and white bitmap
	MODE_MULTICOLOR,			// multicolor
	MODE_DUAL_MULTICOLOR,		// dual-multicolor
	MODE_HALF_MULTICOLOR,		// Half-multicolor
	MODE_COLOR_ONLY,			// bitmap with all pattern bytes set to 0 (32x192)
	MODE_F18_PALETTE,			// F18 Palette
	MODE_F18_PALETTE_SCANLINE	// F18 Palette per scanline
};

// handle to global shared memory used for multiple instances to share random fileloads
// this just makes it easier to compare different render options side by side, hopefully
// it won't break anything
HANDLE hSharedMemory = NULL;
LPVOID pSharedPointer = NULL;
wchar_t szLastFilename[256];

// Assuming TIAP files which are normally 6144 bytes each, PROGRAM image, no 6 byte header
// nSize must be less than 64k 
void DoTIFILES(FILE *fp, int nSize) {
	/* create TIFILES header */
	unsigned char h[128];						// header

	memset(h, 0, 128);							// initialize
	h[0] = 7;
	h[1] = 'T';
	h[2] = 'I';
	h[3] = 'F';
	h[4] = 'I';
	h[5] = 'L';
	h[6] = 'E';
	h[7] = 'S';
	h[8] = 0;									// length in sectors HB
	h[9] = (nSize+255)/256;						// LB (24*256=6144)
	h[10] = 1;									// File type (1=PROGRAM)
	h[11] = 0;									// records/sector
	h[12] = nSize%256;							// # of bytes in last sector (0=256)
	h[13] = 0;									// record length 
	h[14] = 0;									// # of records(FIX)/sectors(VAR) LB!
	h[15] = 0;									// HB 
	fwrite(h, 1, 128, fp);
}

// Assuming TIAP files which are 6144 bytes each, PROGRAM image, no 6 byte header
// used only with magic filename V9T9, but we pass it anyway. Must be less
// than 8 chars and have no magic symbols
// nSize has the same restrictions as TIFILES above
void DoV9T9(FILE *fp, CString cs, int nSize) {
	/* create V9T9 header */
	unsigned char h[128];								// header

	memset(h, 0, 128);							// initialize
	h[0] = cs.GetLength()>0?cs[0]:' ';
	h[1] = cs.GetLength()>1?cs[1]:' ';
	h[2] = cs.GetLength()>2?cs[2]:' ';
	h[3] = cs.GetLength()>3?cs[3]:' ';
	h[4] = cs.GetLength()>4?cs[4]:' ';
	h[5] = cs.GetLength()>5?cs[5]:' ';
	h[6] = cs.GetLength()>6?cs[6]:' ';
	h[7] = cs.GetLength()>7?cs[7]:' ';
	h[8] = cs.GetLength()>8?cs[8]:' ';
	h[9] = cs.GetLength()>9?cs[9]:' ';
	h[10] = 0;
	h[11] = 0;
	h[12] = 1;					
	h[13] = 0;						
	h[14] = 0;						
	h[15] = (nSize+255)/256;						
	fwrite(h, 1, 128, fp);
}

// update a dv254 TI file (just skipping record markers)
// returns new target
unsigned char*UpdateWorkBuf(unsigned char *src, unsigned char *dest, int cnt) {
	while (cnt--) {
		if ((*dest==0xff)&&(*(dest+1)==0xfe)) {
			dest+=2;	// skip end of record and beginning of the next one
		}
		*dest++ = *src++;
	}

	return dest;
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTIPicViewDlg dialog

CTIPicViewDlg::CTIPicViewDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTIPicViewDlg::IDD, pParent)
	, m_nOrderSlide(0)
	,m_chkPowerPaint(FALSE)
{
	//{{AFX_DATA_INIT(CTIPicViewDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	StretchHist=false;
	PIXA=2;
	PIXB=2;
	PIXC=2;
	PIXD=2;
	PIXE=1;
	PIXF=1;
}

void CTIPicViewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTIPicViewDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDX_Control(pDX,IDC_COMBO1,m_ctrlList);
	DDX_Text(pDX,IDC_EDIT1,m_pixelD);
	DDV_MinMaxUInt(pDX,m_pixelD,0,16);
	DDX_Text(pDX,IDC_EDIT2,m_pixelC);
	DDV_MinMaxUInt(pDX,m_pixelC,0,16);
	DDX_Text(pDX,IDC_EDIT3,m_pixelB);
	DDV_MinMaxUInt(pDX,m_pixelB,0,16);
	DDX_Text(pDX,IDC_EDIT4,m_pixelA);
	DDV_MinMaxUInt(pDX,m_pixelA,0,16);
	DDX_Text(pDX,IDC_EDIT6,m_pixelE);
	DDV_MinMaxUInt(pDX,m_pixelE,0,16);
	DDX_Text(pDX,IDC_EDIT5,m_pixelF);
	DDV_MinMaxUInt(pDX,m_pixelF,0,16);
	DDX_CBIndex(pDX,IDC_FILTER,m_nFilter);
	DDX_CBIndex(pDX,IDC_PORTRAIT,m_nPortraitMode);
	DDX_CBIndex(pDX,IDC_ERRMODE,m_ErrMode);
	DDX_Text(pDX,IDC_MAXDIFF,m_MaxDiff);
	DDV_MinMaxInt(pDX,m_MaxDiff,1,100);
	DDX_Text(pDX,IDC_COLDIFF,m_MaxColDiff);
	DDV_MinMaxInt(pDX,m_MaxColDiff,0,100);
	DDX_Text(pDX,IDC_PERPR,m_PercepR);
	DDV_MinMaxInt(pDX,m_PercepR,0,100);
	DDX_Text(pDX,IDC_PERPG,m_PercepG);
	DDV_MinMaxInt(pDX,m_PercepG,0,100);
	DDX_Text(pDX,IDC_PERPB,m_PercepB);
	DDV_MinMaxInt(pDX,m_PercepB,0,100);
	DDX_Text(pDX,IDC_STATICCNT,m_StaticCols);
	DDV_MinMaxInt(pDX,m_StaticCols,0,14);
	DDX_Text(pDX,IDC_LUMA,m_Luma);
	DDX_Text(pDX,IDC_GAMMA,m_Gamma);
	DDX_Slider(pDX,IDC_ORDERSLIDE,m_nOrderSlide);
	DDV_MinMaxInt(pDX,m_nOrderSlide,0,16);
	DDX_Control(pDX,IDC_ORDERSLIDE,ctrlOrderSlide);
	DDX_Check(pDX,IDC_CHKPOWERPAINT,m_chkPowerPaint);
}

BEGIN_MESSAGE_MAP(CTIPicViewDlg, CDialog)
	//{{AFX_MSG_MAP(CTIPicViewDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
	ON_BN_CLICKED(IDC_BUTTON2, OnButton2)
	ON_BN_CLICKED(IDC_CHECK1, OnCheck1)
	ON_BN_CLICKED(IDC_RND, OnRnd)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON4, OnButton4)
	ON_BN_DOUBLECLICKED(IDC_RND, OnDoubleclickedRnd)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_DROPFILES()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON3, &CTIPicViewDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON5, &CTIPicViewDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_PERCEPT, &CTIPicViewDlg::OnBnClickedPercept)
	ON_BN_CLICKED(IDC_RELOAD, &CTIPicViewDlg::OnBnClickedReload)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CTIPicViewDlg::OnCbnSelchangeCombo1)
	ON_BN_CLICKED(IDC_PALETTE, &CTIPicViewDlg::OnBnClickedPalette)
	ON_BN_CLICKED(IDC_DISTDEFAULT, &CTIPicViewDlg::OnBnClickedFloyd)
	ON_BN_CLICKED(IDC_PERPRESET, &CTIPicViewDlg::OnBnClickedPerpreset)
	ON_BN_CLICKED(IDC_PATTERN, &CTIPicViewDlg::OnBnClickedPattern)
	ON_BN_CLICKED(IDC_NODITHER, &CTIPicViewDlg::OnBnClickedNodither)
	ON_BN_CLICKED(IDC_DISTDEFAULT2, &CTIPicViewDlg::OnBnClickedAtkinson)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_ORDERED, &CTIPicViewDlg::OnBnClickedOrdered)
	ON_BN_CLICKED(IDC_ORDERED2, &CTIPicViewDlg::OnBnClickedOrdered2)
	ON_BN_CLICKED(IDC_DIAG, &CTIPicViewDlg::OnBnClickedDiag)
	ON_BN_CLICKED(IDC_ORDERED3, &CTIPicViewDlg::OnBnClickedOrdered3)
	ON_BN_CLICKED(IDC_ORDERED4, &CTIPicViewDlg::OnBnClickedOrdered4)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTIPicViewDlg message handlers

BOOL CTIPicViewDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// fill in the drop list (order must match the enum!)
	m_ctrlList.ResetContent();
	m_ctrlList.AddString(_T("Bitmap 9918A"));
	m_ctrlList.AddString(_T("Greyscale Bitmap 9918A"));
	m_ctrlList.AddString(_T("B&W Bitmap 9918A"));
	m_ctrlList.AddString(_T("Multicolor 9918"));
	m_ctrlList.AddString(_T("Dual-Multicolor (flicker) 9918"));
	m_ctrlList.AddString(_T("Half-Multicolor (flicker) 9918A"));
	m_ctrlList.AddString(_T("Bitmap color only 9918A"));
	m_ctrlList.AddString(_T("Paletted Bitmap F18A"));
	m_ctrlList.AddString(_T("Scanline Palette Bitmap F18A"));
	m_ctrlList.SetCurSel(g_ctrlList);
	OnCbnSelchangeCombo1();

	// set up the slider
	ctrlOrderSlide.SetRange(0, 16);

	// update configuration pane
	m_pixelA=PIXA;
	m_pixelB=PIXB;
	m_pixelC=PIXC;
	m_pixelD=PIXD;
	m_pixelE=PIXE;
	m_pixelF=PIXF;
	m_nOrderSlide=g_orderSlide;
	m_nFilter=g_nFilter;
	m_chkPowerPaint = g_PowerPaint ? true : false;
	m_nPortraitMode=g_nPortraitMode;
	m_ErrMode=g_AccumulateErrors?1:0;
	m_MaxDiff=g_MaxMultiDiff;
	m_MaxColDiff = g_MaxColDiff;
	m_PercepR = (int)(g_PercepR*100.0);
	m_PercepG = (int)(g_PercepG*100.0);
	m_PercepB = (int)(g_PercepB*100.0);
	m_Luma = (int)(g_LumaEmphasis*100.0);
	m_Gamma = (int)(g_Gamma*100.0);
	m_StaticCols = g_StaticColors;
	m_Popularity = g_bStaticByPopularity ? 1: 0;
	PrepareData();

	// interface to the dithering ordered buttons
	if (g_OrderedDither != 0) {
		EnableDitherCtrls(false);
		int option = g_OrderedDither + (g_MapSize==4 ? 2: 0);
		switch (option) {
		case 1: SendDlgItemMessage(IDC_ORDERED, BM_SETCHECK, BST_CHECKED, 0); break;
		case 2: SendDlgItemMessage(IDC_ORDERED2, BM_SETCHECK, BST_CHECKED, 0); break;
		case 3: SendDlgItemMessage(IDC_ORDERED3, BM_SETCHECK, BST_CHECKED, 0); break;
		case 4: SendDlgItemMessage(IDC_ORDERED4, BM_SETCHECK, BST_CHECKED, 0); break;
		}
	}

	// hacky command line interface
	if (cmdFileIn != NULL) {
		LaunchMain(2, cmdFileIn);	// load
	}
	if (cmdFileOut != NULL) {
		OnButton4();		// save
		EndDialog(IDOK);	// and quit
	}

	InterlockedExchange(&cs, 0);		// no thread running yet
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CTIPicViewDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTIPicViewDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CPaintDC dc(this);

		if (NULL != buf8) {
			int dpi = GetDpiForWindow(GetSafeHwnd());
			IS40_StretchDraw8Bit(dc, buf8, 256, 192, 256, winpal, DPIFIX(XOFFSET), DPIFIX(0), DPIFIX(256*2), DPIFIX(192*2));
		}
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTIPicViewDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CTIPicViewDlg::OnOK() {}

void maincode(int mode, CString pFile, double dark);
// this is actually 'Open' now
void CTIPicViewDlg::OnRnd() 
{
	CString csPath;

	if (fInSlideMode) {
		OnDoubleclickedRnd();
		return;
	}

	// Get Path filename
	CFileDialog dlg(true);
	dlg.m_ofn.lpstrTitle=_T("Select file to open: BMP, GIF, JPG, PNG, PCX or TIFF");
	if (IDOK != dlg.DoModal()) {
		return;
	}
	csPath=dlg.GetPathName();

	LaunchMain(2, csPath.GetString());

}

void CTIPicViewDlg::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	fInSlideMode=true;
	CWnd *pCtl=GetDlgItem(IDC_RND);
	if (pCtl) pCtl->SetWindowText(_T("Next"));
	SetTimer(0, 500, NULL);		// start a regular tick to watch for new files

	CDialog::OnLButtonDblClk(nFlags, point);
}

// This function doesn't get called normally, but that's okay, we can call it the long way ;)
void CTIPicViewDlg::OnDoubleclickedRnd() 
{
	static CString csPath;

	if (csPath=="") {
		// Get Path image
		CFileDialog dlg(true);
		dlg.m_ofn.lpstrTitle=_T("Select any file in a folder for random load - filename will be ignored");
		if (IDOK != dlg.DoModal()) {
			return;
		}
		csPath=dlg.GetPathName();
		if (csPath.ReverseFind('\\') != -1) {
			csPath=csPath.Left(csPath.ReverseFind('\\'));
		}
	}

	// make sure the shared memory is available
	if (NULL == hSharedMemory) {
		hSharedMemory = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, _T("Convert9918MappedData"));
		if (NULL != hSharedMemory) {
			pSharedPointer = MapViewOfFile(hSharedMemory, FILE_MAP_ALL_ACCESS, 0, 0, 4096);
		} else {
			pSharedPointer = NULL;
		}
	}

	LaunchMain(0,csPath);
	// display the picture on the dialog
}


void CTIPicViewDlg::OnCancel() 
{
}

void CTIPicViewDlg::OnButton2() 
{
	if (pixeloffset < 7) {
		pixeloffset++;
		LaunchMain(1,"");
	}
}

void CTIPicViewDlg::OnButton1() 
{
	if (pixeloffset > -7) {
		pixeloffset--;
		LaunchMain(1,"");
	}
}

// stretch histogram
void CTIPicViewDlg::OnCheck1() 
{
	CButton *pCtrl=(CButton*)GetDlgItem(IDC_CHECK1);
	if (pCtrl) {
		if (pCtrl->GetCheck() == BST_CHECKED) {
			StretchHist=true;
		} else {
			StretchHist=false;
		}
	}
}

void CTIPicViewDlg::OnClose() 
{
	EndDialog(IDOK);
	CDialog::OnClose();
	if (pSharedPointer) {
		UnmapViewOfFile(pSharedPointer);
		pSharedPointer = NULL;
	}
	if (hSharedMemory) {
		CloseHandle(hSharedMemory);
	}
}

unsigned char *CTIPicViewDlg::RLEEncode(unsigned char *pbuf, int *nSize, int InSize) {
	int nOffset=0;
	int nOutputSize=0;
	unsigned char *pRet = (unsigned char*)malloc(InSize*2);	// worst case
	while (nOffset < InSize) {
		// decide whether to do a run or data
		if ((pbuf[nOffset]==pbuf[nOffset+1]) && (pbuf[nOffset] == pbuf[nOffset+2])) {
			// worth doing an RLE run
			// work out for how far!
			int i=nOffset+3;
			while (pbuf[i] == pbuf[nOffset]) {
				i++;
				if (i>=InSize) break;
				if (i-nOffset >= 127) break;
			}
			pRet[nOutputSize++] = 0x80|(i-nOffset);
			pRet[nOutputSize++] = pbuf[nOffset];
			nOffset=i;
		} else {
			// not worth doing RLE -- see for how long
			int i=nOffset+1;
			while ((pbuf[i]!=pbuf[i+1]) || (pbuf[i] != pbuf[i+2])) {
				i++;
				if (i>=InSize) break;
				if (i-nOffset >= 127) break;
			}
			pRet[nOutputSize++] = i-nOffset;
			for (int c=0; c<i-nOffset; c++) {
				pRet[nOutputSize++] = pbuf[nOffset+c];
			}
			nOffset=i;
		}
	}
	*nSize = nOutputSize;
	debug(_T("RLE compress table from %d to %d bytes\n"), InSize, nOutputSize);
	return pRet;
}

extern unsigned char buf8[256*192];
extern bool fFirstLoad;
void CTIPicViewDlg::OnButton4() 
{
	FILE *fP = NULL, *fC = NULL, *fM = NULL;
	unsigned char cbuf[6144], pbuf[6144], mbuf[6144];
	unsigned char *pRLEC = NULL, *pRLEP = NULL, *pRLEM = NULL;
	int nOffset;

	memset(cbuf, 0, sizeof(cbuf));
	memset(pbuf, 0, sizeof(pbuf));
	memset(mbuf, 0, sizeof(mbuf));

	if (!fFirstLoad) {
		if (fVerbose) {
			debug(_T("No file loaded to save.\n"));
		}
		return;
	}

    enum FILETYPES {
	    ftTIFILES = 1,
	    ftV9T9,
	    ftRaw,
	    ftRLE,
	    ftTIXB,
        ftTIXBRLE,
	    ftMSXSC2,
        ftCVPaint,
		ftCVPowerPaint,
	    ftColecoROM,
	    ftColecoROMRLE,
	    ftPNG
    };

	CString csFmt = _T("TIFILES Format Header|*.*|V9T9 Format Header|*.*|Raw Files|*.*|RLE Files|*.*|TI XB Program|*.*|TI XB RLE Program|*.*|MSX SC2|*.SC2|Coleco CVPaint|*.pc|Adam PowerPaint 10k|*.pp|ColecoVision Cart|*.ROM|ColecoVision RLE Cart (Broken)|*.ROM|PNG file (PC)|*.PNG||");
	CFileDialog dlg(false, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, csFmt);

	// Save image
	dlg.m_ofn.lpstrTitle=_T("Enter base name - do not include extensions.");

	if (!ResetToValidConversion()) {
		return;
	}

	if ((cmdFileOut!=NULL) || (IDOK == dlg.DoModal())) {
		CString cs, outfile, rawfile;
		HANDLE hFile;
		bool bOverwrite=false;

		outfile=dlg.GetPathName();
		if (cmdFileOut != NULL) {
			outfile = cmdFileOut;
		}
		if (dlg.m_pOFN->nFilterIndex==ftV9T9) {
			rawfile=dlg.GetFileName();
			if (rawfile.GetLength() > 6) {
				printf("Truncating outfile filename for V9T9\n");
				int nCount=rawfile.GetLength()-6;
				outfile=outfile.Left(outfile.GetLength()-nCount);
				rawfile=rawfile.Left(6);
			}
		}

		// strip extension if present
		if (outfile.ReverseFind('.') != -1) {
			outfile=outfile.Left(outfile.ReverseFind('.'));
		}
		
		// check for existance
        bool checkTI = false;
        switch (dlg.m_pOFN->nFilterIndex) {
            case ftTIFILES:
            case ftV9T9:
            case ftRaw:
            case ftRLE:
                checkTI = true;
                break;

		    case ftTIXB:
		    case ftTIXBRLE:
		    case ftMSXSC2:
			case ftCVPaint:
			case ftCVPowerPaint:
		    case ftColecoROM:
		    case ftColecoROMRLE:
		    case ftPNG:
                checkTI = false;
                break;
        }

		// TODO: this is probably no longer very good with the multitude of extensions we have
		if (checkTI) {
			// overwrite test for TIAP filenames
			cs=outfile;
			cs+=".TIAP";
			hFile=CreateFile(cs, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (INVALID_HANDLE_VALUE != hFile) {
				CloseHandle(hFile);
				if (cmdFileIn == NULL) {
					if (IDYES != AfxMessageBox(_T("File(s) exist. Overwrite?"), MB_YESNO)) {
						return;
					}
				}
				bOverwrite=true;
			}

			if (nOutputCSize > 0) {
				cs=outfile;
				cs+=".TIAC";
				hFile=CreateFile(cs, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if (INVALID_HANDLE_VALUE != hFile) {
					CloseHandle(hFile);
					if (!bOverwrite) {
						if (cmdFileIn == NULL) {
							if (IDYES != AfxMessageBox(_T("File(s) exist. Overwrite?"), MB_YESNO)) {
								return;
							}
						}
					}
					bOverwrite=true;
				}
			}

			if (g_UseHalfMulticolor) {
				cs=outfile;
				cs+=".TIAM";
				hFile=CreateFile(cs, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if (INVALID_HANDLE_VALUE != hFile) {
					CloseHandle(hFile);
					if (!bOverwrite) {
						if (cmdFileIn == NULL) {
							if (IDYES != AfxMessageBox(_T("File(s) exist. Overwrite?"), MB_YESNO)) {
								return;
							}
						}
					}
					bOverwrite=true;
				}
			}
		}
		
		// we now get to process buf8. we have to increment all color indexes by 1 to save
		switch (dlg.m_pOFN->nFilterIndex) {
			default:
			case ftTIFILES:	// TIFILES
				dlg.m_pOFN->nFilterIndex=ftTIFILES; // just to be sure (because of the default case)
				debug(_T("Writing TIFILES headers.\n"));
				break;

			case ftV9T9:	// V9T9
				debug(_T("Writing V9T9 headers.\n"));
				break;

			case ftRaw:	// No header
				debug(_T("No headers will be written.\n"));
				break;

			case ftRLE: // RLE
				debug(_T("RLE files with no header will be written\n"));
				break;

			case ftTIXB: // XB Program
				if (g_UseHalfMulticolor) {
					AfxMessageBox(_T("Half-multicolor not supported with Extended BASIC."));
					return ;
				}
				if (g_UseMulticolorOnly) {
					AfxMessageBox(_T("Multicolor not supported with Extended BASIC."));
					return ;
				}
				if (g_UsePerLinePalette) {
					AfxMessageBox(_T("Per-Line paletted images not supported with Extended BASIC."));
					return ;
				}
				if (g_UsePalette) {
					AfxMessageBox(_T("Paletted images not supported with Extended BASIC."));
					return ;
				}
				debug(_T("XB Program image with TIFILES header will be written\n"));
				break;

			case ftTIXBRLE: // XB RLE Program
				if (g_UseHalfMulticolor) {
					AfxMessageBox(_T("Half-multicolor not supported with Extended BASIC."));
					return ;
				}
				if (g_UseMulticolorOnly) {
					AfxMessageBox(_T("Multicolor not supported with Extended BASIC."));
					return ;
				}
				if (g_UsePerLinePalette) {
					AfxMessageBox(_T("Per-Line paletted images not supported with Extended BASIC."));
					return ;
				}
				if (g_UsePalette) {
					AfxMessageBox(_T("Paletted images not supported with Extended BASIC."));
					return ;
				}
				debug(_T("XB RLE Program image with TIFILES header will be written\n"));
				break;

			case ftMSXSC2: // MSX SC2
				if (g_UseHalfMulticolor) {
					AfxMessageBox(_T("Half-multicolor not supported with MSX dump."));
					return ;
				}
				if (g_UseMulticolorOnly) {
					AfxMessageBox(_T("Multicolor not supported with MSX dump."));
					return ;
				}
				if (g_UsePerLinePalette) {
					AfxMessageBox(_T("Per-Line paletted images not supported with MSX dump."));
					return ;
				}
				if (g_UsePalette) {
					AfxMessageBox(_T("Paletted images not supported with MSX dump."));
					return ;
				}
				debug(_T("MSX *.SC2 type image will be written\n"));
				break;

			case ftCVPaint:	// No header
			case ftCVPowerPaint:
				debug(_T("No headers will be written.\n"));
				break;

            case ftColecoROM:	// ColecoVision cartridge
				if (g_UseHalfMulticolor) {
					AfxMessageBox(_T("Half-multicolor not supported with ColecoVision ROM."));
					return ;
				}
				if (g_UseMulticolorOnly) {
					AfxMessageBox(_T("Multicolor not supported with ColecoVision ROM."));
					return ;
				}
				if (g_UsePerLinePalette) {
					AfxMessageBox(_T("Per-Line paletted images not supported with ColecoVision ROM."));
					return ;
				}
				if (g_UsePalette) {
					AfxMessageBox(_T("Paletted images not supported with ColecoVision ROM."));
					return ;
				}
				debug(_T("ColecoVision cartridge will be written\n"));
				break;

			case ftColecoROMRLE:	// ColecoVision RLE cartridge
				if (g_UseHalfMulticolor) {
					AfxMessageBox(_T("Half-multicolor not supported with ColecoVision ROM."));
					return ;
				}
				if (g_UseMulticolorOnly) {
					AfxMessageBox(_T("Multicolor not supported with ColecoVision ROM."));
					return ;
				}
				if (g_UsePerLinePalette) {
					AfxMessageBox(_T("Per-Line paletted images not supported with ColecoVision ROM."));
					return ;
				}
				if (g_UsePalette) {
					AfxMessageBox(_T("Paletted images not supported with ColecoVision ROM."));
					return ;
				}
				debug(_T("ColecoVision RLE cartridge will be written\n"));
				break;

			case ftPNG:	// PNG file for PC
				// this might work, right now I don't want to figure out how ;)
				if (g_UsePerLinePalette) {
					AfxMessageBox(_T("Per-Line paletted images not currently supported with PNG export."));
					return ;
				}
				debug(_T("PC PNG file will be written\n"));
				break;
		}

		// WARNING: All file types must open their file in this switch!!
		// TODO: the filename changes here mean that we don't always warn about overwrite
		// We should also check whether we needed to add the extension...
		cs=outfile;
		switch (dlg.m_pOFN->nFilterIndex) {
	        case ftTIFILES:
	        case ftV9T9:
	        case ftRaw:
	        case ftRLE:
		        if ((cs.Right(2).MakeUpper() != _T("_P")) && (cs.Right(5).MakeUpper() != _T(".TIAP"))) {
				    cs+="_P";
				}
			    cs.MakeUpper();
                {
			        CString csRealName = cs;
			        if (csRealName.Right(2).MakeUpper() == _T("_P")) {
				        csRealName = csRealName.Left(csRealName.GetLength()-2) + _T(".TIAP");
			        }
			        _wfopen_s(&fP, csRealName, _T("wb"));
                }
			    if (NULL == fP) {
				    AfxMessageBox(_T("Failed to open _P file"));
				    return;
			    }
                break;

	        case ftTIXB:
	        case ftTIXBRLE:
				// we still need to open the output file - no name change from requested
			    _wfopen_s(&fP, cs, _T("wb"));
			    if (NULL == fP) {
				    AfxMessageBox(_T("Failed to open output file"));
				    return;
			    }
                break;

			case ftMSXSC2:
		        if (cs.Right(4).MakeUpper() != _T(".SC2")) {
					cs +=".sc2";
				}
				// we still need to open the output file - no name change from requested
			    _wfopen_s(&fP, cs, _T("wb"));
			    if (NULL == fP) {
				    AfxMessageBox(_T("Failed to open output file"));
				    return;
			    }
                break;

			// TODO: we should import CVPaint and PowerPaint too
            case ftCVPaint:
                // just a single output file for Coleco CVPaint
		        if (cs.Right(3).MakeUpper() != _T(".PC")) {
					cs+=_T(".pc");
				}
			    _wfopen_s(&fP, cs, _T("wb"));
			    if (NULL == fP) {
				    AfxMessageBox(_T("Failed to open output file"));
				    return;
			    }
				break;

            case ftCVPowerPaint:
                // just a single output file for Coleco Adam Powerpaint
		        if (cs.Right(3).MakeUpper() != _T(".PP")) {
					cs+=_T(".pp");
				}
			    _wfopen_s(&fP, cs, _T("wb"));
			    if (NULL == fP) {
				    AfxMessageBox(_T("Failed to open output file"));
				    return;
			    }
				break;

			case ftColecoROM:
	        case ftColecoROMRLE:
				// colecovision
		        if (cs.Right(4).MakeUpper() != _T(".ROM")) {
					cs+=_T(".rom");
				}
			    _wfopen_s(&fP, cs, _T("wb"));
			    if (NULL == fP) {
				    AfxMessageBox(_T("Failed to open output file"));
				    return;
			    }
				break;

            case ftPNG:
				// PC PNG
		        if (cs.Right(4).MakeUpper() != _T(".PNG")) {
					cs+=_T(".png");
				}
			    _wfopen_s(&fP, cs, _T("wb"));
			    if (NULL == fP) {
				    AfxMessageBox(_T("Failed to open output file"));
				    return;
			    }
                break;

		}

		// Is this a bug for TIFILES headers? The RLE files write the wrong filesize??
		// prepare headers
		switch (dlg.m_pOFN->nFilterIndex) {
			case ftTIFILES:	// TIFILES
				DoTIFILES(fP, nOutputPSize);
				break;

			case ftV9T9:	// V9T9
				DoV9T9(fP,rawfile+_T("_P"), nOutputPSize);
				break;

            case ftRaw:
            case ftRLE:
                // no headers
                break;

			case ftTIXB:	// XB
			case ftTIXBRLE: // XB w RLE
				// get a clean copy to work on
				memcpy(XBWorkBuf, XBTEST, SIZE_OF_XBTEST);
				break;

			case ftMSXSC2:	// MSX
				// there's a fixed (for us) 7 byte header to write
				// TODO: no good for multicolor
				fwrite("\xfe\x00\x00\x00\x38\x00\x00", 1, 7, fP);	// FE, load >0000, End >3800, Boot >0000 (disabled)
				break;

            case ftCVPaint:
			case ftCVPowerPaint:
			case ftColecoROM:	// ColecoVision
			case ftColecoROMRLE:// ColecoVision with RLE
				// no preparation needed here
				break;

			case ftPNG:	// PNG file
				// we don't need the conversion below, so just do the work and return here
				if (NULL != fP) {
					fclose(fP);
					fP=NULL;
				}
				HISDEST dst = IS40_OpenFileDest(cs, FALSE);
				if (NULL != dst) {
					if (0 == IS40_WritePNG(dst, buf8, 256, 192, 256, 8, 256, winpal, 3, 0.0, NULL, 0)) {
						AfxMessageBox(_T("Error while writing file!"));
					}
					IS40_CloseDest(dst);
				} else {
					AfxMessageBox(_T("Failed to save file!"));
				}
				return;
		}

		// only need 1 file for the demo outputs, otherwise we open the C and maybe M file here
        bool multiFile = false;
        switch (dlg.m_pOFN->nFilterIndex) {
            case ftTIFILES:
            case ftV9T9:
            case ftRaw:
            case ftRLE:
                multiFile = true;
                break;

		    case ftTIXB:
		    case ftTIXBRLE:
		    case ftMSXSC2:
            case ftCVPaint:
			case ftCVPowerPaint:
		    case ftColecoROM:
		    case ftColecoROMRLE:
		    case ftPNG:
                multiFile = false;
                break;
        }

		if ((multiFile) && (nOutputCSize > 0)) {
			if ((g_UseMulticolorOnly == 0) || (g_UseHalfMulticolor == 1)) {
				cs=outfile;
				cs+="_C";
				cs.MakeUpper();
				CString csRealName = cs;
				if (csRealName.Right(2).MakeUpper() == "_C") {
					csRealName = csRealName.Left(csRealName.GetLength()-2) + ".TIAC";
				}
				_wfopen_s(&fC, csRealName, _T("wb"));
				if (NULL == fC) {
					fclose(fP);
					AfxMessageBox(_T("Failed to open _C file"));
					return;
				}

                // color file headers
				switch (dlg.m_pOFN->nFilterIndex) {
					case ftTIFILES:	// TIFILES
						DoTIFILES(fC, nOutputCSize);
						break;

					case ftV9T9:	// V9T9
						DoV9T9(fC,rawfile+"_C", nOutputCSize);
						break;

                    case ftRaw:
                    case ftRLE:
		            case ftTIXB:
		            case ftTIXBRLE:
		            case ftMSXSC2:
                    case ftCVPaint:
					case ftCVPowerPaint:
		            case ftColecoROM:
		            case ftColecoROMRLE:
		            case ftPNG:
                        break;
				}

				if (!g_UseMulticolorOnly) {
					if ((g_UseHalfMulticolor) || (g_UsePalette)) {
						cs=outfile;
						cs+="_M";
						cs.MakeUpper();
						CString csRealName = cs;
						if (csRealName.Right(2).MakeUpper() == "_M") {
							csRealName = csRealName.Left(csRealName.GetLength()-2) + ".TIAM";
						}
						_wfopen_s(&fM, csRealName, _T("wb"));
						if (NULL == fM) {
							fclose(fP);
							fclose(fC);
							AfxMessageBox(_T("Failed to open _M file"));
							return;
						}

                        // multi file headers
						switch (dlg.m_pOFN->nFilterIndex) {
							case ftTIFILES:	// TIFILES
								DoTIFILES(fM, nOutputMSize);
								break;

							case ftV9T9:	// V9T9
								DoV9T9(fM,rawfile+"_M", nOutputMSize);
								break;

                            case ftRaw:
                            case ftRLE:
		                    case ftTIXB:
		                    case ftTIXBRLE:
		                    case ftMSXSC2:
                            case ftCVPaint:
							case ftCVPowerPaint:
		                    case ftColecoROM:
		                    case ftColecoROMRLE:
		                    case ftPNG:
                                break;
						}
					}
				}
			}
		}

		// Now process the image
		if (!g_UseMulticolorOnly) {
			nOffset=0;
			for (int y=0; y<192; y+=8) {
				for (int x=0; x<256; x+=8) {
					for (int y2=0; y2<8; y2++) {
						unsigned char *p=buf8+((y+y2)*256+x);
						int bg=0, fg=0;
						int fgcnt=0, bgcnt=0;
						unsigned char c=*p;
						unsigned char outpat=0;
						unsigned char outbit=0x80;
						if (g_UseHalfMulticolor) {
							// extract only the bitmap portion
							c=(c&0xf0)>>4;
						}
						bg=c; fg=0x01;					// set both here in case fg is never set (foreground black by default)
						for (int x2=1; x2<8; x2++) {
							outbit>>=1;
							c=*(p+x2);
							if (g_UseHalfMulticolor) {
								// extract only the bitmap portion
								c=(c&0xf0)>>4;
							}
							if (c!=bg) {
								fg=c;				// should only get set once
								outpat|=outbit;		// this may be many times
								fgcnt++;
							} else {
								bgcnt++;
							}
						}

						// sort the colors - fewest pixels as FG so that we can use
						// BG colors for software sprites in the future, perhaps
						// The exception is monochrome mode, where we have to have
						// 0x1f for color, so black pixels on white background
						if (g_MatchColors < 3) {
							// colors have to be 0x1f - black on white
							// of course, we are pre-remap, so white is 0x00
							if ((fg != 0x01)||(bg == 0x01)) {
								outpat=~outpat;
							}
							fg=0x01;
							bg=0x00;
						} else {
							// do the fewest pixel sort
							if (fgcnt > bgcnt) {
								int x=bg;
								bg=fg;
								fg=x;
								outpat=~outpat;
							}
						}

						// special case for color only, which always needs both
						if (g_UseColorOnly) {
							if (outpat == 0x00) {
								// make both colors background
								fg=bg;
							} else if (outpat == 0xff) {
								// doesn't happen, but just in case
								bg=fg;
							}
							outpat = 0xf0;
						}

						// write pattern table
						pbuf[nOffset]=outpat;
						// remap white and grey in color table
						fg = REMAP_COLOR(fg);
						bg = REMAP_COLOR(bg);
						// write color table
						cbuf[nOffset]=(fg<<4)|(bg);
						// update pointer
						nOffset++;
					}
				}
			}

			// a little more work if we're in half multicolor
			if (g_UseHalfMulticolor) {
				// first, we'll be very hacky and rotate the color and pattern tables here
				char tmpbuf[32*2*8];		// enough buffer space for two rows of pattern tables
			
				// first block - rotate down by one row
				memmove(tmpbuf, &pbuf[(256+32*7)*8], 32*8);
				memmove(&pbuf[(256+32)*8], &pbuf[256*8], 32*7*8);
				memmove(&pbuf[256*8], tmpbuf, 32*8);

				memmove(tmpbuf, &cbuf[(256+32*7)*8], 32*8);
				memmove(&cbuf[(256+32)*8], &cbuf[256*8], 32*7*8);
				memmove(&cbuf[256*8], tmpbuf, 32*8);

				// second block - rotate down by two rows
				memmove(tmpbuf, &pbuf[(512+6*32)*8], 32*2*8);
				memmove(&pbuf[(512+32*2)*8], &pbuf[512*8], 32*6*8);
				memmove(&pbuf[512*8], tmpbuf, 32*2*8);

				memmove(tmpbuf, &cbuf[(512+6*32)*8], 32*2*8);
				memmove(&cbuf[(512+32*2)*8], &cbuf[512*8], 32*6*8);
				memmove(&cbuf[512*8], tmpbuf, 32*2*8);

				// Now we need to convert the multicolor mode data as well
				// For multicolor, each group of four loads two bytes per row (representing two vertical pixels), at the appropriate offset:
				// 
				// - the first in a group loads bytes 0-1
				// - the second in a group loads bytes 2-3
				// - the third in a group loads bytes 4-5
				// - the fourth in a group loads bytes 6-7
				// 
				// Each value appears only three times, so two bytes in each definition are unused. Which two bytes that is varies, though.
				// we'll just run through the screen linearly and calculate where to put each piece of data
				for (int y=0; y<192; y+=8) {		// this represents two bytes and two lines, but 1 character
					for (int x=0; x<256; x+=8) {	// this represents one byte and two columns, but 1 character
						unsigned char *p=buf8 + (y*256) + x;	// starting position in the 8-bit data
					
						// so this 8x8 cell contains four pixels, we'll get them all here
						int c1,c2,c3,c4;
						c1=(*p)&0x0f;			// pix 1 at pointer
						c2=(*(p+4))&0x0f;		// pix 2 to the right
						c3=(*(p+256*4))&0x0f;	// pix 3 down one
						c4=(*(p+256*4+4))&0x0f;	// pix 4 down and to the right

						// remap colors
						c1=REMAP_COLOR(c1);
						c2=REMAP_COLOR(c2);
						c3=REMAP_COLOR(c3);
						c4=REMAP_COLOR(c4);

						// which bitmap group are we in?
						int bgroup = (y/8)/8;			// returns 0-2
						// which multicolor group are we in?
						int mgroup = (y/8)%4;			// returns 0-3
						// calculate base character for group 0
						int nchar = (y/8)*32 + (x/8);	// returns 0-255
						// add bitmap group rotation (wrapping around)
						nchar=(nchar+32*bgroup)&0xff;	// returns 0-255
						// calculate character address
						int add = nchar*8;				// returns 0-2040
						// add multicolor group offset
						add+=2*mgroup;					// returns 0-2046

						// Now we know where it goes, so we can write the data
						mbuf[add] = (c1<<4)|c2;			// first two pixels
						mbuf[add+1] = (c3<<4)|c4;		// second two pixels
					}
				}
			}

			if (g_UsePalette) {
				// F18A palette into the mbuf
				// note color remapping happens here
				memset(mbuf, 0, sizeof(mbuf));
				for (int y=0; y<192; y++) {
					mbuf[(y*16*2)]=0;
					mbuf[(y*16*2)+1]=0;
					for (int n=0; n<15; n++) {
						int c = REMAP_COLOR(n);
						int r=scanlinepal[y][n][0]&0xf0;
						int g=scanlinepal[y][n][1]&0xf0;
						int b=scanlinepal[y][n][2]&0xf0;
						mbuf[(y*16*2)+(c*2)] = r>>4;			// 12 bit format 0000RRRR GGGGBBBB
						mbuf[(y*16*2)+(c*2)+1] = (g)|(b>>4);
					}
					if (!g_UsePerLinePalette) break;
				}
			}
		} else {
			// this is either multicolor or dual multicolor (ie: no bitmap). Use pBuf for multicolor and both for dual
			// This multicolor mode doesn't need anything to be scrambled or shifted, just a straight count through
			for (int y=0; y<192; y+=8) {		// this represents two bytes and two lines, but 1 character
				for (int x=0; x<256; x+=8) {	// this represents one byte and two columns, but 1 character
					unsigned char *p=buf8 + (y*256) + x;	// starting position in the 8-bit data
					
					// so this 8x8 cell contains four pixels, we'll get them all here
					int c1,c2,c3,c4;
					c1=(*p)&0x0f;			// pix 1 at pointer
					c2=(*(p+4))&0x0f;		// pix 2 to the right
					c3=(*(p+256*4))&0x0f;	// pix 3 down one
					c4=(*(p+256*4+4))&0x0f;	// pix 4 down and to the right

					// remap colors
					c1=REMAP_COLOR(c1);
					c2=REMAP_COLOR(c2);
					c3=REMAP_COLOR(c3);
					c4=REMAP_COLOR(c4);

					// SIT should be initialized so that each character is repeated across 4 rows, as
					// per the E/A manual. So, 00-1F 4 times, then 20-3F 4 times, etc through A0-BF 4 times
					// Which character group are we in?
					int cgroup = y/32;				// returns 0-5
					// which offset in the character?
					int coffset = (y-(cgroup*32))/8;// returns 0-3
					// calculate character
					int nchar = (cgroup*0x20)+x/8;		// returns 0x00-0xBF
					// calculate address
					int add = (nchar*8) + (coffset*2);

					// Now we know where it goes, so we can write the data
					pbuf[add] = (c1<<4)|c2;			// first two pixels
					pbuf[add+1] = (c3<<4)|c4;		// second two pixels
				}
			}

			// for dual multicolor, repeat the same thing but into cbuf 
			// hacky almost duplicate code
			if (g_UseHalfMulticolor) {
				for (int y=0; y<192; y+=8) {		// this represents two bytes and two lines, but 1 character
					for (int x=0; x<256; x+=8) {	// this represents one byte and two columns, but 1 character
						unsigned char *p=buf8 + (y*256) + x;	// starting position in the 8-bit data
					
						// so this 8x8 cell contains four pixels, we'll get them all here
						int c1,c2,c3,c4;
						c1=((*p)&0xf0)>>4;			// pix 1 at pointer
						c2=((*(p+4))&0xf0)>>4;		// pix 2 to the right
						c3=((*(p+256*4))&0xf0)>>4;	// pix 3 down one
						c4=((*(p+256*4+4))&0xf0)>>4;// pix 4 down and to the right

						// remap colors
						c1=REMAP_COLOR(c1);
						c2=REMAP_COLOR(c2);
						c3=REMAP_COLOR(c3);
						c4=REMAP_COLOR(c4);

						// SIT should be initialized so that each character is repeated across 4 rows, as
						// per the E/A manual. So, 00-1F 4 times, then 20-3F 4 times, etc through A0-BF 4 times
						// Which character group are we in?
						int cgroup = y/32;				// returns 0-5
						// which offset in the character?
						int coffset = (y-(cgroup*32))/8;// returns 0-3
						// calculate character
						int nchar = (cgroup*0x20)+x/8;		// returns 0x00-0xBF
						// calculate address
						int add = (nchar*8) + (coffset*2);

						// Now we know where it goes, so we can write the data
						cbuf[add] = (c1<<4)|c2;			// first two pixels
						cbuf[add+1] = (c3<<4)|c4;		// second two pixels
					}
				}
			}
		}
		// write out data
		switch (dlg.m_pOFN->nFilterIndex) { 
			case ftTIFILES: // tifiles
			case ftV9T9: // v9t9
			case ftRaw: // raw
				// not RLE - dump the raw data
				fwrite(pbuf, 1, nOutputPSize, fP);				// always at least a pattern table
				if (fC) fwrite(cbuf, 1, nOutputCSize, fC);
				if (fM) fwrite(mbuf, 1, nOutputMSize, fM);
				break;

			case ftRLE:
				// it IS RLE, do a simple compression
				// high bit set - run length, else raw data
				{	
					int sizeC, sizeP, sizeM;
					pRLEP = RLEEncode(pbuf, &sizeP, nOutputPSize);
					
					if (nOutputCSize > 0) {
						pRLEC = RLEEncode(cbuf, &sizeC, nOutputCSize);
					}

					if ((fM)&&(!g_UsePalette)) {
						pRLEM = RLEEncode(mbuf, &sizeM, nOutputMSize);
						if (sizeP+sizeC+sizeM > nOutputPSize+nOutputCSize+nOutputMSize) {
							AfxMessageBox(_T("Warning: RLE file size greater than uncompressed would be. Will save anyway."));
						}
						// write the multicolor file here.
						fwrite(pRLEM, 1, sizeM, fM);
						free(pRLEM);
					} else {
						if (sizeP+sizeC > nOutputPSize+nOutputCSize) {
							AfxMessageBox(_T("Warning: RLE file size greater than uncompressed would be. Will save anyway."));
						}
					}

					// palette is NOT compressed - note palette and half-multicolor are exclusive!
					if ((fM) && (g_UsePalette)) {
						fwrite(mbuf, 1, nOutputMSize, fM);
					}

					// do this part in both cases
					fwrite(pRLEP, 1, sizeP, fP);			// always at least a pattern table
					if (fC) fwrite(pRLEC, 1, sizeC, fC);
					free(pRLEP);
					free(pRLEC);
				}
				break;

			case ftTIXB:
				// if it's XB, we need to patch it into the program (full size version)
				{
					// pattern table first - we read two bytes there for the assumed
                    // CPU RAM location of the image - the address is just there to ensure
                    // that we are looking at the right place and it will be overwritten.
                    // If changing xbtest.h, you need to manually patch these addresses.
					unsigned char *p = 0x3fe+XBWorkBuf;
					if ((*p!=0xcd)||(*(p+1)!=0xe6)) {
						AfxMessageBox(_T("Internal error finding pattern table location."));
					} else {
						UpdateWorkBuf(pbuf, p, nOutputPSize);
					}
					// then color table - same deal, same hand-patched verify bytes
					p = 0x1c2e+XBWorkBuf;
					if ((*p!=0xe5)||(*(p+1)!=0xe6)) {
						AfxMessageBox(_T("Internal error finding color table location."));
					} else {
						if (nOutputCSize == 0) {
							// make a fake one
							memset(cbuf, 0x1F, 6144);
							UpdateWorkBuf(cbuf, p, 6144);
						} else {
							UpdateWorkBuf(cbuf, p, nOutputCSize);
						}
					}

					// now just write out the file
					fwrite(XBWorkBuf, 1, SIZE_OF_XBTEST, fP);
				}
				break;

			case ftTIXBRLE:
				// if it's XB, we need to patch it into the program (RLE version)
				{
					// it IS RLE, do a simple compression
					// high bit set - run length, else raw data
					int sizeC, sizeP;
					pRLEP = RLEEncode(pbuf, &sizeP, nOutputPSize);
					if (nOutputCSize == 0) {
						// make a fake one
						memset(cbuf, 0x1F, 6144);
						pRLEC = RLEEncode(cbuf, &sizeC, 6144);
					} else {
						pRLEC = RLEEncode(cbuf, &sizeC, nOutputCSize);
					}

					if (sizeP+sizeC > nOutputPSize+nOutputCSize) {
						AfxMessageBox(_T("Warning: RLE file size greater than uncompressed would be. CAN NOT PROCEED!"));
						break;
					}

					// figure out where everything is going to go in the program
					// The program and data buffer do not move, everything else shifts
					int nColorTable = 0xfde6 - ((sizeC+1)&0xfffe);		// round up to even number (fde6 = asm program)
					int nPatternTable = nColorTable - ((sizeP+1)&0xfffe);
					int nNewXBOffset = (nPatternTable-0xcde6);			// difference between old start and new (cde6 = pattern)
					nNewXBOffset=((nNewXBOffset-253)/254)*254;			// keep an exact multiple of record size

					// because of the rounding, we need to use nNewXBOffset to get the address of the tables over again
					// We just add to the original locations in memory. The above is still needed because it calculates
					// how much space we NEED to reserve
					nPatternTable = 0xcde6 + nNewXBOffset;
					nColorTable = nPatternTable + ((sizeP+1)&0xfffe);

					// the first part of the copy works as per normal, nothing moves
					// pattern table first
					unsigned char *p = 0x3fe+XBWorkBuf;
					if ((*p!=0xcd)||(*(p+1)!=0xe6)) {
						AfxMessageBox(_T("Internal error finding pattern table location."));
					} else {
						p = UpdateWorkBuf(pRLEP, p, sizeP);
						if (sizeP&0x01) {
							// if odd, add one
							p = UpdateWorkBuf(pRLEP, p, 1);
						}
					}
					// then color table - this one won't line up, we just need to calculate it
					// and hope it doesn't fall on a record boundary
					p = UpdateWorkBuf(pRLEC, p, sizeC);
					if (sizeC&0x01) {
						// if odd, add one
						p = UpdateWorkBuf(pRLEC, p, 1);
					}

					// relocate the BASIC program - to do this we need to change the load information
					// and the update the line number table
					// First calculate the size of the line number table (must fit in one record)
					int nLineStart = XBWorkBuf[0x83]*256 + XBWorkBuf[0x84];
					int nLineEnd = XBWorkBuf[0x85]*256 + XBWorkBuf[0x86];
					int nLineSize = nLineEnd - nLineStart;
					nLineStart += nNewXBOffset;
					nLineEnd += nNewXBOffset;
					int nLineXOR = nLineEnd ^ nLineStart;
					XBWorkBuf[0x83] = nLineStart / 256;
					XBWorkBuf[0x84] = nLineStart % 256;
					XBWorkBuf[0x85] = nLineEnd / 256;
					XBWorkBuf[0x86] = nLineEnd % 256;
					XBWorkBuf[0x87] = nLineXOR / 256;
					XBWorkBuf[0x88] = nLineXOR % 256;

					// update the line number table pointers
					for (int nIdx = 0x181; nIdx<0x181+nLineSize; nIdx+=4) {
//						debug(_T("Relocate line %d...\n"),XBWorkBuf[nIdx]*256+XBWorkBuf[nIdx+1]);
						int nAddress = XBWorkBuf[nIdx+2]*256+XBWorkBuf[nIdx+3];
						nAddress += nNewXBOffset;
						XBWorkBuf[nIdx+2] = nAddress/256;
						XBWorkBuf[nIdx+3] = nAddress%256;
					}

					// fix the comment with correct address information
					char hex[] = "0123456789ABCDEF";
					XBWorkBuf[0x0352]=hex[(nPatternTable&0xf000)>>12];
					XBWorkBuf[0x0353]=hex[(nPatternTable&0xf00)>>8];
					XBWorkBuf[0x0354]=hex[(nPatternTable&0xf0)>>4];
					XBWorkBuf[0x0355]=hex[(nPatternTable&0xf)];
					XBWorkBuf[0x0362]=hex[(nColorTable&0xf000)>>12];
					XBWorkBuf[0x0363]=hex[(nColorTable&0xf00)>>8];
					XBWorkBuf[0x0364]=hex[(nColorTable&0xf0)>>4];
					XBWorkBuf[0x0365]=hex[(nColorTable&0xf)];

					// relocate the assembly program pointers
					// TIAP
					if ((XBWorkBuf[0x34a6]!=0xcd)||(XBWorkBuf[0x34a7]!=0xe6)) {
						printf("Can't find reference to TIAP\n");
					} else {
						XBWorkBuf[0x34a6]=nPatternTable/256;
						XBWorkBuf[0x34a7]=nPatternTable%256;
					}
					// TIAC
					if ((XBWorkBuf[0x34b6]!=0xe5)||(XBWorkBuf[0x34b7]!=0xe6)) {
						printf("Can't find reference to TIAC\n");
					} else {
						XBWorkBuf[0x34b6]=nColorTable/256;
						XBWorkBuf[0x34b7]=nColorTable%256;
					}

					// change the assembly program to use RLE decode
                    // LOADDIR is at >FEDA
                    // LOADRLE is at >FEAA
					if ((XBWorkBuf[0x34b2]!=0xfe)||(XBWorkBuf[0x34b3]!=0xda)) {
						printf("Can't find first reference to LOADDIR\n");
					} else {
						XBWorkBuf[0x34b3]=0xaa;
					}
					if ((XBWorkBuf[0x34c2]!=0xfe)||(XBWorkBuf[0x34c3]!=0xda)) {
						printf("Can't find second reference to LOADDIR\n");
					} else {
						XBWorkBuf[0x34c3]=0xaa;
					}

					// move the executable block down into place
					// it starts at 0x3380 in the current. We have adjusted size
					// so that we don't need to reformat the records, thus
					// we can just use a block move. The /254*2 adds two bytes of
					// offset for each record to help alignment.
                    // (Note: 0x3380 is the start of the /record/, not the code start)
					memmove(&XBWorkBuf[0x3380-nNewXBOffset-(nNewXBOffset/254*2)], &XBWorkBuf[0x3380], SIZE_OF_XBTEST-0x3380);

					// figure out the new record count
					int nRecordCnt = XBWorkBuf[0x0f]*256+XBWorkBuf[0x0e];
					nRecordCnt-=(nNewXBOffset/254);
					XBWorkBuf[0x0f]=nRecordCnt/256;
					XBWorkBuf[0x0e]=nRecordCnt%256;

					// now just write out the file
					fwrite(XBWorkBuf, 1, nRecordCnt*256+128, fP);

					// free data
					free(pRLEP);
					free(pRLEC);
				}
				break;

			case ftMSXSC2:	// MSX SC2
				// MSX format:
				//	 
				// 7 byte header: (per http://www.msx.org/wiki/MSX-BASIC_file_formats)
				// 
				// 0	FE		File identifier
				// 1-2	xx xx	Load address
				// 3-4	xx xx	End address
				// 5-6	xx xx	Boot address (only used for ,R - not pictures)
				// 
				// So in the example I have:
				// 
				// FE			Header
				// 00 00 		Load at >0000
				// 00 38		End at >3800 (little endian CPU!)
				// 00 00		No boot address (it's a picture!)
				// 
				// And the file is >3807 bytes long, with pattern table at >0000 and color table at >2000. The SIT
				// seems to be loaded at >1800 and the sprite table at >1B00 (All sprites at Y = >D1)
				// 
				// According to http://msx.jannone.org/conv/, you can display this file with:
				// 
				// 10 SCREEN 2
				// 20 BLOAD "FILENAME.SC2",S
				// 30 GOTO 30
					
				// we already wrote the header, so first write the pattern table
				fwrite(pbuf, 1, nOutputPSize, fP);
				// next we write a SIT - three copies of 0-256. 
				for (int i1=0; i1<3; i1++) {
					for (int i2=0; i2<256; i2++) {
						fputc(i2, fP);
					}
				}
				// next we need a sprite table with the sprites offscreen - it should be
				// only 128 bytes but we'll just fill the remaining space, which is 1280 bytes
				for (int i1=0; i1<320; i1++) {
					fwrite("\xd1\x00\x00\x00", 1, 4, fP);
				}
				// now write the color table, and we'll be done
				if (nOutputCSize == 0) {
					memset(cbuf, 0x1f, 6144);
					fwrite(cbuf, 1, 6144, fP);
				} else {
					fwrite(cbuf, 1, nOutputCSize, fP);
				}
				break;

            case ftCVPaint:
				// not RLE - dump the raw data - but all to one file
                // first P, then C.
                // as our own extension, we'll add the M data after the C data
				fwrite(pbuf, 1, nOutputPSize, fP);				// always at least a pattern table
				if (nOutputCSize>0) fwrite(cbuf, 1, nOutputCSize, fP);
				if (nOutputMSize>0) fwrite(mbuf, 1, nOutputMSize, fP);	// this never happens?
				break;

			case ftCVPowerPaint:
				// very similar to CVPaint, but not all the data is used
				// the left two columns are unused, as well as the bottom
				// four rows. We're going to save the image from the left,
				// so we need to fake those two unused rows.
				// Note there is a 40k and 80k multi-screen format that uses
				// the opposite order of color/pattern. (40k is 2x2 screens,
				// and 80k is 2x4 screens)
				// pattern table
				for (int idx=0; idx<20*32*8; idx+=32*8) {
					// write 16 bytes of left buffer, pattern
					fwrite("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 1, 16, fP);
					// write one 240 pixel row
					fwrite(&pbuf[idx], 1, 30*8, fP);
				}
				// should always be a color table...?
				if (nOutputCSize>0) {
					for (int idx=0; idx<20*32*8; idx+=32*8) {
						// write 16 bytes of left buffer, color
						fwrite("\xf1\xf1\xf1\xf1\xf1\xf1\xf1\xf1\xf1\xf1\xf1\xf1\xf1\xf1\xf1\xf1", 1, 16, fP);
						// write one 240 pixel row
						fwrite(&cbuf[idx], 1, 30*8, fP);
					}
				} else {
					// write a default color table
					// not sure if this should be inverted...
					for (int idx=0; idx<5*1024; ++idx) {
						fputc(0xf1, fP);
					}
				}
				// there's no good way to deal with the M file here...
				if (nOutputMSize > 0) {
					printf("** warning: powerpaint doesn't support M files...\n");
				}
				break;

			case ftColecoROM:	// colecovision cartridge
			case ftColecoROMRLE:// colecovision RLE cartridge  (not working for some reason...)
				{
					unsigned char buf[32*1024];		// more than enough
					int nSize;						// actual size
					HRSRC hRsrc;
					HGLOBAL hGlob;
					int sizeC, sizeP;//, sizeM;
	
					// get the resource into our work buffer (on the stack. good lord.)
					memset(buf, 0, sizeof(buf));
					hRsrc=FindResource(NULL, MAKEINTRESOURCE(IDR_COLECOROM), _T("ROM"));
					if (hRsrc) {
						int nRealLen=SizeofResource(NULL, hRsrc);

						hGlob=LoadResource(NULL, hRsrc);
						if (NULL != hGlob) {
							char *pData=(char*)LockResource(hGlob);	// no need to unlock per MSDN
							memcpy(buf, pData, nRealLen);
							nSize=nRealLen;
						} else {
							AfxMessageBox(_T("Can't build ROM - internal error 1"));
							return;
						}
					} else {
						AfxMessageBox(_T("Can't build ROM - internal error 2"));
						return;
					}

					// good, next, we hack up the default header a bit
					// First, move the startup code a bit higher
					memcpy(&buf[0xf0], &buf[0x2d], 13);
					// and patch the start address
					buf[0x0a] = 0xf0;

					// again, just fake the color table if needed
					if (nOutputCSize == 0) {
						memset(cbuf, 0x1f, 6144);
						nOutputCSize = 6144;
					}

					// next, set the RLE flag for the embedded code
					buf[0xff]=0;		// not RLE
					if (dlg.m_pOFN->nFilterIndex == ftColecoROMRLE) {
						buf[0xff]=1;	// is RLE
						pRLEP = RLEEncode(pbuf, &sizeP, nOutputPSize);
						pRLEC = RLEEncode(cbuf, &sizeC, nOutputCSize);
					} else {
						sizeP = nOutputPSize;
						sizeC = nOutputCSize;
					}

					// now copy the pattern file
					// put a tag in there
					memcpy((char*)&buf[0xc0], "Convert9918Tursi", 16);

					// Z80 is little endian
					buf[0xe2]=(0x8000+nSize)&0xff;
					buf[0xe3]=((0x8000+nSize)>>8)&0xff;
					memcpy(&buf[nSize], pbuf, sizeP);
					nSize+=sizeP;

					// now repeat for the color table
					// got a file, so set the pointer first
					// Z80 is little endian
					buf[0xe0]=(0x8000+nSize)&0xff;
					buf[0xe1]=((0x8000+nSize)>>8)&0xff;
					memcpy(&buf[nSize], cbuf, sizeC);
					nSize+=sizeC;

					// we've done it, so save
					fwrite(buf, 1, nSize+1, fP);
				}
				break;

            case ftPNG:
                // never hit
                break;
		}

		if (NULL != fC) {
			fclose(fC);
			fC=NULL;
		}
		if (NULL != fP) {
			fclose(fP);
			fP=NULL;
		}
		if (NULL != fM) {
			fclose(fM);
			fM=NULL;
		}
	}
}

void CTIPicViewDlg::OnDropFiles(HDROP hDropInfo) 
{
	wchar_t szFile[1024];

	ZeroMemory(szFile, sizeof(szFile));

	DragQueryFile(hDropInfo, 0, szFile, 1024);
	DragFinish(hDropInfo);

	if (wcslen(szFile)) {
		LaunchMain(2,szFile);
	}
}

void CTIPicViewDlg::OnBnClickedButton3()
{
	bool bShift=false;

	heightoffset--;
	if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
		heightoffset--;		// shift moves by one more
		bShift=true;
	}
	if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
		heightoffset-=3;	// ctrl moves by three more
		if (bShift) {
			// if it was both, add another 4 to make 8
			heightoffset-=4;
		}
	}
	LaunchMain(1,"");
}

void CTIPicViewDlg::OnBnClickedButton5()
{
	bool bShift=false;

	heightoffset++;
	if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
		heightoffset++;		// shift moves by one more
		bShift=true;
	}
	if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
		heightoffset+=3;	// ctrl moves by three more
		if (bShift) {
			// if it was both, add another 4 to make 8
			heightoffset+=4;
		}
	}
	LaunchMain(1,"");
}

void CTIPicViewDlg::EnableItem(int Ctrl) {
	CWnd *pWnd = GetDlgItem(Ctrl);
	if (NULL != pWnd) {
		pWnd->EnableWindow(TRUE);
	}
}
void CTIPicViewDlg::DisableItem(int Ctrl) {
	CWnd *pWnd = GetDlgItem(Ctrl);
	if (NULL != pWnd) {
		pWnd->EnableWindow(FALSE);
	}
}

void CTIPicViewDlg::ResetControls() {
	// set all controls to enabled, then filter in the perceptual button just to centralize that
	// only controls that get enabled/disabled are listed
	static int nControls[] = {
		IDC_EDIT2, IDC_EDIT1, IDC_EDIT3, IDC_EDIT4, IDC_PERPR, IDC_PERPG, IDC_PERPB, IDC_MAXDIFF, IDC_STATICCNT,
		IDC_ERRMODE, IDC_RADIO1, IDC_RADIO2, IDC_COLDIFF, IDC_LUMA
	};

	for (int idx=0; idx<sizeof(nControls)/sizeof(nControls[0]); idx++) {
		EnableItem(nControls[idx]);
	}
	if (!g_Perceptual) {
		DisableItem(IDC_PERPR);
		DisableItem(IDC_PERPG);
		DisableItem(IDC_PERPB);
	} else {
		DisableItem(IDC_LUMA);
	}
}

// Perceptual color
void CTIPicViewDlg::OnBnClickedPercept()
{
	CButton *pCtrl=(CButton*)GetDlgItem(IDC_PERCEPT);
	if (pCtrl) {
		if (pCtrl->GetCheck() == BST_CHECKED) {
			g_Perceptual=true;
		} else {
			g_Perceptual=false;
		}
	}
	ResetControls();
	OnCbnSelchangeCombo1();
}

void CTIPicViewDlg::LaunchMain(int mode, CString pFile) {
	CollectData();

	PIXA=m_pixelA;
	PIXB=m_pixelB;
	PIXC=m_pixelC;
	PIXD=m_pixelD;
	PIXE=m_pixelE;
	PIXF=m_pixelF;
	g_orderSlide = m_nOrderSlide;
    g_ctrlList = m_ctrlList.GetCurSel();
	g_nFilter = m_nFilter;
	g_PowerPaint = m_chkPowerPaint ? 1 : 0;
	g_nPortraitMode = m_nPortraitMode;
	g_AccumulateErrors = (m_ErrMode == 0) ? 0 : 1;
	g_MaxMultiDiff = m_MaxDiff;
	g_MaxColDiff = m_MaxColDiff;
	g_PercepR = m_PercepR / 100.0;
	g_PercepG = m_PercepG / 100.0;
	g_PercepB = m_PercepB / 100.0;
	g_LumaEmphasis = m_Luma / 100.0;
	g_Gamma = m_Gamma / 100.0;
	g_StaticColors = m_StaticCols;
	g_bStaticByPopularity = m_Popularity ? true : false;

	if (InterlockedCompareExchange(&cs, 1, 0) == 1) {
		printf("Error locking - already processing\n");
	} else {
		maincode(mode, pFile, m_nOrderSlide);
		InterlockedExchange(&cs, 0);
	}
}

void CTIPicViewDlg::OnBnClickedReload()
{
	LaunchMain(1,"");
}

void CTIPicViewDlg::OnCbnSelchangeCombo1()
{
	int idx = m_ctrlList.GetCurSel();
	ResetControls();

	switch (idx) {
	case MODE_BITMAP:		// bitmap
		g_UseColorOnly = 0;
		g_UseHalfMulticolor = 0;
		g_UseMulticolorOnly = 0;
		g_UsePerLinePalette = 0;
		g_UsePalette = 0;
		g_GreyPalette = 0;
		g_MatchColors = 15;
		DisableItem(IDC_MAXDIFF);
		DisableItem(IDC_STATICCNT);
		DisableItem(IDC_RADIO1);
		DisableItem(IDC_RADIO2);
		nOutputPSize=6144;
		nOutputCSize=6144;
		nOutputMSize=0;
		break;

	case MODE_GREY_BITMAP:	// greyscale bitmap - this assumes a black and white monitor or saturation turned off
		g_UseColorOnly = 0;
		g_UseHalfMulticolor = 0;
		g_UseMulticolorOnly = 0;
		g_UsePerLinePalette = 0;
		g_UsePalette = 0;
		g_GreyPalette = 1;
		g_MatchColors = 15;
		DisableItem(IDC_MAXDIFF);
		DisableItem(IDC_STATICCNT);
		DisableItem(IDC_RADIO1);
		DisableItem(IDC_RADIO2);
		nOutputPSize=6144;
		nOutputCSize=6144;	// still needs a full color table
		nOutputMSize=0;
		break;

	case MODE_MONO_BITMAP:	// B&W bitmap
		g_UseColorOnly = 0;
		g_UseHalfMulticolor = 0;
		g_UseMulticolorOnly = 0;
		g_UsePerLinePalette = 0;
		g_UsePalette = 0;
		g_GreyPalette = 0;
		g_MatchColors = 2;	// white and black only
		DisableItem(IDC_MAXDIFF);
		DisableItem(IDC_STATICCNT);
		DisableItem(IDC_RADIO1);
		DisableItem(IDC_RADIO2);
		nOutputPSize=6144;
		nOutputCSize=0;		// no color table needed
		nOutputMSize=0;
		break;

	case MODE_MULTICOLOR:		// multicolor
		g_UseColorOnly = 0;
		g_UseHalfMulticolor = 0;
		g_UseMulticolorOnly = 1;
		g_UsePerLinePalette = 0;
		g_UsePalette = 0;
		g_GreyPalette = 0;
		g_MatchColors = 15;
		DisableItem(IDC_EDIT1);
		DisableItem(IDC_EDIT2);
		DisableItem(IDC_EDIT3);
		DisableItem(IDC_EDIT4);
		DisableItem(IDC_ERRMODE);

		DisableItem(IDC_MAXDIFF);
		DisableItem(IDC_STATICCNT);
		DisableItem(IDC_RADIO1);
		DisableItem(IDC_RADIO2);
		nOutputPSize=1536;
		nOutputCSize=0;
		nOutputMSize=0;
		break;

	case MODE_DUAL_MULTICOLOR:		// dual-multicolor
		g_UseColorOnly = 0;
		g_UseHalfMulticolor = 1;
		g_UseMulticolorOnly = 1;
		g_UsePerLinePalette = 0;
		g_UsePalette = 0;
		g_GreyPalette = 0;
		g_MatchColors = 15;
		DisableItem(IDC_EDIT1);
		DisableItem(IDC_EDIT2);
		DisableItem(IDC_EDIT3);
		DisableItem(IDC_EDIT4);
		DisableItem(IDC_ERRMODE);

		DisableItem(IDC_COLDIFF);
		DisableItem(IDC_STATICCNT);
		DisableItem(IDC_RADIO1);
		DisableItem(IDC_RADIO2);
		nOutputPSize=1536;
		nOutputCSize=1536;
		nOutputMSize=0;
		break;

	case MODE_HALF_MULTICOLOR:		// Half-multicolor
		g_UseColorOnly = 0;
		g_UseHalfMulticolor = 1;
		g_UseMulticolorOnly = 0;
		g_UsePerLinePalette = 0;
		g_UsePalette = 0;
		g_GreyPalette = 0;
		g_MatchColors = 15;
		DisableItem(IDC_COLDIFF);
		DisableItem(IDC_STATICCNT);
		DisableItem(IDC_RADIO1);
		DisableItem(IDC_RADIO2);
		nOutputPSize=6144;
		nOutputCSize=6144;
		nOutputMSize=2048;
		break;

	case MODE_COLOR_ONLY:	// bitmap with color table only
		g_UseColorOnly = 1;
		g_UseHalfMulticolor = 0;
		g_UseMulticolorOnly = 0;
		g_UsePerLinePalette = 0;
		g_UsePalette = 0;
		g_GreyPalette = 0;
		g_MatchColors = 15;
		DisableItem(IDC_MAXDIFF);
		DisableItem(IDC_STATICCNT);
		DisableItem(IDC_RADIO1);
		DisableItem(IDC_RADIO2);
		nOutputPSize=6144;
		nOutputCSize=6144;
		nOutputMSize=0;
		break;

	case MODE_F18_PALETTE:		// F18 Palette
		g_UseColorOnly = 0;
		g_UseHalfMulticolor = 0;
		g_UseMulticolorOnly = 0;
		g_UsePerLinePalette = 0;
		g_UsePalette = 1;
		g_GreyPalette = 0;
		g_MatchColors = 15;		// not used with palette
		DisableItem(IDC_COLDIFF);
		DisableItem(IDC_MAXDIFF);
		DisableItem(IDC_STATICCNT);
		nOutputPSize=6144;
		nOutputCSize=6144;
		nOutputMSize=32;
		break;

	case MODE_F18_PALETTE_SCANLINE:		// F18 Palette per scanline
		g_UseColorOnly = 0;
		g_UseHalfMulticolor = 0;
		g_UseMulticolorOnly = 0;
		g_UsePerLinePalette = 1;
		g_UsePalette = 1;
		g_GreyPalette = 0;
		g_MatchColors = 15;				// not used with palette
		DisableItem(IDC_COLDIFF);
		DisableItem(IDC_MAXDIFF);
		nOutputPSize=6144;
		nOutputCSize=6144;
		nOutputMSize=6144;
		break;
	}

}


void CTIPicViewDlg::OnBnClickedPalette()
{
	CButton *pCtrl=(CButton*)GetDlgItem(IDC_PALETTE);
	if (pCtrl) {
		if (pCtrl->GetCheck() == BST_CHECKED) {
			g_bDisplayPalette=true;
		} else {
			g_bDisplayPalette=false;
		}
	}
}

void CTIPicViewDlg::MakeConversionModeValid() {
	nLastValidConversionMode = m_ctrlList.GetCurSel();
}

bool CTIPicViewDlg::ResetToValidConversion() {
	if (nLastValidConversionMode != m_ctrlList.GetCurSel()) {
		if (IDNO == AfxMessageBox(_T("Warning: Image has not been converted since mode was changed -- save in old mode?"), MB_YESNO)) {
			return false;
		}
		m_ctrlList.SetCurSel(nLastValidConversionMode);
		OnCbnSelchangeCombo1();
	}
	return true;
}

// old ErrDistDlg
void CTIPicViewDlg::PrepareData()
{
	CButton *pBtn;
	if (m_Popularity) {
		pBtn = (CButton*)GetDlgItem(IDC_RADIO1);
		if (pBtn) pBtn->SetCheck(BST_UNCHECKED);
		pBtn = (CButton*)GetDlgItem(IDC_RADIO2);
		if (pBtn) pBtn->SetCheck(BST_CHECKED);
	} else {
		pBtn = (CButton*)GetDlgItem(IDC_RADIO1);
		if (pBtn) pBtn->SetCheck(BST_CHECKED);
		pBtn = (CButton*)GetDlgItem(IDC_RADIO2);
		if (pBtn) pBtn->SetCheck(BST_UNCHECKED);
	}

	CButton *pCtrl=(CButton*)GetDlgItem(IDC_PERCEPT);
	if (pCtrl) {
		if (g_Perceptual) {
			pCtrl->SetCheck(BST_CHECKED);
		} else {
			pCtrl->SetCheck(BST_UNCHECKED);
		}
	}

	UpdateData(false);
}

// helper for dither controls
void CTIPicViewDlg::EnableDitherCtrls(bool enable) {
	if (enable) {
		EnableItem(IDC_EDIT1);
		EnableItem(IDC_EDIT2);
		EnableItem(IDC_EDIT3);
		EnableItem(IDC_EDIT4);
		EnableItem(IDC_EDIT5);
		EnableItem(IDC_EDIT6);
		DisableItem(IDC_ORDERSLIDE);
	} else{
		DisableItem(IDC_EDIT1);
		DisableItem(IDC_EDIT2);
		DisableItem(IDC_EDIT3);
		DisableItem(IDC_EDIT4);
		DisableItem(IDC_EDIT5);
		DisableItem(IDC_EDIT6);
		EnableItem(IDC_ORDERSLIDE);
	}
}

// Floyd-Steinberg Dither
void CTIPicViewDlg::OnBnClickedFloyd()
{
	UpdateData(true);
	m_pixelA=3;
	m_pixelB=5;
	m_pixelC=1;
	m_pixelD=7;
	m_pixelE=0;
	m_pixelF=0;
	UpdateData(false);
	untoggleOrdered();

}

// atkinson dither
void CTIPicViewDlg::OnBnClickedAtkinson()
{
	UpdateData(true);
	m_pixelA=2;
	m_pixelB=2;
	m_pixelC=2;
	m_pixelD=2;
	m_pixelE=1;
	m_pixelF=1;
	UpdateData(false);
	untoggleOrdered();
}

void CTIPicViewDlg::OnBnClickedPattern()
{
	UpdateData(true);
	m_pixelA=0;
	m_pixelB=8;
	m_pixelC=0;
	m_pixelD=8;
	m_pixelE=0;
	m_pixelF=0;
	UpdateData(false);
	untoggleOrdered();
}

void CTIPicViewDlg::OnBnClickedDiag()
{
	UpdateData(true);
	m_pixelA=1;
	m_pixelB=3;
	m_pixelC=2;
	m_pixelD=3;
	m_pixelE=1;
	m_pixelF=1;
	UpdateData(false);
	untoggleOrdered();
}

void CTIPicViewDlg::OnBnClickedNodither()
{
	UpdateData(true);
	m_pixelA=0;
	m_pixelB=0;
	m_pixelC=0;
	m_pixelD=0;
	m_pixelE=0;
	m_pixelF=0;
	UpdateData(false);
	untoggleOrdered();
}

void CTIPicViewDlg::untoggleOrdered() 
{
	SendDlgItemMessage(IDC_ORDERED, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_ORDERED2, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_ORDERED3, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_ORDERED4, BM_SETCHECK, BST_UNCHECKED, 0);
	EnableDitherCtrls(TRUE);
	g_OrderedDither = 0;
}

void CTIPicViewDlg::OnBnClickedOrdered()
{
	// this one is a toggle
	SendDlgItemMessage(IDC_ORDERED2, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_ORDERED3, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_ORDERED4, BM_SETCHECK, BST_UNCHECKED, 0);
	if (BST_CHECKED == SendDlgItemMessage(IDC_ORDERED, BM_GETCHECK, 0, 0)) {
		EnableDitherCtrls(FALSE);
		g_OrderedDither = 1;
		g_MapSize = 2;
	} else {
		EnableDitherCtrls(TRUE);
		g_OrderedDither = 0;
	}
}

void CTIPicViewDlg::OnBnClickedOrdered2()
{
	// this one is a toggle but keeps the dither up
	SendDlgItemMessage(IDC_ORDERED, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_ORDERED3, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_ORDERED4, BM_SETCHECK, BST_UNCHECKED, 0);
	if (BST_CHECKED == SendDlgItemMessage(IDC_ORDERED2, BM_GETCHECK, 0, 0)) {
		EnableDitherCtrls(TRUE);
		g_OrderedDither = 2;
		g_MapSize = 2;

		UpdateData(true);
		m_pixelA=1;
		m_pixelB=2;
		m_pixelC=2;
		m_pixelD=2;
		m_pixelE=0;
		m_pixelF=0;
		UpdateData(false);

	} else {
		EnableDitherCtrls(TRUE);
		g_OrderedDither = 0;
	}
}

void CTIPicViewDlg::OnBnClickedOrdered3()
{
	// this one is a toggle
	SendDlgItemMessage(IDC_ORDERED, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_ORDERED2, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_ORDERED4, BM_SETCHECK, BST_UNCHECKED, 0);
	if (BST_CHECKED == SendDlgItemMessage(IDC_ORDERED3, BM_GETCHECK, 0, 0)) {
		EnableDitherCtrls(FALSE);
		g_OrderedDither = 1;
		g_MapSize = 4;
	} else {
		EnableDitherCtrls(TRUE);
		g_OrderedDither = 0;
	}
}

void CTIPicViewDlg::OnBnClickedOrdered4()
{
	// this one is a toggle but keeps the dither up
	SendDlgItemMessage(IDC_ORDERED, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_ORDERED2, BM_SETCHECK, BST_UNCHECKED, 0);
	SendDlgItemMessage(IDC_ORDERED3, BM_SETCHECK, BST_UNCHECKED, 0);
	if (BST_CHECKED == SendDlgItemMessage(IDC_ORDERED4, BM_GETCHECK, 0, 0)) {
		EnableDitherCtrls(TRUE);
		g_OrderedDither = 2;
		g_MapSize = 4;

		UpdateData(true);
		m_pixelA=1;
		m_pixelB=2;
		m_pixelC=2;
		m_pixelD=2;
		m_pixelE=0;
		m_pixelF=0;
		UpdateData(false);

	} else {
		EnableDitherCtrls(TRUE);
		g_OrderedDither = 0;
	}
}

void CTIPicViewDlg::OnBnClickedPerpreset()
{
	UpdateData(true);
	m_PercepR = 30;
	m_PercepG = 52;		// no longer 59 - reduced to make up the blue difference
	m_PercepB = 18;		// no longer 11 - changed to reduce blue in dark areas
	UpdateData(false);

}

void CTIPicViewDlg::CollectData()
{
	CButton *pBtn = (CButton*)GetDlgItem(IDC_RADIO1);
	if (pBtn) {
		if (pBtn->GetCheck() == BST_CHECKED) {
			m_Popularity = 0;
		} else {
			m_Popularity = 1;
		}
	}
	UpdateData(true);
}




void CTIPicViewDlg::OnTimer(UINT_PTR nIDEvent)
{
	CDialog::OnTimer(nIDEvent);

	// check if there's a clipboard image
	if (clipboardRead()) {
		LaunchMain(2, myTmpFile.getfilename());		// image is already loaded to RAM
		return;
	}

	// don't even check if the flag is set - we set it to '2' here
	// maincode will set it to '1'
	if (InterlockedCompareExchange(&cs, 1, 2) != 0) {
		return;
	}

	// check for an updated shared filename - allows multiple instances to all convert the same image
	if (NULL != pSharedPointer) {
		// this isn't very atomic, but, it's never supposed to
		// change after the first time it's set.
		wchar_t buf[256];
		memcpy(buf, pSharedPointer, sizeof(buf));
//		InterlockedExchange((LONG*)pSharedPointer, 0);	// this only works as long as it takes us longer to load the image as the other app needs to scan.
														// TODO: we could do better.

		if (buf[0] != '\0') {
			if (0 != wcscmp(szLastFilename, buf)) {
				// don't bother if it's the same as we already have
				LaunchMain(2, buf);
			}
		}
	}
}
