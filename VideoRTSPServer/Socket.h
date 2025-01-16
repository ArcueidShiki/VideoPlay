#pragma once
#include <WinSock2.h>

class Socket
{
public:
	Socket(bool bIsTCP = true)
	{
		m_socket = socket(PF_INET, bIsTCP ? SOCK_STREAM : SOCK_DGRAM, 0);
	}
	Socket(SOCKET s)
	{
		m_socket = s;
	}
	void Close()
	{
		if (m_socket != INVALID_SOCKET)
		{
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
		}
	}
	~Socket()
	{
		Close();
	}
	operator SOCKET()
	{
		return m_socket;
	}
private:
	SOCKET m_socket;
};

class SPSocket
{
public:
	SPSocket(bool bIsTCP = true)
	{
		m_socket = std::make_shared<Socket>(bIsTCP);
	}

	~SPSocket()
	{
		m_socket.reset();
	}
	operator SOCKET()
	{
		return (SOCKET)*m_socket;
	}
	SPSocket(const SPSocket& other)
	{
		// TODO problems
		m_socket = std::make_shared <Socket>(other);
	}
	SPSocket& operator=(const SPSocket& other)
	{
		if (this != &other)
		{
			m_socket = other.m_socket;
		}
		return *this;
	}
private:
	std::shared_ptr<Socket> m_socket;
};

class SocketInitializer
{
public:
	SocketInitializer()
	{
		WSAData wsa;
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		{
			throw "WSAStartup failed!";
		}
	}
	~SocketInitializer()
	{
		WSACleanup();
	}
};

