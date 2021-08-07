/*
 * @Author: Wade
 * @Date: 2021-08-01 16:41:10
 * @LastEditors: Wade
 * @LastEditTime: 2021-08-07 11:21:17
 * @Description: file content
 */
#include "H264.h"

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

H264Parser::H264Parser(const char *filename)
{
    this->fd = open(filename, O_RDONLY);
    assert(this->fd >= 0);

    struct stat file_stat;
    fstat(this->fd, &file_stat);

    this->file_size = file_stat.st_size;
    this->ptr_mapped_file_start = this->ptr_mapped_file_cur = reinterpret_cast<uint8_t *>(mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, this->fd, 0));
    this->ptr_mapped_file_end = this->ptr_mapped_file_start + this->file_size;
    assert(this->ptr_mapped_file_cur != MAP_FAILED);
}

H264Parser::~H264Parser()
{
    assert(close(this->fd) == 0);
    assert(munmap(reinterpret_cast<void *>(this->ptr_mapped_file_start), this->file_size) == 0);
    this->ptr_mapped_file_cur = this->ptr_mapped_file_start = this->ptr_mapped_file_end = nullptr;
}

bool H264Parser::is_start_code(const uint8_t *_buffer, const int64_t buffer_len, const uint8_t start_code_type)
{
    switch (start_code_type)
    {
    case 3:
        if (buffer_len < 3)
            break;
        return ((_buffer[0] == 0x00) && (_buffer[1] == 0x00) && (_buffer[2] == 0x01));
    case 4:
        if (buffer_len < 4)
            break;
        return ((_buffer[0] == 0x00) && (_buffer[1] == 0x00) && (_buffer[2] == 0x00) && (_buffer[3] == 0x01));
    default:
        fprintf(stderr, "static H264Parser::is_start_code() failed: start_code_type error\n");
        break;
    }
    return false;
}

const uint8_t *H264Parser::find_next_start_code(const uint8_t *_buffer, const int64_t buffer_len)
{
    for (int64_t i = 0; i < buffer_len - 3; i++)
    {
        if (H264Parser::is_start_code(_buffer, buffer_len - i, 3) || H264Parser::is_start_code(_buffer, buffer_len - i, 4))
            return _buffer;
        ++_buffer;
    }
    // return nullptr represents reaching the end of this video
    return H264Parser::is_start_code(_buffer, 3, 3) ? _buffer : nullptr;
}

std::pair<const uint8_t *, int64_t> H264Parser::get_next_frame()
{
    // assert(this->fd > 0);
    auto remain_bytes = this->ptr_mapped_file_end - this->ptr_mapped_file_cur;
    if (remain_bytes == 0)
        return {nullptr, 0};

    if (!H264Parser::is_start_code(this->ptr_mapped_file_cur, remain_bytes, 4) && !H264Parser::is_start_code(this->ptr_mapped_file_cur, remain_bytes, 3))
    {
        fprintf(stderr, "H264Parser::get_one_frame() failed: H264 file not start with startcode\n");
        return {nullptr, -1};
    }

    const uint8_t *ptr_next_start_code = H264Parser::find_next_start_code(this->ptr_mapped_file_cur + 3, remain_bytes - 3);
    // Reach the end of this H264 video
    if (!ptr_next_start_code)
        return {nullptr, 0};
    const int64_t frame_size = ptr_next_start_code - this->ptr_mapped_file_cur;
    auto ptr_ret = this->ptr_mapped_file_cur;
    this->ptr_mapped_file_cur += frame_size;
    return {ptr_ret, frame_size};
}
