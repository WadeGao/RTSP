#pragma once

#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdint>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

constexpr size_t RTP_VERSION = 2;
constexpr size_t RTP_HEADER_SIZE = 12;
constexpr size_t RTP_PAYLOAD_TYPE_H264 = 96;
constexpr size_t FU_Size = 2;
constexpr size_t RTP_MAX_DATA_SIZE = 576 - 8 - 20 - RTP_HEADER_SIZE - FU_Size;
constexpr size_t RTP_MAX_PACKET_LEN = RTP_MAX_DATA_SIZE + RTP_HEADER_SIZE + FU_Size;

class RTP_Header
{
private:
    uint8_t header[12];

public:
    RTP_Header(
        //byte 0
        const uint8_t __version,
        const uint8_t __padding,
        const uint8_t __extension,
        const uint8_t __csrcCount,
        //byte 1
        const uint8_t __marker,
        const uint8_t __payloadType,
        //byte 2, 3
        const uint16_t __seq,
        //byte 4-7
        const uint32_t __timestamp,
        //byte 8-11
        const uint32_t __ssrc);
    RTP_Header(const uint16_t __seq, const uint32_t __timestamp, const uint32_t __ssrc);

    RTP_Header(const RTP_Header &) = default;
    ~RTP_Header() = default;

    const uint8_t *getHeader() const { return this->header; }

    void setTimestamp(const uint32_t __newtimestamp)
    {
        uint32_t *pTimeStamp = reinterpret_cast<uint32_t *>(&this->header[4]);
        *pTimeStamp = htonl(__newtimestamp);
    }
    const uint32_t getTimestamp()
    {
        uint32_t *pTimeStamp = reinterpret_cast<uint32_t *>(&this->header[4]);
        return ntohl(*pTimeStamp);
    }

    void setSeq(const uint32_t __seq)
    {
        uint16_t *pSeq = reinterpret_cast<uint16_t *>(&this->header[2]);
        *pSeq = htons(__seq);
    }
    const uint32_t getSeq()
    {
        uint16_t *pSeq = reinterpret_cast<uint16_t *>(&this->header[2]);
        return ntohs(*pSeq);
    }

    void setSSRC(const uint32_t __SSRC)
    {
        uint32_t *pTimeStamp = reinterpret_cast<uint32_t *>(&this->header[8]);
        *pTimeStamp = htonl(__SSRC);
    }

    const uint32_t getSSRC()
    {
        uint32_t *pTimeStamp = reinterpret_cast<uint32_t *>(&this->header[8]);
        return ntohl(*pTimeStamp);
    }
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

    ssize_t rtp_sendto(int sockfd, /*const RTP_Packet &rtpPacket,*/ int flags, const sockaddr *to);

    void setHeadertSeq(const uint32_t __seq) { this->header.setSeq(__seq); }

    const uint32_t getHeaderSeq() { return this->header.getSeq(); }
    //ssize_t rtp_recvfrom(int sockfd, void *buffer, const size_t len, int flags, sockaddr *from);
};

/*class RTP
{
private:
public:
    sockaddr_in addr;
    RTP(const sa_family_t __af, const char *__IP, const uint16_t __port) = default;
    ~RTP() = default;
    ssize_t rtp_sendto(int sockfd, const RTP_Packet &rtpPacket, int flags, const sockaddr *to);
    ssize_t rtp_recvfrom(int sockfd, void *buffer, const size_t len, int flags, sockaddr *from);
};*/