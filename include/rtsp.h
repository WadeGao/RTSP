#pragma once
#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <cstdint>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ctime>
#include <cassert>
#include <sys/stat.h>
#include <fcntl.h>

#include "rtp.h"
#include "H264.h"

constexpr uint16_t SERVER_RTSP_PORT = 8554;
constexpr uint16_t SERVER_RTP_PORT = 12345;
constexpr uint16_t SERVER_RTCP_PORT = SERVER_RTP_PORT + 1;
constexpr size_t maxBufferSize = (1 << 21);
constexpr size_t maxFileNameLen = 128;

class RTSP
{
private:
    char filename[maxFileNameLen]{0};
    H264Parser h264File;
    int servRtspSockfd, servRtpSockfd, servRtcpSockfd;
    int cliRtpPort, cliRtcpPort;
    int Socket(int __domain, int __type, int __protocol = 0);
    bool Bind(int sockfd, const char *IP, const uint16_t port);
    bool rtspSockInit(int rtspSockfd, const char *IP, const uint16_t port, const size_t ListenQueue = 5);

    void replyCmd_OPTIONS(char *buffer, const size_t bufferLen, const int cseq)
    {
        snprintf(buffer, bufferLen, "RTSP/1.0 200 OK\r\nCseq: %d\r\nPublic: OPTIONS, DESCRIBE, SETUP, PLAY\r\n\r\n", cseq);
    }
    void replyCmd_SETUP(char *buffer, const size_t bufferLen, const int cseq, const int clientRTP_Port, const int ssrcNum, const char *sessionID, const int timeout)
    {
        snprintf(buffer, bufferLen, "RTSP/1.0 200 OK\r\nCseq: %d\r\nTransport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d;ssrc=%d;mode=play\r\nSession: %s; timeout=%d\r\n\r\n",
                 cseq, clientRTP_Port, clientRTP_Port + 1, SERVER_RTP_PORT, SERVER_RTCP_PORT, ssrcNum, sessionID, timeout);
    }
    void replyCmd_PLAY(char *buffer, const size_t bufferLen, const int cseq, const char *sessionID, const int timeout)
    {
        snprintf(buffer, bufferLen, "RTSP/1.0 200 OK\r\nCseq: %d\r\nRange: npt=0.000-\r\nSession: %s; timeout=%d\r\n\r\n", cseq, sessionID, timeout);
    }
    void replyCmd_HEARTBEAT(char *buffer, const size_t bufferLen, const int cseq, const char *sessionID)
    {
        snprintf(buffer, bufferLen, "RTSP/1.0 200 OK\r\nCseq: %d\r\nRange: npt=0.000-\r\nHeartbeat: %s; \r\n\r\n", cseq, sessionID);
    }
    void replyCmd_DESCRIBE(char *buffer, const size_t bufferLen, const int cseq, const char *url)
    {
        char ip[100]{0};
        char sdp[500]{0};
        sscanf(url, "rtsp://%[^:]:", ip);
        snprintf(sdp, sizeof(sdp), "v=0\r\no=- 9%ld 1 IN IP4 %s\r\nt=0 0\r\na=control:*\r\nm=video 0 RTP/AVP 96\r\na=rtpmap:96 H264/90000\r\na=control:track0\r\n", time(nullptr), ip);
        snprintf(buffer, bufferLen, "RTSP/1.0 200 OK\r\nCseq: %d\r\nContent-Base: %s\r\nContent-type: application/sdp\r\nContent-length: %ld\r\n\r\n%s", cseq, url, strlen(sdp), sdp);
    }
    char *lineParser(char *src, char *line);
    void serveClient(int clientfd, const sockaddr_in &cliAddr, const int ssrcNum, const char *sessionID, const int timeout, const float fps);

public:
    RTSP(const char *filename);
    ~RTSP();

    void Start(const int ssrcNum, const char *sessionID, const int timeout, const float fps);
};