#pragma once
#include "Utils.h"
#include "Socket.h"

class RTPHeader
{
public:
	// in datasheet: left is high bit, right is low bit
	// declaration sequence(from low bit to high bit) separate by byte
	// low bit
	BYTE csrc_count : 4;
	BYTE extension : 1;
	BYTE padding : 1;
	BYTE version : 2;
	// hight bit

	BYTE pytype : 7;
	BYTE mark : 1;

	USHORT serial;
	unsigned timestamp;
	unsigned ssrc;
	unsigned csrc[15];
public:
	RTPHeader();
	RTPHeader(const RTPHeader& other);
	RTPHeader& operator=(const RTPHeader& other);
	~RTPHeader() {}
	operator Buffer() const;
};

// A RTP packet better not over 1400 bytes.
// if udp packet is too large
// RTP playload, need slice, if it's too large

/*
1. RTP Header

2.FU Indicator(F + NRI + TYPE) BYTE
	F : 1 bit, 0: normal, 1: error
	NRI : 2 bit, 00 lowest, 0110 medium, 11 highest
	TYPE : 5 bit, 28(0x1C=11100): FU-A(sliced), 29: FU-B

3.FU Header(S + E + R + TYPE) BYTE
	S : 1 bit, 1: start, 0: middle
	E : 1 bit, 1: end, 0: middle
	R : 1 bit, 0: reserved, only be 0
	TYPE : 5 bit, low 5bits of the first byte in a frame

4. RTP Palyload: NALU Data
*/

class RTPFrame
{
public:
	Buffer m_payload;
	RTPHeader m_head;
	operator Buffer();
};

class RTPHelper
{
public:
	RTPHelper() :
		m_timestamp(0),
		m_udp(false)
	{
		m_udp.Bind(Address("0.0.0.0", 50000));
	};
	~RTPHelper() {};
	int SendMediaFrame(RTPFrame& rtpframe, Buffer& frame, const Address& clientAddr);
private:
	int GetFrameHeaderSize(Buffer& frame);
	int SendFrame(const Buffer& frame, const Address &clientAddr);
	DWORD m_timestamp;
	SPSocket m_udp;
};

