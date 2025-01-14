
// VideoClientDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "VideoClient.h"
#include "VideoClientDlg.h"
#include "afxdialogex.h"
#include "VideoClientController.h"
#include <vector>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CVideoClientDlg dialog

CVideoClientDlg::CVideoClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_VIDEOCLIENT_DIALOG, pParent)
	, m_volStr(_T(""))
	, m_timeStr(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_isPlaying = false;
}

void CVideoClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_PLAY, m_video);
	DDX_Control(pDX, IDC_SLIDER_VOLUME, m_volume);
	DDX_Control(pDX, IDC_EDIT_URL, m_url);
	DDX_Control(pDX, IDC_BTN_PLAY, m_btnPlay);
	DDX_Control(pDX, IDC_SLIDER_POS, m_pos);
	DDX_Text(pDX, IDC_STATIC_VOLUME, m_volStr);
	DDX_Text(pDX, IDC_STATIC_TIME, m_timeStr);
}

BEGIN_MESSAGE_MAP(CVideoClientDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BTN_STOP, &CVideoClientDlg::OnBnClickedBtnStop)
	ON_BN_CLICKED(IDC_BTN_PLAY, &CVideoClientDlg::OnBnClickedBtnPlay)
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER_POS, &CVideoClientDlg::OnNMReleasedcaptureSliderPos)
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
		IDC_SLIDER_VOLUME,
		IDC_EDIT_URL,
		IDC_BTN_PLAY,
		IDC_BTN_STOP,
		IDC_STATIC_MEDIA_LOCATION,
		IDC_STATIC_VOLUME,
		IDC_STATIC_TIME,
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
	SetTimer(0, 500, NULL);
	m_pos.SetRange(0, 1);
	m_pos.SetPos(0);
	m_volume.SetRange(0, 100);
	m_volume.SetPos(100);
	m_volume.SetTicFreq(20);
	SetDlgItemText(IDC_STATIC_VOLUME, L"100%");
	SetDlgItemText(IDC_STATIC_TIME, L"00:00:00/00:00:00");
	m_controller->SetWnd(m_video.GetSafeHwnd());
	m_url.SetWindowTextW(_T("file:///E:\\GitHubBase\\projects\\VideoPlay\\resources\\¡¾FSF¡¿¼ª¶ûÙ¤ÃÀÊ² vs ¶÷Ææ¶¼.mp4"));
	m_btnPlay.SetWindowTextW(L"Play");

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
	if (nIDEvent == 0)
	{
		float pos = m_controller->VideoCtrl(VLC_GET_POSITION);
		if (pos != -1.0f)
		{
			if (m_total <= 0.0f)
			{
				m_total = m_controller->VideoCtrl(VLC_GET_TOTAL);
			}
			// initialize m_pos slider bar
			if (m_pos.GetRangeMax() <= 1)
			{
				m_pos.SetRange(0, (int)m_total);
			}
			// TODO 100% display may have issues
			m_timeStr.Format(L"%f%%", pos / m_total * 100);
			m_pos.SetPos(static_cast<int>(pos * m_total));
			UpdateData(FALSE);
		}
	}
	CDialogEx::OnTimer(nIDEvent);
}


void CVideoClientDlg::OnDestroy()
{
	CDialogEx::OnDestroy();
	KillTimer(0);
}


void CVideoClientDlg::OnBnClickedBtnStop()
{
	m_isPlaying = false;
	m_btnPlay.SetWindowTextW(L"Play");
	m_controller->VideoCtrl(VLC_STOP);
}

void CVideoClientDlg::OnBnClickedBtnPlay()
{
	if (!m_isPlaying)
	{
		CString url;
		m_url.GetWindowText(url);
		m_controller->SetMedia(m_controller->Unicode2Utf8(url.GetString()));
		m_controller->VideoCtrl(VLC_PLAY);
		m_isPlaying = true;
		m_btnPlay.SetWindowTextW(L"Pause");
	}
	else
	{
		m_controller->VideoCtrl(VLC_PAUSE);
		m_isPlaying = false;
		m_btnPlay.SetWindowTextW(L"Play");
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
	m_controller->VideoCtrl(VLC_UPDATE_SIZE);
}


void CVideoClientDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (pScrollBar == (CScrollBar*)&m_pos)
	{
		float percent = m_pos.GetPos() / m_total;
		m_timeStr.Format(L"%f%%", percent * 100);
		m_controller->SetPosition(percent);
		UpdateData(FALSE);
		//SetDlgItemText(IDC_STATIC_TIME, posStr);
		//TRACE(L"Position nSBCode:[%u], nPos:[%u], m_pos.GetPos():[%d], pScrollBar:[%p] Scroll\n", nSBCode, nPos, m_pos.GetPos(), pScrollBar);
	}
	else if (pScrollBar == (CScrollBar*)&m_volume)
	{
		int volume = m_volume.GetPos();
		m_volStr.Format(L"%d%%", volume);
		SetDlgItemText(IDC_STATIC_VOLUME, m_volStr);
		m_controller->SetVolume(volume);
		//TRACE(L"Volume nSBCode:[%u], nPos:[%u], pScrollBar:[%p] Scroll\n", nSBCode, nPos, pScrollBar);
	}
	else {
		CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
	}
}

void CVideoClientDlg::OnNMReleasedcaptureSliderPos(NMHDR* pNMHDR, LRESULT* pResult)
{
	m_controller->VideoCtrl(VLC_PLAY);
	m_btnPlay.SetWindowTextW(L"Pause");
	*pResult = 0;
}
