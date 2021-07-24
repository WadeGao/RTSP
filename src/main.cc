/*
 * @Author: your name
 * @Date: 2021-06-07 16:46:34
 * @LastEditTime: 2021-06-11 13:55:11
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /rtsp/src/main.cc
 */
#include "rtsp.h"

#include <iostream>
#include <cstdlib>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stdout, "usage: %s <file name> <fps>\n", argv[0]);
        return 1;
    }
    RTSP rtspServer(argv[1]);
    rtspServer.Start(19990825, "wadegao", 600, atof(argv[2]));

    return 0;
}
