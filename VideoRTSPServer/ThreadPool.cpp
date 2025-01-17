#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t size)
{
	m_threads.resize(size);
	for (size_t i = 0; i < size; i++)
	{
		m_threads[i] = new CThread();
	}
}

ThreadPool::~ThreadPool()
{
	StopPool();
	m_threads.clear();
}

bool ThreadPool::Invoke()
{
	bool ret = true;
	for (size_t i = 0; i < m_threads.size(); i++)
	{
		if (!m_threads[i] || !m_threads[i]->Start())
		{
			ret = false;
			break;
		}
	}
	if (!ret) StopPool();
	return ret;
}

void ThreadPool::StopPool()
{
	for (size_t i = 0; i < m_threads.size(); i++)
	{
		if (m_threads[i])
		{
			m_threads[i]->Stop();
			delete m_threads[i];
			m_threads[i] = nullptr;
		}
	}
}

int ThreadPool::DispatchWorker(const ThreadWorker &worker)
{
	if (!worker.IsValid())
	{
		return -1;
	}
	int index = -1;
	m_lock.lock();
	for (size_t i = 0; i < m_threads.size(); i++)
	{
		// TODO optimization
		if (m_threads[i] && m_threads[i]->IsIdle())
		{
			m_threads[i]->UpdateWorker(worker);
			index = int(i);
			break;
		}
	}
	m_lock.unlock();
	return index;
}

bool ThreadPool::CheckThreadValid(size_t index)
{
	if (index < m_threads.size())
	{
		return m_threads[index]->IsValid();
	}
	return false;
}
