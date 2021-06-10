#pragma once

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "rtp.h"

//constexpr uint8_t NALU_F_MASK = 0x80;
constexpr uint8_t NALU_NRI_MASK = 0x60;
constexpr uint8_t NALU_TYPE_MASK = 0x1F;

constexpr uint8_t FU_S_MASK = 0x80;
constexpr uint8_t FU_E_MASK = 0x40;
constexpr uint8_t SET_FU_A_MASK = 0x1C;

class H264Parser
{
private:
    int fd = -1;
    static bool isStartCode(const uint8_t *_buffer, size_t _bufLen, uint8_t startCodeType);
    static uint8_t *findNextStartCode(uint8_t *_buffer, size_t _bufLen);

public:
    explicit H264Parser(const char *filename);
    ~H264Parser();

    ssize_t getOneFrame(uint8_t *frameBuffer, size_t bufferLen) const;
    static ssize_t pushStream(int sockfd, /*RTP_Header &rtpHeader*/ RTP_Packet &rtpPack, const uint8_t *data, size_t dataSize, const sockaddr *to, uint32_t timeStampStep);
};
