#pragma once
#include <memory>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>
#include "Utils.h"

class Address
{
public:
	Address()
	{
		memset(&m_addr, 0, sizeof(m_addr));
		m_port = -1;
		m_addr.sin_family = AF_INET;
	}
	Address(const Address& other)
	{
		m_strIP = other.m_strIP;
		m_port = other.m_port;
		m_addr = other.m_addr;
	}
	Address(const std::string& strIP, USHORT port)
	{
		m_strIP = strIP;
		m_port = port;
		m_addr.sin_port = htons(m_port);
		inet_pton(AF_INET, m_strIP.c_str(), &m_addr.sin_addr);
	}
	Address& operator=(const Address& other)
	{
		if (this != &other)
		{
			m_strIP = other.m_strIP;
			m_port = other.m_port;
			m_addr = other.m_addr;
		}
		return *this;
	}
	~Address() {}
	void Update(const std::string& strIP, USHORT port)
	{
		m_strIP = strIP;
		m_port = port;
		m_addr.sin_port = htons(m_port);
		inet_pton(AF_INET, m_strIP.c_str(), &m_addr.sin_addr);
	}
	operator sockaddr* () const
	{
		return (sockaddr*)&m_addr;
	}
	operator sockaddr_in* ()
	{
		return &m_addr;
	}
	const sockaddr* GetAddr() const
	{
		return (sockaddr*)&m_addr;
	}
	int size() const { return sizeof(sockaddr_in); }
	Address Setoport(USHORT port)
	{
		m_port = port;
		m_addr.sin_port = htons(port);
		return *this;
	}
private:
	std::string m_strIP;
	USHORT m_port;
	sockaddr_in m_addr;
};

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
		m_isTCP = bIsTCP;
	}

	~SPSocket()
	{
		m_socket.reset();
	}
	operator SOCKET() const
	{
		return (SOCKET)*m_socket;
	}
	SPSocket(const SPSocket& other)
	{
		m_socket = std::make_shared<Socket>((SOCKET)other);
		m_isTCP = other.m_isTCP;
	}
	SPSocket& operator=(const SPSocket& other)
	{
		if (this != &other)
		{
			m_socket = other.m_socket;
			m_isTCP = other.m_isTCP;
		}
		return *this;
	}
	SPSocket(SOCKET socket, bool isTCP = true)
	{
		m_socket = std::make_shared<Socket>(socket);
		m_isTCP = isTCP;
	}
	int Bind(const Address& addr)
	{
		if (m_socket == nullptr)
		{
			m_socket = std::make_shared<Socket>(m_isTCP);
		}
		return bind(*m_socket, addr, addr.size());
	}
	int Listen(int backlog = 5)
	{
		return listen(*m_socket, backlog);
	}

	SPSocket Accept(Address &addr)
	{
		int len = addr.size();
		if (m_socket == nullptr) return SPSocket(INVALID_SOCKET);
		SOCKET server = *m_socket;
		if (server == INVALID_SOCKET) return SPSocket(INVALID_SOCKET);
		SOCKET s = accept(*m_socket, addr, &len);
		return SPSocket(s, m_isTCP);
	}

	int Connect(const Address& addr)
	{
		return connect(*m_socket, addr, addr.size());
	}
	void Close()
	{
		// potential double reset
		m_socket.reset();
	}

	int Recv(Buffer &buffer)
	{
		return recv(*m_socket, buffer, buffer.size(), 0);
	}

	int Send(const Buffer& buffer)
	{
		printf("Send:\n%s\n", buffer.c_str());
		size_t index = 0;
		char* pData = buffer;
		while (index < buffer.size())
		{
			int ret = send(*m_socket, pData + index, buffer.size() - index, 0);
			if (ret < 0)
			{
				return ret;
			}
			if (ret == 0)
			{
				break;
			}
			index += ret;
		}
		return index;
	}
private:
	std::shared_ptr<Socket> m_socket;
	bool m_isTCP;
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

