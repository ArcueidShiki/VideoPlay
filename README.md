# Configuration:

## Architecture : `x86`

## Include Direcotries:

Choose this: `Properties -> VC Directories -> include direcotries (this way is adding to env path)`

Instead of this: `Properties -> C/C++ additional include directories (this way is add compile arguments)`

`../ThirdParty/vlc/include`

`../ThirdParty/vlc/include/vlc;`

`.`

## Link Static Libraries:

`Properties -> Linker -> Input -> addtional dependencies(static Libraries) ->`

`..\ThirdParty\vlc\lib\libvlc.lib`

`..\ThirdParty\vlc\lib\libvlccore.lib`

## Dynamic Libraries:

`Properties -> Debugging -> Environment ->`

`PATH=$(ProjectDir)..\ThirdParty\vlc\;$(PATH)`

## Output

`Properties -> Advanced -> Use of MFC -> Use of MFC in a Shared DLL / static Library`

## RTSP

**OPTIONS**

```txt
Request:
OPTIONS rtsp://example.com/media.mp4
RTSP/1.0 CSeq: 1
User-Agent: VideoClient/1.0

Response:
RTSP/1.0 200 OK
CSeq: 1
Public: OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN
```

**DESCRIBE**
```txt
Request:
DESCRIBE rtsp://example.com/media.mp4 RTSP/1.0
CSeq: 2
Accept: application/sdp
User-Agent: VideoClient/1.0

Response:
RTSP/1.0 200 OK
CSeq: 2
Content-Base: rtsp://example.com/media.mp4/
Content-Type: application/sdp
Content-Length: 460


v=0
o=- 2890844526 2890842807 IN IP4 127.0.0.1
s=RTSP Session
m=video 0 RTP/AVP 96
a=control:streamid=0
a=rtpmap:96 MP4V-ES/5544
m=audio 0 RTP/AVP 97
a=control:streamid=1
a=rtpmap:97 mpeg4-generic/44100

•	-: Username (omitted).
•	2890844526: Session ID.
•	2890842807: Session version.
•	IN IP4 127.0.0.1: Network type (IN), address type (IP4), and address (127.0.0.1).

```

**SETUP**
```txt
Request:
SETUP rtsp://example.com/media.mp4/streamid=0 RTSP/1.0
CSeq: 3
Transport: RTP/AVP;unicast;client_port=8000-8001
User-Agent: VideoClient/1.0

Response:
RTSP/1.0 200 OK
CSeq: 3
Transport: RTP/AVP;unicast;client_port=8000-8001;server_port=9000-9001
Session: 12345678
```

**PLAY**
```txt
Request:
PLAY rtsp://example.com/media.mp4 RTSP/1.0
CSeq: 4
Session: 12345678
Range: npt=0.000-
User-Agent: VideoClient/1.0

Response:
RTSP/1.0 200 OK
CSeq: 4
Session: 12345678
RTP-Info: url=rtsp://example.com/media.mp4/streamid=0;seq=9810092;rtptime=3450012
```

**TEARDOWN**
```txt
Request:
TEARDOWN rtsp://example.com/media.mp4 RTSP/1.0
CSeq: 5
Session: 12345678
User-Agent: VideoClient/1.0

Response:
RTSP/1.0 200 OK
CSeq: 5
Session: 12345678
```

## SDP(Session Description Protocol)

```txt
•	v=: Protocol version (currently 0).
•	o=: Originator and session identifier (username, session ID, version number, network type, address type, and address).
•	s=: Session name.
•	i=: Session information (optional).
•	u=: URI of description (optional).
•	e=: Email address (optional).
•	p=: Phone number (optional).
•	c=: Connection information (network type, address type, and connection address).
•	b=: Bandwidth information (optional).
•	t=: Time the session is active.
•	r=: Repeat times (optional).
•	z=: Time zone adjustments (optional).
•	k=: Encryption key (optional).
•	a=: Session attribute (optional).
•	m=: Media name and transport address.
•	a=: Media attribute (optional).


v=0
o=- 2890844526 2890842807 IN IP4 127.0.0.1
s=RTSP Session
i=Example of SDP format
u=http://www.example.com
e=contact@example.com
c=IN IP4 192.168.1.1
t=0 0
a=tool:libavformat 58.29.100
m=video 0 RTP/AVP 96
a=rtpmap:96 H264/90000
a=control:streamid=0
m=audio 0 RTP/AVP 97
a=rtpmap:97 mpeg4-generic/44100/2
a=control:streamid=1
a=control:track0
96 for H264
AVP(audio video protocol)
```

## RTP

```txt
•	Version (V): 2 bits, indicates the version of RTP.
•	Padding (P): 1 bit, indicates if there are extra padding bytes at the end of the RTP packet.
•	Extension (X): 1 bit, indicates if there is an extension header.
•	CSRC Count (CC): 4 bits, indicates the number of CSRC identifiers.
•	Marker (M): 1 bit, used to mark significant events in the media stream.
•	Payload Type (PT): 7 bits, identifies the format of the RTP payload.
•	Sequence Number: 16 bits, increments by one for each RTP packet sent.
•	Timestamp: 32 bits, reflects the sampling instant of the first byte in the RTP payload.
•	SSRC: 32 bits, identifies the synchronization source.
•	CSRC: 0 to 15 items, 32 bits each, identifies contributing sources.

•	SSRC: 0x45678901 (assigned by the mixer)
•	CSRC List: [0x12345678, 0x23456789, 0x34567890]
•	Payload: Mixed audio data from Alice, Bob, and Carol
```