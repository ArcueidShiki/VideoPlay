#pragma once
#include <Windows.h>
#include <atomic>
#include <vector>
#include <mutex>
#include "ThreadWorker.h"
#include <varargs.h>
#include <cstdarg>

#ifndef TRACE
#define TRACE Trace
void Trace(const char* fmt, ...);
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