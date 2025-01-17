#pragma once
#include <atomic>
#include <vector>
#include <mutex>
#include "ThreadWorker.h"
#include <Windows.h>

class CThread
{
public:
	CThread();
	~CThread();
	static void ThreadEntry(void* arg);
	bool Start();
	bool Stop();
	void UpdateWorker(const ::ThreadWorker &worker = ::ThreadWorker());
private:
	friend class ThreadPool;
	bool IsValid();
	bool IsIdle();
	void ThreadWorker();
	HANDLE m_hThread;
	std::atomic<bool> m_aRunning;
	// it can only be trivial copyable, potential memory leak with pointer
	std::atomic<const ::ThreadWorker*> m_pWorker;
};