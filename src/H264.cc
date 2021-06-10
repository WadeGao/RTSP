#include "H264.h"
#include <algorithm>

const uint8_t startCode3[3] = {0, 0, 1};
const uint8_t startCode4[4] = {0, 0, 0, 1};

H264Parser::H264Parser(const char *filename)
{
    auto h264FileDescriptor = open(filename, O_RDONLY);
    assert(h264FileDescriptor > 0);
    struct stat fileMetaData;
    if (fstat(h264FileDescriptor, &fileMetaData) < 0)
    {
        fprintf(stderr, "fstat() failed: %s\n", strerror(errno));
        _exit(EXIT_FAILURE);
    }

    //this->fileSize = fileMetaData.st_size;
    this->fd = h264FileDescriptor;
}

H264Parser::~H264Parser()
{

    assert(close(this->fd) == 0);
}

ssize_t H264Parser::getOneFrame(uint8_t *frameBuffer, const size_t bufferLen)
{
    assert(this->fd > 0);
    auto isThreeBytesStartCode = [](const uint8_t *const buffer, const size_t len) -> bool
    {
        assert(len >= 3);
        return ((buffer[0] == 0x00) && (buffer[1] == 0x00) && (buffer[2] == 0x01));
    };
    auto readBytes = read(this->fd, frameBuffer, bufferLen);

    if ((ntohl(*((uint32_t *)frameBuffer)) != 1) && !isThreeBytesStartCode(frameBuffer, readBytes - 3))
    {
        fprintf(stderr, "H264Parser::getOneFrame() failed: H264 file not start with startcode\n");
        return -1;
    }

    const auto nextStartCode3 = std::search(frameBuffer + 3, frameBuffer + readBytes, startCode3, startCode3 + sizeof(startCode3));
    const auto nextStartCode4 = std::search(frameBuffer + 3, frameBuffer + readBytes, startCode4, startCode4 + sizeof(startCode4));

    if (nextStartCode3 - frameBuffer == readBytes && nextStartCode4 == nextStartCode3)
    {
        fprintf(stderr, "H264Parser::getOneFrame() failed: startCode not find\n");
        return -1;
    }

    const size_t frameSize = std::min(nextStartCode3, nextStartCode4) - frameBuffer;

    if (bufferLen < frameSize)
    {
        fprintf(stderr, "H264Parser::getOneFrame() failed: provided buffer can't hold one frame\n");
        return -1;
    }

    lseek(this->fd, frameSize - readBytes, SEEK_CUR);
    return frameSize;
}

ssize_t H264Parser::pushStream(int sockfd, /*RTP_Header &rtpHeader*/ RTP_Packet &rtpPack, const uint8_t *data, const size_t dataSize, const sockaddr *to, const uint32_t timeStampStep)
{
    const uint8_t naluHeader = data[0];
    ssize_t sentBytes = 0;
    //RTP_Packet rtpPack(rtpHeader);
    if (dataSize <= RTP_MAX_PACKET_LEN)
    {
        rtpPack.loadData(data, dataSize);
        auto ret = rtpPack.rtp_sendto(sockfd, dataSize + RTP_HEADER_SIZE, 0, to, timeStampStep);
        if (ret < 0)
        {
            fprintf(stderr, "RTP_Packet::rtp_sendto() failed: %s\n", strerror(errno));
            return -1;
        }
        sentBytes += ret;
        return sentBytes;
    }
    const size_t packetNum = dataSize / RTP_MAX_DATA_SIZE;
    const size_t remainPacketSize = dataSize % RTP_MAX_DATA_SIZE;
    size_t pos = 1;
    auto payload = rtpPack.getPayload();
    for (size_t i = 0; i < packetNum; i++)
    {
        rtpPack.loadData(data + pos, RTP_MAX_DATA_SIZE, FU_Size);
        payload[0] = (naluHeader & NALU_NRI_MASK) | SET_FU_A_MASK;
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
        //seq++;
        sentBytes += ret;
        pos += RTP_MAX_DATA_SIZE;
    }
    if (remainPacketSize > 0)
    {
        rtpPack.loadData(data + pos, remainPacketSize, FU_Size);
        payload[0] = (naluHeader & NALU_NRI_MASK) | SET_FU_A_MASK;
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