#pragma once
#include "Socket.h"
#include "ThreadPool.h"
#include "Queue.h"
#include <string>
#include <map>
#include <rpc.h>

enum class ServerStatus
{
	UNINITIALIZED = 0,
	INITIALIZED,
	RUNNING,
	STOPPED
};

enum class RTSPMethod
{
	UINITIALIZED = -1,
	OPTIONS = 0,
	DESCRIBE,
	SETUP,
	PLAY,
	TEARDOWN
};

class RTSPRequest
{
public:
	RTSPRequest();
	RTSPRequest(const RTSPRequest& other);
	RTSPRequest& operator=(const RTSPRequest& other);
	void SetMethod(const Buffer& method);
	void SetUrl(const Buffer& url);
	void SetSequence(const Buffer& seq);
	void SetClientPort(USHORT ports[]);
	void SetSession(const Buffer& session);
	const RTSPMethod method() const { return m_method; }
	const Buffer& url() const { return m_url; }
	const Buffer& session() const { return m_session; }
	const Buffer& sequence() const { return m_seq; }
	const Buffer& port(int index = 0) const { return index ? m_client_port[1] : m_client_port[0]; }
	~RTSPRequest();
private:
	RTSPMethod m_method;
	Buffer m_seq;
	Buffer m_url;
	Buffer m_session;
	Buffer m_client_port[2];
};

class RTSPResponse
{
public:
	RTSPResponse();
	RTSPResponse(const RTSPResponse& other);
	RTSPResponse& operator=(const RTSPResponse& other);
	void SetOptions(const Buffer& options);
	void SetSequence(const Buffer& seq);
	void SetSdp(const Buffer& sdp);
	void SetClientPorts(const Buffer& port0, const Buffer& port1);
	void SetServerPorts(const Buffer& port0, const Buffer& port1);
	void SetSession(const Buffer& session);
	~RTSPResponse() {}
	Buffer toBuffer();
private:
	RTSPMethod m_method;
	Buffer m_sdp;
	Buffer m_options;
	USHORT m_client_port[2];
	USHORT m_server_port[2];
	Buffer m_session;
	Buffer m_seq;
};

class RTSPSession
{
public:
	RTSPSession();
	RTSPSession(const SPSocket& client);
	RTSPSession(const RTSPSession& other);
	RTSPSession& operator=(const RTSPSession& other);
	int ResponseRequest();
	~RTSPSession() {}
private:
	Buffer Pick();
	Buffer PickLine(Buffer& buffer);
	RTSPRequest ParseRequest(const Buffer& buffer);
	RTSPResponse Response(const RTSPRequest& request);
private:
	std::string m_id;
	SPSocket m_client;
};

class RTSPServer : public ThreadFuncBase
{
public:
	RTSPServer();
	int Init(const std::string& strIP = "0.0.0.0", USHORT port = 8554);
	int Invoke();
	void Stop();
	~RTSPServer();
protected:
	int ThreadWorker();
	int ThreadSession();
private:
	ServerStatus m_status;
	Address m_addr;
	CThread m_threadMain;
	SPSocket m_socket;
	ThreadPool m_pool;
	static SocketInitializer m_initializer;
	CQueue<RTSPSession> m_qSessions;
};

