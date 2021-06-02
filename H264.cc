#include "H264.h"

const char startCode3[3] = {0, 0, 1};
const char startCode4[4] = {0, 0, 0, 1};

H264Parser::H264Parser(const char *filename)
{
    this->fd = open(filename, O_RDONLY);
    assert(this->fd > 0);
    struct stat fileMetaData;
    if (fstat(this->fd, &fileMetaData) < 0)
    {
        fprintf(stderr, "fstat() failed: %s\n", strerror(errno));
        _exit(EXIT_FAILURE);
    }
    this->fileSize = fileMetaData.st_size;
    this->mappedFilePtr = reinterpret_cast<uint8_t *>(mmap(nullptr, this->fileSize, PROT_READ, MAP_SHARED, this->fd, 0));
    if (this->mappedFilePtr == MAP_FAILED)
    {
        fprintf(stderr, "mmap() failed: %s\n", strerror(errno));
        _exit(EXIT_FAILURE);
    }
    this->curFilePtr = this->mappedFilePtr;
}

H264Parser::~H264Parser()
{
    if (munmap(this->mappedFilePtr, this->fileSize) < 0)
    {
        fprintf(stderr, "munmap() failed: %s\n", strerror(errno));
        _exit(EXIT_FAILURE);
    }
    assert(close(this->fd) == 0);
    this->curFilePtr = this->mappedFilePtr = nullptr;
}

ssize_t H264Parser::getOneFrame(uint8_t *frameBuffer, const size_t bufferLen)
{
    assert(this->fd > 0);
    auto isThreeBytesStartCode = [](const uint8_t *buffer, const size_t len) -> bool
    {
        assert(len >= 3);
        return (buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 1);
    };

    if ((htonl(*((uint32_t *)this->curFilePtr)) != 1) && !isThreeBytesStartCode(this->curFilePtr, this->mappedFilePtr + this->fileSize - this->curFilePtr))
        return -1;

    const char *nextStartCode3 = strstr(reinterpret_cast<char *>(this->curFilePtr + 3), startCode3);
    const char *nextStartCode4 = strstr(reinterpret_cast<char *>(this->curFilePtr + 3), startCode4);

    if (!nextStartCode3 && !nextStartCode4)
        return -1;

    const char *startCode = !nextStartCode3 ? nextStartCode4 : !nextStartCode4 ? nextStartCode3
                                                                               : std::min(nextStartCode3, nextStartCode4);
    //const ssize_t frameSize = startCode - reinterpret_cast<char *>(frameBuffer);
    const size_t frameSize = startCode - reinterpret_cast<char *>(this->curFilePtr);
    if (bufferLen < frameSize)
    {
        fprintf(stderr, "H264Parser::getOneFrame() failed: provided buffer can't hold one frame\n");
        return -1;
    }
    memcpy(frameBuffer, this->curFilePtr, frameSize);
    this->curFilePtr += frameSize;
    return frameSize;
}
//12891347918735239572386235001523958259070562300524534000423500013475
ssize_t H264Parser::pushStream(int sockfd, RTP_Header &rtpHeader, const uint8_t *data, const size_t dataSize, const sockaddr *to)
{
    const uint8_t naluHeader = *data;
    ssize_t sentBytes = 0;
    uint16_t seq = rtpHeader.getSeq();

    if (dataSize <= RTP_MAX_PACKET_LEN)
    {
        RTP_Packet rtpPack(rtpHeader, data, dataSize);
        auto ret = rtpPack.rtp_sendto(sockfd, 0, to);
        if (ret < 0)
        {
            fprintf(stderr, "RTP_Packet::rtp_sendto() failed: %s\n", strerror(errno));
            return -1;
        }
        sentBytes += ret;
        seq++;
        return sentBytes;
    }
    const size_t packetNum = dataSize / RTP_MAX_DATA_SIZE;
    const size_t remainPacketSize = dataSize % RTP_MAX_DATA_SIZE;
    size_t pos = 1;
    for (size_t i = 0; i < packetNum; i++)
    {
        auto curHeader = rtpHeader;
        curHeader.setSeq(seq);
        RTP_Packet curPack(curHeader, data + pos, RTP_MAX_DATA_SIZE, FU_Size);
        auto payload = curPack.getPayload();
        payload[0] = (naluHeader & NALU_NRI_MASK) | SET_FU_A_MASK;
        payload[1] = naluHeader & NALU_TYPE_MASK;
        if (!i)
            payload[1] |= FU_S_MASK;
        else if (i == packetNum - 1 && remainPacketSize == 0)
            payload[1] |= FU_E_MASK;

        auto ret = curPack.rtp_sendto(sockfd, 0, to);
        if (ret < 0)
        {
            fprintf(stderr, "RTP_Packet::rtp_sendto() failed: %s\n", strerror(errno));
            return -1;
        }
        seq++;
        sentBytes += ret;
        pos += RTP_MAX_DATA_SIZE;
    }
    if (remainPacketSize > 0)
    {
        auto curHeader = rtpHeader;
        curHeader.setSeq(seq);
        RTP_Packet curPack(curHeader, data + pos, remainPacketSize, FU_Size);
        auto payload = curPack.getPayload();
        payload[0] = (naluHeader & NALU_NRI_MASK) | SET_FU_A_MASK;
        payload[1] = (naluHeader & NALU_TYPE_MASK) | FU_E_MASK;
        auto ret = curPack.rtp_sendto(sockfd, 0, to);
        if (ret < 0)
        {
            fprintf(stderr, "RTP_Packet::rtp_sendto() failed: %s\n", strerror(errno));
            return -1;
        }
        seq++;
        sentBytes += ret;
    }
    //TODO:怎么直接更改序列号
    rtpHeader.setSeq(seq);
    return sentBytes;
}