#include "pch.h"
#include "VideoClientController.h"

CVideoClientController::CVideoClientController()
{
	m_dlg.m_controller = this;
}

CVideoClientController::~CVideoClientController()
{
}

int CVideoClientController::Init(CWnd** pWnd)
{
	*pWnd = &m_dlg;
	return 0;
}

int CVideoClientController::Invoke()
{
	INT_PTR nResponse = m_dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "Warning: dialog creation failed, so application is terminating unexpectedly.\n");
		TRACE(traceAppMsg, 0, "Warning: if you are using MFC controls on the dialog, you cannot #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
	}
	return nResponse;
}

int CVideoClientController::SetMedia(const std::string& strUrl)
{
	return m_vlc.SetMedia(strUrl);
}

float CVideoClientController::VideoCtrl(EVlcCommand cmd)
{
	switch (cmd)
	{
		case VLC_PLAY: return (float)m_vlc.Play();
		case VLC_PAUSE: return (float)m_vlc.Pause();
		case VLC_STOP: return (float)m_vlc.Stop();
		case VLC_GET_POSITION: return m_vlc.GetPosition();
		case VLC_SET_VOLUME: return (float)m_vlc.GetVolume();
		case VLC_GET_TOTAL: return m_vlc.GetTotal();
		default:break;
	}
	return -1.0f;
}

void CVideoClientController::SetPosition(float pos)
{
	m_vlc.SetPosition(pos);
}

int CVideoClientController::SetWnd(HWND hWnd)
{
	return m_vlc.SetHwnd(hWnd);
}

void CVideoClientController::SetVolume(int vol)
{
	return m_vlc.SetVolume(vol);
}

VlcSize CVideoClientController::GetMediaInfo()
{
	return m_vlc.GetMediaInfo();
}

std::string CVideoClientController::Unicode2Utf8(const std::wstring& wstr)
{
	return m_vlc.Unicode2Utf8(wstr);
}
