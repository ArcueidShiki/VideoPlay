#pragma once
#include <atomic>
#include <vector>
#include <mutex>
#include "ThreadWorker.h"
#include <varargs.h>
#include <cstdarg>

void Trace(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	std::string sBuffer;
	sBuffer.resize(1024 * 10);
	vsprintf((char*)(sBuffer.c_str()), fmt, args);
	va_end(args);
}

#ifndef TRACE
#define TRACE Trace
#endif

class CThread
{
public:
	CThread();
	~CThread();
	static void ThreadEntry(void* arg);
	bool Start();
	void UpdateWorker(const ::ThreadWorker &worker = ::ThreadWorker());
private:
	friend class ThreadPool;
	bool IsValid();
	bool IsIdle();
	bool Stop();
	void ThreadWorker();
	HANDLE m_hThread;
	std::atomic<bool> m_aRunning;
	// it can only be trivial copyable, potential memory leak with pointer
	std::atomic<const ::ThreadWorker*> m_pWorker;
};