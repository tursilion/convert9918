// DlgErrDist.cpp : implementation file
//

#include "stdafx.h"
#include "TIPicView.h"
#include "DlgErrDist.h"


// DlgErrDist dialog

IMPLEMENT_DYNAMIC(DlgErrDist, CDialog)

DlgErrDist::DlgErrDist(CWnd* pParent /*=NULL*/)
	: CDialog(DlgErrDist::IDD, pParent)
	, m_pixelD(0)
	, m_pixelC(0) 
	, m_pixelB(0)
	, m_pixelA(0)
	, m_nFilter(4)
	, m_nPortraitMode(0)
	, m_ErrMode(0)
	, m_MaxDiff(0)
	, m_PercepR(0)
	, m_PercepG(0)
	, m_PercepB(0)
	, m_StaticCols(4)
	, m_Popularity(0)
{

}

DlgErrDist::~DlgErrDist()
{
}

void DlgErrDist::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_pixelD);
	DDV_MinMaxUInt(pDX, m_pixelD, 0, 16);
	DDX_Text(pDX, IDC_EDIT2, m_pixelC);
	DDV_MinMaxUInt(pDX, m_pixelC, 0, 16);
	DDX_Text(pDX, IDC_EDIT3, m_pixelB);
	DDV_MinMaxUInt(pDX, m_pixelB, 0, 16);
	DDX_Text(pDX, IDC_EDIT4, m_pixelA);
	DDV_MinMaxUInt(pDX, m_pixelA, 0, 16);
	DDX_CBIndex(pDX, IDC_FILTER, m_nFilter);
	DDX_CBIndex(pDX, IDC_PORTRAIT, m_nPortraitMode);
	DDX_CBIndex(pDX, IDC_ERRMODE, m_ErrMode);
	DDX_Text(pDX, IDC_MAXDIFF, m_MaxDiff);
	DDV_MinMaxInt(pDX, m_MaxDiff, 1, 100);
	DDX_Text(pDX, IDC_PERPR, m_PercepR);
	DDV_MinMaxInt(pDX, m_PercepR, 0, 100);
	DDX_Text(pDX, IDC_PERPG, m_PercepG);
	DDV_MinMaxInt(pDX, m_PercepG, 0, 100);
	DDX_Text(pDX, IDC_PERPB, m_PercepB);
	DDV_MinMaxInt(pDX, m_PercepB, 0, 100);
	DDX_Text(pDX, IDC_STATICCNT, m_StaticCols);
	DDV_MinMaxInt(pDX, m_StaticCols, 0, 14);
}


BEGIN_MESSAGE_MAP(DlgErrDist, CDialog)
	ON_BN_CLICKED(IDC_DISTDEFAULT, &DlgErrDist::OnBnClickedDistDefault)
	ON_BN_CLICKED(IDC_PERPRESET, &DlgErrDist::OnBnClickedPerpreset)
	ON_BN_CLICKED(IDOK, &DlgErrDist::OnBnClickedOk)
END_MESSAGE_MAP()


// DlgErrDist message handlers

BOOL DlgErrDist::OnInitDialog()
{
	CDialog::OnInitDialog();
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

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void DlgErrDist::OnBnClickedDistDefault()
{
	m_pixelA=3;
	m_pixelB=5;
	m_pixelC=1;
	m_pixelD=7;
	UpdateData(false);
}

void DlgErrDist::OnBnClickedPerpreset()
{
	m_PercepR = 30;
	m_PercepG = 52;		// no longer 59 - reduced to make up the blue difference
	m_PercepB = 18;		// no longer 11 - changed to reduce blue in dark areas
	UpdateData(false);

}


void DlgErrDist::OnBnClickedOk()
{
	CButton *pBtn = (CButton*)GetDlgItem(IDC_RADIO1);
	if (pBtn) {
		if (pBtn->GetCheck() == BST_CHECKED) {
			m_Popularity = 0;
		} else {
			m_Popularity = 1;
		}
	}
	CDialog::OnOK();
}
