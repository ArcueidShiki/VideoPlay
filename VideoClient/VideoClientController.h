#pragma once
#include "Vlc.h"
#include "VideoClientDlg.h"

enum EVlcCommand
{
	VLC_PLAY,
	VLC_PAUSE,
	VLC_STOP,
	VLC_GET_POSITION,
	VLC_SET_VOLUME,
};

class CVideoClientController
{
public:
	CVideoClientController();
	~CVideoClientController();
	int Init(CWnd** pWnd);
	int Invoke();
	int SetMedia(const std::string& strUrl); // unicode(wbs/wchar_t) to utf8 string(mbs)
	float VideoCtrl(EVlcCommand cmd);
	void SetPosition(float pos);
	int SetWnd(HWND hWnd);
	void SetVolume(int vol);
	VlcSize GetMediaInfo();
	std::string Unicode2Utf8(const std::wstring& wstr);
protected:
	CVlc m_vlc;
	CVideoClientDlg m_dlg;
};

