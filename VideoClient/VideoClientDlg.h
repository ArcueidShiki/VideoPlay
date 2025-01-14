
// VideoClientDlg.h : header file
//

#pragma once
#include <map>
#include <string>
class CVideoClientController;

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
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedBtnStop();
	afx_msg void OnBnClickedBtnPlay();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

protected:
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	CVideoClientController* m_controller;

protected:
	CString m_volStr;
	CString m_timeStr;
	CButton m_btnPlay;
	CEdit m_video;
	CSliderCtrl m_pos;
	CSliderCtrl m_volume;
	CEdit m_url;
	HICON m_hIcon;
	CRect m_lastRect;
	float m_total;
	bool m_isPlaying;
	typedef int CONTROL_ID;
	std::map<CONTROL_ID, CRect> m_controlRects;
public:
	afx_msg void OnNMReleasedcaptureSliderPos(NMHDR* pNMHDR, LRESULT* pResult);
};
