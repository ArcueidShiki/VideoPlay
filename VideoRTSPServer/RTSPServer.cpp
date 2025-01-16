#include "RTSPServer.h"
// uuid
#pragma comment(lib, "Rpcrt4.lib")
constexpr const char *OPTIONS = "OPTIONS";
constexpr const char* DESCRIBE = "DESCRIBE";
constexpr const char* SETUP = "SETUP";
constexpr const char* PLAY = "PLAY";
constexpr const char* PAUSE = "PAUSE";
constexpr const char* TEARDOWN = "TEARDOWN";

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
		RTSPSession session(client_sock);
		m_qSessions.PushBack(session);
		m_pool.DispatchWorker(::ThreadWorker(this, (FUNC)&RTSPServer::ThreadSession));
	}
	return 0;
}

int RTSPServer::ThreadSession()
{
	RTSPSession session;
	if (m_qSessions.PopFront(session))
	{
		return session.ResponseRequest();
	}
	return 0;
}

RTSPSession::RTSPSession()
{
	UUID uuid;
	UuidCreate(&uuid);
	m_id.resize(8);
	sprintf_s((char*)m_id.c_str(), m_id.size(), "08%d", uuid.Data1);
}

RTSPSession::RTSPSession(const SPSocket& client)
{
	m_client = client;
	UUID uuid;
	UuidCreate(&uuid);
	m_id.resize(8);
	sprintf_s((char*)m_id.c_str(), m_id.size(), "08%d", uuid.Data1);
}

RTSPSession::RTSPSession(const RTSPSession& other)
{
	m_id = other.m_id;
	m_client = other.m_client;
}

RTSPSession& RTSPSession::operator=(const RTSPSession& other)
{
	if (this != &other)
	{
		m_id = other.m_id;
		m_client = other.m_client;
	}
	return *this;
}

int RTSPSession::ResponseRequest()
{
	Buffer buffer = Pick();
	if (buffer.size() <= 0) return -1;

	RTSPRequest request = ParseRequest(buffer);
	if (request.method() == RTSPMethod::UINITIALIZED)
	{
		TRACE("Error at: [%s]\r\n", buffer.c_str());
		return -2;
	}

	RTSPResponse response = Response(request);
	int ret = m_client.Send(response.toBuffer());
	if (ret < 0) return ret;
	return ret;
}

Buffer RTSPSession::Pick()
{
	Buffer result;
	Buffer buf(1);
	int n_recv = 1;
	while (n_recv > 0)
	{
		n_recv = m_client.Recv(buf);
		if (n_recv > 0)
		{
			result += buf;
			if (result.size() >= 4)
			{
				// a RTSP packet end
				if (result.find("\r\n\r\n", result.size() - 4) != std::string::npos)
				{
					break;
#if 0
					// Optionally, handle the body if Content-Length is specified
					size_t content_length = 0;
					size_t pos = result.find("Content-Length:");
					if (pos != std::string::npos)
					{
						pos += 15; // Move past "Content-Length:"
						while (pos < result.size() && isspace(result[pos])) ++pos;
						while (pos < result.size() && isdigit(result[pos]))
						{
							content_length = content_length * 10 + (result[pos] - '0');
							++pos;
						}
					}

					// If there is a body, ensure we have read the entire body
					size_t header_end = result.find("\r\n\r\n") + 4;
					if (result.size() >= header_end + content_length)
					{
						break;
					}
#endif
				}
			}

		}
	}
	return result;
}

Buffer RTSPSession::PickLine(Buffer& buffer)
{
	Buffer result, tmp;
	if (buffer.size() <= 0) return result;
	size_t i = 0;
	for (; i < buffer.size(); i++)
	{
		result += buffer.at(i);
		if (buffer.at(i) == '\n') break;
	}
	if (i < buffer.size() - 1)
	{
		buffer = buffer.substr(i + 1).c_str();
	}
	else
	{
		buffer.clear();
	}
	return result;
}

