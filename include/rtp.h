/*
 * @Author: your name
 * @Date: 2021-06-10 21:21:44
 * @LastEditTime: 2021-08-07 11:30:31
 * @LastEditors: Wade
 * @Description: In User Settings Edit
 * @FilePath: /rtsp/include/rtp.h
 */
#pragma once

#include <cstddef>
#include <cstdint>

#include <arpa/inet.h>

constexpr int64_t MAX_UDP_PACKET_SIZE = 65535;
constexpr int64_t RTP_VERSION = 2;
constexpr int64_t RTP_HEADER_SIZE = 12;
constexpr int64_t RTP_PAYLOAD_TYPE_H264 = 96;
constexpr int64_t FU_Size = 2;
constexpr int64_t RTP_MAX_DATA_SIZE = MAX_UDP_PACKET_SIZE - 8 - 20 - RTP_HEADER_SIZE - FU_Size;
constexpr int64_t RTP_MAX_PACKET_LEN = RTP_MAX_DATA_SIZE + RTP_HEADER_SIZE + FU_Size;

#pragma pack(1)
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

    void set_timestamp(const uint32_t _newtimestamp);
    void set_ssrc(const uint32_t SSRC);
    void set_seq(const uint32_t _seq);

    void *get_header() const;
    uint32_t get_timestamp() const;
    uint32_t get_seq() const;
};

class RTP_Packet
{
private:
    RTP_Header header;
    uint8_t RTP_Payload[RTP_MAX_DATA_SIZE + FU_Size]{0};

    uint32_t cached_cur_timestamp = 0;
    uint16_t cached_cur_seq = 0;

public:
    explicit RTP_Packet(const RTP_Header &rtpHeader);

    RTP_Packet(const RTP_Packet &) = default;
    ~RTP_Packet() = default;

    void load_data(const uint8_t *data, int64_t dataSize, int64_t bias = 0);
    int64_t rtp_sendto(int sockfd, int64_t _bufferLen, int flags, const sockaddr *to, uint32_t timeStampStep);

    void set_header_seq(const uint32_t _seq);
    void set_header_timestamp(const uint32_t _newtimestamp);

    uint8_t *get_payload() { return this->RTP_Payload; }
    uint32_t get_header_seq();
    uint32_t get_header_timestamp();
};

#pragma pack()
