#include "H264.h"
//#include "rtp.h"

#include <cassert>
#include <cstdint>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>

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
