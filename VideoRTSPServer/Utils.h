#pragma once
#include <string>
#include <intsafe.h>

class Buffer : public std::string
{
public:
	Buffer& operator=(const Buffer& other)
	{
		if (this != &other)
		{
			std::string::operator=(other);
		}
		return *this;
	}
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
	Buffer& operator=(USHORT data)
	{
		char s[16];
		sprintf_s(s, "%d", data);
		*this += s;
		return *this;
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
		return (BYTE*)const_cast<char*>(c_str());
	}
	operator void* () const
	{
		return (void*)const_cast<char*>(c_str());
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
		strcat_s((char*)c_str(), size() + data.size() + 1, data.c_str());
		return *this;
	}

	Buffer& operator<<(const std::string str)
	{
		*this += str;
		return *this;
	}

	Buffer& operator<<(const char* str)
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
	const Buffer& operator>>(unsigned short& data) const
	{
		data = (unsigned short)atoi(c_str());
		return *this;
	}

	bool operator==(const char* str) const
	{
		return strcmp(c_str(), str) == 0;
	}
};

#ifndef TRACE
#define TRACE Trace
void Trace(const char* fmt, ...);
#endif