// VideoPlay.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vlc.h>
#include <codecvt>
#include <Windows.h>
#include <conio.h>

#define VIDEO_PATH "..\\resources\\Login_Movie.mp4"

std::string Unicode2Utf8(const std::wstring& wstr)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(wstr);
}

std::wstring Utf82Unicode(const std::string& str)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.from_bytes(str);
}

std::string UnicodeToUtf8(const std::wstring& wstr)
{
    std::string str;
    str.reserve(wstr.size() * 4);
    ::WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wstr.size(), (LPSTR)str.c_str(), str.length(), NULL, NULL);
    return str;
}

std::wstring Utf8ToUnicode(const std::string& str)
{
	std::wstring wstr;
	wstr.reserve(str.size());
	::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.size(), (LPWSTR)wstr.c_str(), wstr.length());
	return wstr;
}



int main()
{
    int argc = 1;
    char* argv[2];
    argv[0] = (char*)"--ignore-config";
    libvlc_instance_t* vlc_ins = libvlc_new(argc, argv);
    libvlc_media_t* media = libvlc_media_new_path(vlc_ins, VIDEO_PATH);
    //media = libvlc_media_new_location(vlc_ins, "file:///"VIDEO_PATH);
    libvlc_media_player_t* player = libvlc_media_player_new_from_media(media);
    do
    {
        int ret = libvlc_media_player_play(player);
        if (ret == -1)
        {
            printf("err found!\r\n");
            break;
        }
        Sleep(300);
        int vol = libvlc_audio_get_volume(player);
		printf("Volume = %d\r\n", vol);
        libvlc_audio_set_volume(player, 10);
        libvlc_time_t time = libvlc_media_player_get_length(player);
        printf("%02lld:%02lld:%02lld.%03lld\r\n", time / 3600000, (time / 60000) % 60, (time / 1000) % 60, time % 1000);
        int height = libvlc_video_get_height(player);
        int width = libvlc_video_get_width(player);
        printf("Width = %d, Height = %d\r\n", width, height);
        while (!_kbhit())
        {
            printf("Playing... %08.2f%%\r", 100 * libvlc_media_player_get_position(player));
            Sleep(500);
        }
        getchar();
        libvlc_media_player_pause(player);
        getchar();
        libvlc_media_player_play(player);
        getchar();
        libvlc_media_player_stop(player);
    } while (0);
    libvlc_media_player_release(player);
    libvlc_media_release(media);
    libvlc_release(vlc_ins);
    return 0;
}
 