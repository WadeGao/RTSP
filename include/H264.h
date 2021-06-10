/*
 * @Author: your name
 * @Date: 2021-06-07 16:46:34
 * @LastEditTime: 2021-06-10 10:55:39
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /rtsp/include/H264.h
 */
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

constexpr uint8_t NALU_F_MASK = 0x80;
constexpr uint8_t NALU_NRI_MASK = 0x60;
constexpr uint8_t NALU_TYPE_MASK = 0x1F;

constexpr uint8_t FU_S_MASK = 0x80;
constexpr uint8_t FU_E_MASK = 0x40;
constexpr uint8_t SET_FU_A_MASK = 0x1C;

class H264Parser
{
private:
    int fd = -1;
    /*uint8_t *mappedFilePtr = nullptr, *curFilePtr = nullptr;
    size_t fileSize = 0;*/

public:
    H264Parser(const char *filename);
    ~H264Parser();

    ssize_t getOneFrame(uint8_t *frameBuffer, const size_t bufferLen);
    ssize_t pushStream(int sockfd, /*RTP_Header &rtpHeader*/ RTP_Packet &rtpPack, const uint8_t *data, const size_t dataSize, const sockaddr *to, const uint32_t timeStampStep);
};
