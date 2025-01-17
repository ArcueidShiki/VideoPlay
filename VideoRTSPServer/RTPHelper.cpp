#include "RTPHelper.h"

#define RTP_HEADER_NO_ERROR 0x0
#define RTP_HEADER_ERROR 0x80
#define RTP_HEADER_IMPORTANT 0x60
#define RTP_HEADER_TYPE_FUA 0x1C
#define RTP_LOW_5_BIT 0x1F
#define RTP_HEADER_START 0x80
#define RTP_HEADER_END 0x40

#define RTP_MAX_PACKET_SIZE 1300
#define FREQUENCY 90000
#define FRAME_RATE 24

int RTPHelper::SendMediaFrame(RTPFrame &rtpframe, Buffer& frame, const Address& clientAddr)
{
    size_t framesz = frame.size();
    int headersz = GetFrameHeaderSize(frame);
    framesz -= headersz;
	BYTE* pFrame = (BYTE*)frame + headersz;

    if (framesz > RTP_MAX_PACKET_SIZE)
    {
		BYTE nalu = pFrame[0] & RTP_LOW_5_BIT;
        size_t count = framesz / RTP_MAX_PACKET_SIZE;
        size_t restsz = framesz % RTP_MAX_PACKET_SIZE;
        for (size_t i = 0; i < count; i++)
        {
            rtpframe.m_payload.resize(RTP_MAX_PACKET_SIZE + 2);
            // 28 0x1C, 0110 0000 | 0001 1100
            ((BYTE*)rtpframe.m_payload)[0] = RTP_HEADER_IMPORTANT | RTP_HEADER_TYPE_FUA;
            ((BYTE*)rtpframe.m_payload)[1] = nalu;
            if (i == 0)
            {
                //start 0x80: 1000 0000, 1F low 5bit of the first byte 0001 1111
                ((BYTE*)rtpframe.m_payload)[1] |= RTP_HEADER_START;
            }
            else if (restsz == 0 && i == count - 1)
            {
                //end 0x80: 0100 0000, 1F low 5bit of the first byte 0001 1111
                ((BYTE*)rtpframe.m_payload)[1] |= RTP_HEADER_END;
            }
			// + 1, becuase pFrame[0] aka nalu is already copied in m_payload[1]
            memcpy((BYTE*)rtpframe.m_payload + 2, pFrame + RTP_MAX_PACKET_SIZE * i + 1, RTP_MAX_PACKET_SIZE);
            SendFrame(rtpframe, clientAddr);
            rtpframe.m_head.serial++;
        }
        if (restsz > 0)
        {
            ((BYTE*)rtpframe.m_payload)[0] = RTP_HEADER_IMPORTANT | RTP_HEADER_TYPE_FUA;
            ((BYTE*)rtpframe.m_payload)[1] = nalu | RTP_HEADER_END;
            memcpy((BYTE*)rtpframe.m_payload + 2, pFrame + RTP_MAX_PACKET_SIZE * count + 1, restsz);
            SendFrame(rtpframe, clientAddr);
            rtpframe.m_head.serial++;
        }
    }
    else
    {
        rtpframe.m_payload.resize(framesz);
		memcpy(rtpframe.m_payload, pFrame, framesz);
        SendFrame(rtpframe, clientAddr);
        rtpframe.m_head.serial++;
    }
    rtpframe.m_head.timestamp += FREQUENCY / FRAME_RATE;
    Sleep(1000 / 30);
    return 0;
}

int RTPHelper::GetFrameHeaderSize(Buffer& frame)
{
    BYTE buf[] = { 0, 0, 0, 1 };
    if (memcmp(frame, buf, 4) == 0) return 4;
    return 3;
}

int RTPHelper::SendFrame(const Buffer& frame, const Address& clientAddr)
{
    int ret = sendto(m_udp, frame, frame.size(), 0, (sockaddr*)clientAddr, clientAddr.size());
    printf("Send Frame ret:%d, size %d, ip:%s, port:%hu\r\n", ret, frame.size(), clientAddr.ip().c_str(), clientAddr.port());
    return 0;
}

RTPHeader::RTPHeader()
{
	memset(this, 0, sizeof(RTPHeader));
    csrc_count = 0;
    extension = 0;
    padding = 0;
    version = 2;
    pytype = 96;
    mark = 0;
    serial = 0;
    timestamp = 0;
    ssrc = 0x98765432;
}

RTPHeader::RTPHeader(const RTPHeader& other)
{
    memset(this, 0, sizeof(RTPHeader));
    memcpy(this, &other, sizeof(RTPHeader));
}

RTPHeader& RTPHeader::operator=(const RTPHeader& other)
{
    if (this != &other)
    {
        memset(this, 0, sizeof(RTPHeader));
        memcpy(this, &other, sizeof(RTPHeader));
    }
    return *this;
}

RTPHeader::operator Buffer() const
{
    RTPHeader header = *this;
    header.serial = htons(header.serial);
    header.timestamp = htonl(header.timestamp);
    header.ssrc = htonl(header.ssrc);
    // this is extremely critical, calculating size
    int size = 12 + 4 * csrc_count;
    Buffer result(size);
    memcpy(result, &header, size);
    return result;
}

RTPFrame::operator Buffer()
{
    Buffer result;
    result += (Buffer)m_head;
    result += m_payload;
    return result;
}
