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

    auto mapPtr = reinterpret_cast<uint8_t *>(mmap(nullptr, fileMetaData.st_size, PROT_READ, MAP_SHARED, h264FileDescriptor, 0));
    if (mapPtr == MAP_FAILED)
    {
        fprintf(stderr, "mmap() failed: %s\n", strerror(errno));
        _exit(EXIT_FAILURE);
    }

    /*for (int i = 0; i < 32; i++)
        printf("0x%X ", *(mapPtr + i));
    printf("\n");*/

    this->fileSize = fileMetaData.st_size;
    this->fd = h264FileDescriptor;
    this->curFilePtr = this->mappedFilePtr = mapPtr;
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
    auto isThreeBytesStartCode = [](const uint8_t *const buffer, const size_t len) -> bool
    {
        assert(len >= 3);
        return ((buffer[0] == 0x00) && (buffer[1] == 0x00) && (buffer[2] == 0x01));
    };

    if ((ntohl(*((uint32_t *)this->curFilePtr)) != 1) && !isThreeBytesStartCode(this->curFilePtr, this->mappedFilePtr + this->fileSize - this->curFilePtr))
    {
        fprintf(stderr, "H264Parser::getOneFrame() failed: H264 file not start with startcode\n");
        return -1;
    }

    const auto nextStartCode3 = std::search(this->curFilePtr + 3, this->mappedFilePtr + this->fileSize, startCode3, startCode3 + sizeof(startCode3));
    const auto nextStartCode4 = std::search(this->curFilePtr + 3, this->mappedFilePtr + this->fileSize, startCode4, startCode4 + sizeof(startCode4));

    if (nextStartCode3 - this->mappedFilePtr == this->fileSize && nextStartCode4 == nextStartCode3)
    {
        fprintf(stderr, "H264Parser::getOneFrame() failed: startCode not find\n");
        return -1;
    }

    const size_t frameSize = std::min(nextStartCode3, nextStartCode4) - this->curFilePtr;

    if (bufferLen < frameSize)
    {
        fprintf(stderr, "H264Parser::getOneFrame() failed: provided buffer can't hold one frame\n");
        return -1;
    }
    fprintf(stdout, "current Frame Size: %ld\n", frameSize);
    memcpy(frameBuffer, this->curFilePtr, frameSize);

    /*uint8_t *pos = this->curFilePtr;
    for (size_t i = 0; i < frameSize; i++)
        fprintf(stdout, "0x%X ", *pos++);
    printf("\n");*/

    this->curFilePtr += frameSize;

    return frameSize;
}

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
        rtpHeader.setSeq(++seq);
        return sentBytes;
    }
    const size_t packetNum = dataSize / RTP_MAX_DATA_SIZE;
    const size_t remainPacketSize = dataSize % RTP_MAX_DATA_SIZE;
    size_t pos = 1;
    for (size_t i = 0; i < packetNum; i++)
    {
        rtpHeader.setSeq(seq++);
        RTP_Packet curPack(rtpHeader, data + pos, RTP_MAX_DATA_SIZE, FU_Size);

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
        //seq++;
        sentBytes += ret;
        pos += RTP_MAX_DATA_SIZE;
    }
    if (remainPacketSize > 0)
    {
        rtpHeader.setSeq(seq++);
        RTP_Packet curPack(rtpHeader, data + pos, remainPacketSize, FU_Size);

        auto payload = curPack.getPayload();
        payload[0] = (naluHeader & NALU_NRI_MASK) | SET_FU_A_MASK;
        payload[1] = (naluHeader & NALU_TYPE_MASK) | FU_E_MASK;
        auto ret = curPack.rtp_sendto(sockfd, 0, to);
        if (ret < 0)
        {
            fprintf(stderr, "RTP_Packet::rtp_sendto() failed: %s\n", strerror(errno));
            return -1;
        }
        sentBytes += ret;
    }
    return sentBytes;
}