RTSPRequest RTSPSession::ParseRequest(const Buffer& buffer)
{
	Buffer data = buffer;
	RTSPRequest request;
	Buffer line = PickLine(data);
	Buffer method(32), url(1024), version(16), seq(64);
	int n_args = 3;
	if (sscanf_s(line.c_str(), "%31s %1023s %15s\r\n", (char*)method, 32, (char*)url, 1024, (char*)version, 16) < n_args)
	{
		TRACE("Error at: [%s]\r\n", line.c_str());
		return request;
	}

	line = PickLine(data);
	if (sscanf_s(line.c_str(), "CSeq: %63s\r\n", (char*)seq, 64) < 1)
	{
		TRACE("Error at: [%s]\r\n", line.c_str());
		return request;
	}

	request.SetMethod(method);
	request.SetUrl(url);
	request.SetSequence(seq);

	if (strcmp(method, OPTIONS) == 0)
	{
		return request;
	}
	else if (strcmp(method, DESCRIBE) == 0)
	{
		return request;
	}
	else if (strcmp(method, SETUP) == 0)
	{
		line = PickLine(data);
		USHORT ports[2] = { 0, 0 };
		if (sscanf_s(line.c_str(), "Transport: RTP/AVP;unicast;client_port=%hu-%hu\r\n", ports, ports + 1) != 2)
		{
			TRACE("Error at: [%s]\r\n", line.c_str());
			return request;
		}
		request.SetClientPort(ports);
	}
	else if (strcmp(method, PLAY) == 0 || strcmp(method, TEARDOWN) == 0)
	{
		line = PickLine(data);
		Buffer session(64);
		if (sscanf_s(line.c_str(), "Session: %63s\r\n", (char*)session, 64) < 1)
		{
			TRACE("Error at: [%s]\r\n", line.c_str());
			return request;
		}
		request.SetSession(session);
		return request;
	}

	return request;
}

RTSPResponse RTSPSession::Response(const RTSPRequest& request)
{
	RTSPResponse response;
	response.SetSequence(request.sequence());
	if (request.session().size() > 0)
	{
		response.SetSession(request.session());
	}
	switch (request.method())
	{
		case RTSPMethod::OPTIONS: response.SetOptions("Public: OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN"); break;
		case RTSPMethod::DESCRIBE:
		{
			Buffer sdp;
			sdp << "v=0\r\n"
				<< "o=- "
				<< m_id
				<< "1 IN IP4 127.0.0.1"
				<< "t= 0 0\r\n"
				<< "a=control:*\r\n"
				<< "m=video 0 RTP/AVP 96\r\n"
				<< "a=rtpmap:96 H264/90000\r\n"
				<< "a=control:track0\r\n";
			response.SetSdp(sdp);
		}
		break;
		case RTSPMethod::SETUP:
		{
			response.SetClientPorts(request.port(0), request.port(1));
			response.SetServerPorts("50000", "50001");
			//"Transport: RTP/AVP;unicast;client_port=8000-8001;server_port=9000-9001";
		}
		break;
		case RTSPMethod::PLAY: break;
		case RTSPMethod::TEARDOWN: break;
	}
	return RTSPResponse();
}

RTSPRequest::RTSPRequest()
{
	m_method = RTSPMethod::UINITIALIZED;
}

RTSPRequest::RTSPRequest(const RTSPRequest& other)
{
	m_method = other.m_method;
	m_seq = other.m_seq;
	m_url = other.m_url;
	m_session = other.m_session;
	m_client_port[0] = other.m_client_port[0];
	m_client_port[1] = other.m_client_port[1];
}

RTSPRequest& RTSPRequest::operator=(const RTSPRequest& other)
{
	if (this != &other)
	{
		m_method = other.m_method;
		m_seq = other.m_seq;
		m_url = other.m_url;
		m_session = other.m_session;
		m_client_port[0] = other.m_client_port[0];
		m_client_port[1] = other.m_client_port[1];
	}
	return *this;
}

