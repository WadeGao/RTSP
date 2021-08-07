/*
 * @Author: your name
 * @Date: 2021-06-10 21:21:44
 * @LastEditTime: 2021-08-07 11:38:34
 * @LastEditors: Wade
 * @Description: In User Settings Edit
 * @FilePath: /rtsp/include/rtsp.h
 */
#pragma once

#include "H264.h"
#include "rtp.h"

#include <cstddef>
#include <cstdint>

constexpr uint16_t SERVER_RTSP_PORT = 8554;
constexpr uint16_t SERVER_RTP_PORT = 12345;
constexpr uint16_t SERVER_RTCP_PORT = SERVER_RTP_PORT + 1;

class RTSP
{
private:
    H264Parser h264_file;
    int server_rtsp_sock_fd{-1}, server_rtp_sock_fd{-1}, server_rtcp_sock_fd{-1};
    int client_rtp_port{-1}, client_rtcp_port{-1};
    static int Socket(int domain, int type, int protocol = 0);
    static bool Bind(int sockfd, const char *IP, uint16_t port);
    static bool rtsp_sock_init(int rtspSockfd, const char *IP, uint16_t port, int64_t ListenQueue = 5);

    static void replyCmd_OPTIONS(char *buffer, const int64_t bufferLen, const int cseq);
    static void replyCmd_SETUP(char *buffer, const int64_t bufferLen, const int cseq, const int clientRTP_Port, const int ssrcNum, const char *sessionID, const int timeout);
    static void replyCmd_PLAY(char *buffer, const int64_t bufferLen, const int cseq, const char *sessionID, const int timeout);
    static void replyCmd_HEARTBEAT(char *buffer, const int64_t bufferLen, const int cseq, const char *sessionID);
    static void replyCmd_DESCRIBE(char *buffer, const int64_t bufferLen, const int cseq, const char *url);

    static char *line_parser(char *src, char *line);

    void serve_client(int clientfd, const sockaddr_in &cliAddr, int rtpFD, int ssrcNum, const char *sessionID, int timeout, float fps);

    static int64_t push_stream(int sockfd, RTP_Packet &rtpPack, const uint8_t *data, int64_t dataSize, const sockaddr *to, uint32_t timeStampStep);

public:
    explicit RTSP(const char *filename);
    ~RTSP();

    void Start(int ssrcNum, const char *sessionID, int timeout, float fps);
};
