// TIPicViewDlg.h : header file
//

#include "afxwin.h"
#include "afxcmn.h"
#if !defined(AFX_TIPICVIEWDLG_H__B6982AC5_ECCF_44C3_B525_9A2ADEE846F2__INCLUDED_)
#define AFX_TIPICVIEWDLG_H__B6982AC5_ECCF_44C3_B525_9A2ADEE846F2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CTIPicViewDlg dialog

class CTIPicViewDlg : public CDialog
{
// Construction
public:
	CTIPicViewDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CTIPicViewDlg)
	enum { IDD = IDD_TIPICVIEW_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTIPicViewDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

	unsigned char *RLEEncode(unsigned char *cbuf, int *nSize, int nInSize);

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CTIPicViewDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnButton1();
	afx_msg void OnButton2();
	afx_msg void OnCheck1();
	afx_msg void OnRnd();
	afx_msg void OnClose();
	afx_msg void OnButton4();
	afx_msg void OnDoubleclickedRnd();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedPercept();
	CComboBox m_ctrlList;
	afx_msg void OnBnClickedReload();
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnBnClickedPalette();

	void MakeConversionModeValid();
	bool ResetToValidConversion();
	void ResetControls();
	void EnableItem(int Ctrl);
	void DisableItem(int Ctrl);
	void LaunchMain(int mode, char *pFile);	// wrapper for maincode()

	// old ErrDistDlg
	virtual void PrepareData();
	void EnableDitherCtrls(bool enable);
	void untoggleOrdered();
	afx_msg void OnBnClickedFloyd();
	afx_msg void OnBnClickedPerpreset();
	afx_msg void OnBnClickedPattern();
	afx_msg void OnBnClickedNodither();
	afx_msg void OnBnClickedAtkinson();
	afx_msg void CollectData();

	DWORD m_pixelF;
	DWORD m_pixelE;
	DWORD m_pixelD;
	DWORD m_pixelC;
	DWORD m_pixelB;
	DWORD m_pixelA;

	int m_nFilter;
	int m_nPortraitMode; 
	int m_ErrMode;
	int m_MaxDiff;
	int m_MaxColDiff;
	int m_PercepR;
	int m_PercepG;
	int m_PercepB;
	int m_Popularity;
	int m_StaticCols;
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	// psuedo mutex used to track state that may or may not be on different threads
	// This code is getting pretty crappy over the years!
	volatile LONG cs;
	int m_Luma;
	int m_Gamma;
	afx_msg void OnBnClickedOrdered();
	afx_msg void OnBnClickedOrdered2();
	int m_nOrderSlide;
	CSliderCtrl ctrlOrderSlide;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TIPICVIEWDLG_H__B6982AC5_ECCF_44C3_B525_9A2ADEE846F2__INCLUDED_)
