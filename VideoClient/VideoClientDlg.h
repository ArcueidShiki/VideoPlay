
// VideoClientDlg.h : header file
//

#pragma once
#include <map>
#include <string>
// CVideoClientDlg dialog
class CVideoClientDlg : public CDialogEx
{
// Construction
public:
	CVideoClientDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VIDEOCLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
	

protected:
	HICON m_hIcon;
	CRect m_lastRect;
	typedef int CONTROL_ID;
	std::map<CONTROL_ID, CRect> m_controlRects;
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedBtnStop();
	CEdit m_video;
	CSliderCtrl m_pos;
	CSliderCtrl m_volume;
	CEdit m_url;
	afx_msg void OnBnClickedBtnPlay();
	CButton m_btnPlay;
	afx_msg void OnSize(UINT nType, int cx, int cy);
};