void RTSPRequest::SetMethod(const Buffer& method)
{
	if (method == OPTIONS) m_method = RTSPMethod::OPTIONS;
	else if (method == DESCRIBE) m_method = RTSPMethod::DESCRIBE;
	else if (method == SETUP) m_method = RTSPMethod::SETUP;
	else if (method == PLAY) m_method = RTSPMethod::PLAY;
	else if (method == TEARDOWN) m_method = RTSPMethod::TEARDOWN;
}

void RTSPRequest::SetUrl(const Buffer& url)
{
	m_url = url;
}

void RTSPRequest::SetSequence(const Buffer& seq)
{
	m_seq = seq;
}

void RTSPRequest::SetClientPort(USHORT ports[])
{
	m_client_port[0] = ports[0];
	m_client_port[1] = ports[1];
}

void RTSPRequest::SetSession(const Buffer& session)
{
	m_session = session;
}

RTSPRequest::~RTSPRequest()
{
	m_method = RTSPMethod::UINITIALIZED;
}

RTSPResponse::RTSPResponse()
{
	m_method = RTSPMethod::UINITIALIZED;
}

RTSPResponse::RTSPResponse(const RTSPResponse& other)
{
	m_sdp = other.m_sdp;
	m_seq = other.m_seq;
	m_method = other.m_method;
	m_options = other.m_options;
	m_session = other.m_session;
	m_client_port[0] = other.m_client_port[0];
	m_client_port[1] = other.m_client_port[1];
	m_server_port[0] = other.m_server_port[0];
	m_server_port[1] = other.m_server_port[1];
}

RTSPResponse& RTSPResponse::operator=(const RTSPResponse& other)
{
	if (this != &other)
	{
		m_sdp = other.m_sdp;
		m_seq = other.m_seq;
		m_method = other.m_method;
		m_options = other.m_options;
		m_session = other.m_session;
		m_client_port[0] = other.m_client_port[0];
		m_client_port[1] = other.m_client_port[1];
		m_server_port[0] = other.m_server_port[0];
		m_server_port[1] = other.m_server_port[1];
	}
	return *this;
}

void RTSPResponse::SetOptions(const Buffer& options)
{
	m_options = options;
}

void RTSPResponse::SetSequence(const Buffer& seq)
{
	m_seq = seq;
}

void RTSPResponse::SetSdp(const Buffer& sdp)
{
	m_sdp = sdp;
}

void RTSPResponse::SetClientPorts(const Buffer& port0, const Buffer& port1)
{
	port0 >> m_client_port[0];
	port1 >> m_client_port[1];
}

void RTSPResponse::SetServerPorts(const Buffer& port0, const Buffer& port1)
{
	port0 >> m_server_port[0];
	port1 >> m_server_port[1];
}

void RTSPResponse::SetSession(const Buffer& session)
{
	m_session = session;
}

Buffer RTSPResponse::toBuffer()
{
	Buffer result;
	result << "RTSP/1.0 200 OK\r\n"
		<< "CSeq: " << m_seq << "\r\n";

	switch (m_method)
	{
		case RTSPMethod::UINITIALIZED:break;
		case RTSPMethod::OPTIONS:
		{
			result << m_options << "\r\n\r\n";
		}
		break;
		case RTSPMethod::DESCRIBE:
		{
			result << "Content-Base: 127.0.0.1\r\n"
				// lower case?
				<< "Content-type: application/sdp\r\n"
				<< "Content-length: " << m_sdp.size() << "\r\n\r\n"
				<< m_sdp;
		}
			break;
		case RTSPMethod::SETUP:
		{
			result << "Transport: RTP/AVP;unicast;client_port="
				<< m_client_port[0] << "-" << m_client_port[1]
				<< ";server_port="
				<< m_server_port[0] << "-" << m_server_port[1] << "\r\n"
				<< "Session: " << m_session << "\r\n\r\n";
		}
			break;
		case RTSPMethod::PLAY:
		{
			result << "Range: npt=0.000-\r\n"
				<< "Session: " << m_session << "\r\n\r\n";

		}
			break;
		case RTSPMethod::TEARDOWN:
		{
			result << "Session: " << m_session << "\r\n\r\n";
		}
		break;
		default:break;
	}
	return result;
}
