/*
 * @Author: your name
 * @Date: 2021-06-07 16:46:34
 * @LastEditTime: 2021-06-09 16:11:20
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /rtsp/include/rtp.h
 */
#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

constexpr size_t RTP_VERSION = 2;
constexpr size_t RTP_HEADER_SIZE = 12;
constexpr size_t RTP_PAYLOAD_TYPE_H264 = 96;
constexpr size_t FU_Size = 2;
constexpr size_t RTP_MAX_DATA_SIZE = 1500 - 8 - 20 - RTP_HEADER_SIZE - FU_Size;
constexpr size_t RTP_MAX_PACKET_LEN = RTP_MAX_DATA_SIZE + RTP_HEADER_SIZE + FU_Size;

class RTP_Header
{
private:
    //byte 0
    uint8_t csrcCount : 4;
    uint8_t extension : 1;
    uint8_t padding : 1;
    uint8_t version : 2;

    //byte 1
    uint8_t payloadType : 7;
    uint8_t marker : 1;
    //byte 2, 3
    uint16_t seq;
    //byte 4-7
    uint32_t timestamp;
    //byte 8-11
    uint32_t ssrc;

public:
    RTP_Header(
        //byte 0
        uint8_t _version,
        uint8_t _padding,
        uint8_t _extension,
        uint8_t _csrcCount,
        //byte 1
        uint8_t _marker,
        uint8_t _payloadType,
        //byte 2, 3
        uint16_t _seq,
        //byte 4-7
        uint32_t _timestamp,
        //byte 8-11
        uint32_t _ssrc);

    RTP_Header(uint16_t _seq, uint32_t _timestamp, uint32_t _ssrc);

    RTP_Header(const RTP_Header &) = default;
    ~RTP_Header() = default;

    //const uint8_t *getHeader() const { return this->header; }
    void *getHeader() const { return (void *)this; }

    void setTimestamp(const uint32_t _newtimestamp) { this->timestamp = htonl(_newtimestamp); }
    uint32_t getTimestamp() const { return ntohl(this->timestamp); }

    void setSeq(const uint32_t _seq) { this->seq = htons(_seq); }
    uint32_t getSeq() const { return ntohs(this->seq); }

    void setSSRC(const uint32_t _SSRC) { this->ssrc = htonl(_SSRC); }
    uint32_t getSSRC() const { return ntohl(this->ssrc); }
};

class RTP_Packet
{
private:
    RTP_Header header;
    size_t packetLen;
    uint8_t RTP_Payload[RTP_MAX_PACKET_LEN]{0};

public:
    RTP_Packet(const RTP_Header &rtpHeader);
    RTP_Packet(const RTP_Header &rtpHeader, const uint8_t *data, const size_t dataSize, const size_t bias = 0);

    //void writeData(const uint8_t *data, const size_t dataSize, const size_t bias = 0);

    RTP_Packet(const RTP_Packet &) = default;
    ~RTP_Packet() = default;

    const uint8_t *getRealPacket() const { return this->RTP_Payload; }
    const size_t getPacketLen() const { return this->packetLen; }
    uint8_t *getPayload() { return this->RTP_Payload + 12; }

    ssize_t rtp_sendto(int sockfd, int flags, const sockaddr *to);

    void setHeadertSeq(const uint32_t _seq) { this->header.setSeq(_seq); }

    const uint32_t getHeaderSeq() { return this->header.getSeq(); }
};