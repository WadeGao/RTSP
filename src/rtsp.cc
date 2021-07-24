#include "rtsp.h"
#include "rtp.h"
#include "H264.h"

#include <cstdint>

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

RTSP::RTSP(const char *filename) : h264File(filename)
{
}

RTSP::~RTSP()
{
    close(this->servRtcpSockfd);
    close(this->servRtpSockfd);
    close(this->servRtspSockfd);
}

int RTSP::Socket(int domain, int type, int protocol)
{
    int sockfd = socket(domain, type, protocol);
    if (sockfd < 0)
    {
        fprintf(stderr, "RTSP::Socket() failed: %s\n", strerror(errno));
        return sockfd;
    }
    const int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        fprintf(stderr, "setsockopt() failed: %s\n", strerror(errno));
        return -1;
    }
    return sockfd;
}

bool RTSP::Bind(int sockfd, const char *IP, const uint16_t port)
{
    sockaddr_in addr{};
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, IP, &addr.sin_addr);
    if (bind(sockfd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        fprintf(stderr, "bind() failed: %s\n", strerror(errno));
        return false;
    }
    return true;
}

bool RTSP::rtspSockInit(int rtspSockfd, const char *IP, const uint16_t port, const size_t ListenQueue)
{
    if (!RTSP::Bind(rtspSockfd, IP, port))
        return false;

    if (listen(rtspSockfd, ListenQueue) < 0)
    {
        fprintf(stderr, "listen() failed: %s\n", strerror(errno));
        return false;
    }
    return true;
}

