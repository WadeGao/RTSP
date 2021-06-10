#pragma once
#include <arpa/inet.h>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "H264.h"
#include "rtp.h"

constexpr uint16_t SERVER_RTSP_PORT = 8554;
constexpr uint16_t SERVER_RTP_PORT = 12345;
constexpr uint16_t SERVER_RTCP_PORT = SERVER_RTP_PORT + 1;
constexpr size_t maxBufferSize = (1 << 21);
//constexpr size_t maxFileNameLen = 128;

class RTSP
{
private:
    //char filename[maxFileNameLen]{0};
    H264Parser h264File;
    int servRtspSockfd{-1}, servRtpSockfd{-1}, servRtcpSockfd{-1};
    int cliRtpPort{-1}, cliRtcpPort{-1};
    static int Socket(int __domain, int __type, int __protocol = 0);
    static bool Bind(int sockfd, const char *IP, uint16_t port);
    static bool rtspSockInit(int rtspSockfd, const char *IP, uint16_t port, size_t ListenQueue = 5);

    static void replyCmd_OPTIONS(char *buffer, const size_t bufferLen, const int cseq)
    {
        snprintf(buffer, bufferLen, "RTSP/1.0 200 OK\r\nCseq: %d\r\nPublic: OPTIONS, DESCRIBE, SETUP, PLAY\r\n\r\n", cseq);
    }
    static void replyCmd_SETUP(char *buffer, const size_t bufferLen, const int cseq, const int clientRTP_Port, const int ssrcNum, const char *sessionID, const int timeout)
    {
        snprintf(buffer, bufferLen, "RTSP/1.0 200 OK\r\nCseq: %d\r\nTransport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d;ssrc=%d;mode=play\r\nSession: %s; timeout=%d\r\n\r\n",
                 cseq, clientRTP_Port, clientRTP_Port + 1, SERVER_RTP_PORT, SERVER_RTCP_PORT, ssrcNum, sessionID, timeout);
    }
    static void replyCmd_PLAY(char *buffer, const size_t bufferLen, const int cseq, const char *sessionID, const int timeout)
    {
        snprintf(buffer, bufferLen, "RTSP/1.0 200 OK\r\nCseq: %d\r\nRange: npt=0.000-\r\nSession: %s; timeout=%d\r\n\r\n", cseq, sessionID, timeout);
    }
    static void replyCmd_HEARTBEAT(char *buffer, const size_t bufferLen, const int cseq, const char *sessionID)
    {
        snprintf(buffer, bufferLen, "RTSP/1.0 200 OK\r\nCseq: %d\r\nRange: npt=0.000-\r\nHeartbeat: %s; \r\n\r\n", cseq, sessionID);
    }
    static void replyCmd_DESCRIBE(char *buffer, const size_t bufferLen, const int cseq, const char *url)
    {
        char ip[100]{0};
        char sdp[500]{0};
        sscanf(url, "rtsp://%[^:]:", ip);
        snprintf(sdp, sizeof(sdp), "v=0\r\no=- 9%ld 1 IN IP4 %s\r\nt=0 0\r\na=control:*\r\nm=video 0 RTP/AVP 96\r\na=rtpmap:96 H264/90000\r\na=control:track0\r\n", time(nullptr), ip);
        snprintf(buffer, bufferLen, "RTSP/1.0 200 OK\r\nCseq: %d\r\nContent-Base: %s\r\nContent-type: application/sdp\r\nContent-length: %ld\r\n\r\n%s", cseq, url, strlen(sdp), sdp);
    }
    static char *lineParser(char *src, char *line);
    void serveClient(int clientfd, const sockaddr_in &cliAddr, int rtpFD, int ssrcNum, const char *sessionID, int timeout, float fps);

public:
    explicit RTSP(const char *filename);
    ~RTSP();

    void Start(int ssrcNum, const char *sessionID, int timeout, float fps);
};