#include "pch.h"
#include "Vlc.h"
#include <codecvt>

VlcSize::VlcSize(int width, int height)
	: nWidth(width)
	, nHeight(height)
{
}

VlcSize::VlcSize(const VlcSize& other)
{
	nWidth = other.nWidth;
	nHeight = other.nHeight;
}

VlcSize& VlcSize::operator=(const VlcSize& other)
{
	if (this != &other)
	{
		nWidth = other.nWidth;
		nHeight = other.nHeight;
	}
	return *this;
}

CVlc::CVlc()
{
	m_instance = libvlc_new(0, NULL);
	m_media = NULL;
	m_player = NULL;
}

CVlc::~CVlc()
{
	if (m_player)
	{
		libvlc_media_player_release(m_player);
		m_player = NULL;
	}
	if (m_media)
	{
		libvlc_media_release(m_media);
		m_media = NULL;
	}
	if (m_instance)
	{
		libvlc_release(m_instance);
		m_instance = NULL;
	}
}

int CVlc::SetMedia(const std::string& strUrl)
{
	if (!m_instance) return -1;
	m_media = libvlc_media_new_location(m_instance, strUrl.c_str());
	if (!m_media) return -2;
	m_player = libvlc_media_player_new_from_media(m_media);
	if (!m_player) return -3;
	return 0;
}

int CVlc::SetHwnd(HWND hWnd)
{
	if (!IsValid()) return -1;
	libvlc_media_player_set_hwnd(m_player, hWnd);
	return 0;
}

bool CVlc::IsValid()
{
	return 	m_instance && m_media && m_player;
}

int CVlc::Play()
{
	if (!IsValid()) return -1;
	return libvlc_media_player_play(m_player);
}

int CVlc::Pause()
{
	if (!IsValid()) return -1;
	libvlc_media_player_pause(m_player);
	return 0;
}

int CVlc::Stop()
{
	if (!IsValid()) return -1;
	libvlc_media_player_stop(m_player);
	return 0;
}

float CVlc::GetPosition()
{
	if (!IsValid()) return -1;
	return libvlc_media_player_get_position(m_player);
}

void CVlc::SetPosition(float pos)
{
	if (!IsValid()) return;
	libvlc_media_player_set_position(m_player, pos);
}

int CVlc::GetVolume()
{
	if (!IsValid()) return -1;
	return libvlc_audio_get_volume(m_player);
}

void CVlc::SetVolume(int vol)
{
	if (!IsValid()) return;
	libvlc_audio_set_volume(m_player, vol);
}

VlcSize CVlc::GetMediaInfo()
{
	if (!IsValid()) return VlcSize(-1, -1);
	int width = libvlc_video_get_width(m_player);
	int height = libvlc_video_get_height(m_player);
	return VlcSize(width, height);
}

std::string CVlc::Unicode2Utf8(const std::wstring& wstr)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
	return conv.to_bytes(wstr);
}
