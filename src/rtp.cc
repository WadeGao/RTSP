/*
 * @Author: your name
 * @Date: 2021-06-07 16:46:34
 * @LastEditTime: 2021-06-11 13:48:01
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /rtsp/src/rtp.cc
 */
#include "rtp.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <netinet/in.h>
#include <arpa/inet.h>

RTP_Header::RTP_Header(
    //byte 0
    const uint8_t _version,
    const uint8_t _padding,
    const uint8_t _extension,
    const uint8_t _csrcCount,
    //byte 1
    const uint8_t _marker,
    const uint8_t _payloadType,
    //byte 2, 3
    const uint16_t _seq,
    //byte 4-7
    const uint32_t _timestamp,
    //byte 8-11
    const uint32_t _ssrc)
{
    this->version = _version;
    this->padding = _padding;
    this->extension = _extension;
    this->csrcCount = _csrcCount;
    this->marker = _marker;
    this->payloadType = _payloadType;
    this->seq = htons(_seq);
    this->timestamp = htonl(_timestamp);
    this->ssrc = htonl(_ssrc);
}

RTP_Header::RTP_Header(const uint16_t _seq, const uint32_t _timestamp, const uint32_t _ssrc)
{
    this->version = RTP_VERSION;
    this->padding = 0;
    this->extension = 0;
    this->csrcCount = 0;

    this->marker = 0;
    this->payloadType = RTP_PAYLOAD_TYPE_H264;

    this->seq = htons(_seq);
    this->timestamp = htonl(_timestamp);
    this->ssrc = htonl(_ssrc);
}

inline void RTP_Header::setTimeStamp(const uint32_t _newtimestamp) { this->timestamp = htonl(_newtimestamp); }
inline void RTP_Header::setSSRC(const uint32_t SSRC) { this->ssrc = htonl(SSRC); }
inline void RTP_Header::setSeq(const uint32_t _seq) { this->seq = htons(_seq); }

inline void *RTP_Header::getHeader() const { return (void *)this; }
inline uint32_t RTP_Header::getTimeStamp() const { return ntohl(this->timestamp); }
inline uint32_t RTP_Header::getSeq() const { return ntohs(this->seq); }

RTP_Packet::RTP_Packet(const RTP_Header &rtpHeader) : header(rtpHeader)
{
    this->cachedCurSeq = rtpHeader.getSeq();
    this->cachedCurTimeStamp = rtpHeader.getTimeStamp();
}

void RTP_Packet::loadData(const uint8_t *data, const size_t dataSize, const size_t bias)
{
    //TODO:就是这里，气死我了，取min的，原先是RTP_MAX_DATA_SIZE，但是数组的大小是RTP_MAX_DATA_SIZE + FU_Size
    //     所以就会发生数据截断咯，直接用sizeof(this->RTP_Payload)不香吗???
    memcpy(this->RTP_Payload + bias, data, std::min(dataSize, sizeof(this->RTP_Payload) - bias));
}

ssize_t RTP_Packet::rtp_sendto(int sockfd, const size_t _bufferLen, const int flags, const sockaddr *to, const uint32_t timeStampStep)
{
    auto sentBytes = sendto(sockfd, this, _bufferLen, flags, to, sizeof(sockaddr));
    this->setHeadertSeq(this->getHeaderSeq() + 1);
    this->setHeaderTimeStamp(this->getHeaderTimeStamp() + timeStampStep);
    return sentBytes;
}

inline void RTP_Packet::setHeadertSeq(const uint32_t _seq)
{
    this->header.setSeq(_seq);
    this->cachedCurSeq = _seq;
}

inline void RTP_Packet::setHeaderTimeStamp(const uint32_t _newtimestamp)
{
    this->header.setTimeStamp(_newtimestamp);
    this->cachedCurTimeStamp = _newtimestamp;
}

//inline uint8_t *RTP_Packet::getPayload() { return reinterpret_cast<uint8_t *>(this->RTP_Payload); }
inline uint32_t RTP_Packet::getHeaderSeq() { return this->cachedCurSeq; }
inline uint32_t RTP_Packet::getHeaderTimeStamp() { return this->cachedCurTimeStamp; }
