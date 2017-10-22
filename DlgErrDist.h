#pragma once


// DlgErrDist dialog

class DlgErrDist : public CDialog
{
	DECLARE_DYNAMIC(DlgErrDist)

public:
	DlgErrDist(CWnd* pParent = NULL);   // standard constructor
	virtual ~DlgErrDist();

// Dialog Data
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedDistDefault();
	DWORD m_pixelD;
	DWORD m_pixelC;
	DWORD m_pixelB;
	DWORD m_pixelA;

	int m_nFilter;
	int m_nPortraitMode; 
	int m_ErrMode;
	int m_MaxDiff;
	int m_PercepR;
	int m_PercepG;
	int m_PercepB;
	int m_Popularity;
	afx_msg void OnBnClickedPerpreset();
	afx_msg void OnBnClickedOk();
	int m_StaticCols;
};
