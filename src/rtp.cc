/*
 * @Author: your name
 * @Date: 2021-06-07 16:46:34
 * @LastEditTime: 2021-06-10 09:00:03
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /rtsp/src/rtp.cc
 */
#include "rtp.h"

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

RTP_Packet::RTP_Packet(const RTP_Header &rtpHeader) : header(rtpHeader), packetLen(RTP_HEADER_SIZE) { memcpy(this->RTP_Payload, rtpHeader.getHeader(), RTP_HEADER_SIZE); }

RTP_Packet::RTP_Packet(const RTP_Header &rtpHeader, const uint8_t *data, const size_t dataSize, const size_t bias) : header(rtpHeader), packetLen(RTP_HEADER_SIZE + dataSize + bias)
{
    memcpy(this->RTP_Payload, rtpHeader.getHeader(), RTP_HEADER_SIZE);
    memcpy(this->RTP_Payload + RTP_HEADER_SIZE + bias, data, std::min(dataSize, RTP_MAX_DATA_SIZE - bias));
}

ssize_t RTP_Packet::rtp_sendto(int sockfd, int flags, const sockaddr *to)
{
    return sendto(sockfd, this->getRealPacket(), this->getPacketLen(), flags, to, sizeof(sockaddr));
}
