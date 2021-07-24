/*
 * @Author: your name
 * @Date: 2021-06-10 21:21:44
 * @LastEditTime: 2021-06-11 14:37:38
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /rtsp/include/H264.h
 */
#pragma once

#include <cstddef>
#include <cstdint>

#include <sys/types.h>

//constexpr uint8_t NALU_F_MASK = 0x80;
constexpr uint8_t NALU_NRI_MASK = 0x60;
constexpr uint8_t NALU_F_NRI_MASK = 0xe0;
constexpr uint8_t NALU_TYPE_MASK = 0x1F;

constexpr uint8_t FU_S_MASK = 0x80;
constexpr uint8_t FU_E_MASK = 0x40;
constexpr uint8_t SET_FU_A_MASK = 0x1C;

class H264Parser
{
private:
    int fd = -1;

    static uint8_t *findNextStartCode(uint8_t *_buffer, const size_t _bufLen);

public:
    explicit H264Parser(const char *filename);
    ~H264Parser();

    static bool isStartCode(uint8_t *_buffer, size_t _bufLen, uint8_t startCodeType);
    ssize_t getOneFrame(uint8_t *frameBuffer, size_t bufferLen) const;
};
