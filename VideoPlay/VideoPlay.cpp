// VideoPlay.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vlc.h>

#define VIDEO_PATH "..\\resources\\Login_Movie.mp4"

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
        libvlc_time_t time = libvlc_media_player_get_length(player);
        printf("%02lld:%02lld:%02lld\r\n", time / 3600, (time / 60) % 60, time % 60);
        int height = libvlc_video_get_height(player);
        int width = libvlc_video_get_width(player);
        printf("Width = %d, Height = %d\r\n", width, height);
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
 