/*
 * @Author: your name
 * @Date: 2021-06-07 16:46:34
 * @LastEditTime: 2021-08-07 11:30:35
 * @LastEditors: Wade
 * @Description: In User Settings Edit
 * @FilePath: /rtsp/src/rtp.cc
 */
#include "rtp.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <arpa/inet.h>
#include <netinet/in.h>

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

inline void RTP_Header::set_timestamp(const uint32_t _newtimestamp) { this->timestamp = htonl(_newtimestamp); }
inline void RTP_Header::set_ssrc(const uint32_t SSRC) { this->ssrc = htonl(SSRC); }
inline void RTP_Header::set_seq(const uint32_t _seq) { this->seq = htons(_seq); }

inline void *RTP_Header::get_header() const { return (void *)this; }
inline uint32_t RTP_Header::get_timestamp() const { return ntohl(this->timestamp); }
inline uint32_t RTP_Header::get_seq() const { return ntohs(this->seq); }

RTP_Packet::RTP_Packet(const RTP_Header &rtpHeader) : header(rtpHeader)
{
    this->cached_cur_seq = rtpHeader.get_seq();
    this->cached_cur_timestamp = rtpHeader.get_timestamp();
}

void RTP_Packet::load_data(const uint8_t *data, const int64_t dataSize, const int64_t bias)
{
    //TODO:就是这里，气死我了，取min的，原先是RTP_MAX_DATA_SIZE，但是数组的大小是RTP_MAX_DATA_SIZE + FU_Size
    //     所以就会发生数据截断咯，直接用sizeof(this->RTP_Payload)不香吗???
    memcpy(this->RTP_Payload + bias, data, std::min(dataSize, static_cast<int64_t>(sizeof(this->RTP_Payload) - bias)));
}

int64_t RTP_Packet::rtp_sendto(int sockfd, const int64_t _bufferLen, const int flags, const sockaddr *to, const uint32_t timeStampStep)
{
    auto sentBytes = sendto(sockfd, this, _bufferLen, flags, to, sizeof(sockaddr));
    this->set_header_seq(this->get_header_seq() + 1);
    this->set_header_timestamp(this->get_header_timestamp() + timeStampStep);
    return sentBytes;
}

inline void RTP_Packet::set_header_seq(const uint32_t _seq)
{
    this->header.set_seq(_seq);
    this->cached_cur_seq = _seq;
}

inline void RTP_Packet::set_header_timestamp(const uint32_t _newtimestamp)
{
    this->header.set_timestamp(_newtimestamp);
    this->cached_cur_timestamp = _newtimestamp;
}

//inline uint8_t *RTP_Packet::get_payload() { return reinterpret_cast<uint8_t *>(this->RTP_Payload); }
inline uint32_t RTP_Packet::get_header_seq() { return this->cached_cur_seq; }
inline uint32_t RTP_Packet::get_header_timestamp() { return this->cached_cur_timestamp; }
