#pragma once
#include <memory>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <string>

// don't forget this
#pragma comment(lib, "Ws2_32.lib")
class Buffer : public std::string
{
public:
	Buffer(size_t size = 0) : std::string()
	{
		resize(size, 0);
		memset((void*)data(), 0, this->size());
	}
	Buffer(void* buf, size_t size) : std::string()
	{
		resize(size, 0);
		memcpy((void*)data(), buf, size);
	}
	Buffer(const char* str)
	{
		resize(strlen(str));
		memcpy((void*)c_str(), str, strlen(str));
	}
	~Buffer()
	{
		std::string::~basic_string();
	}

	operator char* () const
	{
		return const_cast<char*>(c_str());
	}
	operator const char* () const
	{
		return c_str();
	}
	operator BYTE* () const
	{
		return (BYTE*)c_str();
	}
	operator void* () const
	{
		return (void*)c_str();
	}
	Buffer& operator=(const char* str)
	{
		std::string::operator=(str);
		return *this;
	}
	void Update(void* buf, size_t size)
	{
		resize(size);
		memcpy((void*)data(), buf, size);
	}
	Buffer& operator<<(const Buffer& data)
	{
		if (this != &data)
		{
			*this += data;
		}
		else
		{
			Buffer tmp = data;
			*this += tmp;
		}
		return *this;
	}

	Buffer& operator<<(const std::string str)
	{
		*this += str;
		return *this;
	}

	Buffer& operator<<(const char *str)
	{
		*this += str;
		return *this;
	}

	Buffer& operator<<(int data)
	{
		char s[16];
		sprintf_s(s, "%d", data);
		*this += s;
		return *this;
	}
	const Buffer& operator>>(USHORT& data) const
	{
		data = (USHORT)atoi(c_str());
		return *this;
	}
};

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

