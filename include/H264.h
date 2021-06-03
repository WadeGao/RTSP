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

template <class ForwardIterator1, class ForwardIterator2>
ForwardIterator1 search(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2, ForwardIterator2 last2)
{
    if (first2 == last2)
        return first1;
    while (first1 != last1)
    {
        ForwardIterator1 it1 = first1;
        ForwardIterator2 it2 = first2;
        while (*it1 == *it2)
        { // 或者 while (pred(*it1,*it2)) 对应第二种语法格式
            if (it2 == last2)
                return first1;
            if (it1 == last1)
                return last1;
            ++it1;
            ++it2;
        }
        ++first1;
    }
    return last1;
}