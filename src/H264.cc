#include "H264.h"

H264Parser::H264Parser(const char *filename)
{
    auto h264FileDescriptor = open(filename, O_RDONLY);
    assert(h264FileDescriptor > 0);
    this->fd = h264FileDescriptor;
}

H264Parser::~H264Parser() { assert(close(this->fd) == 0); }

bool H264Parser::isStartCode(uint8_t *_buffer, const size_t _bufLen, const uint8_t startCodeType)
{
    switch (startCodeType)
    {
    case 3:
        assert(_bufLen >= 3);
        return ((_buffer[0] == 0x00) && (_buffer[1] == 0x00) && (_buffer[2] == 0x01));
    case 4:
        assert(_bufLen >= 4);
        return ((_buffer[0] == 0x00) && (_buffer[1] == 0x00) && (_buffer[2] == 0x00) && (_buffer[3] == 0x01));
    default:
        fprintf(stderr, "static H264Parser::isStartCode() failed: startCodeType error\n");
        break;
    }
    return false;
}

uint8_t *H264Parser::findNextStartCode(uint8_t *_buffer, const size_t _bufLen)
{
    for (size_t i = 0; i < _bufLen - 3; i++)
    {
        if (H264Parser::isStartCode(_buffer, _bufLen - i, 3) || H264Parser::isStartCode(_buffer, _bufLen - i, 4))
            return _buffer;
        ++_buffer;
    }
    // return nullptr represents reaching the end of this video
    return H264Parser::isStartCode(_buffer, 3, 3) ? _buffer : nullptr;
}

ssize_t H264Parser::getOneFrame(uint8_t *frameBuffer, const size_t bufferLen) const
{
    assert(this->fd > 0);
    auto readBytes = read(this->fd, frameBuffer, bufferLen);

    if (!H264Parser::isStartCode(frameBuffer, readBytes - 4, 4) && !H264Parser::isStartCode(frameBuffer, readBytes - 3, 3))
    {
        fprintf(stderr, "H264Parser::getOneFrame() failed: H264 file not start with startcode\n");
        return -1;
    }

    const auto nextStartCode = H264Parser::findNextStartCode(frameBuffer + 3, readBytes - 3);
    if (!nextStartCode)
    {
        //fprintf(stderr, "H264Parser::getOneFrame() Finished: Reach the end of this H264 video\n");
        return 0;
    }
    const ssize_t frameSize = nextStartCode - frameBuffer;
    if (bufferLen < frameSize)
    {
        fprintf(stderr, "H264Parser::getOneFrame() failed: provided buffer can't hold one frame\n");
        return -1;
    }

    lseek(this->fd, frameSize - readBytes, SEEK_CUR);
    return frameSize;
}

ssize_t H264Parser::pushStream(int sockfd, RTP_Packet &rtpPack, const uint8_t *data, const size_t dataSize, const sockaddr *to, const uint32_t timeStampStep)
{
    const uint8_t naluHeader = data[0];
    if (dataSize <= RTP_MAX_DATA_SIZE)
    {
        rtpPack.loadData(data, dataSize);
        auto ret = rtpPack.rtp_sendto(sockfd, dataSize + RTP_HEADER_SIZE, 0, to, timeStampStep);
        if (ret < 0)
            fprintf(stderr, "RTP_Packet::rtp_sendto() failed: %s\n", strerror(errno));
        return ret;
    }

    const size_t packetNum = dataSize / RTP_MAX_DATA_SIZE;
    const size_t remainPacketSize = dataSize % RTP_MAX_DATA_SIZE;
    size_t pos = 1;
    ssize_t sentBytes = 0;
    auto payload = rtpPack.getPayload();
    for (size_t i = 0; i < packetNum; i++)
    {
        rtpPack.loadData(data + pos, RTP_MAX_DATA_SIZE, FU_Size);
        payload[0] = (naluHeader & NALU_F_NRI_MASK) | SET_FU_A_MASK;
        payload[1] = naluHeader & NALU_TYPE_MASK;
        if (!i)
            payload[1] |= FU_S_MASK;
        else if (i == packetNum - 1 && remainPacketSize == 0)
            payload[1] |= FU_E_MASK;

        auto ret = rtpPack.rtp_sendto(sockfd, RTP_MAX_PACKET_LEN, 0, to, timeStampStep);
        if (ret < 0)
        {
            fprintf(stderr, "RTP_Packet::rtp_sendto() failed: %s\n", strerror(errno));
            return -1;
        }
        sentBytes += ret;
        pos += RTP_MAX_DATA_SIZE;
    }
    if (remainPacketSize > 0)
    {
        rtpPack.loadData(data + pos, remainPacketSize, FU_Size);
        payload[0] = (naluHeader & NALU_F_NRI_MASK) | SET_FU_A_MASK;
        payload[1] = (naluHeader & NALU_TYPE_MASK) | FU_E_MASK;
        auto ret = rtpPack.rtp_sendto(sockfd, remainPacketSize + RTP_HEADER_SIZE + FU_Size, 0, to, timeStampStep);
        if (ret < 0)
        {
            fprintf(stderr, "RTP_Packet::rtp_sendto() failed: %s\n", strerror(errno));
            return -1;
        }
        sentBytes += ret;
    }
    return sentBytes;
}