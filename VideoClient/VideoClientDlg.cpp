
// VideoClientDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "VideoClient.h"
#include "VideoClientDlg.h"
#include "afxdialogex.h"
#include <vector>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CVideoClientDlg dialog



CVideoClientDlg::CVideoClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_VIDEOCLIENT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVideoClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_PLAY, m_video);
	DDX_Control(pDX, IDC_SLIDER_POS, m_pos);
	DDX_Control(pDX, IDC_SLIDER_VOLUME, m_volume);
	DDX_Control(pDX, IDC_EDIT_URL, m_url);
	DDX_Control(pDX, IDC_BTN_PLAY, m_btnPlay);
}

BEGIN_MESSAGE_MAP(CVideoClientDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BTN_STOP, &CVideoClientDlg::OnBnClickedBtnStop)
	ON_BN_CLICKED(IDC_BTN_PLAY, &CVideoClientDlg::OnBnClickedBtnPlay)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CVideoClientDlg message handlers

BOOL CVideoClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	GetClientRect(&m_lastRect);
	std::vector<CONTROL_ID> controlIds ={
		IDC_EDIT_PLAY,
		IDC_SLIDER_POS,
		IDC_SLIDER_VOLUME,
		IDC_EDIT_URL,
		IDC_BTN_PLAY,
		IDC_BTN_STOP,
		IDC_STATIC,
	};
	for (CONTROL_ID id : controlIds)
	{
		CWnd* pControl = GetDlgItem(id);
		if (pControl)
		{
			CRect rect;
			pControl->GetWindowRect(&rect);
			ScreenToClient(&rect);
			m_controlRects[id] = rect;
		}
	}
	//ShowWindow(SW_MAXIMIZE);

	//ShowWindow(SW_MINIMIZE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CVideoClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

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
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CVideoClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CVideoClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default

	CDialogEx::OnTimer(nIDEvent);
}


void CVideoClientDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

}


void CVideoClientDlg::OnBnClickedBtnStop()
{
}

void CVideoClientDlg::OnBnClickedBtnPlay()
{
	static bool isPlaying = false;
	if (!isPlaying)
	{
		m_btnPlay.SetWindowTextW(L"Play");
		isPlaying = true;
	}
	else
	{
		m_btnPlay.SetWindowTextW(L"Pause");
		isPlaying = false;
	}
}


void CVideoClientDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	if (nType == SIZE_MINIMIZED) return;
	CRect clientRect;
	GetClientRect(&clientRect);
	float xRatio = static_cast<float>(clientRect.Width()) / m_lastRect.Width();
	float yRatio = static_cast<float>(clientRect.Height()) / m_lastRect.Height();

	// 调整所有控件的大小和位置
	for (const auto& pair : m_controlRects)
	{
		int id = pair.first;
		CRect lastRect = pair.second;
		CWnd* pControl = GetDlgItem(id);
		if (pControl)
		{
			CRect newRect;
			newRect.left = static_cast<int>(lastRect.left * xRatio);
			newRect.top = static_cast<int>(lastRect.top * yRatio);
			newRect.right = static_cast<int>(lastRect.right * xRatio);
			newRect.bottom = static_cast<int>(lastRect.bottom * yRatio);
			pControl->MoveWindow(newRect);
		}
	}
}