void RTSP::Start(const int ssrcNum, const char *sessionID, const int timeout, const float fps)
{
    this->servRtspSockfd = RTSP::Socket(AF_INET, SOCK_STREAM);
    if (!RTSP::rtspSockInit(this->servRtspSockfd, "0.0.0.0", SERVER_RTSP_PORT))
    {
        fprintf(stderr, "failed to create RTSP socket: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    this->servRtpSockfd = RTSP::Socket(AF_INET, SOCK_DGRAM);
    this->servRtcpSockfd = RTSP::Socket(AF_INET, SOCK_DGRAM);

    if (!RTSP::Bind(this->servRtpSockfd, "0.0.0.0", SERVER_RTP_PORT))
    {
        fprintf(stderr, "failed to create RTP socket: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (!RTSP::Bind(this->servRtcpSockfd, "0.0.0.0", SERVER_RTCP_PORT))
    {
        fprintf(stderr, "failed to create RTCP socket: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "rtsp://127.0.0.1:%d\n", SERVER_RTSP_PORT);

    //while (true)
    //{
    sockaddr_in cliAddr{};
    bzero(&cliAddr, sizeof(cliAddr));
    socklen_t addrLen = sizeof(cliAddr);
    auto cli_sockfd = accept(this->servRtspSockfd, reinterpret_cast<sockaddr *>(&cliAddr), &addrLen);
    if (cli_sockfd < 0)
    {
        fprintf(stderr, "accept error(): %s\n", strerror(errno));
        //continue;
        return;
    }
    char IPv4[16]{0};
    fprintf(stdout,
            "Connection from %s:%d\n",
            inet_ntop(AF_INET, &cliAddr.sin_addr, IPv4, sizeof(IPv4)),
            ntohs(cliAddr.sin_port));
    this->serveClient(cli_sockfd, cliAddr, this->servRtpSockfd, ssrcNum, sessionID, timeout, fps);
    //}
}

char *RTSP::lineParser(char *src, char *line)
{
    while (*src != '\n')
        *(line++) = *(src++);

    *line = '\n';
    *(++line) = 0;
    return (src + 1);
}

void RTSP::serveClient(int clientfd, const sockaddr_in &cliAddr, int rtpFD, const int ssrcNum, const char *sessionID, const int timeout, const float fps)
{
    char method[10]{0};
    char url[100]{0};
    char version[10]{0};
    char line[500]{0};
    int cseq;
    size_t heartbeatCount = 0;
    char recvBuf[1024]{0}, sendBuf[1024]{0};
    while (true)
    {
        auto recvLen = recv(clientfd, recvBuf, sizeof(recvBuf), 0);
        if (recvLen <= 0)
            break;
        recvBuf[recvLen] = 0;
        fprintf(stdout, "--------------- [C->S] --------------\n");
        fprintf(stdout, "%s", recvBuf);

        char *bufferPtr = RTSP::lineParser(recvBuf, line);
        /* 解析方法 */
        if (sscanf(line, "%s %s %s\r\n", method, url, version) != 3)
        {
            fprintf(stdout, "RTSP::lineParser() parse method error\n");
            break;
        }
        /* 解析序列号 */
        bufferPtr = RTSP::lineParser(bufferPtr, line);
        if (sscanf(line, "CSeq: %d\r\n", &cseq) != 1)
        {
            fprintf(stdout, "RTSP::lineParser() parse seq error\n");
            break;
        }
        /* 如果是SETUP，那么就再解析client_port */
        if (!strcmp(method, "SETUP"))
        {
            while (true)
            {
                bufferPtr = RTSP::lineParser(bufferPtr, line);
                if (!strncmp(line, "Transport:", strlen("Transport:")))
                {
                    sscanf(line, "Transport: RTP/AVP;unicast;client_port=%d-%d\r\n", &this->cliRtpPort, &this->cliRtcpPort);
                    break;
                }
            }
        }

        if (!strcmp(method, "OPTIONS"))
            RTSP::replyCmd_OPTIONS(sendBuf, sizeof(sendBuf), cseq);
        else if (!strcmp(method, "DESCRIBE"))
            RTSP::replyCmd_DESCRIBE(sendBuf, sizeof(sendBuf), cseq, url);
        else if (!strcmp(method, "SETUP"))
            RTSP::replyCmd_SETUP(sendBuf, sizeof(sendBuf), cseq, this->cliRtpPort, ssrcNum, sessionID, timeout);
        else if (!strcmp(method, "PLAY"))
            RTSP::replyCmd_PLAY(sendBuf, sizeof(sendBuf), cseq, sessionID, timeout);
        else
        {
            fprintf(stderr, "Parse method error\n");
            break;
        }

        fprintf(stdout, "--------------- [S->C] --------------\n");
        fprintf(stdout, "%s", sendBuf);
        if (send(clientfd, sendBuf, strlen(sendBuf), 0) < 0)
        {
            fprintf(stderr, "RTSP::serveClient() send() failed: %s\n", strerror(errno));
            break;
        }

        if (!strcmp(method, "PLAY"))
        {
            char IPv4[16]{0};
            inet_ntop(AF_INET, &cliAddr.sin_addr, IPv4, sizeof(IPv4));

            auto frameBuffer = new uint8_t[maxBufferSize]{0};

            struct sockaddr_in clientSock
            {
            };
            bzero(&clientSock, sizeof(sockaddr_in));
            clientSock.sin_family = AF_INET;
            inet_pton(clientSock.sin_family, IPv4, &clientSock.sin_addr);
            clientSock.sin_port = htons(this->cliRtpPort);

            fprintf(stdout, "start send stream to %s:%d\n", IPv4, ntohs(clientSock.sin_port));

            //uint32_t tmpTimeStamp = 0;
            const auto timeStampStep = uint32_t(90000 / fps);
            const auto sleepPeriod = uint32_t(1000 * 1000 / fps);
            RTP_Header rtpHeader(0, 0, ssrcNum);
            RTP_Packet rtpPack{rtpHeader};

            while (true)
            {
                auto frameSize = this->h264File.getOneFrame(frameBuffer, maxBufferSize);

                if (frameSize < 0)
                {
                    fprintf(stderr, "RTSP::serveClient() H264::getOneFrame() failed\n");
                    break;
                }
                else if (!frameSize)
                {
                    fprintf(stdout, "Finish serving the user\n");
                    return;
                }

                const ssize_t startCodeLen = H264Parser::isStartCode(frameBuffer, frameSize, 4) ? 4 : 3;
                frameSize -= startCodeLen;
                RTSP::pushStream(rtpFD, rtpPack, frameBuffer + startCodeLen, frameSize, (sockaddr *)&clientSock, timeStampStep);
                usleep(sleepPeriod);
            }
            delete[] frameBuffer;
            frameBuffer = nullptr;
            break;
        }
    }
    fprintf(stdout, "finish\n");
    close(clientfd);
}

ssize_t RTSP::pushStream(int sockfd, RTP_Packet &rtpPack, const uint8_t *data, const size_t dataSize, const sockaddr *to, const uint32_t timeStampStep)
{
    const uint8_t naluHeader = data[0];
    if (dataSize <= RTP_MAX_DATA_SIZE)
    {
        rtpPack.loadData(data, dataSize);
        auto ret = rtpPack.rtp_sendto(sockfd, dataSize + RTP_HEADER_SIZE, 0, to, timeStampStep);
        if (ret < 0)
            fprintf(stderr, "RTP_Packet::rtp_sendto() failed: %s\n", strerror(errno));
        return ret;
    }

    const size_t packetNum = dataSize / RTP_MAX_DATA_SIZE;
    const size_t remainPacketSize = dataSize % RTP_MAX_DATA_SIZE;
    size_t pos = 1;
    ssize_t sentBytes = 0;
    auto payload = rtpPack.getPayload();
    for (size_t i = 0; i < packetNum; i++)
    {
        rtpPack.loadData(data + pos, RTP_MAX_DATA_SIZE, FU_Size);
        payload[0] = (naluHeader & NALU_F_NRI_MASK) | SET_FU_A_MASK;
        payload[1] = naluHeader & NALU_TYPE_MASK;
        if (!i)
            payload[1] |= FU_S_MASK;
        else if (i == packetNum - 1 && remainPacketSize == 0)
            payload[1] |= FU_E_MASK;

        auto ret = rtpPack.rtp_sendto(sockfd, RTP_MAX_PACKET_LEN, 0, to, timeStampStep);
        if (ret < 0)
        {
            fprintf(stderr, "RTP_Packet::rtp_sendto() failed: %s\n", strerror(errno));
            return -1;
        }
        sentBytes += ret;
        pos += RTP_MAX_DATA_SIZE;
    }
    if (remainPacketSize > 0)
    {
        rtpPack.loadData(data + pos, remainPacketSize, FU_Size);
        payload[0] = (naluHeader & NALU_F_NRI_MASK) | SET_FU_A_MASK;
        payload[1] = (naluHeader & NALU_TYPE_MASK) | FU_E_MASK;
        auto ret = rtpPack.rtp_sendto(sockfd, remainPacketSize + RTP_HEADER_SIZE + FU_Size, 0, to, timeStampStep);
        if (ret < 0)
        {
            fprintf(stderr, "RTP_Packet::rtp_sendto() failed: %s\n", strerror(errno));
            return -1;
        }
        sentBytes += ret;
    }
    return sentBytes;
}

void RTSP::replyCmd_OPTIONS(char *buffer, const size_t bufferLen, const int cseq)
{
    snprintf(buffer, bufferLen, "RTSP/1.0 200 OK\r\nCseq: %d\r\nPublic: OPTIONS, DESCRIBE, SETUP, PLAY\r\n\r\n", cseq);
}

void RTSP::replyCmd_SETUP(char *buffer, const size_t bufferLen, const int cseq, const int clientRTP_Port, const int ssrcNum, const char *sessionID, const int timeout)
{
    snprintf(buffer, bufferLen, "RTSP/1.0 200 OK\r\nCseq: %d\r\nTransport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d;ssrc=%d;mode=play\r\nSession: %s; timeout=%d\r\n\r\n",
             cseq, clientRTP_Port, clientRTP_Port + 1, SERVER_RTP_PORT, SERVER_RTCP_PORT, ssrcNum, sessionID, timeout);
}

void RTSP::replyCmd_PLAY(char *buffer, const size_t bufferLen, const int cseq, const char *sessionID, const int timeout)
{
    snprintf(buffer, bufferLen, "RTSP/1.0 200 OK\r\nCseq: %d\r\nRange: npt=0.000-\r\nSession: %s; timeout=%d\r\n\r\n", cseq, sessionID, timeout);
}

void RTSP::replyCmd_HEARTBEAT(char *buffer, const size_t bufferLen, const int cseq, const char *sessionID)
{
    snprintf(buffer, bufferLen, "RTSP/1.0 200 OK\r\nCseq: %d\r\nRange: npt=0.000-\r\nHeartbeat: %s; \r\n\r\n", cseq, sessionID);
}

void RTSP::replyCmd_DESCRIBE(char *buffer, const size_t bufferLen, const int cseq, const char *url)
{
    char ip[100]{0};
    char sdp[500]{0};
    sscanf(url, "rtsp://%[^:]:", ip);
    snprintf(sdp, sizeof(sdp), "v=0\r\no=- 9%ld 1 IN IP4 %s\r\nt=0 0\r\na=control:*\r\nm=video 0 RTP/AVP 96\r\na=rtpmap:96 H264/90000\r\na=control:track0\r\n", time(nullptr), ip);
    snprintf(buffer, bufferLen, "RTSP/1.0 200 OK\r\nCseq: %d\r\nContent-Base: %s\r\nContent-type: application/sdp\r\nContent-length: %ld\r\n\r\n%s", cseq, url, strlen(sdp), sdp);
}