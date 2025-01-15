#pragma once
#include <string>
#include <vlc.h>

class VlcSize
{
public:
	int nWidth;
	int nHeight;
	VlcSize(int width = 0, int height = 0);
	VlcSize(const VlcSize& other);
	VlcSize& operator=(const VlcSize& other);
};
class CVlc
{
public:
	CVlc();
	~CVlc();
	// strUrl : unicode(wbs/wchar_t) to utf8 string(mbs)
	int SetMedia(const std::string& strUrl);
#ifdef WIN32
	int SetHwnd(HWND hWnd);
#endif
	bool IsValid();
	int Play();
	int Pause();
	int Stop();
	float GetPosition();
	void SetPosition(float pos);
	int GetVolume();
	void SetVolume(int vol);
	float GetTotal();
	VlcSize GetMediaInfo();
	std::string Unicode2Utf8(const std::wstring& wstr);
protected:
	libvlc_instance_t* m_instance;
	libvlc_media_t* m_media;
	libvlc_media_player_t* m_player;
	std::string m_url;
	CRect m_rect;
#ifdef WIN32
	HWND m_hWnd;
#endif
};

