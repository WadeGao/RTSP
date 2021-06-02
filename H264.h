#pragma once

#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <cassert>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "rtp.h"

constexpr uint8_t NALU_F_MASK = 0x80;
constexpr uint8_t NALU_NRI_MASK = 0x60;
constexpr uint8_t NALU_TYPE_MASK = 0x1F;
constexpr uint8_t FU_S_MASK = 0x80;
constexpr uint8_t FU_E_MASK = 0x40;
constexpr uint8_t SET_FU_A_MASK = 28;

class H264Parser
{
private:
    int fd = -1;
    uint8_t *mappedFilePtr = nullptr, *curFilePtr = nullptr;
    size_t fileSize = 0;

public:
    H264Parser(const char *filename);
    ~H264Parser();

    ssize_t getOneFrame(uint8_t *frameBuffer, const size_t bufferLen);
    ssize_t pushStream(int sockfd, RTP_Header &rtpHeader, const uint8_t *data, const size_t dataSize, const sockaddr *to);
};