// VideoRTSPServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
// 1. a TCP based RTSP Server
// 2. a UDP based RTP Server
// 3. Add protocol process
// 4. Add data process

#include "RTSPServer.h"

int main()
{
	RTSPServer server;
	server.Init();
	server.Invoke();
	printf("Press any key to exit\n");
	getchar();
	return 0;
}