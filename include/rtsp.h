/*
 * @Author: your name
 * @Date: 2021-06-10 21:21:44
 * @LastEditTime: 2021-06-11 11:41:00
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /rtsp/include/rtsp.h
 */
#pragma once

#include "rtp.h"
#include "H264.h"

#include <cstring>
#include <cstdint>
#include <cstddef>
#include <iostream>

#include <arpa/inet.h>

constexpr uint16_t SERVER_RTSP_PORT = 8554;
constexpr uint16_t SERVER_RTP_PORT = 12345;
constexpr uint16_t SERVER_RTCP_PORT = SERVER_RTP_PORT + 1;
constexpr size_t maxBufferSize = (1 << 21);

class RTSP
{
private:
    H264Parser h264File;
    int servRtspSockfd{-1}, servRtpSockfd{-1}, servRtcpSockfd{-1};
    int cliRtpPort{-1}, cliRtcpPort{-1};
    static int Socket(int domain, int type, int protocol = 0);
    static bool Bind(int sockfd, const char *IP, uint16_t port);
    static bool rtspSockInit(int rtspSockfd, const char *IP, uint16_t port, size_t ListenQueue = 5);

    static void replyCmd_OPTIONS(char *buffer, const size_t bufferLen, const int cseq);
    static void replyCmd_SETUP(char *buffer, const size_t bufferLen, const int cseq, const int clientRTP_Port, const int ssrcNum, const char *sessionID, const int timeout);
    static void replyCmd_PLAY(char *buffer, const size_t bufferLen, const int cseq, const char *sessionID, const int timeout);
    static void replyCmd_HEARTBEAT(char *buffer, const size_t bufferLen, const int cseq, const char *sessionID);
    static void replyCmd_DESCRIBE(char *buffer, const size_t bufferLen, const int cseq, const char *url);

    static char *lineParser(char *src, char *line);

    void serveClient(int clientfd, const sockaddr_in &cliAddr, int rtpFD, int ssrcNum, const char *sessionID, int timeout, float fps);

    static ssize_t pushStream(int sockfd, RTP_Packet &rtpPack, const uint8_t *data, size_t dataSize, const sockaddr *to, uint32_t timeStampStep);

public:
    explicit RTSP(const char *filename);
    ~RTSP();

    void Start(int ssrcNum, const char *sessionID, int timeout, float fps);
};
