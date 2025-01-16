#include "RTSPServer.h"

RTSPServer::RTSPServer() :
	m_socket(true),
	m_status(ServerStatus::UNINITIALIZED),
	m_pool(10)
{
	m_threadMain.UpdateWorker(::ThreadWorker(this, (FUNC)&RTSPServer::ThreadWorker));
}

int RTSPServer::Init(const std::string& strIP, USHORT port)
{
	m_addr.Update(strIP, port);
	m_socket.Bind(m_addr);
	m_socket.Listen();
	return 0;
}

int RTSPServer::Invoke()
{
	m_threadMain.Start();
	return 0;
}

void RTSPServer::Stop()
{
	m_socket.Close();
	m_pool.StopPool();
}

RTSPServer::~RTSPServer()
{
	Stop();
}

/**
* @return 0 success continue, -1 failed, other warnning
*/
int RTSPServer::ThreadWorker()
{
	Address client_addr;
	SPSocket client_sock = m_socket.Accept(client_addr);
	if (client_sock != INVALID_SOCKET)
	{
		m_qClientSockets.PushBack(client_sock);
		m_pool.DispatchWorker(::ThreadWorker(this, (FUNC)&RTSPServer::ThreadSession));
	}
	return 0;
}

int RTSPServer::ThreadSession()
{
	// TODO Get Request, Parse Request, Make Response, Send Response
	// Session passed
	SPSocket client_sock;
	Buffer buffer(1024 * 16);
	int len = client_sock.Recv(buffer);
	if (len <= 0)
	{
		// TODO clear client_sock
		return -1;
	}
	buffer.resize(len);
	RTSPRequest request = ParseRequest(buffer);
	RTSPResponse response = MakeResponse(request);

	client_sock.Send(response.toBuffer());
	return 0;
}

RTSPRequest RTSPServer::ParseRequest(const std::string& data)
{
	return RTSPRequest();
}

RTSPResponse RTSPServer::MakeResponse(RTSPRequest& request)
{
	return RTSPResponse();
}
